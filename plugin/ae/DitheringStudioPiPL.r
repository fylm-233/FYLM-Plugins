#include "AEConfig.h"
#include "AE_EffectVers.h"
resource 'PiPL' (16000) {
 { Kind { AEEffect }, Name { "Dithering Studio" }, Category { "Stylize" },
 CodeWin64X86 { "EffectMain" }, AE_PiPL_Version {2,0},
 AE_Effect_Spec_Version {PF_PLUG_IN_VERSION,PF_PLUG_IN_SUBVERS},
 AE_Effect_Version {98305}, AE_Effect_Info_Flags {0},
 AE_Effect_Global_OutFlags {0x06000004},
#ifdef DITHER_ENABLE_MFR
 AE_Effect_Global_OutFlags_2 {0x08000480},
#else
 AE_Effect_Global_OutFlags_2 {0x00000480},
#endif
 AE_Effect_Match_Name {"OSLO_DitheringStudio"}, AE_Reserved_Info {8},
 AE_Effect_Support_URL {"https://github.com/Oslonline/dithering-studio"} }
};

