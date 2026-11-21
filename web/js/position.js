// Position estimates and calibration timing are owned by the ESP, never by this page.
function positionText(key) {
  const lang = document.getElementById("cfg_lang").value === "1" ? "en" : "de";
  return user_translations[key]?.[lang] || key;
}
document.addEventListener("DOMContentLoaded", () => {
  const $ = id => document.getElementById(id);
  const dialog = $("p04_position_dialog"), select = $("p04_selected_shutter");
  const dialogHome=dialog?.parentElement, inlineHost=$("p04_position_inline"), setup=$("p04_service_setup");
  let inlineMode=null;
  function closeWizard() {
    dialog.close();$("p04_position_intro").hidden=false;inlineMode=null;
  }
  function layoutWizard(opening=false) {
    if (!dialog.open && !opening) return;
    const inline=setup.clientWidth>=820;
    if (dialog.open && inline===inlineMode) return;
    if (dialog.open) dialog.close();
    inlineMode=inline;dialog.classList.toggle("position-inline",inline);
    $("p04_position_intro").hidden=inline;
    if(inline){inlineHost.append(dialog);dialog.show();}
    else{dialogHome.append(dialog);dialog.showModal();}
  }
  if (!dialog || !select) return;
  let snapshot, ownToken = 0, pending = false, renderKey = "", intro = false, language = $("cfg_lang").value;
  const connected = () => typeof ws !== "undefined" && ws?.readyState === WebSocket.OPEN;
  const channel = () => Number(select.value);
  const noticeText = key => ({busy:positionText("pos_busy"), queue_full:positionText("pos_queue_full"), unavailable:positionText("pos_unavailable"), not_calibrated:positionText("pos_not_calibrated"), position_unknown:positionText("pos_direct_unknown"), send_failed:positionText("pos_send_failed"), invalid_times:positionText("pos_invalid_times"), saved:positionText("pos_saved"), cancelled:positionText("pos_cancelled"), timeout:positionText("pos_timeout")}[key] || "");
  const send = (a, extra = {}) => {
    if (!connected()) return false;
    sendData("p04_position_cmd", JSON.stringify({ a, c: channel(), token: ownToken, ...extra }));
    return true;
  };
  function el(tag, text, className) {
    const node = document.createElement(tag);
    if (text !== undefined) node.textContent = text;
    if (className) node.className = className;
    return node;
  }
  function visual(height, live=false) {
    const figure=el("div",undefined,"position-illustration");figure.setAttribute("aria-hidden","true");
    const frame=el("div",undefined,"position-window"), shutter=el("div",undefined,"position-panzer");
    shutter.style.height=height+"%";frame.append(shutter);figure.append(frame,el("small",positionText("pos_visual_hint")));
    if(live){const clock=el("div","","position-value");clock.id="p04_position_clock";figure.append(clock);}
    return figure;
  }
  function steps(active) {
    const bar=$("p04_position_steps");bar.replaceChildren();
    [positionText("pos_prepare"),positionText("pos_up"),positionText("pos_down"),positionText("pos_review")].forEach((label,i)=>bar.append(el("span",label,i===active?"active":"")));
  }
  const widgets = Array.from({ length: 16 }, (_, c) => {
    const name = $("p01_jaro_ch_name_" + c);
    const box = el("div", undefined, "position-control");box.hidden=true;
    const label = el("p", "", "info-text"), row = el("div", undefined, "position-actions");
    const status=el("div",undefined,"position-channel-status");status.hidden=true;
    status.setAttribute("role","status");
    const statusCopy=el("div"), statusTitle=el("p","","position-label"), statusValue=el("p","","position-channel-value");
    const frame=el("div",undefined,"position-window"), shutter=el("div",undefined,"position-panzer"), targetLine=el("div",undefined,"position-target-line");
    frame.setAttribute("aria-hidden","true");frame.append(shutter,targetLine);statusCopy.append(statusTitle,statusValue);status.append(statusCopy,frame);
    const slider = document.createElement("input"); slider.type = "range"; slider.min = "0"; slider.max = "100"; slider.value = "50";
    slider.id="p01_position_target_"+c;
    slider.setAttribute("aria-label", positionText("pos_goal"));
    const goalLabel=el("label",positionText("pos_goal"),"position-label");goalLabel.htmlFor=slider.id;
    const output = el("output", "50 %", "position-value");
    slider.style.setProperty("--position-progress",slider.value+"%");
    slider.addEventListener("input", () => {slider.style.setProperty("--position-progress",slider.value+"%");output.textContent = slider.value + " %";targetLine.style.height=(100-Number(slider.value))+"%";});
    const button = el("button", "▶", "position-go"); button.type = "button";button.setAttribute("data-ui-local","");
    button.addEventListener("click", () => {if (connected()) sendData("p04_position_cmd", JSON.stringify({ a:"target", c, v:Number(slider.value) }));});
    const ends=el("div",undefined,"position-ends");ends.append(el("span",positionText("pos_closed")),el("span",positionText("pos_open")));
    const setupLink=el("a",positionText("pos_calibrate_link"),"position-setup-link");setupLink.href="#";
    setupLink.addEventListener("click",event=>{
      event.preventDefault();
      if(!snapshot?.enabled)return;
      select.value=String(c);select.dispatchEvent(new Event("change",{bubbles:true}));
      document.querySelector('a[data-tab="tab4"]')?.click();
      $("p04_position_begin").scrollIntoView({block:"nearest"});$("p04_position_begin").focus();
    });
    row.append(output, button);box.append(label,status,goalLabel,slider,ends,row,setupLink);name.parentElement.append(box);
    return {box,label,status,statusTitle,statusValue,frame,shutter,targetLine,slider,button,ends,row,goalLabel,setupLink};
  });
  function render() {
    if (!snapshot) return;
    const s=snapshot, cal=s.cal, queue=s.queue || [], active=cal.phase || s.moving>=0 || queue.length;
    $("cfg_position_enable").checked=s.enabled;
    $("cfg_position_reference").checked=s.reference!==false;
    const valid=s.channels[channel()]?.[0];
    const count=s.channels.filter((x,i) => x[0] && !$("p04_shutter_name_"+i).hidden && getComputedStyle($("p04_shutter_name_"+i)).display!=="none").length;
    $("p04_position_summary").textContent=s.enabled ? `${count} ${positionText("pos_total")} · ${positionText(valid?"pos_ready":"pos_missing")}` : positionText("pos_disabled");
    $("p04_position_notice").textContent=s.notice ? noticeText(s.notice) : "";
    $("p04_position_begin").disabled=!s.enabled || !!active || !connected();
    $("p04_position_begin").textContent=positionText(valid?"pos_recalibrate":"pos_calibrate");
    $("p04_position_begin").setAttribute("data-i18n",valid?"pos_recalibrate":"pos_calibrate");
    widgets.forEach((w,c) => {
      const [ready,pos]=s.channels[c];w.box.hidden=!s.enabled;
      w.box.classList.toggle("position-uncalibrated",s.enabled && !ready);
      const queueIndex=queue.findIndex(entry=>entry[0]===c), waiting=queueIndex>=0;
      const queuedGoal=waiting?queue[queueIndex][1]:null;
      if(waiting && w.queuedGoal!==queuedGoal) {
        w.slider.value=String(queuedGoal);w.slider.style.setProperty("--position-progress",queuedGoal+"%");
        w.row.children[0].textContent=queuedGoal+" %";
      }
      w.queuedGoal=queuedGoal;
      w.label.hidden=!s.enabled || ready;w.status.hidden=!s.enabled || !ready;w.goalLabel.hidden=w.ends.hidden=w.row.hidden=!s.enabled || !ready;
      w.goalLabel.textContent=positionText(waiting?"pos_queued_goal":"pos_goal");
      w.setupLink.hidden=!s.enabled || ready;w.setupLink.textContent=positionText("pos_calibrate_link");
      w.button.textContent=waiting?"":"▶";
      w.button.setAttribute("aria-busy",String(waiting));
      w.button.title=waiting?`${positionText("pos_waiting")} · ${queuedGoal} % · ${positionText("pos_queue_place")} ${queueIndex+1}`:positionText("pos_go");
      w.button.setAttribute("aria-label",waiting?w.button.title+" · "+positionText("pos_go"):positionText("pos_go"));
      w.slider.setAttribute("aria-label",positionText("pos_goal"));
      w.ends.children[0].textContent=positionText("pos_closed");w.ends.children[1].textContent=positionText("pos_open");
      const moving=s.moving===c;
      w.label.textContent=positionText("pos_missing");
      // No intermediate position is invented during a reference or target run.
      const known=pos>=0 && !moving;
      w.status.classList.toggle("position-unknown",!known);
      w.status.classList.toggle("position-referencing",moving);
      w.statusTitle.textContent=positionText("pos_estimate");
      const progress=s.phase===1 && Number.isFinite(s.remaining)?`${Math.max(0,Math.ceil(s.remaining))} s`:`${s.target} %`;
      w.statusValue.textContent=moving ? ["",positionText("pos_reference"),positionText("pos_settle"),positionText("pos_moving")][s.phase]+` · ${progress}` : known ? `${pos} %` : positionText(s.reference===false?"pos_direct_unknown":"pos_unknown");
      w.shutter.style.height=(known?100-pos:100)+"%";
      w.targetLine.hidden=!known;
      w.targetLine.style.height=(100-Number(w.slider.value))+"%";
      w.slider.hidden=w.button.hidden=!s.enabled || !ready;w.slider.disabled=w.button.disabled=!ready || !!cal.phase || moving || !connected();
    });
    if (!dialog.open || intro) return;
    if (!cal.phase) {closeWizard();ownToken=0;renderKey="";return;}
    if (cal.token!==ownToken) {closeWizard();return;}
    $("p04_position_channel").textContent=select.selectedOptions[0]?.textContent;
    const key=`${cal.phase}:${cal.mark}:${$("cfg_lang").value}`;
    if (key!==renderKey) {
      renderKey=key;
      steps(cal.phase<=2?1:cal.phase<=4?2:3);
      const body=$("p04_position_body");body.replaceChildren();
      if (cal.phase===1 || cal.phase===3) {
        const guide=el("div",undefined,"position-guide");guide.append(visual(cal.phase===1?100:0),el("p",positionText(cal.phase===1?"pos_up_hint":"pos_down_hint")));body.append(guide);
        $("p04_position_next").textContent=positionText(cal.phase===1?"pos_start_up":"pos_start_down");
      } else if (cal.phase===2 || cal.phase===4) {
        const keys=cal.phase===2?[positionText("pos_lift"),positionText("pos_half"),positionText("pos_full_open")]:[positionText("pos_half"),positionText("pos_bottom"),positionText("pos_full_closed")];
        const guide=el("div",undefined,"position-guide"), copy=el("div");
        guide.append(visual((cal.phase===2?[100,50,0]:[50,95,100])[cal.mark],true),copy);
        copy.append(el("h3",keys[cal.mark]),el("p",positionText("pos_mark_hint"),"info-text"));
        const list=el("ol",undefined,"position-checkpoints");keys.forEach((k,i)=>list.append(el("li",k+(i<cal.mark?` · ${(cal.times[(cal.phase===2?0:3)+i]/1000).toFixed(1)} s`:""),i<cal.mark?"position-done":i===cal.mark?"position-current":"")));copy.append(list);body.append(guide);
        $("p04_position_next").textContent=positionText("pos_now");
      } else {
        body.append(el("p",positionText("pos_review_hint"),"info-text"));
        const table=el("table");[positionText("pos_lift"),positionText("pos_half"),positionText("pos_full_open"),positionText("pos_half"),positionText("pos_bottom"),positionText("pos_full_closed")].forEach((k,i)=>{const row=el("tr");row.append(el("th",positionText(i<3?"pos_up":"pos_down")+" · "+k),el("td",(cal.times[i]/1000).toFixed(1)+" s"));table.append(row);});body.append(table);
        $("p04_position_next").textContent=positionText("pos_apply");
      }
    }
    const clock=$("p04_position_clock");if(clock){clock.textContent=cal.elapsed.toFixed(1)+" s";clock.setAttribute("aria-label",positionText("pos_time")+": "+clock.textContent);}
    pending=false;$("p04_position_next").disabled=false;
  }
  $("p04_position_begin").addEventListener("click", () => {
    intro=true;pending=false;renderKey="";ownToken=crypto.getRandomValues(new Uint32Array(1))[0]||1;
    $("p04_position_channel").textContent=select.selectedOptions[0]?.textContent;
    steps(0);
    const guide=el("div",undefined,"position-guide"), copy=el("div");
    copy.append(el("h3",positionText("pos_initial")),el("p",positionText("pos_initial_hint")),el("p",positionText("pos_reference_hint"),"info-text"));guide.append(visual(100),copy);$("p04_position_body").replaceChildren(guide);
    $("p04_position_next").textContent=positionText("pos_confirm");$("p04_position_next").disabled=false;layoutWizard(true);
  });
  $("p04_position_next").addEventListener("click", () => {
    if(pending)return;
    if(intro) {intro=false;pending=send("begin");}
    else {const phase=snapshot?.cal.phase;pending=send(phase===1||phase===3?"start":phase===5?"save":"mark");}
    $("p04_position_next").disabled=pending;
  });
  const cancel=()=>{if(ownToken)send("cancel");intro=false;pending=false;ownToken=0;closeWizard();};
  $("p04_position_cancel").addEventListener("click",cancel);dialog.addEventListener("cancel",event=>{event.preventDefault();cancel();});
  select.addEventListener("change",()=>{if(dialog.open)cancel();render();});
  window.addEventListener("webui:message",event=>{if(event.detail.type==="shutterPosition"){snapshot=event.detail;render();}});
  window.addEventListener("webui:connected",()=>send("status"));
  $("cfg_lang").addEventListener("change",()=>{widgets.forEach(w=>{w.button.title=positionText("pos_go");w.button.setAttribute("aria-label",positionText("pos_go"));w.slider.setAttribute("aria-label",positionText("pos_goal"));});renderKey="";render();});
  if(typeof ResizeObserver!=="undefined") new ResizeObserver(()=>layoutWizard()).observe(setup);
  setInterval(()=>{
    if(!connected()) {
      if(dialog.open){closeWizard();intro=false;pending=false;ownToken=0;$("p04_position_notice").textContent=positionText("pos_no_link");}
      widgets.forEach(w=>{w.button.disabled=w.slider.disabled=true;});return;
    }
    if(dialog.open && !intro && ownToken)send("beat");
    if(language!==$("cfg_lang").value){language=$("cfg_lang").value;renderKey="";render();}
  },1000);
});
