// Gocce che si fondono (Param1 = dimensione, Param2 = colore)
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float v=0.;
  for(int i=0;i<6;i++){
    float k=float(i);
    vec2 c=.45*vec2(sin(iTime*(.4+k*.13)+k*2.),cos(iTime*(.5+k*.11)+k));
    vec2 d=uv-c;
    v+=(.01+.02*iBass+iParam1*.02)/dot(d,d);
  }
  float e=smoothstep(.95,1.05,v);
  vec3 col=rainbow(v*.05+iTime*.05+iParam2)*e+vec3(.9,.6,.3)*min(v*.04,1.)*(.3+iBeat*.7);
  o=vec4(col,1.);
}
