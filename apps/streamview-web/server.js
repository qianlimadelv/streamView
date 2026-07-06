#!/usr/bin/env node
// StreamView Web — a zero-dependency local backend for the browser UI.
//
// It is a thin presentation/orchestration layer: all bitstream analysis is done
// by the `streamview` CLI (sv-analysis) and all decoding by sv-decode. Video
// playback is served by remuxing to MP4 with ffmpeg. No parsing logic lives here,
// preserving the "core independent of the UI" architecture rule.

const http = require('http');
const { execFile, spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');
const crypto = require('crypto');

const ROOT = __dirname;
const PUBLIC_DIR = path.join(ROOT, 'public');
const PORT = process.env.PORT ? Number(process.env.PORT) : 8787;

// Locate the streamview CLI: env override, then the conventional build/ path.
const STREAMVIEW_BIN =
  process.env.STREAMVIEW_BIN ||
  path.resolve(ROOT, '../../build/apps/streamview-cli/streamview');
const FFMPEG_BIN = process.env.FFMPEG_BIN || 'ffmpeg';
const TMP_DIR = fs.mkdtempSync(path.join(os.tmpdir(), 'streamview-web-'));

function sendJson(res, code, obj) {
  const body = Buffer.from(JSON.stringify(obj));
  res.writeHead(code, { 'Content-Type': 'application/json', 'Content-Length': body.length });
  res.end(body);
}

function run(bin, args, opts = {}) {
  return new Promise((resolve, reject) => {
    execFile(bin, args, { maxBuffer: 512 * 1024 * 1024, ...opts }, (err, stdout, stderr) => {
      if (err) {
        err.stderr = stderr;
        return reject(err);
      }
      resolve({ stdout, stderr });
    });
  });
}

// GET /api/browse?dir=<path> -> subdirectories + media files, for the file picker.
// (Browsers can't hand a server a local absolute path, so we browse server-side.)
function handleBrowse(res, query) {
  const dir = query.get('dir') || os.homedir();
  let resolved = dir;
  try {
    resolved = path.resolve(dir);
    const entries = fs.readdirSync(resolved, { withFileTypes: true });
    const dirs = [];
    const files = [];
    for (const e of entries) {
      if (e.name.startsWith('.')) continue;
      if (e.isDirectory()) dirs.push(e.name);
      else if (/\.(h264|264|h265|265|hevc|mp4|mov|m4v|mkv|webm|ts)$/i.test(e.name)) files.push(e.name);
    }
    dirs.sort((a, b) => a.localeCompare(b));
    files.sort((a, b) => a.localeCompare(b));
    sendJson(res, 200, { dir: resolved, parent: path.dirname(resolved), dirs, files });
  } catch (e) {
    sendJson(res, 400, { error: String(e.message), dir: resolved });
  }
}

// GET /api/analyze?path=<file> -> full StreamAnalysis JSON from the CLI.
async function handleAnalyze(res, query) {
  const input = query.get('path');
  if (!input) return sendJson(res, 400, { error: 'missing path' });
  if (!fs.existsSync(input)) return sendJson(res, 404, { error: 'file not found: ' + input });
  try {
    const { stdout } = await run(STREAMVIEW_BIN, [
      'analyze', input, '--format', 'json', '--output', '-',
    ]);
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(stdout);
  } catch (e) {
    sendJson(res, 500, { error: 'analyze failed', detail: String(e.stderr || e.message) });
  }
}

// GET /api/frame?path=<file>&index=N -> decoded-frame detail: metadata, motion
// vectors, and a base64 PPM thumbnail (parsed client-side onto a canvas).
async function handleFrame(res, query) {
  const input = query.get('path');
  const index = query.get('index');
  const size = query.get('size') || '360';
  if (!input || index === null) return sendJson(res, 400, { error: 'missing path or index' });
  if (!fs.existsSync(input)) return sendJson(res, 404, { error: 'file not found: ' + input });

  const stamp = crypto.randomBytes(6).toString('hex');
  const ppmPath = path.join(TMP_DIR, `f_${stamp}.ppm`);
  const jsonPath = path.join(TMP_DIR, `f_${stamp}.json`);
  try {
    await run(STREAMVIEW_BIN, [
      'decode', input, '--frame', String(index),
      '--thumb', ppmPath, '--thumb-size', String(size),
      '--mv-json', jsonPath,
    ]);
    const meta = JSON.parse(fs.readFileSync(jsonPath, 'utf8'));
    let thumb_ppm_base64 = null;
    if (fs.existsSync(ppmPath)) {
      thumb_ppm_base64 = fs.readFileSync(ppmPath).toString('base64');
    }
    sendJson(res, 200, { ...meta, thumb_ppm_base64 });
  } catch (e) {
    sendJson(res, 500, { error: 'decode failed', detail: String(e.stderr || e.message) });
  } finally {
    fs.rmSync(ppmPath, { force: true });
    fs.rmSync(jsonPath, { force: true });
  }
}

// GET /api/frames?path=<file>&start=N&count=M&size=S -> batch of thumbnails,
// decoded in a single pass (much cheaper than M single-frame decodes for the
// filmstrip). Range is in decode order.
async function handleFrames(res, query) {
  const input = query.get('path');
  const start = Number(query.get('start') || 0);
  const count = Number(query.get('count') || 1);
  const size = query.get('size') || '140';
  if (!input) return sendJson(res, 400, { error: 'missing path' });
  if (!fs.existsSync(input)) return sendJson(res, 404, { error: 'file not found: ' + input });

  const dir = path.join(TMP_DIR, 'strip_' + crypto.randomBytes(6).toString('hex'));
  fs.mkdirSync(dir, { recursive: true });
  try {
    const { stdout } = await run(STREAMVIEW_BIN, [
      'decode', input, '--frames', `${start}:${count}`,
      '--thumb-dir', dir, '--thumb-size', String(size),
    ]);
    const parsed = JSON.parse(stdout);
    const frames = (parsed.frames || []).map(f => ({
      decode_index: f.decode_index,
      coded_width: f.coded_width,
      coded_height: f.coded_height,
      pict_type: f.pict_type,
      keyframe: f.keyframe,
      thumb_ppm_base64: f.thumb && fs.existsSync(f.thumb) ? fs.readFileSync(f.thumb).toString('base64') : null,
    }));
    sendJson(res, 200, { frames });
  } catch (e) {
    sendJson(res, 500, { error: 'batch decode failed', detail: String(e.stderr || e.message) });
  } finally {
    fs.rmSync(dir, { recursive: true, force: true });
  }
}

// GET /api/block?path=<file>&index=N&layer=qp|partition|intra|motion -> a base64
// PPM of one HEVC block-level overlay (libde265). Errors for non-HEVC / no libde265.
async function handleBlock(res, query) {
  const input = query.get('path');
  const index = query.get('index');
  const layer = query.get('layer') || 'qp';
  const size = query.get('size') || '480';
  if (!input || index === null) return sendJson(res, 400, { error: 'missing path or index' });
  const ppmPath = path.join(TMP_DIR, `b_${crypto.randomBytes(6).toString('hex')}.ppm`);
  try {
    await run(STREAMVIEW_BIN, [
      'decode', input, '--frame', String(index),
      '--block-layer', layer, '--block-out', ppmPath, '--thumb-size', String(size),
    ]);
    const b64 = fs.existsSync(ppmPath) ? fs.readFileSync(ppmPath).toString('base64') : null;
    sendJson(res, 200, { layer, thumb_ppm_base64: b64 });
  } catch (e) {
    sendJson(res, 500, { error: 'block overlay failed', detail: String(e.stderr || e.message) });
  } finally {
    fs.rmSync(ppmPath, { force: true });
  }
}

// GET /api/dump?path=<file>&nal=N[&format=hex|payload|rbsp] -> raw NAL bytes as
// a hex dump (or binary), for the byte-level view in the NAL panel.
async function handleDump(res, query) {
  const input = query.get('path');
  const nal = query.get('nal');
  const format = query.get('format') || 'hex';
  if (!input || nal === null) return sendJson(res, 400, { error: 'missing path or nal' });
  try {
    const { stdout } = await run(STREAMVIEW_BIN, [
      'dump', input, '--nal', String(nal), '--format', format, '--output', '-',
    ]);
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end(stdout);
  } catch (e) {
    sendJson(res, 500, { error: 'dump failed', detail: String(e.stderr || e.message) });
  }
}

async function handleValidate(res, query) {
  const input = query.get('path');
  if (!input) return sendJson(res, 400, { error: 'missing path' });
  try {
    const { stdout } = await run(STREAMVIEW_BIN, ['validate', input, '--json']);
    res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
    res.end(stdout);
  } catch (e) {
    sendJson(res, 500, { error: 'validate failed', detail: String(e.stderr || e.message) });
  }
}

// Format a buffer as a classic hexdump -C block (offset relative to 0).
function hexdump(buf) {
  const lines = [];
  for (let i = 0; i < buf.length; i += 16) {
    let hex = '', ascii = '';
    for (let j = 0; j < 16; j++) {
      if (j === 8) hex += ' ';
      if (i + j < buf.length) {
        const b = buf[i + j];
        hex += b.toString(16).padStart(2, '0') + ' ';
        ascii += b >= 32 && b < 127 ? String.fromCharCode(b) : '.';
      } else {
        hex += '   ';
      }
    }
    lines.push(`${i.toString(16).padStart(8, '0')}  ${hex} |${ascii}|`);
  }
  return lines.join('\n');
}

// GET /api/hex?path=<file>&offset=N&size=M&limit=L -> hexdump of a byte range,
// read directly (no re-parse). analyze already gives the NAL's payload_offset /
// payload_size, so this is instant even on huge streams (dump re-parses the
// whole file and takes seconds). limit caps how many bytes are formatted.
function handleHex(res, query) {
  const input = query.get('path');
  const offset = Number(query.get('offset'));
  const size = Number(query.get('size'));
  const limit = Number(query.get('limit') || 8192);
  if (!input || !Number.isFinite(offset) || !Number.isFinite(size)) {
    return sendJson(res, 400, { error: 'missing path/offset/size' });
  }
  const readLen = Math.max(0, Math.min(size, limit));
  const buf = Buffer.alloc(readLen);
  try {
    const fd = fs.openSync(input, 'r');
    const n = fs.readSync(fd, buf, 0, readLen, offset);
    fs.closeSync(fd);
    sendJson(res, 200, { hex: hexdump(buf.subarray(0, n)), total: size, shown: n, truncated: n < size });
  } catch (e) {
    sendJson(res, 500, { error: 'read failed', detail: String(e.message) });
  }
}

// Remux (or copy) the input to a faststart MP4, cached by path+mtime, so the
// browser <video> element can play raw .h264/.h265 elementary streams too.
const videoCache = new Map();
function ensurePlayableMp4(input) {
  return new Promise((resolve, reject) => {
    let stat;
    try {
      stat = fs.statSync(input);
    } catch (e) {
      return reject(new Error('file not found: ' + input));
    }
    const key = `${input}:${stat.mtimeMs}:${stat.size}`;
    const cached = videoCache.get(key);
    if (cached && fs.existsSync(cached)) return resolve(cached);

    const out = path.join(TMP_DIR, 'play_' + crypto.createHash('md5').update(key).digest('hex') + '.mp4');
    const args = [
      '-y', '-loglevel', 'error', '-i', input,
      '-c', 'copy', '-movflags', '+faststart', out,
    ];
    execFile(FFMPEG_BIN, args, (err) => {
      // Stream copy fails for some raw streams (no timestamps); fall back to re-encode.
      if (err) {
        const reArgs = [
          '-y', '-loglevel', 'error', '-i', input,
          '-c:v', 'libx264', '-preset', 'veryfast', '-pix_fmt', 'yuv420p',
          '-movflags', '+faststart', out,
        ];
        return execFile(FFMPEG_BIN, reArgs, (err2) => {
          if (err2) return reject(err2);
          videoCache.set(key, out);
          resolve(out);
        });
      }
      videoCache.set(key, out);
      resolve(out);
    });
  });
}

// Serve a file with HTTP range support so <video> can seek.
function serveFileWithRange(req, res, filePath, contentType) {
  const stat = fs.statSync(filePath);
  const range = req.headers.range;
  if (range) {
    const m = /bytes=(\d*)-(\d*)/.exec(range);
    const start = m[1] ? parseInt(m[1], 10) : 0;
    const end = m[2] ? parseInt(m[2], 10) : stat.size - 1;
    res.writeHead(206, {
      'Content-Range': `bytes ${start}-${end}/${stat.size}`,
      'Accept-Ranges': 'bytes',
      'Content-Length': end - start + 1,
      'Content-Type': contentType,
    });
    fs.createReadStream(filePath, { start, end }).pipe(res);
  } else {
    res.writeHead(200, {
      'Content-Length': stat.size,
      'Accept-Ranges': 'bytes',
      'Content-Type': contentType,
    });
    fs.createReadStream(filePath).pipe(res);
  }
}

async function handleVideo(req, res, query) {
  const input = query.get('path');
  if (!input) return sendJson(res, 400, { error: 'missing path' });
  try {
    const mp4 = await ensurePlayableMp4(input);
    serveFileWithRange(req, res, mp4, 'video/mp4');
  } catch (e) {
    sendJson(res, 500, { error: 'video prep failed', detail: String(e.stderr || e.message) });
  }
}

const STATIC_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
};

function serveStatic(res, urlPath) {
  const rel = urlPath === '/' ? 'index.html' : urlPath.replace(/^\/+/, '');
  const filePath = path.join(PUBLIC_DIR, rel);
  if (!filePath.startsWith(PUBLIC_DIR) || !fs.existsSync(filePath)) {
    res.writeHead(404, { 'Content-Type': 'text/plain' });
    return res.end('not found');
  }
  const body = fs.readFileSync(filePath);
  res.writeHead(200, { 'Content-Type': STATIC_TYPES[path.extname(filePath)] || 'application/octet-stream' });
  res.end(body);
}

const server = http.createServer((req, res) => {
  const url = new URL(req.url, `http://${req.headers.host}`);
  const q = url.searchParams;
  if (url.pathname === '/api/browse') return handleBrowse(res, q);
  if (url.pathname === '/api/analyze') return handleAnalyze(res, q);
  if (url.pathname === '/api/frame') return handleFrame(res, q);
  if (url.pathname === '/api/frames') return handleFrames(res, q);
  if (url.pathname === '/api/block') return handleBlock(res, q);
  if (url.pathname === '/api/dump') return handleDump(res, q);
  if (url.pathname === '/api/validate') return handleValidate(res, q);
  if (url.pathname === '/api/hex') return handleHex(res, q);
  if (url.pathname === '/api/video') return handleVideo(req, res, q);
  if (url.pathname === '/api/health') return sendJson(res, 200, { ok: true, bin: STREAMVIEW_BIN });
  return serveStatic(res, url.pathname);
});

server.listen(PORT, () => {
  console.log(`StreamView Web on http://localhost:${PORT}`);
  console.log(`  streamview: ${STREAMVIEW_BIN}${fs.existsSync(STREAMVIEW_BIN) ? '' : '  (NOT FOUND — build it or set STREAMVIEW_BIN)'}`);
});
