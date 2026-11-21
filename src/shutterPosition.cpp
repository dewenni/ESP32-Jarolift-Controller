#include <Arduino.h>
#include <ArduinoJson.h>
#include <EspSysUtil.h>
#include <config.h>
#include <jarolift.h>
#include <positionModel.h>
#include <shutterPosition.h>
#include <webUI.h>

namespace {
PositionMove moves[16];
constexpr uint32_t COLLECT_MS = 200;
constexpr uint32_t STOP_WINDOW_MS = 1000;
// Two ~101 ms telegrams, pauses, NVS and its 100 ms delay, plus margin.
constexpr uint32_t SEND_BUDGET_MS = 450;
uint32_t collectedAt = 0;
// At most one waiting target per channel; fixed storage, no heap allocation.
struct Target { uint8_t channel, goal; } targets[16];
uint8_t targetCount = 0;
void clearTargets() { targetCount = 0; }
Target popTarget(uint8_t index) {
  const Target next = targets[index];
  for (uint8_t i=index+1; i<targetCount; ++i) targets[i-1] = targets[i];
  --targetCount;
  return next;
}
int8_t positions[16] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
// Passive endpoint estimates for manual/group/received remote commands (no motor sends).
struct Travel { uint32_t since=0, duration=0; uint8_t target=0; } travel[16];
struct CalibrationSession {
  uint8_t phase = 0, channel = 0, mark = 0; // 1 ready up, 2 up, 3 ready down, 4 down, 5 review
  uint32_t since = 0, lease = 0, token = 0;
  PositionCalibration values;
} cal;
uint32_t lastUpdate = 0;
bool dirty = true;
void setPosition(uint8_t ch, int8_t value) {
  if(positions[ch]==value)return;
  positions[ch]=value; mqttSendPosition(ch,value); dirty=true;
}

bool previousEnable = false, previousReference = true;
const char *notice = "";

bool send(uint8_t ch, bool up) { return jaroPositionSend(ch, up ? CMD_UP : CMD_DOWN); }
void status() {
  JsonDocument doc;
  doc["type"] = "shutterPosition";
  doc["enabled"] = config.position.enable;
  doc["reference"] = config.position.reference;
  doc["notice"] = notice;
  JsonArray channels = doc["channels"].to<JsonArray>();
  for (uint8_t i=0; i<16; ++i) {
    JsonArray row = channels.add<JsonArray>();
    row.add(config.position.calibration[i].valid()); row.add(positions[i]);
  }
  JsonArray active = doc["moves"].to<JsonArray>();
  const uint32_t now = millis();
  for (const auto &move : moves) if (move.active()) {
    JsonArray row = active.add<JsonArray>();
    row.add(move.channel); row.add(int(move.phase)); row.add(move.target);
    const uint32_t elapsed = now - move.since;
    row.add(move.phase == PositionMove::Reference && elapsed < move.duration
        ? (move.duration - elapsed + 999) / 1000 : 0);
  }
  JsonArray waiting = doc["queue"].to<JsonArray>();
  for (uint8_t i=0; i<targetCount; ++i) {
    JsonArray entry = waiting.add<JsonArray>();
    entry.add(targets[i].channel); entry.add(targets[i].goal);
  }
  JsonObject session = doc["cal"].to<JsonObject>();
  session["phase"] = cal.phase; session["channel"] = cal.channel;
  session["mark"] = cal.mark; session["token"] = cal.token;
  session["elapsed"] = (cal.phase==2 || cal.phase==4) ? (millis()-cal.since)/1000.0f : 0;
  JsonArray times = session["times"].to<JsonArray>();
  for (auto t : cal.values.up) times.add(t);
  for (auto t : cal.values.down) times.add(t);
  webUI.wsUpdateWebJSON(doc);
  dirty = false; lastUpdate = millis();
}
void cancel(bool stopMotor) {
  clearTargets();
  uint16_t mask = 0;
  for (auto &move : moves) if (move.active()) {
    mask |= uint16_t(1) << move.channel;
    setPosition(move.channel,-1); move.cancel();
  }
  if (stopMotor && mask) jaroPositionGroupSend(mask,CMD_STOP);
  if (cal.phase) {
    const uint8_t ch=cal.channel; const bool moving=cal.phase==2 || cal.phase==4;
    cal.phase=0; setPosition(ch,-1);
    if (moving && stopMotor) jaroPositionSend(ch, CMD_STOP);
  }
  dirty = true;
}
void failure(const char *key) { notice=key; cancel(true); }
bool available(uint8_t ch) {
  return ch<16 && config.position.enable && config.jaro.ch_enable[ch] && !setupMode &&
         !EspSysUtil::OTA::getInstance().isActive();
}
// All deadlines are less than 2^31 ms away, including across millis() rollover.
struct StopGroup { uint16_t mask = 0; uint32_t earliest = 0, at = 0; };
StopGroup nextStop(uint32_t now) {
  StopGroup group;
  for (const auto &move : moves) if (move.phase == PositionMove::Target) {
    const uint32_t at = move.stopSendAt();
    if (!group.mask || int32_t(at-now) < int32_t(group.earliest-now)) {
      group.mask = uint16_t(1) << move.channel; group.earliest = at;
    }
  }
  if (!group.mask) return group;
  uint32_t span = 0;
  for (const auto &move : moves) if (move.phase == PositionMove::Target) {
    const uint32_t offset = move.stopSendAt() - group.earliest;
    if (offset <= STOP_WINDOW_MS) {
      group.mask |= uint16_t(1) << move.channel;
      if (offset > span) span = offset;
    }
  }
  // Replanning cannot postpone the earliest member by more than half the window.
  group.at = group.earliest + span/2;
  return group;
}
bool canStart(const StopGroup &group, uint32_t now) {
  // Reserve one complete send before the planned group STOP. Reserving another
  // half-window before the earliest individual STOP needlessly stalls starts.
  // Replan after every send; a new short move can then be stopped immediately.
  return !group.mask || int32_t(group.at-now) > int32_t(SEND_BUDGET_MS);
}
void startPending() {
  // One radio operation per cyclic call; STOP is reconsidered before the next send.
  if (!targetCount || millis()-collectedAt < COLLECT_MS) return;
  for (uint8_t i=0; i<targetCount;) {
    const auto next = targets[i];
    if (moves[next.channel].active()) {++i;continue;}
    if (!available(next.channel) || !shutterPositionReady(next.channel)) {
      popTarget(i); notice="unavailable"; dirty=true; continue;
    }
    const uint8_t ch=next.channel, goal=next.goal;
    if (!config.position.reference && goal>0 && goal<100) {
      const int8_t current=positions[ch];
      if (current<0) {popTarget(i);notice="position_unknown";dirty=true;continue;}
      if (current==goal) {popTarget(i);dirty=true;continue;}
      popTarget(i);travel[ch].duration=0;setPosition(ch,-1);
      if (!send(ch,goal>current)) {failure("send_failed");return;}
      moves[ch].beginDirect(ch,goal,current,config.position.calibration[ch],jaroPositionSentAt());
    } else {
      // Reference starts and endpoints share one telegram per direction.
      const bool up=goal>=50;
      uint16_t mask=0;
      for (uint8_t j=i; j<targetCount; ++j) {
        const auto &t=targets[j];
        if (!moves[t.channel].active() && available(t.channel) && shutterPositionReady(t.channel) &&
            (config.position.reference || t.goal==0 || t.goal==100) && (t.goal>=50)==up)
          mask |= uint16_t(1)<<t.channel;
      }
      for (uint8_t channel=0; channel<16; ++channel) if (mask & (uint16_t(1)<<channel)) {
        travel[channel].duration=0;setPosition(channel,-1);
      }
      if (!jaroPositionGroupSend(mask,up?CMD_UP:CMD_DOWN)) {failure("send_failed");return;}
      const uint32_t sentAt=jaroPositionSentAt();
      for (uint8_t j=0; j<targetCount;) {
        const auto t=targets[j];
        if (!(mask & (uint16_t(1)<<t.channel))) {++j;continue;}
        popTarget(j);travel[t.channel].duration=0;setPosition(t.channel,-1);
        if (t.goal==0 || t.goal==100) {
          // The motor handles the endpoint; do not keep an automatic STOP owner.
          auto &estimate=travel[t.channel];
          estimate.since=sentAt;estimate.target=t.goal;
          estimate.duration=config.position.calibration[t.channel].referenceTime(up);
        } else moves[t.channel].begin(t.channel,t.goal,config.position.calibration[t.channel],sentAt);
      }
    }
    notice="";dirty=true;return;
  }
}
bool requestTarget(uint8_t ch, uint8_t goal) {
  dirty=true;
  if(!available(ch)) {notice="unavailable";return false;}
  if(cal.phase || jaroCommandsPending()) {notice="busy";return false;}
  if(!shutterPositionReady(ch)) {notice="not_calibrated";return false;}
  if(!config.position.reference && goal>0 && goal<100 && !moves[ch].active() && positions[ch]<0) {
    notice="position_unknown";return false;
  }
  for(uint8_t i=0; i<targetCount; ++i) if(targets[i].channel==ch) {
    targets[i].goal=goal;notice="";return true;
  }
  if(targetCount>=16) {notice="queue_full";return false;}
  if(!targetCount) collectedAt=millis();
  targets[targetCount++]={ch,goal};notice="";
  return true;
}
}

bool shutterPositionReady(uint8_t ch) {
  return ch<16 && config.position.enable && config.jaro.ch_enable[ch] && config.position.calibration[ch].valid();
}
bool shutterPositionTarget(uint8_t ch, const char *payload) {
  if(!payload || !*payload)return false;
  unsigned value=0;
  for(const char *p=payload;*p;++p) {
    if(*p<'0' || *p>'9')return false;
    value=value*10+unsigned(*p-'0');
    if(value>100)return false;
  }
  return requestTarget(ch,static_cast<uint8_t>(value));
}

bool shutterPositionBusy() {
  if (cal.phase || targetCount) return true;
  for (const auto &move : moves) if (move.active()) return true;
  return false;
}
void shutterPositionManual(uint16_t mask, bool stopMotor) {
  // Manual radio commands cancel automation before their potentially long transmission.
  if (shutterPositionBusy()) cancel(stopMotor);
  for (uint8_t i=0;i<16;++i) if(mask & (uint16_t(1)<<i)) {travel[i].duration=0;setPosition(i,-1);}
  notice=""; dirty=true;
}
void shutterPositionRemote(uint16_t mask, JaroCmdType command) {
  // A received command has already changed the motor state. Do not override it.
  if (mask) clearTargets(); // Do not resume queued automation after remote operation.
  for (auto &move : moves) if (move.active() && (mask & (uint16_t(1)<<move.channel))) {
    move.cancel(); setPosition(move.channel,-1);
  }
  if (cal.phase && (mask & (uint16_t(1)<<cal.channel))) {
    setPosition(cal.channel,-1);cal.phase=0;
  }
  shutterPositionCommand(mask,command);
  dirty=true;
}
void shutterPositionPublish() {
  for(uint8_t ch=0;ch<16;++ch) if(config.jaro.ch_enable[ch]) mqttSendPosition(ch,positions[ch]);
}
void shutterPositionCommand(uint16_t mask, JaroCmdType command) {
  for(uint8_t ch=0;ch<16;++ch) if(mask & (uint16_t(1)<<ch)) {
    auto &t=travel[ch];
    const bool endpoint=command==CMD_UP || command==CMD_DOWN;
    const uint8_t target=command==CMD_UP?100:0;
    // Repeated RF frames of one held button must not keep postponing the endpoint.
    if(endpoint && t.duration && t.target==target)continue;
    t.duration=0;
    if(!endpoint) {setPosition(ch,-1);continue;}
    const auto &calibration=config.position.calibration[ch];
    if(calibration.valid()) {
      setPosition(ch,-1);t.since=millis();t.target=target;
      t.duration=calibration.referenceTime(command==CMD_UP);
    } else {
      // Without measured travel times retain the legacy expected-endpoint indication.
      setPosition(ch,target);
    }
  }
}
void shutterPositionInvalidate(uint8_t ch) {
  if(ch>=16)return;
  shutterPositionManual(uint16_t(1)<<ch);
  config.position.calibration[ch]=PositionCalibration{};
}
void shutterPositionWebCommand(const char *payload) {
  JsonDocument req;
  if (deserializeJson(req,payload) || !req.is<JsonObject>()) return;
  const char *action=req["a"] | "";
  if (!strcmp(action,"status")) {status(); return;}
  const uint32_t token=req["token"] | uint32_t(0);
  if (!strcmp(action,"beat")) {if(cal.phase && token==cal.token)cal.lease=millis(); return;}
  if (!strcmp(action,"cancel")) {
    if(cal.phase && token==cal.token) {cancel(true);notice="cancelled";}
    return;
  }
  if (!req["c"].is<unsigned int>() || req["c"].as<unsigned int>()>=16) return;
  const uint8_t ch=req["c"];
  if (!available(ch)) {notice="unavailable";dirty=true;return;}
  if (!strcmp(action,"target")) {
    if (!req["v"].is<unsigned int>() || req["v"].as<unsigned int>()>100) return;
    requestTarget(ch,req["v"].as<uint8_t>());
  } else if (!strcmp(action,"begin")) {
    if(shutterPositionBusy() || jaroCommandsPending() || token==0){notice="busy";dirty=true;return;}
    travel[ch].duration=0;cal=CalibrationSession{};cal.channel=ch;cal.phase=1;cal.token=token;cal.lease=millis();notice="";dirty=true;
  } else if (cal.phase && token==cal.token && ch==cal.channel) {
    cal.lease=millis();
    if (!strcmp(action,"start") && (cal.phase==1 || cal.phase==3)) {
      const bool up=cal.phase==1;
      if(!send(ch,up)){failure("send_failed");return;}
      cal.phase=up?2:4;cal.mark=0;cal.since=jaroPositionSentAt();setPosition(ch,-1);
    } else if(!strcmp(action,"mark") && (cal.phase==2 || cal.phase==4)) {
      uint32_t elapsed=millis()-cal.since;
      uint32_t *times=cal.phase==2?cal.values.up:cal.values.down;
      if(elapsed>180000 || (cal.mark && elapsed<=times[cal.mark-1])) {failure("invalid_times");return;}
      times[cal.mark++]=elapsed;
      if(cal.mark==3) {
        if(!jaroPositionSend(ch,CMD_STOP)){failure("send_failed");return;}
        setPosition(ch,cal.phase==2?100:0);cal.phase=cal.phase==2?3:5;cal.mark=0;
      }
    } else if(!strcmp(action,"save") && cal.phase==5) {
      if(!cal.values.valid()){notice="invalid_times";dirty=true;return;}
      config.position.calibration[ch]=cal.values;cal.phase=0;notice="saved";
    }
    dirty=true;
  }
}
void shutterPositionCyclic() {
  const uint32_t now=millis();
  if (previousEnable != config.position.enable || previousReference != config.position.reference) {
    previousEnable=config.position.enable;previousReference=config.position.reference;dirty=true;
  }
  if(shutterPositionBusy() && (!config.position.enable || setupMode || EspSysUtil::OTA::getInstance().isActive())) failure("cancelled");
  bool invalid=cal.phase && !available(cal.channel);
  for (const auto &move : moves) if (move.active() &&
      (!available(move.channel) || !config.position.calibration[move.channel].valid())) invalid=true;
  if(invalid) failure("cancelled");
  if(cal.phase && (now-cal.lease>6000 || ((cal.phase==2 || cal.phase==4) && now-cal.since>180000))) failure("timeout");
  for (auto &move : moves) if (move.active() && move.phase!=PositionMove::Target) {
    const auto previous=move.phase;
    if (move.next(millis())==PositionMove::Complete) setPosition(move.channel,move.target);
    if (move.phase!=previous) dirty=true;
  }
  for(uint8_t ch=0;ch<16;++ch) {
    auto &t=travel[ch];
    if(t.duration && millis()-t.since>=t.duration) {t.duration=0;setPosition(ch,t.target);}
  }
  const uint32_t dispatchAt=millis();
  const auto group=nextStop(dispatchAt);
  if (group.mask && int32_t(dispatchAt-group.at)>=0) {
    if (!jaroPositionGroupSend(group.mask,CMD_STOP)) failure("send_failed");
    else for (auto &move : moves) if (move.active() && (group.mask & (uint16_t(1)<<move.channel))) {
      setPosition(move.channel,move.target);move.cancel();
    }
    dirty=true;
  } else if (!cal.phase && !jaroCommandsPending() && canStart(group,dispatchAt)) {
    bool started=false;
    for (auto &move : moves) if (move.phase==PositionMove::Settle && millis()-move.since>=600) {
      if (send(move.channel,move.targetUp())) move.targetStarted(config.position.calibration[move.channel],jaroPositionSentAt());
      else failure("send_failed");
      dirty=true;started=true;break;
    }
    if (!started) startPending();
  }
  // All timing stays on the ESP. Browsers only receive a small snapshot once per second.
  if (!EspSysUtil::OTA::getInstance().isActive() && (dirty || (shutterPositionBusy() && millis()-lastUpdate>=1000))) status();
}
