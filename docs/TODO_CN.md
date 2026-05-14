# 待办事项

这个文件作为 StreamView 的工作清单。后续所有研发进展优先按这里的条目推进和更新。

## 状态说明

- `[x]` 已完成
- `[ ]` 未开始
- `[~]` 进行中

## 当前已完成

- `[x]` H.264 Annex B 扫描
- `[x]` H.264 NAL header 解析
- `[x]` H.264 SPS 基础解析
- `[x]` H.264 PPS 基础解析
- `[x]` H.264 slice header 基础解析
- `[x]` `StreamAnalysis` MVP 数据模型
- `[x]` CLI JSON 导出
- `[x]` golden JSON 测试
- `[x]` 中文项目文档
- `[x]` 可选 FFmpeg demux 模块骨架

## 下一步

### 1. 验证 MP4 输入

- `[ ]` 在安装 FFmpeg 开发库后编译 `sv-demux`
- `[ ]` 用真实 `.mp4` 样本验证 H.264 demux 到 Annex B
- `[ ]` 为 demux 结果增加集成测试

验收标准：

- `.mp4` 输入可以走分析流程
- 输出 JSON 与 raw `.h264` 共享同一套 `StreamAnalysis`
- 出错时给出明确错误信息

### 2. 补 H.265 基线

- `[ ]` 增加 H.265 NAL header 解析
- `[ ]` 增加 H.265 VPS/SPS/PPS 基础解析
- `[ ]` 让 `sv-analysis` 支持 H.265 `StreamAnalysis`
- `[ ]` 增加 H.265 golden 样本和测试

验收标准：

- `.h265` 裸流可以输出 `stream_summary`
- H.265 输出结构与 H.264 保持一致的顶层模型

### 3. 扩展帧级分析

- `[ ]` 引入 frame model
- `[ ]` 记录 GOP 结构
- `[ ]` 统计 I/P/B/IDR 等关键帧信息
- `[ ]` 补充时间戳字段，为容器输入做准备

验收标准：

- 输出里能按帧查看结构
- 可以区分帧级和 NAL 级信息

### 4. 增强测试体系

- `[ ]` 增加更多合法样本
- `[ ]` 增加畸形输入测试
- `[ ]` 增加 golden 覆盖主要输出字段
- `[ ]` 增加跨平台构建检查

验收标准：

- 任何 parser 改动都能被测试捕获
- 输出变更可追踪、可解释

## 后续

### 5. GUI 原型

- `[ ]` Qt 6 窗口骨架
- `[ ]` 文件打开
- `[ ]` stream tree
- `[ ]` NAL / frame detail 面板
- `[ ]` timeline / chart 视图

### 6. 深度 codec 分析

- `[ ]` 更完整的 slice header 解析
- `[ ]` POC / reference frame 解析
- `[ ]` QP 统计
- `[ ]` CTU/block 可视化可行性研究
- `[ ]` 运动矢量可视化可行性研究

### 7. 构建与发布

- `[ ]` 补齐 Windows / macOS / Linux 构建文档
- `[ ]` 补齐依赖安装说明
- `[ ]` 定义 release 流程
- `[ ]` 定义版本号和打包方式

## 工作原则

- 一次只推进一个待办项或一个很小的子项。
- 每次改动后都要跑相关测试。
- 如果输出结构变了，要同步更新 golden 文件。
- 如果待办事项完成，要把 `[ ]` 改成 `[x]`，并补简短说明。
