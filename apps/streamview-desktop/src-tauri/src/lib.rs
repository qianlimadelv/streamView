use std::process::Command;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .setup(|_app| {
            // Start the Node backend on a dedicated port (avoids clashing with a
            // separately-running web dev server on 8787). v1 (dev): system `node`
            // + the web app's server.js. TODO(packaging): bundled sidecar binary.
            let server = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
                .join("../../streamview-web/server.js");
            match Command::new("node").arg(&server).env("PORT", "8799").spawn() {
                Ok(_) => eprintln!("streamview: backend started ({})", server.display()),
                Err(e) => eprintln!("streamview: failed to start backend: {e}"),
            }
            // Wait for the backend to accept connections before the window loads it.
            for _ in 0..80 {
                if std::net::TcpStream::connect("127.0.0.1:8799").is_ok() {
                    break;
                }
                std::thread::sleep(std::time::Duration::from_millis(100));
            }
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
