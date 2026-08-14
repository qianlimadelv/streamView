# Phase 9 可行性调研：宏块 / CTU 级数据来源

**问题：** 要做 StreamEye 级的 block 级可视化（QP 图、预测模式、分区网格、运动
矢量、残差），哪些能从 FFmpeg 公共 API 拿到，哪些必须自己做熵解码？

**方法：** 实证——检查本机已安装的 FFmpeg 头文件 + 用生成的 H.264 片段实际抽取
数据。以下每条结论都在本机验证过，不是凭记忆。

## 测试环境

- FFmpeg 6.1.1（Ubuntu），`libavcodec 60.31`、`libavutil 58.29`、`libavformat 60.16`
- 头文件齐全：`libavutil/motion_vector.h`、`frame.h`、`libavcodec/avcodec.h`
- `codecview` filter 可用。

## 按数据类型的结论

| Block 级数据 | FFmpeg 6.1 公共 API | 证据 |
|-------------|--------------------|------|
| **运动矢量** | ✅ **完全可得** | `AV_FRAME_DATA_MOTION_VECTORS` side data + `AV_CODEC_FLAG2_EXPORT_MVS`。已验证：测试片段 25 帧中 23 帧带 MV side data（I 帧无、P/B 有）。`codecview` 成功渲染出箭头叠加层。 |
| **每 block 的 QP** | ❌ **已移除** | 旧的 `qscale_table` / `av_frame_get_qp_table()` / `AV_FRAME_DATA_QP_TABLE_*` 在 libavutil 58 里**已彻底删除**（grep 一无所获）。只剩 `FF_DEBUG_QP`——一个日志/绘制的 debug 标志，不是结构化数据。 |
| **MB 类型 / 预测模式** | ❌ **未暴露** | 只有 `FF_DEBUG_MB_TYPE`（画到帧上/打日志）。没有 frame side-data 类型，没有结构化访问接口。 |
| **分区 / CU-CTU 结构** | ❌ **未暴露** | 无公共 API。 |
| **残差 / 系数** | ❌ **未暴露** | 任何版本都没有公共 API。 |

### 我们能拿到的每 block MV 字段（`AVMotionVector`）

`source`（参考方向，±过去/未来）、block `w`/`h`、`src_x/src_y`、`dst_x/dst_y`、
`motion_x/motion_y`、`motion_scale`。足以画出带 block 尺寸和参考方向的逐 block
箭头——也就是 StreamEye 的 MV 叠加层。

### 编解码器注意事项(已验证)

MV 导出与编解码器相关:FFmpeg 的 **H.264** 解码器会填充
`AV_FRAME_DATA_MOTION_VECTORS`,但本机构建的 **HEVC** 解码器**不会**(ffprobe
对 H.265 片段显示无 MV side data)。所以 FFmpeg 给的这唯一一项 block 级数据在实践
中也只有 H.264 才有——更加说明 HEVC 的 block 数据必须来自 libde265(Phase 9b)。

### 关键的否定结论

现代 FFmpeg 公共 API 在 block 级**只导出运动矢量，别的都没有**。QP table 多年前
就被废弃并彻底删除；MB 类型、分区/CU 结构、残差从来就不是公共的。`FF_DEBUG_*`
标志和 `codecview` 能把其中一些**画进像素**，但不会把结构化的、每 block 的数值
交还给我们，我们无法据此做自己的叠加层/统计。

## 策略选项

### 选项 A — 只用 FFmpeg 公共 API
- **能拿到：** 解码图像（Phase 5）+ **运动矢量**。
- **缺：** QP 图、预测/分区图、残差。
- **成本：** 低；**维护：** 低（API 稳定）。
- **结论：** 能覆盖 Phase 10 的 MV 叠加和缩略图，但**给不了** StreamEye block
  视图的核心——QP/模式/分区图。

### 选项 B — Patch / fork FFmpeg 暴露内部数据
- 通过打补丁的构建或对着 FFmpeg 内部编译的 shim，伸进 `H264Context` /
  `HEVCContext` 的内部表（mb_type、qp、cbp……）。
- **能拿到：** FFmpeg 内部算出来的大部分 block 元数据。
- **成本：** 中偏高；**维护：** **高且脆弱**——内部结构体在 FFmpeg 版本间会变，
  你要永远维护一个 fork。残差依然别扭。
- **结论：** 对长期产品是陷阱。除非走投无路，别用。

### 选项 C — 采用参考 / 分析友好的解码器
- **HEVC：** `libde265` 暴露 CB/PB/intra-mode/MV 内部数据；它自带的 `sherlock265`
  查看器已经在可视化 CTB/CU/PU/intra 模式/MV——正是我们 Phase 9/10 目标的直接
  先例。
- **H.264：** JM 参考解码器（内部数据完整，慢）或改造过的 `openh264`。
- **能拿到：** 完整 block 元数据，含 QP、模式、分区、MV，以及通往残差的路径。
- **成本：** 高（集成/适配第二个解码器）；**维护：** 中（这些 API 更偏分析用途、
  比 FFmpeg 内部更稳定）。
- **结论：** 通往*完整* StreamEye 对标的现实路线，按编解码器逐个来。

### 选项 D — 自己写熵解码器（CABAC/CAVLC）
- **成本：** 极高（每个编解码器数月，强标准依赖）；**掌控力：** 完全。
- **结论：** 只在选项 C 不够用时才考虑。不建议作为起点。

## Spike 结果:libde265 `draw_*` 路径(已验证)

为评估选项 C(libde265)成本,跑了一个最小 spike。结论均在本机验证(Debian
libde265 1.0.15):

- **公共** `de265.h` 只给解码图像,没有块级数据——和 FFmpeg 一样。
- 但 libde265 的 `visualize.h` 暴露了半官方的 `draw_*` 辅助函数——
  `draw_CB_grid`、`draw_intra_pred_modes`、`draw_QuantPY`(QP)、`draw_Motion`、
  `draw_PB_pred_modes`、`draw_Slices`、`draw_Tiles`——而且**系统预编译的 `.so`
  已经全部导出**。
- 它们接收公共 API 返回的同一个 `de265_image*`,所以可以自己声明原型,绕开内部
  `image.h`。
- **无需 sudo、无需自编、无需 fork**:运行时库已在(被 libheif 带装);头文件用
  `apt-get download` + `dpkg-deb -x` 取得(不需要 root)。spike 对着解包出的公共
  头编译、链接系统 `.so`。
- 结果:解码真实 `.h265` 并调用这些函数,产出正确的 **HEVC** 叠加图——CB 四叉树
  分区网格、帧内预测方向、逐块 QP 热力图(14 个不同 QP 档)、运动矢量。这正是
  StreamEye 的块级视图,且是 FFmpeg 覆盖不了的编解码器。

局限(如实):
- `draw_*` 是"画像素",**不返回结构化数值**。要做交互式"悬停读出该 CB 的 QP=32"、
  自定义配色、图例,仍需结构化 internals API——而预编译库**未导出**它,得自编
  libde265 并开启 internals。
- 这些辅助是半官方("TODO: 挪到 sherlock265 或公共 API")——需锁定 libde265 版本
  或构建时探测符号是否存在。
- 仅 HEVC。H.264 块级仍只能靠 FFmpeg 的 MV,或另找路径。

**成本重估:** HEVC 块级*可视化*("看图版")**远低于最初评估**——只是一个小集成
(在解码路径里调 `draw_*`,Web UI 加图层切换),不是数月的自编工程。真正贵的是
结构化数值(自编 internals)和 H.264 块级数据。

## A-spike:H.264 块级数据(已验证)

与 HEVC 不同,H.264 **没有低成本路径**。在 FFmpeg 6.1 验证:

- `codecview` 有 `qp` 和 `block`(分区)选项,但对 H.264 都产出**零像素叠加**
  (QP table 已从 libavutil 移除;block/mb_type 数据不导出)。只有 `mv` 有效——
  已被 Phase 9a 使用。
- openh264(本机已有)不提供块级可视化 API。
- H.264 没有 libde265 `draw_*` 那样的对应物。

所以 H.264 块级(QP / 分区 / 帧内)只能选:魔改 FFmpeg `H264Context` 内部
(脆弱 fork)、集成 JM 参考解码器(慢、难嵌入)、或自写 CABAC/CAVLC 解码
(极重)。都远比 HEVC 路径贵。**建议:推迟 9c**——相比已完成的 HEVC 块级和其他
路线图工作,性价比差。

## 建议

**分阶段的混合路线——不要用一个工具解决所有问题：**

1. **Phase 9a（尽快，低成本）：** 用 **FFmpeg 公共 API 交付 MV 叠加 + 解码缩略图**
   （选项 A）。这在 MV 上已经很像 StreamEye 且便宜。今天已在本机验证可行。
2. **Phase 9b（真正的 block 视图）：** **先集成 libde265 做 HEVC**（分析导向的 API
   最好 + `sherlock265` 先例）拿到 QP / 预测 / 分区图。先在一个编解码器上跑通叠加
   层交互，再铺开。
3. **Phase 9c：** 经 JM/openh264 扩展到 H.264 的 block 数据（或到那时评估：一个
   聚焦的 CABAC/CAVLC 元数据解码器是不是反而更省）。
4. **避开选项 B**（打补丁改 FFmpeg 内部）用于任何长期的东西。

### 对路线图的影响

- 把 Phase 9 拆成 **9a（FFmpeg MV，便宜，可和里程碑 A 一起做）** 和
  **9b/9c（逐解码器的 block 元数据，真正的成本中心）**。
- QP/模式/分区图**不是**改个 FFmpeg 配置就能有——它们需要集成第二个解码器。要按
  此做预算；这仍然是项目最大的技术风险和成本，与路线图里标注的一致。
- 排序建议：把 **MV 叠加（9a）** 提前到 Phase 8 旁边，因为它成本低、视觉冲击大；
  把 9b/9c 推后到 GUI 外壳和一个编解码器的 block 流水线跑通之后。
