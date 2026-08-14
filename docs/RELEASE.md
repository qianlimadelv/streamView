# Release

StreamView Lite is released primarily as a Windows x64 desktop application.
Pushing a tag matching `v*` runs `.github/workflows/release-windows.yml`, which
builds the C++ core, checks the local backend, packages the Tauri installer, and
publishes the installer, portable ZIP, and SHA-256 checksums to GitHub Releases.

Create a release with:

```bash
git tag v0.1.0
git push origin v0.1.0
```

The portable package includes `streamview.exe`, FFmpeg, Node, the Web frontend,
and `run.bat`. The official Lite package does not include optional libde265, so
HEVC block overlays are disabled by default.

Before tagging:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure
node --check apps/streamview-web/server.js
node --check apps/streamview-web/public/app.js
```
