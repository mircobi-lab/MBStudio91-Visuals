// Città dall'Alto: volo lento sopra una città notturna di grattacieli, vista quasi dall'alto.
// Facciate grigio-blu con finestre accese: più musica c'è nella banda del palazzo, più finestre si accendono.
// Attraversamento a griglia (DDA) con un palazzo per isolato e le strade in mezzo.
// Param1 = altezza dei palazzi, Param2 = quantità di finestre accese
const float INS=.16;                                         // metà larghezza della strada, in isolati
float bH(vec2 c){float r=hash(c);return .6+r*r*(2.+iParam1*7.)+step(.93,r)*3.;}
void mainImage(out vec4 o,in vec2 f){
  vec2 uv=(f-.5*iResolution.xy)/iResolution.y;
  float px=1./iResolution.y;
  float t=iTime*.35;
  vec3 ro=vec3(t*.6+sin(t*.2)*2.,14.,t*1.4);
  vec3 rd=normalize(vec3(uv.x,uv.y*.9-1.15,1.));
  rd.xz=rot2(rd.xz,.35+sin(t*.07)*.15);
  vec3 col=vec3(.02,.025,.035);
  float tg=(ro.y-12.)/-rd.y;                                 // sopra i palazzi più alti non c'è niente
  vec3 p=ro+rd*max(tg,0.);
  vec2 cell=floor(p.xz), st=sign(rd.xz), td=abs(1./rd.xz);
  vec2 tm=(st*(cell-p.xz)+st*.5+.5)*td+max(tg,0.);
  float hitT=-1.; vec3 n=vec3(0.); vec2 hc=vec2(0.); float hh=0.;
  for(int i=0;i<64;i++){
    float h=bH(cell);
    vec3 bmin=vec3(cell.x+INS,0.,cell.y+INS), bmax=vec3(cell.x+1.-INS,h,cell.y+1.-INS);
    vec3 t0=(bmin-ro)/rd, t1=(bmax-ro)/rd;
    vec3 tn=min(t0,t1), tf=max(t0,t1);
    float a=max(max(tn.x,tn.y),tn.z), b=min(min(tf.x,tf.y),tf.z);
    if(b>a&&b>0.){
      hitT=a; hc=cell; hh=h;
      n=a==tn.y?vec3(0.,1.,0.):(a==tn.x?vec3(-st.x,0.,0.):vec3(0.,0.,-st.y));
      break;
    }
    float tnext=min(tm.x,tm.y);
    if(ro.y+rd.y*tnext<0.){ hitT=-ro.y/rd.y; n=vec3(0.,1.,0.); hc=vec2(-1.); break; }  // strada
    if(tm.x<tm.y){ tm.x+=td.x; cell.x+=st.x; } else { tm.y+=td.y; cell.y+=st.y; }
  }
  if(hitT>0.){
    vec3 h=ro+rd*hitT;
    float fog=exp(-hitT*.035);
    vec3 c;
    if(hc.x==-1.&&hc.y==-1.){                                // strada con qualche luce che corre
      vec2 g=fract(h.xz);
      float lane=glow(.02,abs(min(g.x,g.y)-.03));
      c=vec3(.015,.018,.025)+vec3(1.,.55,.2)*lane*step(.7,hash(floor(h.xz*3.)+floor(iTime*2.)))*.6;
    } else if(n.y>.5){                                       // tetto
      vec2 g=fract(h.xz)-.5;
      float rim=glow(px*hitT*1.5,abs(max(abs(g.x),abs(g.y))-(.5-INS)));
      c=vec3(.11,.12,.14)+vec3(.25)*rim;
      c+=vec3(1.,.15,.1)*glow(.03,length(g))*iBeat*step(.8,hash(hc+.3));  // luce d'ostacolo sui più alti
    } else {                                                 // facciata con finestre
      float u=n.x!=0.?h.z:h.x;
      vec2 w=vec2(u*9.,h.y*6.);
      vec2 wf=fract(w), wi=floor(w);
      float win=step(.22,wf.x)*step(wf.x,.78)*step(.25,wf.y)*step(wf.y,.8);
      float band=audioSpec(.04+hash(hc)*.6);
      float on=step(hash(wi+hc*17.),.12+iParam2*.35+band*.45);
      vec3 warm=mix(vec3(1.,.62,.3),vec3(.75,.85,1.),step(.7,hash(wi.yx+hc)));
      float shade=n.x!=0.?.75:1.;
      c=vec3(.05,.06,.075)*shade*(.6+.4*h.y/hh)+warm*win*on*(.55+band*.8);
    }
    col=mix(col,c,fog);
  }
  col+=vec3(.02,.03,.05)*(1.-exp(-max(hitT,0.)*.02));       // foschia notturna
  o=vec4(col*(1.+iBeat*.15),1.);
}
