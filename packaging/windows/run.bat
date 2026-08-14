@echo off
setlocal
set "ROOT=%~dp0"
set "STREAMVIEW_BIN=%ROOT%streamview.exe"
set "FFMPEG_BIN=%ROOT%ffmpeg.exe"
set "HOST=127.0.0.1"
set "PORT=8787"

start "" /b "%ROOT%node.exe" "%ROOT%streamview-web\server.js"
for /l %%i in (1,1,40) do (
  curl.exe --silent --fail http://127.0.0.1:8787/api/health >nul 2>&1
  if not errorlevel 1 goto ready
  timeout /t 1 /nobreak >nul
)
echo StreamView backend did not start on port 8787.
exit /b 1

:ready
start "" http://127.0.0.1:8787
echo StreamView is running at http://127.0.0.1:8787
echo Close this window to leave the backend running, or press Ctrl+C to stop it.
pause
