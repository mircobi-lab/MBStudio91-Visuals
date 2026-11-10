// Zoom infinito: 6 strati a fasi diverse, anelli e razzi
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float spd=0.5+iParam1*1.5;
  vec3 col=vec3(0.);
  float r0=length(uv), a=atan(uv.y,uv.x);                 // l'angolo non dipende dallo zoom: una volta sola
  float spokes=0.5+0.5*sin(a*(5.+floor(iParam2*12.))+iTime*0.7);
  for(int i=0;i<6;i++){
    float k=float(i);
    float z=exp(mod(iTime*spd+k/5.,1.)*3.2);
    float r=r0*z;
    float rings=pow(1.-abs(fract(r*2.-iTime*1.5-k/5.)-0.5)*2.,6.);
    float d=rings*spokes*exp(-r*0.35);
    vec3 c=rainbow(fract(iTime*0.08+k*0.15)+iBass*0.5+vec3(0.,0.3,0.6));
    col+=c*d*0.75;
  }
  col*=0.9+iBeat*0.8;
  o=vec4(col,1.);
}
