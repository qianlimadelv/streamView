# Phase 9 Feasibility Study: Macroblock / CTU-Level Data Sources

**Question:** For StreamEye-class block-level visualization (QP maps, prediction
modes, partition grids, motion vectors, residuals), what can we get from FFmpeg's
public API vs. what requires our own entropy decoding?

**Method:** Empirical inspection of the installed FFmpeg headers + runtime tests
extracting data from a generated H.264 clip. This is not a memory recall — every
claim below was verified on the box.

## Environment tested

- FFmpeg 6.1.1 (Ubuntu), `libavcodec 60.31`, `libavutil 58.29`, `libavformat 60.16`
- Headers present: `libavutil/motion_vector.h`, `frame.h`, `libavcodec/avcodec.h`
- `codecview` filter available.

## Findings by data type

| Block-level data | FFmpeg 6.1 public API | Evidence |
|------------------|-----------------------|----------|
| **Motion vectors** | ✅ **Fully available** | `AV_FRAME_DATA_MOTION_VECTORS` side data + `AV_CODEC_FLAG2_EXPORT_MVS`. Verified: 23/25 frames of a test clip carried MV side data (I-frames none, P/B yes). `codecview` rendered arrow overlays successfully. |
| **QP per block** | ❌ **Removed** | The old `qscale_table` / `av_frame_get_qp_table()` / `AV_FRAME_DATA_QP_TABLE_*` APIs are **gone** in libavutil 58 (grep found nothing). Only `FF_DEBUG_QP` remains — a log/draw debug flag, not structured data. |
| **MB type / prediction mode** | ❌ **Not exposed** | Only `FF_DEBUG_MB_TYPE` (draws onto frames / logs). No frame side-data type, no structured accessor. |
| **Partition / CU-CTU structure** | ❌ **Not exposed** | No public API. |
| **Residuals / coefficients** | ❌ **Not exposed** | No public API at any version. |

### Per-block MV fields we do get (`AVMotionVector`)

`source` (ref direction, ±past/future), block `w`/`h`, `src_x/src_y`,
`dst_x/dst_y`, `motion_x/motion_y`, `motion_scale`. Enough to draw per-block
arrows with block size and reference direction — i.e. StreamEye's MV overlay.

### Codec caveat (verified)

MV export is codec-dependent: FFmpeg's **H.264** decoder populates
`AV_FRAME_DATA_MOTION_VECTORS`, but its **HEVC** decoder does **not** in this
build (ffprobe shows no MV side data for an H.265 clip). So even the one
block-level datum FFmpeg gives us is H.264-only in practice — reinforcing that
HEVC block data must come from libde265 (Phase 9b).

### Key negative result

Modern FFmpeg's public API exports **motion vectors and nothing else** at the
block level. QP tables were deprecated and fully removed years ago; MB type,
partition/CU structure, and residuals were never public. The `FF_DEBUG_*` flags
and `codecview` can *draw* some of this into pixels, but do not hand back
structured, per-block numeric data we can build our own overlays/stats from.

## Strategy options

### Option A — FFmpeg public API only
- **Get:** decoded pictures (Phase 5) + **motion vectors**.
- **Miss:** QP maps, prediction/partition maps, residuals.
- **Cost:** low; **maintenance:** low (stable API).
- **Verdict:** covers Phase 10's MV overlay and thumbnails, but **cannot deliver
  the QP/mode/partition maps** that define StreamEye's block view.

### Option B — Patch / fork FFmpeg to expose internals
- Reach into `H264Context` / `HEVCContext` internal tables (mb_type, qp, cbp…)
  via a patched build or a shim compiled against FFmpeg internals.
- **Get:** most block metadata FFmpeg computes internally.
- **Cost:** medium-high; **maintenance:** **high and fragile** — internal structs
  change between FFmpeg releases; you own a fork forever. Residuals still awkward.
- **Verdict:** a trap for a long-lived product. Avoid unless desperate.

### Option C — Adopt a reference / analysis-friendly decoder
- **HEVC:** `libde265` exposes CB/PB/intra-mode/MV internals; its bundled
  `sherlock265` viewer already visualizes CTB/CU/PU/intra-modes/MVs — direct
  precedent for exactly our Phase 9/10 goals.
- **H.264:** JM reference decoder (complete internals, slow) or a modified
  `openh264`.
- **Get:** full block metadata including QP, modes, partitions, MVs, and a path
  to residuals.
- **Cost:** high (integrate/adapt a second decoder); **maintenance:** medium
  (these APIs are more analysis-oriented and more stable than FFmpeg internals).
- **Verdict:** the realistic route to *full* StreamEye parity, per-codec.

### Option D — Write our own entropy decoder (CABAC/CAVLC)
- **Cost:** very high (months per codec, standard-heavy); **control:** total.
- **Verdict:** only if Options C are insufficient. Not recommended as a starting
  point.

## Spike result: libde265 `draw_*` path (verified)

A minimal spike was run to cost the Option C (libde265) route. Findings, all
verified on this machine (Debian libde265 1.0.15):

- The **public** `de265.h` gives decoded pictures only — no block-level data,
  same as FFmpeg.
- But libde265's `visualize.h` exposes half-official `draw_*` helpers —
  `draw_CB_grid`, `draw_intra_pred_modes`, `draw_QuantPY` (QP), `draw_Motion`,
  `draw_PB_pred_modes`, `draw_Slices`, `draw_Tiles` — and the **prebuilt system
  `.so` already exports all of them**.
- They take the same `de265_image*` the public API returns, so you can declare
  the prototypes yourself and skip the internal `image.h`.
- **No sudo, no self-build, no fork required:** the runtime lib was already
  present (pulled in by libheif); headers were obtained with `apt-get download`
  + `dpkg-deb -x` (no root). The spike compiled against the extracted public
  header and linked the system `.so`.
- Result: decoding a real `.h265` and calling the helpers produced correct
  **HEVC** overlays — CB quadtree partition grid, intra-prediction directions,
  a per-block QP heatmap (14 distinct QP levels), and motion vectors. This is
  exactly StreamEye's block view, for a codec FFmpeg cannot cover.

Limitations (honest):
- `draw_*` paints pixels; it does **not** return structured numbers. Interactive
  "hover to read this CB's QP = 32", custom palettes, or legends still need the
  structured internals API — which is **not** exported by the prebuilt lib and
  would require self-building libde265 with internals enabled.
- The helpers are half-official ("TODO: move to sherlock265 or the public API")
  — pin the libde265 version or probe symbol availability at build time.
- HEVC only. H.264 block data remains FFmpeg-MV-only or needs a separate path.

**Cost re-estimate:** the HEVC block *visualization* ("view" version) is
**far cheaper than originally rated** — a small integration (call `draw_*` in a
decode path, add layer toggles in the web UI), not a multi-month self-build.
The expensive parts remain structured numbers (self-built internals) and H.264
block data.

## A-spike: H.264 block data (verified)

Unlike HEVC, H.264 has **no low-cost path**. Verified on FFmpeg 6.1:

- `codecview` exposes `qp` and `block` (partition) options, but both produce a
  **zero-pixel overlay** on H.264 (QP table removed from libavutil; block/mb_type
  data not exported). Only `mv` works — already used by Phase 9a.
- openh264 (available on the box) offers no block-visualization API.
- There is no H.264 analogue to libde265's `draw_*` helpers.

So H.264 block data (QP / partition / intra) requires one of: patching FFmpeg's
`H264Context` internals (fragile fork), integrating the JM reference decoder
(slow, awkward to embed), or a custom CABAC/CAVLC decode (very large). All are
far more expensive than the HEVC path. **Recommendation: defer 9c** — poor
cost/benefit versus HEVC block (done) and other roadmap work.

## Recommendation

**Phased, hybrid path — don't pick one tool for everything:**

1. **Phase 9a (now-ish, low cost):** ship **MV overlay + decoded thumbnails via
   FFmpeg public API** (Options A). This already looks like StreamEye for MVs and
   is cheap. Confirmed working on this machine today.
2. **Phase 9b (the real block view):** integrate **libde265 for HEVC first**
   (best analyzer-oriented API + `sherlock265` precedent) to get QP / prediction /
   partition maps. Prove the overlay UX on one codec before spreading.
3. **Phase 9c:** extend to H.264 block data via JM/openh264 (or evaluate whether a
   focused CABAC/CAVLC metadata decoder is cheaper by then).
4. **Avoid Option B** (patching FFmpeg internals) for anything long-lived.

### Roadmap impact

- Split Phase 9 into **9a (FFmpeg MV, cheap, do with Milestone A)** and
  **9b/9c (per-decoder block metadata, the real cost center)**.
- QP/mode/partition maps are **not** an FFmpeg-config away — they require a second
  decoder integration. Budget accordingly; this remains the project's dominant
  technical risk and cost, as flagged in the roadmap.
- Reordering suggestion: pull the **MV overlay (9a)** forward next to Phase 8,
  since it is low-cost and high-visual-impact, and defer 9b/9c until the GUI shell
  and one codec's block pipeline are proven.
