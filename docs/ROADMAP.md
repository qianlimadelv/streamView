# Roadmap

This roadmap tracks StreamView from its current CLI/parser foundation toward a
StreamEye-class visual bitstream analyzer. Phases 0–4.5 are complete and shown
for context. Phases 5+ are the gap-to-StreamEye plan, grouped into two
milestones:

- **Milestone A — "StreamEye Lite":** open a file, browse the timeline, see
  per-frame type/size/bitrate charts, view decoded thumbnails, and inspect NAL
  syntax in a GUI. Realistic, clear path.
- **Milestone B — "Full StreamEye":** macroblock/CTU-level QP, prediction,
  partition, motion-vector and residual visualization, plus broader codec and
  container coverage. Industry-hard; the true differentiator.

Status legend: ✅ done · 🟡 partial · ⬜ not started.

---

## Phase 0: Project Skeleton ✅

- CMake workspace
- CLI application
- Core and bitstream libraries
- Unit tests
- Project documentation

## Phase 1: H.264 Annex B Scanner ✅

- Detect 3-byte and 4-byte start codes
- Extract NAL unit offsets and payload sizes
- Identify H.264 NAL unit types
- Export JSON
- Malformed input tests

## Phase 2: H.264 Parameter Sets ✅

- Bit reader, emulation prevention removal, Exp-Golomb decoding
- SPS/PPS baseline fields, width/height derivation
- Slice header baseline fields

## Phase 3: H.265 Elementary Streams ✅

- H.265 NAL scanner reuse, VPS/SPS/PPS baseline parsing
- Unified stream model

## Phase 4: Container Input ✅

- FFmpeg demux adapter, MP4 input
- PTS/DTS propagation, AVCC/HVCC handling

## Phase 4.5: Engineering ✅

- Linux/macOS/Windows CI, cross-platform build docs
- Release artifact structure, real-stream regression set

---

# Milestone A — "StreamEye Lite"

> **Note:** Milestone A's GUI is being realized as a **cross-platform web app**
> (`apps/streamview-web`) rather than Qt — the browser gives wider platform
> reach and the C++ core stays the single source of analysis/decoding. A native
> Qt client remains a possible future addition.

## Phase 5: Decode Layer (`sv-decode`) 🟡

Foundation for all visualization: turn packets/frames into displayable images.
Delivered: FFmpeg `libavcodec` adapter, decode-any-frame, RGB thumbnails,
`streamview decode` CLI, motion-vector export, optional-dependency build. Not
yet: frame caching.

Tasks:
- FFmpeg `libavcodec` adapter behind an interface, isolated like `sv-demux`.
- Decode any frame on demand (seek by decode-order index / PTS).
- Emit RGB/YUV thumbnails at configurable size; cache recently decoded frames.
- Map decoded pictures back to the existing `FrameAnalysis` timeline.
- Keep the parser core buildable without `libavcodec` (optional dependency).

Verify:
- `streamview decode <file> --frame N --out thumb.png` produces a correct image.
- Decoded frame count and order match `frames[]` from the parser.
- Builds and passes tests with FFmpeg absent (decode commands report "unavailable").

Difficulty: medium. Risk: FFmpeg version/API drift; keep the adapter thin.

## Phase 6: GUI Prototype (web) 🟡

The first user-facing app: `apps/streamview-web` (zero-dependency Node server +
browser front-end), wrapping the analysis model + Phase 5 decode.

Delivered:
- Open file (Annex B + MP4); in-browser playback via ffmpeg remux + range serving.
- Timeline: frames colored by type (I/P/B), bar height ∝ size, keyframe markers,
  click-to-inspect.
- Frame detail panel: type, size, POC, PTS/DTS, keyframe, NAL count.
- Decoded thumbnail synced to selection + motion-vector overlay (Phase 9a).
- Summary stats (codec, resolution, counts, parse errors).
- Server holds no parsing logic (reuses `streamview` CLI + `sv-decode`).

Not yet:
- Stream tree (container → streams → NAL/parameter sets).
- NAL syntax panel with expandable parsed fields (belongs with Phase 7).

Verified: headless API checks + a headless-Chrome render of a real `.h264`
(timeline ordering matches CLI JSON; MV overlay endpoints in-bounds).

## Phase 7: Full Syntax Coverage 🟡

Match StreamEye's "every syntax element is inspectable" expectation.

Delivered (web UI): a bitstream/parameter-set tree, a per-frame NAL syntax panel
that expands the parsed header + SPS/PPS/VPS/slice fields already produced by
`sv-analysis`, a lazy-loaded byte-level hex view per NAL, SEI message parsing
(payload type + size, listed in the tree), and H.264 SPS VUI parsing
(aspect ratio, colour primaries/transfer/matrix, timing/frame-rate).

Tasks (remaining):
- Complete SPS/PPS/VPS field parsing (VUI, HRD, scaling lists, extensions).
- SEI message parsing (buffering period, pic timing, user data, etc.).
- Reference picture list / reorder / marking fields in slice headers.
- Full POC derivation (not just `pic_order_cnt_lsb` export).
- Multi-slice frame aggregation.
- Hex view ↔ syntax-tree bit-range linkage (highlight bytes for a field).

Verify:
- Golden JSON expands to full field set; add samples per parsed element.
- Selecting a syntax field in the GUI highlights its exact byte/bit range.

Difficulty: medium (breadth, not depth). Risk: standard-conformance edge cases.

## Phase 8: Charts & Analytics ⬜

Turn already-collected data into StreamEye-style graphs.

Tasks:
- Bitrate-over-time and frame-size charts (data already in `FrameAnalysis`).
- GOP structure visualization; per-type size distribution.
- Frame-size / type filters and jump-to-frame from chart.
- Export charts and stats (extend `sv-export`).

Verify:
- Charts match summed `size_bytes` from CLI JSON for the same stream.
- Clicking a chart point selects the corresponding timeline frame.

Difficulty: low-medium. Risk: minimal (data exists).

---

# Milestone B — "Full StreamEye"

## Phase 9: Macroblock / CTU-Level Data 🟡

The core of StreamEye and the hardest part.

Feasibility spike: ✅ **done** — see `docs/PHASE9-FEASIBILITY.md`. Verified on
FFmpeg 6.1: the public API exports **motion vectors only**; QP tables were
removed, and MB type / partition / residuals are not exposed. Decision: hybrid,
split into:
- **9a** ✅ — MV overlay + decoded thumbnails via FFmpeg public API.
- **9b** ✅ (HEVC) — QP / CB partition / intra / motion overlays via libde265's
  `draw_*` helpers (optional dependency; `sv-decode` + `decode --block-layer` +
  web layer selector). These paint pixels, not structured numbers; interactive
  per-block values would need a self-built libde265 internals build.
- **9c** ⬜ — H.264 block data via JM/openh264, or a focused CABAC/CAVLC metadata
  decoder. Avoid patching FFmpeg internals (fragile fork).

Tasks (9b/9c, per chosen decoder):
- Extract per-MB (H.264) / per-CTU+CU (H.265) QP, type, partition, prediction mode.
- Data model + export for block-level maps aligned to decoded picture geometry.
- GUI overlay: QP heatmap, prediction-mode map, partition grid on the thumbnail.

Verify:
- QP overlay for a known encoder output matches expected QP ranges.
- Block grid aligns pixel-accurately with the decoded frame.

Difficulty: very high. Risk: FFmpeg may not expose enough; entropy decoding is
a large, standard-heavy effort. This phase can dominate total project cost.

## Phase 10: Motion Vectors & Residuals ⬜

Tasks:
- Per-block motion vectors, drawn as arrows over the decoded picture.
- Reference-frame indication; forward/backward prediction coloring.
- Residual/coefficient visualization where obtainable.

Verify:
- MV overlay direction/magnitude matches motion in a known test clip.

Difficulty: very high (builds on Phase 9's data source).

## Phase 11: Codec & Container Expansion ⬜

Tasks:
- Containers: MPEG-TS, MKV (via FFmpeg `libavformat`).
- Codecs: incremental VP9 / AV1 / VVC / MPEG-2 parsing + decode.
- Per-codec syntax panels reusing the Milestone-A framework.

Verify: golden regression samples per new codec/container.

Difficulty: high (each codec is a project); prioritize by user demand.

## Phase 12: Conformance & Validation ⬜

Tasks:
- Extend `validate` from basic checks to structured conformance rules.
- Error/warning list linked to offending NAL/frame in the GUI.
- Buffer-model (HRD) and reference-integrity checks.

Verify: curated conformance-failing streams produce the expected diagnostics.

Difficulty: medium-high.

---

## Sequencing Notes

- Phases 5→6→8 deliver a usable tool fastest; do them before 7's long tail.
- Phase 9's feasibility spike gates all of Milestone B — run it early even if
  implementation is deferred, since it determines the whole technical strategy.
- Keep the "parser core independent of Qt/FFmpeg" rule from `ARCHITECTURE.md`
  intact through every phase.
