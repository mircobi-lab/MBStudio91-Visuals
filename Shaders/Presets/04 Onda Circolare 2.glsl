// Variante dell'Onda Circolare (idea DeepSeek, corretta): cerchio pieno con anelli decorativi.
// Param1 = ampiezza, Param2 = spessore linea. Nel silenzio mostra un'onda sintetica di anteprima.
float wv(float s,float silent){return mix(audioWave(s)-.5, sin(s*30.+iTime*2.)*.3*(.5+.5*sin(iTime*.7)), silent);}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float r=length(uv);
  float a=atan(uv.y,uv.x);
  float t=(a+3.14159265)/6.2831853;               // 0..1 per il colore
  float s=abs(t*2.-1.);                            // campionamento speculare: niente cucitura a sinistra
  float silent=1.-smoothstep(0.,.02,iLevel);
  float wav=wv(s,silent);
  float baseR=.32;
  float amp=min(.15+iBass*.12+iParam1*.2,.26);     // limite: l'onda non attraversa più il centro
  float waveR=baseR+wav*amp*2.;
  float d=abs(r-waveR);
  // ricerca fine solo vicino all'onda (fascia min/max locale, inclusa l'onda di anteprima)
  float lo=baseR+mix(waveMin(s)-.5,-.3,silent)*amp*2., hi=baseR+mix(waveMax(s)-.5,.3,silent)*amp*2.;
  if(r>lo-.045&&r<hi+.045){
    float s0=abs(fract(t-.016/6.2831853)*2.-1.);
    vec2 pp=(baseR+wv(s0,silent)*amp*2.)*vec2(1.,-.016);
    for(int j=1;j<=8;j++){
      float da=-.016+float(j)*.004;
      float sj=abs(fract(t+da/6.2831853)*2.-1.);
      vec2 pj=(baseR+wv(sj,silent)*amp*2.)*vec2(1.,da);
      vec2 pa=vec2(r,0.)-pp, ba=pj-pp;
      d=min(d,length(pa-ba*clamp(dot(pa,ba)/max(dot(ba,ba),1e-9),0.,1.)));
      pp=pj;
    }
  }
  float thick=.006+iParam2*.02;
  float line=glow(thick,d);
  float fill=step(r,waveR)*smoothstep(.05,max(waveR*1.2,.06),r)*.2;
  float ring1=glow(.003,abs(r-baseR*.55));
  float ring2=glow(.002,abs(r-baseR*.8))*.6;
  vec3 hue=rainbow(t+iTime*.1+wav*.4);
  vec3 col=hue*line*1.4+hue*fill;
  col+=vec3(.4,.6,1.)*(ring1+ring2)*.4;
  col+=vec3(1.,.85,.5)*glow(.04,d)*.35*iBeat;
  col+=hue*glow(.02,d)*.3;
  o=vec4(col,1.);
}
