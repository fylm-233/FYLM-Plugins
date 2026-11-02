// Public source metadata only. No copyrighted screenshots or user artwork are bundled.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'..'),dir=path.join(root,'docs/presets');fs.mkdirSync(dir,{recursive:true});
const works=[
 ['pokemon-red-gb','Pokémon Red','original Game Boy',null,'https://www.nintendo.com/en-gb/Games/Game-Boy/Pokemon-Red-Version-266109.html'],
 ['links-awakening-gb',"The Legend of Zelda: Link’s Awakening",'original Game Boy, not Switch remake',null,'https://zelda.nintendo.com/about/'],
 ['yuno-pc98','YU-NO','1996 PC-98, not 2019 remake or Windows port',null,'https://lospec.com/palette-list/yu-no-pc-98'],
 ['princess-maker-2-pc98','Princess Maker 2','PC-98, not Refine or Regeneration',null,'https://www.gamingalexandria.com/pc98/Princess%20Maker%202/Princess%20Maker%202%20-%20Operators%20Manual.pdf'],
 ['touhou-mystic-square','東方怪綺談','original PC-98',null,'https://retrorama.fr/en/game/touhou-kaikidan-mystic-square'],
 ['minish-cap-gba','The Legend of Zelda: The Minish Cap','original GBA / VC emulation',null,'https://www.palaiszelda.com/theminishcap/screenshots.php'],
 ['pokemon-emerald-gba','Pokémon Emerald','original GBA',null,'https://www.nintendo.com/en-gb/News/2016/August/What-is-Pokemon-Catch-up-on-the-phenomenon-with-our-feature-news-1128960.html'],
 ['golden-sun-gba','Golden Sun','original GBA / VC emulation',null,'https://www.nintendo.com/en-gb/Games/Game-Boy-Advance/Golden-Sun-866531.html'],
 ['fire-emblem-gba','Fire Emblem: The Blazing Blade','original GBA',null,'https://www.nintendo.com/jp/fe/history/index.html'],
 ['metroid-fusion-gba','Metroid Fusion','original GBA / Wii U VC screenshots',null,'https://www.nintendo.com/en-gb/Games/Game-Boy-Advance/Metroid-Fusion-267046.html'],
 ['metroid-zero-mission-gba','Metroid: Zero Mission','original GBA',null,'https://www.nintendo.com/eu/media/downloads/games_8/emanuals/game_boy_advance_8/Manual_GameBoyAdvance_MetroidZeroMission_EN_DE_FR_ES_IT.pdf'],
 ['eastward','Eastward','PC Steam',977880],['stardew-valley','Stardew Valley','PC Steam',413150],
 ['celeste','Celeste','PC Steam',504230],['sea-of-stars','Sea of Stars','PC Steam',1244090],
 ['hyper-light-drifter','Hyper Light Drifter','PC Steam',257850],['va11-halla','VA-11 Hall-A','PC Steam',447530],
 ['metal-slug-3','Metal Slug 3','Steam port; original arcade reference',250180],['garou','Garou: Mark of the Wolves','Steam port; original arcade reference',366240],
 ['blasphemous','Blasphemous','PC Steam',774361],['aria-of-sorrow','Castlevania: Aria of Sorrow','GBA game within Advance Collection; identify individual screenshots',null,'https://www.konami.com/games/castlevania/advance_collection/us/en-us/'],
 ['obra-dinn','Return of the Obra Dinn','PC Steam',653530],['world-of-horror','World of Horror','PC Steam',913740]
];
const designs=[
 ['PA01','Pocket Memory',['pokemon-red-gb','links-awakening-gb'],'Olive','Pocket Grey','4','图标、UI、简洁人物','四级明度与轮廓优先'],
 ['PA02','Soft Desktop 98',['yuno-pc98','princess-maker-2-pc98'],'Peach Day','Lilac Window','16','二次元立绘、日光室内','暖肤色或灰紫阴影'],
 ['PA03','Midnight 98',['yuno-pc98','touhou-mystic-square'],'Indigo Neon','Scarlet Fantasy','16','夜景、幻想人物、音乐视觉','青紫暗底或红衣暖色强调'],
 ['PA04','Sunny Cartridge',['minish-cap-gba','pokemon-emerald-gba'],'Meadow','Coast','48','草地、小镇、旅行与角色','暖土绿植或浅海沙滩'],
 ['PA05','Golden Chronicle',['golden-sun-gba','fire-emblem-gba'],'Sandstone Gold','Steel Banner','64','奇幻人物、建筑、地图','金棕暖光或铠甲蓝灰'],
 ['PA06','Orbital Cartridge',['metroid-fusion-gba','metroid-zero-mission-gba'],'Fusion Lab','Amber Alarm','48','科幻、机械、宇宙','冷空间暖主体或警报琥珀色'],
 ['PA07','Quiet Town',['eastward','stardew-valley'],'Autumn Town','Spring Farm','96','生活插画、室内、田园','秋木红砖或春日绿植'],
 ['PA08','Crystal Journey',['celeste','sea-of-stars'],'Alpine Dusk','Solstice Coast','64','轻幻想、天空、山与水','蓝紫远景或青海金光'],
 ['PA09','Neon Afterhours',['hyper-light-drifter','va11-halla'],'Drifter Cyan','Afterhours Rose','64','赛博人物、夜城、音乐封面','青品红对比或克制玫瑰色'],
 ['PA10','Arcade Motion',['metal-slug-3','garou'],'Olive Steel','Fighter Sunset','64','动作、机械、火花弹丸','厚重土绿金属或夕阳暖色'],
 ['PA11','Ashen Cathedral',['blasphemous','aria-of-sorrow'],'Bone and Rust','Moonlit Stone','64','暗黑角色、城堡、宗教建筑','骨白铁锈或冷石月光'],
 ['PA12','Two-Tone Chronicle',['obra-dinn','world-of-horror'],'Paper','Terminal','2','悬疑、档案、恐怖UI','纸张明暗或绿色终端']
];
(async()=>{
 const sources=[];
 for(const [id,title,edition,app,link] of works){
  const source={id,title,edition,url:link||`https://store.steampowered.com/app/${app}/`,samples:[],status:'needs-original-edition-scene-lock'};
  if(app){const api=`https://store.steampowered.com/api/appdetails?appids=${app}&l=english`;let json;if(process.argv[2])json=JSON.parse(fs.readFileSync(path.join(process.argv[2],`${app}.json`),'utf8').replace(/^\uFEFF/,''));else{const response=await fetch(api);if(!response.ok)throw Error(`${id}: ${response.status}`);json=await response.json();}const data=json[app];if(!data?.success)throw Error(`${id}: unavailable`);source.samples=data.data.screenshots.slice(0,6).map(s=>({id:String(s.id),url:s.path_full,scene_tags:[],status:'publisher-gallery-link-locked; scene-review-pending'}));source.api=api;source.status=source.samples.length>=4?'gallery-links-locked; scene-review-pending':'insufficient-samples';}
  if(id==='metroid-fusion-gba')source.samples=Array.from({length:6},(_,i)=>({id:String(i+1),url:`https://www.nintendo.com/eu/media/images/06_screenshots/games_5/virtual_console_wii_u_3/wiiuvc_metroidfusion/WiiUVC_MetroidFusion_0${i+1}.bmp`,scene_tags:[],status:'official-original-game-VC; scene-review-pending'}));
  if(id==='touhou-mystic-square')source.samples=['438627','438626','438629','438630'].map(n=>({id:n,url:`https://retrorama.fr/assets/images/screenshots/screenshot-${n}.jpg`,scene_tags:[],status:'archive-PC98-gallery; scene-review-pending'}));
  source.metadata_sha256=crypto.createHash('sha256').update(JSON.stringify(source)).digest('hex');sources.push(source);
 }
 const library=JSON.parse(fs.readFileSync(path.join(root,'assets/presets/pixel-art/catalog-v1.json')));
 const cards=designs.map(([id,name,references,a,b,colors,scenario,tone])=>({id,name,references,scenario,tone,target_colors:Number(colors),variants:[{variant:'A',name:a},{variant:'B',name:b}],palette_status:library.presets.some(p=>p.id===id)?'complete-original-palettes-implemented; visual-acceptance-pending':'design-only',sampling_method:'Reference art direction, not direct image extraction; original FYLM semantic RGB ramps.',scene_requirements:['character/skin','sky/water','foliage','architecture/interior','night/effects'],status:'partial; scene reviews and original-edition gaps remain'}));
 fs.writeFileSync(path.join(dir,'reference-cards-v1.json'),JSON.stringify({schema_version:1,locked_at:new Date().toISOString(),rights:'External screenshot links for visual research only. No game screenshots or user artwork distributed.',cards,sources},null,2)+'\n');
 console.log(`${cards.length} cards / ${cards.length*2} designs; ${sources.length} works / ${sources.reduce((n,s)=>n+s.samples.length,0)} sample links. NOT full ST-M0 acceptance.`);
})().catch(e=>{console.error(e);process.exitCode=1;});
