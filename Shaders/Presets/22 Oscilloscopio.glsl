// Forma d'onda (Param1 = ampiezza, Param2 = spessore)
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=f/iResolution.xy;
  float g=1.+iParam1*4.;
  float y=.5+(audioWave(uv.x)-.5)*g;
  float d=abs(uv.y-y);
  float th=.002+iParam2*.012;
  vec3 col=vec3(.35,1.,.75)*glow(th*6.,d)*.5+vec3(1.)*glow(th,d);
  col+=vec3(.02,.03,.04)*step(.5,fract(f.y*.5));
  col+=iBeat*.06;
  o=vec4(col,1.);
}
