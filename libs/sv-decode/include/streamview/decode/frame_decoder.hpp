#pragma once

#include "streamview/core/status.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace streamview::decode {

// One motion-vector block, mirroring FFmpeg's AVMotionVector. This is the only
// block-level metadata the FFmpeg public API exposes; see
// docs/PHASE9-FEASIBILITY.md for why QP/mode/partition/residual data is not.
struct MotionVector {
    int source{};       // <0 from a past ref, >0 from a future ref
    int w{};            // block width in luma samples
    int h{};            // block height in luma samples
    int src_x{};        // absolute source position
    int src_y{};
    int dst_x{};        // absolute destination position (block center)
    int dst_y{};
    int motion_x{};     // src_x = dst_x + motion_x / motion_scale
    int motion_y{};
    int motion_scale{1};
};

// A decoded thumbnail in packed RGB24, row-major, size == width*height*3.
struct RgbImage {
    int width{};
    int height{};
    std::vector<std::uint8_t> rgb;
};

struct DecodedFrame {
    std::size_t decode_index{};       // 0-based decode order
    int coded_width{};
    int coded_height{};
    char pict_type{'?'};              // 'I' / 'P' / 'B' / '?'
    bool keyframe{};
    std::optional<std::int64_t> pts;
    std::optional<std::int64_t> dts;
    std::vector<MotionVector> motion_vectors;
    std::optional<RgbImage> thumbnail; // present when requested and convertible
};

struct DecodeOptions {
    std::size_t frame_index{};        // which decoded frame (decode order)
    bool want_thumbnail{true};
    int thumbnail_max_dim{320};       // longest side in pixels; 0 = coded size
    bool want_motion_vectors{true};
};

struct DecodeFrameResult {
    streamview::Status status;
    std::optional<DecodedFrame> frame;
};

// Decode a single frame from any FFmpeg-openable input (.h264/.h265/.mp4).
[[nodiscard]] DecodeFrameResult decode_frame(const std::string& input_path,
                                             const DecodeOptions& options);

// Decode a contiguous range of frames in one pass (thumbnails only, no motion
// vectors) — far cheaper than calling decode_frame N times, which re-decodes
// from frame 0 each time. Used to fill the filmstrip.
struct DecodeRangeOptions {
    std::size_t start_index{};
    std::size_t count{1};
    int thumbnail_max_dim{140};       // longest side in pixels
};

struct DecodeRangeResult {
    streamview::Status status;
    std::vector<DecodedFrame> frames; // decode-order, thumbnails populated
};

[[nodiscard]] DecodeRangeResult decode_frames(const std::string& input_path,
                                              const DecodeRangeOptions& options);

// Block-level overlay layers rendered by libde265's draw_* helpers (HEVC only).
enum class BlockLayer {
    Qp,          // per-block quantization parameter heatmap
    Partition,   // CB quadtree partition grid
    IntraPred,   // intra-prediction directions
    Motion,      // motion vectors
};

struct BlockOverlayResult {
    streamview::Status status;
    std::optional<RgbImage> image;
};

// Decode HEVC Annex B and render one block-level layer of the given decode-order
// frame using libde265. Requires libde265 at build time; otherwise returns
// unsupported. See docs/PHASE9-FEASIBILITY.md.
[[nodiscard]] BlockOverlayResult render_hevc_block_overlay(
    std::span<const std::uint8_t> hevc_annex_b,
    std::size_t frame_index,
    BlockLayer layer,
    int max_dim);

} // namespace streamview::decode
