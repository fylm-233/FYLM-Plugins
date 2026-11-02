#define PF_DEEP_COLOR_AWARE 1
#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_EffectSuites.h"
#include "AE_Macros.h"
#include "AE_PluginData.h"
#include "Param_Utils.h"
#include "dither/core.hpp"
#include "dither/i18n.hpp"
#include "retro/params.hpp"
#include "retro/gpu_bridge.hpp"
#include "retro/render_audit.hpp"
#include "entry.h"
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <limits>
#include <cstring>
namespace {
enum {
    Input,
    Algorithm,
    Palette,
    Threshold,
    Resolution,
    Invert,
    Serpentine,
    Contrast,
    Midtones,
    Highlights,
    Alpha,
    Language, // Append only: preserve all 0.1 parameter indices and disk IDs.
    Help,
    LegacyCount
};
constexpr int Count=retro::ae::Count;
constexpr int ids[] = {1, 2, 8, 15, 17};
constexpr const char *palettes[] = {"none", "bw", "gb", "pico8", "db16", "db32", "cga16", "ega16"};
struct Frame {
    dither::RenderParams p;
    PF_LRect bounds{};
    dither::i18n::Locale locale = dither::i18n::Locale::English;
    retro::Controls retro;
    std::int64_t timeValue=0;
    std::uint64_t timeScale=1;
};
namespace loc = dither::i18n;
constexpr const char *labelKeys[] = {"",
                                     "param.algorithm",
                                     "param.palette",
                                     "param.threshold",
                                     "param.resolution",
                                     "param.invert",
                                     "param.serpentine",
                                     "param.contrast",
                                     "param.midtones",
                                     "param.highlights",
                                     "param.alpha",
                                     "param.language",
                                     "param.help"};
loc::Locale hostLocale(PF_InData *in_data) noexcept {
    if (!in_data || !in_data->pica_basicP)
        return loc::Locale::English;
    const PFAppSuite6 *suite = nullptr;
    if (in_data->pica_basicP->AcquireSuite(kPFAppSuite, kPFAppSuiteVersion6,
                                           reinterpret_cast<const void **>(&suite)) ||
        !suite)
        return loc::Locale::English;
    char language[PF_APP_LANG_TAG_SIZE]{};
    auto err = suite->PF_AppGetLanguage(language);
    in_data->pica_basicP->ReleaseSuite(kPFAppSuite, kPFAppSuiteVersion6);
    return err ? loc::Locale::English : loc::fromCode(language);
}
PF_Err message(PF_OutData *out, loc::Locale locale, std::string_view key, PF_Err err) {
    loc::copyUtf8(out->return_msg, sizeof(out->return_msg), loc::text(locale, key));
    if (err != PF_Interrupt_CANCEL)
        out->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
    return err;
}
void showHelp(loc::Locale locale) {
    auto wide = [](std::string_view s) {
        int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), nullptr, 0);
        std::wstring w(n, L'\0');
        if (n)
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), w.data(), n);
        return w;
    };
    auto text = wide(loc::text(locale, "help.text")), title = wide(loc::text(locale, "param.help"));
    MessageBoxW(GetActiveWindow(), text.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}
void dispose(void *p) { delete static_cast<Frame *>(p); }
PF_Err updateUI(PF_InData *in_data, PF_ParamDef *params[]) {
    const PF_ParamUtilsSuite3 *suite = nullptr;
    auto err = in_data->pica_basicP->AcquireSuite(kPFParamUtilsSuite, kPFParamUtilsSuiteVersion3,
                                                  reinterpret_cast<const void **>(&suite));
    if (err)
        return PF_Err_BAD_CALLBACK_PARAM;
    auto locale = hostLocale(in_data);
    (void)params;
    for (int index = 1; index < Count && !err; ++index) {
        if(index==retro::ae::Group||index==retro::ae::EndGroup)continue;
        auto def = *params[index];
        auto key=index<LegacyCount?labelKeys[index]:retro::ae::labelKey(index);
        loc::copyUtf8(def.PF_DEF_NAME, sizeof(def.PF_DEF_NAME), loc::text(locale,key));
        if (index == Language)
            def.ui_flags |= PF_PUI_INVISIBLE;
        if (index == Serpentine) {
            if (params[Algorithm]->u.pd.value == 1)
                def.ui_flags &= ~PF_PUI_DISABLED;
            else
                def.ui_flags |= PF_PUI_DISABLED;
        }
        err = suite->PF_UpdateParamUI(in_data->effect_ref, index, &def);
    }
    in_data->pica_basicP->ReleaseSuite(kPFParamUtilsSuite, kPFParamUtilsSuiteVersion3);
    return err;
}
PF_Err setup(PF_InData *in_data, PF_OutData *out_data) {
    PF_ParamDef def{};
    // AE cannot update native popup item strings. Use invariant algorithm notation here;
    // the companion ScriptUI panel supplies fully translated item names at runtime.
    PF_ADD_POPUPX("Algorithm", 5, 1, "Floyd-Steinberg|Bayer 4x4|Bayer 8x8|0 / 1|64x64",
                  PF_ParamFlag_CANNOT_TIME_VARY, Algorithm);
    PF_ADD_POPUPX("Palette", 8, 1, "--|0 / 255|Game Boy|PICO-8|DawnBringer 16|DawnBringer 32|CGA|EGA",
                  PF_ParamFlag_CANNOT_TIME_VARY, Palette);
    AEFX_CLR_STRUCT(def);
    PF_ADD_SLIDER("Threshold", 0, 255, 0, 255, 128, Threshold);
    AEFX_CLR_STRUCT(def);
    PF_ADD_SLIDER("Work Size (0=Full)", 0, 8192, 0, 8192, 512, Resolution);
    PF_ADD_CHECKBOXX("Invert", false, 0, Invert);
    PF_ADD_CHECKBOXX("Serpentine", true, 0, Serpentine);
    PF_ADD_FLOAT_SLIDERX("Contrast", -100, 100, -100, 100, 0, 1, 0, 0, Contrast);
    PF_ADD_FLOAT_SLIDERX("Midtones", .5, 2, .5, 2, 1, 2, 0, 0, Midtones);
    PF_ADD_FLOAT_SLIDERX("Highlights", 0, 100, 0, 100, 0, 1, 0, 0, Highlights);
    PF_ADD_POPUPX("Alpha", 2, 1, "1|A", PF_ParamFlag_CANNOT_TIME_VARY, Alpha);
    // Keep the old parameter type and index for project compatibility, but hide
    // the manual selector. Language now always follows the active AE UI.
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POPUP;
    PF_STRNNCPY(def.PF_DEF_NAME, "Language (automatic)", sizeof(def.PF_DEF_NAME));
    def.u.pd.num_choices = 4;
    def.u.pd.dephault = 1;
    def.u.pd.value = 1;
    def.u.pd.u.PF_DEF_NAMESPTR = "English|简体中文|繁體中文|日本語";
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
    def.ui_flags = PF_PUI_INVISIBLE;
    def.uu.id = Language;
    auto topErr = PF_ADD_PARAM(in_data, -1, &def);
    if (topErr)
        return topErr;
    PF_ADD_BUTTON("Help", "?", 0, PF_ParamFlag_SUPERVISE, Help);
    return retro::ae::AppendParams(in_data,out_data,[](const char* key,const char* fallback){auto value=loc::text(loc::Locale::English,key);return value==key?fallback:value.data();});
}
PF_Err pre(PF_InData *in_data, PF_PreRenderExtra *e, loc::Locale &locale) {
    auto f = std::make_unique<Frame>();
    PF_ParamDef p[Count]{};
    locale = f->locale = hostLocale(in_data);
    for (int i = 1; i < Language; ++i) {
        auto err = PF_CHECKOUT_PARAM(in_data, i, in_data->current_time, in_data->time_step,
                                     in_data->time_scale, &p[i]);
        if (err)
            return err;
    }
    f->p.algorithmId = ids[std::clamp(int(p[Algorithm].u.pd.value) - 1, 0, 4)];
    f->p.palette = dither::palette(palettes[std::clamp(int(p[Palette].u.pd.value) - 1, 0, 7)]);
    f->p.threshold = p[Threshold].u.sd.value;
    f->p.workingResolution = p[Resolution].u.sd.value;
    f->p.invert = p[Invert].u.bd.value != 0;
    f->p.serpentine = p[Serpentine].u.bd.value != 0;
    f->p.tone = {p[Contrast].u.fs_d.value, p[Midtones].u.fs_d.value, p[Highlights].u.fs_d.value};
    f->p.alphaMode = p[Alpha].u.pd.value == 2 ? dither::AlphaMode::Preserve : dither::AlphaMode::Opaque;
    auto retroErr=retro::ae::CheckoutParams(in_data,f->retro);if(retroErr)return retroErr;
    f->retro.workingResolution=f->p.workingResolution;
    f->retro.dither.algorithmId=f->p.algorithmId;f->retro.dither.paletteSize=int(f->p.palette.size());
    f->retro.dither.threshold=float(f->p.threshold);f->retro.dither.invert=f->p.invert;
    f->retro.dither.preserveAlpha=f->p.alphaMode==dither::AlphaMode::Preserve;
    f->retro.toneContrast=float(f->p.tone.contrast);f->retro.toneMidtones=float(f->p.tone.midtones);f->retro.toneHighlights=float(f->p.tone.highlights);
    f->timeValue=in_data->current_time;f->timeScale=in_data->time_scale;
    PF_RenderRequest req = e->input->output_request;
    req.rect = {0, 0, 0, 0};
    PF_CheckoutResult meta{};
    auto err = e->cb->checkout_layer(in_data->effect_ref, Input, 1, &req, in_data->current_time,
                                     in_data->time_step, in_data->time_scale, &meta);
    if (err)
        return err;
    f->bounds = meta.max_result_rect;
    // Opaque output reveals transparent pixels throughout the original layer as well.
    f->bounds.left = std::min<A_long>(0, f->bounds.left);
    f->bounds.top = std::min<A_long>(0, f->bounds.top);
    auto sampled=[](A_long value,PF_RationalScale scale){if(scale.num<=0||scale.den<=0)return value;return A_long(std::ceil(double(value)*scale.num/scale.den));};
    f->bounds.right = std::max(f->bounds.right,sampled(in_data->width,in_data->downsample_x));
    f->bounds.bottom = std::max(f->bounds.bottom,sampled(in_data->height,in_data->downsample_y));
    req.rect = f->bounds;
    req.channel_mask = PF_ChannelMask_ARGB;
    PF_CheckoutResult full{};
    err = e->cb->checkout_layer(in_data->effect_ref, Input, 0, &req, in_data->current_time,
                                in_data->time_step, in_data->time_scale, &full);
    if (err)
        return err;
    e->output->max_result_rect = f->bounds;
    auto r = e->input->output_request.rect;
    r.left = std::max(r.left, f->bounds.left);
    r.top = std::max(r.top, f->bounds.top);
    r.right = std::max(r.left, std::min(r.right, f->bounds.right));
    r.bottom = std::max(r.top, std::min(r.bottom, f->bounds.bottom));
    e->output->result_rect = r;
    e->output->solid = false;
    e->output->pre_render_data = f.release();
    e->output->delete_pre_render_data_func = dispose;
    return PF_Err_NONE;
}
struct Progress : dither::IProgress {
    PF_InData *in;
    PF_Err err = 0;
    explicit Progress(PF_InData *i) : in(i) {}
    bool cancelled() override {
        err = PF_ABORT(in);
        return err != 0;
    }
    void report(double f) override {
        if (!err)
            err = PF_PROGRESS(in, A_long(f * 1000), 1000);
    }
};
PF_Err smart(PF_InData *in_data, PF_OutData *out_data, PF_SmartRenderExtra *e) {
    auto f = static_cast<const Frame *>(e->input->pre_render_data);
    if (!f)
        return message(out_data, loc::Locale::English, "error.frame", PF_Err_INTERNAL_STRUCT_DAMAGED);
    retro::ae::RenderAudit audit(f->timeValue,f->timeScale);
    if (e->input->bitdepth != 8 && e->input->bitdepth != 16)
        return message(out_data, f->locale, "error.unsupported_format", PF_Err_BAD_CALLBACK_PARAM);
    PF_EffectWorld *src = nullptr;
    PF_EffectWorld *dst = nullptr;
    auto err = e->cb->checkout_layer_pixels(in_data->effect_ref, 0, &src);
    if (err)
        return err;
    struct Checkin {
        PF_InData *in;
        PF_SmartRenderExtra *e;
        ~Checkin() { e->cb->checkin_layer_pixels(in->effect_ref, 0); }
    } guard{in_data, e};
    err = e->cb->checkout_output(in_data->effect_ref, &dst);
    if (err)
        return err;
    if (!dst || !dst->data)
        return PF_Err_NONE;
    // Full layer request fixes the diffusion history and ordered pattern phase for every ROI.
    auto width64=std::int64_t(f->bounds.right)-f->bounds.left,height64=std::int64_t(f->bounds.bottom)-f->bounds.top;
    if(width64>std::numeric_limits<int>::max()||height64>std::numeric_limits<int>::max())return message(out_data,f->locale,"error.overflow",PF_Err_OUT_OF_MEMORY);
    int w=int(width64),h=int(height64);
    if (w <= 0 || h <= 0)
        return PF_Err_NONE;
    if(size_t(w)>SIZE_MAX/size_t(h)/16)return message(out_data,f->locale,"error.overflow",PF_Err_OUT_OF_MEMORY);
    bool useRetro=f->retro.retroEnabled&&(f->retro.crt.vhsEnabled||f->retro.crt.crtEnabled);
    // Independent switches: bypass both engines without quantizing 16-bit channels.
    if(!useRetro&&!f->retro.dither.enabled){
        const std::size_t pixelBytes=e->input->bitdepth==16?sizeof(PF_Pixel16):sizeof(PF_Pixel8);
        for(int y=0;y<dst->height;++y){
            if(y%32==0){auto cancelled=PF_ABORT(in_data);if(cancelled)return cancelled;}
            auto row=reinterpret_cast<unsigned char*>(dst->data)+y*dst->rowbytes;
            const auto sy=std::int64_t(dst->origin_y)+y-(src?src->origin_y:0);
            for(int x=0;x<dst->width;++x){
                auto target=row+std::size_t(x)*pixelBytes;
                const auto sx=std::int64_t(dst->origin_x)+x-(src?src->origin_x:0);
                if(src&&src->data&&sx>=0&&sx<src->width&&sy>=0&&sy<src->height){
                    auto source=reinterpret_cast<const unsigned char*>(src->data)+sy*src->rowbytes+sx*pixelBytes;
                    std::memmove(target,source,pixelBytes);
                }else std::memset(target,0,pixelBytes);
            }
        }
        return PF_Err_NONE;
    }
    std::size_t channels=size_t(w)*h*4;
    std::vector<unsigned char> input(useRetro?0:channels,0),output(input.size());
    std::vector<float> gpuInput(useRetro?channels:0,0),gpuOutput;
    bool deep = e->input->bitdepth == 16;
    if (src && src->data)
        for (int y = 0; y < src->height; ++y) {
            if (y % 32 == 0) {
                err = PF_ABORT(in_data);
                if (err)
                    return err;
            }
            auto row = reinterpret_cast<const unsigned char *>(src->data) + y * src->rowbytes;
            for (int x = 0; x < src->width; ++x) {
                int xx = src->origin_x + x - f->bounds.left, yy = src->origin_y + y - f->bounds.top;
                if (xx < 0 || xx >= w || yy < 0 || yy >= h)
                    continue;
                auto i = (size_t(yy) * w + xx) * 4;
                if (deep) {
                    auto q = reinterpret_cast<const PF_Pixel16 *>(row)[x];
                    if(useRetro){gpuInput[i]=float(q.red)/PF_MAX_CHAN16;gpuInput[i+1]=float(q.green)/PF_MAX_CHAN16;gpuInput[i+2]=float(q.blue)/PF_MAX_CHAN16;gpuInput[i+3]=float(q.alpha)/PF_MAX_CHAN16;continue;}
                    input[i] = dither::toU8Clamped(q.red * 255.0 / PF_MAX_CHAN16);
                    input[i + 1] = dither::toU8Clamped(q.green * 255.0 / PF_MAX_CHAN16);
                    input[i + 2] = dither::toU8Clamped(q.blue * 255.0 / PF_MAX_CHAN16);
                    input[i + 3] = dither::toU8Clamped(q.alpha * 255.0 / PF_MAX_CHAN16);
                } else {
                    auto q = reinterpret_cast<const PF_Pixel8 *>(row)[x];
                    if(useRetro){gpuInput[i]=q.red/255.f;gpuInput[i+1]=q.green/255.f;gpuInput[i+2]=q.blue/255.f;gpuInput[i+3]=q.alpha/255.f;continue;}
                    input[i] = q.red;
                    input[i + 1] = q.green;
                    input[i + 2] = q.blue;
                    input[i + 3] = q.alpha;
                }
            }
        }
    Progress progress(in_data);
    if(useRetro){
        retro::RenderRequest request;request.controls=f->retro;request.size={w,h};request.timeValue=f->timeValue;request.timeScale=f->timeScale;request.input=std::move(gpuInput);
        for(auto color:f->p.palette){request.palette.push_back(color.r/255.f);request.palette.push_back(color.g/255.f);request.palette.push_back(color.b/255.f);}
        request.cancelled=[&progress]{return progress.cancelled();};
        request.cpuDither=[&](const std::vector<float>& source){
            std::vector<unsigned char> s(channels),d(channels);for(std::size_t i=0;i<channels;++i)s[i]=dither::toU8Clamped(source[i]*255.0);
            auto r=dither::render({s.data(),w,h,size_t(w)*4,dither::PixelFormat::RGBA8},{d.data(),w,h,size_t(w)*4,dither::PixelFormat::RGBA8},f->p,&progress);
            if(!r.ok)throw std::runtime_error(r.code==dither::ErrorCode::Cancelled?"retro.error.cancelled":"error.internal");
            std::vector<float> result(channels);for(std::size_t i=0;i<channels;++i)result[i]=d[i]/255.f;return result;
        };
        auto result=retro::ae::renderWithHostDevice(in_data,request);
        if(!result.ok)return message(out_data,f->locale,result.error,progress.err?progress.err:PF_Err_INTERNAL_STRUCT_DAMAGED);
        gpuOutput=std::move(result.pixels);
    }else if(f->retro.dither.enabled){
        auto result =
            dither::render({input.data(), w, h, size_t(w) * 4, dither::PixelFormat::RGBA8},
                           {output.data(), w, h, size_t(w) * 4, dither::PixelFormat::RGBA8}, f->p, &progress);
        if (progress.err)
            return message(out_data, f->locale, "error.cancelled", progress.err);
        if (!result.ok)
            return message(out_data, f->locale, dither::errorKey(result.code), PF_Err_INTERNAL_STRUCT_DAMAGED);
    }else{
        output=input;
    }
    for (int y = 0; y < dst->height; ++y) {
        if(y%32==0){auto cancelled=PF_ABORT(in_data);if(cancelled)return cancelled;}
        auto row = reinterpret_cast<unsigned char *>(dst->data) + y * dst->rowbytes;
        for (int x = 0; x < dst->width; ++x) {
            int xx = dst->origin_x + x - f->bounds.left, yy = dst->origin_y + y - f->bounds.top;
            unsigned char q[4] = {};
            if(useRetro){float pixel[4]={};if(xx>=0&&xx<w&&yy>=0&&yy<h)std::copy_n(gpuOutput.data()+(size_t(yy)*w+xx)*4,4,pixel);
                auto normalized=[](float value){return std::isfinite(value)?std::clamp(double(value),0.0,1.0):0.0;};
                if(deep){auto& v=reinterpret_cast<PF_Pixel16*>(row)[x];v.red=A_u_short(std::floor(normalized(pixel[0])*PF_MAX_CHAN16+.5));v.green=A_u_short(std::floor(normalized(pixel[1])*PF_MAX_CHAN16+.5));v.blue=A_u_short(std::floor(normalized(pixel[2])*PF_MAX_CHAN16+.5));v.alpha=A_u_short(std::floor(normalized(pixel[3])*PF_MAX_CHAN16+.5));}
                else{auto& v=reinterpret_cast<PF_Pixel8*>(row)[x];v.red=dither::toU8Clamped(pixel[0]*255.0);v.green=dither::toU8Clamped(pixel[1]*255.0);v.blue=dither::toU8Clamped(pixel[2]*255.0);v.alpha=dither::toU8Clamped(pixel[3]*255.0);}continue;
            }
            if (xx >= 0 && xx < w && yy >= 0 && yy < h)
                std::copy_n(output.data() + (size_t(yy) * w + xx) * 4, 4, q);
            if (deep) {
                auto &v = reinterpret_cast<PF_Pixel16 *>(row)[x];
                v.red = A_u_short((q[0] * PF_MAX_CHAN16 + 127) / 255);
                v.green = A_u_short((q[1] * PF_MAX_CHAN16 + 127) / 255);
                v.blue = A_u_short((q[2] * PF_MAX_CHAN16 + 127) / 255);
                v.alpha = A_u_short((q[3] * PF_MAX_CHAN16 + 127) / 255);
            } else {
                auto &v = reinterpret_cast<PF_Pixel8 *>(row)[x];
                v.red = q[0];
                v.green = q[1];
                v.blue = q[2];
                v.alpha = q[3];
            }
        }
    }
    return PF_Err_NONE;
}
} // namespace
extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr p, PF_PluginDataCB2 cb, SPBasicSuite *,
                                                     const char *, const char *) {
    PF_Err result = PF_Err_NONE;
    return PF_REGISTER_EFFECT_EXT2(p, cb, "Dithering Studio", "OSLO_DitheringStudio", "Stylize",
                                   AE_RESERVED_INFO, "EffectMain",
                                   "https://github.com/Oslonline/dithering-studio");
}
extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData *in_data, PF_OutData *out_data,
                                       PF_ParamDef *params[], PF_LayerDef *, void *extra) {
    auto locale = hostLocale(in_data);
    if (cmd == PF_Cmd_SMART_RENDER && extra) {
        auto e = static_cast<PF_SmartRenderExtra *>(extra);
        if (e->input->pre_render_data)
            locale = static_cast<const Frame *>(e->input->pre_render_data)->locale;
    }
    try {
        switch (cmd) {
        case PF_Cmd_ABOUT:
            std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                          "Dithering Studio 0.3\rDithering Studio / Oslo418 (Apache-2.0)\rCRT/VHS / Marcel Jovic (MIT)");
            break;
        case PF_Cmd_GLOBAL_SETUP:
            out_data->my_version = PF_VERSION(0, 3, 0, PF_Stage_DEVELOP, 1);
            // VHS depends on composition time even for still input and constant controls.
            out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_SEND_UPDATE_PARAMS_UI |
                                  PF_OutFlag_NON_PARAM_VARY;
            out_data->out_flags2 = PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_REVEALS_ZERO_ALPHA;
#if DITHER_ENABLE_MFR
            out_data->out_flags2|=PF_OutFlag2_SUPPORTS_THREADED_RENDERING;
#endif
            break;
        case PF_Cmd_UPDATE_PARAMS_UI: {
            auto err=updateUI(in_data,params);
            return err?message(out_data,locale,"error.callback",err):PF_Err_NONE;
        }
        case PF_Cmd_USER_CHANGED_PARAM:
            if (static_cast<PF_UserChangedParamExtra *>(extra)->param_index == Help) {
                showHelp(locale);
                return PF_Err_NONE;
            }
            {auto err=updateUI(in_data,params);return err?message(out_data,locale,"error.callback",err):PF_Err_NONE;}
        case PF_Cmd_PARAMS_SETUP: {
            auto err=setup(in_data,out_data);
            return err?message(out_data,locale,"error.callback",err):PF_Err_NONE;
        }
        case PF_Cmd_SMART_PRE_RENDER: {
            auto err = pre(in_data, static_cast<PF_PreRenderExtra *>(extra), locale);
            return err ? message(out_data, locale, "error.callback", err) : PF_Err_NONE;
        }
        case PF_Cmd_SMART_RENDER: {
            out_data->return_msg[0] = 0;
            auto err = smart(in_data, out_data, static_cast<PF_SmartRenderExtra *>(extra));
            return err && !out_data->return_msg[0]
                       ? message(out_data, locale,
                                 err == PF_Interrupt_CANCEL ? "error.cancelled" : "error.callback", err)
                       : err;
        }
        case PF_Cmd_SEQUENCE_SETUP:
        case PF_Cmd_SEQUENCE_RESETUP:
        case PF_Cmd_SEQUENCE_SETDOWN:
            break;
        default:
            break;
        }
        return PF_Err_NONE;
    } catch (const std::bad_alloc &) {
        return message(out_data, locale, "error.memory", PF_Err_OUT_OF_MEMORY);
    } catch (...) {
        return message(out_data, locale, "error.internal", PF_Err_INTERNAL_STRUCT_DAMAGED);
    }
}
