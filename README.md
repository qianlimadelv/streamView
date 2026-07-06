# StreamView

StreamView is a cross-platform compressed video bitstream analysis prototype.

The first milestone focuses on H.264/H.265 elementary streams and a CLI-first
analysis pipeline. GUI, container demuxing, decoding, and deeper codec
visualization will be added incrementally.

## Current Scope

- H.264 Annex B NAL unit scanning
- Basic CLI JSON output
- CMake-based C++20 workspace
- Testable core libraries

See `docs/PRD.md`, `docs/ARCHITECTURE.md`, and `docs/CODEC-SCOPE.md` for the
product and engineering boundaries.

中文文档：

- `docs/PRD_CN.md`
- `docs/ARCHITECTURE_CN.md`
- `docs/CODEC-SCOPE_CN.md`
- `docs/BUILD_CN.md`
- `docs/CLI_CN.md`
- `docs/ROADMAP_CN.md`
- `docs/RELEASE_CN.md`
- `AGENTS_CN.md`

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Real Sample Tests

If `/home/zhangyp/code/media` exists, CMake automatically enables local smoke
tests for:

- `video_264_hd.h264`
- `video_265_sd.h265`

For another local media directory:

```bash
cmake -S . -B build -DSTREAMVIEW_REAL_MEDIA_DIR=/path/to/media
ctest --test-dir build --output-on-failure
```

These media files are not part of the repository.

Optional FFmpeg demux support requires FFmpeg development packages
(`libavformat`, `libavcodec`, `libavutil`) discoverable through `pkg-config`.
If they are missing, the raw H.264 Annex B analyzer still builds. When present,
container inputs (`.mp4`, `.mov`, `.m4v`, `.mkv`, `.webm`) are demuxed to Annex B
and decoding (`decode` / the web UI) is enabled.

Ubuntu/Debian:

```bash
sudo apt install libavformat-dev libavcodec-dev libavutil-dev pkg-config
```

Optional HEVC block-level overlays (per-block QP / CB partition / intra
prediction / motion vectors, via `decode --block-layer` and the web UI's layer
selector) require libde265. It is auto-detected through `pkg-config`; if missing,
everything else still builds.

```bash
sudo apt install libde265-dev
```

## CLI

```bash
./build/apps/streamview-cli/streamview analyze samples/example.h264
./build/apps/streamview-cli/streamview --version
./build/apps/streamview-cli/streamview analyze samples/example.h264 --json out.json
./build/apps/streamview-cli/streamview analyze samples/example.h264 --json summary.json --json-mode summary
./build/apps/streamview-cli/streamview analyze samples/example.h264 --json first-nals.json --limit-nals 100
./build/apps/streamview-cli/streamview analyze samples/example.h265 --codec h265 --format json --output -
./build/apps/streamview-cli/streamview analyze samples/example.h264 --format text --output summary.txt
./build/apps/streamview-cli/streamview inspect samples/example.h264 --nal 0
./build/apps/streamview-cli/streamview inspect samples/example.h264 --frame 0
./build/apps/streamview-cli/streamview inspect samples/example.h264 --gop 0
./build/apps/streamview-cli/streamview errors samples/example.h264
./build/apps/streamview-cli/streamview errors samples/example.h264 --json
./build/apps/streamview-cli/streamview validate samples/example.h264
./build/apps/streamview-cli/streamview validate samples/example.h264 --json
./build/apps/streamview-cli/streamview dump samples/example.h264 --nal 0 --format hex
./build/apps/streamview-cli/streamview dump samples/example.h264 --nal 0 --format payload --output nal0.bin
./build/apps/streamview-cli/streamview decode samples/example.h264 --frame 3 --thumb f3.ppm --mv-json f3.json
./build/apps/streamview-cli/streamview decode samples/example.h265 --frame 0 --block-layer qp --block-out qp.ppm
```

Without `--json`, the CLI prints a concise text summary, including parse error
counts. With `--json`, it writes the full NAL/frame/GOP analysis model to the
selected file. Use `--json-mode summary` for large streams when only the top
level summary is needed. Use `--limit-nals` when only the first N NAL details
are needed in full JSON. Use `inspect` to print one NAL/frame/GOP directly.
Use `errors` to list parse failures quickly. Use `validate` to run basic
scriptable stream checks. Use `dump` to export one NAL as hex text, raw payload
bytes, or RBSP bytes.

The newer output options are:

- `--format text|json`: choose stdout/file output format.
- `--output <path|->`: write to a file, or use `-` for stdout.
- `--codec auto|h264|h265`: override codec detection when extension-based auto
  detection is not enough.

`--json <path>` is kept as a compatibility alias for `--format json --output
<path>`.

Use `decode` (requires FFmpeg) to decode one frame to a PPM thumbnail and export
its per-block motion vectors as JSON — the data source behind the web UI's
motion-vector overlay.

## Web UI

A cross-platform browser UI (playback, frame-type/size timeline, per-frame
detail with a decoded thumbnail and motion-vector overlay, a bitstream tree, a
per-frame NAL syntax + hex panel, and — for HEVC with libde265 — a layer selector
for QP / CB partition / intra-prediction / motion overlays) lives in
`apps/streamview-web`. It has zero runtime dependencies and reuses the
`streamview` CLI + `ffmpeg`:

```bash
cmake -S . -B build && cmake --build build   # build the CLI first
cd apps/streamview-web && node server.js      # http://localhost:8787
```

See `apps/streamview-web/README.md` for details.

Exit codes:

- `0`: command succeeded; `errors` found no parse errors.
- `1`: invalid command line usage.
- `2`: input, output, analysis, or inspect lookup failed.
- `3`: `errors` command completed and found parse errors.
