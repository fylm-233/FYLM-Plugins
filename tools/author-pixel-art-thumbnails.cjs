// Original four-scene diagnostic illustration. Never consumes user or game artwork.
const fs=require('fs'),path=require('path'),zlib=require('zlib'),cp=require('child_process');
const root=path.resolve(__dirname,'..'),dir=path.join(root,'assets/presets/pixel-art/thumbnails'),work=path.join(root,'build/p2-preview');fs.mkdirSync(dir,{recursive:true});fs.mkdirSync(work,{recursive:true});
const w=768,h=512,input=Buffer.alloc(w*h*4);
for(let y=0;y<h;y++)for(let x=0;x<w;x++){
 const X=x%384,Y=y%256,scene=(x>=384?1:0)+(y>=256?2:0);let c;
 if(scene===0){c=[85+X/5,115+Y/4,157];if((X-192)**2/85**2+(Y-139)**2/98**2<1)c=[238,183,152];if(Y<90&&Math.abs(X-192)<90)c=[43+(X%19<3?36:0),36,63];if(Y>218&&X>105&&X<280)c=[86,80,155];if(Y>128&&Y<151&&(Math.abs(X-155)<14||Math.abs(X-226)<14))c=[35,39,62];if(Y>132&&Y<139&&(Math.abs(X-152)<5||Math.abs(X-223)<5))c=[250,242,218];}
 if(scene===1){c=Y<120?[102+Y/3,153+Y/5,192+Y/6]:[81,127,83];if(Y>180)c=[74,133+Y/8,161];if(Y>70+Math.abs(X-180)*.5&&Y<155)c=[105,104,139];if(Y>110&&Y<185&&X%63<12)c=[61,80,64];}
 if(scene===2){c=[137+X/5,103+Y/5,98];if(X>28&&X<160&&Y>25&&Y<145)c=X%65<4||Y%64<4?[62,67,87]:[132,178,194];if(Y>190)c=[117+X%17,84+Y%13,66];if(Y>135&&Y<154&&X>170)c=[206,172,111];if(X>250&&X<278&&Y>60&&Y<136)c=[236,204,136];}
 if(scene===3){c=[39+Y/8,52+X/16,71+Y/7];if(X>58&&X<328&&Y>65&&Y<209)c=[94,110,121];if((X-145)**2+(Y-133)**2<44**2)c=[39,54,72];if(Math.abs(X-145)<8&&Y>80&&Y<187)c=[186,145,87];if(X>242&&X<295&&Y>93&&Y<150)c=[77,197,167];if(Y>230&&X%27<15)c=[222,167,87];}
 const i=(y*w+x)*4;for(let k=0;k<3;k++)input[i+k]=Math.max(0,Math.min(255,Math.round(c[k])));input[i+3]=255;
}
function crc(b){let c=~0;for(const v of b){c^=v;for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xedb88320:0);}return (c^~0)>>>0;}
function chunk(name,data){const type=Buffer.from(name),body=Buffer.concat([type,data]),head=Buffer.alloc(4),tail=Buffer.alloc(4);head.writeUInt32BE(data.length);tail.writeUInt32BE(crc(body));return Buffer.concat([head,body,tail]);}
function png(bytes,W,H){const header=Buffer.alloc(13);header.writeUInt32BE(W);header.writeUInt32BE(H,4);header[8]=8;header[9]=6;const rows=Buffer.alloc((W*4+1)*H);for(let y=0;y<H;y++)bytes.copy(rows,y*(W*4+1)+1,y*W*4,(y+1)*W*4);return Buffer.concat([Buffer.from([137,80,78,71,13,10,26,10]),chunk('IHDR',header),chunk('IDAT',zlib.deflateSync(rows)),chunk('IEND',Buffer.alloc(0))]);}
fs.writeFileSync(path.join(work,'source.rgba'),input);fs.writeFileSync(path.join(dir,'source.png'),png(input,w,h));
fs.writeFileSync(path.join(dir,'source-annotations.json'),JSON.stringify({origin:'FYLM original procedural diagnostic artwork',width:w,height:h,scenes:['portrait','landscape','interior','mechanical'],details:{eyes:[[141,128,28,23],[212,128,28,23]],hair:[102,30,180,60],highlights:[384+242,256+93,53,57],thin_lines:[28,256+25,132,120]},status:'diagnostic thumbnails; subjective visual acceptance pending'},null,2)+'\n');
const catalog=JSON.parse(fs.readFileSync(path.join(root,'assets/presets/pixel-art/catalog-v1.json')));
for(const p of catalog.presets)for(const v of p.variants){const file=path.join(work,p.id+'-'+v.variant+'.rgba');cp.execFileSync(path.join(root,'build/Release/pixel_art_cli.exe'),['--preset',p.id,v.variant,path.join(work,'source.rgba'),file,String(w),String(h)]);const raw=fs.readFileSync(file),small=Buffer.alloc(192*128*4);for(let y=0;y<128;y++)for(let x=0;x<192;x++)raw.copy(small,(y*192+x)*4,((y*4)*w+x*4)*4,((y*4)*w+x*4)*4+4);fs.writeFileSync(path.join(dir,p.id+'-'+v.variant+'.png'),png(small,192,128));}
console.log('24 engine-rendered thumbnails from an original four-scene annotated fixture.');
