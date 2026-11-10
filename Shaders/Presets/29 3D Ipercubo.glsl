// 3D Ipercubo: un tesseratto (cubo in 4 dimensioni) che ruota e viene proiettato in 3D e poi sullo schermo.
// 16 vertici, 32 spigoli; ogni direzione dello spazio 4D ha la sua banda di frequenza.
// Param1 = profondità della quarta dimensione, Param2 = velocità di rotazione
float sdSeg(vec2 p,vec2 a,vec2 b){vec2 pa=p-a,ba=b-a;return length(pa-ba*clamp(dot(pa,ba)/dot(ba,ba),0.,1.));}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  float t=iTime*(.15+iParam2*.6);
  float a1=t+iBass*.35, a2=t*.71, a3=t*.43;
  float wd=2.9-iParam1*1.1-iMid*.35;                      // distanza dell'osservatore nella quarta dimensione
  float sc=.5*(1.+iBeat*.06);
  vec2 P[16]; float D[16];
  for(int i=0;i<16;i++){
    vec4 v=vec4(float(i&1),float((i>>1)&1),float((i>>2)&1),float((i>>3)&1))*2.-1.;
    v.xw=rot2(v.xw,a1); v.yz=rot2(v.yz,a2); v.zw=rot2(v.zw,a3);   // rotazioni nei piani XW, YZ, ZW
    vec3 p=v.xyz/(wd-v.w);                                // 4D -> 3D
    p.xz=rot2(p.xz,t*.31);
    P[i]=p.xy*sc*(1.8/(2.6+p.z));                         // 3D -> schermo
    D[i]=p.z-v.w*.3;
  }
  vec3 col=vec3(0.);
  for(int i=0;i<16;i++){
    for(int b=0;b<4;b++){
      int j=i^(1<<b);                                     // spigolo = vertici che differiscono in una sola coordinata
      if(j<i) continue;
      float d=sdSeg(uv,P[i],P[j]);
      float br=(.3+.7*smoothstep(.6,-.6,(D[i]+D[j])*.5))*(.55+.9*pow(audioSpec(.04+float(b)*.18),1.3));
      col+=vec3(br)*(glow(px*1.5,d)+glow(px*8.,d)*.15);
    }
    col+=vec3(glow(px*3.,length(uv-P[i])))*.8;
  }
  col*=1.+iBeat*.3;
  o=vec4(col,1.);
}
