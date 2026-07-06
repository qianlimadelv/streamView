# Desktop shell (Tauri) — migration plan

Goal: turn the existing web app (`apps/streamview-web`) into a cross-platform
desktop application (Windows / macOS / Linux) using Tauri, reusing the current
HTML/JS front-end and the C++ core unchanged.

## Why Tauri

- Reuses the existing front-end as-is (system webview, no rewrite).
- Small binaries (~10 MB vs Electron's ~100 MB); low memory.
- Native file dialogs, single installer per platform.
- The C++ core (`streamview` CLI, `sv-decode`) stays the source of truth.

## Architecture

```
Tauri app
├─ webview  → loads apps/streamview-web/public (unchanged front-end)
├─ backend  → one of:
│    (A) Node sidecar: bundle server.js + node, front-end keeps fetching /api/*
│    (B) Rust: reimplement the 6 endpoints, front-end unchanged (localhost)
├─ native file dialog (replaces /api/browse)
└─ bundled deps: streamview CLI + ffmpeg (+ optional libde265)
```

Only three things are "web-only" today and need handling:
1. Node `server.js` orchestration → sidecar (A) or Rust (B).
2. `/api/browse` file picker → native dialog (front-end already has a fallback
   shim, see `openFileDialog()` in `app.js`).
3. `<video>` playback via ffmpeg remux → keep as-is (backend still remuxes), or
   later switch to canvas frame-stepping.

## Backend options

| Option | Front-end change | Effort | Size | Notes |
|--------|------------------|--------|------|-------|
| **A. Node sidecar** | none | low | +node runtime | Fastest path to an app; `pkg`/Node SEA bundles `server.js`. |
| **B. Rust reimpl** | none | higher | smallest | Rust HTTP server (axum/tiny_http) reproduces `/api/*`. |

Recommended: **start with A (sidecar)** to ship an app quickly, optionally move
to B later for a smaller binary. Both keep the front-end untouched.

## Dependency bundling (the real work)

The app must run standalone, so it bundles per-platform binaries:
- `streamview` CLI — already built by the existing CMake + CI.
- `ffmpeg` — ship a static build per platform, or require it on PATH for a v1.
- `libde265` — optional (HEVC block overlays); ship or degrade gracefully.

Tauri references these via `tauri.conf.json` `bundle.externalBin` / `resources`.

## Project layout (to add)

```
apps/streamview-desktop/
├─ src-tauri/
│  ├─ Cargo.toml
│  ├─ tauri.conf.json      # window, bundle, externalBin (sidecar + deps)
│  ├─ build.rs
│  └─ src/main.rs          # spawn sidecar, native dialogs, window
├─ package.json            # tauri deps + build scripts
└─ (front-end served from ../streamview-web/public)
```

## CI (cross-platform builds)

All three platforms are built on GitHub Actions (no single machine can produce
Win+Mac+Linux). Sketch:

```yaml
strategy:
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]
steps:
  - uses: actions/checkout@v4
  - # build streamview CLI (existing CMake)
  - # fetch/stage ffmpeg (+libde265) for the OS
  - uses: dtolnay/rust-toolchain@stable
  - # (option A) pkg-bundle server.js into a sidecar binary
  - uses: tauri-apps/tauri-action@v0   # builds + packages installers
```

Artifacts: `.msi`/`.exe` (Windows), `.dmg`/`.app` (macOS), `.AppImage`/`.deb`
(Linux).

## Local development (Linux)

Requires a Rust toolchain and the webview lib (needs network + sudo):

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
sudo apt install libwebkit2gtk-4.1-dev build-essential libssl-dev \
     libgtk-3-dev libayatana-appindicator3-dev librsvg2-dev
cargo install create-tauri-app
```

Windows/macOS packaging is done in CI regardless.

## Open decisions (confirm before implementing)

1. Backend: **A (Node sidecar)** for speed vs **B (Rust)** for size.
2. Whether to install Rust locally to validate the Linux build, or leave all
   builds to CI.
3. ffmpeg strategy: **bundle** a static build (bigger, standalone) vs **require
   on PATH** (smaller v1, needs user to have ffmpeg).

## Phased plan

1. Front-end dialog shim (done) — native dialog when in Tauri, web fallback else.
2. Scaffold `apps/streamview-desktop/src-tauri` with sidecar (option A).
3. Bundle `streamview` CLI as a resource; wire the sidecar to find it.
4. ffmpeg strategy (bundle or PATH).
5. GitHub Actions matrix → per-OS installers.
6. (later) Optional Rust backend to drop the Node runtime.
