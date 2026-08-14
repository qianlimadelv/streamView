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

## GitHub Actions Windows 发布

仓库中的 Windows workflow 会在 GitHub runner 上下载 FFmpeg 和 portable Node，
编译 `streamview.exe`，运行 CTest，并把 Web 前端和 Tauri 资源一起打包。

推送版本标签即可创建 GitHub Release：

```bash
git tag v0.1.0
git push origin v0.1.0
```

发布 workflow 会上传 Windows `.msi` 安装包、portable ZIP 和 `SHA256SUMS.txt`。
如果只想验证构建而不创建 Release，可以在 Actions 页面手动运行
`Windows desktop (.msi)` workflow。
