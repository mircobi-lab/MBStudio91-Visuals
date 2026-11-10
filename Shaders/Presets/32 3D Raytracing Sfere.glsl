// 3D Raytracing Sfere: tre sfere su un pavimento a scacchi, con ombre nette e un rimbalzo di riflesso.
// La grande salta sul beat, la media sugli ottavi, la piccola segue gli acuti. In scala di grigi:
// dà il meglio con Pixel 3-6 e le palette 1-bit, grigi o monocolore.
// Param1 = forza dei riflessi, Param2 = velocità della camera
vec4 S[3];
float sph(vec3 ro,vec3 rd,vec4 s){vec3 oc=ro-s.xyz;float b=dot(oc,rd),c=dot(oc,oc)-s.w*s.w,h=b*b-c;if(h<0.)return -1.;return -b-sqrt(h);}
float trace(vec3 ro,vec3 rd,out int id){
  float t=1e9; id=-1;
  for(int i=0;i<3;i++){float tt=sph(ro,rd,S[i]);if(tt>1e-3&&tt<t){t=tt;id=i;}}
  if(rd.y<-1e-4){float tt=-ro.y/rd.y;if(tt>1e-3&&tt<t){t=tt;id=3;}}
  return t;
}
float shadow(vec3 p,vec3 l){for(int i=0;i<3;i++) if(sph(p,l,S[i])>1e-3) return .25; return 1.;}
float shade(vec3 p,int id,vec3 rd,vec3 L,out vec3 n,out float refl){
  if(id==3){
    n=vec3(0.,1.,0.);
    float ch=mod(floor(p.x)+floor(p.z),2.);
    float a=mix(.18,.75,ch); refl=.25;
    float dif=max(dot(n,L),0.)*shadow(p+n*1e-3,L);
    return a*(.25+.75*dif)*exp(-length(p.xz)*.05);
  }
  n=normalize(p-S[id].xyz); refl=.55;
  float dif=max(dot(n,L),0.)*shadow(p+n*1e-3,L);
  float spe=pow(max(dot(reflect(rd,n),L),0.),40.);
  return .08+.62*dif+.9*spe*step(.5,dif);
}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float ph=iPhase, ph2=fract(iPhase*2.);
  S[0]=vec4(cos(iTime*.4)*1.7,0.,sin(iTime*.4)*1.7,.75+.25*iBass);
  S[1]=vec4(cos(iTime*.4+2.1)*1.7,0.,sin(iTime*.4+2.1)*1.7,.5+.25*iMid);
  S[2]=vec4(cos(iTime*.4+4.2)*1.7,0.,sin(iTime*.4+4.2)*1.7,.35+.25*iHigh);
  S[0].y=S[0].w+1.3*4.*ph*(1.-ph);                         // parabola: tocca terra esattamente sul beat
  S[1].y=S[1].w+.7*4.*ph2*(1.-ph2);
  S[2].y=S[2].w+.4+.6*iHigh;
  float ca=iTime*(.05+iParam2*.4);
  vec3 ro=vec3(sin(ca)*6.,2.3+.5*sin(ca*.7),cos(ca)*6.);
  vec3 ta=vec3(0.,.9,0.);
  vec3 ww=normalize(ta-ro), uu=normalize(cross(ww,vec3(0.,1.,0.))), vv=cross(uu,ww);
  vec3 rd=normalize(uv.x*uu+uv.y*vv+1.6*ww);
  vec3 L=normalize(vec3(.6,.9,.3));
  float sky=.08+.25*smoothstep(-.1,.6,rd.y);
  int id; float t=trace(ro,rd,id);
  float c=sky;
  if(id>=0){
    vec3 p=ro+rd*t, n; float rf;
    c=shade(p,id,rd,L,n,rf);
    vec3 rd2=reflect(rd,n);                                 // un rimbalzo di riflesso
    int id2; float t2=trace(p+n*1e-3,rd2,id2);
    float c2=.08+.25*smoothstep(-.1,.6,rd2.y);
    if(id2>=0){vec3 n2; float rf2; c2=shade(p+n*1e-3+rd2*t2,id2,rd2,L,n2,rf2);}
    c=mix(c,c2,rf*(.2+.8*iParam1));
    c=mix(sky,c,exp(-t*.025));
  }
  c*=1.+iBeat*.15;
  o=vec4(vec3(c),1.);
}
