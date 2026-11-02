#define PF_DEEP_COLOR_AWARE 1
#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_EffectSuites.h"
#include "AE_PluginData.h"
#include "entry.h"
#include "dither/core.hpp"
#include "dither/i18n.hpp"
#include "retro/gpu_bridge.hpp"
#include "retro/workspace.hpp"
#ifdef FYLM_CRT_ONLY
#include "fylm/backend.hpp"
#include "fylm/ui_values.hpp"
#endif
#include "FYLMParams.hpp"
#include "FYLMVersion.hpp"
#ifdef FYLM_SHADERGLASS
#include "ShaderGlassAE.hpp"
#elif defined(FYLM_CRT_ONLY)
#include "RetroPresetsAE.hpp"
#elif !defined(FYLM_CRT_ONLY)
#include "PixelArtAE.hpp"
#endif
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
namespace {
namespace loc=dither::i18n;
void diagnostic(PF_InData*,const char*,PF_Err,const char* detail=nullptr) noexcept;
loc::Locale locale(PF_InData* in) noexcept {
    if(!in||!in->pica_basicP)return loc::Locale::English;
    const PFAppSuite6* s=nullptr;
    if(in->pica_basicP->AcquireSuite(kPFAppSuite,kPFAppSuiteVersion6,reinterpret_cast<const void**>(&s))||!s)return loc::Locale::English;
    char tag[PF_APP_LANG_TAG_SIZE]{};auto error=s->PF_AppGetLanguage(tag);
    in->pica_basicP->ReleaseSuite(kPFAppSuite,kPFAppSuiteVersion6);
    return error?loc::Locale::English:loc::fromCode(tag);
}
std::size_t gpuBudget() noexcept {
    return retro::WorkspaceLease::limit;
}
PF_Err error(PF_OutData* out,loc::Locale language,const char* key,PF_Err code){
    if(code!=PF_Interrupt_CANCEL){loc::copyUtf8(out->return_msg,sizeof(out->return_msg),loc::text(language,key));out->out_flags|=PF_OutFlag_DISPLAY_ERROR_MESSAGE;}
    return code;
}
struct Frame {
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
    bool pc98=false,pixelEnabled=false;pixel_art::Params pixel;int paletteSource=1,paletteLimit=16;A_long referenceTime=0;
#endif
#ifdef FYLM_CRT_ONLY
    fylm::crt::SignalSettings tape;
#endif
#ifdef FYLM_SHADERGLASS
    std::shared_ptr<const fylm::sg::Preset> preset;fylm::sg::Settings settings;
#endif
    PF_LRect bounds{};loc::Locale language=loc::Locale::English;
    dither::RenderParams dither;retro::Controls retro;bool enabled=true,linear=false;
    std::int64_t time=0;std::uint64_t scale=1;
};
void dispose(void* data){delete static_cast<Frame*>(data);}
// Every PF_CHECKOUT_PARAM needs a matching PF_CHECKIN_PARAM, including early returns,
// validation failures, arbitrary-data reads and exception unwinding. AE keeps the
// parameter locked until check-in, so a missing pair leaks memory and can deadlock.
struct ParamLease {
    PF_InData* in;PF_ParamDef* param;
    ParamLease(PF_InData* data,PF_ParamDef* value):in(data),param(value){}
    ~ParamLease(){release();}
    ParamLease(const ParamLease&)=delete;ParamLease& operator=(const ParamLease&)=delete;
    void release() noexcept{
        PF_ParamDef* checked=param;param=nullptr;
        if(!checked||!in->inter.checkin_param)return;
        (*in->inter.checkin_param)(in->effect_ref,checked);
    }
};
bool disabled(int i,PF_ParamDef* params[]){
#ifdef FYLM_CRT_ONLY
    int mode=params[fylm::index("retro.mode")]->u.pd.value;const char* key=fylm::Params[i-1].key;
    if(i>=39){if(mode==1||!params[fylm::index("retro.vhs")]->u.bd.value)return true;if(i>=40&&i<=42&&params[fylm::index("retro.signal_model")]->u.pd.value==1)return true;if(std::strcmp(key,"retro.apply_style")==0&&params[fylm::index("retro.style")]->u.pd.value==1)return true;}
    if(mode==1){for(const char* v:{"retro.vhs","retro.chroma","retro.jitter","retro.jitter_frequency","retro.noise","retro.noise_color","retro.tracking_height","retro.tracking_distortion","retro.tracking_noise","retro.streak_frequency","retro.streak_intensity","retro.streak_height","retro.dropout_frequency","retro.dropout_intensity","retro.dropout_height","retro.luma_blur","retro.chroma_bleed"})if(std::strcmp(key,v)==0)return true;}
    // Brightness, contrast and saturation belong to the display stage and stay available in
    // all three modes; only genuinely CRT-only or VHS-only controls are gated here.
    if(mode==2&&params[fylm::index("retro.signal_model")]->u.pd.value==2)for(const char* v:{"retro.scan_count","retro.scan_opacity","retro.scan_sharpness"})if(std::strcmp(key,v)==0)return true;
    if(mode==2){for(const char* v:{"retro.crt","retro.curvature","retro.mask_type","retro.mask_density","retro.mask_intensity","retro.convergence","retro.halation","retro.halation_radius","retro.vignette"})if(std::strcmp(key,v)==0)return true;}
#endif
#ifdef FYLM_SHADERGLASS
    const char* key=fylm::Params[i-1].key;
    if(std::strcmp(key,"sg.follow_preset")==0)return params[fylm::index("sg.preset")]->u.pd.value<=3;
    if(std::strcmp(key,"sg.virtual_height")==0)return false;
    if(std::strcmp(key,"sg.time_rate")==0)return params[fylm::index("sg.animate")]->u.bd.value==0;
    if(std::strncmp(key,"sg.slot_",8)==0)return params[fylm::index("sg.use_slots")]->u.bd.value==0||(key[8]>='i'&&!params[fylm::index("sg.extended_slots")]->u.bd.value);
#endif
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
    int algorithm=params[fylm::index("param.algorithm")]->u.pd.value;
    const char* key=fylm::Params[i-1].key;
    if(std::strncmp(key,"custom.",7)==0)return algorithm>5||params[fylm::index("param.palette")]->u.pd.value!=11||(i>=62&&i<=69&&i-61>params[fylm::index("custom.count")]->u.pd.value+1);
    if(std::strcmp(key,"param.bayer_size")==0)return algorithm!=2&&algorithm!=3;
    if(std::strcmp(key,"halftone.mode")==0)return algorithm!=6;
    if(std::strcmp(key,"halftone.angle")==0||std::strcmp(key,"halftone.gain")==0)return algorithm!=6||params[fylm::index("halftone.mode")]->u.pd.value!=2;
    // Retired hardware profile slots remain serialized for project compatibility.
    if(std::strcmp(key,"pa.spec")==0||std::strcmp(key,"pa.indexed_colors")==0||std::strcmp(key,"pa.dac")==0)return true;
    if(std::strncmp(key,"library.",8)==0){int mode=paLibraryMode(i);return mode==0||mode!=algorithm||(i%2==1&&params[paLibraryChooser(mode)]->u.pd.value==1);}
    if(std::strncmp(key,"gba.",4)==0)return algorithm!=8||(std::strcmp(key,"gba.feature")==0&&params[fylm::index("gba.sampling")]->u.pd.value!=3);
    if(std::strncmp(key,"modern.",7)==0){if(algorithm!=9)return true;if(std::strcmp(key,"modern.feature")==0)return params[fylm::index("modern.sampling")]->u.pd.value!=3;if(std::strcmp(key,"modern.cluster_area")==0)return params[fylm::index("modern.merge")]->u.fs_d.value<=0;return false;}
    if(std::strncmp(key,"pa.",3)==0){if(algorithm<7)return true;if(algorithm>7)for(auto name:{"pa.block","pa.matrix","pa.strength","pa.edge","pa.colors"})if(std::strcmp(key,name)==0)return true;if(std::strcmp(key,"pa.reference")==0)return params[fylm::index("pa.palette")]->u.pd.value!=3;if(std::strcmp(key,"pa.export")==0)return params[fylm::index("pa.palette")]->u.pd.value==3;if(std::strcmp(key,"pa.alpha_threshold")==0)return params[fylm::index("pa.alpha")]->u.pd.value!=2;if(std::strcmp(key,"pa.strength")==0)return params[fylm::index("pa.matrix")]->u.pd.value==1;if(std::strcmp(key,"pa.edge")==0)return params[fylm::index("pa.matrix")]->u.pd.value==1||params[fylm::index("pa.strength")]->u.fs_d.value<=0;return false;}
    if(algorithm>=7)return std::strcmp(key,"param.algorithm")!=0&&std::strcmp(key,"effect.enabled")!=0;
    if(std::strcmp(key,"param.serpentine")==0)return algorithm!=1;
    if(std::strncmp(key,"halftone.",9)==0||std::strcmp(key,"color.linear_srgb")==0)return algorithm!=6;
    if(algorithm==6)return std::strcmp(key,"param.algorithm")!=0&&std::strcmp(key,"effect.enabled")!=0;
#endif
    return false;
}
PF_Err setup(PF_InData* in,PF_OutData* out){
    auto language=locale(in);
    for(int i=1;i<fylm::Count;++i){const auto& s=fylm::Params[i-1];PF_ParamDef d{};
        loc::copyUtf8(d.PF_DEF_NAME,sizeof(d.PF_DEF_NAME),loc::text(language,s.key));d.uu.id=i;
#ifdef FYLM_CRT_ONLY
        if(s.kind==fylm::Kind::Button){d.param_type=PF_Param_BUTTON;d.u.button_d.u.PF_DEF_NAMESPTR=loc::text(language,s.key).data();d.flags=PF_ParamFlag_SUPERVISE;}else
#endif
#ifdef FYLM_SHADERGLASS
        if(s.kind==fylm::Kind::Blob){d.param_type=PF_Param_ARBITRARY_DATA;d.ui_flags=PF_PUI_INVISIBLE;d.flags=PF_ParamFlag_CANNOT_TIME_VARY;d.u.arb_d.id=A_short(i);d.u.arb_d.dephault=sgMake(in,{});}
        else if(s.kind==fylm::Kind::Button){d.param_type=PF_Param_BUTTON;d.u.button_d.u.PF_DEF_NAMESPTR=loc::text(language,s.key).data();d.flags=PF_ParamFlag_SUPERVISE;}
        else
#elif !defined(FYLM_CRT_ONLY)
        if(s.kind==fylm::Kind::Blob){d.param_type=PF_Param_ARBITRARY_DATA;d.ui_flags=PF_PUI_INVISIBLE;d.flags=PF_ParamFlag_CANNOT_TIME_VARY;d.u.arb_d.id=A_short(i);d.u.arb_d.dephault=paMake(in,{});}
        else if(s.kind==fylm::Kind::Button){d.param_type=PF_Param_BUTTON;d.u.button_d.u.PF_DEF_NAMESPTR=loc::text(language,s.key).data();d.flags=PF_ParamFlag_SUPERVISE;}
        else
#endif
        if(s.kind==fylm::Kind::Color){d.param_type=PF_Param_COLOR;unsigned rgb=unsigned(s.value);d.u.cd.value=d.u.cd.dephault=PF_Pixel{255,A_u_char(rgb>>16),A_u_char(rgb>>8),A_u_char(rgb)};}
        else if(s.kind==fylm::Kind::Check){d.param_type=PF_Param_CHECKBOX;d.u.bd.value=d.u.bd.dephault=s.value!=0;d.u.bd.u.PF_DEF_NAMEPTR="";}
        else if(s.kind==fylm::Kind::Integer){d.param_type=PF_Param_SLIDER;d.u.sd.value=d.u.sd.dephault=A_long(s.value);d.u.sd.valid_min=d.u.sd.slider_min=A_long(s.low);d.u.sd.valid_max=d.u.sd.slider_max=A_long(s.high);}
        else if(s.kind==fylm::Kind::Popup){d.param_type=PF_Param_POPUP;d.u.pd.num_choices=s.choices;d.u.pd.value=d.u.pd.dephault=A_long(s.value);d.u.pd.u.PF_DEF_NAMESPTR=s.items;d.flags|=PF_ParamFlag_CANNOT_TIME_VARY|PF_ParamFlag_SUPERVISE;
#ifdef FYLM_CRT_ONLY
         if(std::strcmp(s.key,"retro.style")==0)d.u.pd.u.PF_DEF_NAMESPTR=loc::text(language,"retro.style.items").data();
         if(std::strcmp(s.key,"retro.signal_model")==0)d.u.pd.u.PF_DEF_NAMESPTR=loc::text(language,"retro.signal_model.items").data();
#endif
#ifdef FYLM_SHADERGLASS
         if(std::strcmp(s.key,"sg.preset")==0)d.u.pd.u.PF_DEF_NAMESPTR=loc::text(language,"sg.preset.items").data();
#endif
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
         if(std::strncmp(s.key,"library.",8)==0){auto key=std::string(s.key);if(key=="library.recipe")d.u.pd.u.PF_DEF_NAMESPTR=loc::text(language,"library.items").data();else if(key.size()>7&&key.substr(key.size()-7)==".preset"){key=key.substr(0,key.size()-7)+".items";d.u.pd.u.PF_DEF_NAMESPTR=loc::text(language,key).data();}}
         if(std::strcmp(s.key,"param.palette")==0||std::strcmp(s.key,"halftone.mode")==0||std::strcmp(s.key,"param.bayer_size")==0)d.u.pd.u.PF_DEF_NAMESPTR=loc::text(language,std::string(s.key)+".items").data();
         if(std::strcmp(s.key,"pa.spec")==0)d.u.pd.u.PF_DEF_NAMESPTR=loc::text(language,"pa.spec.items").data();
         if(std::strcmp(s.key,"pa.dac")==0)d.u.pd.u.PF_DEF_NAMESPTR=loc::text(language,"pa.dac.items").data();
#endif
        }
        else{d.param_type=PF_Param_FLOAT_SLIDER;d.u.fs_d.value=d.u.fs_d.dephault=s.value;d.u.fs_d.valid_min=d.u.fs_d.slider_min=s.low;d.u.fs_d.valid_max=d.u.fs_d.slider_max=s.high;d.u.fs_d.precision=PF_Precision_HUNDREDTHS;d.u.fs_d.curve_tolerance=1;}
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
        if(i>=49&&i<=51)d.ui_flags|=PF_PUI_INVISIBLE;
        if(std::strcmp(s.key,"pa.strength")==0||std::strcmp(s.key,"modern.merge")==0)d.flags|=PF_ParamFlag_SUPERVISE;
#endif
#ifdef FYLM_SHADERGLASS
        // These switches change other controls' enabled state. AE must send the
        // supervised UI callback immediately, including when no frame is rendered.
        if(std::strcmp(s.key,"sg.virtual_height")==0||std::strcmp(s.key,"sg.use_slots")==0||std::strcmp(s.key,"sg.extended_slots")==0||std::strcmp(s.key,"sg.follow_preset")==0||std::strcmp(s.key,"sg.animate")==0)d.flags|=PF_ParamFlag_SUPERVISE;
#endif
        auto err=PF_ADD_PARAM(in,-1,&d);if(err)return err;
    }
    out->num_params=fylm::Count;return 0;
}
PF_Err update(PF_InData* in,PF_ParamDef* params[]){
    if(!params||!in->pica_basicP)return 0;
    const PF_ParamUtilsSuite3* s=nullptr;
    auto err=in->pica_basicP->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&s));
    if(err||!s)return err?err:PF_Err_BAD_CALLBACK_PARAM;
#ifdef FYLM_SHADERGLASS
    std::shared_ptr<const fylm::sg::Preset> selected;
    try{int choice=params[fylm::index("sg.preset")]->u.pd.value;selected=sgPreset(choice,choice==3?sgRead(in,params[fylm::index("sg.data")]->u.arb_d.value):std::vector<unsigned char>{});}catch(...){}
    unsigned available=0,active=0;unsigned long long updated=0,failed=0;int firstFailed=0;
    for(unsigned slot=0;selected&&slot<16;++slot)if(fylm::sg::slotControl(*selected,slot))available|=1u<<slot;
#endif
    for(int i=1;i<fylm::Count;++i){auto d=*params[i];
#ifndef FYLM_SHADERGLASS
        if(err)break;
#endif
        if(fylm::Params[i-1].kind==fylm::Kind::Blob)continue;
        loc::copyUtf8(d.PF_DEF_NAME,sizeof(d.PF_DEF_NAME),loc::text(locale(in),fylm::Params[i-1].key));
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
        if(i==fylm::index("pa.palette")){int mode=params[2]->u.pd.value;d.u.pd.u.PF_DEF_NAMESPTR=mode==8?"FYLM GBA|Imported palette|Fixed reference frame":mode==9?"FYLM Modern|Imported palette|Fixed reference frame":fylm::Params[i-1].items;}
#endif
#ifdef FYLM_SHADERGLASS
        auto key=std::string(fylm::Params[i-1].key);
        if(key=="sg.import_status")loc::copyUtf8(d.PF_DEF_NAME,sizeof(d.PF_DEF_NAME),loc::text(locale(in),sgImportStatus));
        if(key=="sg.virtual_height"&&params[fylm::index("sg.preset")]->u.pd.value>=4&&params[fylm::index("sg.follow_preset")]->u.bd.value){auto label=std::string(loc::text(locale(in),"sg.input_auto"));auto pos=label.find("{height}");if(pos!=std::string::npos)label.replace(pos,8,std::to_string(fylm::sg::recommendedHeight(params[2]->u.pd.value,params[3]->u.sd.value,true)));loc::copyUtf8(d.PF_DEF_NAME,sizeof(d.PF_DEF_NAME),label);}
        if(selected&&key.starts_with("sg.slot_")){int slot=key.back()-'a';if(slot>=0&&slot<16){auto control=fylm::sg::slotControl(*selected,slot);if(control){auto translationKey=std::string("sg.uniform.")+control->name;auto translated=loc::text(locale(in),translationKey);auto display=translated==translationKey?control->name:std::string(translated);auto label=std::string(1,char('A'+slot))+": "+display+" (%)";loc::copyUtf8(d.PF_DEF_NAME,sizeof(d.PF_DEF_NAME),label);}else{auto label=std::string(1,char('A'+slot))+": "+std::string(loc::text(locale(in),"sg.unavailable"));loc::copyUtf8(d.PF_DEF_NAME,sizeof(d.PF_DEF_NAME),label);}}}
#endif
        bool inactive=disabled(i,params);
#ifdef FYLM_SHADERGLASS
        if(key.starts_with("sg.slot_")&&selected&&!fylm::sg::slotControl(*selected,key.back()-'a'))inactive=true;
        if(key=="sg.use_slots"&&selected&&!available)inactive=true;
        if(key=="sg.extended_slots"&&selected&&!(available&0xff00))inactive=true;
        if(key=="sg.advanced"&&selected&&selected->controls.empty())inactive=true;
        if(key=="sg.import_cancel"&&!sgImportTask)inactive=true;
#endif
        if(inactive)d.ui_flags|=PF_PUI_DISABLED;else d.ui_flags&=~PF_PUI_DISABLED;
        auto rowError=s->PF_UpdateParamUI(in->effect_ref,i,&d);
#ifdef FYLM_SHADERGLASS
        // UI rows are independent: an earlier cosmetic update must not prevent
        // the I-P bank from receiving its enabled state. Preserve the first error.
        if(rowError){failed|=1ull<<i;if(!firstFailed)firstFailed=i;}
        else updated|=1ull<<i;
        if(key.starts_with("sg.slot_")&&!inactive)active|=1u<<(key.back()-'a');
#endif
        if(!err)err=rowError;
    }
#ifdef FYLM_SHADERGLASS
    // Log state transitions, not every frame. No image/project content is recorded.
    char state[256]{};
    std::snprintf(state,sizeof(state),"effect=%p,preset=%ld,custom=%d,extra=%d,loaded=%d,available=%04x,active=%04x,updated=%llx,failed=%llx,firstFailed=%d",
        in->effect_ref,long(params[2]->u.pd.value),int(params[4]->u.bd.value),int(params[30]->u.bd.value),selected?1:0,available,active,updated,failed,firstFailed);
    static thread_local std::string lastUiState;
    if(lastUiState!=state){diagnostic(in,"sg-ui-state",err,state);lastUiState=state;}
#endif
    in->pica_basicP->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
    if(!err)paLibraryVisibility(in,params[2]->u.pd.value,params,disabled);
#endif
    return err;
}
PF_Err pre(PF_InData* in,PF_PreRenderExtra* e){
    auto f=std::make_unique<Frame>();f->language=locale(in);f->time=in->current_time;f->scale=in->time_scale;
    std::array<double,fylm::Count> values{};
#if !defined(FYLM_CRT_ONLY)
    std::vector<unsigned char> presetBytes;
#endif
    std::vector<dither::Rgb8> customColors;
    for(int i=1;i<fylm::Count;++i){PF_ParamDef p{};auto err=PF_CHECKOUT_PARAM(in,i,in->current_time,in->time_step,in->time_scale,&p);if(err)return err;
        ParamLease lease(in,&p);
#ifdef FYLM_SHADERGLASS
        if(fylm::Params[i-1].kind==fylm::Kind::Blob){if(values[fylm::index("sg.preset")]==3)presetBytes=sgRead(in,p.u.arb_d.value);continue;}
        if(fylm::Params[i-1].kind==fylm::Kind::Button)continue;
#elif !defined(FYLM_CRT_ONLY)
        if(fylm::Params[i-1].kind==fylm::Kind::Blob){presetBytes=paRead(in,p.u.arb_d.value);continue;}
        if(fylm::Params[i-1].kind==fylm::Kind::Button)continue;
#endif
        if(fylm::Params[i-1].kind==fylm::Kind::Color){customColors.push_back({p.u.cd.value.red,p.u.cd.value.green,p.u.cd.value.blue});continue;}
        switch(fylm::Params[i-1].kind){case fylm::Kind::Check:values[i]=p.u.bd.value;break;case fylm::Kind::Integer:values[i]=p.u.sd.value;break;case fylm::Kind::Popup:values[i]=p.u.pd.value;break;default:values[i]=p.u.fs_d.value;}
        const auto& spec=fylm::Params[i-1];
        if(!std::isfinite(values[i])||values[i]<spec.low||values[i]>spec.high||
           (spec.kind!=fylm::Kind::Float&&values[i]!=std::round(values[i])))return PF_Err_BAD_CALLBACK_PARAM;
    }
    auto value=[&](const char* key){int i=fylm::index(key);return i>0?values[i]:0.;};
    f->enabled=value("effect.enabled")!=0;
#if defined(FYLM_SHADERGLASS)
    if(f->enabled)f->preset=sgPreset(int(value("sg.preset")),presetBytes);
    f->settings.virtualHeight=fylm::sg::recommendedHeight(int(value("sg.preset")),int(value("sg.virtual_height")),value("sg.follow_preset")!=0);f->settings.slots=value("sg.use_slots")!=0;
    const char* slots[]={"sg.slot_a","sg.slot_b","sg.slot_c","sg.slot_d","sg.slot_e","sg.slot_f","sg.slot_g","sg.slot_h","sg.slot_i","sg.slot_j","sg.slot_k","sg.slot_l","sg.slot_m","sg.slot_n","sg.slot_o","sg.slot_p"};for(int i=0;i<16;++i)f->settings.values[i]=float(value(slots[i]));
    f->settings.mix=float(value("sg.strength")/100);
    f->settings.extendedSlots=value("sg.extended_slots")!=0;
    f->settings.frameDirection=value("sg.time_rate")<0?-1:1;f->settings.pixelAspect=in->pixel_aspect_ratio.num>0&&in->pixel_aspect_ratio.den>0?float(in->pixel_aspect_ratio.num)/in->pixel_aspect_ratio.den:1;
    auto timing=fylm::sg::sampleTime(in->current_time,in->time_scale,in->time_step,value("sg.animate")!=0,value("sg.time_rate"),value("sg.time_offset"),int(value("sg.frame_offset")));f->settings.frameIndex=timing.frame;f->settings.timeSeconds=timing.seconds;f->settings.frameRate=timing.frameRate;
#elif defined(FYLM_CRT_ONLY)
    f->tape=fylm::crt::signalFromUi(value);f->retro=fylm::crt::controlsFromUi(value);auto& c=f->retro.crt;
    f->enabled=f->enabled&&(c.vhsEnabled||c.crtEnabled);
#else
    constexpr int algorithms[]={1,2,8,15,17,31,1001,1002,1003};constexpr const char* palettes[]={"none","bw","gb","pico8","db16","db32","cga16","ega16","amber_crt","plasma1986","none"};
    f->dither.algorithmId=algorithms[int(value("param.algorithm"))-1];f->dither.palette=dither::palette(palettes[int(value("param.palette"))-1]);
    if(int(value("param.palette"))==11){customColors.resize(std::size_t(value("custom.count"))+1);std::stable_sort(customColors.begin(),customColors.end(),[](auto a,auto b){return 2126*a.r+7152*a.g+722*a.b<2126*b.r+7152*b.g+722*b.b;});f->dither.palette=customColors;}
    constexpr int matricesClassic[]={0,2,4,8};f->dither.orderedMatrix=matricesClassic[int(value("param.bayer_size"))-1];
    f->dither.smoothHalftone=value("halftone.mode")==2;f->dither.halftoneAngle=float(value("halftone.angle"));f->dither.halftoneGain=float(value("halftone.gain"));
    f->dither.threshold=int(value("param.threshold"));f->dither.workingResolution=int(value("param.resolution"));
    f->dither.invert=value("param.invert")!=0;f->dither.serpentine=value("param.serpentine")!=0;
    f->dither.tone={value("param.contrast"),value("param.midtones"),value("param.highlights")};
    f->dither.alphaMode=value("param.alpha")==2?dither::AlphaMode::Preserve:dither::AlphaMode::Opaque;
    if(f->dither.algorithmId==31){f->dither.workingResolution=int(value("halftone.bake"));f->dither.halftonePitch=float(value("halftone.pitch"));f->dither.alphaMode=dither::AlphaMode::Preserve;f->linear=value("color.linear_srgb")!=0;}
    f->pc98=f->dither.algorithmId==1001;f->pixelEnabled=f->dither.algorithmId>=1001;
    if(f->pixelEnabled){auto& p=f->pixel;p.pc98=f->pc98;p.style=f->dither.algorithmId==1002?pixel_art::Style::GBA:f->dither.algorithmId==1003?pixel_art::Style::Modern:pixel_art::Style::Basic;p.grid.block=int(value(f->pc98?"pa.block":p.style==pixel_art::Style::GBA?"gba.block":"modern.block"));p.grid.originX=int(value("pa.origin_x"));p.grid.originY=int(value("pa.origin_y"));p.grid.pixelAspect=in->pixel_aspect_ratio.num>0&&in->pixel_aspect_ratio.den>0?double(in->pixel_aspect_ratio.num)/in->pixel_aspect_ratio.den:1.;
      p.grid.downsampleX=in->downsample_x.num>0&&in->downsample_x.den>0?double(in->downsample_x.num)/in->downsample_x.den:1.;p.grid.downsampleY=in->downsample_y.num>0&&in->downsample_y.den>0?double(in->downsample_y.num)/in->downsample_y.den:1.;
      constexpr int matrices[]={0,2,4,8};p.ditherMatrix=matrices[int(value("pa.matrix"))-1];p.ditherStrength=value("pa.strength")/100;p.edgeProtection=value("pa.edge")/100;p.binaryAlpha=value("pa.alpha")==2;p.alphaThreshold=value("pa.alpha_threshold");p.encoding=value("pa.encoding")==2?pixel_art::ColorEncoding::LinearSrgb:pixel_art::ColorEncoding::Srgb;
      if(!f->pc98){bool gba=p.style==pixel_art::Style::GBA;constexpr pixel_art::Sampling methods[]={pixel_art::Sampling::Nearest,pixel_art::Sampling::Area,pixel_art::Sampling::Feature};p.grid.sampling=methods[int(value(gba?"gba.sampling":"modern.sampling"))-1];p.featureStrength=value(gba?"gba.feature":"modern.feature")/100;p.outlineStrength=value(gba?"gba.outline":"modern.outline")/100;
       if(!gba){p.mergeThreshold=value("modern.merge")*.0012;p.maxClusterArea=int(value("modern.cluster_area"));p.cleanSpeckles=value("modern.speckles")!=0;p.cleanSpurs=value("modern.spurs")!=0;p.cleanStairs=value("modern.stairs")!=0;p.cleanupIterations=int(value("modern.iterations"));p.detailProtection=value("modern.protection")/100;p.debugChanges=value("modern.debug")==2;}}
      // PC-98 uses the accepted artistic palette workflow regardless of retired slots58-60.
      p.pc98ColorSpec=pixel_art::PC98ColorSpec::Art;
      f->paletteSource=int(value("pa.palette"));auto colorCount=std::size_t(value(f->pc98?"pa.colors":p.style==pixel_art::Style::GBA?"gba.colors":"modern.colors"));
      p.palette=f->paletteSource==2?paDecode(presetBytes):pixel_art::stylePalette(p.style);
      f->paletteLimit=int(colorCount);if(p.palette.size()>colorCount)p.palette.resize(colorCount);
      if(f->paletteSource==3){double reference=value("pa.reference")*in->time_scale;if(reference>LONG_MAX)return PF_Err_BAD_CALLBACK_PARAM;f->referenceTime=A_long(std::llround(reference));}
    }
#endif
    PF_RenderRequest req=e->input->output_request;req.rect={0,0,0,0};PF_CheckoutResult meta{};
    auto err=e->cb->checkout_layer(in->effect_ref,0,1,&req,in->current_time,in->time_step,in->time_scale,&meta);if(err)return err;
    f->bounds=meta.max_result_rect;f->bounds.left=std::min<A_long>(0,f->bounds.left);f->bounds.top=std::min<A_long>(0,f->bounds.top);
    auto sampled=[](A_long v,PF_RationalScale s){return s.num>0&&s.den>0?A_long(std::ceil(double(v)*s.num/s.den)):v;};
    f->bounds.right=std::max(f->bounds.right,sampled(in->width,in->downsample_x));f->bounds.bottom=std::max(f->bounds.bottom,sampled(in->height,in->downsample_y));
    req.rect=f->bounds;req.channel_mask=PF_ChannelMask_ARGB;PF_CheckoutResult full{};
    err=e->cb->checkout_layer(in->effect_ref,0,0,&req,in->current_time,in->time_step,in->time_scale,&full);if(err)return err;
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
    if(f->enabled&&f->pixelEnabled&&f->paletteSource==3){PF_CheckoutResult probe{},reference{};auto referenceRequest=req;referenceRequest.rect={0,0,0,0};err=e->cb->checkout_layer(in->effect_ref,0,3,&referenceRequest,f->referenceTime,in->time_step,in->time_scale,&probe);if(err)return err;
      referenceRequest.rect=probe.max_result_rect;referenceRequest.rect.left=std::min(referenceRequest.rect.left,f->bounds.left);referenceRequest.rect.top=std::min(referenceRequest.rect.top,f->bounds.top);referenceRequest.rect.right=std::max(referenceRequest.rect.right,f->bounds.right);referenceRequest.rect.bottom=std::max(referenceRequest.rect.bottom,f->bounds.bottom);
      err=e->cb->checkout_layer(in->effect_ref,0,2,&referenceRequest,f->referenceTime,in->time_step,in->time_scale,&reference);if(err)return err;
    }
#endif
    e->output->max_result_rect=f->bounds;auto r=e->input->output_request.rect;
    r.left=std::max(r.left,f->bounds.left);r.top=std::max(r.top,f->bounds.top);r.right=std::max(r.left,std::min(r.right,f->bounds.right));r.bottom=std::max(r.top,std::min(r.bottom,f->bounds.bottom));
    e->output->result_rect=r;e->output->solid=false;e->output->pre_render_data=f.release();e->output->delete_pre_render_data_func=dispose;return 0;
}
struct Progress:dither::IProgress {
    PF_InData* in;PF_Err status=0;explicit Progress(PF_InData* p):in(p){}
    bool cancelled()override{if(!status){status=PF_ABORT(in);if(status)diagnostic(in,"abort-callback",status);}return status!=0;}
    // AE owns SmartFX/MFR progress; avoid non-monotonic PF_PROGRESS calls from sub-stages.
    void report(double)override{}
};
float encode(float x){return x<=.0031308f?12.92f*x:1.055f*std::pow(x,1.f/2.4f)-.055f;}
float decode(float x){return x<=.04045f?x/12.92f:std::pow((x+.055f)/1.055f,2.4f);}
PF_Err smart(PF_InData* in,PF_OutData* out,PF_SmartRenderExtra* e){
    auto f=static_cast<const Frame*>(e->input->pre_render_data);if(!f)return PF_Err_INTERNAL_STRUCT_DAMAGED;
    bool deep=e->input->bitdepth==16,fp=e->input->bitdepth==32;
#if defined(FYLM_CRT_ONLY) || defined(FYLM_SHADERGLASS)
    if(fp)return error(out,f->language,"error.unsupported_format",PF_Err_BAD_CALLBACK_PARAM);
#endif
    if(!deep&&!fp&&e->input->bitdepth!=8)return error(out,f->language,"error.unsupported_format",PF_Err_BAD_CALLBACK_PARAM);
    PF_EffectWorld* src=nullptr;PF_EffectWorld* dst=nullptr;auto err=e->cb->checkout_layer_pixels(in->effect_ref,0,&src);if(err)return err;
    struct Checkin{PF_InData* in;PF_SmartRenderExtra* e;~Checkin(){e->cb->checkin_layer_pixels(in->effect_ref,0);}} guard{in,e};
    err=e->cb->checkout_output(in->effect_ref,&dst);if(err)return err;if(!dst||!dst->data)return 0;
    const std::size_t pixelBytes=fp?sizeof(PF_PixelFloat):deep?sizeof(PF_Pixel16):sizeof(PF_Pixel8);Progress progress(in);
    if(!f->enabled){for(int y=0;y<dst->height;++y){if(y%16==0&&progress.cancelled())return progress.status;
        auto row=reinterpret_cast<unsigned char*>(dst->data)+y*dst->rowbytes;
        auto sy=std::int64_t(dst->origin_y)+y-(src?src->origin_y:0);
        for(int x=0;x<dst->width;++x){auto sx=std::int64_t(dst->origin_x)+x-(src?src->origin_x:0);auto p=row+x*pixelBytes;
            if(src&&src->data&&sx>=0&&sx<src->width&&sy>=0&&sy<src->height)std::memmove(p,reinterpret_cast<unsigned char*>(src->data)+sy*src->rowbytes+sx*pixelBytes,pixelBytes);else std::memset(p,0,pixelBytes);}}
        return 0;}
    std::int64_t w64=std::int64_t(f->bounds.right)-f->bounds.left,h64=std::int64_t(f->bounds.bottom)-f->bounds.top;
    if(w64<=0||h64<=0)return 0;
    if(w64>INT_MAX||h64>INT_MAX||std::uint64_t(w64)>SIZE_MAX/std::uint64_t(h64)/4)return error(out,f->language,"error.overflow",PF_Err_OUT_OF_MEMORY);
    int w=int(w64),h=int(h64);std::size_t channels=std::size_t(w)*h*4;
    // Bound CPU staging before allocating; the GL renderer has its own texture budget.
    std::size_t stagingBytesPerChannel=8;
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
    if(f->pixelEnabled)stagingBytesPerChannel=sizeof(float);
#endif
    if(channels>512u*1024u*1024u/stagingBytesPerChannel)return error(out,f->language,"error.memory",PF_Err_OUT_OF_MEMORY);
    auto sourcePixel=[&](int x,int y,float* p){
        auto sx=std::int64_t(x)+f->bounds.left-(src?src->origin_x:0),sy=std::int64_t(y)+f->bounds.top-(src?src->origin_y:0);
        if(!src||!src->data||sx<0||sy<0||sx>=src->width||sy>=src->height){std::fill_n(p,4,0.f);return;}
        auto row=reinterpret_cast<const unsigned char*>(src->data)+sy*src->rowbytes;
        if(fp){auto q=reinterpret_cast<const PF_PixelFloat*>(row)[sx];p[0]=q.red;p[1]=q.green;p[2]=q.blue;p[3]=q.alpha;}
        else if(deep){auto q=reinterpret_cast<const PF_Pixel16*>(row)[sx];p[0]=float(q.red)/PF_MAX_CHAN16;p[1]=float(q.green)/PF_MAX_CHAN16;p[2]=float(q.blue)/PF_MAX_CHAN16;p[3]=float(q.alpha)/PF_MAX_CHAN16;}
        else{auto q=reinterpret_cast<const PF_Pixel8*>(row)[sx];p[0]=q.red/255.f;p[1]=q.green/255.f;p[2]=q.blue/255.f;p[3]=q.alpha/255.f;}
    };
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
    if(f->pixelEnabled){
      struct PACancel:pixel_art::IProgress{Progress& progress;explicit PACancel(Progress& p):progress(p){}bool cancelled()override{return progress.cancelled();}void report(double)override{}} cancel{progress};
      auto params=f->pixel;if(params.pc98)params.workers=0;std::vector<float> input(channels);
      auto straight=[](float* p){float alpha=std::isfinite(p[3])?std::clamp(p[3],0.f,1.f):0;for(int k=0;k<3;k++)p[k]=alpha>1e-8f&&std::isfinite(p[k])?p[k]/alpha:0;p[3]=alpha;};
      for(int y=0;y<h;y++){if(y%16==0&&progress.cancelled())return progress.status;for(int x=0;x<w;x++){auto p=input.data()+(std::size_t(y)*w+x)*4;sourcePixel(x,y,p);straight(p);}}
      if(f->paletteSource==3){PF_EffectWorld* reference=nullptr;err=e->cb->checkout_layer_pixels(in->effect_ref,2,&reference);if(err)return err;
        struct ReturnReference{PF_InData* in;PF_SmartRenderExtra* e;~ReturnReference(){e->cb->checkin_layer_pixels(in->effect_ref,2);}} referenceGuard{in,e};
        if(!reference||!reference->data||reference->width<1||reference->height<1)return PF_Err_BAD_CALLBACK_PARAM;
        std::uint64_t count=std::uint64_t(reference->width)*reference->height*4;if(count>256ull*1024*1024/sizeof(float))return PF_Err_OUT_OF_MEMORY;std::vector<float> pixels(static_cast<std::size_t>(count));
        for(int y=0;y<reference->height;y++){if(y%16==0&&progress.cancelled())return progress.status;auto row=reinterpret_cast<const unsigned char*>(reference->data)+std::ptrdiff_t(y)*reference->rowbytes;for(int x=0;x<reference->width;x++){auto p=pixels.data()+(std::size_t(y)*reference->width+x)*4;
          if(fp){auto q=reinterpret_cast<const PF_PixelFloat*>(row)[x];p[0]=q.red;p[1]=q.green;p[2]=q.blue;p[3]=q.alpha;}else if(deep){auto q=reinterpret_cast<const PF_Pixel16*>(row)[x];p[0]=float(q.red)/PF_MAX_CHAN16;p[1]=float(q.green)/PF_MAX_CHAN16;p[2]=float(q.blue)/PF_MAX_CHAN16;p[3]=float(q.alpha)/PF_MAX_CHAN16;}else{auto q=reinterpret_cast<const PF_Pixel8*>(row)[x];p[0]=q.red/255.f;p[1]=q.green/255.f;p[2]=q.blue/255.f;p[3]=q.alpha/255.f;}straight(p);}}
        try{auto extracted=pixel_art::extractPalette({pixels.data(),reference->width,reference->height,std::size_t(reference->width)*16,pixel_art::PixelFormat::RGBA32F},params.encoding,f->paletteLimit,&cancel);if(!extracted.empty())params.palette=std::move(extracted);}catch(...){if(progress.status)return progress.status;throw;}
      }
      double ox=f->bounds.left/params.grid.downsampleX,oy=f->bounds.top/params.grid.downsampleY;
      auto outputFormat=fp?pixel_art::PixelFormat::ARGB32FPremultiplied:deep?pixel_art::PixelFormat::ARGB16_32768Premultiplied:pixel_art::PixelFormat::ARGB8Premultiplied;
      auto result=pixel_art::render({input.data(),w,h,std::size_t(w)*16,pixel_art::PixelFormat::RGBA32F,ox,oy},{dst->data,dst->width,dst->height,std::size_t(dst->rowbytes),outputFormat,dst->origin_x/params.grid.downsampleX,dst->origin_y/params.grid.downsampleY},params,&cancel);
      if(progress.status)return progress.status;if(!result.ok)return error(out,f->language,pixel_art::errorKey(result.code),result.code==pixel_art::ErrorCode::OutOfMemory?PF_Err_OUT_OF_MEMORY:PF_Err_BAD_CALLBACK_PARAM);
      return 0;
    }
#endif
#if defined(FYLM_SHADERGLASS) || defined(FYLM_CRT_ONLY)
    // Serialize before allocating host conversion frames, not only after they
    // have been allocated by every AE worker. The inner lease is recursive on
    // the same Windows thread and remains shared with the CRT staging path.
    retro::WorkspaceLease hostStaging;if(!hostStaging.acquire([&]{return progress.cancelled();}))return progress.status?progress.status:PF_Interrupt_CANCEL;
    MEMORYSTATUSEX hostMemory{};hostMemory.dwLength=sizeof(hostMemory);if(GlobalMemoryStatusEx(&hostMemory)&&std::uint64_t(channels)*8>hostMemory.ullAvailPhys/3)return error(out,f->language,"sg.error.budget",PF_Err_OUT_OF_MEMORY);
#endif
    std::vector<float> gpu;std::vector<unsigned char> bytes,processed;
#if defined(FYLM_CRT_ONLY) || defined(FYLM_SHADERGLASS)
    gpu.resize(channels);
#else
    bytes.resize(channels);processed.resize(channels);
#endif
    for(int y=0;y<h;++y){if(y%16==0&&progress.cancelled())return progress.status;
        for(int x=0;x<w;++x){float p[4];sourcePixel(x,y,p);auto i=(std::size_t(y)*w+x)*4;
#if defined(FYLM_CRT_ONLY) || defined(FYLM_SHADERGLASS)
            if(p[3]>0)for(int c=0;c<3;++c)p[c]/=p[3];else std::fill_n(p,3,0.f);
            std::copy_n(p,4,gpu.data()+i);
#else
            for(int c=0;c<3;++c){float v=p[3]>0?p[c]/p[3]:0;if(f->linear)v=encode(std::max(0.f,v));bytes[i+c]=dither::toU8Clamped(v*255.);}
            bytes[i+3]=dither::toU8Clamped(p[3]*255.);
#endif
        }
    }
#if defined(FYLM_CRT_ONLY) || defined(FYLM_SHADERGLASS)
    retro::RenderRequest r;r.controls=f->retro;r.input=std::move(gpu);r.size={w,h};r.timeValue=f->time;r.timeScale=f->scale;r.memoryBudget=gpuBudget();r.cancelled=[&]{return progress.cancelled();};
#ifdef FYLM_SHADERGLASS
    auto result=fylm::sg::render(*f->preset,r,f->settings);
#else
    auto result=fylm::crt::render(r,f->tape);
#endif
    if(!result.ok){if(progress.status)return progress.status;
        auto code=result.error=="retro.error.memory"||result.error=="sg.error.memory"||result.error=="sg.error.budget"?PF_Err_OUT_OF_MEMORY:PF_Err_INTERNAL_STRUCT_DAMAGED;
        return error(out,f->language,result.error.c_str(),code);}gpu=std::move(result.pixels);
#else
    auto result=dither::render({bytes.data(),w,h,std::size_t(w)*4,dither::PixelFormat::RGBA8},{processed.data(),w,h,std::size_t(w)*4,dither::PixelFormat::RGBA8},f->dither,&progress);
    if(progress.status)return progress.status;if(!result.ok)return error(out,f->language,dither::errorKey(result.code),PF_Err_INTERNAL_STRUCT_DAMAGED);
#endif
    for(int y=0;y<dst->height;++y){if(y%16==0&&progress.cancelled())return progress.status;
        auto row=reinterpret_cast<unsigned char*>(dst->data)+y*dst->rowbytes;
        for(int x=0;x<dst->width;++x){auto xx=std::int64_t(dst->origin_x)+x-f->bounds.left,yy=std::int64_t(dst->origin_y)+y-f->bounds.top;float p[4]{};
            if(xx>=0&&xx<w&&yy>=0&&yy<h){auto i=(std::size_t(yy)*w+xx)*4;
#if defined(FYLM_CRT_ONLY) || defined(FYLM_SHADERGLASS)
                std::copy_n(gpu.data()+i,4,p);
#ifdef FYLM_SHADERGLASS
                float original[4];sourcePixel(int(xx),int(yy),original);p[3]=original[3];
#endif
                for(int c=0;c<3;++c)p[c]*=p[3];
#else
                float original[4];sourcePixel(int(xx),int(yy),original);
                p[3]=f->dither.alphaMode==dither::AlphaMode::Preserve?original[3]:1.f;
                for(int c=0;c<3;++c){float v=processed[i+c]/255.f;if(f->linear)v=decode(v);p[c]=v*p[3];}
#endif
            }
            auto unit=[](float v){return std::isfinite(v)?std::clamp(v,0.f,1.f):0.f;};
            if(fp){auto& q=reinterpret_cast<PF_PixelFloat*>(row)[x];q.red=unit(p[0]);q.green=unit(p[1]);q.blue=unit(p[2]);q.alpha=unit(p[3]);}
            else if(deep){auto& q=reinterpret_cast<PF_Pixel16*>(row)[x];q.red=A_u_short(std::round(unit(p[0])*PF_MAX_CHAN16));q.green=A_u_short(std::round(unit(p[1])*PF_MAX_CHAN16));q.blue=A_u_short(std::round(unit(p[2])*PF_MAX_CHAN16));q.alpha=A_u_short(std::round(unit(p[3])*PF_MAX_CHAN16));}
            else{auto& q=reinterpret_cast<PF_Pixel8*>(row)[x];q.red=dither::toU8Clamped(unit(p[0])*255.);q.green=dither::toU8Clamped(unit(p[1])*255.);q.blue=dither::toU8Clamped(unit(p[2])*255.);q.alpha=dither::toU8Clamped(unit(p[3])*255.);}
        }
    }return 0;
}
void diagnostic(PF_InData* in,const char* stage,PF_Err status,const char* detail) noexcept {
    if(!status&&!detail)return;
    // FYLM_RENDER_LOG wins; otherwise fall back to the per-user temp directory so a
    // missing or unwritable variable never turns diagnostics into a silent no-op.
    wchar_t path[1024]{};DWORD n=GetEnvironmentVariableW(L"FYLM_RENDER_LOG",path,1024);
    bool fromEnvironment=n>0&&n<1024;
    if(!fromEnvironment){wchar_t temp[MAX_PATH]{};DWORD m=GetTempPathW(MAX_PATH,temp);if(m>0&&m<MAX_PATH)std::swprintf(path,1024,L"%sFYLM-%hs.log",temp,fylm::Match);}
    if(!path[0])return;
    auto open=[&](const wchar_t* file){return CreateFileW(file,FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);};
    HANDLE handle=open(path);DWORD openError=handle==INVALID_HANDLE_VALUE?GetLastError():0;
    char note[512]{};
    if(handle==INVALID_HANDLE_VALUE&&fromEnvironment){
        // Report the failed explicit path once, then continue on the temp fallback.
        std::snprintf(note,sizeof(note),"stage=log-open,error=%lu,path=%ls\n",static_cast<unsigned long>(openError),path);
        wchar_t temp[MAX_PATH]{};DWORD m=GetTempPathW(MAX_PATH,temp);
        if(m>0&&m<MAX_PATH){std::swprintf(path,1024,L"%sFYLM-%hs.log",temp,fylm::Match);handle=open(path);openError=handle==INVALID_HANDLE_VALUE?GetLastError():0;}
    }
    if(handle==INVALID_HANDLE_VALUE)return;
    auto append=[&](const char* text,std::size_t length){DWORD written=0;if(length&&length<1024)WriteFile(handle,text,DWORD(length),&written,nullptr);};
    if(note[0]){append(note,std::strlen(note));std::snprintf(note,sizeof(note),"stage=log-fallback,path=%ls\n",path);append(note,std::strlen(note));}
    char line[512];
    int length=detail?std::snprintf(line,sizeof(line),"%s,version=%s,pid=%lu,stage=%s,time=%ld/%lu,error=%ld,detail=%s\n",fylm::Match,FYLM_VERSION_TEXT,static_cast<unsigned long>(GetCurrentProcessId()),stage,long(in->current_time),static_cast<unsigned long>(in->time_scale),long(status),detail)
                     :std::snprintf(line,sizeof(line),"%s,version=%s,pid=%lu,stage=%s,time=%ld/%lu,error=%ld\n",fylm::Match,FYLM_VERSION_TEXT,static_cast<unsigned long>(GetCurrentProcessId()),stage,long(in->current_time),static_cast<unsigned long>(in->time_scale),long(status));
    if(length>0&&length<int(sizeof(line)))append(line,std::size_t(length));
    CloseHandle(handle);
}
}
extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr p,PF_PluginDataCB2 cb,SPBasicSuite*,const char*,const char*){
    PF_Err result=0;return PF_REGISTER_EFFECT_EXT2(p,cb,fylm::Name,fylm::Match,fylm::Category,AE_RESERVED_INFO,"EffectMain","https://github.com/Oslonline/dithering-studio");
}
extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd,PF_InData* in,PF_OutData* out,PF_ParamDef* params[],PF_LayerDef*,void* extra){
    PF_Err status=0;const char* stage="dispatch";
    try{switch(cmd){
    case PF_Cmd_ABOUT:std::snprintf(out->return_msg,sizeof(out->return_msg),"%s %s\rFYLM Plugins",fylm::Name,FYLM_VERSION_TEXT);break;
    case PF_Cmd_GLOBAL_SETUP:
        out->my_version=PF_VERSION(FYLM_VERSION_MAJOR,FYLM_VERSION_MINOR,FYLM_VERSION_PATCH,PF_Stage_DEVELOP,FYLM_VERSION_BUILD);out->out_flags=PF_OutFlag_DEEP_COLOR_AWARE|PF_OutFlag_SEND_UPDATE_PARAMS_UI;
#if defined(FYLM_CRT_ONLY) || defined(FYLM_SHADERGLASS)
        out->out_flags|=PF_OutFlag_NON_PARAM_VARY;
#endif
        out->out_flags2=PF_OutFlag2_SUPPORTS_SMART_RENDER|PF_OutFlag2_REVEALS_ZERO_ALPHA;
#if !defined(FYLM_CRT_ONLY) && !defined(FYLM_SHADERGLASS)
        out->out_flags2|=PF_OutFlag2_FLOAT_COLOR_AWARE;
#endif
#ifdef DITHER_ENABLE_MFR
        out->out_flags2|=PF_OutFlag2_SUPPORTS_THREADED_RENDERING;
#endif
        break;
    case PF_Cmd_PARAMS_SETUP:stage="params";status=setup(in,out);break;
    case PF_Cmd_UPDATE_PARAMS_UI:case PF_Cmd_USER_CHANGED_PARAM:stage="ui";
#ifdef FYLM_SHADERGLASS
        if(cmd==PF_Cmd_USER_CHANGED_PARAM){
            int index=static_cast<PF_UserChangedParamExtra*>(extra)->param_index;
            if(index==fylm::index("sg.extended_slots")&&params[index]->u.bd.value&&!params[fylm::index("sg.use_slots")]->u.bd.value){
                // One explicit user action enables the whole dependency chain;
                // commit the master in the same AE undo transaction. Never do
                // this during UPDATE_PARAMS_UI or on project load.
                auto& master=*params[fylm::index("sg.use_slots")];
                master.u.bd.value=1;master.uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;
                out->out_flags|=PF_OutFlag_FORCE_RERENDER|PF_OutFlag_REFRESH_UI;
            }
            if(index==fylm::index("sg.virtual_height")&&params[2]->u.pd.value>=4&&params[fylm::index("sg.follow_preset")]->u.bd.value){
                params[fylm::index("sg.follow_preset")]->u.bd.value=0;
                params[fylm::index("sg.follow_preset")]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;
                out->out_flags|=PF_OutFlag_FORCE_RERENDER|PF_OutFlag_REFRESH_UI;
            }
            if(index==fylm::index("sg.use_slots")||index==fylm::index("sg.extended_slots")||index==fylm::index("sg.follow_preset")||index==fylm::index("sg.animate"))out->out_flags|=PF_OutFlag_REFRESH_UI;
            if(index==fylm::index("sg.load")){status=sgBrowse(in,out,params);if(status)break;}
            else if(index==fylm::index("sg.defaults")){status=sgDefaults(in,out,params);if(status)break;}
            else if(index==fylm::index("sg.import_source")){status=sgBeginImport(in);out->out_flags|=PF_OutFlag_REFRESH_UI;if(status)break;}
            else if(index==fylm::index("sg.import_cancel")){sgImportTask.reset();sgImportStatus="sg.import_cancelled";out->out_flags|=PF_OutFlag_REFRESH_UI;}
            else if(index==fylm::index("sg.import_status")){A_long sleep=0;sgImportIdle(nullptr,nullptr,&sleep);out->out_flags|=PF_OutFlag_REFRESH_UI;auto message=sgWide(loc::text(locale(in),sgImportStatus));MessageBoxW(GetActiveWindow(),message.c_str(),L"FYLM ShaderGlass",MB_OK|(sgImportStatus.starts_with("sg.error.")?MB_ICONERROR:MB_ICONINFORMATION));}
            else if(index==fylm::index("sg.advanced")){status=sgEditParameters(in,out,params,locale(in));if(status)break;}
            else if(index==fylm::index("sg.export")){status=sgExportPreset(in,params);if(status)break;}
        }
#elif defined(FYLM_CRT_ONLY)
        if(cmd==PF_Cmd_USER_CHANGED_PARAM&&static_cast<PF_UserChangedParamExtra*>(extra)->param_index==fylm::index("retro.apply_style")){status=retroApplyStyle(in,out,params);if(status)break;}
#elif !defined(FYLM_CRT_ONLY)
        if(cmd==PF_Cmd_USER_CHANGED_PARAM){int index=static_cast<PF_UserChangedParamExtra*>(extra)->param_index;if(index==51||(paLibraryMode(index)&&index%2==1)){status=paApplyLibrary(in,out,params,index);if(status)break;}else if(index==fylm::index("pa.import")||index==fylm::index("pa.export")||index==fylm::index("pa.preset")){status=paAction(in,out,params,index,locale(in));if(status)break;}}
#endif
        status=update(in,params);break;
#ifdef FYLM_SHADERGLASS
    case PF_Cmd_ARBITRARY_CALLBACK:stage="preset-data";status=sgArbitrary(in,static_cast<PF_ArbParamsExtra*>(extra));break;
#elif !defined(FYLM_CRT_ONLY)
    case PF_Cmd_ARBITRARY_CALLBACK:stage="palette-data";status=paArbitrary(in,static_cast<PF_ArbParamsExtra*>(extra));break;
#endif
    case PF_Cmd_SMART_PRE_RENDER:stage="pre";status=pre(in,static_cast<PF_PreRenderExtra*>(extra));break;
    case PF_Cmd_SMART_RENDER:stage="render";out->return_msg[0]=0;status=smart(in,out,static_cast<PF_SmartRenderExtra*>(extra));break;
    default:break;
    }}catch(const std::bad_alloc&){status=error(out,locale(in),"error.memory",PF_Err_OUT_OF_MEMORY);}catch(const std::exception& ex){status=error(out,locale(in),(std::strncmp(ex.what(),"pa.error.",9)==0||std::strncmp(ex.what(),"sg.error.",9)==0||std::strncmp(ex.what(),"retro.error.",12)==0)?ex.what():"error.internal",PF_Err_BAD_CALLBACK_PARAM);}catch(...){status=error(out,locale(in),"error.internal",PF_Err_INTERNAL_STRUCT_DAMAGED);}
    diagnostic(in,stage,status);return status;
}
