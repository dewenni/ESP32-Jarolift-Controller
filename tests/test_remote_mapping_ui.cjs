const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const root=path.resolve(__dirname,'..'),nodes=new Map(),docEvents={},events={},sent=[];
class Node {
 constructor(tag='div'){this.tagName=tag;this.children=[];this.listeners={};this.hidden=false;this.disabled=false;this.value='';this.open=false;}
 set id(v){this._id=v;nodes.set(v,this);}get id(){return this._id;}
 append(...items){this.children.push(...items);}replaceChildren(...items){this.children=[];this.append(...items);}
 setAttribute(k,v){this[k]=v;}addEventListener(k,f){this.listeners[k]=f;}showModal(){this.open=true;}close(){this.open=false;}
 click(){assert(!this.disabled);this.listeners.click?.({preventDefault(){}});}
}
for(const file of ['04_service.html','12_settings.html'])for(const [,id] of fs.readFileSync(path.join(root,'web/html',file),'utf8').matchAll(/id="([^"]+)"/g)){const n=new Node();n.id=id;}
nodes.get('cfg_lang').value='0';nodes.get('p04_selected_shutter').value='0';const nav=new Node();
const context=vm.createContext({console,crypto:require('crypto').webcrypto,ws:{readyState:1},WebSocket:{OPEN:1},confirm:()=>true,setTimeout:()=>1,clearTimeout(){},sendData:(id,p)=>sent.push({id,...JSON.parse(p)}),document:{getElementById:id=>nodes.get(id),createElement:tag=>new Node(tag),querySelectorAll:()=>[nav],addEventListener:(k,f)=>docEvents[k]=f},window:{addEventListener:(k,f)=>events[k]=f}});
vm.runInContext(fs.readFileSync(path.join(root,'web/js/lang.js'),'utf8'),context);vm.runInContext(fs.readFileSync(path.join(root,'web/js/remote_mappings.js'),'utf8'),context);docEvents.DOMContentLoaded();events['webui:connected']();assert(sent.at(-1).a==='status');
let state={type:'remoteMappings',channel:0,channelName:'Kitchen',enabled:true,token:0,phase:0,captureChannel:0,serial:0,mask:0,error:'',limit:5,items:[]};
const update=()=>events['webui:message']({detail:{...state}});update();assert(!nodes.get('remote_mapping_empty').hidden);
nodes.get('remote_mapping_add').click();assert(sent.at(-1).a==='listen' && nodes.get('remote_mapping_dialog').open);let token=sent.at(-1).token;
state={...state,token,phase:1};update();assert(nodes.get('remote_mapping_save').disabled);
state={...state,phase:2,serial:0xc7c0001,mask:2};update();assert(nodes.get('remote_mapping_signal').textContent.includes('0c7c0001 · 0002'));assert(nodes.get('remote_mapping_save').disabled);
const prior=nodes.get('remote_mapping_dialog_notice').textContent;
events['webui:message']({detail:{...state,reply:token+1,error:'busy'}});
assert(nodes.get('remote_mapping_dialog_notice').textContent===prior);
state={...state,phase:3,existingName:'Shared'};update();assert(!nodes.get('remote_mapping_name').disabled && !nodes.get('remote_mapping_save').disabled);assert(!nodes.get('remote_mapping_shared').hidden);
nodes.get('remote_mapping_name').value='😀'.repeat(33);const before=sent.length;nodes.get('remote_mapping_save').click();assert(sent.length===before);
nodes.get('remote_mapping_name').value='😀'.repeat(32);nodes.get('remote_mapping_save').click();assert(sent.at(-1).a==='save' && sent.at(-1).channel===0 && !('targets' in sent.at(-1)));
state={...state,token:0,phase:0,reply:token,result:'saved',items:[{id:0,name:'Living',serial:0xc7c0001,mask:2,targets:3}]};update();assert(!nodes.get('remote_mapping_dialog').open && nodes.get('remote_mapping_rows').children.length===1);
nodes.get('remote_mapping_rows').children[0].children[1].click();assert(sent.at(-1).a==='delete' && sent.at(-1).serial===0xc7c0001 && sent.at(-1).mask===2);
state={...state,reply:sent.at(-1).token,result:'deleted',items:[]};update();assert(!nodes.get('remote_mapping_empty').hidden);
nodes.get('p04_selected_shutter').value='1';nodes.get('p04_selected_shutter').listeners.change();assert(sent.at(-1).channel===1);
state={...state,channel:1,captureChannel:1,result:'',reply:0};update();nodes.get('remote_mapping_add').click();context.ws.readyState=3;events['webui:disconnected']();assert(!nodes.get('remote_mapping_dialog').open && nodes.get('remote_mapping_add').disabled);context.ws.readyState=1;events['webui:connected']();assert(sent.at(-2).a==='cancel' && sent.at(-2).channel===1 && sent.at(-1).a==='status');
nav.click();assert(sent.at(-1).a==='status');
console.log('PASS: actual Service UI, selected-motor scoping, confirm-before-save, Unicode limit, shared source naming, removal, offline cleanup, reconnect');
