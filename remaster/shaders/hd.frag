#version 330 core
uniform sampler2D sourceFrame;
uniform float strength;
in vec2 uv;
out vec4 fragColor;
float lum(vec3 c){return dot(c,vec3(0.2126,0.7152,0.0722));}
void main(){
 vec2 size=vec2(textureSize(sourceFrame,0));
 vec2 p=uv*size-0.5;
 ivec2 i=ivec2(floor(p));
 vec2 f=fract(p);
 ivec2 hi=ivec2(size)-1;
 vec3 a=texelFetch(sourceFrame,clamp(i,ivec2(0),hi),0).rgb;
 vec3 b=texelFetch(sourceFrame,clamp(i+ivec2(1,0),ivec2(0),hi),0).rgb;
 vec3 c=texelFetch(sourceFrame,clamp(i+ivec2(0,1),ivec2(0),hi),0).rgb;
 vec3 d=texelFetch(sourceFrame,clamp(i+ivec2(1,1),ivec2(0),hi),0).rgb;
 vec3 smoothColor=mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
 vec3 nearest=texelFetch(sourceFrame,clamp(ivec2(floor(p+0.5)),ivec2(0),hi),0).rgb;
 float edge=max(max(abs(lum(a)-lum(b)),abs(lum(c)-lum(d))),max(abs(lum(a)-lum(c)),abs(lum(b)-lum(d))));
 fragColor=vec4(mix(nearest,mix(smoothColor,nearest,smoothstep(0.06,0.22,edge)),clamp(strength,0.0,1.0)),1.0);
}
