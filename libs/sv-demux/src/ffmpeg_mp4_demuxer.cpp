#include "streamview/demux/ffmpeg_mp4_demuxer.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/bsf.h>
#include <libavformat/avformat.h>
}

#include <cstddef>
#include <cstdint>
#include <memory>

namespace streamview::demux {
namespace {

struct FormatContextDeleter {
    void operator()(AVFormatContext* context) const {
        if (context != nullptr) {
            avformat_close_input(&context);
        }
    }
};

struct PacketDeleter {
    void operator()(AVPacket* packet) const {
        if (packet != nullptr) {
            av_packet_free(&packet);
        }
    }
};

struct BsfContextDeleter {
    void operator()(AVBSFContext* context) const {
        if (context != nullptr) {
            av_bsf_free(&context);
        }
    }
};

void append_start_code(std::vector<std::uint8_t>& out) {
    out.push_back(0x00);
    out.push_back(0x00);
    out.push_back(0x00);
    out.push_back(0x01);
}

[[nodiscard]] const char* bitstream_filter_name(AVCodecID codec_id) {
    switch (codec_id) {
    case AV_CODEC_ID_H264:
        return "h264_mp4toannexb";
    case AV_CODEC_ID_HEVC:
        return "hevc_mp4toannexb";
    default:
        return nullptr;
    }
}

[[nodiscard]] std::optional<DemuxCodec> codec_for_id(AVCodecID codec_id) {
    switch (codec_id) {
    case AV_CODEC_ID_H264:
        return DemuxCodec::H264;
    case AV_CODEC_ID_HEVC:
        return DemuxCodec::H265;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<std::int64_t> to_optional_timestamp(std::int64_t value) {
    if (value == AV_NOPTS_VALUE) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::optional<std::int64_t> to_optional_duration(std::int64_t value) {
    if (value <= 0) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] Mp4StreamMetadata build_stream_metadata(const AVFormatContext& format_context,
                                                      const AVStream& stream,
                                                      std::size_t stream_index) {
    const auto* codec_descriptor = avcodec_descriptor_get(stream.codecpar->codec_id);
    Mp4StreamMetadata metadata{};
    metadata.container_format_name = format_context.iformat && format_context.iformat->name
                                         ? format_context.iformat->name
                                         : "";
    if (format_context.iformat && format_context.iformat->long_name) {
        metadata.container_format_long_name = format_context.iformat->long_name;
    }
    metadata.stream_codec_name = avcodec_get_name(stream.codecpar->codec_id);
    if (codec_descriptor != nullptr && codec_descriptor->long_name != nullptr) {
        metadata.stream_codec_long_name = codec_descriptor->long_name;
    }
    metadata.stream_index = stream_index;
    metadata.time_base_num = stream.time_base.num;
    metadata.time_base_den = stream.time_base.den;
    metadata.duration_ts = to_optional_duration(stream.duration);
    metadata.start_time_ts = to_optional_timestamp(stream.start_time);
    metadata.width = stream.codecpar->width;
    metadata.height = stream.codecpar->height;
    metadata.bit_rate = stream.codecpar->bit_rate;
    metadata.avg_frame_rate_num = stream.avg_frame_rate.num;
    metadata.avg_frame_rate_den = stream.avg_frame_rate.den;
    metadata.r_frame_rate_num = stream.r_frame_rate.num;
    metadata.r_frame_rate_den = stream.r_frame_rate.den;
    return metadata;
}

void append_packet_timing(std::vector<Mp4PacketTiming>& packets, const AVPacket& packet) {
    Mp4PacketTiming timing{};
    timing.index = packets.size();
    timing.pts = to_optional_timestamp(packet.pts);
    timing.dts = to_optional_timestamp(packet.dts);
    timing.duration = to_optional_duration(packet.duration);
    timing.position = to_optional_timestamp(packet.pos);
    timing.keyframe = (packet.flags & AV_PKT_FLAG_KEY) != 0;
    packets.push_back(std::move(timing));
}

bool drain_bsf(AVBSFContext* context, std::vector<std::uint8_t>& annex_b) {
    std::unique_ptr<AVPacket, PacketDeleter> output(av_packet_alloc());
    if (!output) {
        return false;
    }

    while (true) {
        const auto result = av_bsf_receive_packet(context, output.get());
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
            return true;
        }
        if (result < 0) {
            return false;
        }

        append_start_code(annex_b);
        annex_b.insert(annex_b.end(), output->data, output->data + output->size);
        av_packet_unref(output.get());
    }
}

bool send_and_drain(AVBSFContext* context, AVPacket* packet, std::vector<std::uint8_t>& annex_b) {
    if (av_bsf_send_packet(context, packet) < 0) {
        return false;
    }
    return drain_bsf(context, annex_b);
}

} // namespace

Mp4AnnexBDemuxResult demux_mp4_to_annex_b(const std::string& input_path) {
    AVFormatContext* raw_context = nullptr;
    if (avformat_open_input(&raw_context, input_path.c_str(), nullptr, nullptr) < 0) {
        return {Status::io_error("failed to open input with FFmpeg"), {}, std::nullopt, std::nullopt, {}};
    }
    std::unique_ptr<AVFormatContext, FormatContextDeleter> format_context(raw_context);

    if (avformat_find_stream_info(format_context.get(), nullptr) < 0) {
        return {Status::parse_error("failed to read stream info with FFmpeg"), {}, std::nullopt, std::nullopt, {}};
    }

    const int stream_index = av_find_best_stream(format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (stream_index < 0) {
        return {Status::unsupported("no video stream found"), {}, std::nullopt, std::nullopt, {}};
    }

    const AVStream* stream = format_context->streams[stream_index];
    const AVCodecParameters* codecpar = stream->codecpar;
    const auto codec = codec_for_id(codecpar->codec_id);
    const auto* filter_name = bitstream_filter_name(codecpar->codec_id);
    if (!codec.has_value() || filter_name == nullptr) {
        return {Status::unsupported("video stream is not H.264 or H.265"), {}, std::nullopt, std::nullopt, {}};
    }

    const AVBitStreamFilter* filter = av_bsf_get_by_name(filter_name);
    if (filter == nullptr) {
        return {Status::unsupported("required FFmpeg bitstream filter is unavailable"), {}, std::nullopt, std::nullopt, {}};
    }

    AVBSFContext* raw_bsf = nullptr;
    if (av_bsf_alloc(filter, &raw_bsf) < 0 || raw_bsf == nullptr) {
        return {Status::io_error("failed to allocate FFmpeg bitstream filter context"), {}, std::nullopt, std::nullopt, {}};
    }
    std::unique_ptr<AVBSFContext, BsfContextDeleter> bsf(raw_bsf);

    if (avcodec_parameters_copy(bsf->par_in, codecpar) < 0) {
        return {Status::parse_error("failed to copy codec parameters to bitstream filter"), {}, std::nullopt, std::nullopt, {}};
    }
    bsf->time_base_in = stream->time_base;
    if (av_bsf_init(bsf.get()) < 0) {
        return {Status::parse_error("failed to initialize FFmpeg bitstream filter"), {}, std::nullopt, std::nullopt, {}};
    }

    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    if (!packet) {
        return {Status::io_error("failed to allocate FFmpeg packet"), {}, std::nullopt, std::nullopt, {}};
    }

    std::vector<std::uint8_t> annex_b;
    std::vector<Mp4PacketTiming> packets;
    while (av_read_frame(format_context.get(), packet.get()) >= 0) {
        if (packet->stream_index == stream_index) {
            append_packet_timing(packets, *packet);
            if (!send_and_drain(bsf.get(), packet.get(), annex_b)) {
                av_packet_unref(packet.get());
                return {Status::parse_error("failed to convert MP4 packet to Annex B"), {}, std::nullopt, std::nullopt, {}};
            }
        }
        av_packet_unref(packet.get());
    }

    if (!send_and_drain(bsf.get(), nullptr, annex_b)) {
        return {Status::parse_error("failed to flush FFmpeg bitstream filter"), {}, std::nullopt, std::nullopt, {}};
    }

    return {
        Status::ok(),
        std::move(annex_b),
        codec,
        build_stream_metadata(*format_context, *stream, static_cast<std::size_t>(stream_index)),
        std::move(packets),
    };
}

} // namespace streamview::demux
