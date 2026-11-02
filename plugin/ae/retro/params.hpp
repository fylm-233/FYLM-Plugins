#pragma once
#include "AE_Effect.h"
#include "retro/retro.hpp"
namespace retro::ae {
enum Index { Group=13,Enabled,PipelineOrder,VhsOn,CrtOn,Curvature,ScanCount,ScanOpacity,
 ChromaPixels,JitterPercent,JitterFrequency,NoisePercent,NoiseColorPercent,TimeRate,TimeOffset,
 AnimateTime,ClampOutput,DitherOn,TrackingHeight,TrackingDistortion,TrackingNoise,
 StreakFrequency,StreakIntensity,StreakHeight,DropoutFrequency,DropoutIntensity,DropoutHeight,
 EndGroup,Count };
using LabelFn=const char*(*)(const char*,const char*);
PF_Err AppendParams(PF_InData*,PF_OutData*,LabelFn);
PF_Err CheckoutParams(PF_InData*,retro::Controls&);
const char* labelKey(int index) noexcept;
}
