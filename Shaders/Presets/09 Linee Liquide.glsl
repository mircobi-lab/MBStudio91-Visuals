// Linee neon su superficie liquida: isolinee di un rumore deformato (stile "abstract neon lines").
// Param1 = densità delle linee, Param2 = palette (fino a metà multicolore, a destra oro)
float fbm(vec2 p){float v=0.,a=.55;for(int i=0;i<3;i++){v+=a*noise(p);p=p*2.03+vec2(17.,9.);a*=.45;}return v;}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float t=iTime*.08;
  vec2 p=uv*1.1;
  vec2 w=vec2(fbm(p+vec2(0.,t)),fbm(p+vec2(5.2,1.3)-t));
  float fld=fbm(p+(1.6+iBass*.6)*w+vec2(t*.5,-t*.3));
  float N=9.+iParam1*14.;
  float v=fld*N;
  float px=fwidth(v);                              // linee di spessore costante in pixel
  float d=abs(fract(v)-.5)/max(px,1e-4);
  float thick=1.+2.5*noise(p*1.3+t*2.);            // spessore variabile come nell'originale
  float core=glow(thick,d), halo=glow(thick*8.,d);
  float pal=smoothstep(.6,.8,iParam2);
  float hsel=fbm(p*.7+w*.8+3.);
  float hc=fract(hsel*3.+t*.5);                     // blu → magenta → rosso → ciano, niente verde
  vec3 cM=hc<.33?mix(vec3(.15,.3,1.),vec3(1.,.15,.75),hc*3.):hc<.66?mix(vec3(1.,.15,.75),vec3(1.,.1,.3),hc*3.-1.):mix(vec3(1.,.1,.3),vec3(.25,1.,1.),hc*3.-2.);
  vec3 cG=mix(vec3(1.,.45,.05),vec3(1.,.95,.35),smoothstep(.3,.7,hsel));
  vec3 c=mix(cM,cG,pal);
  vec3 col=c*(core*1.3+halo*.35);
  col+=c*.04*smoothstep(.3,.8,fld);                 // velo di colore tra le linee
  col*=.85+iBeat*.35+iLevel*.2;
  o=vec4(col,1.);
}
