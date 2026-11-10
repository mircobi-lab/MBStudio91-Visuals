// Video, immagine o camera caricati sul deck (iChannel0)
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=f/iResolution.xy;
  vec2 c=uv-.5;
  c*=1.-iBeat*.08*(.2+iParam1);
  float sp=.004+iBass*.03*iParam2;
  vec3 col;
  col.r=texture(iChannel0,coverUV(c*(1.+sp)+.5)).r;
  col.g=texture(iChannel0,coverUV(c+.5)).g;
  col.b=texture(iChannel0,coverUV(c*(1.-sp)+.5)).b;
  if(iChannel0Ready<.5) col=vec3(.04)+vec3(.06)*step(.5,fract((uv.x+uv.y)*20.+iTime*.2));
  o=vec4(col,1.);
}
