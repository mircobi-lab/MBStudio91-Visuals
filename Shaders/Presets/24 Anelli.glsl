// Un anello nasce su ogni battito della battuta
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float r=length(uv);
  vec3 col=vec3(0.);
  for(int i=0;i<4;i++){
    float k=float(i);
    float age=fract(iBar-k*.25);
    float rad=age*(.9+iParam1);
    float w=.01+iParam2*.05;
    col+=glow(w,abs(r-rad))*pow(1.-age,1.5)*(.5+.5*cos(vec3(0.,2.,4.)+k*1.3+iTime*.2));
  }
  col+=glow(.15+iBass*.15,r)*vec3(1.,.6,.3)*iBeat;
  o=vec4(col,1.);
}
