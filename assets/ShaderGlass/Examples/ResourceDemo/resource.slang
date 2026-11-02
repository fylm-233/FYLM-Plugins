#version 450
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
