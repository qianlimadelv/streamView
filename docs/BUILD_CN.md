# 构建说明

StreamView 使用 CMake 和 C++20。核心 CLI 和 parser library 不强制依赖第三方
运行时库。

## Linux

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure
```

可选 FFmpeg demux 支持需要系统安装 FFmpeg 开发包，并能通过 `pkg-config` 找到。

Ubuntu/Debian：

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

可选 FFmpeg 包可通过 Homebrew 安装：

```bash
brew install ffmpeg pkg-config
```

## Windows

使用支持 C++20 的 Visual Studio 生成器：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Windows 上的 FFmpeg demux 支持是可选项；分析 raw H.264/H.265 Annex B 裸流不需要它。
