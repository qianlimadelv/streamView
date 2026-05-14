# 产品需求

## 产品目标

构建一个跨平台的压缩视频码流分析工具，面向处理 H.264/H.265 码流的工程师。

长期目标是帮助用户检查码流结构、诊断编码器输出、比较 GOP/帧级统计信息，并逐步支持 QP、CTU/block 结构、运动矢量、残差等 codec 内部信息的可视化。

## MVP 目标

MVP 是一个 CLI 优先的原型，可以检查 H.264/H.265 elementary stream，并输出稳定的 JSON/CSV，供测试和后续 GUI 使用。

## MVP 用户

- 视频 codec 工程师
- 验证编码器输出的 QA 工程师
- 集成 FFmpeg 或硬件编码器的开发者
- 学习压缩视频结构的学生

## MVP 功能

- 分析 `.h264` Annex B 码流。
- 在 H.264 基线稳定后分析 `.h265` Annex B 码流。
- 列出 NAL unit 的 offset、size 和 type。
- 增量解析 parameter set。
- 导出适合 golden tests 的 JSON。
- 对畸形输入提供稳健错误。

## MVP 非目标

- 完整标准合规验证
- 实时流监控
- VMAF/SSIM/PSNR
- 运动矢量可视化
- CTU/block 可视化
- 生产级 GUI
- 完整 MP4/TS/MKV demuxing

## 成功标准

- `streamview analyze input.h264 --json out.json` 可以产生确定性的输出。
- 单元测试覆盖 Annex B 扫描边界情况。
- 畸形输入可以安全失败。
- 架构支持后续加入 H.265、FFmpeg demuxing 和 Qt GUI，而不需要重写 parser core。
