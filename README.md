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
- `docs/CLI_CN.md`
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

Exit codes:

- `0`: command succeeded; `errors` found no parse errors.
- `1`: invalid command line usage.
- `2`: input, output, analysis, or inspect lookup failed.
- `3`: `errors` command completed and found parse errors.
