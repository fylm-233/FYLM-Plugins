// Original deterministic fixture, not user artwork. A 2x2 24-bit BMP lookup image.
const fs=require('fs'),p=require('path'),dir=process.argv[2];fs.mkdirSync(dir,{recursive:true});
const b=Buffer.alloc(70);b.write('BM');b.writeUInt32LE(70,2);b.writeUInt32LE(54,10);b.writeUInt32LE(40,14);b.writeInt32LE(2,18);b.writeInt32LE(2,22);b.writeUInt16LE(1,26);b.writeUInt16LE(24,28);b.writeUInt32LE(16,34);
Buffer.from([0,0,255,0,255,0,0,0,255,0,0,255,255,255,0,0]).copy(b,54);fs.writeFileSync(p.join(dir,'lookup.bmp'),b);
fs.writeFileSync(p.join(dir,'resource.slang'),`#version 450
layout(push_constant) uniform Push { vec4 SourceSize; vec4 OutputSize; float ResourceLOD; float BlurSize; } params;
#pragma parameter ResourceLOD "Lookup mip level" 0.0 0.0 1.0 1.0
#pragma parameter BlurSize "Source mip level" 0.0 0.0 2.0 1.0
layout(set=0,binding=0) uniform sampler2D Source;
layout(set=0,binding=1) uniform sampler2D Lookup;
#pragma stage vertex
layout(location=0) in vec4 Position;
layout(location=1) in vec2 TexCoord;
layout(location=0) out vec2 uv;
void main(){gl_Position=vec4(Position.xy*2.-1.,0.,1.);uv=TexCoord;}
#pragma stage fragment
layout(location=0) in vec2 uv;
layout(location=0) out vec4 FragColor;
void main(){FragColor=vec4(textureLod(Lookup,uv*2.,params.ResourceLOD).rgb*.75+textureLod(Source,uv,params.BlurSize).rgb*.25,1.);}
`);
fs.writeFileSync(p.join(dir,'resource.slangp'),'shaders = 1\nshader0 = resource.slang\nmipmap_input0 = true\nfilter_linear0 = false\ntextures = Lookup\nLookup = lookup.bmp\nLookup_linear = false\nLookup_wrap_mode = repeat\nLookup_mipmap = true\n');
