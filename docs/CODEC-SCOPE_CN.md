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

尚未支持：

- SPS/PPS 字段完整解析
- POC 推导
- 多 slice 帧聚合
- 合规性验证

## H.265 Phase 1

已支持：

- 复用 Annex B 扫描
- H.265 NAL header 解析
- VPS/SPS/PPS 识别
- H.265 SPS 基础字段
- H.265 PPS 基础字段
- H.265 宽高推导
- H.265 slice header 前缀字段
- H.265 slice_type 基础解析
- VCL NAL 计数
- CLI golden JSON 测试

尚未支持：

- H.265 VPS 字段解析
- H.265 PPS 完整字段解析
- H.265 POC 推导

## Parser 标准说明

新增语法字段时，需要在本文档中更新对应标准章节和用于验证的测试样本。
