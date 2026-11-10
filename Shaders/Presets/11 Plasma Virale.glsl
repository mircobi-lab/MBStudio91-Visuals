// Plasma virale: 4 strati rotanti, saturazione alta, risposta ai bassi
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float t=iTime*(0.4+iParam1*0.8);
  vec3 col=vec3(0.);
  for(int i=0;i<4;i++){
    float k=float(i);
    float a=t*0.25+k*1.5708;
    mat2 rot=mat2(cos(a),sin(a),-sin(a),cos(a));
    vec2 p=rot*uv*(1.+k*0.6);
    float v=sin(p.x*3.+t*1.8)*cos(p.y*3.-t*1.4);
    v+=sin(length(p)*4.5-t*2.2)*0.6;
    v+=sin(p.x*4.5+p.y*2.5+t*0.9+iMid*2.)*0.35;
    col+=rainbow(v*0.18+k*0.22+iBass*0.35+iParam2)*0.42;
  }
  col*=0.7+0.6*iLevel+0.5*iBeat;
  col=pow(col,vec3(0.85));
  o=vec4(col,1.);
}
