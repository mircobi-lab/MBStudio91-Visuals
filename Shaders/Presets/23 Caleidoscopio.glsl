// Griglia specchiata (Param1 = spicchi, Param2 = densità)
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float seg=floor(3.+iParam1*9.);
  float a=atan(uv.y,uv.x), r=length(uv);
  float s=6.28318/seg;
  a=abs(mod(a+iTime*.1,s)-s*.5);
  vec2 p=vec2(cos(a),sin(a))*r;
  p*=3.+iParam2*6.;
  p+=vec2(iTime*.4,0.);
  vec2 g=abs(fract(p)-.5);
  float l=glow(.06+iBass*.1,min(g.x,g.y));
  vec3 col=.5+.5*cos(vec3(0.,2.,4.)+r*4.-iTime+iMid*2.);
  o=vec4(col*l*(.6+iBeat)+col*.08,1.);
}
