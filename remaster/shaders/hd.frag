#version 300 es
precision highp float;
precision highp int;
uniform sampler2D sourceFrame;
uniform float strength;
in vec2 uv;
out vec4 fragColor;
float lum(vec3 c){return dot(c,vec3(0.2126,0.7152,0.0722));}
float contrast(vec3 a,vec3 b){vec3 d=abs(a-b);return max(abs(lum(a)-lum(b)),0.65*max(d.r,max(d.g,d.b)));}
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
 if(strength<=0.0){fragColor=vec4(f.y<0.5?(f.x<0.5?a:b):(f.x<0.5?c:d),1.0);return;}
 vec3 smoothColor=mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
 vec3 nearest=f.y<0.5?(f.x<0.5?a:b):(f.x<0.5?c:d);
 float edge=max(max(contrast(a,b),contrast(c,d)),max(contrast(a,c),contrast(b,d)));
 float diagonalA=contrast(a,d), diagonalB=contrast(b,c);
 float diagonalWeight=smoothstep(0.025,0.16,abs(diagonalA-diagonalB));
 vec3 diagonal=diagonalA<diagonalB?mix(a,d,clamp((f.x+f.y)*0.5,0.0,1.0)):mix(b,c,clamp((1.0-f.x+f.y)*0.5,0.0,1.0));
 float proximity=diagonalA<diagonalB?1.0-abs(f.x-f.y):1.0-abs(f.x+f.y-1.0);
 smoothColor=mix(smoothColor,diagonal,0.60*diagonalWeight*(proximity*proximity*proximity*proximity));
 vec3 lo=min(min(a,b),min(c,d)), hiColor=max(max(a,b),max(c,d));
 float detail=1.0-smoothstep(0.08,0.30,edge);
 vec3 localMean=(a+b+c+d)*0.25;
 vec3 enhanced=clamp(smoothColor+(smoothColor-localMean)*0.24*detail,lo,hiColor);
 float boundary=smoothstep(0.08,0.30,edge);
 vec3 hd=mix(enhanced,nearest,0.72*boundary);
 fragColor=vec4(mix(nearest,hd,clamp(strength,0.0,1.0)),1.0);
}
