// 3D Toro Raymarch: un toro a sezione quadrata che si attorciglia e respira con lo spettro.
// Ombreggiatura in scala di grigi con contorno chiaro. Param1 = attorcigliamento, Param2 = deformazione audio
float audioAt(float a){return pow(audioSpec(.02+abs(fract(a/6.2831853+.5)*2.-1.)*.6),1.4);}
float map(vec3 p){
  p.xz=rot2(p.xz,iTime*.31); p.xy=rot2(p.xy,iTime*.19);
  float a=atan(p.z,p.x);
  vec2 q=vec2(length(p.xz)-1.15,p.y);
  q=rot2(q,a*(1.+floor(iParam1*4.))+iTime*.7);             // sezione che gira lungo il toro
  float r=.28+(.05+iParam2*.25)*audioAt(a)+iBeat*.04;
  vec2 d=abs(q)-vec2(r,r*.6);
  return (length(max(d,0.))+min(max(d.x,d.y),0.)-.03)*.7;  // .7: la torsione deforma la distanza, si avanza piano
}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  vec3 ro=vec3(0.,0.,4.6), rd=normalize(vec3(uv,-1.6));
  float t=0., dmin=1e9, tmin=0.; bool hit=false;
  for(int i=0;i<64;i++){
    float d=map(ro+rd*t);
    if(d<dmin){dmin=d;tmin=t;}
    if(d<.002){hit=true;break;}
    t+=d; if(t>8.) break;
  }
  float bg=.04+.04*(1.-length(uv));
  vec3 col=vec3(bg);
  if(hit){
    vec3 p=ro+rd*t;
    const vec2 k=vec2(1.,-1.)*.0015;
    vec3 n=normalize(k.xyy*map(p+k.xyy)+k.yyx*map(p+k.yyx)+k.yxy*map(p+k.yxy)+k.xxx*map(p+k.xxx));
    vec3 L=normalize(vec3(.5,.8,.6));
    float dif=max(dot(n,L),0.), rim=pow(1.-max(dot(n,-rd),0.),3.);
    float spe=pow(max(dot(reflect(rd,n),L),0.),24.);
    col=vec3(.07+.55*dif+.6*spe+.5*rim*(.5+iHigh));
  } else {
    col+=vec3(.7)*exp(-dmin*60.)*step(tmin,7.9);            // contorno luminoso dove il raggio sfiora il toro
  }
  col*=1.+iBeat*.2;
  o=vec4(col,1.);
}
