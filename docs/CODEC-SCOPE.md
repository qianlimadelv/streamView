# Codec Scope

## H.264 Phase 1

Supported:

- Annex B start code scanning
- NAL unit boundaries
- H.264 NAL header parsing
- NAL type names

Not yet supported:

- SPS/PPS field parsing
- Slice header parsing
- POC derivation
- Frame reconstruction
- Compliance validation

## H.264 Phase 2

Planned:

- RBSP extraction
- Exp-Golomb decoding
- SPS baseline fields
- PPS baseline fields
- Width/height derivation

## H.265 Phase 1

Planned after H.264 scanner stabilizes:

- Annex B scanning reuse
- H.265 NAL header parsing
- VPS/SPS/PPS identification

## Parser Standards Notes

When adding syntax fields, update this document with the relevant standard
section and test samples used for validation.
