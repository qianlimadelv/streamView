#include "streamview/demux/ffmpeg_h264_demuxer.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

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

void append_start_code(std::vector<std::uint8_t>& out) {
    out.push_back(0x00);
    out.push_back(0x00);
    out.push_back(0x00);
    out.push_back(0x01);
}

bool append_avcc_packet_as_annex_b(std::vector<std::uint8_t>& out, const AVPacket& packet, int length_size) {
    if (length_size <= 0 || length_size > 4) {
        return false;
    }

    int offset = 0;
    while (offset + length_size <= packet.size) {
        std::uint32_t nal_size = 0;
        for (int i = 0; i < length_size; ++i) {
            nal_size = (nal_size << 8) | packet.data[offset + i];
        }
        offset += length_size;

        if (nal_size == 0 || offset + static_cast<int>(nal_size) > packet.size) {
            return false;
        }

        append_start_code(out);
        out.insert(out.end(), packet.data + offset, packet.data + offset + nal_size);
        offset += static_cast<int>(nal_size);
    }

    return offset == packet.size;
}

int avcc_length_size(const AVCodecParameters& codecpar) {
    if (codecpar.extradata == nullptr || codecpar.extradata_size < 5) {
        return 0;
    }
    return (codecpar.extradata[4] & 0x03) + 1;
}

void append_avcc_parameter_sets(std::vector<std::uint8_t>& out, const AVCodecParameters& codecpar) {
    if (codecpar.extradata == nullptr || codecpar.extradata_size < 7) {
        return;
    }

    const auto* data = codecpar.extradata;
    int offset = 5;
    const int sps_count = data[offset++] & 0x1F;
    for (int i = 0; i < sps_count && offset + 2 <= codecpar.extradata_size; ++i) {
        const int size = (data[offset] << 8) | data[offset + 1];
        offset += 2;
        if (size <= 0 || offset + size > codecpar.extradata_size) {
            return;
        }
        append_start_code(out);
        out.insert(out.end(), data + offset, data + offset + size);
        offset += size;
    }

    if (offset >= codecpar.extradata_size) {
        return;
    }
    const int pps_count = data[offset++];
    for (int i = 0; i < pps_count && offset + 2 <= codecpar.extradata_size; ++i) {
        const int size = (data[offset] << 8) | data[offset + 1];
        offset += 2;
        if (size <= 0 || offset + size > codecpar.extradata_size) {
            return;
        }
        append_start_code(out);
        out.insert(out.end(), data + offset, data + offset + size);
        offset += size;
    }
}

} // namespace

H264AnnexBDemuxResult demux_h264_to_annex_b(const std::string& input_path) {
    AVFormatContext* raw_context = nullptr;
    if (avformat_open_input(&raw_context, input_path.c_str(), nullptr, nullptr) < 0) {
        return {Status::io_error("failed to open input with FFmpeg"), {}};
    }
    std::unique_ptr<AVFormatContext, FormatContextDeleter> format_context(raw_context);

    if (avformat_find_stream_info(format_context.get(), nullptr) < 0) {
        return {Status::parse_error("failed to read stream info with FFmpeg"), {}};
    }

    const int stream_index = av_find_best_stream(format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (stream_index < 0) {
        return {Status::unsupported("no video stream found"), {}};
    }

    const AVCodecParameters* codecpar = format_context->streams[stream_index]->codecpar;
    if (codecpar->codec_id != AV_CODEC_ID_H264) {
        return {Status::unsupported("video stream is not H.264"), {}};
    }

    const int length_size = avcc_length_size(*codecpar);
    if (length_size == 0) {
        return {Status::unsupported("H.264 extradata is not AVCC or is missing"), {}};
    }

    std::vector<std::uint8_t> annex_b;
    append_avcc_parameter_sets(annex_b, *codecpar);

    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    if (!packet) {
        return {Status::io_error("failed to allocate FFmpeg packet"), {}};
    }

    while (av_read_frame(format_context.get(), packet.get()) >= 0) {
        if (packet->stream_index == stream_index &&
            !append_avcc_packet_as_annex_b(annex_b, *packet, length_size)) {
            av_packet_unref(packet.get());
            return {Status::parse_error("failed to convert H.264 packet from AVCC to Annex B"), {}};
        }
        av_packet_unref(packet.get());
    }

    return {Status::ok(), std::move(annex_b)};
}

} // namespace streamview::demux
