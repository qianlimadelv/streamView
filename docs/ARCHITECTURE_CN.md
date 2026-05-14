# 架构

## 概览

StreamView 拆分为小型库和应用：

```text
apps/
  streamview-cli        命令行分析器
  streamview-gui        后续 Qt 桌面 UI

libs/
  sv-core               通用数据模型和错误类型
  sv-bitstream          原始字节流和 NAL 扫描
  sv-analysis           帧/GOP/统计分析
  sv-demux              后续 FFmpeg demux adapter
  sv-decode             后续 FFmpeg decode adapter
  sv-export             JSON/CSV 导出辅助
```

## 数据流

```text
输入文件
  -> raw reader 或 demux adapter
  -> bitstream scanner
  -> codec parser
  -> stream model
  -> analysis/export/UI
```

## 设计规则

- Codec parser 不能依赖 Qt。
- Codec parser 不能依赖 FFmpeg。
- FFmpeg adapter 可以提供 packet 和 decoded preview，但 parser core 必须保持独立可测试。
- CLI 输出是测试和未来 GUI 集成的第一个稳定 API。

## 未来组件

- `sv-demux`：通过 FFmpeg `libavformat` 支持 MP4/TS/MKV 输入。
- `sv-decode`：通过 FFmpeg `libavcodec` 支持 decoded frame preview。
- `streamview-gui`：Qt 6 桌面 UI，用于 timeline、frame details 和 preview。
