#version 450
// SPDX-License-Identifier: GPL-3.0-or-later
layout(std140,set=0,binding=0) uniform UBO {mat4 MVP;vec4 SourceSize;vec4 OutputSize;} global;
#pragma stage vertex
layout(location=0) in vec4 Position;
layout(location=1) in vec2 TexCoord;
layout(location=0) out vec2 uv;
void main(){gl_Position=global.MVP*Position;uv=TexCoord;}
#pragma stage fragment
layout(location=0) in vec2 uv;
layout(location=0) out vec4 FragColor;
layout(set=0,binding=2) uniform sampler2D Source;
void main(){FragColor=texture(Source,uv);}
