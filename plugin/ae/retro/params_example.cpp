// Append-only live registration for the unified 0.3 effect; also compiled by the standalone kit.
// Keep SDK headers here in plugin/ae; the reusable retro module has no Adobe dependency.
#define PF_DEEP_COLOR_AWARE 1
#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "retro/retro.hpp"
#include "params.hpp"
namespace retro::ae {
// Existing 0.2 indices 0..12 remain unchanged. Do not insert these before Language/Help.
const char* labelKey(int index) noexcept {
 static constexpr const char* keys[]={"retro.group","retro.enabled","retro.order","retro.vhs","retro.crt","retro.curvature","retro.scan_count","retro.scan_opacity","retro.chroma","retro.jitter","retro.jitter_frequency","retro.noise","retro.noise_color","retro.time_rate","retro.time_offset","retro.animate","retro.clamp","dither.enabled","retro.tracking_height","retro.tracking_distortion","retro.tracking_noise","retro.streak_frequency","retro.streak_intensity","retro.streak_height","retro.dropout_frequency","retro.dropout_intensity","retro.dropout_height","retro.group"};
 return index>=Group&&index<=EndGroup?keys[index-Group]:"";
}
PF_Err AppendParams(PF_InData *in_data, PF_OutData *out_data, LabelFn label) {
    PF_ParamDef def{};
    // In production label resolves current i18n keys; API-owned popup items remain stable notations.
    PF_ADD_TOPIC(label("retro.group", "CRT / VHS"), Group);
    PF_ADD_CHECKBOXX(label("retro.enabled", "CRT / VHS"), false, 0, Enabled);
    PF_ADD_POPUPX(label("retro.order", "Pipeline Order"), 2, 1, "D > V > C|V > C > D",
                  PF_ParamFlag_CANNOT_TIME_VARY, PipelineOrder);
    PF_ADD_CHECKBOXX(label("retro.vhs", "VHS"), true, 0, VhsOn);
    PF_ADD_CHECKBOXX(label("retro.crt", "CRT"), true, 0, CrtOn);
    PF_ADD_FLOAT_SLIDERX(label("retro.curvature", "Curvature (%)"), 0, 100, 0, 100, 0, 1, 0, 0, Curvature);
    PF_ADD_FLOAT_SLIDERX(label("retro.scan_count", "Scanline Count"), 0, 8192, 0, 1080, 288, 0, 0, 0,
                         ScanCount);
    PF_ADD_FLOAT_SLIDERX(label("retro.scan_opacity", "Scanline Opacity (%)"), 0, 100, 0, 100, 25, 1, 0, 0,
                         ScanOpacity);
    PF_ADD_FLOAT_SLIDERX(label("retro.chroma", "Chroma Shift (px)"), -20, 20, 0, 10, 2, 2, 0, 0,
                         ChromaPixels);
    PF_ADD_FLOAT_SLIDERX(label("retro.jitter", "Jitter (% width)"), 0, 10, 0, 1, .2, 3, 0, 0, JitterPercent);
    PF_ADD_FLOAT_SLIDERX(label("retro.jitter_frequency", "Jitter Frequency"), 0, 100, 0, 20, 4, 1, 0, 0,
                         JitterFrequency);
    PF_ADD_FLOAT_SLIDERX(label("retro.noise", "Noise (%)"), 0, 100, 0, 100, 10, 1, 0, 0, NoisePercent);
    PF_ADD_FLOAT_SLIDERX(label("retro.noise_color", "Noise Color (%)"), 0, 100, 0, 100, 30, 1, 0, 0,
                         NoiseColorPercent);
    PF_ADD_FLOAT_SLIDERX(label("retro.time_rate", "Time Rate"), -4, 4, 0, 4, 1, 2, 0, 0, TimeRate);
    PF_ADD_FLOAT_SLIDERX(label("retro.time_offset", "Time Offset (s)"), -3600, 3600, -10, 10, 0, 3, 0, 0,
                         TimeOffset);
    PF_ADD_CHECKBOXX(label("retro.animate", "Animate VHS"), true, 0, AnimateTime);
    PF_ADD_CHECKBOXX(label("retro.clamp", "SDR Clamp"), true, 0, ClampOutput);
    PF_ADD_CHECKBOXX(label("dither.enabled", "Enable Dithering"), true, 0, DitherOn);
    PF_ADD_FLOAT_SLIDERX(label("retro.tracking_height", "Tracking Height (%)"), 0, 100, 0, 100, 40, 0, 0, 0,
                         TrackingHeight);
    PF_ADD_FLOAT_SLIDERX(label("retro.tracking_distortion", "Tracking Distortion (%)"), 0, 100, 0, 100, 1,
                         2, 0, 0, TrackingDistortion);
    PF_ADD_FLOAT_SLIDERX(label("retro.tracking_noise", "Tracking Noise (%)"), 0, 100, 0, 100, 60, 0, 0, 0,
                         TrackingNoise);
    PF_ADD_FLOAT_SLIDERX(label("retro.streak_frequency", "Streak Frequency (%)"), 0, 100, 0, 20, 2, 1, 0, 0,
                         StreakFrequency);
    PF_ADD_FLOAT_SLIDERX(label("retro.streak_intensity", "Streak Intensity (%)"), 0, 100, 0, 100, 10, 0, 0, 0,
                         StreakIntensity);
    PF_ADD_FLOAT_SLIDERX(label("retro.streak_height", "Streak Height (%)"), 0.1, 100, 0.1, 100, 2, 1, 0, 0,
                         StreakHeight);
    PF_ADD_FLOAT_SLIDERX(label("retro.dropout_frequency", "Dropout Frequency (%)"), 0, 100, 0, 20, 2, 1,
                         0, 0, DropoutFrequency);
    PF_ADD_FLOAT_SLIDERX(label("retro.dropout_intensity", "Dropout Intensity (%)"), 0, 100, 0, 100, 20, 0,
                         0, 0, DropoutIntensity);
    PF_ADD_FLOAT_SLIDERX(label("retro.dropout_height", "Dropout Height (%)"), 0.1, 100, 0.1, 100, 20, 1,
                         0, 0, DropoutHeight);
    PF_END_TOPIC(EndGroup);
    out_data->num_params = Count;
    return PF_Err_NONE;
}
// Use only in SMART_PRE_RENDER. AE owns checkins for params checked out in this selector.
// ParamsSetup registers controls; PreRender must not register UI controls.
PF_Err CheckoutParams(PF_InData *in_data, retro::Controls &snapshot) {
    PF_ParamDef p[Count]{};
    for (int index = Enabled; index < EndGroup; ++index) {
        auto err = PF_CHECKOUT_PARAM(in_data, index, in_data->current_time, in_data->time_step,
                                     in_data->time_scale, &p[index]);
        if (err)
            return err;
    }
    auto f = [&](int index) { return p[index].u.fs_d.value; };
    snapshot.retroEnabled = p[Enabled].u.bd.value != 0;
    snapshot.order = static_cast<retro::Order>(p[PipelineOrder].u.pd.value);
    snapshot.crt.vhsEnabled = p[VhsOn].u.bd.value != 0;
    snapshot.crt.crtEnabled = p[CrtOn].u.bd.value != 0;
    snapshot.crt.curvature = float(f(Curvature) / 100.0);
    snapshot.crt.scanlineCount = float(f(ScanCount));
    snapshot.crt.scanlineOpacity = float(f(ScanOpacity) / 100.0);
    snapshot.crt.chromaShiftPixels = float(f(ChromaPixels));
    snapshot.crt.wobbleIntensity = float(f(JitterPercent) / 100.0);
    snapshot.crt.wobbleFrequency = float(f(JitterFrequency));
    snapshot.crt.noiseAmount = float(f(NoisePercent) / 100.0);
    snapshot.crt.noiseColor = float(f(NoiseColorPercent) / 100.0);
    snapshot.timeRate = f(TimeRate);
    snapshot.timeOffset = f(TimeOffset);
    snapshot.animateTime = p[AnimateTime].u.bd.value != 0;
    snapshot.crt.clampOutput = p[ClampOutput].u.bd.value != 0;
    snapshot.dither.enabled = p[DitherOn].u.bd.value != 0;
    snapshot.crt.trackingHeight = float(f(TrackingHeight) / 100.0);
    snapshot.crt.trackingDistortion = float(f(TrackingDistortion) / 100.0);
    snapshot.crt.trackingNoise = float(f(TrackingNoise) / 100.0);
    snapshot.crt.streakFrequency = float(f(StreakFrequency) / 100.0);
    snapshot.crt.streakIntensity = float(f(StreakIntensity) / 100.0);
    snapshot.crt.streakHeight = float(f(StreakHeight) / 100.0);
    snapshot.crt.dropoutFrequency = float(f(DropoutFrequency) / 100.0);
    snapshot.crt.dropoutIntensity = float(f(DropoutIntensity) / 100.0);
    snapshot.crt.dropoutHeight = float(f(DropoutHeight) / 100.0);
    // Existing Dither algorithm/palette/tone/work resolution must also be copied by the caller.
    return PF_Err_NONE;
}
// Call from the existing SMART_PRE_RENDER after CheckoutParams, and store the result per frame:
retro::UniformFrame MakeUniforms(PF_InData *in_data, const retro::Controls &immutableSnapshot, int width,
                                 int height) {
    return retro::makeUniformFrame(immutableSnapshot, {width, height}, in_data->current_time,
                                   in_data->time_scale);
}
} // namespace retro::ae
