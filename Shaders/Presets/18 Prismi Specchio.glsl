// Prismi Specchio: corridoio di blocchi a fil di ferro, simmetrico, con fasci di luce che escono dal fondo.
// Pareti alta e bassa nel colore d'accento, pareti laterali chiare; i blocchi sporgono con lo spettro.
// Param1 = sporgenza dei blocchi, Param2 = colore d'accento (0.5 = rosso, 0.62 = oro, 0.0 = ciano)
const float CW=.26, L=.42;                                    // larghezza e profondità di un blocco
float blockH(vec2 c){                                        // c = (cella trasversale, cella in profondità)
  float r=hash(c);
  float s=audioSpec(.04+fract(abs(c.x)*.173+c.y*.031)*.6);
  return .05+r*r*.3+s*(.06+iParam1*.5);
}
// Attraversa una parete cella per cella (a = verso la parete, b = trasversale, D = distanza della parete).
// Restituisce z dell'impatto, x = coordinata trasversale, y = coordinata di bordo, w = 1 frontale / 0 tetto
vec4 wall(float a,float b,float D,float zoff){
  if(a<.002) return vec4(1e4,0.,0.,0.);
  float HM=.41+.06+iParam1*.5;
  float z=max(.2,(D-HM)/a);
  float j=floor((z+zoff)/L);
  for(int i=0;i<26;i++){
    float zf=j*L-zoff, zb=zf+L;
    float u=b*z+CW*.5;                                       // mezza cella: niente spigolo sull'asse dello specchio
    float h=blockH(vec2(floor(u/CW),j));
    float d=D-h;
    if(a*z>=d){                                              // colpita la faccia frontale del blocco
      float eu=abs(fract(u/CW)-.5)*CW;
      return vec4(z,CW*.5-eu,a*z-d,1.);
    }
    float zt=d/a;
    if(zt<zb){                                               // colpito il tetto del blocco
      float ut=b*zt+CW*.5;
      float eu=abs(fract(ut/CW)-.5)*CW;
      float ez=min(zt-zf,zb-zt);
      return vec4(zt,CW*.5-eu,ez,0.);
    }
    z=zb; j+=1.;
    if(z>40.) break;
  }
  return vec4(1e4,0.,0.,0.);
}
void mainImage(out vec4 o,in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  vec2 p=abs(uv);                                            // specchio orizzontale e verticale
  vec3 acc=hsv2rgb(vec3(fract(iParam2+.5),.92,1.));
  vec3 wht=vec3(.92,.94,1.);
  float zoff=iTime*(1.1+iBass*.8);
  vec4 hTop=wall(p.y,p.x,.62,zoff);
  vec4 hSide=wall(p.x,p.y,1.15,zoff+L*.5);
  bool side=hSide.x<hTop.x;
  vec4 h=side?hSide:hTop;
  vec3 tint=side?wht:acc;
  vec3 col=vec3(0.);
  if(h.x<1e3){
    float w=h.x*px*1.4;
    float edge=1.-smoothstep(w*.6,w*2.2,min(h.y,h.z));
    float fog=exp(-h.x*.16);
    float face=h.w>.5?.16:.05;
    col=tint*(face+edge*(1.1+iBeat*.6))*fog;
  }
  // fasci di luce dal fondo
  float r=length(uv);
  float an=atan(p.y,p.x);
  float ray=pow(noise(vec2(an*16.,iTime*.35)),3.)*.9+pow(noise(vec2(an*41.,-iTime*.5)),6.)*.8;
  float fall=exp(-r*3.)*(.4+iLevel*.8);
  col+=mix(wht,acc,smoothstep(.1,.5,p.y/(r+.001)))*ray*fall;
  // nebbia luminosa vicino al centro
  float neb=noise(uv*3.+vec2(iTime*.15,0.))*noise(uv*7.-vec2(0.,iTime*.2));
  col+=wht*neb*exp(-r*3.)*.6;
  col+=wht*exp(-r*14.)*(.6+iBass*.8);
  o=vec4(col,1.);
}
