// Chinese user-facing labels; stable IDs and uniform bindings remain unchanged.
const fs=require('fs'),p=require('path'),cp=require('child_process');const root=p.resolve(__dirname,'..');
const lines=`CRTgamma|输入明暗响应
monitorgamma|屏幕明暗响应
d|观看距离
CURVATURE|屏幕弯曲
invert_aspect|曲面方向反转
R|曲面半径
cornersize|屏幕圆角大小
cornersmooth|屏幕边角柔化
SHARPNESS_H|水平清晰度
SHARPNESS_V|垂直清晰度
MASK_STRENGTH|彩色栅格强度
MASK_DOT_WIDTH|栅格亮点宽度
MASK_DOT_HEIGHT|栅格亮点高度
MASK_STAGGER|栅格错行程度
MASK_SIZE|栅格大小
SCANLINE_STRENGTH|扫描线强度
BRIGHT_BOOST|亮度补偿
GAMMA_INPUT|输入明暗响应
GAMMA_OUTPUT|输出明暗响应
DILATION|亮点扩张
hardScan|扫描线锐度
hardPix|像素边缘锐度
maskDark|暗栅格亮度
maskLight|亮栅格亮度
bloomAmount|荧光扩散强度
brightBoost|亮度补偿
warpX|水平曲面弯曲
warpY|垂直曲面弯曲
GLOW_HALATION|亮边光晕
GLOW_DIFFUSION|光晕扩散
BRIGHTNESS|画面亮度
SHARPNESS_IMAGE|画面清晰度
SHARPNESS_EDGES|轮廓清晰度
SCANLINE_SIZE_MIN|暗部扫描线宽度
SCANLINE_SIZE_MAX|亮部扫描线宽度
DOTMASK|彩色栅格强度
scanline_weight|扫描线宽度
SHARPER|画面清晰度
lum|暗部亮度
MASK_INTENSITY|彩色栅格强度
SCANLINE_THINNESS|扫描线强度
SCAN_BLUR|扫描边缘清晰度
CORNER|屏幕圆角
CRT_GAMMA|显像管明暗响应
MASK|屏幕栅格
TRINITRON_CURVE|单向曲面效果
SCANLINE_WEIGHT|扫描线宽度
SCANLINE_GAP_BRIGHTNESS|线间暗带亮度
MASK_BRIGHTNESS|栅格亮度
BLOOM_FACTOR|荧光扩散
CURVATURE_X|水平曲面弯曲
CURVATURE_Y|垂直曲面弯曲
INPUT_GAMMA|输入明暗响应
OUTPUT_GAMMA|输出明暗响应
BLURSCALEX|水平柔化
LOWLUMSCAN|暗部扫描线
HILUMSCAN|亮部扫描线
BRIGHTBOOST|暗像素提亮
MASK_DARK|栅格暗部强度
MASK_FADE|栅格淡化
SPOT_WIDTH|荧光点宽度
SPOT_HEIGHT|荧光点高度
COLOR_BOOST|色彩增强
InputGamma|输入明暗响应
OutputGamma|输出明暗响应
CURV|曲面开关
scanlines|扫描线强度
INTERL|隔行模拟开关
SAT|色彩饱和度
gain|整体亮度增益
gamma|明暗过渡
blacklevel|黑色底亮度
ambient|环境反光
BGR|红蓝排列反转
RSUBPIX_R|红色亮点亮度
GSUBPIX_G|绿色亮点亮度
BSUBPIX_B|蓝色亮点亮度
shine|亮点反光
blend|点阵混合强度
soft|点阵柔化
BORDERMULT|像素边框强度
GBAGAMMA|掌机明暗响应
reflectionBrightness|反射亮度
reflectionDistanceX|反射水平偏移
reflectionDistanceY|反射垂直偏移
lightBrightness|前光亮度
SHARP_BILINEAR_PRE_SCALE|手动重建倍率
AUTO_PRESCALE|自动倍率开关
ia_saturation|色彩饱和度
ia_contrast|画面对比度
ia_luminance|整体亮度
ia_black_level|黑色底亮度
ia_bright_boost|亮部增益
ia_R|红色增益
ia_G|绿色增益
ia_B|蓝色增益
g_lum|整体亮度
g_cntrst|画面对比度
g_mid|中间调亮度
g_sat|色彩饱和度
g_vibr|自然鲜艳度
wp_temperature|白色色温
g_hue_degrees|整体色相
g_vstr|边缘暗角强度
frameShape|屏幕边框形状
frameLimit|边框遮挡范围
frameSharpness|边框边缘锐度
TVNoise|信号调制与噪声
PALSignal|PAL信号开关
phosphors|荧光点开关
border|屏幕边框开关
ntsc_sat|信号饱和度
ntsc_bright|信号亮度
ntsc_gamma|信号明暗响应
cust_artifacting|交错彩纹强度
cust_fringing|边缘串色强度
ntsc_rainbow1|彩虹纹强度
ntsc_sharp|信号边缘锐度
quality|信号重建质量
SFX_CLR|相近颜色判定
SFX_SAA|轮廓柔化开关
SFX_SCN|拐角修整开关
blend_none|平坦区域混合
blend_normal|普通边缘混合
blend_dominant|主要边缘混合`;
const map=Object.fromEntries(lines.split('\n').map(line=>line.split('|'))),catalog=JSON.parse(fs.readFileSync(p.join(root,'assets/ShaderGlass/Library/catalog-v1.json'))),tool=p.join(root,'build/modules/shaderglass/Release/FYLM-sgtool.exe');
const descriptions={};for(const base of [...new Set(catalog.entries.map(e=>e.base).filter(Boolean))]){const j=JSON.parse(cp.execFileSync(tool,['--inspect',p.join(root,'assets/ShaderGlass/Library',base+'.fylmsg')],{encoding:'utf8',windowsHide:true}));for(const u of j.controls)descriptions[u.name]=u.description;}
const categories={Art:'艺术显示',CRT:'显像管屏幕',LCD:'液晶与掌机',Signal:'模拟信号',Scaling:'像素重建',Colour:'画面调色'};
for(const lang of ['en','zh-Hans','zh-Hant','ja']){const f=p.join(root,'i18n',lang+'.json'),j=JSON.parse(fs.readFileSync(f,'utf8'));
 for(const e of catalog.entries)for(const name of e.display_slots||e.slots||[]){const key='sg.uniform.'+name;if(lang==='en'&&!j[key])j[key]=descriptions[name]||name;if(lang==='zh-Hans'){if(!map[name]&&!j[key])throw Error('Missing Chinese label '+name);if(map[name])j[key]=map[name];}}
 if(lang==='zh-Hans'){
  Object.assign(j,{'sg.preset':'显示风格','sg.browser':'内置显示风格','sg.virtual_height':'输入网格高度','sg.follow_preset':'使用推荐输入网格','sg.use_slots':'启用自定义参数','sg.extended_slots':'更多参数 I–P','sg.defaults':'恢复此风格默认值','sg.strength':'整体效果强度','sg.category.Scaling':'像素重建与放大','sg.library_hint':'先选风格；需要调参时先恢复默认值，再启用自定义参数。I–P需另开“更多参数”。','sg.unavailable':'此风格未提供','sg.tip.grid':'这是送入效果的画面高度。数值越低，像素块越明显；0表示保留原尺寸。直接修改会自动关闭推荐输入网格。','sg.tip.overrides':'勾选后由A–H滑块替代风格默认值。建议先点“恢复此风格默认值”；I–P还需开启“更多参数”。','sg.tip.extra':'开启 I–P 时会自动启用自定义参数；关闭 I–P 不影响 A–H。仅当前风格提供扩展参数时可用。脚本面板中编辑后请点击应用。','sg.tip.slot':'滑块表示此参数真实范围的0–100%。开关类参数建议0%关、100%开；恢复默认值后再逐项调整。','sg.tip.time':'控制使用时间的效果。关闭时间动画后，固定于时间偏移和帧相位指定的位置。静态风格不会因此自动变成动画。','sg.source_algorithm':'参考算法：','sg.no_parameters':'此风格没有独立参数，可调整输入网格或整体效果强度。','sg.native_notice':'原尺寸重建在放大比例为1时可能无变化。普通图片请先试SG09–SG12，并启用推荐输入网格。'});
  Object.assign(j,{'retro.signal_model':'磁带处理方式','retro.signal_model.items':'旧版兼容通道偏移|亮度与颜色分离（YIQ）','retro.tracking_height':'底部干扰范围','retro.tracking_distortion':'底部横向扭曲','retro.tracking_noise':'底部噪声强度','retro.streak_frequency':'色带出现频率','retro.streak_intensity':'色带干扰强度','retro.streak_height':'色带高度','retro.dropout_frequency':'掉磁细线频率','retro.dropout_intensity':'掉磁细线强度','retro.dropout_height':'掉磁细线高度','retro.luma_blur':'亮度细节柔化','retro.chroma_bleed':'颜色横向拖尾','retro.chroma':'颜色边缘偏移'});
 }
 if(lang==='en')Object.assign(j,{'sg.unavailable':'Unused by this style','sg.tip.grid':'Height of the image grid supplied to the effect.0 keeps original size. Editing switches to manual input grid.','sg.tip.overrides':'Restore style defaults before enabling custom parameter overrides. I-P also require the extended bank.','sg.tip.extra':'Requires custom overrides and a style with I-P bindings.','sg.tip.slot':'0-100% maps the underlying parameter range. Toggle parameters normally use0% off and100% on.','sg.tip.time':'Time controls affect time-aware shaders. Disabled animation uses the specified offset/phase.','sg.source_algorithm':'Source algorithm: ','sg.no_parameters':'This style has no separate parameters. Adjust input grid or overall strength.','sg.native_notice':'At1:1 scale a native-size reconstruction can look unchanged. TrySG09-SG12 with the recommended input grid.'});
 if(lang==='zh-Hans')j['sg.preset.items']=catalog.entries.map(e=>e.menu<=3?e.zh:categories[e.category]+' / '+e.zh).join('|');else if(lang==='en'||lang==='ja')j['sg.preset.items']=catalog.entries.map(e=>e.menu<=3?e.name:e.category+' / '+e.name).join('|');fs.writeFileSync(f,JSON.stringify(j,null,2)+'\n');}
console.log('Localized all '+Object.keys(map).length+' upstream slot labels;38 display-style choices');

const english=JSON.parse(fs.readFileSync(p.join(root,"i18n/en.json")));for(const language of ["zh-Hans","zh-Hant","ja"]){const file=p.join(root,"i18n",language+".json"),labels=JSON.parse(fs.readFileSync(file,"utf8").replace(/^\uFEFF/,""));for(const [key,value]of Object.entries(english))if(!(key in labels))labels[key]=value;fs.writeFileSync(file,JSON.stringify(labels,null,2)+"\n");}
