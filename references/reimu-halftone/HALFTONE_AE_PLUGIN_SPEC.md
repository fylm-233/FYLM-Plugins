# 封面印刷网纹（Halftone）算法规格 —— AE 插件实现说明

> **读者**：另一个项目的 codex / 插件开发者。本文自包含，不需要阅读原项目源码即可实现。
> **源实现**：`reimuplayer/src/gui/halftone.rs`（Rust，egui 桌面播放器内，8-bit RGBA 纯字节层）。
> **结论先行**：核心算法 `apply_halftone` **零第三方依赖、纯函数、确定性**，可 100% 独立抽取。

---

## 0. 一句话概括

对一张已裁成正方形的封面图，叠加**三层印刷质感**：① 按局部亮度调制的黑色调幅（AM）网点；
② 两层错位的品红/青色套印网点（模拟印刷套印不准）；③ 轻微的全局降饱和 + 提对比。
点距 2.5px（在 96×96 烘焙尺寸下），网点密度由「单元平均亮度」驱动。

---

## 1. 可抽取性判定

| 层 | 函数 | 依赖 | 可抽取 |
|---|---|---|---|
| **核心算法** | `apply_halftone(rgba, w, h)` | **无**（纯 std 数学，无 `use`） | ✅ 直接搬，逐行等价 |
| 图片预处理 | `halftone_cover_bytes(bytes, size)` | `image 0.25`（png/jpeg 解码 + Triangle 缩放） | ✅ 可换成本平台解码器 |
| 程序化占位封面 | `procedural_cover(seed, w, h)` | **无** | ✅ 可选，与主算法无关 |

**判定：可以独立抽取。** 三个耦合点都可剥离：

1. `COVER_PX = 96` 是**布局决策**（保证各处封面网点密度一致），不是算法必需。AE 端应改成参数。
2. `image` crate 只出现在解码包装里，核心算法不认识任何图片格式。
3. 没有任何全局状态、随机数、时间、I/O —— 同一输入必得同一输出（原项目有单测钉住）。

⚠ **唯一不可"重写得更干净"的地方**：取样窗口的 `floor/ceil` 取整与**顺序依赖**（见 §8）。
用"标准 box filter"重写会得到近似但**非逐字节相同**的结果。

---

## 2. 数据契约

### 输入
- `rgba: u8[]`，长度必须 `= W * H * 4`，通道顺序 **R,G,B,A**，**非预乘**，sRGB **非线性** 8-bit。
- `W`, `H`：像素尺寸。原项目固定 `W = H = 96`；`W==0 || H==0` 直接返回。
- 尺寸不匹配时原实现 `assert` 失败（AE 端应改为静默跳过或报错，不要崩）。

### 输出
- **原地修改**同一个 `rgba` 缓冲区（AE 端写输出 world）。
- **Alpha 通道完全不变**（有单测钉住）。RGB 被改写。
- 输出仍是 8-bit sRGB 非线性。

### 色彩约定（非常重要）
- 全程**不做**线性化 / gamma 解码。所有运算在 **sRGB 编码值 0–255** 上直接做。
- 亮度权重 Rec.709：`0.2126 R + 0.7152 G + 0.0722 B`。
- 若 AE 工程开启色彩管理（线性工作空间），必须先转到 sRGB 非线性 8-bit 再算，否则结果完全不同。

---

## 3. 管线总览

```
[源图] ──解码──> [居中裁正方形] ──Triangle 缩放──> [W×H RGBA8]
                                                        │
                        ┌───────────────────────────────┘
                        ▼
        ① 黑网点（AM halftone, Multiply, 逐格顺序）
                        ▼
        ② 套印偏色 M 层（Screen, 5px 网格, 偏移 0）
                        ▼
        ③ 套印偏色 C 层（Screen, 5px 网格, 偏移 +2.5,+2.5）
                        ▼
        ④ 全局 saturate(0.9) → contrast(1.06, pivot 128)
                        ▼
                   [输出 RGBA8]
```

图层顺序不可交换（②③ 与 ① 有重叠，Screen 叠加顺序影响结果）。

---

## 4. 逐阶段精确规格

### 4.1 预处理（不在核心算法内）

```
decode(bytes)                      // PNG/JPEG；原实现限制 4096×4096、32MiB 解码预算
side = min(w, h)
square = crop(img, ((w-side)/2, (h-side)/2, side, side))   // 居中裁正方形
scaled = resize_exact(square, size, size, Triangle)         // 三角/双线性，非最近邻
rgba   = scaled.to_rgba8()
```

- `size` 原项目 = **96**（下限 `max(8)`）。
- 缩放滤波必须是 **Triangle（tent / 双线性）**。最近邻会把细网点糊掉，Lanczos 会振铃。
- 裁剪是**居中裁**，不是拉伸 —— 对应 CSS `object-fit: cover` 的语义。

### 4.2 黑网点（AM 半调）

常量：
```
CELL          = 2.5      // 网格边长（px，96 尺寸下）
RADIUS_K      = 0.42     // r = (1-l) * CELL * RADIUS_K
MIN_R         = 0.05     // r <= 0.05 不画
DOT_ALPHA     = 0.62     // 黑点颜色 alpha
DOT_LAYER_OP  = 0.55     // 图层不透明度
EFF_ALPHA     = 0.62 * 0.55 = 0.341
```

流程（**行优先、逐格、顺序执行**）：
```
for cy = 0, 2.5, 5.0, ... while cy < H:
  for cx = 0, 2.5, 5.0, ... while cx < W:
      l = mean_luma(rgba, W, H, cx, cy, CELL)      // ← 读「当前」缓冲区，含已被前面网点改过的像素
      r = (1 - l) * CELL * 0.42                    // 0 .. 1.05
      if r > 0.05:
          draw_disk(center = (cx + 1.25, cy + 1.25), radius = r,
                    src = (0,0,0), alpha = 0.341, mode = MULTIPLY)
```

- 网格原点在 **(0,0)**，不是图像中心。所以封面左上角总是一个完整格。
- 单元数：`cols = ceil(W / 2.5)`，`rows = ceil(H / 2.5)`。96 时 = **39 × 39**。
- **最后一列/行是残格**：例如 W=96 时第 39 格 `cx=95.0`，点心 `96.25` 落在图外，
  网点被裁掉大半 —— 这是原样行为，必须保留。

`mean_luma` 的**精确取整**（不要改成几何边界）：
```
x0 = floor(cx)          y0 = floor(cy)
x1 = min(ceil(cx+CELL), W)   y1 = min(ceil(cy+CELL), H)
sum = 0; n = 0
for y in [y0, y1): for x in [x0, x1):
    sum += 0.2126*(R/255) + 0.7152*(G/255) + 0.0722*(B/255);  n++
return n == 0 ? 0 : clamp(sum/n, 0, 1)
```

> ⚠ 因为 `floor(cx)` 与 `ceil(cx+CELL)` 的组合，相邻格的取样窗口会**重叠 1 个像素**
> （`cell=2.5` 时窗口恒为 3px 宽，隔一格重叠一次）。这是原实现的行为，见 §8。

### 4.3 套印偏色（misregistration）

常量：
```
MISREG_CELL   = CELL * 2 = 5.0
MISREG_R      = CELL * 0.28 = 0.7      // 恒定半径，不随亮度变化
MISREG_LAYER_OP = 0.5

M 层: rgb(255,   0, 120), color_alpha = 0.5  → 有效 alpha 0.25
C 层: rgb(  0, 180, 255), color_alpha = 0.45 → 有效 alpha 0.225
```

流程（**先 M 后 C**）：
```
for (color, offset) in [ (M, 0.0), (C, 2.5) ]:
    for cy = offset, offset+5, ... while cy < H:
      for cx = offset, offset+5, ... while cx < W:
          draw_disk(center = (cx + 2.5, cy + 2.5), radius = 0.7,
                    src = color, alpha = color_alpha * 0.5, mode = SCREEN)
```

- M 层点心：`(2.5 + 5i, 2.5 + 5j)` —— 96 时 20×20。
- C 层点心：`(5.0 + 5i, 5.0 + 5j)` —— 96 时 19×19（整体右下偏 2.5px）。
- 两层**半径恒定**，与图像内容无关，纯粹是"套印没对准"的装饰。
- 注意 C 层从 `offset=2.5` 起循环，所以第一颗点心在 `5.0`，不是 `2.5`。

### 4.4 全局调色

```
for each pixel:
    luma = 0.2126*R + 0.7152*G + 0.0722*B        // 注意：在 0..255 上算
    R = luma + (R - luma) * 0.9                  // saturate 0.9：向亮度收敛
    G = luma + (G - luma) * 0.9
    B = luma + (B - luma) * 0.9
    R = clamp_u8((R - 128) * 1.06 + 128)         // contrast 1.06：绕中灰 128 拉伸
    G = clamp_u8((G - 128) * 1.06 + 128)
    B = clamp_u8((B - 128) * 1.06 + 128)
```

`clamp_u8(v) = round(v).clamp(0, 255)`（`round` 为 **half away from zero**，C++ `std::round` 同语义）。

---

## 5. 关键参数总表

| 参数 | 值 | 单位 | 说明 / 敏感度 |
|---|---|---|---|
| `COVER_PX` | 96 | px | 烘焙边长；AE 端改为参数或"格数" |
| `CELL` | 2.5 | px | **2.5 是"轻微网纹"与"廉价半调"的分界**；<2 过密糊成噪点，>3 变廉价网点 |
| `RADIUS_K` | 0.42 | — | `r = (1-l)·cell·k`；上限 `1.05 < cell/2`，保证点不跨格 |
| `MIN_R` | 0.05 | px | 小于此不画（避免浅色区噪点） |
| `DOT_ALPHA` | 0.62 | — | 黑点颜色 alpha |
| `DOT_LAYER_OP` | 0.55 | — | 图层不透明度 |
| `MISREG_CELL` | `2·CELL` = 5.0 | px | 套印网格 |
| `MISREG_R` | `0.28·CELL` = 0.7 | px | 恒定半径 |
| `MISREG_M` | `(255,0,120)`, a=0.5 | — | 品红 |
| `MISREG_C` | `(0,180,255)`, a=0.45 | — | 青 |
| `MISREG_LAYER_OP` | 0.5 | — | 套印层不透明度 |
| `CONTRAST` | 1.06 | — | pivot = 128 |
| `SATURATE` | 0.9 | — | 向 Rec.709 luma 收敛 |
| AA 斜坡 | 1.0 | px | `coverage = clamp(r - d + 0.5, 0, 1)` |
| 最小尺寸 | 8 | px | `size.max(8)` |

---

## 6. 参考伪代码（可直接翻译成 C++）

```
function halftone(rgba, W, H):
    if W == 0 or H == 0: return

    # ---- ① 黑网点 ----
    cy = 0.0
    while cy < H:
        cx = 0.0
        while cx < W:
            l = mean_luma(rgba, W, H, cx, cy, 2.5)
            r = (1.0 - l) * 2.5 * 0.42
            if r > 0.05:
                disk(rgba, W, H, cx+1.25, cy+1.25, r,
                     0,0,0, 0.62*0.55, MULTIPLY)
            cx += 2.5
        cy += 2.5

    # ---- ②③ 套印 ----
    for (col, ca, off) in [((255,0,120), 0.5, 0.0), ((0,180,255), 0.45, 2.5)]:
        cy = off
        while cy < H:
            cx = off
            while cx < W:
                disk(rgba, W, H, cx+2.5, cy+2.5, 0.7,
                     col.r, col.g, col.b, ca*0.5, SCREEN)
                cx += 5.0
            cy += 5.0

    # ---- ④ 调色 ----
    for each px:
        luma = 0.2126*px.r + 0.7152*px.g + 0.0722*px.b
        px.r = clamp_u8((luma + (px.r-luma)*0.9 - 128)*1.06 + 128)
        px.g = clamp_u8((luma + (px.g-luma)*0.9 - 128)*1.06 + 128)
        px.b = clamp_u8((luma + (px.b-luma)*0.9 - 128)*1.06 + 128)


function mean_luma(rgba, W, H, cx, cy, cell):
    x0 = floor(cx); y0 = floor(cy)
    x1 = min(ceil(cx+cell), W); y1 = min(ceil(cy+cell), H)
    sum = 0.0; n = 0
    for y in y0..y1-1: for x in x0..x1-1:
        i = (y*W + x)*4
        sum += 0.2126*(rgba[i]/255) + 0.7152*(rgba[i+1]/255) + 0.0722*(rgba[i+2]/255)
        n += 1
    return n == 0 ? 0.0 : clamp(sum/n, 0.0, 1.0)


function disk(rgba, W, H, cx, cy, r, sr, sg, sb, alpha, mode):
    x0 = max(floor(cx-r), 0); y0 = max(floor(cy-r), 0)
    x1 = min(ceil(cx+r), W);  y1 = min(ceil(cy+r), H)
    for y in y0..y1-1: for x in x0..x1-1:
        dx = (x+0.5) - cx; dy = (y+0.5) - cy
        d  = sqrt(dx*dx + dy*dy)
        cov = clamp(r - d + 0.5, 0.0, 1.0)
        if cov <= 0: continue
        a = alpha * cov
        i = (y*W + x)*4
        for k in 0..2:
            src = (sr,sg,sb)[k]; dst = rgba[i+k]
            blended = (mode == MULTIPLY) ? dst*src/255
                                         : 255 - (255-dst)*(255-src)/255
            rgba[i+k] = clamp_u8(dst + (blended-dst)*a)


function clamp_u8(v): return round(clamp(v, 0, 255))     // round = half away from zero
```

---

## 7. 依赖条件

- **核心算法：无任何依赖。** 只需 `sqrt`、`floor`、`ceil`、`round`、`clamp`。
- 浮点建议用 **f32**（原实现即 f32）。用 double 在 `round` 边界上可能有 1 LSB 差异。
- 解码/缩放：原项目用 `image 0.25`（features: `png`, `jpeg`），Triangle 滤波。
  **AE 端不需要**——AE 直接给你 `PF_EffectWorld` 像素，跳过解码。
- 线程/内存：算法只读写一块连续 RGBA 缓冲，无分配（除调用方传入的缓冲）。

---

## 8. 逐字节复现要点（移植时最容易错的地方）

1. **`mean_luma` 的取样窗口用 `floor(cx)` / `ceil(cx+cell)`，不是几何单元边界。**
   `cell=2.5` 时窗口恒为 3px 宽，且**相邻窗口隔一格重叠 1px**。改成精确 box filter → 结果不逐字节相同。

2. **① 阶段是顺序依赖的，不能预计算后并行。**
   `mean_luma` 读的是**当前**缓冲区。暗部网点半径大（`r` 接近 1.05），会覆盖到下一格取样窗口里的那个
   重叠像素，把下一格的平均亮度压低一点点 → 下一格网点略微变大。
   实测：把单元均值改成"从未修改的原图预计算"，在渐变/彩色图上有 **63–66 字节不同、最大差 2 级**；
   在平灰/纯黑/纯白图上完全一致。**追求逐字节相同就必须顺序执行。**

3. **每次混合后立即量化到 u8。**
   `blend_disk` 里每个像素混合完就 `clamp_u8`，不是全程浮点最后再量化。
   一个像素最多经历 3 次量化（黑点 → M → C）。少了这步，暗部会偏亮 1–2 级。

4. **采样点是像素中心 `(x+0.5, y+0.5)`。** 用整数坐标会整体偏移半像素。

5. **覆盖率是 1px 线性斜坡**，不是硬阈值也不是 2×2 超采样：
   `cov = clamp(r - d + 0.5, 0, 1)`。这保证半径很小时仍有部分覆盖，网点不会整颗消失。

6. **残格要保留**：最后一列/行的点心可能落在图外，网点被裁切。

7. **Alpha 一律不碰。**

8. **`r > 0.05` 才画**（严格大于，不是 ≥）。

---

## 9. 移植到 AE 插件

### 9.1 位深
原算法是 8-bit。AE 可能是 8/16/32-bit：
- **忠实模式**：每步混合后量化到 8-bit（`round(clamp(v*255,0,255))/255`），再继续。推荐。
- **平滑模式**：全程浮点、最后量化。视觉更干净，但暗部与 8-bit 结果差 1–2 级。

### 9.2 分辨率无关性（关键设计决策）

原实现的视觉身份 = **在 96×96 上烘焙，再用 NEAREST 放大显示**。
所以屏幕上的点距 = `2.5 × (显示尺寸 / 96)`，且边缘是**硬像素块**（不是抗锯齿圆点）。

AE 端两种做法：

| 模式 | 做法 | 结果 |
|---|---|---|
| **A. 忠实（推荐）** | 内部降到 96×96 → 跑算法 → NEAREST 放大到图层尺寸 | 与原产品**完全一致**，含像素块感 |
| **B. 分辨率无关** | 在图层分辨率直接跑，`cell = 2.5 × (min(W,H)/96)`，AA 斜坡同比放大 | 几何比例一致，但边缘平滑，少了"廉价印刷"味 |

参数建议：暴露 **"Dots Across"（横向格数，默认 38.4 = 96/2.5）** 比暴露绝对像素更稳，
换分辨率时视觉密度不变。

### 9.3 Alpha
算法不碰 A。但 AE 常见**预乘**像素：对半透明像素直接算 RGB 会得到错误结果。
建议：先反预乘 → 跑算法 → 再预乘；或明确声明本效果假设输入不透明（原产品封面恒不透明）。

### 9.4 建议暴露的控件

| 控件 | 默认 | 范围 |
|---|---|---|
| Dot Pitch / Dots Across | 38.4 格 | 10–120 |
| Black Dot Alpha | 0.62 | 0–1 |
| Black Layer Opacity | 0.55 | 0–1 |
| Dot Radius Scale | 0.42 | 0–0.5（>0.5 会跨格重叠） |
| Min Dot Radius | 0.05 | 0–0.3 |
| Misregistration | On | — |
| Misreg Grid ×Cell | 2.0 | 1.5–4 |
| Misreg Radius ×Cell | 0.28 | 0–0.5 |
| Magenta Color / Opacity | (255,0,120) / 0.5 | — |
| Cyan Color / Opacity | (0,180,255) / 0.45 | — |
| Contrast | 1.06 | 0.5–2 |
| Saturation | 0.9 | 0–2 |
| 8-bit Emulation | On | — |

### 9.5 性能
- 单元数在 96 基准下仅 **39×39 ≈ 1.5k**，每格画一颗 ≤3px 半径的圆 —— 单线程也远低于 1ms。
- 若走模式 B（大图层），仍保持"约 38 格横向"，单元数不随分辨率增长；只有第 ④ 步是 O(W·H)。
- 第 ④ 步可按行并行；① 必须顺序（见 §8.2）。

### 9.6 AE SDK 落点
- `PF_Cmd_RENDER` → 取 `PF_EffectWorld* output`（或先拷贝输入再原地改）。
- 按 `output->height` 逐行遍历；注意 `rowbytes` 可能 > `width*4`，**不要假设紧密排列**。
- 坐标系：`PF_EffectWorld` 的 `origin_x/origin_y` 可能非 0，网格原点要相对图层左上角算，
  否则效果会随图层位置漂移。

---

## 10. 验证

### 10.1 金标准向量（f32 参考实现实测）

哈希算法：**FNV-1a 64**（`h=0xcbf29ce484222325`；逐字节 `h^=b; h*=0x100000001b3`），对**输出全部字节**计算。

| 用例 | 尺寸 | 输入 | 输出字节数 | FNV-1a 64 |
|---|---|---|---|---|
| `flat128_8` | 8×8 | RGB(128,128,128), A=255 | 256 | `8530b123a7a4d966` |
| `flat200_16` | 16×16 | RGB(200,200,200), A=255 | 1024 | `6aea2fbf03866a4e` |
| `flat128_96` | 96×96 | RGB(128,128,128), A=255 | 36864 | `aa68fe2ba62c37be` |
| `black_32` | 32×32 | RGB(0,0,0), A=255 | 4096 | `2e2e094e5fc1c495` |
| `white_32` | 32×32 | RGB(255,255,255), A=255 | 4096 | `c0b014328067f325` |
| `grad_16` | 16×16 | R=x·16, G=y·16, B=128, A=255 | 1024 | `b631719ac201152e` |
| `color_16` | 16×16 | R=x·12+40, G=y·10+60, B=(x+y)·6, A=255 | 1024 | `0b49ffcfbcc77aa2` |

前 64 字节（便于快速肉眼比对）：

```
flat128_8   : 808080ff 757575ff 808080ff 757575ff 808080ff 808080ff 757575ff 808080ff
              757575ff 616161ff 868083ff 616161ff 757575ff 757575ff 616161ff 868083ff
flat200_16  : ccccccff ×16
flat128_96  : 808080ff 757575ff 808080ff 757575ff 808080ff 808080ff 757575ff 808080ff
              757575ff 808080ff 808080ff 757575ff 808080ff 757575ff 808080ff 808080ff
black_32    : 000000ff ×16
white_32    : ffffffff ×16
grad_16     : 000063ff 050058ff 190074ff 1e0059ff 300065ff 3f0068ff 44005cff 670076ff
              5e005eff 78006aff 88006cff 870061ff b50078ff a20063ff c3006eff d40071ff
color_16    : 213200ff 262d02ff 3b380aff 3b2d0cff 4d3414ff 59341aff 592f1cff 763928ff
              6f3026ff 873731ff 943838ff 8f3236ff b13b46ff a53343ff c33950ff cf3b56ff
```

> 注意：`flat128_8` 与 `flat128_96` 前 16 像素完全相同 —— 因为网格原点固定在 (0,0)，
> 与画布尺寸无关。可用这条做"网格原点"的回归检查。

### 10.2 不变量（应写进单测）

- 纯白输入 → 输出仍全 255（`r=0`，套印 screen 在白上无效，调色不动）。**已用哈希验证**。
- 纯黑输入 → **不是全 0**。黑网点与调色都不动它，但**套印层会留下斑点**：
  `screen(0, src) = src`，再乘有效 alpha（M 0.25 / C 0.225），网点正心约得 `(55,0,23)` 这类暗品红/暗青。
  未被套印覆盖的像素仍为 0。这是原样行为，不要"修正"成纯黑。
- Alpha 通道输入输出完全一致。
- 同一输入两次调用结果相同（确定性）。
- `W=0` 或 `H=0` → 无操作。
- 平灰 200 → 最暗像素 < 200（网点确实压暗了）。

### 10.3 视觉验收
- 亮部网点小而稀、暗部网点大而密，但**不连成一片**（`r_max=1.05 < cell/2=1.25`）。
- 品红/青网点应呈现为**规律的双色错位**，不应糊成紫色噪声。
- 放大到 400% 应能看到**硬边像素块**（若走模式 A）。

---

## 11. 可选：程序化占位封面（`procedural_cover`）

与主算法无关，仅在无封面时用 `seed` 生成抽象几何图，再跑一遍 `apply_halftone`。
`style = seed % 5`，五种构图（渐变+横线+日盘 / 斜渐变+月盘+同心纹 / 斜条纹+圆环 / 米白紫渐变+橙日 / 粉黑棋盘+高光）。
若 AE 插件只需要"给现有封面加网纹"，**整节可忽略**。

---

## 12. 取舍建议

| 想要 | 建议 |
|---|---|
| 与原产品**逐字节一致** | 模式 A（96 烘焙 + NEAREST 放大）+ 顺序执行 + 每步 8-bit 量化 |
| 高质量、可缩放 | 模式 B（分辨率无关）+ 全程浮点；接受"更干净的网纹" |
| 实时预览 | 模式 B + 预计算单元均值（放弃 §8.2 的顺序依赖，视觉差 ≤2 级） |
| 只想要"印刷感" | 可去掉套印层，只留黑网点 + 调色，风格更克制 |

**最容易被忽略的三件事**：① `mean_luma` 的 `floor/ceil` 窗口；② 每步混合后立刻量化；
③ 网格原点在 (0,0) 而非居中。
