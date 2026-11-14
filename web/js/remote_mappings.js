// ESP status registration only: this dialog never transmits motor commands.
document.addEventListener("DOMContentLoaded",()=>{
  const $=id=>document.getElementById(id),dialog=$("remote_mapping_dialog");if(!dialog)return;
  const text=key=>user_translations[key]?.[$("cfg_lang").value==="1"?"en":"de"]||key;
  const errorText=key=>({busy:text("rm_busy"),unavailable:text("rm_unavailable"),invalid:text("rm_invalid"),no_signal:text("rm_no_signal"),targets:text("rm_targets"),full:text("rm_full"),motor_full:text("rm_motor_full"),timeout:text("rm_timeout"),group:text("rm_group"),different:text("rm_different")}[key]||text("rm_invalid"));
  const hex=(value,width)=>Number(value).toString(16).padStart(width,"0");
  const online=()=>typeof ws!=="undefined" && ws?.readyState===WebSocket.OPEN;
  const el=(tag,value)=>{const n=document.createElement(tag);if(value!==undefined)n.textContent=value;return n;};
  const selected=()=>Number($("p04_selected_shutter").value);
  let state,token=0,pending=false,wait,channel=0,abandoned=0,abandonedChannel=0,namedSource="";
  const send=(a,extra={})=>{if(!online())return false;sendData("remote_mapping_cmd",JSON.stringify({a,token,channel:dialog.open?channel:selected(),...extra}));return true;};
  const notice=message=>$(dialog.open?"remote_mapping_dialog_notice":"remote_mapping_notice").textContent=message;
  function render(){
    if(!state || state.channel!==selected())return;
    const body=$("remote_mapping_rows");body.replaceChildren();
    $("remote_mapping_empty").hidden=state.items.length>0;
    $("remote_mapping_count").textContent=`${state.items.length} / ${state.limit}`;
    for(const row of state.items){
      const line=el("div");line.className="remote-mapping-row";
      const info=el("div"),name=el("strong",row.name);
      const details=el("small",`${hex(row.serial,8)} · ${hex(row.mask,4)}${row.legacy?" · "+text("rm_legacy"):""}`);
      info.append(name,details);line.append(info);
      const remove=el("button",text("rm_delete"));remove.type="button";remove.setAttribute("data-ui-local","");remove.disabled=pending||!online();
      remove.addEventListener("click",()=>{
        if(!confirm(text("rm_delete_confirm")))return;
        token=crypto.getRandomValues(new Uint32Array(1))[0]||1;
        request("delete",{id:row.id,serial:row.serial,mask:row.mask});
      });line.append(remove);body.append(line);
    }
    $("remote_mapping_add").disabled=pending||!online()||!state.enabled;
    if(!dialog.open)return;
    const owns=state.token===token && state.captureChannel===channel;
    const phase=owns?state.phase:0;
    $("remote_mapping_steps").textContent=[text("rm_step_capture"),text("rm_step_confirm"),text("rm_step_save")].map((label,i)=>`${phase===i+1?"●":"○"} ${label}`).join("  ·  ");
    $("remote_mapping_signal").textContent=phase>1?`${hex(state.serial,8)} · ${hex(state.mask,4)}`:text("rm_waiting");
    $("remote_mapping_phase").textContent=text(phase===2?"rm_confirm":phase===3?"rm_confirmed":"rm_hint");
    const source=`${state.serial}:${state.mask}`;
    if(owns && phase>1 && source!==namedSource){namedSource=source;$("remote_mapping_name").value=state.existingName||"";}
    $("remote_mapping_shared").hidden=!(owns && state.existingName);
    $("remote_mapping_name").disabled=phase!==3||pending;
    $("remote_mapping_save").disabled=pending||!online()||phase!==3;
    $("remote_mapping_retry").disabled=pending;
  }
  function request(a,extra={}){
    if(!send(a,extra)){notice(text("rm_offline"));return;}
    pending=true;render();clearTimeout(wait);
    wait=setTimeout(()=>{pending=false;notice(text("rm_no_reply"));render();},6000);
  }
  function close(){
    clearTimeout(wait);
    if(token && !send("cancel")){abandoned=token;abandonedChannel=channel;}
    dialog.close();token=0;pending=false;namedSource="";render();
  }
  $("remote_mapping_add").addEventListener("click",()=>{
    channel=selected();token=crypto.getRandomValues(new Uint32Array(1))[0]||1;namedSource="";
    $("remote_mapping_name").value="";$("remote_mapping_channel").textContent=state?.channelName||"";
    dialog.showModal();notice("");request("listen");
  });
  $("remote_mapping_close").addEventListener("click",close);
  dialog.addEventListener("cancel",event=>{event.preventDefault();close();});
  $("remote_mapping_retry").addEventListener("click",()=>{namedSource="";notice("");request("listen");});
  $("remote_mapping_save").addEventListener("click",()=>{
    if(pending)return;
    const name=$("remote_mapping_name").value.trim();
    if(!name || Array.from(name).length>32 || /[\u0000-\u001f\u007f]/u.test(name)){notice(text("rm_invalid"));return;}
    request("save",{name});
  });
  $("p04_selected_shutter").addEventListener("change",()=>{state=undefined;$("remote_mapping_rows").replaceChildren();send("status");});
  document.querySelectorAll('[data-tab="tab4"]').forEach(n=>n.addEventListener("click",()=>send("status")));
  window.addEventListener("webui:message",event=>{
    const data=event.detail;if(data.type!=="remoteMappings" || data.channel!==selected())return;
    const reply=token && data.reply===token,owns=token && data.token===token;
    if(reply||(!data.reply && owns)){clearTimeout(wait);pending=false;notice(data.error?errorText(data.error):"");}
    if(reply && ["saved","deleted","cancelled"].includes(data.result)){token=0;dialog.close();namedSource="";}
    state=data;render();
  });
  window.addEventListener("webui:connected",()=>{
    if(abandoned){send("cancel",{token:abandoned,channel:abandonedChannel});abandoned=0;}
    send("status");
  });
  window.addEventListener("webui:disconnected",()=>{if(dialog.open)close();pending=false;render();notice(text("rm_offline"));});
  $("cfg_lang").addEventListener("change",render);
  // Hidden fields are required by the existing configuration-to-DOM loader.
  const hidden=$("remote_mapping_config");
  for(let i=0;i<80;++i)for(const field of ["name","serial","sourceMask","targets","legacy"]){const n=el("input");n.type="hidden";n.id=`cfg_remoteMappings_${i}_${field}`;hidden.append(n);}
});
