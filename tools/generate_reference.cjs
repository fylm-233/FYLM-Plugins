// Build-time only: execute the pinned, unmodified TypeScript reference.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'..'),ref=path.resolve(root,'../dithering-studio');
const execFile=require('child_process').execFileSync;
if(execFile('git',['-C',ref,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='9298a44c54a8d02a5b5b5956c1c1c7f9785fda59')throw Error('Reference commit differs from pinned revision');
if(execFile('git',['-C',ref,'diff','HEAD','--','src'],{encoding:'utf8'}).trim())throw Error('Reference sources have local modifications');
const ts=require(path.join(ref,'node_modules/typescript'));
require.extensions['.ts']=(m,f)=>m._compile(ts.transpileModule(fs.readFileSync(f,'utf8'),{compilerOptions:{module:ts.ModuleKind.CommonJS,target:ts.ScriptTarget.ES2020}}).outputText,f);
const {blueNoiseMask}=require(path.join(ref,'src/utils/algorithms/blueNoise.ts'));
fs.writeFileSync(path.join(root,'core/src/blue_noise.inc'),'// Generated from pinned reference; Apache-2.0\nstatic constexpr unsigned short blueNoise[4096]={'+Array.from(blueNoiseMask).join(',')+'};\n');
const {predefinedPalettes}=require(path.join(ref,'src/utils/palettes.ts'));
let cpp='#include "dither/core.hpp"\n#include <stdexcept>\nnamespace dither { std::vector<Rgb8> palette(const std::string& id){\nif(id=="none")return {};\nif(id=="bw")return {{0,0,0},{255,255,255}};\n';
for(const p of predefinedPalettes)cpp+='if(id=="'+p.id+'")return {'+p.colors.map(c=>'{'+c.join(',')+'}').join(',')+'};\n';
cpp+='throw std::invalid_argument("unknown palette: "+id);\n}}\n';fs.writeFileSync(path.join(root,'core/src/palettes.cpp'),cpp);
const fsrun=require(path.join(ref,'src/utils/algorithms/errorDiffusion.ts')).runFloydSteinberg;
const bayer=require(path.join(ref,'src/utils/algorithms/bayer.ts')),ordered=require(path.join(ref,'src/utils/algorithms/orderedAndOther.ts'));
const pre=require(path.join(ref,'src/utils/preprocess.ts')).applyLuminancePreprocess;
const runners={1:fsrun,2:bayer.runBayer4,8:bayer.runBayer8,15:ordered.runBinaryThreshold,17:ordered.runBlueNoise};
let cases=[];let idx=0;
for(const [w,h] of [[1,1],[2,2],[17,13],[64,64]])for(const kind of ['black','white','gray','color'])for(const id of [1,2,8,15,17])for(const paletteId of ['none','gb','pico8'])for(const invert of [false,true]){
 const tone=idx%3===0?{contrast:25,midtones:1.2,highlights:35}:{contrast:0,midtones:1,highlights:0};
 const threshold=[0,77,128,201,255][idx%5],serpentine=idx%2===0;let input=new Uint8ClampedArray(w*h*4);
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){let i=(y*w+x)*4,v=kind==='black'?0:kind==='white'?255:kind==='gray'?128:(x*17+y*13)%256;input.set([v,kind==='color'?(x*31+y*7)%256:v,kind==='color'?(x*3+y*29)%256:v,(x+y)%3===0?0:(x+y)%3===1?128:255],i);}
 let srcData=new Uint8ClampedArray(input);pre(srcData,tone);const pal=predefinedPalettes.find(p=>p.id===paletteId)?.colors;
 if(pal&&invert)for(let i=0;i<srcData.length;i+=4)for(let c=0;c<3;c++)srcData[i+c]=255-srcData[i+c];
 const out=runners[id]({srcData,width:w,height:h,params:{threshold,invert:pal?false:invert,serpentine,palette:pal,errorDiffusionStrength:100}});
 let name=String(idx++).padStart(4,'0');fs.writeFileSync(path.join(root,'tests/golden',name+'.in.rgba'),input);fs.writeFileSync(path.join(root,'tests/golden',name+'.out.rgba'),out);
 cases.push({name,w,h,id,threshold,serpentine,invert,paletteId,tone,sha256:crypto.createHash('sha256').update(out).digest('hex')});
}
fs.writeFileSync(path.join(root,'tests/golden/manifest.json'),JSON.stringify({reference:'9298a44c54a8d02a5b5b5956c1c1c7f9785fda59',bitDepth:8,workingResolution:0,alpha:'Opaque',cases},null,2));
fs.writeFileSync(path.join(root,'tests/golden/cases.txt'),cases.map(c=>[c.name,c.w,c.h,c.id,c.threshold,+c.serpentine,+c.invert,c.paletteId,c.tone.contrast,c.tone.midtones,c.tone.highlights].join(' ')).join('\n'));
for(const file of ['LICENSE','NOTICE'])fs.copyFileSync(path.join(ref,file),path.join(root,file));fs.appendFileSync(path.join(root,'NOTICE'),'\nNative C++ port and AE adapter added in 2026.\nBuilt upon Steinberg Image – Dithering Studio by Oslo418 (Apache-2.0)\n');
console.log('Generated '+cases.length+' independent TypeScript golden fixtures and native constant tables.');
