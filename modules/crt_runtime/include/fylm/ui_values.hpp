#pragma once
#include "retro/retro.hpp"
#include "fylm/backend.hpp"
namespace fylm::crt {
template<class Value> SignalSettings signalFromUi(Value value){return {int(value("retro.signal_model"))-1,int(value("retro.seed")),float(value("retro.chroma_noise")/100),float(value("retro.head_switch")/100),float(value("retro.vhs_strength")/100)};}
template<class Value> retro::Controls controlsFromUi(Value value){retro::Controls result;
    result.retroEnabled=true;result.dither.enabled=0;
    auto& c=result.crt;c.vhsEnabled=value("retro.vhs")!=0;c.crtEnabled=value("retro.crt")!=0;
    c.curvature=float(value("retro.curvature")/100);c.scanlineCount=float(value("retro.scan_count"));
    c.scanlineOpacity=float(value("retro.scan_opacity")/100);c.chromaShiftPixels=float(value("retro.chroma"));
    c.wobbleIntensity=float(value("retro.jitter")/100);c.wobbleFrequency=float(value("retro.jitter_frequency"));
    c.noiseAmount=float(value("retro.noise")/100);c.noiseColor=float(value("retro.noise_color")/100);
    result.timeRate=value("retro.time_rate");result.timeOffset=value("retro.time_offset");result.animateTime=value("retro.animate")!=0;c.clampOutput=value("retro.clamp")!=0;
    c.trackingHeight=float(value("retro.tracking_height")/100);c.trackingDistortion=float(value("retro.tracking_distortion")/100);c.trackingNoise=float(value("retro.tracking_noise")/100);
    c.streakFrequency=float(value("retro.streak_frequency")/100);c.streakIntensity=float(value("retro.streak_intensity")/100);c.streakHeight=float(value("retro.streak_height")/100);
    c.dropoutFrequency=float(value("retro.dropout_frequency")/100);c.dropoutIntensity=float(value("retro.dropout_intensity")/100);c.dropoutHeight=float(value("retro.dropout_height")/100);
    int mode=int(value("retro.mode"));c.vhsEnabled=c.vhsEnabled&&mode!=1;c.crtEnabled=c.crtEnabled&&mode!=2;
    c.brightness=float(value("retro.brightness"));c.contrast=float(value("retro.contrast"));c.saturation=float(value("retro.saturation"));
    c.maskType=int(value("retro.mask_type"));c.maskDensity=float(value("retro.mask_density"));c.maskIntensity=float(value("retro.mask_intensity")/100);
    c.convergence=float(value("retro.convergence")/100);c.halationIntensity=float(value("retro.halation")/100);c.halationRadius=float(value("retro.halation_radius"));
    c.vignetteIntensity=float(value("retro.vignette")/100);c.luminanceBlur=float(value("retro.luma_blur"));c.chromaBleed=float(value("retro.chroma_bleed"));c.scanlineSharpness=float(value("retro.scan_sharpness"));
 return result;
}
}
