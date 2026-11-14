// Small DOM harness for the actual frontend: node tests/test_position_ui.cjs
const fs=require('fs'),path=require('path'),vm=require('vm'),assert=require('assert');
const root=path.resolve(__dirname,'..'), nodes=new Map(),docEvents={},winEvents={},sent=[],intervals=[];
class Node {
  constructor(tag='div'){this.tagName=tag;this.children=[];this.listeners={};this.hidden=false;this.disabled=false;this.value='';this.classList={toggle(){}};this.style={setProperty(k,v){this[k]=v;}};}
  set id(v){this._id=v;nodes.set(v,this);} get id(){return this._id;}
  append(...items){for(const x of items){if(x && typeof x==='object')x.parentElement=this;this.children.push(x);}}
  replaceChildren(...items){this.children=[];this.append(...items);}
  setAttribute(k,v){this[k]=v;}
  addEventListener(k,fn){this.listeners[k]=fn;}
  show(){this.open=true;} showModal(){this.open=true;} close(){this.open=false;}
  click(){assert(!this.disabled);this.listeners.click?.({preventDefault(){}});}
}
function node(id){const n=new Node();n.id=id;return n;}
const html=fs.readFileSync(path.join(root,'web/html/04_service.html'),'utf8');
for(const [,id] of html.matchAll(/id="([^"]+)"/g))node(id);
node('cfg_lang').value='0';node('cfg_position_enable');
for(let i=0;i<16;i++){const n=node('p01_jaro_ch_name_'+i);n.parentElement=new Node();}
nodes.get('p04_position_dialog').parentElement=new Node();nodes.get('p04_service_setup').clientWidth=1000;
const select=nodes.get('p04_selected_shutter');select.value='0';select.selectedOptions=[{textContent:'Wohnzimmer'}];
const context=vm.createContext({console,crypto:require('crypto').webcrypto,ws:{readyState:1},WebSocket:{OPEN:1},getComputedStyle:()=>({display:'block'}),sendData:(id,data)=>sent.push({id,...JSON.parse(data)}),setInterval:fn=>intervals.push(fn),document:{getElementById:id=>nodes.get(id),createElement:tag=>new Node(tag),addEventListener:(k,fn)=>docEvents[k]=fn},window:{addEventListener:(k,fn)=>winEvents[k]=fn}});
vm.runInContext(fs.readFileSync(path.join(root,'web/js/lang.js'),'utf8'),context);
vm.runInContext(fs.readFileSync(path.join(root,'web/js/position.js'),'utf8'),context);
docEvents.DOMContentLoaded();winEvents['webui:connected']();assert(sent.at(-1).a==='status');
let snapshot={enabled:true,notice:'',channels:Array.from({length:16},()=>[false,-1]),moving:-1,phase:0,target:25,cal:{phase:0,channel:0,mark:0,token:0,elapsed:0,times:[2400,15800,28600,12200,26400,29800]}};
const update=()=>winEvents['webui:message']({detail:{type:'shutterPosition',...snapshot}});
update();const box=nodes.get('p01_jaro_ch_name_0').parentElement.children.at(-1);assert(!box.hidden && box.children.find(n=>n.type==='range').hidden);assert(box.children.find(n=>n.className==='position-actions').hidden);assert(box.children.find(n=>n.className==='position-label').hidden);assert(!box.children.at(-1).hidden && box.children.at(-1).textContent==='Zur Kalibrierung');
snapshot.channels[0]=[true,-1];update();assert(!box.children.find(n=>n.type==='range').hidden);assert(!box.children.find(n=>n.className==='position-actions').hidden);assert(box.children.at(-1).hidden);
const status=box.children.find(n=>n.className==='position-channel-status');
assert(!status.hidden);assert(status.children[0].children[1].textContent==='Position unbekannt');
snapshot.channels[0]=[true,45];update();assert(status.children[0].children[1].textContent==='45 %');
snapshot.moving=0;snapshot.phase=1;update();assert(status.children[1].children[1].hidden);assert(status.children[0].children[1].textContent.includes('Referenzfahrt'));
snapshot.moving=-1;snapshot.channels[0]=[false,-1];update();assert(status.hidden);
snapshot.channels[0]=[true,45];snapshot.enabled=false;update();assert(status.hidden);
snapshot.enabled=true;update();assert(!status.hidden);
const targetButton=box.children.find(n=>n.className==='position-actions').children[1];assert(targetButton['aria-label']);
const slider=box.children.find(n=>n.type==='range');const beforeInput=sent.length;slider.value='65';slider.listeners.input();assert(sent.length===beforeInput);slider.value='50';slider.listeners.input();assert(sent.length===beforeInput);
targetButton.click();assert(sent.at(-1).a==='target' && sent.at(-1).v===50);
nodes.get('p04_position_begin').click();assert(nodes.get('p04_position_dialog').open);assert(nodes.get('p04_position_intro').hidden);assert(nodes.get('p04_position_inline').children.includes(nodes.get('p04_position_dialog')));
nodes.get('p04_position_next').click();assert(sent.at(-1).a==='begin');const token=sent.at(-1).token;assert(token>0);
snapshot.cal={...snapshot.cal,phase:1,token};update();nodes.get('p04_position_next').click();assert(sent.at(-1).a==='start');
for(const phase of [2,4])for(let mark=0;mark<3;mark++){
 snapshot.cal.phase=phase;snapshot.cal.mark=mark;snapshot.cal.elapsed=2.4;update();assert(nodes.get('p04_position_clock').textContent.includes('2.4'));
 nodes.get('p04_position_next').click();assert(sent.at(-1).a==='mark');
}
snapshot.cal.phase=5;update();nodes.get('p04_position_next').click();assert(sent.at(-1).a==='save');snapshot.cal.phase=0;snapshot.notice='saved';update();assert(!nodes.get('p04_position_dialog').open);assert(nodes.get('p04_position_notice').textContent==='Kalibrierung übernommen.');
snapshot.moving=0;snapshot.phase=1;update();assert(targetButton.disabled && nodes.get('p04_position_begin').disabled);
snapshot.moving=-1;update();nodes.get('p04_service_setup').clientWidth=400;nodes.get('p04_position_begin').click();assert(!nodes.get('p04_position_intro').hidden);nodes.get('p04_position_cancel').click();assert(sent.at(-1).a==='cancel' && !nodes.get('p04_position_dialog').open);
snapshot.enabled=false;update();assert(box.hidden && box.children.at(-1).hidden);
snapshot.channels[0]=[false,-1];update();assert(box.hidden && box.children.at(-1).hidden);
console.log('PASS: real UI: status sync, calibrated widgets, target request, guided markers, inline/modal layout, apply/cancel, busy state and disabled feature');
