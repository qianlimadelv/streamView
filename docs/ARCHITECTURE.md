# Architecture

## Overview

StreamView is split into small libraries and applications:

```text
apps/
  streamview-cli        command-line analyzer
  streamview-gui        future Qt desktop UI

libs/
  sv-core               shared data models and errors
  sv-bitstream          raw byte stream and NAL scanning
  sv-analysis           frame/GOP/statistics analysis
  sv-demux              future FFmpeg demux adapter
  sv-decode             future FFmpeg decode adapter
  sv-export             JSON/CSV export helpers
```

## Data Flow

```text
Input file
  -> raw reader or demux adapter
  -> bitstream scanner
  -> codec parser
  -> stream model
  -> analysis/export/UI
```

## Design Rules

- Codec parsers must not depend on Qt.
- Codec parsers must not depend on FFmpeg.
- FFmpeg adapters may provide packets and decoded previews, but the parser core
  remains independently testable.
- CLI output is the first stable API for tests and future GUI integration.

## Future Components

- `sv-demux`: MP4/TS/MKV input through FFmpeg `libavformat`.
- `sv-decode`: decoded frame preview through FFmpeg `libavcodec`.
- `streamview-gui`: Qt 6 desktop UI for timeline, frame details, and preview.
