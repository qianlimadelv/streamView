# Release

StreamView does not have packaged releases yet. This document defines the
intended artifact layout so packaging work can stay consistent.

## Version

The CLI version comes from the root CMake project version:

```bash
streamview --version
```

## Artifact Layout

Recommended archive names:

- `streamview-<version>-linux-x86_64.tar.gz`
- `streamview-<version>-macos-arm64.tar.gz`
- `streamview-<version>-windows-x86_64.zip`

Recommended contents:

```text
streamview-<version>/
  bin/
    streamview
  docs/
    CLI.md
    BUILD.md
    CODEC-SCOPE.md
  LICENSE
  README.md
```

## Release Gate

Before publishing:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure
./build/apps/streamview-cli/streamview --version
```

## Local Install Staging

Use CMake install with a local prefix to inspect release contents:

```bash
cmake --install build --prefix out/install
```
