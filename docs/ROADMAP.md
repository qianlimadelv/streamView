# Roadmap

## Phase 0: Project Skeleton

- CMake workspace
- CLI application
- Core and bitstream libraries
- Unit tests
- Project documentation

## Phase 1: H.264 Annex B Scanner

- Detect 3-byte and 4-byte start codes
- Extract NAL unit offsets and payload sizes
- Identify H.264 NAL unit types
- Export JSON
- Add malformed input tests

## Phase 2: H.264 Parameter Sets

- Bit reader
- Emulation prevention byte removal
- Exp-Golomb decoding
- SPS/PPS baseline fields
- Width/height derivation

## Phase 3: H.265 Elementary Streams

- H.265 NAL scanner reuse
- VPS/SPS/PPS baseline parsing
- Unified stream model

## Phase 4: Container Input

- FFmpeg demux adapter
- MP4 input
- PTS/DTS propagation
- AVCC/HVCC conversion handling

## Phase 4.5: Engineering

- Linux GitHub Actions build/test
- Windows/macOS build instructions
- Release artifact structure
- Real-stream summary regression set

## Phase 5: GUI Prototype

- Qt 6 main window
- File open
- Stream tree
- Frame/NAL details
- Timeline and charts

## Phase 6: Deeper Analysis

- Slice header parsing
- GOP classification
- QP statistics
- CTU/block feasibility study
- Motion vector feasibility study
