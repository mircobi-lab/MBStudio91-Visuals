// Variante del Cubo (idea DeepSeek, corretta): sezioni del cubo che arrivano verso la camera,
// ognuna con la forma d'onda in X e i vettori verticali. Param1 = velocità zoom, Param2 = zoom sull'onda.
float wv(float x,float silent){return mix(audioWave(x)-.5, sin(x*30.+iTime*2.)*.3, silent);}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float t=iTime*(.25+iParam1*.7);
  float silent=1.-smoothstep(0.,.02,iLevel);
  float win=1.-iParam2*.75;                        // porzione di onda mostrata nella sezione
  vec2 bH=vec2(1.3,.75)*(1.+iBeat*.05);            // cubo di misura fissa, pulsa sul beat
  // Le sezioni lontane sono più piccole del punto in cui cade il pixel: si visitano solo quelle che lo
  // raggiungono, dalla più vicina (al bordo dello schermo sono 2-3 invece di 48).
  vec2 au=abs(uv);
  float zMax=min(au.x>.012?bH.x/(au.x-.012):1e4, au.y>.012?bH.y/(au.y-.012):1e4);
  float u=fract(-t)*48., fu=fract(u), ku=floor(u);
  int nMax=int(clamp(floor(zMax/.35-fu),-1.,47.));
  vec3 col=vec3(0.);
  for(int n=0;n<=nMax;n++){
    float phase=(float(n)+fu)/48.;                 // 0 = camera, 1 = fondo
    float k=mod(float(n)-ku,48.);                  // numero della sezione (decide il colore)
    float z=phase*48.*.35;
    if(z<.1) continue;
    vec2 p=uv*z;
    float fade=smoothstep(0.,.2,phase)*(1.-smoothstep(.7,1.,phase));
    float inBox=step(abs(p.x),bH.x);
    float sx=.5+(p.x/bH.x)*.5*win;                 // l'onda occupa la sezione, senza salti
    float amp=min((.06+log(z+1.)*.09)*(.4+iLevel*1.6),bH.y*.9);
    float wav=wv(sx,silent);
    float waveY=amp*wav*2.;
    float d=abs(p.y-waveY)/z;
    float line=glow(.006,d)*inBox;
    float vecMask=step(0.,p.y*wav)*step(abs(p.y),abs(waveY))*inBox*.035;
    float edge=glow(.012,abs(abs(p.y)-bH.y)/z)*step(abs(p.x),bH.x)
              +glow(.012,abs(abs(p.x)-bH.x)/z)*step(abs(p.y),bH.y);
    vec3 c=rainbow(k*.04+phase*.5+iTime*.06);
    col+=c*line*fade*1.2;
    col+=c*vecMask*fade;
    col+=vec3(.35,.55,1.)*edge*fade*.35;
  }
  col=col/(1.+col*.6);                             // compressione morbida: niente bianco bruciato
  col*=.9+iBeat*.4;
  col*=1.-length(uv)*.25;
  o=vec4(col,1.);
}
