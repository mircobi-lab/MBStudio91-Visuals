// Barre di spettro (Param1 = numero barre, Param2 = guadagno)
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=f/iResolution.xy;
  float n=floor(16.+iParam1*112.);
  float id=floor(uv.x*n);
  float x=(id+.5)/n;
  float h=audioSpec(x)*(.8+iParam2*1.5);
  float bar=step(uv.y,h)*step(.15,fract(uv.x*n));
  vec3 col=mix(vec3(.1,.6,1.),vec3(1.,.3,.5),uv.y)*bar;
  col+=glow(.01,abs(uv.y-h))*vec3(1.,.85,.4);
  col+=iBeat*.15*vec3(1.,.7,.3)*(1.-uv.y);
  o=vec4(col,1.);
}
