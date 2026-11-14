// Configuration only: capturing and mapping a remote signal never sends motor commands.
document.addEventListener("DOMContentLoaded",()=>{
  const $=id=>document.getElementById(id), dialog=$("remote_mapping_dialog");if(!dialog)return;
  const text=key=>user_translations[key]?.[$("cfg_lang").value==="1"?"en":"de"]||key;
  const errorText=key=>({busy:text("rm_busy"),unavailable:text("rm_unavailable"),invalid:text("rm_invalid"),no_signal:text("rm_no_signal"),targets:text("rm_targets"),duplicate:text("rm_duplicate"),full:text("rm_full"),timeout:text("rm_timeout")}[key]||text("rm_invalid"));
  const hex=(value,width)=>Number(value).toString(16).padStart(width,"0");
  const online=()=>typeof ws!=="undefined" && ws?.readyState===WebSocket.OPEN;
  let state,token=0,id=-1,draft="",pending=false,wait,abandoned=0;
  const el=(tag,value)=>{const n=document.createElement(tag);if(value!==undefined)n.textContent=value;return n;};
  const send=(a,extra={})=>{if(!online())return false;sendData("remote_mapping_cmd",JSON.stringify({a,token,...extra}));return true;};
  const notice=message=>{const id=dialog.open?"remote_mapping_dialog_notice":"remote_mapping_notice";$(id).textContent=message;};
  function setPending(value){pending=value;$("remote_mapping_save").disabled=value||!online()||!state?.captured||state.token!==token;}
  function request(a,extra={}){
    if(!send(a,extra)){notice(text("rm_offline"));return;}
    setPending(true);clearTimeout(wait);wait=setTimeout(()=>{setPending(false);notice(text("rm_no_reply"));},6000);
  }
  function close(){clearTimeout(wait);if(token && !send("cancel"))abandoned=token;dialog.close();token=0;draft="";setPending(false);}
  function begin(row){
    token=crypto.getRandomValues(new Uint32Array(1))[0]||1;id=row?.id??-1;draft="";
    $("remote_mapping_name").value=row?.name||"";$("remote_mapping_targets").replaceChildren();
    $("remote_mapping_signal").textContent=text("rm_waiting");dialog.showModal();notice("");
    request(row?"edit":"listen",{id});
  }
  function render(){
    if(!state)return;
    const body=$("remote_mapping_rows");body.replaceChildren();
    $("remote_mapping_empty").hidden=state.items.length>0;
    for(const row of state.items){
      const tr=el("tr");tr.append(el("td",row.name),el("td",hex(row.serial,8)),el("td",hex(row.mask,4)));
      const names=state.channels.flatMap(([name],ch)=>row.targets&(1<<ch)?[name||`CH${ch+1}`]:[]);
      tr.append(el("td",names.join(", ")));
      const actions=el("td");actions.className="remote-mapping-actions";
      for(const [key,fn] of [[text("rm_edit"),()=>begin(row)],[text("rm_delete"),()=>{
        if(!confirm(text("rm_delete_confirm")))return;
        token=crypto.getRandomValues(new Uint32Array(1))[0]||1;request("delete",{id:row.id});
      }]]){const b=el("button",key);b.type="button";b.setAttribute("data-ui-local","");b.disabled=!online();b.addEventListener("click",fn);actions.append(b);}
      tr.append(actions);body.append(tr);
    }
    $("remote_mapping_add").disabled=!online();
    if(!dialog.open || state.token!==token)return;
    id=state.id;
    const key=`${token}:${id}:${state.serial}:${state.mask}:${state.captured}`;
    if(key!==draft){
      draft=key;const row=state.items.find(x=>x.id===id);
      if(row)$("remote_mapping_name").value=row.name;
      $("remote_mapping_targets").replaceChildren();
      state.channels.forEach(([name,enabled],ch)=>{
        if(!enabled)return;
        const label=el("label"),input=el("input");input.type="checkbox";input.value=String(ch);input.checked=!!(row?.targets&(1<<ch));label.append(input,el("span",name||`CH${ch+1}`));$("remote_mapping_targets").append(label);
      });
    }
    const command=({2:"DOWN",3:"SHADE",4:"STOP",8:"UP"})[state.command]||"";
    $("remote_mapping_signal").textContent=state.captured?`${hex(state.serial,8)} · ${hex(state.mask,4)}${command?" · "+command:""}`:text("rm_waiting");
    $("remote_mapping_retry").disabled=pending;setPending(pending);
  }
  $("remote_mapping_settings").addEventListener("toggle",()=>{if($("remote_mapping_settings").open)send("status");});
  $("remote_mapping_add").addEventListener("click",()=>begin());
  $("remote_mapping_close").addEventListener("click",close);
  dialog.addEventListener("cancel",event=>{event.preventDefault();close();});
  $("remote_mapping_retry").addEventListener("click",()=>{notice("");request("listen",{id});});
  $("remote_mapping_save").addEventListener("click",()=>{
    if(pending)return;
    const name=$("remote_mapping_name").value.trim();let targets=0;
    $("remote_mapping_targets").querySelectorAll("input:checked").forEach(n=>targets|=1<<Number(n.value));
    if(!name || new TextEncoder().encode(name).length>47){notice(text("rm_invalid"));return;}
    if(!targets){notice(text("rm_targets"));return;}
    request("save",{name,targets});
  });
  window.addEventListener("webui:message",event=>{
    const data=event.detail;if(data.type!=="remoteMappings")return;
    if(token && data.reply===token && ["saved","deleted","cancelled"].includes(data.result)){
      clearTimeout(wait);pending=false;token=0;draft="";dialog.close();
    }
    if(data.reply===token || (!data.reply && data.token===token)){clearTimeout(wait);pending=false;notice(data.error?errorText(data.error):"");}
    state=data;render();
    if(data.result==="expired" && dialog.open && data.reply===token)$("remote_mapping_save").disabled=true;
  });
  window.addEventListener("webui:connected",()=>{if(abandoned){send("cancel",{token:abandoned});abandoned=0;}send("status");});
  $("cfg_lang").addEventListener("change",render);
  setInterval(()=>{if(!online()){if(dialog.open)close();if(state)render();}},1000);
  // Config loader expects field IDs even though this array is edited through the dialog.
  const hidden=$("remote_mapping_config");
  for(let i=0;i<32;++i)for(const field of ["name","serial","sourceMask","targets"]){const n=el("input");n.type="hidden";n.id=`cfg_remoteMappings_${i}_${field}`;hidden.append(n);}
});
