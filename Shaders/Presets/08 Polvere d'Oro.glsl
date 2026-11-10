// Polvere d'oro e bokeh (stile background "golden dust").
// Param1 = densità, Param2 = luce: fino a metà fascio dall'alto, a destra banda orizzontale
float hh(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float ar=iResolution.x/iResolution.y;
  float band=smoothstep(.6,.8,iParam2);
  vec3 gold=vec3(1.,.78,.45);
  float lTop=exp(-length((uv-vec2(0.,.55))*vec2(.9,2.6))*2.2);
  float lBand=exp(-abs(uv.y)*9.)*exp(-(uv.x+ar*.5)*1.6);
  float light=mix(lTop,lBand,band);
  float dens=mix(exp(-length((uv-vec2(0.,.35))*vec2(.8,1.4))*2.),exp(-abs(uv.y)*4.)*(1.-.4*(uv.x/ar+.5)),band);
  vec3 col=gold*light*(.55+iLevel*.4)+vec3(.01,.008,.005);
  float t=iTime*.05;
  for(int l=0;l<5;l++){
    float k=float(l);
    float big=step(3.,k);                          // gli ultimi 2 strati: bokeh grandi e sfocati
    float sc=mix(18.+k*10.,4.+k,big)*(.6+iParam1*.8);
    vec2 g=uv*sc+vec2(t*(1.+k)*sc*.3,-t*.5*sc*.2)+k*13.1;
    vec2 id=floor(g), fr=fract(g)-.5;
    float n=hh(id+k);
    if(n<1.-dens*mix(.55,.35,big)*(.5+iParam1)) continue;
    vec2 off=vec2(hh(id+k+1.),hh(id+k+2.))-.5;
    float d=length(fr-off*.6);
    float tw=.5+.5*sin(iTime*(1.+n*3.)+n*40.);
    float a=dens*(.4+.6*tw)*(1.+iHigh*.8);
    if(big<.5){
      col+=gold*glow(.06+n*.05,d)*a*1.6;             // granelli nitidi
    } else {
      float disc=1.-smoothstep(.22,.3,d);           // bokeh: disco morbido con bordo appena più chiaro
      float rim=glow(.04,abs(d-.27));
      col+=gold*(disc*.16+rim*.08)*a*(1.5-n);
    }
  }
  col*=1.+iBeat*.15;
  o=vec4(col,1.);
}
