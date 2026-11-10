// Campo stellare (Param1 = velocità, Param2 = densità)
float h21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  vec3 col=vec3(0.);
  float sp=iTime*(.2+iParam1*1.5);
  for(int l=0;l<5;l++){
    float d=fract(float(l)*.2+sp);
    float sc=mix(20.,.5,d);
    vec2 p=uv*sc+float(l)*7.3;
    vec2 id=floor(p), g=fract(p)-.5;
    float n=h21(id);
    vec2 off=vec2(h21(id+1.),h21(id+2.))-.5;
    float s=glow(.05+iHigh*.05,length(g-off*.7))*step(.6-iParam2*.4,n);
    col+=s*d*(1.-smoothstep(.8,1.,d))*(1.+iBeat)*mix(vec3(.6,.8,1.),vec3(1.,.8,.5),n);
  }
  o=vec4(col,1.);
}
