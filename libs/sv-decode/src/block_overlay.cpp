#include "streamview/decode/frame_decoder.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace streamview::decode {

#if defined(STREAMVIEW_HAS_LIBDE265)

extern "C" {
#include <libde265/de265.h>
}

// libde265's draw_* helpers (visualize.h) paint block-level info onto an RGB
// buffer. We declare the prototypes ourselves to avoid pulling in the internal
// image.h that visualize.h includes; the prebuilt system library exports these
// symbols. See docs/PHASE9-FEASIBILITY.md for the spike that validated this.
extern "C" {
void draw_CB_grid(const struct de265_image* img, uint8_t* dst, int stride, uint32_t value, int pixelSize);
void draw_intra_pred_modes(const struct de265_image* img, uint8_t* dst, int stride, uint32_t value, int pixelSize);
void draw_QuantPY(const struct de265_image* img, uint8_t* dst, int stride, int pixelSize);
void draw_Motion(const struct de265_image* img, uint8_t* dst, int stride, int pixelSize);
}

namespace {

// Fill an RGB buffer with the frame's luma as grayscale (the base for overlays).
void fill_gray_base(const struct de265_image* img, int w, int h, std::vector<std::uint8_t>& rgb) {
    int ystride = 0;
    const std::uint8_t* y = de265_get_image_plane(img, 0, &ystride);
    rgb.assign(static_cast<std::size_t>(w) * h * 3, 0);
    if (y == nullptr) {
        return;
    }
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            const std::uint8_t v = y[static_cast<std::size_t>(j) * ystride + i];
            const std::size_t o = (static_cast<std::size_t>(j) * w + i) * 3;
            rgb[o] = rgb[o + 1] = rgb[o + 2] = v;
        }
    }
}

// Nearest-neighbour downscale of a full-resolution RGB buffer to max_dim.
RgbImage downscale(const std::vector<std::uint8_t>& src, int w, int h, int max_dim) {
    int dw = w;
    int dh = h;
    if (max_dim > 0 && (w > max_dim || h > max_dim)) {
        if (w >= h) {
            dw = max_dim;
            dh = std::max(1, static_cast<int>(static_cast<long long>(h) * max_dim / w));
        } else {
            dh = max_dim;
            dw = std::max(1, static_cast<int>(static_cast<long long>(w) * max_dim / h));
        }
    }
    RgbImage out;
    out.width = dw;
    out.height = dh;
    out.rgb.resize(static_cast<std::size_t>(dw) * dh * 3);
    for (int j = 0; j < dh; ++j) {
        const int sj = j * h / dh;
        for (int i = 0; i < dw; ++i) {
            const int si = i * w / dw;
            const std::size_t so = (static_cast<std::size_t>(sj) * w + si) * 3;
            const std::size_t d = (static_cast<std::size_t>(j) * dw + i) * 3;
            out.rgb[d] = src[so];
            out.rgb[d + 1] = src[so + 1];
            out.rgb[d + 2] = src[so + 2];
        }
    }
    return out;
}

struct DecoderDeleter {
    void operator()(de265_decoder_context* ctx) const {
        if (ctx != nullptr) {
            de265_free_decoder(ctx);
        }
    }
};

} // namespace

BlockOverlayResult render_hevc_block_overlay(std::span<const std::uint8_t> hevc_annex_b,
                                             std::size_t frame_index,
                                             BlockLayer layer,
                                             int max_dim) {
    std::unique_ptr<de265_decoder_context, DecoderDeleter> ctx(de265_new_decoder());
    if (!ctx) {
        return {Status::io_error("failed to create libde265 decoder"), std::nullopt};
    }
    de265_push_data(ctx.get(), hevc_annex_b.data(), static_cast<int>(hevc_annex_b.size()), 0, nullptr);
    de265_flush_data(ctx.get());

    std::size_t index = 0;
    while (true) {
        int more = 0;
        const de265_error err = de265_decode(ctx.get(), &more);
        if (err != DE265_OK && err != DE265_ERROR_WAITING_FOR_INPUT_DATA) {
            return {Status::parse_error(de265_get_error_text(err)), std::nullopt};
        }
        const struct de265_image* img = nullptr;
        while ((img = de265_get_next_picture(ctx.get())) != nullptr) {
            if (index == frame_index) {
                const int w = de265_get_image_width(img, 0);
                const int h = de265_get_image_height(img, 0);
                std::vector<std::uint8_t> rgb;
                const int stride = w * 3;
                if (layer == BlockLayer::Qp) {
                    rgb.assign(static_cast<std::size_t>(w) * h * 3, 0);
                    draw_QuantPY(img, rgb.data(), stride, 3);
                } else {
                    fill_gray_base(img, w, h, rgb);
                    if (layer == BlockLayer::Partition) {
                        draw_CB_grid(img, rgb.data(), stride, 0x00ff00, 3);
                    } else if (layer == BlockLayer::IntraPred) {
                        draw_intra_pred_modes(img, rgb.data(), stride, 0xff3030, 3);
                    } else if (layer == BlockLayer::Motion) {
                        draw_Motion(img, rgb.data(), stride, 3);
                    }
                }
                RgbImage out = downscale(rgb, w, h, max_dim);
                de265_release_next_picture(ctx.get());
                return {Status::ok(), std::move(out)};
            }
            ++index;
            de265_release_next_picture(ctx.get());
        }
        if (!more) {
            break;
        }
    }
    return {Status::invalid_argument("frame index is past the end of the stream"), std::nullopt};
}

#else // no libde265

BlockOverlayResult render_hevc_block_overlay(std::span<const std::uint8_t>, std::size_t, BlockLayer, int) {
    return {Status::unsupported("block overlays require libde265 at build time"), std::nullopt};
}

#endif

} // namespace streamview::decode
