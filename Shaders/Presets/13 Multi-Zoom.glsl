// Multi-Zoom: 5 zoom simultanei a scale diverse (dimostra zoomRot)
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float t=iTime*(0.3+iParam1*0.8);
  vec3 col=vec3(0.);
  float r0=length(uv), a=atan(uv.y,uv.x);                 // l'angolo non dipende dallo zoom: una volta sola
  float ang=sin(a*(6.+floor(iParam2*10.))+t)*0.5+0.5;
  for(int i=0;i<5;i++){
    float k=float(i);
    float zoom=exp(fract(t*0.5+k*0.2)*2.5);
    float r=r0*zoom;
    float pat=sin(r*6.-t*3.+k*2.)*0.5+0.5;
    pat*=ang;
    pat*=exp(-r*0.5);
    col+=rainbow(k*0.2+t*0.1+iMid*0.4)*pat*0.6;
  }
  col*=1.+iBeat*0.5;
  o=vec4(col,1.);
}
