use std::process::Command;
use tauri::Manager;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            start_backend(app);
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

// Start the Node backend. Packaged (.msi): bundled node + streamview + ffmpeg
// from the resource dir. Dev: system `node` + the repo's server.js.
fn start_backend(app: &tauri::App) {
    let res = app
        .path()
        .resource_dir()
        .expect("failed to resolve resource dir");
    let bundled_server = res.join("resources/streamview-web/server.js");

    let mut cmd;
    if bundled_server.exists() {
        cmd = Command::new(res.join(format!("resources/{}", node_bin())));
        cmd.arg(&bundled_server)
            .env("STREAMVIEW_BIN", res.join(format!("resources/{}", streamview_bin())))
            .env("FFMPEG_BIN", res.join(format!("resources/{}", ffmpeg_bin())));
        eprintln!("streamview: using bundled backend at {}", bundled_server.display());
    } else {
        let dev = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("../../streamview-web/server.js");
        cmd = Command::new("node");
        cmd.arg(dev);
        eprintln!("streamview: dev backend (system node)");
    }
    cmd.env("PORT", "8799");
    match cmd.spawn() {
        Ok(_) => eprintln!("streamview: backend started"),
        Err(e) => eprintln!("streamview: failed to start backend: {e}"),
    }

    // Wait for the backend to accept connections before the window loads it.
    for _ in 0..100 {
        if std::net::TcpStream::connect("127.0.0.1:8799").is_ok() {
            break;
        }
        std::thread::sleep(std::time::Duration::from_millis(100));
    }
}

#[cfg(windows)]
fn node_bin() -> &'static str { "node.exe" }
#[cfg(not(windows))]
fn node_bin() -> &'static str { "node" }

#[cfg(windows)]
fn streamview_bin() -> &'static str { "streamview.exe" }
#[cfg(not(windows))]
fn streamview_bin() -> &'static str { "streamview" }

#[cfg(windows)]
fn ffmpeg_bin() -> &'static str { "ffmpeg.exe" }
#[cfg(not(windows))]
fn ffmpeg_bin() -> &'static str { "ffmpeg" }
