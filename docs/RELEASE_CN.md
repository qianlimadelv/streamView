# 发布说明

StreamView Lite 当前以 Windows x64 桌面版为主要发布产物。GitHub Actions
在推送 `v*` 标签后自动构建并创建 GitHub Release。

## 版本号

CLI、Tauri 和发布标签目前使用同一版本号。CLI 版本来自根目录 CMake
project version：

```bash
streamview --version
```

发布前需要同步修改：

- 根目录 `CMakeLists.txt` 的 `project(VERSION ...)`。
- `apps/streamview-desktop/src-tauri/tauri.conf.json` 的 `version`。
- 发布标签，例如 `v0.1.0`。

## Windows Release 内容

GitHub Release 会包含：

- `StreamView-<version>-windows-x64.msi`：安装版。
- `StreamView-<version>-windows-x64-portable.zip`：免安装版。
- `SHA256SUMS.txt`：所有发布文件的 SHA-256 校验值。

portable ZIP 内含 `streamview.exe`、FFmpeg、Node、Web 前端和 `run.bat`，解压后
双击 `run.bat` 即可启动。

Windows Lite 包默认不包含可选的 libde265，因此 HEVC 块级 overlay 默认关闭；
普通 H.264/H.265 分析、FFmpeg 解码、缩略图和 H.264 运动矢量不受影响。

## 发布前检查

本地至少执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure
./build/apps/streamview-cli/streamview --version
node --check apps/streamview-web/server.js
node --check apps/streamview-web/public/app.js
```

## 创建 GitHub Release

```bash
git tag v0.1.0
git push origin v0.1.0
```

`.github/workflows/release-windows.yml` 会自动执行 C++ 测试、启动 Web 后端健康
检查、Tauri 打包和 GitHub Release 上传。如果同一标签重新运行，已有 Release 的
资产会被覆盖更新。

## 本地安装产物检查

可以用 CMake install 检查 CLI 发布内容：

```bash
cmake --install build --prefix out/install
```
