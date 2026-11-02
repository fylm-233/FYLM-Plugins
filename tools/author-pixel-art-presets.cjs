// Reproducible original FYLM colour ramps. Reference works guide the art
// direction; these are not sampled or represented as their official palettes.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'..');
const pc={
 peach:['26213F','47314D','70465D','A36777','C98D88','EAB7A1','FFE6C9','F5E2D9','243C54','486E8A','84B2CA','C4DADE','354C40','6D8859','ADAA71','D7C793'],
 lilac:['242744','443F62','6B5F83','9B8FB8','C5B9D0','EDE5E0','8B665F','C49485','F1CEB4','294951','558388','A5C6C2','5D744F','ADB78B','887AA2','D8CDDD'],
 indigo:['11172F','27304A','3E4765','697792','A5B8C6','DDE3DB','3D2A56','6D4C91','AB70BA','F29AAF','713E52','B67C83','F0B9A4','245B67','45A49F','E7CB82'],
 scarlet:['21162B','42304B','725171','AB7996','D6A6BB','F0D9D7','58283F','A83B59','D96B75','EFACA0','F2C987','8A6749','274950','518B84','88B7A4','B9D5C1'],
 olive:['183B2D','3F6750','91A86B','D8DEA0'],grey:['252B2E','626C68','AAB3A7','E8E8D3']
};
// Eight independent semantic ramps per colour family: neutral, skin, foliage,
// secondary environment, blue, violet, red and gold (role order is stable).
const ramps={
 meadow:['243B36 718369 E8DEC2','51363E BB8371 F7D3A1','24483E 78A95B C5DB8B','374E34 8F9D56 E6D68D','28445B 5E9DB5 B5D7D5','403650 8B79A1 D1BFCD','663F3F B56E62 EAB091','6A543A C19B58 F5DC9E'],
 coast:['25465F 709DAD E5E2CD','573C46 C08D7B F9D5AB','28585B 69A98D B4D0A1','455541 A7AB75 E5D29A','244D70 55AFBA B6E2DB','4B4263 9688AC DBC5D7','704745 C47D72 EDAE91','705B3E BDA77A F1E3B8'],
 fusion:['11172F 4A5B77 C3C5CA','4A3045 AD7773 F2BF9C','183F43 367F70 8AC9A0','233C59 356A94 89B4D0','162E50 2D83AD 8BDBE2','352443 73598A C4A6C4','5C2B43 C14E64 F4A198','70402C D38C42 F9DB8A'],
 alarm:['22172E 655366 D1C2BB','51323E BA7E70 F4C9A6','244344 4D8A7D A2C8A1','3D3D50 7B7D94 C3CAD0','24394F 42798C 9FC5C9','46274D 90567E D6A0BA','6B2F3C B75843 ED9970','72502F C18F47 F7CB73'],
 autumn:['303D47 8C9184 E6DDC4','553943 BA8874 F4CBA9','304D43 71875A C2BD7C','5C403B AF7658 E0B377','2F4B60 759EAF C7D9D3','493F5A 927B9B D3BDCB','714047 BA7769 EBA697','705A3D C0A166 F2D49B'],
 spring:['344D4B 8DAB98 E9E3CA','563E48 BC8D7B F6D3AE','2C5144 79A568 C2D48A','46543C A2AA6B E0D29B','2D536B 71AFBC C3DDDA','50465E A18CA7 DBC9D7','75464D C4877B F2B7A2','766044 C6AA70 F0DBAB'],
 alpine:['29294F 83859D E0DCE0','53374F BB8490 F7CBBD','2C4A54 648D8D B1C3A3','3C3D62 7778BE BBBEEA','273B68 6296C4 B9D9E6','4E386C A379BC DCC1E4','723E6C C778A1 F4B4C7','756073 BEA398 F0D9B5'],
 solstice:['193A58 6F929D E2DECA','573E4E BD8A83 F7CEAC','245447 72A27B BDCA87','315952 4DACA3 A6D8C2','24466C 5F93C4 B5D6E6','51436C 9A83B4 D9BEDA','78495A C67D83 F0B5A3','795C38 C8A260 F5D997'],
 drifter:['171C35 5C657A C6CAD0','58354C B78286 F3C1AE','1F484B 438B9C 9CD9CB','243953 4783AC 9FCBDE','29346A 657CCC C0CBEA','4E2964 9A57AC D5A1D3','763A6B BC5E9D EC78AF','795845 C5A16B F4DBA0'],
 rose:['221C35 6F5A70 D1BAC1','593D4D B78079 F1B79A','29474B 568A87 A9C3AA','30415D 657D9F B6C5D1','30355C 7077A9 BFC3DF','4D2E53 925774 CDA0B8','773C56 BE6D7B EDA999','7B5B47 C09B68 EBD1A2'],
 sandstone:['34304B 938174 E4D7BC','573741 B98970 F3D6A0','344D3E 80945D C9CF8E','584535 AC8557 EACB88','303F65 758EA9 CBD7DC','4B3B60 9B7EAA D5BED2','703F45 B87568 E9AF90','725535 B18A58 F0D3A1'],
 banner:['252F45 778EA8 DDE2D4','513945 B48275 E7CCB0','2B4845 648B73 B2C69B','404B55 879DA7 C8D1CA','293E63 658CAD B6D0D8','493F68 8D80A3 CEBFD1','693C4C B3656B E8A194','6C593C B5A176 E9D7A8'],
 field:['2A3435 7E8868 C9C9AD','513C3D AE8571 EBC59E','2D463A 6B8051 AFB47E','3B464B 82918E CCD1BD','2C4056 638391 AFC4CE','473D54 8D7890 C1B2B9','6C3D38 A9694D D7986C','705B3B B18D56 DAB583'],
 fighter:['292D48 7A7F9C D6CFCE','563A45 AE6E62 F4CCA0','2B4A43 6E9473 B6C79A','504145 A07970 D8B595','2A4568 648FB2 C1D5DB','4F3B68 9D79AA D7B4CE','783C4E BD6D71 EFA68B','755337 C3935A ECCC98'],
 bone:['282C30 826B5F DED5BA','503A3D A07869 D8B79B','2A3F3B 617B65 A6AE8A','424744 8D9281 C5C9B2','2C4050 647F8C ACBEC0','453B52 83748F BAAFC0','653C39 A05A47 D08B65','67563D A69265 D8C995'],
 moonstone:['24273C 687A88 D9D8C7','4F3947 A17C79 E1BEA6','273E43 5E8980 ADC0A5','393E53 77859B BFC8CF','293858 607FA3 B0CBD7','49375B 8A709E C4B0CF','683E50 A26776 D69797','675A49 AFA17F E1D4B0']
};
const hex=n=>'#'+n.toUpperCase();
function expand(lines,count){const steps=count/lines.length;return lines.flatMap(line=>{const stops=line.split(' ').map(h=>h.match(/../g).map(x=>parseInt(x,16)));return Array.from({length:steps},(_,i)=>{const t=i/(steps-1),s=t<=.5?0:1,u=t<=.5?t*2:(t-.5)*2;return '#'+stops[s].map((v,c)=>Math.round(v+(stops[s+1][c]-v)*u).toString(16).padStart(2,'0')).join('').toUpperCase();});});}
const settings=(mode,entries)=>({'param.algorithm':mode,...entries});
const records=[
 ['PA02','Soft Desktop 98','日光 98',7,640,16,['peach','lilac'],['Peach Day','Lilac Window'],['yuno-pc98','princess-maker-2-pc98'],{'pa.matrix':3,'pa.strength':45,'pa.edge':85}],
 ['PA03','Midnight 98','午夜 98',7,640,16,['indigo','scarlet'],['Indigo Neon','Scarlet Fantasy'],['yuno-pc98','touhou-mystic-square'],{'pa.matrix':3,'pa.strength':65,'pa.edge':90}],
 ['PA04','Sunny Cartridge','晴日卡带',8,320,48,['meadow','coast'],['Meadow','Coast'],['minish-cap-gba','pokemon-emerald-gba'],{'gba.sampling':3,'gba.feature':65,'gba.outline':8}],
 ['PA06','Orbital Cartridge','轨道空间',8,384,48,['fusion','alarm'],['Fusion Lab','Amber Alarm'],['metroid-fusion-gba','metroid-zero-mission-gba'],{'gba.sampling':3,'gba.feature':75,'gba.outline':5}],
 ['PA07','Quiet Town','静谧小镇',9,640,96,['autumn','spring'],['Autumn Town','Spring Farm'],['eastward','stardew-valley'],{'modern.feature':50,'modern.merge':20,'modern.protection':85,'modern.iterations':2}],
 ['PA08','Crystal Journey','水晶旅程',9,480,64,['alpine','solstice'],['Alpine Dusk','Solstice Coast'],['celeste','sea-of-stars'],{'modern.feature':60,'modern.merge':15,'modern.protection':90,'modern.iterations':1}],
 ['PA09','Neon Afterhours','霓虹夜班',9,640,64,['drifter','rose'],['Drifter Cyan','Afterhours Rose'],['hyper-light-drifter','va11-halla'],{'modern.feature':60,'modern.merge':15,'modern.protection':90,'modern.iterations':1}],
 ['PA12','Two-Tone Chronicle','双色档案',7,960,2,['paper','terminal'],['Paper','Terminal'],['obra-dinn','world-of-horror'],{'pa.matrix':3,'pa.strength':80,'pa.edge':90}],
 // Append only: existing mode-local menu ordinals and colour hashes stay stable.
 ['PA01','Pocket Memory','掌机记忆',7,320,4,['olive','grey'],['Olive','Pocket Grey'],['pokemon-red-gb','links-awakening-gb'],{'pa.matrix':2,'pa.strength':30,'pa.edge':85}],
 ['PA05','Golden Chronicle','砂金战记',8,320,64,['sandstone','banner'],['Sandstone Gold','Steel Banner'],['golden-sun-gba','fire-emblem-gba'],{'gba.sampling':3,'gba.feature':70,'gba.outline':12}],
 ['PA10','Arcade Motion','街机动势',9,480,64,['field','fighter'],['Field Steel','Sunset Fighter'],['metal-slug-3','garou'],{'modern.feature':70,'modern.merge':10,'modern.protection':95,'modern.iterations':1,'modern.speckles':0,'modern.spurs':0}],
 ['PA11','Ashen Cathedral','灰烬教堂',9,640,64,['bone','moonstone'],['Bone and Rust','Moonlit Chapel'],['blasphemous','aria-of-sorrow'],{'modern.feature':60,'modern.merge':15,'modern.protection':90,'modern.iterations':1}]
];
const displayNames={PA02:['石印青','紫调暗房'],PA03:['墨绿画报','玫瑰印相'],PA04:['焦糖胶片','海雾蓝晒'],PA06:['金橙画报','旧刊暖褐'],PA07:['米白胶片','栗色暗房'],PA08:['紫丁香印','午夜蓝晒'],PA09:['品红校样','夕照印相'],PA12:['黑白网点','青绿双色'],PA01:['橄榄掌机','灰阶掌机'],PA05:['砂金战记','钢蓝旗帜'],PA10:['军绿钢影','格斗夕照'],PA11:['骨白铁锈','月下石城']};
const catalog={schema_version:1,catalog_id:'FYLM.PixelArt.P1.v1',algorithm_version:1,encoding:'srgb',presets:records.map(([id,name,zh,mode,logical_width,count,variants,variantNames,references,values])=>({id,revision:1,name,name_zh:zh,mode,logical_width,references,color_origin:'original-authored-ramps; not official or directly extracted game palettes',settings:settings(mode,{...(mode===9?{'modern.sampling':3,'modern.cluster_area':4,'modern.speckles':1,'modern.spurs':1,'modern.stairs':0,'modern.debug':1,'modern.outline':0}:{}),...(mode===7?{'pa.spec':1}:{}),...values}),variants:variants.map((key,i)=>{const colors=pc[key]?pc[key].map(hex):key==='paper'?['#20282A','#E5D9B8']:key==='terminal'?['#132B29','#A8C99C']:expand(ramps[key],count);if(colors.length!==count||new Set(colors).size!==count)throw Error(`${id} ${key}: invalid unique colours`);const palette_id=`FYLM.${id}.${i?'B':'A'}.v1`,content_sha256=crypto.createHash('sha256').update(colors.join('\n')+'\n').digest('hex');return {id:palette_id,variant:i?'B':'A',name:displayNames[id][i],legacy_name:variantNames[i],colors,content_sha256};})}))};
const dir=path.join(root,'assets/presets/pixel-art');fs.mkdirSync(dir,{recursive:true});fs.writeFileSync(path.join(dir,'catalog-v1.json'),JSON.stringify(catalog,null,2)+'\n');
for(const preset of catalog.presets)for(const variant of preset.variants)fs.writeFileSync(path.join(dir,`${preset.id}-${variant.variant}.json`),JSON.stringify({schema_version:1,name:`${preset.name} / ${variant.name}`,encoding:'srgb',preset_id:preset.id,palette_id:variant.id,content_sha256:variant.content_sha256,color_origin:preset.color_origin,colors:variant.colors},null,2)+'\n');
console.log(`Authored ${catalog.presets.length} recipes / ${catalog.presets.length*2} complete original palettes`);
for(const locale of ['en','zh-Hans','zh-Hant','ja']){const file=path.join(root,'i18n',locale+'.json'),j=JSON.parse(fs.readFileSync(file,'utf8').replace(/^\uFEFF/,''));for(const [mode,key] of [[7,'pc98'],[8,'gba'],[9,'modern']]){j[`library.${key}.preset`]=`${key==='pc98'?'PC-98':key==='gba'?'GBA':'Modern'} ${({en:'Preset','zh-Hans':'专属预设','zh-Hant':'專屬預設',ja:'専用プリセット'}[locale])}`;j[`library.${key}.apply`]=j['library.apply'];j[`library.${key}.items`]=[({en:'Select a preset','zh-Hans':'请选择预设','zh-Hant':'請選擇預設',ja:'プリセットを選択'}[locale]),...catalog.presets.filter(p=>p.mode===mode).flatMap(p=>p.variants.map(v=>`${p.id} ${v.variant} · ${v.name}`))].join('|');}j['pa.error.preset_mode']=locale==='en'?'Preset does not belong to the current mode; no changes applied.':'预设不属于当前模式，未修改参数。';j['library.help']=locale==='en'?'Choose a mode, then its dedicated named preset in native Effect Controls and click Apply. Applying never switches algorithms.':'请先选择算法模式，再在原生效果控件中选择该模式的专属预设并点击应用；应用不会切换算法。';fs.writeFileSync(file,JSON.stringify(j,null,2)+'\n');}
