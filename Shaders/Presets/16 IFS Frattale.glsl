// IFS Frattale: kaliset (frattale per inversione)
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  vec3 q=vec3(uv,1.);
  float t=iTime*(0.2+iParam1*0.6);
  vec3 off=vec3(1.0+0.2*sin(t),0.5,0.25+0.2*cos(t*1.3+iParam2*6.28));
  for(int i=0;i<10;i++){
    q=abs(q)/max(dot(q,q),0.0001)-off;
  }
  float d=length(q);
  float c=pow(d*0.1,0.5);
  vec3 col=rainbow(c*0.7+iTime*0.1+iBass*0.5+vec3(0.,0.3,0.6));
  col*=1.-smoothstep(0.05,3.,d*0.1);
  col*=1.+iBeat*0.8;
  o=vec4(col,1.);
}
