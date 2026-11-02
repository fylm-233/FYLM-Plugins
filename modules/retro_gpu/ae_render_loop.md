# 同一 .aex 的宿主调度示例（待下一阶段接入）

现有 `DitheringStudio` MODULE / `OSLO_DitheringStudio` 保持唯一；不要新增第二个EffectMain、第二个匹配名或第二个.aex。
`retro_gpu_module` 是静态库，GLSL在构建期嵌入。生产包不需要外部GLSL、SPIR-V或辅助程序。

```cpp
// ParamsSetup: append after old 0..12, update the adapter's Count and i18n/UI schema together.
return retro::ae::AppendParams(in_data, out_data, labelFromI18nWithEnglishFallback);

// SMART_PRE_RENDER: snapshot every pixel-affecting parameter at current_time.
auto frame = std::make_unique<UnifiedFrame>();
copyExistingDitherParams(frame->controls); // IDs 1/2/8/15/17 remain stable
CheckoutParams(in_data, frame->controls);
frame->plan = retro::makePlan(frame->controls, fullInputSize);
if (!frame->plan.ok) {
    // Algorithm 1 = FS: select the tested CPU compatibility pipeline, or report unsupported GPU mode.
    // Never silently replace FS with Bayer. A CPU CRT/VHS fallback still needs implementation.
    return localizedPlanError(frame->plan.error);
}
checkoutFullInputAtCurrentTime();
setPreRenderDataWithDeleteCallback(frame.release());

// SMART_RENDER: same GPU lease/context for EVERY stage; all image resources frame-private.
GpuDeviceLease lease = acquireAEPrimaryOffscreenGLWithExclusiveDeviceAccess();
ScopedCurrentContext context(lease.hdc, lease.hglrc); // save/restore previous HDC/HGLRC
ScopedGLState state; // framebuffer, viewport, program, VAO, texture units, blend, sRGB, pack/unpack...
auto source = uploadAEARGBUsingActualRowBytesAndBitDepth();
source = convertPremultiplicationAndExplicitColorPolicy(source);
for (const auto& stage : frame.plan.stages) {
    auto destination = frameTextures.acquireDistinct(stage.size, GL_RGBA16F);
    auto& program = programs.inThisContext().forPass(stage.pass);
    glUseProgram(program.id);
    bindTexture(0, source, stage.pass==UpsampleNearest ? GL_NEAREST : GL_LINEAR, GL_CLAMP_TO_EDGE);
    bindPaletteTexture(1, paletteRGB8ExpandedToFloat, GL_NEAREST);
    bindRankTexture(2, GL_R16UI, 64, 64, retro::blueNoiseRanking(), GL_RED_INTEGER, GL_UNSIGNED_SHORT);
    auto u = retro::makeUniformFrame(frame.controls, stage.size, frame.time.value, frame.time.scale);
    retro::bindUniforms(glWrappers, program.cachedLocations, u, fullWorkOrigin.x, fullWorkOrigin.y);
    drawFullscreenTriangle(destination); // never sample from the current render target
    if (hostCancelled()) return Cancelled; // RAII releases textures/context/device access/checkouts
    source = destination;
}
source = convertBackToHostColorAndPremultiplication(source);
readBackAndCopyOnlyRequestedRect(source, outputWorld, actualOrigin, actualRowBytes, actualDepth);
```

集成时还需在Dither块补TonePass，并在最近邻放大后补AlphaRestorePass；保留进入Dither块前的全尺寸Alpha纹理，直到恢复完成。CRT在前时，该Alpha应取自CRT结果。当前基础计划器尚未生成这两类Pass。

`UniformApi`用回调包装实际`glGetUniformLocation/glUniform1f/glUniform1i/glUniform2f/glUniform2i`，不是UBO内存拷贝。
Location在Program link后缓存一次，重新link或设备/上下文变化后重建，`-1`表示该Pass优化掉未使用Uniform。
示例伪代码中的GPU租约、纹理池、上传/回读、颜色/Alpha转换、GL状态保护和完整CPU回退尚未实现，不是现成可运行渲染循环。

SDK26.5的PF_GPUDeviceInfo提供主设备offscreen OpenGL HGLRC/HDC，但SmartRender_GPU的设备world是OpenCL/CUDA/Metal/DirectX类别，不能把它的void*当GLuint。
第一步可在CPU SmartFX签出接口中使用主设备GL上下文上传/渲染/回读；这不等于AE原生GPU F32设备路径，不能据此设置GPU/F32标志。
真正的设备内零拷贝/AE GPU F32入口另行实现与验证；当前MFR/GPU/F32标志保持原状。
