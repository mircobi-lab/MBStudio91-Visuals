// Plasma morbido, i bassi spingono il colore
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=f/iResolution.y*(2.+iParam1*6.);
  float t=iTime*.7;
  float v=sin(uv.x+t)+sin(uv.y*1.3-t*1.1)+sin((uv.x+uv.y)*.7+t*.6)
         +sin(length(uv-vec2(2.,1.))*2.-t*1.7);
  v+=iBass*2.;
  vec3 col=.5+.5*cos(3.14159*v*.5+vec3(0.,2.1,4.2)+iParam2*6.28);
  col*=.6+.6*iLevel+iBeat*.4;
  o=vec4(col,1.);
}
