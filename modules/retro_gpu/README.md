# Retro GPU integration kit

这是已集成到**同一个** `DitheringStudio.aex` 0.3 的独立静态模块。主工程已调用实际OpenGL渲染器、注册控件并启用经验证的MFR标志。本文下方的接入示意保留作为模块复用说明；实际集成状态以 [0.3报告](../../docs/0.3_MFR_REPORT.md) 为准。

## 完整代码入口

- `shaders/crt_vhs.glsl`：所需五个独立函数及 `processCRTVHS()`，另提供更适合多Pass的 `processVHS()` / `processCRT()`。
- `shaders/dither.glsl`：Bayer4/8、Binary、BlueNoise量化。
- `shaders/unified.frag.glsl`：统一核心的Pass变体入口；`fullscreen.vert.glsl`为全屏三角形。
- `include/retro/retro.hpp`：参数、时间/分辨率Uniform、顺序计划及GL函数回调。
- `src/retro.cpp`：完整Uniform传递实现；具名设置，不依赖C++/GLSL结构内存布局。
- `include/retro/renderer.hpp` / `src/renderer_win.cpp`：实际多Pass纹理调度、状态隔离、内存预算、取消与FS兼容回调。
- `../../plugin/ae/retro/gpu_bridge.cpp`：租用AE提供的同一OpenGL上下文，独占设备访问并恢复原上下文。
- `../../plugin/ae/retro/params_example.cpp`：实际可编译的PF_ParamDef注册与PreRender签出示例。
- `ae_render_loop.md`：宿主渲染循环伪代码及资源生命周期要求。

所有GLSL由CMake嵌入C++静态字符串，.aex运行无需读取这些文件。
`src/blue_noise.inc`和旧核心表逐字节一致（SHA256 `3E1E5EC7846EE7AC46998D1F6C8041F2ADC9F4D36AC9681D47C196902CCCBA4E`），模块可独立复制，不包含指向核心私有目录的include。

## 构建与验证

```powershell
cmake -S modules/retro_gpu -B build/retro-kit -G 'Visual Studio 18 2026' -A x64 `
  -DAE_SDK_ROOT=G:/AfterEffectsSDK_26.5_win `
  '-DGLSLC=C:/path/to/glslc.exe'
cmake --build build/retro-kit --config Release
ctest --test-dir build/retro-kit -C Release --output-on-failure
```

AE_SDK_ROOT可省略（将不编译SDK示例）；glslc可省略（将不运行GLSL编译测试）。编译器不随模块分发，不是插件运行时依赖。
本机使用MSVC19.51、SDK26.5、shaderc2021.2实际通过C++/SDK示例和7项测试；静态编译不是实际GPU或AE绘制测试。

## 接入主工程的下一阶段改动

```cmake
# Only a STATIC library is added. The existing DitheringStudio remains the sole MODULE/.aex.
add_subdirectory(modules/retro_gpu)
target_link_libraries(DitheringStudio PRIVATE retro_gpu_module)
# After implementing/validating the host hooks, add the AE helper source to that SAME target.
```

注册代码只能追加在旧0.2索引0..12之后，不能将原控件重排。新Retro总开关默认关闭。
实际接线需实现离屏上下文租约、GL状态保存/恢复、纹理池、TonePass、上传/回读、颜色与Alpha、CPU回退及四语UI；详见 `../../docs/CRT_VHS_DEVELOPMENT_PLAN.md`。

## 明确的兼容边界

- GLSL为3.30 core，UV是全帧归一化坐标，uResolution为当前Pass完整工作图像尺寸；不能传ROI尺寸重新起相位。
- 所有sampler用CLAMP_TO_EDGE；最近邻放大Pass使用NEAREST，其余按设计用LINEAR；整数排名表用R16UI/RED_INTEGER/UNSIGNED_SHORT。
- chromaShiftPixels是像素；applyChromaShift的offset是UV单位；wobbleIntensity是宽度比例。scanlineCount为垂直方向总行数。
- ParamsSetup添加UI，PreRender签出参数并冻结有理时间；不能在PreRender注册UI。
- GPU像素算法子集不含FS；实际渲染器为FS调用旧CPU核心后回到同一GPU上下文处理CRT/VHS。validate返回稳定机器键，AE边界已经补齐四语错误。
- 当前矩形和CRT aspect假设方形像素。PAR、非对称降采样、负原点、透明素材及HDR需按计划补齐宿主适配。
- `makePlan()`为基础逻辑顺序，实际`renderer_win.cpp`已插入TonePass和全尺寸AlphaRestorePass，并测试降采样保留源Alpha；读取实际执行细节应以渲染器为准。
- 便捷processCRTVHS会重复计算VHS邻域；生产使用分离VHS→CRT Pass。不会声称与中间纹理路径逐位一致。
- 曲率为新增算法；原项目使用3D屏幕曲面。零值关闭/边界保护也属于明确的适配差异。

MIT来源：Marcel Jovic / punikonta，固定提交6ba87f95c152ec023fc07c4f77e87bdd70783afa；完整许可见third_party。
Dithering部分沿用项目Apache-2.0及NOTICE。原始文件/版本/哈希在references目录。
