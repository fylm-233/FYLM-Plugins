#version 330 core
out vec2 vUV;
// Fullscreen triangle. The upload/readback adapter defines UV.y=0 as AE's top row.
void main(){vec2 p=vec2(float((gl_VertexID<<1)&2),float(gl_VertexID&2));vUV=p;gl_Position=vec4(p*2.0-1.0,0,1);}
