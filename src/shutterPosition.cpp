#include <Arduino.h>
#include <ArduinoJson.h>
#include <EspSysUtil.h>
#include <config.h>
#include <jarolift.h>
#include <positionModel.h>
#include <shutterPosition.h>
#include <webUI.h>

namespace {
PositionMove move;
// At most one waiting target per channel; fixed storage, no heap allocation.
struct Target { uint8_t channel, goal; } targets[16];
uint8_t targetCount = 0;
void clearTargets() { targetCount = 0; }
Target popTarget() {
  const Target next = targets[0];
  for (uint8_t i=1; i<targetCount; ++i) targets[i-1] = targets[i];
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

bool previousEnable = false;
const char *notice = "";

bool send(uint8_t ch, bool up) { return jaroPositionSend(ch, up ? CMD_UP : CMD_DOWN); }
void status() {
  JsonDocument doc;
  doc["type"] = "shutterPosition";
  doc["enabled"] = config.position.enable;
  doc["notice"] = notice;
  JsonArray channels = doc["channels"].to<JsonArray>();
  for (uint8_t i=0; i<16; ++i) {
    JsonArray row = channels.add<JsonArray>();
    row.add(config.position.calibration[i].valid()); row.add(positions[i]);
  }
  doc["moving"] = move.active() ? int(move.channel) : -1;
  doc["phase"] = int(move.phase);
  doc["target"] = move.target;
  const uint32_t elapsed = millis() - move.since;
  doc["remaining"] = move.phase == PositionMove::Reference && elapsed < move.duration
      ? (move.duration - elapsed + 999) / 1000 : 0;
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
  if (move.active()) {
    const uint8_t ch = move.channel;
    move.cancel(); setPosition(ch,-1);
    if (stopMotor) jaroPositionSend(ch, CMD_STOP);
  }
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
bool startTarget(uint8_t ch, uint8_t goal) {
  travel[ch].duration=0;setPosition(ch,-1);notice="";
  if(!send(ch,goal>=50)) {failure("send_failed");return false;}
  if(goal==0 || goal==100) {
    // End stops are handled by the motor. Estimate completion without owning the FIFO.
    shutterPositionCommand(uint16_t(1)<<ch,goal==100?CMD_UP:CMD_DOWN);
  } else {
    move.begin(ch,goal,config.position.calibration[ch],jaroPositionSentAt());
  }
  dirty=true;
  return true;
}
bool requestTarget(uint8_t ch, uint8_t goal) {
  dirty=true;
  if(!available(ch)) {notice="unavailable";return false;}
  if(cal.phase || jaroCommandsPending()) {notice="busy";return false;}
  if(!shutterPositionReady(ch)) {notice="not_calibrated";return false;}
  if(!move.active() && !targetCount) return startTarget(ch,goal);
  for(uint8_t i=0; i<targetCount; ++i) if(targets[i].channel==ch) {
    targets[i].goal=goal;notice="";return true;
  }
  if(targetCount>=16) {notice="queue_full";return false;}
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

bool shutterPositionBusy() { return move.active() || cal.phase || targetCount; }
void shutterPositionManual(uint16_t mask, bool stopMotor) {
  // One automatic sequence at a time. Other radio commands must not delay its STOP.
  if (shutterPositionBusy()) cancel(stopMotor);
  for (uint8_t i=0;i<16;++i) if(mask & (uint16_t(1)<<i)) {travel[i].duration=0;setPosition(i,-1);}
  notice=""; dirty=true;
}
void shutterPositionRemote(uint16_t mask, JaroCmdType command) {
  // A received command has already changed the motor state. Do not override it.
  const bool affected=(move.active() && (mask & (uint16_t(1)<<move.channel))) ||
                      (cal.phase && (mask & (uint16_t(1)<<cal.channel)));
  if (affected) cancel(false);
  else if(mask) clearTargets(); // Do not resume queued automation after manual remote operation.
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
  if (previousEnable != config.position.enable) {previousEnable=config.position.enable;dirty=true;}
  if(shutterPositionBusy() && (!config.position.enable || setupMode || EspSysUtil::OTA::getInstance().isActive())) failure("cancelled");
  if((cal.phase && !available(cal.channel)) || (move.active() && (!available(move.channel) || !config.position.calibration[move.channel].valid()))) failure("cancelled");
  if(cal.phase && (now-cal.lease>6000 || ((cal.phase==2 || cal.phase==4) && now-cal.since>180000))) failure("timeout");
  const auto action=move.next(now);
  if(action==PositionMove::MoveToTarget) {
    if(send(move.channel,move.targetUp())) move.targetStarted(config.position.calibration[move.channel],jaroPositionSentAt());
    else failure("send_failed");
    dirty=true;
  } else if(action==PositionMove::Stop) {
    const uint8_t ch=move.channel,goal=move.target;
    const bool sent=jaroPositionSend(ch,CMD_STOP);
    move.cancel();setPosition(ch,sent?goal:-1);notice=sent?"":"send_failed";
    if(!sent) clearTargets();
    dirty=true;
  } else if(action==PositionMove::Complete) {setPosition(move.channel,move.target);dirty=true;}
  for(uint8_t ch=0;ch<16;++ch) {
    auto &t=travel[ch];
    if(t.duration && now-t.since>=t.duration) {t.duration=0;setPosition(ch,t.target);}
  }
  // Revalidate immediately before each queued move; disabled/uncalibrated channels are skipped.
  while(targetCount && !move.active() && !cal.phase && !jaroCommandsPending()) {
    const Target next=popTarget();dirty=true;
    if(!available(next.channel) || !shutterPositionReady(next.channel)) {notice="unavailable";continue;}
    startTarget(next.channel,next.goal);
  }
  // All timing stays on the ESP. Browsers only receive a small snapshot once per second.
  if (!EspSysUtil::OTA::getInstance().isActive() && (dirty || (shutterPositionBusy() && now-lastUpdate>=1000))) status();
}
