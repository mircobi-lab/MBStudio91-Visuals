// 3D Terreno Spettrale: lo spettro diventa un terreno a fil di ferro con le linee nascoste.
// Davanti c'è la riga più recente, la storia (circa 1,5 s) si allontana verso l'orizzonte.
// Bassi al centro, acuti ai lati. Param1 = altezza delle montagne, Param2 = fittezza della griglia
const int ROWS=48;
float h21(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  const float CAMY=1.6, F=1.1, Z0=1.1, DZ=.3, W=3.4;
  float HMAX=.3+iParam1*1.1;                        // sempre sotto la camera: le creste non superano l'orizzonte
  vec2 q=vec2(uv.x,uv.y-.16);
  float sp=.34-iParam2*.22;                         // passo delle linee longitudinali (in metri)
  vec3 col=vec3(0.);
  // cielo: stelle fisse e un bagliore sull'orizzonte
  vec2 sg=floor(f/3.);
  col+=vec3(step(.996,h21(sg))*.6)*smoothstep(0.,.25,q.y);
  col+=vec3(.06)*exp(-abs(q.y)*14.)*(1.+iBass);
  float yTop=(HMAX-CAMY)*F/(Z0+float(ROWS)*DZ)+px*3.;   // nessuna cresta sale oltre questa quota
  if(q.y<yTop){
    float hor=-1e9;                                 // la cresta più alta tra le righe più vicine
    for(int k=0;k<ROWS;k++){
      float z=Z0+(float(k)+iHistFrac)*DZ;
      float xw=q.x*z/F;
      float h=0.;
      if(abs(xw)<W){
        h=pow(specHist(abs(xw)/W,float(k)),1.4)*HMAX;
        if(k==0) h*=iHistFrac;                      // la riga nuova sale da terra
      }
      float yl=(h-CAMY)*F/z;
      float fog=exp(-z*.07);
      float lw=px*1.1;
      if(abs(q.y-yl)<lw*1.5&&yl>=hor-lw){           // cresta della riga
        col=vec3(glow(lw*1.5,abs(q.y-yl)))*fog*(.6+.6*h/HMAX);
        break;
      }
      if(q.y<yl){                                   // faccia del terreno davanti alla cresta: linee nascoste
        float dl=abs(fract(xw/sp+.5)-.5)*sp*F/z;    // distanza in schermo dalla linea longitudinale più vicina
        col=vec3(glow(px*1.1,dl)*.5*fog);
        break;
      }
      hor=max(hor,yl);
    }
  }
  col*=1.+iBeat*.2;
  o=vec4(col,1.);
}
