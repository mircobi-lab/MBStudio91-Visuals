// Scie neon arrotondate in diagonale, su 3 piani di profondità (stile background "rounded neon lines").
// Param1 = velocità, Param2 = palette (fino a metà blu/viola/magenta, a destra verde/ciano)
float h1(float n){return fract(sin(n*127.1)*43758.5453);}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  vec2 p=rot2(uv,-.62);                            // le scie corrono lungo x ruotato (~35°)
  float spd=.25+iParam1*1.5;
  float pal=smoothstep(.6,.8,iParam2);
  vec3 bg=mix(vec3(.09,.03,.42),vec3(.02,.05,.16),pal);
  vec3 col=mix(bg*.35,bg,smoothstep(-.6,.6,uv.x+uv.y));
  for(int L=0;L<3;L++){
    float l=float(L);
    float sc=1.+l*.8;                              // piani lontani: più piccoli, più lenti, più tenui
    float laneH=.07/sc;
    vec2 q=p;
    float lane=floor(q.y/laneH);
    float ly=(fract(q.y/laneH)-.5)*laneH;
    float r1=h1(lane*1.7+l*31.), r2=h1(lane*3.1+l*17.), r3=h1(lane*5.3+l*7.);
    float P=(1.+r2*2.)/sc;
    float x=q.x+iTime*spd*(.5+r1)/sc+r3*P;
    float cell=floor(x/P);
    float lx=mod(x,P)-P*.5;
    float r4=h1(cell*9.7+lane*1.3+l*5.);
    if(r4<.35) continue;                           // non tutte le corsie hanno una scia
    float len=(.15+r4*.45)/sc;
    float rad=laneH*.13;
    float d=length(vec2(max(abs(lx)-len*.5,0.),ly));
    float hue=h1(lane*7.7+cell*.3+l);
    vec3 cA=mix(vec3(.25,.35,1.),vec3(1.,.2,.75),hue); cA=mix(cA,vec3(.2,.9,1.),step(.85,hue));
    vec3 cB=mix(vec3(.1,1.,.55),vec3(.15,.7,1.),hue);
    vec3 c=mix(cA,cB,pal);
    float amp=(1.-l*.3)*(.8+iHigh*.6+iBeat*.3);
    col+=c*(glow(rad,d)*1.2+glow(rad*5.,d)*.35)*amp;
  }
  o=vec4(col,1.);
}
