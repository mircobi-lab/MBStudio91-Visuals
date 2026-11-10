// 3D Tunnel Wireframe: si viaggia in un tunnel a griglia; la sezione si deforma con lo spettro.
// Calcolo esatto (la camera è sull'asse, ogni pixel guarda in una direzione fissa): nessun ciclo.
// Param1 = velocità, Param2 = quanto lo spettro deforma la sezione
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  float t=iTime;
  uv=rot2(uv,t*.08+sin(t*.31)*.25);
  float r=max(length(uv),1e-3), a=atan(uv.y,uv.x);
  float s=abs(fract(a/6.2831853+.5)*2.-1.);               // angolo speculare: niente cucitura
  float R=1.-(.15+iParam2*.4)*pow(audioSpec(.02+s*.7),1.5)-iBass*.08;
  const float FOC=.9;
  float z=R*FOC/r;                                        // profondità del punto del tunnel visto da questo pixel
  float zz=z+t*(.6+iParam1*3.);
  float fog=exp(-z*.1);
  const float NL=18.;                                     // linee lungo il tunnel
  float ga=a/6.2831853*NL, wa=px/r/6.2831853*NL;          // larghezza di un pixel, calcolata (no fwidth: niente cucitura)
  float la=abs(fract(ga+.5)-.5);
  float gz=zz*.9, wz=px*R*FOC/(r*r)*.9;
  float lz=abs(fract(gz+.5)-.5);
  float lineA=1.-smoothstep(0.,1.4*wa,la);
  float lineZ=1.-smoothstep(0.,1.4*wz,lz);
  lineA*=smoothstep(2.,.5,wa);                            // in fondo le linee si fondono: sfumo invece di sfarfallare
  lineZ*=smoothstep(2.,.5,wz);
  float ringBeat=exp(-z*.6)*iBeat;                         // gli anelli vicini lampeggiano sul beat
  vec3 col=vec3(max(lineA,lineZ*(1.+ringBeat*2.)))*fog;
  col+=vec3(.25)*exp(-r*12.)*(.4+iLevel);                  // luce in fondo
  o=vec4(col,1.);
}
