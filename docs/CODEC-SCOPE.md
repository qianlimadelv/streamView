# Codec Scope

## H.264 Phase 1

Supported:

- Annex B start code scanning
- NAL unit boundaries
- H.264 NAL header parsing
- NAL type names

## H.264 Phase 2

Supported:

- RBSP extraction
- Exp-Golomb decoding
- SPS baseline fields
- PPS baseline fields
- Width/height derivation
- Slice header baseline fields
- SPS POC fields: `pic_order_cnt_type`, `log2_max_pic_order_cnt_lsb_minus4`
- Contextual slice fields: `idr_pic_id`, `pic_order_cnt_lsb`
- Basic slice-to-PPS and PPS-to-SPS reference checks

Not yet supported:

- Full SPS/PPS field parsing
- POC derivation
- Multi-slice frame aggregation
- Full compliance validation

Standards:

- ITU-T H.264 7.3.2.1 Sequence parameter set RBSP syntax
- ITU-T H.264 7.3.2.3 Picture parameter set RBSP syntax
- ITU-T H.264 7.3.3 Slice header syntax

## H.265 Phase 1

Supported:

- Annex B scanning reuse
- H.265 NAL header parsing
- VPS/SPS/PPS identification
- H.265 VPS baseline fields
- H.265 SPS baseline fields
- H.265 PPS baseline fields
- H.265 width/height derivation
- H.265 slice header prefix fields
- H.265 slice_type baseline parsing
- H.265 contextual slice fields: `dependent_slice_segment_flag`, `pic_output_flag`
- H.265 SPS POC field: `log2_max_pic_order_cnt_lsb_minus4`
- H.265 frame-level `poc` export for non-IRAP slices with `pic_order_cnt_lsb`
- H.264 frame-level `poc` export for `pic_order_cnt_type == 0`

Not yet supported:

- H.265 VPS full field parsing
- H.265 PPS full field parsing
- H.265 POC derivation

Standards:

- ITU-T H.265 7.3.2.2.1 Video parameter set RBSP syntax
- ITU-T H.265 7.3.2.2.2 Sequence parameter set RBSP syntax
- ITU-T H.265 7.3.2.3 Picture parameter set RBSP syntax
- ITU-T H.265 7.3.6.1 General slice segment header syntax

## Parser Standards Notes

When adding syntax fields, update this document with the relevant standard
section and test samples used for validation.
