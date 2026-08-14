# StreamView Web

A cross-platform browser UI for StreamView: analyze a stream, play it, and
inspect every frame (type, size, POC, PTS/DTS, motion vectors) with a decoded
thumbnail and a motion-vector overlay. It also shows a bitstream/parameter-set
tree and a per-frame NAL syntax panel with expandable parsed fields and a
byte-level hex view. For HEVC (when libde265 is available), a layer selector
overlays per-block QP, CB partition, intra-prediction directions, and motion.

It is a thin presentation layer with **zero runtime dependencies** (Node's
built-in `http`). All bitstream analysis is done by the `streamview` CLI
(`sv-analysis`), all decoding by `sv-decode`, and playback by remuxing to MP4
with `ffmpeg`. No parsing/decoding logic lives in this app.

## Prerequisites

- Node.js >= 16
- The `streamview` CLI built (see the repo root `README.md`):
  `cmake -S . -B build && cmake --build build`
- `ffmpeg` on `PATH` (for in-browser playback of raw elementary streams)

## Run

```bash
cd apps/streamview-web
node server.js            # http://127.0.0.1:8787
```

Then open the URL, paste an absolute path to a `.h264` / `.h265` / `.mp4` file,
and click **Analyze**. You can also deep-link:
`http://localhost:8787/?path=/abs/file.h264&frame=3`.

Environment overrides:

- `PORT` — listen port (default `8787`).
- `HOST` — listen address (default `127.0.0.1`; the backend is intentionally
  local-only because its API reads local media paths).
- `STREAMVIEW_BIN` — path to the `streamview` binary (default
  `../../build/apps/streamview-cli/streamview`).
- `FFMPEG_BIN` — ffmpeg binary (default `ffmpeg`).
- `STREAMVIEW_HEVC_BLOCKS=1` — advertise HEVC block overlays when the CLI was
  built with optional libde265 support.

## HTTP API

- `GET /api/analyze?path=<file>` — full `StreamAnalysis` JSON.
- `GET /api/frame?path=<file>&index=<decodeIndex>&size=<px>` — decoded-frame
  detail: metadata, motion vectors, and a base64 PPM thumbnail.
- `GET /api/block?path=<file>&index=N&layer=qp|partition|intra|motion` — a base64
  PPM of one HEVC block-level overlay (libde265; errors for non-HEVC).
- `GET /api/dump?path=<file>&nal=N&format=hex|payload|rbsp` — raw NAL bytes
  (hex dump for the byte-level view).
- `GET /api/video?path=<file>` — playable MP4 (stream-copied or re-encoded),
  with HTTP range support for seeking.
- `GET /api/health` — sanity check.

The backend caches a few recent analysis results by file size and modification
time. API responses are marked non-cacheable so a changed input is analyzed
again.

## Scope

This is the Milestone-A GUI (see `docs/ROADMAP.md`): playback, timeline,
per-frame detail, the Phase 9a motion-vector overlay, and — via libde265 —
Phase 9b HEVC block overlays (QP / partition / intra / motion). These `draw_*`
overlays paint pixels, not structured numbers; interactive per-block values and
H.264 block data (Phase 9c) are still future work. See
`docs/PHASE9-FEASIBILITY.md`.
