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

## CLI

```bash
./build/apps/streamview-cli/streamview analyze samples/example.h264 --json out.json
```
