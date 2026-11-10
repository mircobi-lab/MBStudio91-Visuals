// Frattale Julia: c animato, smooth coloring
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float zoom=2.2+iParam1*2.5;
  vec2 z=uv*zoom;
  float ct=iTime*0.2;
  vec2 c=vec2(-0.4+0.35*cos(ct*1.3),0.55*sin(ct*0.9+iParam2*6.28)+iBass*0.15);
  float iter=0.;
  for(int i=0;i<80;i++){
    if(dot(z,z)>4.) break;
    z=vec2(z.x*z.x-z.y*z.y,2.*z.x*z.y)+c;
    iter+=1.;
  }
  float m=iter/80.;
  float sm=m;
  if(dot(z,z)>4.){
    float lz=0.5*log(dot(z,z));
    sm=(iter+1.-log(lz/log(2.))/log(2.))/80.;
  }
  vec3 col=rainbow(sm*9.+iTime*0.05+iBass*0.4+vec3(0.,0.25,0.5));
  col*=smoothstep(0.,0.12,m);
  col+=vec3(0.08)*m;
  o=vec4(col,1.);
}
