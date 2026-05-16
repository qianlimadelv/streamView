# StreamView CLI

当前 CLI 是 GUI 层之前的 MVP 入口，目标是先把码流分析、结构化导出、局部
对象检查和错误排查做成可测试、可脚本化的工具链。

它可以直接处理 raw Annex B 输入；如果编译时启用了 FFmpeg demux，还能处理
MP4 输入。

## 命令

```bash
streamview analyze <input> [--format text|json|csv] [--output <path|->] [--codec auto|h264|h265]
                   [--json <output.json>] [--json-mode full|summary] [--limit-nals <count>]
streamview inspect <input> --nal <index>|--frame <index>|--gop <index>
streamview errors <input> [--json]
streamview validate <input> [--json]
streamview dump <input> --nal <index> [--format hex|payload|rbsp] [--output <path|->]
```

## Analyze

- 默认输出为 stdout 上的文本摘要。
- `--format csv` 会输出一行 CSV 摘要，适合表格或 CI 场景。
- `--format json --output -` 会把 JSON 打到 stdout。
- `--output <path>` 会把当前选择的格式写入文件。
- `--json <path>` 作为兼容写法保留，等价于 `--format json --output <path>`。
- `--json-mode summary` 只输出顶层摘要，适合大码流快速统计。
- `--limit-nals <count>` 可以截断 full JSON 中的 NAL 明细，避免输出过大。
- `--codec auto|h264|h265` 用于覆盖基于扩展名的自动识别。

## Inspect

`inspect` 输出单个 NAL、frame 或 GOP 的 JSON。NAL inspect 会尽量输出当前已解析
的 codec syntax 字段、parse error 文本以及引用该 NAL 的 frame 索引。Frame
inspect 会输出所属 GOP 索引。

## Errors

`errors` 只输出 parser 失败项，适合快速判断真实码流是否存在当前解析器能识别
的问题。脚本场景建议使用 `--json`。

## Validate

`validate` 基于分析模型执行轻量结构检查。当前会报告 parser 错误、空 Annex B
输入、缺失 frame/keyframe、缺失参数集，以及重复的 SPS/PPS/VPS id 等问题。CI
或脚本场景建议使用 `--json`。如果存在 frame/GOP 元数据，还会检查基础一致性。
如果同一分析中解析到的 SPS 分辨率不一致，也会给出 warning。

## Dump

`dump` 用于从 Annex B 码流中导出单个 NAL：

- `--format hex`: 人可读的十六进制视图，默认格式。
- `--format payload`: 原始 NAL payload 字节，不包含 Annex B start code。
- `--format rbsp`: 移除 emulation-prevention byte 之后的 RBSP 字节。
- `--output <path|->`: 写入文件，或用 `-` 输出到 stdout。

## 退出码

- `0`: 命令成功；`errors` 没有发现 parse error。
- `1`: 命令行参数错误。
- `2`: 输入、输出、分析流程或 inspect 查询失败。
- `3`: `errors` 命令执行成功，但发现 parse error。
