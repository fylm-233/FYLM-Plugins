#pragma once
#include <array>
#include <cstring>
#if defined(FYLM_SHADERGLASS)
#include "../../modules/shaderglass/library_catalog.hpp"
#elif defined(FYLM_CRT_ONLY)
#include "../../modules/crt_runtime/include/fylm/preset_catalog.hpp"
#endif
namespace fylm {
enum class Kind { Check, Integer, Float, Popup, Blob, Button, Color };
struct Param {const char* key;Kind kind;float low,high,value;int choices=0;const char* items=nullptr;};
#if defined(FYLM_SHADERGLASS)
inline constexpr const char* Name="FYLM ShaderGlass";
inline constexpr const char* Match="FYLM_ShaderGlass";
inline constexpr Param Params[]={
 {"effect.enabled",Kind::Check,0,1,1},
 {"sg.preset",Kind::Popup,1,float(sg::LibraryMenuCount),1,sg::LibraryMenuCount,sg::LibraryMenuItems},
 {"sg.virtual_height",Kind::Integer,0,2160,240},
 {"sg.use_slots",Kind::Check,0,1,0},
 {"sg.slot_a",Kind::Float,0,100,50},{"sg.slot_b",Kind::Float,0,100,50},
 {"sg.slot_c",Kind::Float,0,100,50},{"sg.slot_d",Kind::Float,0,100,50},
 {"sg.slot_e",Kind::Float,0,100,50},{"sg.slot_f",Kind::Float,0,100,50},
 {"sg.slot_g",Kind::Float,0,100,50},{"sg.slot_h",Kind::Float,0,100,50},
 {"sg.data",Kind::Blob,0,0,0},{"sg.load",Kind::Button,0,0,0},
 {"sg.defaults",Kind::Button,0,0,0},{"sg.follow_preset",Kind::Check,0,1,1},
 {"sg.strength",Kind::Float,0,100,100},
 {"sg.animate",Kind::Check,0,1,1},{"sg.time_rate",Kind::Float,-4,4,1},{"sg.time_offset",Kind::Float,-3600,3600,0},
 {"sg.frame_offset",Kind::Integer,-1000000,1000000,0},
 {"sg.slot_i",Kind::Float,0,100,50},{"sg.slot_j",Kind::Float,0,100,50},{"sg.slot_k",Kind::Float,0,100,50},{"sg.slot_l",Kind::Float,0,100,50},
 {"sg.slot_m",Kind::Float,0,100,50},{"sg.slot_n",Kind::Float,0,100,50},{"sg.slot_o",Kind::Float,0,100,50},{"sg.slot_p",Kind::Float,0,100,50},
 {"sg.extended_slots",Kind::Check,0,1,0},
 {"sg.import_source",Kind::Button,0,0,0},{"sg.import_cancel",Kind::Button,0,0,0},{"sg.import_status",Kind::Button,0,0,0},{"sg.advanced",Kind::Button,0,0,0},{"sg.export",Kind::Button,0,0,0}
};
#elif defined(FYLM_CRT_ONLY)
inline constexpr const char* Name="FYLM CRT VHS";
inline constexpr const char* Match="FYLM_CRTVHS";
inline constexpr Param Params[]={
 {"effect.enabled",Kind::Check,0,1,1},
 {"retro.vhs",Kind::Check,0,1,1},{"retro.crt",Kind::Check,0,1,1},
 {"retro.curvature",Kind::Float,0,100,0},{"retro.scan_count",Kind::Float,0,8192,288},
 {"retro.scan_opacity",Kind::Float,0,100,25},{"retro.chroma",Kind::Float,-20,20,2},
 {"retro.jitter",Kind::Float,0,10,.2f},{"retro.jitter_frequency",Kind::Float,0,100,4},
 {"retro.noise",Kind::Float,0,100,10},{"retro.noise_color",Kind::Float,0,100,30},
 {"retro.time_rate",Kind::Float,-4,4,1},{"retro.time_offset",Kind::Float,-3600,3600,0},
 {"retro.animate",Kind::Check,0,1,1},{"retro.clamp",Kind::Check,0,1,1},
 {"retro.tracking_height",Kind::Float,0,100,40},{"retro.tracking_distortion",Kind::Float,0,100,1},
 {"retro.tracking_noise",Kind::Float,0,100,60},{"retro.streak_frequency",Kind::Float,0,100,2},
 {"retro.streak_intensity",Kind::Float,0,100,10},{"retro.streak_height",Kind::Float,.1f,100,2},
 {"retro.dropout_frequency",Kind::Float,0,100,2},{"retro.dropout_intensity",Kind::Float,0,100,20},
 {"retro.dropout_height",Kind::Float,.1f,100,20},
 {"retro.mode",Kind::Popup,1,3,3,3,"CRT|VHS|CRT + VHS"},
 {"retro.brightness",Kind::Float,-1,1,0},{"retro.contrast",Kind::Float,0,3,1},
 {"retro.saturation",Kind::Float,0,3,1},{"retro.mask_type",Kind::Popup,1,3,1,3,"Aperture Grille|Shadow Mask|Slot Mask"},
 {"retro.mask_density",Kind::Float,0,8192,400},{"retro.mask_intensity",Kind::Float,0,100,15},
 {"retro.convergence",Kind::Float,0,100,1},{"retro.halation",Kind::Float,0,100,12},
 {"retro.halation_radius",Kind::Float,0,5,2},{"retro.vignette",Kind::Float,0,100,20},
 {"retro.luma_blur",Kind::Float,0,5,1.2f},{"retro.chroma_bleed",Kind::Float,0,10,4},
 {"retro.scan_sharpness",Kind::Float,.25f,4,1.5f},
 {"retro.signal_model",Kind::Popup,1,2,1,2,"Legacy RGB|YIQ Tape (720 reference pixels)"},
 {"retro.chroma_noise",Kind::Float,0,100,0},{"retro.head_switch",Kind::Float,0,100,0},
 {"retro.seed",Kind::Integer,0,65535,0},{"retro.vhs_strength",Kind::Float,0,100,100},
 {"retro.style",Kind::Popup,1,9,1,9,crt::VhsMenuItems},{"retro.apply_style",Kind::Button,0,0,0}
};
#else
// Display name changed in 0.6. The match name and every parameter id below are frozen:
// AE restores old projects, scripts and expressions through those, not the display name.
inline constexpr const char* Name="FYLM Pixel Art";
inline constexpr const char* Match="FYLM_DitheringStudio";
inline constexpr Param Params[]={
 {"effect.enabled",Kind::Check,0,1,1},
 {"param.algorithm",Kind::Popup,1,9,1,9,"Floyd-Steinberg|Bayer 4x4|Bayer 8x8|Binary Threshold|Blue Noise 64x64|Cover Print Halftone|PC-98|GBA|Modern Pixel Art"},
 {"param.palette",Kind::Popup,1,11,1,11,"--|Black/White|Game Boy|PICO-8|DawnBringer 16|DawnBringer 32|CGA|EGA|Amber CRT|1986 Plasma|Custom palette"},
 {"param.threshold",Kind::Integer,0,255,128},{"param.resolution",Kind::Integer,0,8192,512},
 {"param.invert",Kind::Check,0,1,0},{"param.serpentine",Kind::Check,0,1,1},
 {"param.contrast",Kind::Float,-100,100,0},{"param.midtones",Kind::Float,.5f,2,1},
 {"param.highlights",Kind::Float,0,100,0},
 {"param.alpha",Kind::Popup,1,2,2,2,"Opaque|Preserve source alpha"},
 {"halftone.bake",Kind::Integer,8,2048,96},{"halftone.pitch",Kind::Float,1,16,2.5f},
 {"color.linear_srgb",Kind::Check,0,1,0},
 {"pa.block",Kind::Integer,1,64,3},
 {"pa.matrix",Kind::Popup,1,4,3,4,"None|Bayer 2x2|Bayer 4x4|Bayer 8x8"},
 {"pa.strength",Kind::Float,0,100,65},
 {"pa.edge",Kind::Float,0,100,80},
 {"pa.palette",Kind::Popup,1,3,1,3,"FYLM PC-98|Imported palette|Fixed reference frame"},
 {"pa.reference",Kind::Float,0,36000,0},
 {"pa.colors",Kind::Integer,2,16,16},
 {"pa.origin_x",Kind::Integer,-32768,32768,0},
 {"pa.origin_y",Kind::Integer,-32768,32768,0},
 {"pa.encoding",Kind::Popup,1,2,1,2,"sRGB encoded|Linear sRGB"},
 {"pa.alpha",Kind::Popup,1,2,1,2,"Coverage|Binary threshold"},
 {"pa.alpha_threshold",Kind::Float,0,1,.5f},
 {"pa.data",Kind::Blob,0,0,0},
 {"pa.import",Kind::Button,0,0,0},
 {"pa.preset",Kind::Button,0,0,0},
 {"pa.export",Kind::Button,0,0,0},
 {"gba.block",Kind::Integer,1,64,6},
 {"gba.colors",Kind::Integer,2,64,48},
 {"gba.sampling",Kind::Popup,1,3,3,3,"Nearest|Area average|Feature representative"},
 {"gba.feature",Kind::Float,0,100,65},
 {"gba.outline",Kind::Float,0,100,15},
 {"modern.block",Kind::Integer,1,64,3},
 {"modern.colors",Kind::Integer,2,128,96},
 {"modern.sampling",Kind::Popup,1,3,3,3,"Nearest|Area average|Feature representative"},
 {"modern.merge",Kind::Float,0,100,35},
 {"modern.cluster_area",Kind::Integer,0,32,4},
 {"modern.speckles",Kind::Check,0,1,1},
 {"modern.spurs",Kind::Check,0,1,1},
 {"modern.stairs",Kind::Check,0,1,0},
 {"modern.iterations",Kind::Integer,1,3,2},
 {"modern.protection",Kind::Float,0,100,80},
 {"modern.debug",Kind::Popup,1,2,1,2,"Final image|Changed pixels mask"},
 {"modern.outline",Kind::Float,0,100,0},
 {"modern.feature",Kind::Float,0,100,50},
 {"library.recipe",Kind::Popup,1,9,1,9,"Select a style|PA02 Soft Desktop 98|PA03 Midnight 98|PA04 Sunny Cartridge|PA06 Orbital Cartridge|PA07 Quiet Town|PA08 Crystal Journey|PA09 Neon Afterhours|PA12 Two-Tone Chronicle"},
 {"library.variant",Kind::Popup,1,2,1,2,"A|B"},
 {"library.apply",Kind::Button,0,0,0},
 // 49-51 remain reserved for 0.9.0 project compatibility; mode-local controls append.
 {"library.pc98.preset",Kind::Popup,1,9,1,9,"Select a preset|PA02 A|PA02 B|PA03 A|PA03 B|PA12 A|PA12 B|PA01 A|PA01 B"},
 {"library.pc98.apply",Kind::Button,0,0,0},
 {"library.gba.preset",Kind::Popup,1,7,1,7,"Select a preset|PA04 A|PA04 B|PA06 A|PA06 B|PA05 A|PA05 B"},
 {"library.gba.apply",Kind::Button,0,0,0},
 {"library.modern.preset",Kind::Popup,1,11,1,11,"Select a preset|PA07 A|PA07 B|PA08 A|PA08 B|PA09 A|PA09 B|PA10 A|PA10 B|PA11 A|PA11 B"},
 {"library.modern.apply",Kind::Button,0,0,0},
 {"pa.spec",Kind::Popup,1,6,1,6,"Art palette (legacy)|PC-9801 Digital 8 (3-bit)|PC-9801 Analog 16 (4-bit index / RGB444)|PC-9821 Indexed 256 (8-bit index)|PC-9821 High Colour (RGB565 / 16-bit)|PC-9821 True Colour (RGB888 / 24-bit)"},
 {"pa.indexed_colors",Kind::Integer,2,256,256},
 {"pa.dac",Kind::Popup,1,2,2,2,"6 bits/channel (18-bit RGB / 262144 colours)|8 bits/channel (24-bit RGB / 16777216 colours)"},
 {"custom.count",Kind::Popup,1,7,3,7,"2|3|4|5|6|7|8"},
 {"custom.color1",Kind::Color,0,16777215,997391},
 {"custom.color2",Kind::Color,0,16777215,3170864},
 {"custom.color3",Kind::Color,0,16777215,9153551},
 {"custom.color4",Kind::Color,0,16777215,10206223},
 {"custom.color5",Kind::Color,0,16777215,12573823},
 {"custom.color6",Kind::Color,0,16777215,14477482},
 {"custom.color7",Kind::Color,0,16777215,15659724},
 {"custom.color8",Kind::Color,0,16777215,16777215},
 {"halftone.mode",Kind::Popup,1,2,1,2,"Original cover print|Smooth CMYK dots"},
 {"halftone.angle",Kind::Float,-180,180,0},
 {"halftone.gain",Kind::Float,-50,50,0},
 {"param.bayer_size",Kind::Popup,1,4,1,4,"Follow algorithm|2 x 2|4 x 4|8 x 8"}
};
#endif
inline constexpr int Count=int(sizeof(Params)/sizeof(Params[0]))+1;
inline int index(const char* key){for(int i=1;i<Count;++i)if(std::strcmp(Params[i-1].key,key)==0)return i;return -1;}
inline constexpr const char* Category="FYLM Plugins";
}
