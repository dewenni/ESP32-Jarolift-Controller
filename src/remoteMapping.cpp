#include <Arduino.h>
#include <ArduinoJson.h>
#include <EspSysUtil.h>
#include <config.h>
#include <remoteMapping.h>
#include <webUI.h>

namespace {
// Capture is a short lease shared by all browser clients. RF repeats in one
// button press do not confirm a source; release and press again to confirm.
struct Capture {
  uint32_t token=0, since=0, lastReceived=0, serial=0;
  uint16_t mask=0;
  uint8_t channel=0, phase=0, command=0;
} capture;
bool single(uint16_t mask) {return mask && !(mask & (mask-1));}
uint32_t sender(uint32_t serial) {return serial & 0x0ffffff0;}
int find(uint32_t serial,uint16_t mask) {
  for(uint8_t i=0;i<REMOTE_MAPPING_LIMIT;++i) {
    const auto &m=config.remoteMappings[i];
    if(remoteMappingValid(m) && !m.legacyGroup && sender(m.serial)==sender(serial) && m.sourceMask==mask)return i;
  }
  return -1;
}
uint8_t countFor(uint8_t ch) {
  uint8_t count=0;
  for(const auto &m:config.remoteMappings)if(remoteMappingValid(m) && (m.targets & (uint16_t(1)<<ch)))++count;
  return count;
}
// Validate UTF-8, counting Unicode code points rather than storage bytes.
// Reject controls, overlong sequences, surrogates and incomplete sequences.
bool validName(const char *name) {
  const auto *p=reinterpret_cast<const unsigned char *>(name);
  unsigned count=0,bytes=0;
  while(*p) {
    uint32_t cp=*p++;unsigned trailing=0;uint32_t minimum=0;
    if(cp<0x80) {if(cp<32 || cp==127)return false;}
    else if(cp>=0xc2 && cp<=0xdf){cp&=31;trailing=1;minimum=0x80;}
    else if(cp>=0xe0 && cp<=0xef){cp&=15;trailing=2;minimum=0x800;}
    else if(cp>=0xf0 && cp<=0xf4){cp&=7;trailing=3;minimum=0x10000;}
    else return false;
    bytes+=trailing+1;
    while(trailing--){if((*p & 0xc0)!=0x80)return false;cp=(cp<<6)|(*p++ & 63);}
    if(cp<minimum || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff) || ++count>REMOTE_MAPPING_NAME_CHARS || bytes>=sizeof(RemoteMapping::name))return false;
  }
  return count>0 && name[0]!=' ' && name[bytes-1]!=' ';
}
void snapshot(uint8_t ch,uint32_t reply=0,const char *result="",const char *error="") {
  JsonDocument doc;
  doc["type"]="remoteMappings";doc["channel"]=ch;doc["channelName"]=config.jaro.ch_name[ch];
  doc["enabled"]=config.jaro.ch_enable[ch];doc["reply"]=reply;doc["result"]=result;doc["error"]=error;
  doc["token"]=capture.token;doc["phase"]=capture.phase;doc["captureChannel"]=capture.channel;
  doc["serial"]=capture.serial;doc["mask"]=capture.mask;doc["command"]=capture.command;
  doc["limit"]=REMOTE_MAPPING_PER_SHUTTER;
  const int existing=find(capture.serial,capture.mask);
  if(existing>=0)doc["existingName"]=config.remoteMappings[existing].name;
  auto rows=doc["items"].to<JsonArray>();
  // Send only the selected motor's associations, never the complete 80-row array.
  for(uint8_t i=0;i<REMOTE_MAPPING_LIMIT;++i) {
    const auto &m=config.remoteMappings[i];
    if(!remoteMappingValid(m) || !(m.targets & (uint16_t(1)<<ch)))continue;
    auto row=rows.add<JsonObject>();row["id"]=i;row["name"]=m.name;row["serial"]=m.serial;
    row["mask"]=m.sourceMask;row["targets"]=m.targets;row["legacy"]=m.legacyGroup;
  }
  webUI.wsUpdateWebJSON(doc);
}
void expire() {
  if(capture.token && (millis()-capture.since>(capture.phase<3?30000u:120000u) || EspSysUtil::OTA::getInstance().isActive())) {
    const auto old=capture;capture=Capture{};snapshot(old.channel,old.token,"expired","timeout");
  }
}
}
bool remoteMappingValid(const RemoteMapping &m) {
  return m.targets && m.sourceMask && m.serial<=0x0fffffff && m.name[0] && (m.legacyGroup || single(m.sourceMask));
}
void remoteMappingReset() {capture=Capture{};}
RemoteMappingMatch remoteMappingReceive(uint32_t serial,uint16_t mask,uint8_t command) {
  expire();RemoteMappingMatch match;
  if(serial>0x0fffffff || !mask || (command!=2 && command!=3 && command!=4 && command!=8))return match;
  for(const auto &m:config.remoteMappings) {
    if(!remoteMappingValid(m) || m.legacyGroup || sender(m.serial)!=sender(serial) || !(m.sourceMask & mask))continue;
    match.targets|=m.targets;
    match.name=match.name?"multiple":m.name;
  }
  if(capture.token && capture.phase<3) {
    if(!single(mask)) {snapshot(capture.channel,0,"","group");return match;}
    const uint32_t now=millis();
    if(capture.phase==1) {
      capture.serial=serial;capture.mask=mask;capture.command=command;capture.phase=2;
      capture.lastReceived=now;capture.since=now;snapshot(capture.channel);
    } else if(sender(serial)==sender(capture.serial) && mask==capture.mask) {
      const bool released=now-capture.lastReceived>=400;
      capture.lastReceived=now;
      if(released){capture.phase=3;capture.since=now;snapshot(capture.channel);}
    } else snapshot(capture.channel,0,"","different");
  }
  return match;
}
void remoteMappingWebCommand(const char *payload) {
  expire();JsonDocument req;
  if(deserializeJson(req,payload) || !req.is<JsonObject>() || !req["channel"].is<unsigned int>() || req["channel"].as<unsigned int>()>=16)return;
  const uint8_t ch=req["channel"];const char *action=req["a"]|"";
  const uint32_t token=req["token"]|uint32_t(0);
  if(!strcmp(action,"status")){snapshot(ch,token);return;}
  if(!token)return;
  if(capture.token && (capture.token!=token || capture.channel!=ch)){snapshot(ch,token,"rejected","busy");return;}
  if(!strcmp(action,"cancel")){remoteMappingReset();snapshot(ch,token,"cancelled");return;}
  if(EspSysUtil::OTA::getInstance().isActive()){snapshot(ch,token,"rejected","unavailable");return;}
  if(!strcmp(action,"listen")) {
    if(!config.jaro.ch_enable[ch]){snapshot(ch,token,"rejected","targets");return;}
    capture=Capture{};capture.token=token;capture.channel=ch;capture.phase=1;capture.since=millis();snapshot(ch,token);return;
  }
  if(!strcmp(action,"delete")) {
    if(!req["id"].is<unsigned int>() || req["id"].as<unsigned int>()>=REMOTE_MAPPING_LIMIT){snapshot(ch,token,"rejected","invalid");return;}
    auto &m=config.remoteMappings[req["id"].as<unsigned int>()];
    if(!remoteMappingValid(m) || !(m.targets & (uint16_t(1)<<ch)) || req["serial"].as<uint32_t>()!=m.serial || req["mask"].as<uint16_t>()!=m.sourceMask){snapshot(ch,token,"rejected","invalid");return;}
    m.targets&=~(uint16_t(1)<<ch);if(!m.targets)m=RemoteMapping{};
    remoteMappingReset();snapshot(ch,token,"deleted");return;
  }
  if(!strcmp(action,"save")) {
    if(capture.token!=token || capture.phase!=3){snapshot(ch,token,"rejected","no_signal");return;}
    const char *name=req["name"]|"";
    if(!validName(name)){snapshot(ch,token,"rejected","invalid");return;}
    if(!config.jaro.ch_enable[ch]){snapshot(ch,token,"rejected","targets");return;}
    int id=find(capture.serial,capture.mask);
    if((id<0 || !(config.remoteMappings[id].targets & (uint16_t(1)<<ch))) && countFor(ch)>=REMOTE_MAPPING_PER_SHUTTER){snapshot(ch,token,"rejected","motor_full");return;}
    if(id<0)for(uint8_t i=0;i<REMOTE_MAPPING_LIMIT;++i)if(!remoteMappingValid(config.remoteMappings[i])){id=i;break;}
    if(id<0){snapshot(ch,token,"rejected","full");return;}
    auto &m=config.remoteMappings[id];snprintf(m.name,sizeof(m.name),"%s",name);
    m.serial=capture.serial;m.sourceMask=capture.mask;m.targets|=uint16_t(1)<<ch;m.legacyGroup=false;
    remoteMappingReset();snapshot(ch,token,"saved");
  }
}
void remoteMappingCyclic() {expire();}
void remoteMappingSave(JsonObject root) {
  auto rows=root["remoteMappings"].to<JsonArray>();
  for(const auto &m:config.remoteMappings)if(remoteMappingValid(m)) {
    auto row=rows.add<JsonObject>();row["name"]=m.name;row["serial"]=m.serial;row["sourceMask"]=m.sourceMask;row["targets"]=m.targets;
    if(m.legacyGroup)row["legacy"]=true;
  }
}
void remoteMappingLoad(JsonVariantConst entries,int version) {
  remoteMappingReset();for(auto &m:config.remoteMappings)m=RemoteMapping{};
  if(version<6)return; // Prefix-only mappings contain no recoverable source bit.
  uint8_t count=0;
  for(JsonObjectConst row:entries.as<JsonArrayConst>()) {
    if(count>=REMOTE_MAPPING_LIMIT)break;
    if(!row["serial"].is<uint32_t>() || row["serial"].as<uint32_t>()>0x0fffffff ||
       !row["sourceMask"].is<unsigned int>() || row["sourceMask"].as<unsigned int>()>65535 ||
       !row["targets"].is<unsigned int>() || row["targets"].as<unsigned int>()>65535)continue;
    RemoteMapping m;const char *name=row["name"]|"";
    // v6 allowed longer names. Keep the first 32 complete UTF-8 code points.
    char migrated[sizeof(RemoteMapping::name)];
    if(version==6) {
      size_t bytes=0;unsigned chars=0;
      while(name[bytes] && bytes<sizeof(migrated)-1) {
        if((static_cast<unsigned char>(name[bytes]) & 0xc0)!=0x80 && chars++==REMOTE_MAPPING_NAME_CHARS)break;
        migrated[bytes]=name[bytes];++bytes;
      }
      while(bytes && migrated[bytes-1]==' ')--bytes;
      migrated[bytes]='\0';name=migrated;
    }
    if(!validName(name))continue;
    snprintf(m.name,sizeof(m.name),"%s",name);m.serial=row["serial"];m.sourceMask=row["sourceMask"];m.targets=row["targets"];
    m.legacyGroup=version==6?!single(m.sourceMask):(row["legacy"]|false);
    if(!remoteMappingValid(m))continue;
    const int duplicate=m.legacyGroup?-1:find(m.serial,m.sourceMask);
    // Preserve existing v6 relationships, including over-limit imported lists;
    // adding more is blocked until the user removes excess associations.
    if(!m.targets)continue;
    if(duplicate>=0)config.remoteMappings[duplicate].targets|=m.targets;
    else config.remoteMappings[count++]=m;
  }
}
