'use strict';

const TYPE_COLOR = { I: '#ff5b6a', P: '#4c8dff', B: '#8a94a6', SP: '#4c8dff', SI: '#ff5b6a' };

const el = (id) => document.getElementById(id);

// ---- i18n (中文默认 / English) --------------------------------------------
const I18N = {
  zh: {
    open_file: '📁 选择文件', analyze: 'Analyze', view_tree: '结构', view_detail: '详情',
    path_ph: '点「选择文件」浏览,或手动填绝对路径',
    tl_select: '时间线 · 选帧', keyframe: '关键帧', click_filter: '← 点击筛选',
    cur_frame: '当前帧', mv: 'MV', filter: '筛选',
    layer_original: '原图 + MV', layer_qp: 'QP 热力 (HEVC)', layer_partition: 'CB 分区 (HEVC)',
    layer_intra: '帧内预测 (HEVC)', layer_motion: '运动矢量 (HEVC)',
    tree_title: '码流结构树', tab_content: '码流内容', tab_overview: '码流总览', tab_playback: '播放',
    ov_codec: '编解码器', ov_res: '分辨率', ov_dur: '时长', ov_fps: '平均帧率',
    ov_avgbr: '平均码率', ov_peakbr: '峰值码率', ov_fsize: '文件大小', ov_frames: '总帧数',
    ov_disttitle: '帧类型分布(条=大小占比)', ov_gop: 'GOP', ov_gopcount: 'GOP 数',
    ov_gopavg: '平均长度', ov_gopminmax: '最短 / 最长', ov_fsizetitle: '帧大小',
    ov_avgframe: '平均帧', ov_maxframe: '最大帧', ov_minframe: '最小帧',
    unknown: '未知', src_container: '容器', win1s: '1s 窗',
    m_disp: '显示序号', m_dec: '解码序号', m_type: '类型', m_key: '关键帧', m_poc: 'POC',
    m_gop: 'GOP', m_size: '大小', m_nalcount: 'NAL 数', m_pts: 'PTS', m_dts: 'DTS',
    m_coded: '编码尺寸', m_mv: '运动矢量', yes: '是', no: '否',
    frames_u: '帧', nal_u: 'NAL', gop_kf: '关键帧', ungrouped: '未分组', other: '其他',
    hex: '十六进制 (hex)', frame_size: '画面大小', theme: '日夜切换',
    need_hevc: '块级图层需 HEVC', frame_word: '帧', avg: '平均', no_gop: '无 GOP 数据',
    exp_frame: '导出当前帧 PNG', exp_json: '导出分析 JSON', drop_hint: '浏览器版拖拽读不到路径,请用「选择文件」;桌面版支持拖拽',
    validate_title: '一致性校验', ok_clean: '未发现问题', sev_error: '错误', sev_warning: '警告',
    loading: '加载中…', hex_shown: '已显示前', hex_of: '共', hex_all: '显示全部',
  },
  en: {
    open_file: '📁 Open file', analyze: 'Analyze', view_tree: 'Tree', view_detail: 'Detail',
    path_ph: 'Click “Open file”, or type an absolute path',
    tl_select: 'Timeline · pick', keyframe: 'keyframe', click_filter: '← click to filter',
    cur_frame: 'current frame', mv: 'MV', filter: 'Filter',
    layer_original: 'Original + MV', layer_qp: 'QP heatmap (HEVC)', layer_partition: 'CB partition (HEVC)',
    layer_intra: 'Intra pred (HEVC)', layer_motion: 'Motion (HEVC)',
    tree_title: 'Bitstream tree', tab_content: 'NAL content', tab_overview: 'Overview', tab_playback: 'Playback',
    ov_codec: 'Codec', ov_res: 'Resolution', ov_dur: 'Duration', ov_fps: 'Avg frame rate',
    ov_avgbr: 'Avg bitrate', ov_peakbr: 'Peak bitrate', ov_fsize: 'File size', ov_frames: 'Frames',
    ov_disttitle: 'Frame-type distribution (bar = size share)', ov_gop: 'GOP', ov_gopcount: 'GOP count',
    ov_gopavg: 'Avg length', ov_gopminmax: 'Min / Max', ov_fsizetitle: 'Frame size',
    ov_avgframe: 'Avg frame', ov_maxframe: 'Max frame', ov_minframe: 'Min frame',
    unknown: 'unknown', src_container: 'container', win1s: '1s win',
    m_disp: 'Display index', m_dec: 'Decode index', m_type: 'Type', m_key: 'Keyframe', m_poc: 'POC',
    m_gop: 'GOP', m_size: 'Size', m_nalcount: 'NAL count', m_pts: 'PTS', m_dts: 'DTS',
    m_coded: 'Coded', m_mv: 'Motion vectors', yes: 'yes', no: 'no',
    frames_u: 'frames', nal_u: 'NAL', gop_kf: 'keyframe', ungrouped: 'ungrouped', other: 'other',
    hex: 'Hex', frame_size: 'Image size', theme: 'Theme',
    need_hevc: 'block layers need HEVC', frame_word: 'frame', avg: 'avg', no_gop: 'no GOP data',
    exp_frame: 'Export frame PNG', exp_json: 'Export analysis JSON', drop_hint: 'Browser drag-drop can’t read the path — use “Open file”; the desktop app supports drag-drop',
    validate_title: 'Validation', ok_clean: 'No issues', sev_error: 'error', sev_warning: 'warning',
    loading: 'loading…', hex_shown: 'showing first', hex_of: 'of', hex_all: 'show all',
  },
};
let LANG = (() => { try { return localStorage.getItem('sv_lang') || 'zh'; } catch (e) { return 'zh'; } })();
function t(k) { return (I18N[LANG] && I18N[LANG][k]) || I18N.zh[k] || k; }
function applyStaticI18n() {
  document.querySelectorAll('[data-i18n]').forEach(e => { e.textContent = t(e.dataset.i18n); });
  document.querySelectorAll('[data-i18n-ph]').forEach(e => { e.placeholder = t(e.dataset.i18nPh); });
  document.querySelectorAll('[data-i18n-title]').forEach(e => { e.title = t(e.dataset.i18nTitle); });
  document.documentElement.lang = LANG === 'zh' ? 'zh-CN' : 'en';
}
function setLang(l) {
  LANG = l;
  try { localStorage.setItem('sv_lang', l); } catch (e) {}
  el('langBtn').textContent = l === 'zh' ? 'EN' : '中';
  applyStaticI18n();
  if (state.analysis) {
    renderOverview(state.analysis);
    renderStreamTree(state.analysis);
    if (state.selected >= 0) selectFrame(state.selected);
  }
}

const state = {
  path: null,
  analysis: null,
  frames: [],      // decode-order frames from the analysis
  selected: -1,
  bars: [],        // {x0,x1} hit-boxes for timeline click
  browseDir: null, // last browsed directory
  zoom: 1,         // timeline horizontal zoom
  filter: { I: true, P: true, B: true }, // frame-type visibility
  aus: [],         // access units (grouped NALs) for the bitstream tree
  thumbCache: {},  // decodeIndex -> base64 PPM, for the filmstrip
  stripStart: -1,  // current filmstrip window start
};

const STRIP_COUNT = 9; // frames shown in the filmstrip at once

function setStatus(msg) { el('status').textContent = msg; }

// ---- Server-side file browser ---------------------------------------------

async function loadDir(dir) {
  try {
    const res = await fetch('/api/browse?dir=' + encodeURIComponent(dir || ''));
    const d = await res.json();
    if (d.error) { setStatus('浏览失败: ' + d.error); }
    if (d.dir) state.browseDir = d.dir;
    el('browserPath').textContent = d.dir || dir || '';
    const rows = [];
    if (d.parent && d.parent !== d.dir) {
      rows.push(`<div class="browser-item dir" data-dir="${d.parent}"><span class="ic">⬆</span>..</div>`);
    }
    for (const name of (d.dirs || [])) {
      rows.push(`<div class="browser-item dir" data-dir="${d.dir}/${name}"><span class="ic">📁</span>${name}</div>`);
    }
    for (const name of (d.files || [])) {
      rows.push(`<div class="browser-item file" data-file="${d.dir}/${name}"><span class="ic">🎬</span>${name}</div>`);
    }
    el('browserList').innerHTML = rows.join('') || '<div class="browser-item">（此目录无子目录或视频文件）</div>';
  } catch (e) {
    setStatus('浏览请求失败: ' + e.message);
  }
}

function openBrowser() {
  el('browser').classList.add('open');
  loadDir(state.browseDir || el('path').value || '');
}
function closeBrowser() { el('browser').classList.remove('open'); }

// Native file dialog under Tauri desktop; falls back to the server-side browser on web.
async function openFileDialog() {
  const tauri = window.__TAURI__;
  if (tauri && tauri.dialog && tauri.dialog.open) {
    try {
      const sel = await tauri.dialog.open({
        multiple: false,
        filters: [{ name: 'Video', extensions: ['h264', '264', 'h265', '265', 'hevc', 'mp4', 'mov', 'm4v', 'mkv', 'webm', 'ts'] }],
      });
      if (typeof sel === 'string') { el('path').value = sel; analyze(); }
      return;
    } catch (e) { /* fall through to web browser */ }
  }
  openBrowser();
}

el('browse').addEventListener('click', openFileDialog);
el('browserClose').addEventListener('click', closeBrowser);
el('browser').addEventListener('click', (ev) => { if (ev.target.id === 'browser') closeBrowser(); });
el('browserList').addEventListener('click', (ev) => {
  const item = ev.target.closest('.browser-item');
  if (!item) return;
  if (item.dataset.dir) {
    loadDir(item.dataset.dir);
  } else if (item.dataset.file) {
    el('path').value = item.dataset.file;
    closeBrowser();
    analyze();
  }
});

async function analyze() {
  const path = el('path').value.trim();
  if (!path) return;
  state.path = path;
  setStatus('analyzing…');
  try {
    const res = await fetch('/api/analyze?path=' + encodeURIComponent(path));
    const data = await res.json();
    if (data.error) { setStatus('error: ' + data.error); return; }
    state.analysis = data;
    state.frames = data.frames || [];
    state.thumbCache = {};
    state.stripStart = -1;
    renderOverview(data);
    configureLayerSelect(data.codec_guess);
    renderStreamTree(data);
    drawTimeline();
    el('player').src = '/api/video?path=' + encodeURIComponent(path);
    setStatus(`${state.frames.length} frames · ${data.codec_guess} · ${data.format}`);
    const wantFrame = Number(new URLSearchParams(location.search).get('frame')) || 0;
    if (state.frames.length) selectFrame(Math.min(wantFrame, state.frames.length - 1));
    loadValidation(path);
  } catch (e) {
    setStatus('request failed: ' + e.message);
  }
}

// Fetch consistency-check issues (non-blocking) and surface a badge + list.
async function loadValidation(path) {
  try {
    const res = await fetch('/api/validate?path=' + encodeURIComponent(path));
    const v = await res.json();
    state.issues = v.issues || [];
  } catch (e) {
    state.issues = [];
  }
  renderValidationBadge();
  if (state.analysis) renderOverview(state.analysis);
}
function renderValidationBadge() {
  const b = el('valBadge');
  if (!b) return;
  const n = (state.issues || []).length;
  if (!n) { b.style.display = 'none'; return; }
  const errs = state.issues.filter(i => i.severity === 'error').length;
  b.style.display = '';
  b.textContent = '⚠ ' + n;
  b.style.background = errs ? '#e5484d' : '#c98a00';
  b.title = state.issues.map(i => `${i.severity}: ${i.message}`).join('\n');
}

function renderSummary(d) {
  const s = d.stream_summary || {};
  const sps = s.active_sps || {};
  const res = sps.width ? `${sps.width}×${sps.height}` : '—';
  const stats = [
    ['Codec', d.codec_guess || '—'],
    ['Resolution', res],
    ['Frames', s.frame_count ?? state.frames.length],
    ['Keyframes', s.keyframe_count ?? '—'],
    ['GOPs', s.gop_count ?? '—'],
    ['Size', fmtBytes(d.size_bytes || 0)],
    ['Parse errors', (s.parse_errors && s.parse_errors.total) ?? 0],
    ['Slices', s.slice_count ?? '—'],
  ];
  el('summary').innerHTML = stats.map(([k, v]) =>
    `<div class="stat"><div class="k">${k}</div><div class="v">${v}</div></div>`).join('');
}

// Block-level overlays are HEVC-only (libde265); disable them for other codecs.
function configureLayerSelect(codec) {
  const hevc = codec === 'h265';
  for (const opt of el('layerSelect').options) {
    if (opt.value !== 'original') opt.disabled = !hevc;
  }
  if (!hevc && el('layerSelect').value !== 'original') el('layerSelect').value = 'original';
  el('layerHint').textContent = hevc ? '' : t('need_hevc');
}

function fmtBytes(n) {
  if (n < 1024) return n + ' B';
  if (n < 1024 * 1024) return (n / 1024).toFixed(1) + ' KB';
  return (n / 1024 / 1024).toFixed(2) + ' MB';
}

function fmtBitrate(bps) {
  if (bps >= 1e6) return (bps / 1e6).toFixed(2) + ' Mbps';
  if (bps >= 1e3) return (bps / 1e3).toFixed(0) + ' kbps';
  return Math.round(bps) + ' bps';
}

function fmtDuration(s) {
  if (s == null) return '—';
  const m = Math.floor(s / 60), sec = s - m * 60;
  return `${m}:${sec.toFixed(2).padStart(5, '0')}`;
}

// Derive stream-level statistics from the analysis JSON (client-side).
function computeOverview(d) {
  const frames = d.frames || [];
  const n = frames.length;
  const sps = (d.stream_summary && d.stream_summary.active_sps) || {};
  let fps = null, fpsSrc = '';
  const c = d.container;
  if (c && c.avg_frame_rate && c.avg_frame_rate.den) {
    fps = c.avg_frame_rate.num / c.avg_frame_rate.den; fpsSrc = '容器';
  }
  if (!fps) {
    for (const nal of (d.nals || [])) {
      const s = (nal.h264 && nal.h264.sps) || (nal.h265 && nal.h265.sps);
      if (s && s.vui && s.vui.timing_info_present && s.vui.num_units_in_tick > 0) {
        const div = d.codec_guess === 'h264' ? 2 : 1;
        fps = s.vui.time_scale / (div * s.vui.num_units_in_tick); fpsSrc = 'VUI'; break;
      }
    }
  }
  const totalBytes = frames.reduce((a, f) => a + (f.size_bytes || 0), 0) || d.size_bytes || 0;
  const totalBits = totalBytes * 8;
  let duration = null;
  if (c && c.duration_ts && c.time_base && c.time_base.den) {
    duration = c.duration_ts * (c.time_base.num || 1) / c.time_base.den;
  }
  if (!duration && fps && n) duration = n / fps;
  const avgBitrate = duration ? totalBits / duration : null;

  let peakBitrate = null;
  if (fps && n) {
    const win = Math.max(1, Math.round(fps));
    const bits = frames.map(f => (f.size_bytes || 0) * 8);
    let sum = 0, max = 0;
    for (let i = 0; i < n; i++) {
      sum += bits[i];
      if (i >= win) sum -= bits[i - win];
      if (i >= win - 1) max = Math.max(max, sum);
    }
    peakBitrate = max; // bits in the busiest 1-second window == bps
  }

  const types = {};
  for (const f of frames) {
    const t = f.frame_type;
    (types[t] = types[t] || { count: 0, bytes: 0 });
    types[t].count++; types[t].bytes += f.size_bytes || 0;
  }
  const gops = d.gops || [];
  const lens = gops.map(g => g.frame_count);
  const gopStats = gops.length ? {
    count: gops.length,
    avg: lens.reduce((a, b) => a + b, 0) / gops.length,
    min: Math.min(...lens), max: Math.max(...lens),
  } : null;
  const sizes = frames.map(f => f.size_bytes || 0);
  const fsize = n ? { max: Math.max(...sizes), min: Math.min(...sizes), avg: totalBytes / n } : null;
  return { n, sps, fps, fpsSrc, totalBytes, duration, avgBitrate, peakBitrate, types, gopStats, fsize, codec: d.codec_guess || '' };
}

function renderOverview(d) {
  const o = computeOverview(d);
  const res = o.sps.width ? `${o.sps.width}×${o.sps.height}` : '—';
  const prof = o.sps.profile_idc != null ? `profile ${o.sps.profile_idc} · level ${o.sps.level_idc}` : '';
  const kpi = (k, v, sub) => `<div class="kpi"><div class="k">${k}</div><div class="v">${v}${sub ? ` <small>${sub}</small>` : ''}</div></div>`;
  const kpis = [
    kpi(t('ov_codec'), (o.codec || '—').toUpperCase(), prof),
    kpi(t('ov_res'), res),
    kpi(t('ov_dur'), fmtDuration(o.duration)),
    kpi(t('ov_fps'), o.fps != null ? o.fps.toFixed(3) + ' fps' : t('unknown'), o.fps != null ? (o.fpsSrc === '容器' ? t('src_container') : o.fpsSrc) : ''),
    kpi(t('ov_avgbr'), o.avgBitrate != null ? fmtBitrate(o.avgBitrate) : '—'),
    kpi(t('ov_peakbr'), o.peakBitrate != null ? fmtBitrate(o.peakBitrate) : '—', o.peakBitrate != null ? t('win1s') : ''),
    kpi(t('ov_fsize'), fmtBytes(o.totalBytes)),
    kpi(t('ov_frames'), o.n),
  ].join('');

  const totalTypeBytes = Object.values(o.types).reduce((a, tt) => a + tt.bytes, 0) || 1;
  const typeRows = ['I', 'P', 'B', 'SP', 'SI'].filter(ty => o.types[ty]).map(ty => {
    const x = o.types[ty];
    const pct = o.n ? (x.count / o.n * 100) : 0;
    const bpct = x.bytes / totalTypeBytes * 100;
    return `<div class="row2"><span class="k">${ty}</span><span>${x.count} ${t('frames_u')} · ${pct.toFixed(1)}% · ${t('avg')} ${fmtBytes(x.bytes / x.count)}</span></div>
      <div class="bar"><span style="width:${bpct.toFixed(1)}%;background:${TYPE_COLOR[ty] || '#666'}"></span></div>`;
  }).join('');

  const gop = o.gopStats
    ? `<div class="row2"><span class="k">${t('ov_gopcount')}</span><span>${o.gopStats.count}</span></div>
       <div class="row2"><span class="k">${t('ov_gopavg')}</span><span>${o.gopStats.avg.toFixed(1)} ${t('frames_u')}</span></div>
       <div class="row2"><span class="k">${t('ov_gopminmax')}</span><span>${o.gopStats.min} / ${o.gopStats.max} ${t('frames_u')}</span></div>`
    : `<div class="row2"><span style="color:var(--muted)">${t('no_gop')}</span></div>`;
  const fs = o.fsize
    ? `<div class="row2"><span class="k">${t('ov_avgframe')}</span><span>${fmtBytes(o.fsize.avg)}</span></div>
       <div class="row2"><span class="k">${t('ov_maxframe')}</span><span>${fmtBytes(o.fsize.max)}</span></div>
       <div class="row2"><span class="k">${t('ov_minframe')}</span><span>${fmtBytes(o.fsize.min)}</span></div>`
    : '';

  const sevName = { error: t('sev_error'), warning: t('sev_warning') };
  const issues = state.issues || [];
  const valHtml = `<div class="valbox${issues.length ? '' : ' ok'}">
    <h3>${t('validate_title')}${issues.length ? ' · ' + issues.length : ''}</h3>
    ${issues.length
      ? issues.map(i => `<div class="issue"><span class="sev sev-${i.severity}">${sevName[i.severity] || i.severity}</span> ${i.message} <code>${i.code}</code></div>`).join('')
      : `<div class="issue muted">✓ ${t('ok_clean')}</div>`}
  </div>`;
  el('overview').innerHTML = valHtml + `<div class="kpis">${kpis}</div>
    <div class="sub">
      <div><h3>${t('ov_disttitle')}</h3>${typeRows}</div>
      <div><h3>${t('ov_gop')}</h3>${gop}</div>
      <div><h3>${t('ov_fsizetitle')}</h3>${fs}</div>
    </div>`;
}

function drawTimeline() {
  const canvas = el('timeline');
  const base = el('timelineScroll').clientWidth || 900;
  const w = Math.round(base * (state.zoom || 1));
  canvas.width = w;
  canvas.style.width = w + 'px';
  const h = canvas.height;
  const ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, w, h);
  const frames = state.frames;
  if (!frames.length) return;

  const maxSize = Math.max(...frames.map(f => f.size_bytes || 1), 1);
  const gap = frames.length > 200 ? 0 : 1;
  const bw = Math.max(1, (w - (frames.length - 1) * gap) / frames.length);
  const pad = 14;
  const usableH = h - pad - 16;
  state.bars = [];

  frames.forEach((f, i) => {
    const x0 = i * (bw + gap);
    const bh = Math.max(2, (Math.sqrt(f.size_bytes || 0) / Math.sqrt(maxSize)) * usableH);
    const y = h - 16 - bh;
    const visible = state.filter[f.frame_type] !== false;
    ctx.fillStyle = TYPE_COLOR[f.frame_type] || '#666';
    ctx.globalAlpha = !visible ? 0.08 : ((i === state.selected) ? 1 : 0.82);
    ctx.fillRect(x0, y, bw, bh);
    if (f.is_keyframe) {
      ctx.fillStyle = '#ffd23f';
      ctx.fillRect(x0, h - 14, Math.max(2, bw), 3);
    }
    if (i === state.selected) {
      ctx.globalAlpha = 1;
      ctx.strokeStyle = '#fff';
      ctx.strokeRect(x0 - 0.5, y - 0.5, bw + 1, bh + 1);
    }
    state.bars.push({ x0, x1: x0 + bw + gap });
  });
  ctx.globalAlpha = 1;

  // Moving-average size trend (a bitrate proxy) drawn over the bars.
  const win = Math.max(1, Math.round(frames.length / 25));
  ctx.strokeStyle = 'rgba(255,255,255,0.55)';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  frames.forEach((f, i) => {
    let sum = 0, n = 0;
    for (let k = Math.max(0, i - win); k <= Math.min(frames.length - 1, i + win); k++) {
      sum += frames[k].size_bytes || 0; n++;
    }
    const avg = sum / n;
    const y = h - 16 - (Math.sqrt(avg) / Math.sqrt(maxSize)) * usableH;
    const x = i * (bw + gap) + bw / 2;
    if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
  });
  ctx.stroke();

  // Playhead: clearly mark where the selected frame sits along the timeline.
  if (state.selected >= 0 && state.selected < frames.length) {
    const f = frames[state.selected];
    const cx = state.selected * (bw + gap) + bw / 2;
    ctx.strokeStyle = '#4c8dff';
    ctx.lineWidth = 2;
    ctx.beginPath(); ctx.moveTo(cx, 15); ctx.lineTo(cx, h - 12); ctx.stroke();
    const lbl = `#${f.index} ${f.frame_type} · ${state.selected + 1}/${frames.length}`;
    ctx.font = '11px system-ui, sans-serif';
    const tw = ctx.measureText(lbl).width;
    const lx = Math.max(2, Math.min(cx - tw / 2 - 5, w - tw - 12));
    ctx.fillStyle = '#4c8dff';
    ctx.fillRect(lx, 0, tw + 10, 15);
    ctx.fillStyle = '#fff';
    ctx.textBaseline = 'middle';
    ctx.fillText(lbl, lx + 5, 8);
    ctx.textBaseline = 'alphabetic';
  }
}

el('timeline').addEventListener('click', (ev) => {
  const rect = ev.target.getBoundingClientRect();
  const x = ev.clientX - rect.left;
  for (let i = 0; i < state.bars.length; i++) {
    if (x >= state.bars[i].x0 && x < state.bars[i].x1) { selectFrame(i); return; }
  }
});

// Draw a base64 PPM into a fixed-size thumbnail canvas, letterboxed.
function drawThumbToCanvas(b64, canvas) {
  const ppm = parsePPM(b64ToBytes(b64));
  const tmp = document.createElement('canvas');
  tmp.width = ppm.w; tmp.height = ppm.h;
  const timg = tmp.getContext('2d').createImageData(ppm.w, ppm.h);
  for (let i = 0, j = 0; i < ppm.rgb.length; i += 3, j += 4) {
    timg.data[j] = ppm.rgb[i]; timg.data[j + 1] = ppm.rgb[i + 1];
    timg.data[j + 2] = ppm.rgb[i + 2]; timg.data[j + 3] = 255;
  }
  tmp.getContext('2d').putImageData(timg, 0, 0);
  const ctx = canvas.getContext('2d');
  const scale = Math.min(canvas.width / ppm.w, canvas.height / ppm.h);
  const dw = Math.round(ppm.w * scale), dh = Math.round(ppm.h * scale);
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.drawImage(tmp, (canvas.width - dw) / 2, (canvas.height - dh) / 2, dw, dh);
}

// Filmstrip: a strip of thumbnails around the selected frame; click to select.
function renderFilmstrip(center) {
  const frames = state.frames;
  const strip = el('filmstrip');
  if (!frames.length) { strip.innerHTML = ''; return; }
  let start = Math.min(center - Math.floor(STRIP_COUNT / 2), frames.length - STRIP_COUNT);
  start = Math.max(0, start);
  const end = Math.min(frames.length, start + STRIP_COUNT);

  if (state.stripStart === start && strip.children.length) {
    strip.querySelectorAll('.thumb').forEach(t =>
      t.classList.toggle('sel', Number(t.dataset.index) === state.selected));
    return;
  }
  state.stripStart = start;
  strip.innerHTML = '';
  for (let i = start; i < end; i++) {
    const f = frames[i];
    const div = document.createElement('div');
    div.className = 'thumb' + (i === state.selected ? ' sel' : '');
    div.dataset.index = i;
    const cv = document.createElement('canvas');
    cv.width = 120; cv.height = 68;
    div.appendChild(cv);
    const lbl = document.createElement('div');
    lbl.className = 'thumb-label';
    lbl.textContent = `#${f.index} ${f.frame_type}${f.is_keyframe ? '★' : ''}`;
    div.appendChild(lbl);
    strip.appendChild(div);
  }
  fillFilmstripThumbs(start, end);
}

// Load the strip's thumbnails in a single batched decode pass (one /api/frames
// over the decode-order range), then paint each canvas from the cache.
async function fillFilmstripThumbs(start, end) {
  const frames = state.frames;
  const dois = [];
  for (let i = start; i < end; i++) dois.push(frames[i].decode_order_index);
  const need = dois.filter(d => state.thumbCache[d] === undefined);
  if (need.length) {
    const lo = Math.min(...need), hi = Math.max(...need);
    try {
      const res = await fetch(`/api/frames?path=${encodeURIComponent(state.path)}&start=${lo}&count=${hi - lo + 1}&size=140`);
      const d = await res.json();
      (d.frames || []).forEach(f => { state.thumbCache[f.decode_index] = f.thumb_ppm_base64 || null; });
    } catch (e) { /* leave uncached; will retry next time */ }
  }
  if (state.stripStart !== start) return; // user moved on; a newer strip is active
  el('filmstrip').querySelectorAll('.thumb').forEach(div => {
    const cv = div.querySelector('canvas');
    const b64 = state.thumbCache[frames[Number(div.dataset.index)].decode_order_index];
    if (b64 && cv && cv.isConnected) drawThumbToCanvas(b64, cv);
  });
}

async function selectFrame(i) {
  if (i < 0 || i >= state.frames.length) return;
  state.selected = i;
  drawTimeline();
  renderFilmstrip(i);
  const f = state.frames[i];
  el('frameTitle').textContent = `#${f.index} (${f.frame_type})`;
  renderNalPanel(f);
  highlightTreeAU(f.index);
  // Instant placeholder: paint the cached thumbnail (upscaled) so the big image
  // isn't blank while the full-res 720px frame decodes.
  const cachedThumb = state.thumbCache[f.decode_order_index];
  if (cachedThumb) drawThumbToCanvas(cachedThumb, el('frameCanvas'));
  setStatus('decoding frame ' + f.decode_order_index + '…');
  try {
    const res = await fetch(`/api/frame?path=${encodeURIComponent(state.path)}&index=${f.decode_order_index}&size=720`);
    const detail = await res.json();
    if (detail.error) { setStatus('decode error: ' + detail.error); return; }
    lastDetail = detail;
    renderCanvasForLayer();
    setStatus(`frame #${f.index} · ${detail.motion_vector_count} MV`);
  } catch (e) {
    setStatus('decode request failed: ' + e.message);
  }
}

// ---- NAL syntax rendering -------------------------------------------------

const NAL_TAG_COLOR = (name) => {
  if (/sps|pps|vps/.test(name)) return '#b06bff';
  if (/idr/.test(name)) return '#ff5b6a';
  if (/slice/.test(name)) return '#4c8dff';
  if (/sei/.test(name)) return '#8a94a6';
  return '#5a6472';
};

// Enum value → human label for VUI fields (ITU-T H.273 / E.2.1).
const ENUM_LABELS = {
  colour_primaries: { 1: 'BT.709', 2: 'unspecified', 4: 'BT.470M', 5: 'BT.470BG', 6: 'BT.601', 7: 'SMPTE240M', 8: 'FILM', 9: 'BT.2020', 10: 'SMPTE428', 11: 'SMPTE431', 12: 'SMPTE432' },
  transfer_characteristics: { 1: 'BT.709', 2: 'unspecified', 4: 'gamma22', 6: 'BT.601', 7: 'SMPTE240M', 8: 'linear', 11: 'IEC61966-2-4', 13: 'sRGB', 14: 'BT.2020-10', 15: 'BT.2020-12', 16: 'PQ (HDR10)', 18: 'HLG' },
  matrix_coefficients: { 0: 'GBR', 1: 'BT.709', 2: 'unspecified', 5: 'BT.470BG', 6: 'BT.601', 7: 'SMPTE240M', 9: 'BT.2020-NCL', 10: 'BT.2020-CL' },
  aspect_ratio_idc: { 0: 'unspecified', 1: '1:1', 2: '12:11', 3: '10:11', 4: '16:11', 5: '40:33', 255: 'Extended_SAR' },
  video_format: { 0: 'Component', 1: 'PAL', 2: 'NTSC', 3: 'SECAM', 4: 'MAC', 5: 'Unspecified' },
};

function labelValue(key, value) {
  const map = ENUM_LABELS[key];
  if (map && map[value] !== undefined) return `${value} (${map[value]})`;
  return fmtVal(value);
}

// Derived frame rate from VUI timing (H.264 divides the tick by 2; H.265 doesn't).
function vuiFps(vui) {
  if (!vui.timing_info_present || !vui.num_units_in_tick) return null;
  const div = (state.analysis && state.analysis.codec_guess === 'h264') ? 2 : 1;
  const fps = vui.time_scale / (div * vui.num_units_in_tick);
  return Math.round(fps * 1000) / 1000;
}

function kvRows(obj) {
  // Flatten a parsed NAL codec object into grouped key/value rows.
  const rows = [];
  const scalars = {}, groups = {};
  for (const [k, v] of Object.entries(obj)) {
    if (k === 'sei_messages') continue; // rendered separately
    if (v !== null && typeof v === 'object') groups[k] = v; else scalars[k] = v;
  }
  for (const [k, v] of Object.entries(scalars)) {
    const cls = /parse_error/.test(k) ? ' pserr' : '';
    rows.push(`<div class="row${cls}"><span class="k">${k}</span><span>${labelValue(k, v)}</span></div>`);
  }
  for (const [g, gv] of Object.entries(groups)) {
    rows.push(`<div class="grouphdr">${g}</div>`);
    for (const [k, v] of Object.entries(gv)) {
      rows.push(`<div class="row"><span class="k">${k}</span><span>${labelValue(k, v)}</span></div>`);
    }
    if (g === 'vui') {
      const fps = vuiFps(gv);
      if (fps !== null) {
        rows.push(`<div class="row"><span class="k">fps (derived)</span><span>${fps}</span></div>`);
      }
    }
  }
  return rows.join('');
}

function fmtVal(v) {
  if (v === null) return '—';
  if (v === true) return 'true';
  if (v === false) return 'false';
  return String(v);
}

function renderSei(msgs) {
  if (!msgs || !msgs.length) return '';
  const rows = msgs.map(m => {
    let extra = '';
    if (m.uuid) {
      extra = `<div class="row"><span class="k" style="padding-left:14px">uuid</span><span>${m.uuid}</span></div>`;
    } else if (m.recovery_frame_cnt !== undefined) {
      extra = `<div class="row"><span class="k" style="padding-left:14px">recovery_frame_cnt</span><span>${m.recovery_frame_cnt}</span></div>`;
    }
    return `<div class="row"><span class="k">type ${m.payload_type} · ${m.name}</span><span>${m.payload_size} B</span></div>${extra}`;
  }).join('');
  return `<div class="kvtable"><div class="grouphdr">SEI messages (${msgs.length})</div>${rows}</div>`;
}

function renderNalDetails(nal, open) {
  const codec = nal.h264 || nal.h265 || {};
  const name = codec.nal_unit_type_name || 'nal';
  const size = nal.payload_size ?? 0;
  return `<details class="nal" data-nal="${nal.index}" data-off="${nal.payload_offset}" data-size="${size}"${open ? ' open' : ''}>
    <summary>
      <span class="tag" style="background:${NAL_TAG_COLOR(name)}">${name}</span>
      <span>#${nal.index}</span>
      <span style="color:var(--muted)">@${nal.payload_offset} · ${size} B</span>
    </summary>
    <div class="kvtable">${kvRows(codec)}</div>
    ${renderSei(codec.sei_messages)}
    <div class="kvtable"><div class="grouphdr">${t('hex')}</div></div>
    <pre class="hex" data-loaded="0">${t('loading')}</pre>
  </details>`;
}

// Lazily fetch a NAL's hex the first time its <details> opens. Reads the byte
// range directly via /api/hex (analyze already knows payload_offset/size), which
// is instant even on huge streams — unlike dump, which re-parses the whole file.
// Only the first 8 KB are shown by default; large NALs get a "show all" link.
async function loadHex(details, full) {
  const pre = details.querySelector('.hex');
  if (!pre) return;
  if (!full && pre.dataset.loaded === '1') return;
  pre.dataset.loaded = '1';
  pre.textContent = t('loading');
  const off = details.dataset.off, size = details.dataset.size;
  const limit = full ? size : 8192;
  try {
    const res = await fetch(`/api/hex?path=${encodeURIComponent(state.path)}&offset=${off}&size=${size}&limit=${limit}`);
    const d = await res.json();
    if (d.error) { pre.textContent = 'hex: ' + d.error; return; }
    pre.textContent = '';
    if (d.truncated) {
      const bar = document.createElement('div');
      bar.className = 'hex-more';
      bar.innerHTML = `${t('hex_shown')} ${fmtBytes(d.shown)} / ${t('hex_of')} ${fmtBytes(d.total)} · <a href="#" class="hex-all">${t('hex_all')}</a>\n`;
      bar.querySelector('.hex-all').addEventListener('click', (e) => { e.preventDefault(); loadHex(details, true); });
      pre.appendChild(bar);
    }
    pre.appendChild(document.createTextNode(d.hex));
  } catch (e) {
    pre.textContent = 'hex load failed: ' + e.message;
  }
}

// Delegate: lazily render an AU body / load NAL hex when a details opens.
document.addEventListener('toggle', (ev) => {
  const d = ev.target;
  if (d.tagName !== 'DETAILS' || !d.open) return;
  if (d.classList.contains('gop')) renderGopBody(d);
  else if (d.classList.contains('au')) renderAuBody(d);
  else if (d.classList.contains('nal')) loadHex(d);
}, true);

// Group all NALs into access units: each frame's slice(s) plus the parameter
// sets / SEI that precede it. Non-frame NALs before the first slice attach to
// the first AU; any trailing non-frame NALs attach to the last AU.
function buildAccessUnits(d) {
  const nals = d.nals || [];
  const frames = d.frames || [];
  const frameOfNal = {};
  frames.forEach(f => (f.nal_indices || []).forEach(ni => { frameOfNal[ni] = f.index; }));
  const aus = [];
  let pending = [];
  let cur = null;
  for (const nal of nals) {
    const fi = frameOfNal[nal.index];
    if (fi === undefined) {
      pending.push(nal);
    } else {
      if (!cur || cur.frameIndex !== fi) {
        cur = { frameIndex: fi, frame: frames[fi], nals: [] };
        aus.push(cur);
      }
      if (pending.length) { cur.nals.push(...pending); pending = []; }
      cur.nals.push(nal);
    }
  }
  if (pending.length) {
    if (cur) cur.nals.push(...pending);
    else aus.push({ frameIndex: null, frame: null, nals: pending });
  }
  return aus;
}

// Tree grouped by GOP: GOP nodes (collapsed) → frames (AU) → NALs. Lazy-rendered.
function renderStreamTree(d) {
  state.aus = buildAccessUnits(d);
  state.aus.forEach((au, i) => { au.idx = i; });
  const gops = d.gops || [];
  const byGop = {};
  state.aus.forEach(au => {
    const gi = au.frame && au.frame.gop_index != null ? au.frame.gop_index : '_';
    (byGop[gi] = byGop[gi] || []).push(au);
  });
  state.ausByGop = byGop;
  const expandAll = new URLSearchParams(location.search).get('expand') === '1';
  const head = `<div class="grouphdr">${d.codec_guess} · ${d.format} · ${gops.length} GOP · ${state.aus.length} ${t('frames_u')}</div>`;
  let rows = gops.map(g => `<details class="gop" data-gop="${g.index}"${expandAll ? ' open' : ''}>
      <summary><span class="tag" style="background:var(--key);color:#000">GOP</span>#${g.index} · ${t('gop_kf')} #${g.keyframe_index} · ${g.frame_count} ${t('frames_u')} · ${fmtBytes(g.size_bytes || 0)}</summary>
      <div class="gop-body"></div>
    </details>`).join('');
  if (byGop['_'] && byGop['_'].length) {
    rows += `<details class="gop" data-gop="_"${expandAll ? ' open' : ''}>
      <summary><span class="tag" style="background:#5a6472">${t('other')}</span>${t('ungrouped')} · ${byGop['_'].length}</summary>
      <div class="gop-body"></div>
    </details>`;
  }
  el('streamTree').innerHTML = head + rows;
  if (expandAll) el('streamTree').querySelectorAll('details.gop').forEach(renderGopBody);
  applyFilterToTree();
}

// Lazily render a GOP's frame (AU) list the first time it opens.
function renderGopBody(gopDetails) {
  if (gopDetails.dataset.rendered === '1') return;
  gopDetails.dataset.rendered = '1';
  const key = gopDetails.dataset.gop;
  const aus = state.ausByGop[key === '_' ? '_' : Number(key)] || [];
  const body = gopDetails.querySelector('.gop-body');
  if (body) body.innerHTML = aus.map(au => renderAuDetails(au)).join('');
  applyFilterToTree();
}

function renderAuDetails(au) {
  const f = au.frame;
  const type = f ? f.frame_type : '·';
  const color = TYPE_COLOR[type] || '#5a6472';
  const label = f
    ? `${t('frame_word')} #${f.index} (${type})${f.is_keyframe ? ' ★' : ''} · ${au.nals.length} ${t('nal_u')}`
    : `tail · ${au.nals.length} ${t('nal_u')}`;
  return `<details class="au" data-au="${au.idx}" data-ftype="${type}">
    <summary><span class="tag" style="background:${color}">${type}</span>${label}</summary>
    <div class="au-body"></div>
  </details>`;
}

// Hide access units whose frame type is filtered out (mirrors the timeline filter).
function applyFilterToTree() {
  el('streamTree').querySelectorAll('details.au').forEach((au) => {
    const t = au.dataset.ftype;
    au.style.display = (t && state.filter[t] === false) ? 'none' : '';
  });
}

// Lazily render an AU's NAL list the first time it opens.
function renderAuBody(auDetails) {
  if (auDetails.dataset.rendered === '1') return;
  auDetails.dataset.rendered = '1';
  const au = state.aus[Number(auDetails.dataset.au)];
  const body = auDetails.querySelector('.au-body');
  if (au && body) body.innerHTML = au.nals.map(n => renderNalDetails(n, false)).join('');
}

function renderNalPanel(frame) {
  // Prefer the frame's access unit (all NALs: param sets + SEI + slice).
  const au = (state.aus || []).find(a => a.frameIndex === frame.index);
  const nals = au ? au.nals : (frame.nal_indices || []).map(i => (state.analysis.nals || [])[i]);
  el('nalTitle').textContent = `(${t('frame_word')} #${frame.index} · ${nals.length} ${t('nal_u')})`;
  el('nalList').innerHTML = nals.map((n, k) => renderNalDetails(n, k === 0)).join('') ||
    '<span style="color:var(--muted)">no NALs</span>';
  el('nalList').querySelectorAll('details.nal[open]').forEach(loadHex); // initial-open
}

// Expand the current frame's GOP and highlight its AU in the structure tree.
function highlightTreeAU(frameIndex) {
  const frame = state.frames.find(f => f.index === frameIndex);
  if (frame && frame.gop_index != null) {
    const gopEl = el('streamTree').querySelector(`details.gop[data-gop="${frame.gop_index}"]`);
    if (gopEl && !gopEl.open) { gopEl.open = true; renderGopBody(gopEl); }
  }
  el('streamTree').querySelectorAll('details.au').forEach(au => {
    const d = state.aus[Number(au.dataset.au)];
    au.classList.toggle('au-current', !!d && d.frameIndex === frameIndex);
  });
}

// Parse a binary P6 PPM (from base64) into {w,h,rgb:Uint8Array}.
function parsePPM(bytes) {
  let p = 0;
  const readToken = () => {
    while (p < bytes.length && /\s/.test(String.fromCharCode(bytes[p]))) p++;
    let s = '';
    while (p < bytes.length && !/\s/.test(String.fromCharCode(bytes[p]))) s += String.fromCharCode(bytes[p++]);
    return s;
  };
  const magic = readToken();
  if (magic !== 'P6') throw new Error('not a P6 ppm');
  const w = parseInt(readToken(), 10);
  const h = parseInt(readToken(), 10);
  readToken(); // maxval
  p++;         // single whitespace after maxval
  return { w, h, rgb: bytes.subarray(p, p + w * h * 3) };
}

function b64ToBytes(b64) {
  const bin = atob(b64);
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out;
}

let lastDetail = null;

// Paint a base64 PPM onto the frame canvas; returns {w,h} or null.
function paintPPM(b64) {
  const canvas = el('frameCanvas');
  const ctx = canvas.getContext('2d');
  if (!b64) {
    canvas.width = 360; canvas.height = 200;
    ctx.fillStyle = '#000'; ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = '#888'; ctx.fillText('no image', 12, 20);
    return null;
  }
  const ppm = parsePPM(b64ToBytes(b64));
  canvas.width = ppm.w; canvas.height = ppm.h;
  const img = ctx.createImageData(ppm.w, ppm.h);
  for (let i = 0, j = 0; i < ppm.rgb.length; i += 3, j += 4) {
    img.data[j] = ppm.rgb[i]; img.data[j + 1] = ppm.rgb[i + 1];
    img.data[j + 2] = ppm.rgb[i + 2]; img.data[j + 3] = 255;
  }
  ctx.putImageData(img, 0, 0);
  return { w: ppm.w, h: ppm.h };
}

// Original decoded thumbnail + optional motion-vector overlay.
function drawFrame(detail) {
  lastDetail = detail;
  const dim = paintPPM(detail.thumb_ppm_base64);
  if (dim && el('mvToggle').checked && detail.motion_vectors && detail.motion_vectors.length) {
    drawMotionVectors(el('frameCanvas').getContext('2d'), detail, dim.w, dim.h);
  }
}

// Render the canvas for the selected layer: original thumbnail or a HEVC block layer.
async function renderCanvasForLayer() {
  const layer = el('layerSelect').value;
  if (layer === 'original') { if (lastDetail) drawFrame(lastDetail); return; }
  if (state.selected < 0) return;
  const f = state.frames[state.selected];
  setStatus('渲染块级图层 ' + layer + '…');
  try {
    const res = await fetch(`/api/block?path=${encodeURIComponent(state.path)}&index=${f.decode_order_index}&layer=${layer}`);
    const d = await res.json();
    if (d.error) { setStatus('块级图层失败(仅 HEVC / 需 libde265):' + d.error); return; }
    paintPPM(d.thumb_ppm_base64);
    setStatus('块级图层:' + layer + ' · frame #' + f.index);
  } catch (e) {
    setStatus('块级请求失败:' + e.message);
  }
}

// Motion vectors are in coded-pixel space; scale them onto the thumbnail.
function drawMotionVectors(ctx, detail, thumbW, thumbH) {
  const sx = thumbW / detail.coded_width;
  const sy = thumbH / detail.coded_height;
  ctx.lineWidth = 1;
  for (const mv of detail.motion_vectors) {
    const dx = mv.dst_x * sx, dy = mv.dst_y * sy;
    const ox = mv.src_x * sx, oy = mv.src_y * sy;
    if (Math.abs(ox - dx) < 0.4 && Math.abs(oy - dy) < 0.4) continue; // skip zero MVs
    ctx.strokeStyle = mv.source < 0 ? 'rgba(80,255,140,0.75)' : 'rgba(255,170,60,0.75)';
    ctx.beginPath();
    ctx.moveTo(dx, dy);
    ctx.lineTo(ox, oy);
    ctx.stroke();
  }
}

el('mvToggle').addEventListener('change', renderCanvasForLayer);
el('layerSelect').addEventListener('change', renderCanvasForLayer);

// Timeline hover tooltip: show the frame under the cursor.
const tooltip = el('tooltip');
el('timeline').addEventListener('mousemove', (ev) => {
  const rect = ev.target.getBoundingClientRect();
  const x = ev.clientX - rect.left;
  for (let i = 0; i < state.bars.length; i++) {
    if (x >= state.bars[i].x0 && x < state.bars[i].x1) {
      const f = state.frames[i];
      tooltip.innerHTML = `#${f.index} ${f.frame_type}${f.is_keyframe ? ' ★' : ''} · ${fmtBytes(f.size_bytes || 0)} · POC ${f.poc ?? '—'}`;
      tooltip.style.display = 'block';
      tooltip.style.left = (ev.clientX + 12) + 'px';
      tooltip.style.top = (ev.clientY + 12) + 'px';
      return;
    }
  }
  tooltip.style.display = 'none';
});
el('timeline').addEventListener('mouseleave', () => { tooltip.style.display = 'none'; });
el('load').addEventListener('click', analyze);

// Auto-load when a ?path= query param is present (shareable/deep links).
const _params = new URLSearchParams(location.search);
const initialPath = _params.get('path');
if (_params.get('layer')) el('layerSelect').value = _params.get('layer');
if (Number(_params.get('zoom')) >= 1) state.zoom = Number(_params.get('zoom'));
(_params.get('hide') || '').split(',').filter(Boolean).forEach((t) => {
  state.filter[t.toUpperCase()] = false;
});
syncFilterUI();
if (initialPath) { el('path').value = initialPath; analyze(); }
if (_params.get('browse') === '1') openBrowser();

// ---- Dockable panels: drag to reorder, close, restore via the 面板 menu -----

let dragKey = null;

function afterLayoutChange() {
  if (!state.analysis) return;
  drawTimeline();
  if (state.selected >= 0) renderFilmstrip(state.selected);
}

function panelName(p) {
  const h2 = p.querySelector('h2');
  const t = h2 ? h2.textContent.replace('⠿', '').trim() : p.dataset.key;
  return t.split(/[（(]/)[0].trim() || p.dataset.key;
}

function setPanelHidden(p, hidden) {
  p.style.display = hidden ? 'none' : '';
  const cb = el('panelsMenu').querySelector(`input[data-key="${CSS.escape(p.dataset.key)}"]`);
  if (cb) cb.checked = !hidden;
  savePanels();
  afterLayoutChange();
}

function savePanels() {
  const main = document.querySelector('main');
  const panels = [...main.querySelectorAll('.panel')];
  const order = panels.map(p => p.dataset.key);
  const hidden = panels.filter(p => p.style.display === 'none').map(p => p.dataset.key);
  try { localStorage.setItem('sv_panels', JSON.stringify({ order, hidden })); } catch (e) {}
}

function loadPanels() {
  let saved;
  try { saved = JSON.parse(localStorage.getItem('sv_panels') || 'null'); } catch (e) {}
  if (!saved) return;
  const main = document.querySelector('main');
  // If the panel set changed (a layout revision), ignore the stale saved layout.
  const curKeys = [...main.querySelectorAll('.panel')].map(p => p.dataset.key);
  const savedKeys = saved.order || [];
  if (curKeys.length !== savedKeys.length || curKeys.some(k => !savedKeys.includes(k))) return;
  (saved.order || []).forEach(key => {
    const p = main.querySelector(`.panel[data-key="${CSS.escape(key)}"]`);
    if (p) main.appendChild(p);
  });
  (saved.hidden || []).forEach(key => {
    const p = main.querySelector(`.panel[data-key="${CSS.escape(key)}"]`);
    if (p) p.style.display = 'none';
  });
  el('panelsMenu').querySelectorAll('input').forEach(cb => {
    const p = main.querySelector(`.panel[data-key="${CSS.escape(cb.dataset.key)}"]`);
    cb.checked = !!p && p.style.display !== 'none';
  });
}

function initPanels() {
  const main = document.querySelector('main');
  const panels = [...main.querySelectorAll('.panel')];
  panels.forEach((p, idx) => {
    p.dataset.key = p.id || ('panel-' + idx);
    const h2 = p.querySelector('h2');
    if (h2 && !h2.querySelector('.drag-handle')) {
      const handle = document.createElement('span');
      handle.className = 'drag-handle'; handle.textContent = '⠿'; handle.draggable = true; handle.title = '拖动重排';
      h2.prepend(handle);
      const close = document.createElement('button');
      close.className = 'panel-close'; close.textContent = '✕'; close.title = '关闭面板';
      close.addEventListener('click', () => setPanelHidden(p, true));
      p.appendChild(close);
      handle.addEventListener('dragstart', (e) => {
        dragKey = p.dataset.key; p.classList.add('dragging'); e.dataTransfer.effectAllowed = 'move';
      });
      handle.addEventListener('dragend', () => { p.classList.remove('dragging'); savePanels(); afterLayoutChange(); });
    }
    p.addEventListener('dragover', (e) => e.preventDefault());
    p.addEventListener('drop', (e) => {
      e.preventDefault();
      const dragged = main.querySelector(`.panel[data-key="${CSS.escape(dragKey || '')}"]`);
      if (dragged && dragged !== p) {
        const rect = p.getBoundingClientRect();
        const after = e.clientY > rect.top + rect.height / 2;
        main.insertBefore(dragged, after ? p.nextSibling : p);
        savePanels(); afterLayoutChange();
      }
    });
  });
  el('panelsMenu').innerHTML = panels.map(p =>
    `<label><input type="checkbox" data-key="${p.dataset.key}" ${p.style.display === 'none' ? '' : 'checked'}> ${panelName(p)}</label>`).join('');
  el('panelsMenu').querySelectorAll('input').forEach(cb => {
    cb.addEventListener('change', () => {
      const p = main.querySelector(`.panel[data-key="${CSS.escape(cb.dataset.key)}"]`);
      if (p) setPanelHidden(p, !cb.checked);
    });
  });
  loadPanels();
}

// Detail-column tabs (码流内容 / 码流总览 / 播放).
document.querySelectorAll('#colDetail .tab').forEach(tab => {
  tab.addEventListener('click', () => {
    document.querySelectorAll('#colDetail .tab').forEach(t => t.classList.toggle('sel', t === tab));
    document.querySelectorAll('#colDetail .tabpane').forEach(p => { p.hidden = p.dataset.pane !== tab.dataset.tab; });
  });
});

// Draggable column splitters (resize the left / right columns).
(function initSplitters() {
  const cols = document.getElementById('cols');
  let active = null, startX = 0, startW = 0;
  document.querySelectorAll('.splitter').forEach(sp => {
    sp.addEventListener('mousedown', (e) => {
      active = sp.dataset.split;
      startX = e.clientX;
      const v = active === 'L' ? '--wL' : '--wR';
      startW = parseInt(getComputedStyle(cols).getPropertyValue(v)) || (active === 'L' ? 280 : 400);
      document.body.style.userSelect = 'none';
      e.preventDefault();
    });
  });
  window.addEventListener('mousemove', (e) => {
    if (!active) return;
    const dx = e.clientX - startX;
    if (active === 'L') cols.style.setProperty('--wL', Math.max(160, startW + dx) + 'px');
    else cols.style.setProperty('--wR', Math.max(220, startW - dx) + 'px');
    drawTimeline();
  });
  window.addEventListener('mouseup', () => { active = null; document.body.style.userSelect = ''; });
})();

// View toggles: show/hide the timeline / tree / detail columns (frame column stays).
const colState = (() => {
  try { return JSON.parse(localStorage.getItem('sv_view')) || { timeline: true, tree: true, detail: true }; }
  catch (e) { return { timeline: true, tree: true, detail: true }; }
})();
function updateView() {
  document.getElementById('colTree').style.display = colState.tree === false ? 'none' : '';
  document.getElementById('colDetail').style.display = colState.detail === false ? 'none' : '';
  document.querySelector('.splitter[data-split="L"]').style.display = colState.tree === false ? 'none' : '';
  document.querySelector('.splitter[data-split="R"]').style.display = colState.detail === false ? 'none' : '';
  const parts = [];
  if (colState.tree !== false) parts.push('var(--wL)', '6px');
  parts.push('minmax(280px, 1fr)');
  if (colState.detail !== false) parts.push('6px', 'var(--wR)');
  document.getElementById('cols').style.gridTemplateColumns = parts.join(' ');
  document.querySelectorAll('.view-toggle .vt').forEach(b =>
    b.classList.toggle('sel', colState[b.dataset.view] !== false));
  try { localStorage.setItem('sv_view', JSON.stringify(colState)); } catch (e) {}
  if (state.analysis) { drawTimeline(); if (state.selected >= 0) renderFilmstrip(state.selected); }
}
document.querySelectorAll('.view-toggle .vt').forEach(b =>
  b.addEventListener('click', () => { colState[b.dataset.view] = colState[b.dataset.view] === false; updateView(); }));
updateView();

// Day / night theme.
function applyTheme(t) {
  document.documentElement.dataset.theme = t;
  el('themeBtn').textContent = t === 'light' ? '☀️' : '🌙';
  try { localStorage.setItem('sv_theme', t); } catch (e) {}
}
el('themeBtn').addEventListener('click', () =>
  applyTheme(document.documentElement.dataset.theme === 'light' ? 'dark' : 'light'));
applyTheme(new URLSearchParams(location.search).get('theme') || localStorage.getItem('sv_theme') || 'dark');

// Language toggle (中文 default / English).
if (new URLSearchParams(location.search).get('lang')) LANG = new URLSearchParams(location.search).get('lang');
el('langBtn').textContent = LANG === 'zh' ? 'EN' : '中';
el('langBtn').addEventListener('click', () => setLang(LANG === 'zh' ? 'en' : 'zh'));
applyStaticI18n();

// ---- Export & drag-drop ---------------------------------------------------
function downloadBlob(filename, blob) {
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url; a.download = filename; a.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}
el('exportFrame').addEventListener('click', () => {
  if (state.selected < 0) return;
  el('frameCanvas').toBlob(b => b && downloadBlob(`frame_${state.frames[state.selected].index}.png`, b));
});
el('exportJson').addEventListener('click', () => {
  if (!state.analysis) return;
  downloadBlob('streamview_analysis.json',
    new Blob([JSON.stringify(state.analysis, null, 2)], { type: 'application/json' }));
});

// Drag a file onto the window to open it. Desktop (Tauri) gives a real path; a
// plain browser can't read the path, so we hint the user to use "Open file".
if (window.__TAURI__ && window.__TAURI__.webview) {
  try {
    window.__TAURI__.webview.getCurrentWebview().onDragDropEvent((e) => {
      if (e.payload && e.payload.type === 'drop' && e.payload.paths && e.payload.paths.length) {
        state.path = e.payload.paths[0];
        el('path').value = state.path;
        analyze();
      }
    });
  } catch (e) {}
}
window.addEventListener('dragover', (e) => e.preventDefault());
window.addEventListener('drop', (e) => {
  e.preventDefault();
  const f = e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files[0];
  if (!f) return;
  if (f.path) { state.path = f.path; el('path').value = f.path; analyze(); }
  else if (!window.__TAURI__) setStatus(t('drop_hint'));
});
el('path').addEventListener('keydown', (e) => { if (e.key === 'Enter') analyze(); });
el('prev').addEventListener('click', () => selectFrame(state.selected - 1));
el('next').addEventListener('click', () => selectFrame(state.selected + 1));

// Arrow keys navigate frames from anywhere (left/up = prev, right/down = next),
// except while typing in a field. Timeline playhead, filmstrip and tree stay in
// sync via selectFrame.
document.addEventListener('keydown', (e) => {
  if (e.metaKey || e.ctrlKey || e.altKey) return;
  const tag = (e.target.tagName || '').toLowerCase();
  if (tag === 'input' || tag === 'select' || tag === 'textarea') return;
  if (state.selected < 0 || !state.frames.length) return;
  if (e.key === 'ArrowLeft' || e.key === 'ArrowUp') {
    e.preventDefault();
    selectFrame(state.selected - 1);
  } else if (e.key === 'ArrowRight' || e.key === 'ArrowDown') {
    e.preventDefault();
    selectFrame(state.selected + 1);
  }
});
el('frameSize').addEventListener('input', () => {
  document.documentElement.style.setProperty('--frameH', el('frameSize').value + 'px');
  try { localStorage.setItem('sv_frameH', el('frameSize').value); } catch (e) {}
});
(() => {
  const s = localStorage.getItem('sv_frameH');
  if (s) { el('frameSize').value = s; document.documentElement.style.setProperty('--frameH', s + 'px'); }
})();
el('filmstrip').addEventListener('click', (ev) => {
  const t = ev.target.closest('.thumb');
  if (t) selectFrame(Number(t.dataset.index));
});

// Click a frame's AU in the structure tree -> show that frame's content on the right.
el('streamTree').addEventListener('click', (ev) => {
  const summary = ev.target.closest('details.au > summary');
  if (!summary) return;
  const d = state.aus[Number(summary.parentElement.dataset.au)];
  if (d && d.frameIndex != null) {
    const arrIdx = state.frames.findIndex(fr => fr.index === d.frameIndex);
    if (arrIdx >= 0) selectFrame(arrIdx);
  }
});
window.addEventListener('resize', drawTimeline);

function setZoom(z) { state.zoom = Math.max(1, Math.min(30, z)); drawTimeline(); }
el('zoomIn').addEventListener('click', () => setZoom((state.zoom || 1) * 1.6));
el('zoomOut').addEventListener('click', () => setZoom((state.zoom || 1) / 1.6));
el('zoomReset').addEventListener('click', () => setZoom(1));

// Frame-type filter shared by the timeline legend and the tree filter chips.
function syncFilterUI() {
  document.querySelectorAll('.legend .ftype').forEach(e =>
    e.classList.toggle('off', state.filter[e.dataset.type] === false));
  document.querySelectorAll('.tree-filter .ftype2').forEach(e =>
    e.classList.toggle('sel', state.filter[e.dataset.type] !== false));
}
function toggleFilter(t) {
  state.filter[t] = state.filter[t] === false; // false→true, true/undef→false
  if (state.analysis) { drawTimeline(); applyFilterToTree(); }
  syncFilterUI();
}
document.querySelectorAll('.legend .ftype').forEach(elm =>
  elm.addEventListener('click', () => toggleFilter(elm.dataset.type)));
document.querySelectorAll('.tree-filter .ftype2').forEach(elm =>
  elm.addEventListener('click', () => toggleFilter(elm.dataset.type)));
