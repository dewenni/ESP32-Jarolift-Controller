#include <Arduino.h>
#include <ArduinoJson.h>
#include <EspSysUtil.h>
#include <config.h>
#include <remoteMapping.h>
#include <webUI.h>

namespace {
struct Capture {
  uint32_t token=0, since=0, serial=0;
  uint16_t mask=0;
  int id=-1;
  uint8_t command=0;
  bool listening=false, captured=false;
} capture;
const char *error="";
int find(uint32_t serial,uint16_t mask) {
  for(uint8_t i=0;i<REMOTE_MAPPING_LIMIT;++i) {
    const auto &m=config.remoteMappings[i];
    if(remoteMappingValid(m) && m.serial==serial && m.sourceMask==mask)return i;
  }
  return -1;
}
void snapshot(uint32_t reply=0,const char *result="") {
  JsonDocument doc;doc["type"]="remoteMappings";doc["reply"]=reply;doc["result"]=result;doc["error"]=error;
  doc["token"]=capture.token;doc["listening"]=capture.listening;doc["captured"]=capture.captured;
  doc["id"]=capture.id;doc["serial"]=capture.serial;doc["mask"]=capture.mask;doc["command"]=capture.command;doc["limit"]=REMOTE_MAPPING_LIMIT;
  auto rows=doc["items"].to<JsonArray>();
  for(uint8_t i=0;i<REMOTE_MAPPING_LIMIT;++i) {
    const auto &m=config.remoteMappings[i];if(!remoteMappingValid(m))continue;
    auto row=rows.add<JsonObject>();row["id"]=i;row["name"]=m.name;row["serial"]=m.serial;row["mask"]=m.sourceMask;row["targets"]=m.targets;
  }
  auto channels=doc["channels"].to<JsonArray>();
  for(uint8_t ch=0;ch<16;++ch) {
    auto row=channels.add<JsonArray>();row.add(config.jaro.ch_name[ch]);row.add(config.jaro.ch_enable[ch]);
  }
  webUI.wsUpdateWebJSON(doc);
}
}
bool remoteMappingValid(const RemoteMapping &m) {
  return m.targets && m.sourceMask && m.serial<=0x0fffffff && m.name[0];
}
void remoteMappingReset() {capture=Capture{};error="";}
const RemoteMapping *remoteMappingReceive(uint32_t serial,uint16_t mask,uint8_t command) {
  const int id=find(serial,mask);
  if(capture.listening && mask && serial<=0x0fffffff &&
     (command==0x2 || command==0x3 || command==0x4 || command==0x8)) {
    error="";capture.serial=serial;capture.mask=mask;capture.command=command;
    capture.listening=false;capture.captured=true;capture.since=millis();
    // Adding an existing signal edits that row instead of creating a duplicate.
    if(capture.id<0 && id>=0)capture.id=id;
    snapshot();
  }
  return id<0?nullptr:&config.remoteMappings[id];
}
void remoteMappingWebCommand(const char *payload) {
  JsonDocument req;if(deserializeJson(req,payload) || !req.is<JsonObject>())return;
  const char *action=req["a"]|"";
  const uint32_t token=req["token"]|uint32_t(0);
  if(!strcmp(action,"status")){snapshot();return;}
  if(!token)return;
  if(capture.token && capture.token!=token){error="busy";snapshot(token,"rejected");return;}
  error="";
  if(!strcmp(action,"cancel")){remoteMappingReset();snapshot(token,"cancelled");return;}
  if(EspSysUtil::OTA::getInstance().isActive()){error="unavailable";snapshot(token,"rejected");return;}
  if(!strcmp(action,"listen") || !strcmp(action,"edit")) {
    if(!req["id"].is<int>()){error="invalid";snapshot(token,"rejected");return;}
    const int id=req["id"];
    if(id < -1 || id>=REMOTE_MAPPING_LIMIT || (id>=0 && !remoteMappingValid(config.remoteMappings[id])) || (!strcmp(action,"edit") && id<0)) {
      error="invalid";snapshot(token,"rejected");return;
    }
    capture=Capture{};capture.token=token;capture.id=id;capture.since=millis();
    capture.listening=!strcmp(action,"listen");
    if(!capture.listening){capture.serial=config.remoteMappings[id].serial;capture.mask=config.remoteMappings[id].sourceMask;capture.captured=true;}
    snapshot();return;
  }
  if(!strcmp(action,"delete")) {
    if(!req["id"].is<unsigned int>() || req["id"].as<unsigned int>()>=REMOTE_MAPPING_LIMIT){error="invalid";snapshot(token,"rejected");return;}
    config.remoteMappings[req["id"].as<unsigned int>()]=RemoteMapping{};
    remoteMappingReset();snapshot(token,"deleted");return;
  }
  if(!strcmp(action,"save")) {
    if(capture.token!=token || !capture.captured){error="no_signal";snapshot(token,"rejected");return;}
    const char *name=req["name"]|"";
    if(!*name || strlen(name)>=sizeof(RemoteMapping::name) || !req["targets"].is<unsigned int>() || req["targets"].as<unsigned int>()>65535) {
      error="invalid";snapshot(token,"rejected");return;
    }
    const uint16_t targets=req["targets"];uint16_t enabled=0;
    for(uint8_t ch=0;ch<16;++ch)if(config.jaro.ch_enable[ch])enabled|=uint16_t(1)<<ch;
    if(!targets || (targets & ~enabled)){error="targets";snapshot(token,"rejected");return;}
    int id=capture.id;const int duplicate=find(capture.serial,capture.mask);
    if(duplicate>=0 && duplicate!=id){error="duplicate";snapshot(token,"rejected");return;}
    if(id<0)for(uint8_t i=0;i<REMOTE_MAPPING_LIMIT;++i)if(!remoteMappingValid(config.remoteMappings[i])){id=i;break;}
    if(id<0){error="full";snapshot(token,"rejected");return;}
    auto &m=config.remoteMappings[id];m=RemoteMapping{};snprintf(m.name,sizeof(m.name),"%s",name);
    m.serial=capture.serial;m.sourceMask=capture.mask;m.targets=targets;
    remoteMappingReset();snapshot(token,"saved");
  }
}
void remoteMappingCyclic() {
  if(capture.token && (millis()-capture.since>(capture.listening?30000u:120000u) || EspSysUtil::OTA::getInstance().isActive())) {
    const auto token=capture.token;remoteMappingReset();error="timeout";snapshot(token,"expired");
  }
}

void remoteMappingSave(JsonObject root) {
  JsonArray rows=root["remoteMappings"].to<JsonArray>();
  for(const auto &m:config.remoteMappings)if(remoteMappingValid(m)) {
    auto row=rows.add<JsonObject>();row["name"]=m.name;row["serial"]=m.serial;row["sourceMask"]=m.sourceMask;row["targets"]=m.targets;
  }
}
void remoteMappingLoad(JsonVariantConst entries,int version) {
  remoteMappingReset();for(auto &m:config.remoteMappings)m=RemoteMapping{};
  // Prefix-only legacy rows cannot be converted to an unambiguous signal identity.
  if(version<6)return;
  uint8_t count=0;
  for(JsonObjectConst row:entries.as<JsonArrayConst>()) {
    if(count>=REMOTE_MAPPING_LIMIT)break;
    if(!row["serial"].is<uint32_t>() || row["serial"].as<uint32_t>()>0x0fffffff ||
       !row["sourceMask"].is<unsigned int>() || row["sourceMask"].as<unsigned int>()>65535 ||
       !row["targets"].is<unsigned int>() || row["targets"].as<unsigned int>()>65535)continue;
    RemoteMapping m;const char *name=row["name"]|"";if(strlen(name)>=sizeof(m.name))continue;
    snprintf(m.name,sizeof(m.name),"%s",name);m.serial=row["serial"];m.sourceMask=row["sourceMask"];m.targets=row["targets"];
    if(remoteMappingValid(m) && find(m.serial,m.sourceMask)<0)config.remoteMappings[count++]=m;
  }
}
