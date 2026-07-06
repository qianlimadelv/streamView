# 桌面外壳(Tauri)——迁移方案

目标:把现有 web 应用(`apps/streamview-web`)用 Tauri 变成跨平台桌面应用
(Windows / macOS / Linux),原样复用现有 HTML/JS 前端和 C++ 核心。

## 为什么 Tauri

- 前端原样复用(系统 webview,无需重写)。
- 体积小(~10MB,Electron ~100MB),内存低。
- 原生文件对话框,每平台一个安装包。
- C++ 核心(`streamview` CLI、`sv-decode`)仍是唯一事实来源。

## 架构

```
Tauri app
├─ webview  → 加载 apps/streamview-web/public(前端不变)
├─ 后端     → 二选一:
│    (A) Node sidecar:打包 server.js + node,前端继续 fetch /api/*
│    (B) Rust:用 Rust 复刻 6 个接口,前端不变(localhost)
├─ 原生文件对话框(替代 /api/browse)
└─ 打包依赖:streamview CLI + ffmpeg(+ 可选 libde265)
```

现在只有三处"只属于 web",需处理:
1. Node `server.js` 编排 → sidecar(A)或 Rust(B)。
2. `/api/browse` 文件选择 → 原生对话框(前端已加回退 shim,见 `app.js` 的
   `openFileDialog()`)。
3. `<video>` 经 ffmpeg 转封装播放 → 保持不变(后端仍转封装),或以后改为 canvas
   逐帧。

## 后端方案

| 方案 | 前端改动 | 工期 | 体积 | 说明 |
|------|---------|------|------|------|
| **A. Node sidecar** | 无 | 低 | +node 运行时 | 最快出 app;用 `pkg`/Node SEA 打包 `server.js`。 |
| **B. Rust 重写** | 无 | 较高 | 最小 | Rust HTTP server(axum/tiny_http)复刻 `/api/*`。 |

推荐:**先 A(sidecar)** 快速出 app,以后可换 B 缩小体积。两者前端都不改。

## 依赖打包(真正的工作量)

app 要独立运行,需打包各平台二进制:
- `streamview` CLI —— 现有 CMake + CI 已能构建。
- `ffmpeg` —— 每平台带静态构建,或 v1 先要求系统 PATH 有 ffmpeg。
- `libde265` —— 可选(HEVC 块级叠加),打包或优雅降级。

Tauri 通过 `tauri.conf.json` 的 `bundle.externalBin` / `resources` 引用它们。

## 目录结构(待新增)

```
apps/streamview-desktop/
├─ src-tauri/
│  ├─ Cargo.toml
│  ├─ tauri.conf.json      # 窗口、bundle、externalBin(sidecar + 依赖)
│  ├─ build.rs
│  └─ src/main.rs          # 启动 sidecar、原生对话框、窗口
├─ package.json            # tauri 依赖 + 构建脚本
└─ (前端从 ../streamview-web/public 提供)
```

## CI(跨平台构建)

三平台都在 GitHub Actions 上构建(没有单机能同时产出 Win+Mac+Linux)。草图:

```yaml
strategy:
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]
steps:
  - uses: actions/checkout@v4
  - # 构建 streamview CLI(现有 CMake)
  - # 按平台准备 ffmpeg(+libde265)
  - uses: dtolnay/rust-toolchain@stable
  - # (方案 A)用 pkg 把 server.js 打成 sidecar 二进制
  - uses: tauri-apps/tauri-action@v0   # 构建 + 打安装包
```

产物:`.msi`/`.exe`(Windows)、`.dmg`/`.app`(macOS)、`.AppImage`/`.deb`(Linux)。

## 本地开发(Linux)

需要 Rust 工具链和 webview 库(需联网 + sudo):

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
sudo apt install libwebkit2gtk-4.1-dev build-essential libssl-dev \
     libgtk-3-dev libayatana-appindicator3-dev librsvg2-dev
cargo install create-tauri-app
```

Windows/macOS 打包无论如何都在 CI 完成。

## 待决策(实施前确认)

1. 后端:**A(Node sidecar)** 求快 vs **B(Rust)** 求小。
2. 是否本地装 Rust 验证 Linux 版,还是全交给 CI。
3. ffmpeg 策略:**打包**静态构建(更大、独立)vs **要求 PATH 有**(v1 更小、需用户自备)。

## 分步计划

1. 前端对话框 shim(已完成)—— Tauri 内用原生对话框,否则回退 web。
2. 用 sidecar(方案 A)搭 `apps/streamview-desktop/src-tauri`。
3. 把 `streamview` CLI 作为资源打包;sidecar 定位它。
4. ffmpeg 策略(打包或 PATH)。
5. GitHub Actions 矩阵 → 各平台安装包。
6. (以后)可选 Rust 后端,去掉 Node 运行时。
