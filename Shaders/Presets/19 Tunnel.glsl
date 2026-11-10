// Tunnel: anelli che passano sul beat
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float r=length(uv)+1e-3, a=atan(uv.y,uv.x);
  float z=.35/r+iTime*(.6+iParam1*2.);
  float seg=floor(6.+iParam2*10.);
  float g=abs(sin(z*3.14159))*abs(sin(a*seg+iTime*.3));
  float ring=pow(1.-abs(fract(z-iPhase)-.5)*2.,8.)*(.4+iBeat*1.5);
  vec3 c=.5+.5*cos(vec3(0.,2.,4.)+z*.6+iMid*3.);
  vec3 col=c*g*(.4+iBass*1.2)+ring*vec3(1.,.8,.5);
  col*=smoothstep(0.,.25,r);
  o=vec4(col,1.);
}
