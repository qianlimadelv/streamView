use std::process::{Child, Command};
use std::sync::Mutex;
use tauri::{Manager, WebviewUrl, WebviewWindowBuilder, WindowEvent};

#[derive(Default)]
struct BackendProcess(Mutex<Option<Child>>);

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .manage(BackendProcess::default())
        .plugin(tauri_plugin_dialog::init())
        .setup(|app| {
            // Start the backend and wait for it to accept connections, then
            // create the window pointing at it — so the first load succeeds
            // instead of showing a blank/error page while the server boots.
            let child = start_backend(app)?;
            app.state::<BackendProcess>().0.lock().unwrap().replace(child);
            WebviewWindowBuilder::new(
                app,
                "main",
                WebviewUrl::External("http://localhost:8799".parse().unwrap()),
            )
            .title("StreamView")
            .inner_size(1400.0, 900.0)
            .build()?;
            Ok(())
        })
        .on_window_event(|window, event| {
            if matches!(event, WindowEvent::CloseRequested { .. }) {
                stop_backend(&window.state::<BackendProcess>());
            }
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}

// Start the Node backend. Packaged (.msi): bundled node + streamview + ffmpeg
// from the resource dir. Dev: system `node` + the repo's server.js.
fn start_backend(app: &tauri::App) -> Result<Child, Box<dyn std::error::Error>> {
    let res = app
        .path()
        .resource_dir()?;
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
    if std::net::TcpStream::connect("127.0.0.1:8799").is_ok() {
        return Err("port 8799 is already in use; close the other StreamView instance and retry".into());
    }
    // Hide the child's console window on Windows (CREATE_NO_WINDOW).
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        cmd.creation_flags(0x08000000);
    }
    let mut child = cmd.spawn()?;
    eprintln!("streamview: backend started");

    // Wait for the backend to accept connections before the window loads it.
    for _ in 0..100 {
        if std::net::TcpStream::connect("127.0.0.1:8799").is_ok() {
            return Ok(child);
        }
        if child.try_wait()?.is_some() {
            return Err("streamview backend exited before it was ready".into());
        }
        std::thread::sleep(std::time::Duration::from_millis(100));
    }
    let _ = child.kill();
    let _ = child.wait();
    Err("timed out waiting for the streamview backend on port 8799".into())
}

fn stop_backend(state: &BackendProcess) {
    if let Some(mut child) = state.0.lock().unwrap().take() {
        let _ = child.kill();
        let _ = child.wait();
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
