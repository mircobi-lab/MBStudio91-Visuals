// Aurora: cortine di luce che danzano + stelle
float h21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float t=iTime*(0.3+iParam1*0.5);
  vec3 col=vec3(0.01,0.01,0.04)+vec3(0.,0.02,0.05)*(1.-uv.y);
  for(int i=0;i<6;i++){
    float k=float(i);
    float y=-0.2+0.5*sin(uv.x*(1.5+k*0.3)+t*1.2+k*0.5)+0.15*sin(uv.x*4.-t*0.8+k*1.7);
    y+=iParam2*0.3*sin(uv.x*2.+k);
    float d=abs(uv.y-y);
    float a=exp(-d*6.)*(0.6+0.4*sin(uv.x*2.+t+k*2.));
    vec3 c=mix(vec3(0.2,1.,0.5),vec3(0.3,0.5,1.),k/6.+iBass*0.3);
    col+=c*a*0.35;
  }
  col+=vec3(0.8,0.9,1.)*pow(h21(floor(uv*80.)),30.)*0.5;
  col*=0.8+iBeat*0.4;
  o=vec4(col,1.);
}
