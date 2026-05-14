# Product Requirements

## Product Goal

Build a cross-platform compressed video bitstream analysis tool for engineers
working with H.264/H.265 streams.

The long-term product should help users inspect stream structure, diagnose
encoder output, compare GOP/frame-level statistics, and eventually visualize
codec internals such as QP, CTU/block structure, motion vectors, and residuals.

## MVP Goal

The MVP is a CLI-first prototype that can inspect H.264/H.265 elementary streams
and emit stable JSON/CSV output for tests and future GUI use.

## MVP Users

- Video codec engineers
- QA engineers validating encoder output
- Developers integrating FFmpeg or hardware encoders
- Students learning compressed video structure

## MVP Features

- Analyze `.h264` Annex B streams.
- Analyze `.h265` Annex B streams after H.264 baseline is stable.
- List NAL units with offsets, sizes, and types.
- Parse parameter sets incrementally.
- Export JSON suitable for golden tests.
- Provide robust errors for malformed inputs.

## Non-Goals For MVP

- Full standards compliance validation
- Real-time stream monitoring
- VMAF/SSIM/PSNR
- Motion vector visualization
- CTU/block visualization
- Production GUI
- Full MP4/TS/MKV demuxing

## Success Criteria

- `streamview analyze input.h264 --json out.json` produces deterministic output.
- Unit tests cover Annex B scanning edge cases.
- Malformed inputs fail safely.
- The architecture supports adding H.265, FFmpeg demuxing, and Qt GUI without
  rewriting the parser core.
