#pragma once
static const char* replacementVertexShader=R"GLSL(#version 300 es
layout(location=0) in vec2 position;
layout(location=1) in vec2 uv;
uniform vec2 logicalSize;
out vec2 texCoord;
void main(){
 vec2 p=position/logicalSize;
 gl_Position=vec4(p.x*2.0-1.0,1.0-p.y*2.0,0.0,1.0);
 texCoord=uv;
}
)GLSL";
static const char* replacementFragmentShader=R"GLSL(#version 300 es
precision mediump float;
uniform sampler2D atlas;
in vec2 texCoord;
out vec4 color;
void main(){color=texture(atlas,texCoord);}
)GLSL";
