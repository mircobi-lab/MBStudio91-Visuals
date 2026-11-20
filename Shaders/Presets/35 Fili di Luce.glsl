// Fili di Luce: fasci di fili bianchi sottili che scorrono in diagonale e si piegano, con qualche filo
// spesso e qualche anello largo che attraversa la scena. Bianco su nero, la palette lo colora.
// Param1 = densità dei fili, Param2 = curvatura
void mainImage(out vec4 o,in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  float t=iTime;
  float dens=6.+iParam1*34.;
  float curv=.15+iParam2*1.4;
  float v=0.;
  for(int k=0;k<3;k++){                                      // tre piani a scala diversa
    float fk=float(k);
    float sc=dens*(.55+fk*.35);
    vec2 q=rot2(uv,-.72+fk*.06)*sc;                          // x lungo il flusso, y attraverso
    q.y+=(noise(q*.045+vec2(t*.07,fk*7.))-.5)*curv*sc*.35;   // piega lenta di tutto il fascio
    float id=floor(q.y);
    float r=hash(vec2(id,fk));
    float fy=fract(q.y)-.5-(r-.5)*.5;
    float thick=r>.94?2.6:(r>.8?1.3:.55);                    // pochi fili spessi
    float w=thick*px*sc;                                     // spessore in pixel indipendente dalla scala
    float line=glow(w*1.2,abs(fy));
    float run=fract(q.x*.02*(.6+r)-t*(.08+r*.25)*(1.+iBass*.8)+r*9.);
    float seg=smoothstep(0.,.08,run)*smoothstep(1.,.55,run);  // tratti che scorrono
    float sp=audioSpec(.05+r*.6);
    v+=line*seg*(.35+sp*1.2+(r>.94?iBeat*.8:0.))*(1.-fk*.22);
  }
  for(int k=0;k<3;k++){                                      // anelli larghi, ellittici
    float fk=float(k);
    vec2 c=vec2(sin(t*.11+fk*2.1)*.8,cos(t*.09+fk*1.3)*.35);
    vec2 d=rot2(uv-c,.6+fk)*vec2(1.,1.9+fk*.4);
    float R=.25+.12*fk+.05*sin(t*.3+fk);
    float ring=glow(px*1.6,abs(length(d)-R));
    float arc=smoothstep(.2,.8,sin(atan(d.y,d.x)+t*(.4+fk*.2)));
    v+=ring*arc*(.5+iMid*.8);
  }
  v*=.85+iLevel*.4;
  o=vec4(vec3(min(v,1.)),1.);
}
