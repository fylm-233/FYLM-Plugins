const fs=require('fs'),p=require('path'),cp=require('child_process'),crypto=require('crypto'),assert=require('assert');
const root=process.argv[2],work=process.argv[3],tool=p.join(root,'build/modules/shaderglass/Release/FYLM-sgtool.exe');fs.mkdirSync(work,{recursive:true});
const hash=b=>crypto.createHash('sha256').update(b).digest('hex'),fixture=fs.readFileSync(p.join(root,'assets/ShaderGlass/Library/crt-geom.fylmsg')),out=p.join(work,'result.fylmsg');fs.writeFileSync(out,fixture);
const bad=Buffer.from(fixture);let changed=0;for(let i=0;i<bad.length-4;i++)if(bad.subarray(i,i+4).toString()==='DXBC'){bad.write('BAD!',i);changed++;}assert.ok(changed>=2);const source=p.join(work,'corrupt.fylmsg');fs.writeFileSync(source,bad);
const rejected=cp.spawnSync(tool,['import',source,out],{windowsHide:true,encoding:'utf8',timeout:20000});assert.notStrictEqual(rejected.status,0,'corrupt bytecode accepted');assert.ok((rejected.stderr||'').includes('sg.error.validation'),'missing validation diagnostic');assert.strictEqual(hash(fs.readFileSync(out)),hash(fixture),'failed import replaced previous output');
fs.writeFileSync(p.join(work,'black.slang'),`#version 450
#pragma stage vertex
layout(location=0) in vec4 Position;
layout(location=1) in vec2 TexCoord;
void main(){gl_Position=vec4(Position.xy*2.-1.,0.,1.);}
#pragma stage fragment
layout(location=0) out vec4 FragColor;
void main(){FragColor=vec4(0.,0.,0.,1.);}
`);fs.writeFileSync(p.join(work,'black.slangp'),'shaders=1\nshader0=black.slang\n');
const black=cp.spawnSync(tool,['import',p.join(work,'black.slangp'),out],{windowsHide:true,encoding:'utf8',timeout:20000});assert.strictEqual(black.status,0,black.stderr);assert.notStrictEqual(hash(fs.readFileSync(out)),hash(fixture));assert.ok(!fs.readdirSync(work).some(f=>f.includes('.partial-')),'partial output leaked');
console.log('GPU validation rejects corrupt bytecode atomically and accepts valid black output');
