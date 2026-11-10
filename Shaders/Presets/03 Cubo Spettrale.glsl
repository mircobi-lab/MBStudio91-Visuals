// Cubo spettrale: spettrogramma 3D dentro un cubo. Lo spettro nasce sul fondo,
// scorre verso la camera sul piano e cresce in Y; il cubo scorre = zoom continuo.
// Param1 = zoom, Param2 = numero barre
// Ottimizzato: le barre si disegnano dalla più vicina e ci si ferma appena il pixel è coperto;
// per ogni pixel si visitano solo le file che possono raggiungerlo, e di linee e anelli solo il più vicino.
const float CAMY=1.2, FLOORY=-1., HW=1.6, ZF=2., DZ=.18, HMAX=1.35, CEIL=1.6;
const int ROWS=48;
float sdSeg(vec2 p,vec2 a,vec2 b){vec2 pa=p-a,ba=b-a;float h=clamp(dot(pa,ba)/dot(ba,ba),0.,1.);return length(pa-ba*h);}
vec2 pr(vec3 P,float F){return vec2(P.x,P.y-CAMY)*F/P.z;}
float ringD(vec2 q,float z,float F){                         // distanza dal contorno del cubo alla profondità z
  vec2 c=vec2(0.,(FLOORY+CEIL-2.*CAMY)*.5*F/z), h=vec2(HW,(CEIL-FLOORY)*.5)*F/z;
  vec2 d=abs(q-c)-h;
  return abs(length(max(d,0.))+min(max(d.x,d.y),0.));
}
void mainImage(out vec4 o, in vec2 f){
  vec2 p=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  float F=(.75+iParam1*.5)*(1.+.06*sin(iTime*.23))*(1.+iBeat*.03);
  vec2 q=vec2(p.x,p.y-.36);
  float ZB=ZF+float(ROWS)*DZ;
  vec3 fogC=vec3(.008,.01,.025);
  float nb=floor(16.+iParam2*48.);
  vec3 acc=vec3(0.); float T=1.;
  if(q.y<0.){
    float zmin=(CAMY-FLOORY-HMAX)*F/(-q.y), zmax=(CAMY-FLOORY)*F/(-q.y);
    int kHi=int(clamp(floor((ZB-zmin)/DZ-iHistFrac),-1.,float(ROWS-1)));
    int kLo=int(clamp(ceil((ZB-zmax-DZ*.72)/DZ-iHistFrac),0.,float(ROWS)));
    for(int k=kHi;k>=kLo;k--){                                // dalla fila più vicina alla più lontana
      float age=float(k)+iHistFrac;
      float zb=ZB-age*DZ, zf=zb-DZ*.72;
      float u=q.x*zf/F/(2.*HW)+.5;
      if(u<0.||u>1.) continue;
      float fr=fract(u*nb);
      if(fr<.12||fr>.88) continue;
      float bx=(floor(u*nb)+.5)/nb;
      float h=pow(specHist(bx,float(k)),1.5)*HMAX;
      float ybf=(FLOORY-CAMY)*F/zf, ytf=(FLOORY+h-CAMY)*F/zf, ytb=(FLOORY+h-CAMY)*F/zb;
      if(q.y<ybf||q.y>max(ytf,ytb)) continue;
      float fog=exp(-(zf-ZF)*.11);
      float vis=smoothstep(0.,1.,age)*(1.-smoothstep(float(ROWS)-6.,float(ROWS),age));
      float hn=h/HMAX;
      vec3 c=mix(vec3(0.,.75,1.),vec3(1.,.7,.2),smoothstep(.15,.7,hn));
      c=mix(c,vec3(1.,.25,.45),smoothstep(.6,1.,hn));
      vec3 face=q.y>ytf ? c*1.25 : c*(.5+.5*(q.y-ybf)/max(ytf-ybf,1e-4));
      face+=vec3(1.)*glow(px*1.5,abs(q.y-ytf))*.6;
      face*=.12+.88*smoothstep(0.,.06,hn);                   // silenzio: solo piastrelle scure
      acc+=T*vis*mix(fogC,face,fog);                          // nebbia + dissolvenza ai bordi
      T*=1.-vis;
      if(T<.004) break;
    }
  }
  vec3 col=acc;
  if(T>=.004){                                                // sfondo solo dove le barre non coprono
    vec3 bg=fogC+vec3(.02,.01,.04)*smoothstep(-.5,.5,p.y);
    vec3 gc=vec3(.15,.35,.9);
    if(abs(q.y)>1e-4){                                        // linee longitudinali: solo la più vicina
      bool fl=q.y<0.;
      float Y=fl?FLOORY:CEIL;
      float X=q.x*(Y-CAMY)/q.y;
      float x=-HW+clamp(floor((X+HW)/(HW/4.)+.5),0.,8.)*HW/4.;
      bg+=gc*glow(px*1.2,sdSeg(q,pr(vec3(x,Y,ZF),F),pr(vec3(x,Y,ZB),F)))*(fl?.35:.2);
    }
    float lp=mod(iHistHead+iHistFrac,4.);                     // anelli del cubo che scorrono: i due candidati
    float zc0=HW*F/max(abs(q.x),1e-4);
    float zc1=q.y<0.?(FLOORY-CAMY)*F/min(q.y,-1e-4):(CEIL-CAMY)*F/max(q.y,1e-4);
    float j0=clamp(floor((ZB-zc0)/(4.*DZ)-lp*.25+.5),0.,11.), j1=clamp(floor((ZB-zc1)/(4.*DZ)-lp*.25+.5),0.,11.);
    float z0=ZB-(j0*4.+lp)*DZ, z1=ZB-(j1*4.+lp)*DZ;
    bg+=gc*glow(px*1.4,ringD(q,z0,F))*.5*exp(-(z0-ZF)*.11);
    if(j1!=j0) bg+=gc*glow(px*1.4,ringD(q,z1,F))*.5*exp(-(z1-ZF)*.11);
    vec2 b0=pr(vec3(-HW,FLOORY,ZB),F), b1=pr(vec3(HW,CEIL,ZB),F);   // parete di fondo
    float inB=step(b0.x,q.x)*step(q.x,b1.x)*step(b0.y,q.y)*step(q.y,b1.y);
    bg+=vec3(1.,.5,.2)*inB*(.03+iBeat*.08);
    bg+=vec3(1.,.7,.3)*glow(px*1.6,ringD(q,ZB,F))*(.5+iBeat*.5);
    col+=T*bg;
  }
  col*=1.+iBeat*.15;
  o=vec4(col,1.);
}
