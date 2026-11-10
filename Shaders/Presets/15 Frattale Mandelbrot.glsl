// Frattale Mandelbrot: zoom continuo nella Valle dei Cavallucci Marini
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float zt=fract(iTime*(0.05+iParam1*0.1));
  float zoom=pow(2.,-zt*9.+2.5);
  vec2 center=vec2(-0.743643887037151,0.13182590420533);
  vec2 c=uv*zoom+center;
  vec2 z=vec2(0.);
  float iter=0.;
  float cq=(c.x-.25)*(c.x-.25)+c.y*c.y;                   // dentro la cardioide o il bulbo: non esce mai
  if(cq*(cq+(c.x-.25))<.25*c.y*c.y||(c.x+1.)*(c.x+1.)+c.y*c.y<.0625) iter=100.;
  else for(int i=0;i<100;i++){
    if(dot(z,z)>4.) break;
    z=vec2(z.x*z.x-z.y*z.y,2.*z.x*z.y)+c;
    iter+=1.;
  }
  float m=iter/100.;
  float sm=m;
  if(dot(z,z)>4.){
    float lz=0.5*log(dot(z,z));
    sm=(iter+1.-log(lz/log(2.))/log(2.))/100.;
  }
  vec3 col=rainbow(sm*8.+iTime*0.08+iBass*0.5+vec3(0.,0.3,0.6));
  col*=smoothstep(0.,0.1,m);
  o=vec4(col,1.);
}
