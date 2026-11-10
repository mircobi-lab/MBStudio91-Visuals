// Variante dell'Oscilloscopio (idea DeepSeek, corretta): fosforo verde su reticolo 20x10.
// Param1 = ampiezza verticale, Param2 = spessore linea. Nel silenzio mostra un'onda sintetica.
float wv(float x,float silent){return mix(audioWave(x)-.5, sin(x*30.+iTime*2.)*.35, silent);}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=f/iResolution.xy;
  float silent=1.-smoothstep(0.,.02,iLevel);
  float amp=.35+iParam1*.75;                       // a fondo scala resta dentro lo schermo
  float thick=1.2+iParam2*4.;
  float H=iResolution.y, hr=thick*12.;
  float lo=(.5+mix(waveMin(uv.x)-.5,-.35,silent)*amp)*H, hi=(.5+mix(waveMax(uv.x)-.5,.35,silent)*amp)*H;
  float line=0., halo=0.;
  if(f.y>lo-hr&&f.y<hi+hr){                        // fuori dalla fascia non c'è traccia né alone
    // Distanza esatta dalla traccia: una spezzata con un vertice per ognuno dei 512 campioni dell'onda.
    // Si percorrono solo i campioni entro ±12 pixel (a 1080p sono 9, prima erano sempre 25).
    float tp=iResolution.x/512., tx=uv.x*512.-.5, t0=floor(tx);
    int n=int(min(12.,ceil(12./tp)));
    float d=1e5, yp=(.5+wv((t0-float(n)+.5)/512.,silent)*amp)*H;
    for(int j=1-n;j<=n+1;j++){
      float xt=t0+float(j), dx=(xt-tx)*tp;
      float yj=(.5+wv((xt+.5)/512.,silent)*amp)*H;
      vec2 a=vec2(dx-tp,yp-f.y), b=vec2(dx,yj-f.y), ba=b-a;
      float h=clamp(dot(-a,ba)/dot(ba,ba),0.,1.);
      d=min(d,length(-a-ba*h));
      yp=yj;
    }
    line=glow(thick,d); halo=glow(hr,d)*.5;
  }
  vec2 cell=iResolution.xy/vec2(20.,10.);
  vec2 gp=abs(fract(uv*vec2(20.,10.)+.5)-.5)*cell;
  float grid=glow(1.,min(gp.x,gp.y));
  float center=glow(1.5,abs(uv.y-.5)*iResolution.y);
  vec3 col=vec3(.2,1.,.4)*(line*1.4+halo);
  col+=vec3(.05,.14,.09)*grid;
  col+=vec3(.15,.4,.2)*center*.25;
  col*=.9+iBeat*.4;
  col*=.96+.04*step(.5,fract(f.y*.5));             // scanline netta ogni 2 px
  o=vec4(col,1.);
}
