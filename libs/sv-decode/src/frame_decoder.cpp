#include "streamview/decode/frame_decoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/motion_vector.h>
#include <libavutil/pixfmt.h>
}

#include <algorithm>
#include <memory>

namespace streamview::decode {
namespace {

struct FormatContextDeleter {
    void operator()(AVFormatContext* context) const {
        if (context != nullptr) {
            avformat_close_input(&context);
        }
    }
};

struct CodecContextDeleter {
    void operator()(AVCodecContext* context) const {
        if (context != nullptr) {
            avcodec_free_context(&context);
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

struct FrameDeleter {
    void operator()(AVFrame* frame) const {
        if (frame != nullptr) {
            av_frame_free(&frame);
        }
    }
};

char pict_type_char(int type) {
    switch (type) {
    case AV_PICTURE_TYPE_I:
        return 'I';
    case AV_PICTURE_TYPE_P:
        return 'P';
    case AV_PICTURE_TYPE_B:
        return 'B';
    default:
        return '?';
    }
}

bool is_keyframe(const AVFrame& frame) {
#ifdef AV_FRAME_FLAG_KEY
    return (frame.flags & AV_FRAME_FLAG_KEY) != 0;
#else
    return frame.key_frame != 0;
#endif
}

std::optional<std::int64_t> to_optional_timestamp(std::int64_t value) {
    if (value == AV_NOPTS_VALUE) {
        return std::nullopt;
    }
    return value;
}

void extract_motion_vectors(const AVFrame& frame, std::vector<MotionVector>& out) {
    const AVFrameSideData* side = av_frame_get_side_data(&frame, AV_FRAME_DATA_MOTION_VECTORS);
    if (side == nullptr) {
        return;
    }
    const auto* mvs = reinterpret_cast<const AVMotionVector*>(side->data);
    const std::size_t count = side->size / sizeof(AVMotionVector);
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const AVMotionVector& mv = mvs[i];
        out.push_back(MotionVector{
            .source = mv.source,
            .w = mv.w,
            .h = mv.h,
            .src_x = mv.src_x,
            .src_y = mv.src_y,
            .dst_x = mv.dst_x,
            .dst_y = mv.dst_y,
            .motion_x = mv.motion_x,
            .motion_y = mv.motion_y,
            .motion_scale = mv.motion_scale != 0 ? mv.motion_scale : 1,
        });
    }
}

std::uint8_t clamp_byte(int value) {
    return static_cast<std::uint8_t>(std::clamp(value, 0, 255));
}

// Nearest-neighbour downscale + BT.601 YUV->RGB for a preview thumbnail. We do
// this by hand rather than pulling in libswscale, keeping sv-decode's dependency
// footprint identical to sv-demux (libav{format,codec,util} only).
std::optional<RgbImage> build_thumbnail(const AVFrame& frame, int max_dim) {
    const int src_w = frame.width;
    const int src_h = frame.height;
    if (src_w <= 0 || src_h <= 0) {
        return std::nullopt;
    }

    int dst_w = src_w;
    int dst_h = src_h;
    if (max_dim > 0 && (src_w > max_dim || src_h > max_dim)) {
        if (src_w >= src_h) {
            dst_w = max_dim;
            dst_h = std::max(1, static_cast<int>(static_cast<long long>(src_h) * max_dim / src_w));
        } else {
            dst_h = max_dim;
            dst_w = std::max(1, static_cast<int>(static_cast<long long>(src_w) * max_dim / src_h));
        }
    }

    const bool planar_420 = frame.format == AV_PIX_FMT_YUV420P ||
                            frame.format == AV_PIX_FMT_YUVJ420P;

    RgbImage image;
    image.width = dst_w;
    image.height = dst_h;
    image.rgb.resize(static_cast<std::size_t>(dst_w) * dst_h * 3);

    const std::uint8_t* y_plane = frame.data[0];
    const int y_stride = frame.linesize[0];
    if (y_plane == nullptr || y_stride <= 0) {
        return std::nullopt;
    }

    for (int dy = 0; dy < dst_h; ++dy) {
        const int sy = dy * src_h / dst_h;
        for (int dx = 0; dx < dst_w; ++dx) {
            const int sx = dx * src_w / dst_w;
            const int y = y_plane[static_cast<std::size_t>(sy) * y_stride + sx];
            std::uint8_t r;
            std::uint8_t g;
            std::uint8_t b;
            if (planar_420 && frame.data[1] != nullptr && frame.data[2] != nullptr) {
                const int cx = sx / 2;
                const int cy = sy / 2;
                const int u = frame.data[1][static_cast<std::size_t>(cy) * frame.linesize[1] + cx] - 128;
                const int v = frame.data[2][static_cast<std::size_t>(cy) * frame.linesize[2] + cx] - 128;
                r = clamp_byte(y + ((91881 * v) >> 16));
                g = clamp_byte(y - ((22554 * u + 46802 * v) >> 16));
                b = clamp_byte(y + ((116130 * u) >> 16));
            } else {
                r = g = b = static_cast<std::uint8_t>(y); // grayscale fallback
            }
            const std::size_t o = (static_cast<std::size_t>(dy) * dst_w + dx) * 3;
            image.rgb[o] = r;
            image.rgb[o + 1] = g;
            image.rgb[o + 2] = b;
        }
    }
    return image;
}

DecodedFrame build_decoded_frame(const AVFrame& frame, std::size_t decode_index,
                                 const DecodeOptions& options) {
    DecodedFrame decoded;
    decoded.decode_index = decode_index;
    decoded.coded_width = frame.width;
    decoded.coded_height = frame.height;
    decoded.pict_type = pict_type_char(frame.pict_type);
    decoded.keyframe = is_keyframe(frame);
    decoded.pts = to_optional_timestamp(frame.pts);
    decoded.dts = to_optional_timestamp(frame.pkt_dts);
    if (options.want_motion_vectors) {
        extract_motion_vectors(frame, decoded.motion_vectors);
    }
    if (options.want_thumbnail) {
        decoded.thumbnail = build_thumbnail(frame, options.thumbnail_max_dim);
    }
    return decoded;
}

} // namespace

DecodeFrameResult decode_frame(const std::string& input_path, const DecodeOptions& options) {
    AVFormatContext* raw_format = nullptr;
    if (avformat_open_input(&raw_format, input_path.c_str(), nullptr, nullptr) < 0) {
        return {Status::io_error("failed to open input with FFmpeg"), std::nullopt};
    }
    std::unique_ptr<AVFormatContext, FormatContextDeleter> format_context(raw_format);

    if (avformat_find_stream_info(format_context.get(), nullptr) < 0) {
        return {Status::parse_error("failed to read stream info with FFmpeg"), std::nullopt};
    }

    const AVCodec* codec = nullptr;
    const int stream_index =
        av_find_best_stream(format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (stream_index < 0 || codec == nullptr) {
        return {Status::unsupported("no decodable video stream found"), std::nullopt};
    }
    const AVStream* stream = format_context->streams[stream_index];

    std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_context(avcodec_alloc_context3(codec));
    if (!codec_context) {
        return {Status::io_error("failed to allocate FFmpeg codec context"), std::nullopt};
    }
    if (avcodec_parameters_to_context(codec_context.get(), stream->codecpar) < 0) {
        return {Status::parse_error("failed to copy codec parameters"), std::nullopt};
    }
    // Ask the decoder to export per-block motion vectors (Phase 9a data source).
    codec_context->flags2 |= AV_CODEC_FLAG2_EXPORT_MVS;
    if (avcodec_open2(codec_context.get(), codec, nullptr) < 0) {
        return {Status::parse_error("failed to open decoder"), std::nullopt};
    }

    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    std::unique_ptr<AVFrame, FrameDeleter> frame(av_frame_alloc());
    if (!packet || !frame) {
        return {Status::io_error("failed to allocate FFmpeg packet/frame"), std::nullopt};
    }

    std::size_t decode_index = 0;
    bool flushing = false;

    // send/receive loop; drain the decoder at EOF by sending a null packet.
    while (true) {
        if (!flushing) {
            const int read = av_read_frame(format_context.get(), packet.get());
            if (read < 0) {
                flushing = true;
                avcodec_send_packet(codec_context.get(), nullptr);
            } else {
                if (packet->stream_index != stream_index) {
                    av_packet_unref(packet.get());
                    continue;
                }
                if (avcodec_send_packet(codec_context.get(), packet.get()) < 0) {
                    av_packet_unref(packet.get());
                    return {Status::parse_error("failed to send packet to decoder"), std::nullopt};
                }
                av_packet_unref(packet.get());
            }
        }

        while (true) {
            const int recv = avcodec_receive_frame(codec_context.get(), frame.get());
            if (recv == AVERROR(EAGAIN)) {
                break;
            }
            if (recv == AVERROR_EOF) {
                return {Status::invalid_argument("frame index is past the end of the stream"),
                        std::nullopt};
            }
            if (recv < 0) {
                return {Status::parse_error("failed to receive frame from decoder"), std::nullopt};
            }

            if (decode_index == options.frame_index) {
                DecodedFrame decoded = build_decoded_frame(*frame, decode_index, options);
                av_frame_unref(frame.get());
                return {Status::ok(), std::move(decoded)};
            }
            ++decode_index;
            av_frame_unref(frame.get());
        }
    }
}

DecodeRangeResult decode_frames(const std::string& input_path, const DecodeRangeOptions& options) {
    DecodeRangeResult result;
    if (options.count == 0) {
        result.status = Status::ok();
        return result;
    }

    AVFormatContext* raw_format = nullptr;
    if (avformat_open_input(&raw_format, input_path.c_str(), nullptr, nullptr) < 0) {
        result.status = Status::io_error("failed to open input with FFmpeg");
        return result;
    }
    std::unique_ptr<AVFormatContext, FormatContextDeleter> format_context(raw_format);

    if (avformat_find_stream_info(format_context.get(), nullptr) < 0) {
        result.status = Status::parse_error("failed to read stream info with FFmpeg");
        return result;
    }

    const AVCodec* codec = nullptr;
    const int stream_index =
        av_find_best_stream(format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (stream_index < 0 || codec == nullptr) {
        result.status = Status::unsupported("no decodable video stream found");
        return result;
    }

    std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_context(avcodec_alloc_context3(codec));
    if (!codec_context) {
        result.status = Status::io_error("failed to allocate FFmpeg codec context");
        return result;
    }
    if (avcodec_parameters_to_context(codec_context.get(),
                                      format_context->streams[stream_index]->codecpar) < 0) {
        result.status = Status::parse_error("failed to copy codec parameters");
        return result;
    }
    // Thumbnails don't need motion vectors, so skip MV export (cheaper decode).
    if (avcodec_open2(codec_context.get(), codec, nullptr) < 0) {
        result.status = Status::parse_error("failed to open decoder");
        return result;
    }

    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    std::unique_ptr<AVFrame, FrameDeleter> frame(av_frame_alloc());
    if (!packet || !frame) {
        result.status = Status::io_error("failed to allocate FFmpeg packet/frame");
        return result;
    }

    DecodeOptions frame_opts;
    frame_opts.want_thumbnail = true;
    frame_opts.want_motion_vectors = false;
    frame_opts.thumbnail_max_dim = options.thumbnail_max_dim;

    const std::size_t last = options.start_index + options.count; // exclusive
    std::size_t decode_index = 0;
    bool flushing = false;

    while (true) {
        if (!flushing) {
            const int read = av_read_frame(format_context.get(), packet.get());
            if (read < 0) {
                flushing = true;
                avcodec_send_packet(codec_context.get(), nullptr);
            } else {
                if (packet->stream_index != stream_index) {
                    av_packet_unref(packet.get());
                    continue;
                }
                if (avcodec_send_packet(codec_context.get(), packet.get()) < 0) {
                    av_packet_unref(packet.get());
                    result.status = Status::parse_error("failed to send packet to decoder");
                    return result;
                }
                av_packet_unref(packet.get());
            }
        }

        while (true) {
            const int recv = avcodec_receive_frame(codec_context.get(), frame.get());
            if (recv == AVERROR(EAGAIN)) {
                break;
            }
            if (recv == AVERROR_EOF) {
                result.status = Status::ok();
                return result;
            }
            if (recv < 0) {
                result.status = Status::parse_error("failed to receive frame from decoder");
                return result;
            }
            if (decode_index >= options.start_index && decode_index < last) {
                result.frames.push_back(build_decoded_frame(*frame, decode_index, frame_opts));
            }
            av_frame_unref(frame.get());
            if (decode_index + 1 >= last) {
                result.status = Status::ok();
                return result;
            }
            ++decode_index;
        }
    }
}

} // namespace streamview::decode
