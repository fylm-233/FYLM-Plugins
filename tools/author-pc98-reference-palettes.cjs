// Original mathematical reference palettes; not sampled official game palettes.
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const root=path.resolve(__dirname,'..'),dir=path.join(root,'assets/palettes');
const rgb=hex=>hex.slice(1).match(/../g).map(x=>parseInt(x,16));
const hex=c=>'#'+c.map(x=>x.toString(16).padStart(2,'0')).join('').toUpperCase();
const art=JSON.parse(fs.readFileSync(path.join(dir,'fylm-pc98.json'),'utf8'));
for(const [file,name,spec,bits,entries] of [
 ['fylm-pc9801-digital8.json','FYLM PC-9801 Digital RGB8',1,1,Array.from({length:8},(_,i)=>[i&4?255:0,i&2?255:0,i&1?255:0])],
 ['fylm-pc9801-rgb444-16.json','FYLM PC-9801 RGB444 Reference 16',2,4,art.colors.map(c=>rgb(c).map(x=>Math.round(x/17)*17))],
 ['fylm-pc9821-256.json','FYLM PC-9821 RGB332 Reference 256',3,8,Array.from({length:256},(_,i)=>[Math.round(((i>>5)&7)*255/7),Math.round(((i>>2)&7)*255/7),(i&3)*85])]
]){const colors=entries.map(hex);if(new Set(colors).size!==colors.length)throw Error('Duplicate reference colours');const content_sha256=crypto.createHash('sha256').update(colors.join('\n')+'\n').digest('hex');fs.writeFileSync(path.join(dir,file),JSON.stringify({schema_version:1,name,encoding:'srgb',pc98_color_spec:spec,palette_channel_bits:bits,color_origin:'FYLM mathematical/reference colour table, not an official game palette',content_sha256,colors},null,2)+'\n');}
console.log('Authored digital 8 / RGB444 16 / indexed 256 reference palettes.');
