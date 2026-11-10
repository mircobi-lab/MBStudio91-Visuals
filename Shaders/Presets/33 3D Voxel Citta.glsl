// 3D Voxel Città: una città di colonne di cubi, alte quanto lo spettro. La storia dello spettro scorre
// verso la camera (il nuovo nasce in fondo). Spigoli dei cubi disegnati a fil di ferro.
// Attraversamento a griglia (DDA): costa poco anche a bassa risoluzione.
// Param1 = altezza dei palazzi, Param2 = larghezza della città
const int ROWS=40;
const float CN=2.;
float CAMY;
float NX;
float cellH(vec2 c){                                         // c = (colonna, fila)
  float u=.06+(abs(c.x+.5))/NX*.9;                          // dal centro (bassi) ai bordi (acuti)
  if(u>1.||c.y<CN||c.y>=CN+float(ROWS)) return 0.;
  float age=CN+float(ROWS)-1.-c.y;                           // in fondo la riga più recente
  return .04+pow(specHist(u,age),1.6)*(1.+iParam1*3.);
}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  NX=6.+floor(iParam2*12.);
  float HMAX=1.04+iParam1*3.;
  CAMY=HMAX*.75+1.1;                                         // la camera vola sempre sopra i tetti
  vec3 ro=vec3(.0,CAMY,iHistFrac);                           // la griglia avanza di una cella a ogni riga di storia
  vec3 rd=normalize(vec3(uv.x,uv.y-.42,1.15));
  vec3 col=vec3(.03+.05*smoothstep(-.1,.4,rd.y));            // cielo
  if(rd.y<0.){
    float t=max(0.,(CAMY-HMAX)/-rd.y);                       // sopra i palazzi più alti non c'è niente da colpire
    vec3 p=ro+rd*t;
    vec2 cell=floor(p.xz), st=sign(rd.xz), td=abs(1./rd.xz);
    vec2 tm=(st*(cell-p.xz)+st*.5+.5)*td+t;
    int axis=-1; float th=-1.; float hc=0.;
    for(int i=0;i<110;i++){
      hc=cellH(cell);
      float tn=min(tm.x,tm.y);
      float y0=ro.y+rd.y*t, y1=ro.y+rd.y*tn;
      if(min(y0,y1)<=hc){
        if(y0<=hc&&axis>=0) th=t;                            // colpito un lato
        else { th=(hc-ro.y)/rd.y; axis=2; }                  // colpito il tetto
        break;
      }
      if(tm.x<tm.y){ t=tm.x; tm.x+=td.x; cell.x+=st.x; axis=0; }
      else { t=tm.y; tm.y+=td.y; cell.y+=st.y; axis=1; }
      if(t>60.) break;
    }
    if(th>0.){
      vec3 h=ro+rd*th;
      vec2 fc;                                               // coordinate sulla faccia colpita
      float face;
      if(axis==2){ fc=fract(h.xz); face=.2; }
      else if(axis==0){ fc=vec2(fract(h.z),h.y); face=.11; }
      else { fc=vec2(fract(h.x),h.y); face=.07; }
      float w=th*px*1.3/1.15;                                // spessore di un pixel alla distanza colpita
      float e=min(fc.x,1.-fc.x);
      if(axis==2) e=min(e,min(fc.y,1.-fc.y));
      else e=min(e,min(abs(fc.y-hc),abs(fc.y-floor(fc.y+.5))));   // spigolo in alto e piani di un cubo
      float edge=1.-smoothstep(w*.5,w*1.8,e);
      float fog=exp(-th*.045);
      float lit=hc>.06?1.:.35;                               // terreno vuoto più spento
      col=mix(col,vec3(face+edge*.9*lit),fog);
    }
  }
  col*=1.+iBeat*.2;
  o=vec4(col,1.);
}
