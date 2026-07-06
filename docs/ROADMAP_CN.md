# 路线图

本路线图记录 StreamView 从当前的 CLI/解析器地基，走向对标 StreamEye 的可视化
码流分析器。Phase 0–4.5 已完成，仅作背景列出；Phase 5 及之后是对标 StreamEye
的差距规划，按两个大里程碑组织：

- **里程碑 A —「StreamEye Lite」：** 打开文件、浏览时间线、看逐帧类型/大小/码率
  图表、看解码缩略图、在 GUI 里查看 NAL 语法。路径现实、清晰。
- **里程碑 B —「完整 StreamEye」：** 宏块/CTU 级的 QP、预测、分区、运动矢量、
  残差可视化，以及更广的编解码器与容器覆盖。行业级难点，也是真正的差异化。

状态图例：✅ 已完成 · 🟡 部分 · ⬜ 未开始。

---

## Phase 0：项目骨架 ✅

- CMake workspace
- CLI 应用
- Core 和 bitstream libraries
- 单元测试
- 项目文档

## Phase 1：H.264 Annex B Scanner ✅

- 检测 3-byte 和 4-byte start code
- 提取 NAL unit offset 和 payload size
- 识别 H.264 NAL unit type
- 导出 JSON、畸形输入测试

## Phase 2：H.264 Parameter Sets ✅

- Bit reader、emulation prevention 去除、Exp-Golomb 解码
- SPS/PPS 基础字段、宽高推导
- Slice header 基础字段

## Phase 3：H.265 Elementary Streams ✅

- 复用 H.265 NAL scanner、VPS/SPS/PPS 基础解析
- 统一 stream 模型

## Phase 4：Container Input ✅

- FFmpeg demux adapter、MP4 输入
- PTS/DTS 传递、AVCC/HVCC 处理

## Phase 4.5：工程化 ✅

- Linux/macOS/Windows CI、跨平台构建文档
- Release artifact 结构、真实码流回归集

---

# 里程碑 A —「StreamEye Lite」

> **说明：** 里程碑 A 的 GUI 以**跨平台 Web 应用**（`apps/streamview-web`）实现，
> 而非 Qt——浏览器覆盖面更广，且 C++ 核心保持为分析/解码的唯一来源。原生 Qt
> 客户端仍是未来可选项。

## Phase 5：解码层（`sv-decode`）🟡

一切可视化的基础：把 packet/frame 变成可显示的图像。已交付：FFmpeg
`libavcodec` adapter、按需解码任意帧、RGB 缩略图、`streamview decode` CLI、运动
矢量导出、可选依赖构建。未做：帧缓存。

任务：
- 在接口后面封装 FFmpeg `libavcodec` adapter，像 `sv-demux` 一样隔离。
- 按需解码任意帧（按 decode-order index / PTS seek）。
- 输出可配置尺寸的 RGB/YUV 缩略图；缓存最近解码的帧。
- 把解码后的图像映射回现有的 `FrameAnalysis` 时间线。
- 保持解析核心在缺少 `libavcodec` 时仍可构建（可选依赖）。

验收：
- `streamview decode <file> --frame N --out thumb.png` 产出正确图像。
- 解码帧数量与顺序和解析器的 `frames[]` 一致。
- FFmpeg 缺失时仍能构建并通过测试（decode 命令报告「不可用」）。

难度：中。风险：FFmpeg 版本/API 漂移，adapter 要保持薄。

## Phase 6：GUI 原型（Web）🟡

第一个面向用户的应用：`apps/streamview-web`(零依赖 Node server + 浏览器前端),
封装分析模型 + Phase 5 解码。

已交付：
- 打开文件(Annex B + MP4);经 ffmpeg 转封装 + range 服务在浏览器内播放。
- 时间线：帧按类型(I/P/B)着色,柱高 ∝ 大小,关键帧标记,点击查看。
- 帧详情面板：类型、大小、POC、PTS/DTS、关键帧、NAL 数。
- 与选择联动的解码缩略图 + 运动矢量叠加(Phase 9a)。
- 汇总统计(编解码器、分辨率、计数、解析错误)。
- server 不含解析逻辑(复用 `streamview` CLI + `sv-decode`)。

未做：
- Stream tree(容器 → 流 → NAL/参数集)。
- 可展开解析字段的 NAL 语法面板(归到 Phase 7)。

已验证：headless 接口检查 + headless-Chrome 渲染真实 `.h264`(时间线顺序与
CLI JSON 一致;MV 叠加端点在界内)。

## Phase 7：完整语法覆盖 🟡

对齐 StreamEye「每个语法元素都可查看」的预期。

已交付(Web UI)：码流/参数集树 + 逐帧 NAL 语法面板(展开 `sv-analysis` 已产出的
解析后 header 及 SPS/PPS/VPS/slice 字段)+ 每个 NAL 的按需字节级 hex 视图 + SEI
消息解析(payload 类型 + 大小,列在树里)+ H.264 SPS VUI 解析(宽高比、色彩
primaries/transfer/matrix、timing/帧率)。

任务(剩余)：
- 完整的 SPS/PPS/VPS 字段解析（VUI、HRD、scaling list、extension）。
- SEI 消息解析（buffering period、pic timing、user data 等）。
- slice header 的 reference picture list / reorder / marking 字段。
- 完整 POC 推导（不只是导出 `pic_order_cnt_lsb`）。
- 多 slice 帧聚合。
- 十六进制视图 ↔ 语法树的位范围联动（高亮某字段对应的字节）。

验收：
- golden JSON 展开为完整字段集；每个解析元素补充样本。
- 在 GUI 里选中某语法字段时，高亮其精确的字节/位范围。

难度：中（是广度不是深度）。风险：标准一致性的边界情况。

## Phase 8：图表与统计 ⬜

把已经采集到的数据变成 StreamEye 风格的图。

任务：
- 码率随时间曲线、帧大小图表（数据已在 `FrameAnalysis` 中）。
- GOP 结构可视化；按类型的大小分布。
- 帧大小/类型过滤，从图表跳转到帧。
- 导出图表与统计（扩展 `sv-export`）。

验收：
- 图表与同一码流 CLI JSON 累加的 `size_bytes` 一致。
- 点击图表上的点选中对应的时间线帧。

难度：低偏中。风险：极小（数据已存在）。

---

# 里程碑 B —「完整 StreamEye」

## Phase 9：宏块 / CTU 级数据 🟡

StreamEye 的灵魂，也是最难的部分。

可行性验证：✅ **已完成** —— 见 `docs/PHASE9-FEASIBILITY_CN.md`。已在 FFmpeg 6.1
验证：公共 API **只导出运动矢量**；QP table 已被移除，MB 类型 / 分区 / 残差都
不暴露。决策：混合方案，拆成：
- **9a** ✅ —— 用 FFmpeg 公共 API 做 MV 叠加 + 解码缩略图。
- **9b** ✅（HEVC）—— 用 libde265 的 `draw_*` 辅助做 QP / CB 分区 / 帧内 / 运动叠加
  (可选依赖;`sv-decode` + `decode --block-layer` + Web 图层下拉)。这些是"画像素"、
  非结构化数值;要交互式读每块数值需自编 libde265 internals。
- **9c** ⬜ —— 经 JM/openh264 做 H.264 block 数据,或写一个聚焦的 CABAC/CAVLC 元数据
  解码器。避开打补丁改 FFmpeg 内部（脆弱的 fork）。

任务（9b/9c，按所选解码器）：
- 提取每 MB（H.264）/ 每 CTU+CU（H.265）的 QP、类型、分区、预测模式。
- 与解码图像几何对齐的 block-level map 数据模型 + 导出。
- GUI 叠加层：QP 热力图、预测模式图、分区网格叠在缩略图上。

验收：
- 已知编码器输出的 QP 叠加层与预期 QP 区间吻合。
- block 网格与解码帧像素级对齐。

难度：极高。风险：FFmpeg 可能暴露不足；熵解码是庞大且强标准依赖的工作。此阶段
可能主导整个项目的成本。

## Phase 10：运动矢量与残差 ⬜

任务：
- 逐 block 运动矢量，以箭头叠在解码图像上。
- 参考帧标识；前向/后向预测着色。
- 可获取处的残差/系数可视化。

验收：
- MV 叠加层的方向/幅度与已知测试片段的运动吻合。

难度：极高（依赖 Phase 9 的数据来源）。

## Phase 11：编解码器与容器扩展 ⬜

任务：
- 容器：MPEG-TS、MKV（经 FFmpeg `libavformat`）。
- 编解码器：逐步加 VP9 / AV1 / VVC / MPEG-2 解析 + 解码。
- 复用里程碑 A 框架的各编解码器语法面板。

验收：每个新编解码器/容器补 golden 回归样本。

难度：高（每个编解码器都是一个项目）；按用户需求排优先级。

## Phase 12：一致性与校验 ⬜

任务：
- 把 `validate` 从基础检查扩展为结构化的一致性规则。
- 错误/警告列表在 GUI 里链接到出问题的 NAL/帧。
- 缓冲模型（HRD）和参考完整性检查。

验收：精选的一致性失败码流产出预期诊断。

难度：中偏高。

---

## 排序建议

- Phase 5→6→8 能最快交付可用工具；在 Phase 7 的长尾之前先做。
- Phase 9 的可行性验证是整个里程碑 B 的闸门——即使实现推后，也要尽早跑，因为
  它决定整个技术路线。
- 每个阶段都保持 `ARCHITECTURE.md` 里「解析核心不依赖 Qt/FFmpeg」的规则不破。
