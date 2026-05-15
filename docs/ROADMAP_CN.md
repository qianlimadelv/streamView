# 路线图

## Phase 0：项目骨架

- CMake workspace
- CLI 应用
- Core 和 bitstream libraries
- 单元测试
- 项目文档

## Phase 1：H.264 Annex B Scanner

- 检测 3-byte 和 4-byte start code
- 提取 NAL unit offset 和 payload size
- 识别 H.264 NAL unit type
- 导出 JSON
- 增加畸形输入测试

## Phase 2：H.264 Parameter Sets

- Bit reader
- Emulation prevention byte removal
- Exp-Golomb decoding
- SPS/PPS 基础字段
- 宽高推导
- Slice header 基础字段
- `StreamAnalysis` MVP 数据模型
- CLI golden JSON 测试

## Phase 3：H.265 Elementary Streams

- 复用 Annex B scanner
- H.265 NAL header 解析
- VPS/SPS/PPS 识别
- SPS 基础字段解析
- H.265 宽高推导
- H.265 slice header 前缀字段
- VCL NAL 计数
- H.265 golden JSON 测试
- 后续补 VPS/PPS 字段解析、slice_type 和 POC

## Phase 4：Container Input

- FFmpeg demux adapter
- MP4 输入
- PTS/DTS 传递
- AVCC/HVCC 转换处理

## Phase 5：GUI 原型

- Qt 6 主窗口
- 文件打开
- Stream tree
- Frame/NAL details
- Timeline 和 charts

## Phase 6：深度分析

- Slice header 解析
- GOP 分类
- FrameAnalysis MVP
- GopAnalysis MVP
- QP 统计
- CTU/block 可行性研究
- 运动矢量可行性研究
