// Generates embedded bytecode and stable menu metadata. No input from private QA directories.
const fs=require('fs'),p=require('path'),crypto=require('crypto');
function write(file,text){if(!fs.existsSync(file)||fs.readFileSync(file,'utf8')!==text)fs.writeFileSync(file,text);}
const root=p.resolve(__dirname,'..'),dir=p.join(root,'assets/ShaderGlass/Library');
const c=JSON.parse(fs.readFileSync(p.join(dir,'catalog-v1.json'),'utf8'));
for(let i=0;i<c.entries.length;i++)if(c.entries[i].menu!==i+1)throw Error('Menu identity/order');
if(c.entries[0].base!=='crt-geom'||c.entries[1].base!=='crt-easymode'||c.entries[2].slug!=='imported')throw Error('Frozen menu identities');
const q=JSON.stringify;
const menu=c.entries.map(e=>e.menu<=3?e.name:e.category+' / '+e.name).join('|');
const panel=c.entries.map(e=>({menu:e.menu,id:e.id,slug:e.slug,category:e.category,name:e.name,zh:e.zh,ja:e.ja,passes:e.passes||0,performance:e.performance||'',use:e.use||'',virtual_height:e.virtual_height,art:e.category==='Art',status:e.status||'',slots:e.display_slots||e.slots||[],pixel_grid_profile:!!e.pixel_grid_profile,zh_hant:e.zh_hant||e.zh,use_hant:e.use_hant||e.use}));
const ascii=s=>s.replace(/[^\x00-\x7F]/g,c=>'\\u'+c.charCodeAt(0).toString(16).padStart(4,'0'));
const literal=(name,value)=>{const s=ascii(value);if(s.length<60000)return 'inline constexpr const char* '+name+'=R"FYLM('+s+')FYLM";\n';const chunks=[];for(let i=0;i<s.length;i+=20000)chunks.push('R"FYLM('+s.slice(i,i+20000)+')FYLM"');return 'inline const std::string '+name+'=std::string('+chunks.shift()+')+'+chunks.join('+')+';\n';};
write(p.join(root,'modules/shaderglass/library_catalog.hpp'),'#pragma once\n#include <string>\nnamespace fylm::sg {\ninline constexpr int LibraryMenuCount='+c.entries.length+';\ninline constexpr const char* LibraryMenuItems='+q(menu)+';\n'+literal('LibraryPanelJson',JSON.stringify(panel))+'}\n');
if(process.argv[2]){const out=p.resolve(process.argv[2]);fs.mkdirSync(p.dirname(out),{recursive:true});let s='#pragma once\n#include <cstddef>\n#include <string>\nnamespace fylm::sg::data {\n';const bases=[...new Set(c.entries.map(e=>e.base).filter(Boolean))];
 for(let i=0;i<bases.length;i++){const bytes=fs.readFileSync(p.join(dir,bases[i]+'.fylmsg'));const expected=c.entries.find(e=>e.base===bases[i]).sha256;if(crypto.createHash('sha256').update(bytes).digest('hex')!==expected)throw Error('Bytecode hash mismatch '+bases[i]);s+='static const unsigned char blob'+i+'[]={'+[...bytes].map(x=>'0x'+x.toString(16)).join(',')+'};\n';}
 s+='struct Blob{const char* name;const unsigned char* bytes;std::size_t size;};\nstatic const Blob blobs[]={'+bases.map((b,i)=>'{'+q(b)+',blob'+i+',sizeof(blob'+i+')}').join(',')+'};\n'+literal('catalog',JSON.stringify(c))+'}\n';write(out,s);}
console.log('Generated stable SG library metadata and embedded data');
