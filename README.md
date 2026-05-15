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
- `docs/ROADMAP_CN.md`
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
If they are missing, the raw H.264 Annex B analyzer still builds.

Ubuntu/Debian:

```bash
sudo apt install libavformat-dev libavcodec-dev libavutil-dev pkg-config
```

## CLI

```bash
./build/apps/streamview-cli/streamview analyze samples/example.h264
./build/apps/streamview-cli/streamview analyze samples/example.h264 --json out.json
./build/apps/streamview-cli/streamview analyze samples/example.h264 --json summary.json --json-mode summary
./build/apps/streamview-cli/streamview analyze samples/example.h264 --json first-nals.json --limit-nals 100
./build/apps/streamview-cli/streamview inspect samples/example.h264 --nal 0
```

Without `--json`, the CLI prints a concise text summary, including parse error
counts. With `--json`, it writes the full NAL/frame/GOP analysis model to the
selected file. Use `--json-mode summary` for large streams when only the top
level summary is needed. Use `--limit-nals` when only the first N NAL details
are needed in full JSON. Use `inspect` to print one NAL's details directly.
