const fs=require('fs'),p=require('path'),crypto=require('crypto');
const root=p.resolve(__dirname,'..'),dir=p.join(root,'assets/ShaderGlass/Library');
const audit=JSON.parse(fs.readFileSync(p.join(root,'docs/shaderglass/audit/candidates.json'))),a=audit.candidates.find(e=>e.slug==='crt-royale');
const results=JSON.parse(fs.readFileSync(p.join(root,'build/sg-candidates/compile-results.json'))),r=results.results.find(e=>e.slug==='crt-royale');if(r.compile!=='passed')throw Error('Royale compile missing');
const source=p.join(root,'third_party/slang-source/slang-shaders-'+audit.commit);
for(const f of [...a.files,{path:'crt/shaders/crt-royale/LICENSE.TXT'},{path:'crt/shaders/crt-royale/README.TXT'}]){
 const src=p.join(source,f.path);if(!fs.existsSync(src)){if(f.sha256)throw Error('Missing source '+f.path);continue;}
 const dest=p.join(dir,'Sources',f.path),bytes=fs.readFileSync(src);if(f.sha256&&crypto.createHash('sha256').update(bytes).digest('hex')!==f.sha256)throw Error('Source changed');fs.mkdirSync(p.dirname(dest),{recursive:true});fs.writeFileSync(dest,bytes);
}
const bytes=fs.readFileSync(p.join(root,'build/sg-candidates/crt-royale.fylmsg'));fs.writeFileSync(p.join(dir,'crt-royale.fylmsg'),bytes);
const catalogPath=p.join(dir,'catalog-v1.json'),c=JSON.parse(fs.readFileSync(catalogPath));
if(c.entries.length===38)c.entries.push({id:'FYLM.SG.crt-royale',slug:'crt-royale',base:'crt-royale',category:'CRT',name:'CRT Royale',zh:'复杂彩管（Royale）',zh_hant:'複雜彩管（Royale）',ja:'CRT Royale',menu:39,virtual_height:480,passes:12,performance:'heavy multi-pass',use:'精细荧光遮罩与多段光晕；开销较高，建议先使用480行输入网格。',use_hant:'精細螢光遮罩與多段光暈；開銷較高，建議先使用480行輸入網格。',slots:r.reflection.controls.slice(0,16).map(e=>e.name),overrides:{},mix:1,source:{repository:audit.repository,commit:audit.commit,path:a.path,dependencies:a.files,license:'GPL-2.0-or-later; preserved Royale source notices and directory LICENSE.TXT'},sha256:crypto.createHash('sha256').update(bytes).digest('hex'),status:'GPU test candidate; actual AE and visual reference pending'});
else if(c.entries.length!==39||c.entries[38].slug!=='crt-royale')throw Error('Unexpected frozen menu order');
c.entries[38].sha256=crypto.createHash('sha256').update(bytes).digest('hex');
c.revision=6;fs.writeFileSync(catalogPath,JSON.stringify(c,null,2)+'\n');
const names=`crt_gamma|显像管明暗响应
lcd_gamma|输出屏幕明暗响应
levels_contrast|对比度
halation_weight|亮部溢光
diffusion_weight|荧光扩散
bloom_underestimate_levels|光晕阈值补偿
bloom_excess|额外光晕
beam_min_sigma|亮扫描线最小宽度
beam_max_sigma|亮扫描线最大宽度
beam_spot_power|扫描光点集中度
beam_min_shape|暗部扫描线形状
beam_max_shape|亮部扫描线形状
beam_shape_power|扫描线形状过渡
beam_horiz_filter|水平重建方式
beam_horiz_sigma|水平柔化宽度
beam_horiz_linear_rgb_weight|水平混合的线性光权重
convergence_offset_x_r|红色水平会聚偏移
convergence_offset_x_g|绿色水平会聚偏移
convergence_offset_x_b|蓝色水平会聚偏移
convergence_offset_y_r|红色垂直会聚偏移
convergence_offset_y_g|绿色垂直会聚偏移
convergence_offset_y_b|蓝色垂直会聚偏移
mask_type|荧光遮罩类型
mask_sample_mode_desired|遮罩采样方式
mask_specify_num_triads|按荧光单元数量定义
mask_triad_size_desired|荧光单元尺寸
mask_num_triads_desired|荧光单元数量
aa_subpixel_r_offset_x_runtime|红色子像素水平位置
aa_subpixel_r_offset_y_runtime|红色子像素垂直位置
aa_cubic_c|抗锯齿重建锐度
aa_gauss_sigma|抗锯齿柔化宽度
geom_mode_runtime|屏幕几何方式
geom_radius|屏幕曲率半径
geom_view_dist|观看距离
geom_tilt_angle_x|屏幕水平倾角
geom_tilt_angle_y|屏幕垂直倾角
geom_aspect_ratio_x|显示比例水平分量
geom_aspect_ratio_y|显示比例垂直分量
geom_overscan_x|水平过扫描
geom_overscan_y|垂直过扫描
border_size|边框宽度
border_darkness|边框暗度
border_compress|边框压缩
interlace_detect_toggle|隔行扫描检测
interlace_bff|下场优先
interlace_1080i|检测1080i输入`;
for(const lang of ['en','zh-Hans','zh-Hant','ja']){
 const file=p.join(root,'i18n',lang+'.json'),j=JSON.parse(fs.readFileSync(file));
 const menu=j['sg.preset.items'].split('|');if(menu.length===38)menu.push(lang==='zh-Hans'?'显像管屏幕 / 复杂彩管（Royale）':lang==='zh-Hant'?'顯像管螢幕 / 複雜彩管（Royale）':'CRT / CRT Royale');if(menu.length!==39)throw Error('Localized menu count');j['sg.preset.items']=menu.join('|');
 for(const line of names.split('\n')){const [key,label]=line.split('|');j['sg.uniform.'+key]=lang.startsWith('zh')?label:r.reflection.controls.find(e=>e.name===key).description;}
 fs.writeFileSync(file,JSON.stringify(j,null,2)+'\n');
}
console.log('Appended Royale as menu39; previous38 selections and bytecode unchanged');
