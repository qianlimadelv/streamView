# Codec 范围

## H.264 Phase 1

已支持：

- Annex B start code 扫描
- NAL unit 边界提取
- H.264 NAL header 解析
- NAL type 名称

## H.264 Phase 2

已支持：

- RBSP extraction
- Exp-Golomb decoding
- SPS 基础字段
- PPS 基础字段
- 宽高推导
- Slice header 基础字段
- SPS POC 基础字段：`pic_order_cnt_type`、`log2_max_pic_order_cnt_lsb_minus4`
- Slice header 上下文字段：`idr_pic_id`、`pic_order_cnt_lsb`
- slice 引用 PPS、PPS 引用 SPS 的基础校验

尚未支持：

- SPS/PPS 字段完整解析
- POC 推导
- 多 slice 帧聚合
- 完整合规性验证

标准来源：

- ITU-T H.264 7.3.2.1 Sequence parameter set RBSP syntax
- ITU-T H.264 7.3.2.3 Picture parameter set RBSP syntax
- ITU-T H.264 7.3.3 Slice header syntax

## H.265 Phase 1

已支持：

- 复用 Annex B 扫描
- H.265 NAL header 解析
- VPS/SPS/PPS 识别
- H.265 VPS 基础字段
- H.265 SPS 基础字段
- H.265 PPS 基础字段
- H.265 宽高推导
- H.265 slice header 前缀字段
- H.265 slice_type 基础解析
- H.265 slice 上下文字段：`dependent_slice_segment_flag`、`pic_output_flag`
- H.265 SPS POC 字段：`log2_max_pic_order_cnt_lsb_minus4`
- H.265 帧级 `poc` 导出（非 IRAP slice 且存在 `pic_order_cnt_lsb`）
- H.264 帧级 `poc` 导出（仅 `pic_order_cnt_type == 0`）
- VCL NAL 计数
- 畸形参数集和 slice 输入测试
- CLI golden JSON 测试

尚未支持：

- H.265 VPS 完整字段解析
- H.265 PPS 完整字段解析
- H.265 POC 推导

标准来源：

- ITU-T H.265 7.3.2.2.1 Video parameter set RBSP syntax
- ITU-T H.265 7.3.2.2.2 Sequence parameter set RBSP syntax
- ITU-T H.265 7.3.2.3 Picture parameter set RBSP syntax
- ITU-T H.265 7.3.6.1 General slice segment header syntax

## Parser 标准说明

新增语法字段时，需要在本文档中更新对应标准章节和用于验证的测试样本。
