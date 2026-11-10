// Sfaccettature triangolari scure con crepe luminose (stile "geometric neon triangular").
// Param1 = dimensione dei triangoli, Param2 = colore crepe (magenta → oro a metà → ciano)
// Ottimizzato: i 16 vertici intorno al pixel si calcolano una volta sola (v00..v33, riga per colonna),
// ogni lato della griglia si controlla una volta sola (33 invece di 54) e senza cicli né array.
float hh(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float seg2(vec2 p,vec2 a,vec2 b){vec2 pa=p-a,ba=b-a;vec2 q=pa-ba*clamp(dot(pa,ba)/dot(ba,ba),0.,1.);return dot(q,q);}   // distanza al quadrato
float cr(vec2 a,vec2 b,vec2 c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);}
vec2 vtx(vec2 ij,float S){return vec2(ij.x+ij.y*.5,ij.y*.8660254)*S+(vec2(hh(ij),hh(ij+7.3))-.5)*S*.55;}   // vertice spostato a caso
// triangolo (a,b,c) della cella (ci,cj), metà k: se contiene il pixel lo ricorda
#define TRI(a,b,c,ci,cj,k) {float s1=cr(a,b,p),s2=cr(b,c,p),s3=cr(c,a,p);if((s1>=0.&&s2>=0.&&s3>=0.)||(s1<=0.&&s2<=0.&&s3<=0.)){cIn=J+vec2(ci,cj);kIn=k;}}
// lato (a,b) con chiave e = somma degli indici dei due vertici: solo alcuni lati sono accesi
#define EDGE(a,b,ex,ey) {vec2 e=2.*J+vec2(ex,ey);if(hh(e*.37)>.62){float d=seg2(p,a,b);if(d<eg2){eg2=d;eB=e;}}}
void mainImage(out vec4 o, in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float t=iTime*.04;
  vec2 p=rot2(uv,t*.5)*(1.+.08*sin(t*3.))+vec2(t*.6,t*.25);   // deriva lenta della camera
  float S=.22+iParam1*.35;
  vec2 J; J.y=floor(p.y/(S*.8660254)); J.x=floor(p.x/S-J.y*.5);
  float px=1.5/iResolution.y;
  float m=max(.025,px*1.6);                        // raggio dell'alone: oltre, un lato non si vede
  vec2 v00=vtx(J+vec2(-1.,-1.),S), v01=vtx(J+vec2(0.,-1.),S), v02=vtx(J+vec2(1.,-1.),S), v03=vtx(J+vec2(2.,-1.),S);
  vec2 v10=vtx(J+vec2(-1.,0.),S), v11=vtx(J+vec2(0.,0.),S), v12=vtx(J+vec2(1.,0.),S), v13=vtx(J+vec2(2.,0.),S);
  vec2 v20=vtx(J+vec2(-1.,1.),S), v21=vtx(J+vec2(0.,1.),S), v22=vtx(J+vec2(1.,1.),S), v23=vtx(J+vec2(2.,1.),S);
  vec2 v30=vtx(J+vec2(-1.,2.),S), v31=vtx(J+vec2(0.,2.),S), v32=vtx(J+vec2(1.,2.),S), v33=vtx(J+vec2(2.,2.),S);
  // 1) in quale triangolo cade il pixel (9 celle, 2 triangoli ciascuna)
  vec2 cIn=J; float kIn=0.;
  TRI(v00,v01,v10,-1.,-1.,0.) TRI(v01,v11,v10,-1.,-1.,1.) TRI(v01,v02,v11,0.,-1.,0.) TRI(v02,v12,v11,0.,-1.,1.) TRI(v02,v03,v12,1.,-1.,0.) TRI(v03,v13,v12,1.,-1.,1.)
  TRI(v10,v11,v20,-1.,0.,0.) TRI(v11,v21,v20,-1.,0.,1.) TRI(v11,v12,v21,0.,0.,0.) TRI(v12,v22,v21,0.,0.,1.) TRI(v12,v13,v22,1.,0.,0.) TRI(v13,v23,v22,1.,0.,1.)
  TRI(v20,v21,v30,-1.,1.,0.) TRI(v21,v31,v30,-1.,1.,1.) TRI(v21,v22,v31,0.,1.,0.) TRI(v22,v32,v31,0.,1.,1.) TRI(v22,v23,v32,1.,1.,0.) TRI(v23,v33,v32,1.,1.,1.)
  float rnd=hh(cIn*3.1+kIn*11.);
  vec3 nrm=normalize(vec3(hh(cIn+kIn+.5)-.5,hh(cIn-kIn+2.)-.5,1.));
  float shade=max(dot(nrm,normalize(vec3(cos(t*2.),sin(t*2.),1.2))),0.);
  // 2) crepe: i 33 lati della griglia, ognuno una volta (orizzontali, verticali, diagonali)
  float eg2=1e9; vec2 eB=vec2(0.);
  EDGE(v00,v01,-1.,-2.) EDGE(v00,v10,-2.,-1.) EDGE(v01,v02,1.,-2.) EDGE(v01,v11,0.,-1.) EDGE(v01,v10,-1.,-1.) EDGE(v02,v03,3.,-2.) EDGE(v02,v12,2.,-1.) EDGE(v02,v11,1.,-1.) EDGE(v03,v13,4.,-1.) EDGE(v03,v12,3.,-1.)
  EDGE(v10,v11,-1.,0.) EDGE(v10,v20,-2.,1.) EDGE(v11,v12,1.,0.) EDGE(v11,v21,0.,1.) EDGE(v11,v20,-1.,1.) EDGE(v12,v13,3.,0.) EDGE(v12,v22,2.,1.) EDGE(v12,v21,1.,1.) EDGE(v13,v23,4.,1.) EDGE(v13,v22,3.,1.)
  EDGE(v20,v21,-1.,2.) EDGE(v20,v30,-2.,3.) EDGE(v21,v22,1.,2.) EDGE(v21,v31,0.,3.) EDGE(v21,v30,-1.,3.) EDGE(v22,v23,3.,2.) EDGE(v22,v32,2.,3.) EDGE(v22,v31,1.,3.) EDGE(v23,v33,4.,3.) EDGE(v23,v32,3.,3.)
  EDGE(v30,v31,-1.,4.) EDGE(v31,v32,1.,4.) EDGE(v32,v33,3.,4.)
  float eg=sqrt(eg2), eKey=hh(eB*.91);
  vec3 facet=mix(vec3(.025,.03,.045),vec3(.16,.17,.2),pow(shade,3.))*(.6+.4*rnd);
  vec3 col=facet;
  if(eg<m){                                        // solo vicino alle crepe accese
    float c2=iParam2;
    vec3 cc2=c2<.5?mix(vec3(1.,.2,.65),vec3(1.,.72,.2),c2*2.):mix(vec3(1.,.72,.2),vec3(.25,1.,.85),c2*2.-1.);
    float flick=.55+.45*sin(iTime*(1.5+eKey*3.)+eKey*30.);
    float on=flick*(.7+iBass*.6+iBeat*.5);
    col+=cc2*glow(px*1.6,eg)*1.4*on;
    col+=cc2*glow(.025,eg)*.45*on;
    if(eg<.01){
      float spark=pow(hh(floor(p*vec2(140.))+floor(iTime*8.)),20.)*(1.+iHigh*3.);
      col+=vec3(1.,.95,.8)*spark*glow(.01,eg)*on;
    }
  }
  o=vec4(col,1.);
}
