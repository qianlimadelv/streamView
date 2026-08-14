# Build

StreamView uses CMake and C++20. The core CLI and parser libraries have no
required third-party runtime dependencies.

## Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure
```

Optional FFmpeg demux support requires FFmpeg development packages discoverable
through `pkg-config`.

Ubuntu/Debian:

```bash
sudo apt install libavformat-dev libavcodec-dev libavutil-dev pkg-config
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

## macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure
```

Optional FFmpeg packages can be installed with Homebrew:

```bash
brew install ffmpeg pkg-config
```

## Windows

Use the Visual Studio generator available on the current Windows runner, targeting x64:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

FFmpeg demux support on Windows is optional and not required for raw H.264/H.265
Annex B analysis.
