// 3D Solidi Wireframe: icosaedro e ottaedro in rotazione, disegnati a linee come su un monitor vettoriale.
// Ogni vertice segue una banda dello spettro e si allunga con la musica; il beat dà una spinta.
// Param1 = quanto lo spettro deforma i vertici, Param2 = velocità di rotazione
const vec3 IV[12]=vec3[12](vec3(-1,1.61803,0),vec3(1,1.61803,0),vec3(-1,-1.61803,0),vec3(1,-1.61803,0),vec3(0,-1,1.61803),vec3(0,1,1.61803),vec3(0,-1,-1.61803),vec3(0,1,-1.61803),vec3(1.61803,0,-1),vec3(1.61803,0,1),vec3(-1.61803,0,-1),vec3(-1.61803,0,1));
const ivec2 IE[30]=ivec2[30](ivec2(0,1),ivec2(0,5),ivec2(0,7),ivec2(0,10),ivec2(0,11),ivec2(1,5),ivec2(1,7),ivec2(1,8),ivec2(1,9),ivec2(2,3),ivec2(2,4),ivec2(2,6),ivec2(2,10),ivec2(2,11),ivec2(3,4),ivec2(3,6),ivec2(3,8),ivec2(3,9),ivec2(4,5),ivec2(4,9),ivec2(4,11),ivec2(5,9),ivec2(5,11),ivec2(6,7),ivec2(6,8),ivec2(6,10),ivec2(7,8),ivec2(7,10),ivec2(8,9),ivec2(10,11));
const vec3 OV[6]=vec3[6](vec3(1,0,0),vec3(-1,0,0),vec3(0,1,0),vec3(0,-1,0),vec3(0,0,1),vec3(0,0,-1));
const ivec2 OE[12]=ivec2[12](ivec2(0,2),ivec2(0,3),ivec2(0,4),ivec2(0,5),ivec2(1,2),ivec2(1,3),ivec2(1,4),ivec2(1,5),ivec2(2,4),ivec2(2,5),ivec2(3,4),ivec2(3,5));
mat3 rotX(float a){float c=cos(a),s=sin(a);return mat3(1.,0.,0.,0.,c,s,0.,-s,c);}
mat3 rotY(float a){float c=cos(a),s=sin(a);return mat3(c,0.,-s,0.,1.,0.,s,0.,c);}
float sdSeg(vec2 p,vec2 a,vec2 b){vec2 pa=p-a,ba=b-a;return length(pa-ba*clamp(dot(pa,ba)/dot(ba,ba),0.,1.));}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  float t=iTime*(.12+iParam2*.6);
  mat3 R1=rotY(t+iBass*.25)*rotX(t*.71);
  mat3 R2=rotX(-t*1.3)*rotY(-t*.83);
  float defo=.1+iParam1*.7, sc=.42*(1.+iBeat*.07);
  vec2 P[12]; float Z[12];
  for(int i=0;i<12;i++){                                       // icosaedro: proiezione prospettica dei vertici
    vec3 v=normalize(IV[i])*(1.+defo*pow(audioSpec(.03+float(i)*.075),1.6));
    v=R1*v*sc;
    P[i]=v.xy*(1.7/(2.4+v.z)); Z[i]=v.z;
  }
  vec2 Q[6]; float W[6];
  for(int i=0;i<6;i++){                                        // ottaedro interno, gira al contrario
    vec3 v=R2*OV[i]*sc*(.5+.25*iMid);
    Q[i]=v.xy*(1.7/(2.4+v.z)); W[i]=v.z;
  }
  vec3 col=vec3(0.);
  for(int e=0;e<30;e++){
    ivec2 k=IE[e];
    float d=sdSeg(uv,P[k.x],P[k.y]);
    float br=.35+.65*smoothstep(.45,-.45,(Z[k.x]+Z[k.y])*.5);  // i lati davanti sono più luminosi
    col+=vec3(br)*(glow(px*1.6,d)+glow(px*9.,d)*.18);
  }
  for(int e=0;e<12;e++){
    ivec2 k=OE[e];
    float d=sdSeg(uv,Q[k.x],Q[k.y]);
    float br=.3+.5*smoothstep(.3,-.3,(W[k.x]+W[k.y])*.5);
    col+=vec3(br)*(glow(px*1.3,d)+glow(px*6.,d)*.12)*(.6+.8*iHigh);
  }
  for(int i=0;i<12;i++) col+=vec3(glow(px*3.5,length(uv-P[i])))*(.5+.5*smoothstep(.45,-.45,Z[i]));   // vertici
  col+=vec3(.05)*glow(.9,length(uv))*(.4+iLevel);                  // alone centrale appena accennato
  col*=1.+iBeat*.25;
  o=vec4(col,1.);
}
