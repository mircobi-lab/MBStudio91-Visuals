// Mosaico Barre: colonne di barre arrotondate a colori, ogni colonna scorre a una velocità sua.
// Le colonne si accendono con la loro banda di spettro; sul battito alcune barre cambiano colore.
// Param1 = numero di colonne, Param2 = velocità di scorrimento
float rbox(vec2 p,vec2 h,float r){vec2 d=abs(p)-h+r;return length(max(d,0.))+min(max(d.x,d.y),0.)-r;}
void mainImage(out vec4 o,in vec2 f){
  vec2 uv=f/iResolution.y;
  float px=1./iResolution.y;
  float N=4.+floor(iParam1*14.);                             // colonne per altezza dello schermo
  float cx=floor(uv.x*N);
  float fx=fract(uv.x*N)-.5;
  float rc=hash(vec2(cx,3.1));
  float spd=(rc-.5)*2.*(.15+iParam2*1.6)+sign(rc-.5)*.1;
  const float HC=2.3;                                        // altezza di una cella in larghezze di colonna
  float y=uv.y*N/HC+iTime*spd*(1.+iBass*.6)+rc*20.;
  float cy=floor(y);
  float fy=(fract(y)-.5)*HC;
  float rb=hash(vec2(cx,cy));
  float d=rbox(vec2(fx,fy),vec2(.39,HC*(.33+rb*.15)),.07);   // barre di lunghezze diverse con fessure
  float sz=px*N;
  float fill=1.-smoothstep(-sz,sz,d);
  float lip=1.-smoothstep(0.,sz*2.,abs(d+sz*2.));            // bordo più chiaro
  float beatSwap=step(.82,hash(vec2(cx,cy+floor(iTime*iBPM/60.))))*iBeat;
  vec3 c=hsv2rgb(vec3(fract(rb*.83+rc*.31+beatSwap*.37),.3+.42*hash(vec2(cy,cx)),.42+.5*hash(vec2(cx*1.7,cy))));
  float sp=audioSpec(.03+fract(cx/N*.37)*.7);
  float lit=.45+sp*1.;
  vec3 col=c*fill*lit+vec3(1.)*lip*fill*.18*sp;
  col+=c*glow(sz*10.,max(d,0.))*.12*sp;                       // alone leggero
  o=vec4(col,1.);
}
