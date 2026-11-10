// Neon Grid: synthwave, griglia al neon e sole al tramonto
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=f/iResolution.xy;
  float t=iTime;
  float horizon=0.45;
  vec3 sky=mix(vec3(0.02,0.01,0.1),vec3(0.8,0.15,0.4),pow(uv.y,1.3));
  vec2 sc=vec2(0.5,horizon+0.05);
  float sd=length((uv-sc)*vec2(1.,1.6));
  float sun=smoothstep(0.28,0.27,sd);
  float scan=step(0.5,fract((uv.y-sc.y)*80.));
  sky+=sun*vec3(1.,0.6,0.2)*(0.7+0.3*scan)*smoothstep(0.3,0.25,sd);
  float gz=1./(horizon-uv.y+0.01);
  vec2 gp=vec2((uv.x-0.5)*gz,(gz+t*2.));
  vec2 g=abs(fract(gp)-0.5);
  float line=step(0.46,max(g.x,g.y))*step(uv.y,horizon);
  sky+=vec3(0.,1.,1.)*line*0.9;
  sky+=vec3(1.,0.2,0.6)*exp(-abs(uv.y-horizon)*30.);
  sky*=0.8+iBeat*0.5;
  o=vec4(sky,1.);
}
