// Oscilloscopio con trigger + barre di frequenza con peak-hold
// Param1 = guadagno onda, Param2 = numero barre
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=f/iResolution.xy;
  vec3 col=vec3(.005,.012,.01);
  vec2 cell=iResolution.xy/vec2(10.,8.);
  vec2 gp=abs(fract(uv*vec2(10.,8.)+.5)-.5)*cell;            // distanza in pixel dal reticolo
  col+=vec3(.05,.14,.1)*glow(1.2,min(gp.x,gp.y));
  float n=floor(24.+iParam2*104.);
  float id=floor(uv.x*n), x=(id+.5)/n;
  float slot=step(.18,fract(uv.x*n));
  float h=pow(audioSpec(x),1.4)*.42;
  vec3 cb=mix(vec3(.05,.9,.55),vec3(1.,.85,.2),clamp(uv.y/.42,0.,1.));
  cb=mix(cb,vec3(1.,.25,.3),smoothstep(.3,.42,uv.y));
  col+=cb*step(uv.y,h)*slot*.85;
  col+=vec3(.05,.6,.4)*step(1.-h*.5,uv.y)*slot*.12;          // riflesso in alto
  float pk=pow(audioPeak(x),1.4)*.42;                        // picchi calcolati sulla CPU (1 lettura invece di 20)
  col+=vec3(1.,.95,.8)*step(abs(uv.y-pk),2./iResolution.y)*slot*step(.01,pk)*.9;
  float gain=.1+iParam1*.45;
  float H=iResolution.y;
  float lo=(.64+(waveMin(uv.x)-.5)*2.*gain)*H, hi=(.64+(waveMax(uv.x)-.5)*2.*gain)*H;   // fascia min/max locale
  if(f.y>lo-12.&&f.y<hi+12.){                                // fuori dalla fascia non c'è traccia né alone
    // Distanza esatta dalla traccia: la forma d'onda ha 512 campioni, quindi è una spezzata con un vertice
    // per campione. Si percorrono solo i campioni entro ±12 pixel (a 1080p sono 9, prima erano sempre 25).
    float tp=iResolution.x/512., tx=uv.x*512.-.5, t0=floor(tx);
    int n=int(min(12.,ceil(12./tp)));
    float d=1e5, yp=(.64+(audioWave((t0-float(n)+.5)/512.)-.5)*2.*gain)*H;
    for(int j=1-n;j<=n+1;j++){
      float xt=t0+float(j), dx=(xt-tx)*tp;
      float yj=(.64+(audioWave((xt+.5)/512.)-.5)*2.*gain)*H;
      vec2 a=vec2(dx-tp,yp-f.y), b=vec2(dx,yj-f.y), ba=b-a;
      float t=clamp(dot(-a,ba)/dot(ba,ba),0.,1.);
      d=min(d,length(-a-ba*t));
      yp=yj;
    }
    col+=vec3(.35,1.,.7)*glow(12.,d)*.3+vec3(.85,1.,.9)*glow(1.8,d);
  }
  col*=1.+iBeat*.2;
  col*=.94+.06*step(.5,fract(f.y*.5));
  o=vec4(col,1.);
}
