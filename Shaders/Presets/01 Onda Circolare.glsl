// Onda circolare: forma d'onda in ascolto in coordinate polari + raggi di spettro
// Param1 = ampiezza onda, Param2 = rotazione (0.5 = ferma)
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float r=length(uv);
  float a=atan(uv.y,uv.x)+iTime*(iParam2-.5)*1.5;
  float s=abs(fract(a/6.2831853+.5)*2.-1.);          // 0..1 speculare: nessuna cucitura
  float base=.26+iBass*.04+iBeat*.025;
  float amp=.06+iParam1*.22;
  float w=(audioWave(s)-.5)*2.;
  float rw=base+w*amp;
  float d=abs(r-rw);                                         // distanza radiale (sovrastima)
  // ricerca fine solo vicino all'onda: fuori dalla fascia min/max locale non c'è nulla da disegnare
  float lo=base+(waveMin(s)-.5)*2.*amp, hi=base+(waveMax(s)-.5)*2.*amp;
  if(r>lo-.04&&r<hi+.04){
    float s0=abs(fract((a-.018)/6.2831853+.5)*2.-1.);
    vec2 pp=(base+(audioWave(s0)-.5)*2.*amp)*vec2(1.,-.018);     // angolo piccolo: cos≈1, sin≈angolo
    for(int j=1;j<=8;j++){
      float da=-.018+float(j)*.0045;
      float sj=abs(fract((a+da)/6.2831853+.5)*2.-1.);
      vec2 pj=(base+(audioWave(sj)-.5)*2.*amp)*vec2(1.,da);
      vec2 pa=vec2(r,0.)-pp, ba=pj-pp;                           // distanza dal segmento tra due campioni
      d=min(d,length(pa-ba*clamp(dot(pa,ba)/max(dot(ba,ba),1e-9),0.,1.)));
      pp=pj;
    }
  }
  vec3 cw=rainbow(s*.5+iTime*.05+iMid*.3);
  vec3 col=cw*glow(.0045,d)+cw*glow(.035,d)*.3;
  float rw2=base*.62-w*amp*.45;                              // eco interna speculare
  col+=vec3(.55,.8,1.)*glow(.003,abs(r-rw2))*.45;
  float n=96., id=floor(s*n), sx=(id+.5)/n;
  float h=pow(audioSpec(sx),1.6);
  float r0=base+amp+.03, r1=r0+h*.26;
  float gap=step(.25,fract(s*n));
  float bar=step(r0,r)*step(r,r1)*gap;
  vec3 cb=mix(vec3(.1,.6,1.),vec3(1.,.3,.5),clamp((r-r0)/.26,0.,1.));
  col+=cb*bar*(.55+.45*h);
  col+=vec3(1.,.85,.5)*glow(.004,abs(r-r1))*gap*step(.02,h);
  col+=vec3(1.,.6,.3)*glow(.12,r)*iBeat*.5;
  col*=.85+.3*iLevel;
  o=vec4(col,1.);
}
