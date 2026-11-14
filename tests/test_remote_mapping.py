"""Run the real mapping, capture and persistence implementation without radio hardware."""
from pathlib import Path
import subprocess
import tempfile
from host_helpers import arduino_json_headers
ROOT=Path(__file__).resolve().parents[1]
HEADERS={
'Arduino.h':'#pragma once\n#include <cstdint>\n#include <cstring>\n#include <cstdio>\nuint32_t millis();\n',
'EspSysUtil.h':'#pragma once\nnamespace EspSysUtil {struct OTA {bool active=false;static OTA&getInstance(){static OTA x;return x;}bool isActive(){return active;}};}\n',
'config.h':'#pragma once\n#include <remoteMapping.h>\nstruct Config{RemoteMapping remoteMappings[REMOTE_MAPPING_LIMIT];struct{bool ch_enable[16]={};char ch_name[16][64]={};}jaro;};extern Config config;\n',
'webUI.h':'#pragma once\n#include <ArduinoJson.h>\n#include <string>\nstruct Web{std::string data;void wsUpdateWebJSON(JsonDocument&d){data.clear();serializeJson(d,data);}};extern Web webUI;\n'}
MAIN=r'''
#include <Arduino.h>
#include <config.h>
#include <webUI.h>
#include <EspSysUtil.h>
#include <cassert>
Config config;Web webUI;uint32_t clockMs=1000;uint32_t millis(){return clockMs;}
void command(const char*s){remoteMappingWebCommand(s);}
JsonDocument state(){JsonDocument d;assert(!deserializeJson(d,webUI.data));return d;}
void listen(unsigned ch=0){char s[100];snprintf(s,sizeof(s),"{\"a\":\"listen\",\"token\":7,\"channel\":%u}",ch);command(s);}
void signal(uint32_t serial=0xc7c0001,uint16_t mask=2){remoteMappingReceive(serial,mask,4);}
void confirmed(uint32_t serial=0xc7c0001,uint16_t mask=2){signal(serial,mask);clockMs+=500;signal(serial,mask);assert(state()["phase"]==3);}
void save(unsigned ch=0,const char*name="Wohnzimmer"){JsonDocument d;d["a"]="save";d["token"]=7;d["channel"]=ch;d["name"]=name;std::string s;serializeJson(d,s);command(s.c_str());}
void cancel(){command(R"({"a":"cancel","token":7,"channel":0})");}
int main(){
 for(unsigned ch=0;ch<16;++ch)config.jaro.ch_enable[ch]=true;
 listen();assert(state()["phase"]==1);
 command(R"({"a":"listen","token":8,"channel":0})");assert(state()["error"]=="busy");
 signal(0xc7c0000,0x3f);assert(state()["error"]=="group");assert(state()["phase"]==1);
 signal();assert(state()["phase"]==2);save();assert(state()["error"]=="no_signal");
 for(int i=0;i<10;++i){clockMs+=100;signal();}assert(state()["phase"]==2);
 clockMs+=500;signal(0xabc0001);assert(state()["error"]=="different");
 signal();assert(state()["phase"]==3);save();assert(state()["result"]=="saved");
 assert(remoteMappingReceive(0xc7c000f,2,8).targets==1);
 assert(remoteMappingReceive(0xc7c0000,0x3f,8).targets==1);
 assert(!remoteMappingReceive(0xc7d0000,2,8).targets);
 listen(1);confirmed();save(1);assert(remoteMappingReceive(0xc7c0000,0x3f,8).targets==3);
 command(R"({"a":"delete","token":7,"channel":0,"id":0,"serial":1234,"mask":2})");
 // Wrong/stale identity cannot delete another entry.
 assert(state()["result"]=="rejected");
 JsonDocument del;del["a"]="delete";del["token"]=7;del["channel"]=0;del["id"]=0;del["serial"]=0xc7c0001;del["mask"]=2;
 std::string payload;serializeJson(del,payload);command(payload.c_str());assert(state()["result"]=="deleted");assert(remoteMappingReceive(0xc7c000f,2,8).targets==2);
 listen();confirmed(0xc7c0000,1);save();assert(remoteMappingReceive(0xc7c000f,3,8).targets==3);
 // Different sender families on the two-channel wall remote must remain separate.
 listen(2);confirmed(0x04d46401,2);save(2);listen(3);confirmed(0x04d46501,2);save(3);
 assert(remoteMappingReceive(0x04d46401,2,8).targets==4);assert(remoteMappingReceive(0x04d46501,2,8).targets==8);
 // Channel 16 uses the high bit, including in a group telegram.
 listen(4);confirmed(0x004c000f,0x8000);save(4);assert(remoteMappingReceive(0x004c0000,0x8889,8).targets==16);
 listen(5);confirmed(0x004c000e,0x4000);save(5);assert(remoteMappingReceive(0x004c000e,0x403f,8).targets==32);
 // UTF-8 names are limited by Unicode characters, not bytes.
 std::string name;for(int i=0;i<32;++i)name+="😀";
 listen(6);confirmed(0x1111,1);save(6,name.c_str());assert(state()["result"]=="saved");
 listen(6);confirmed(0x1111,1);name+="x";save(6,name.c_str());assert(state()["error"]=="invalid");
 name=std::string("bad")+char(0xc0)+char(0x80);save(6,name.c_str());assert(state()["error"]=="invalid");
 clockMs+=120001;save(6);assert(state()["error"]=="no_signal");
 remoteMappingReset();for(auto&m:config.remoteMappings)m=RemoteMapping{};
 for(unsigned ch=0;ch<16;++ch)for(unsigned i=0;i<5;++i){listen(ch);confirmed(0x10000+ch*0x100+i*0x10,1);save(ch);assert(state()["result"]=="saved");}
 listen();confirmed(0x888888,1);save();assert(state()["error"]=="motor_full");cancel();
 JsonDocument stored;remoteMappingSave(stored.to<JsonObject>());assert(stored["remoteMappings"].size()==80);
 std::string saved;serializeJson(stored,saved);stored.clear();assert(!deserializeJson(stored,saved));remoteMappingLoad(stored["remoteMappings"],7);assert(remoteMappingReceive(0x1000f,1,8).targets==1);
 command(R"({"a":"status","channel":15})");assert(state()["items"].size()==5);
 JsonDocument old;auto rows=old.to<JsonArray>();
 auto row=rows.add<JsonObject>();row["name"]="old single";row["serial"]=0xc7c0001;row["sourceMask"]=2;row["targets"]=1;
 row=rows.add<JsonObject>();row["name"]="old group";row["serial"]=0xc7c0000;row["sourceMask"]=0x3f;row["targets"]=2;
 remoteMappingLoad(old,6);assert(remoteMappingReceive(0xc7c000f,0x3f,8).targets==1);assert(config.remoteMappings[1].legacyGroup);
 stored.clear();remoteMappingSave(stored.to<JsonObject>());saved.clear();serializeJson(stored,saved);stored.clear();assert(!deserializeJson(stored,saved));remoteMappingLoad(stored["remoteMappings"],7);assert(config.remoteMappings[1].legacyGroup);
 remoteMappingLoad(old,5);assert(!remoteMappingReceive(0xc7c000f,2,8).targets);
 listen();clockMs+=30001;remoteMappingCyclic();assert(state()["result"]=="expired");
 listen();EspSysUtil::OTA::getInstance().active=true;remoteMappingCyclic();assert(state()["result"]=="expired");
 printf("PASS: source/group matching, two presses, ownership, per-motor delete, Unicode, 5/80 limits, v6 migration, scoped snapshots, expiry and OTA. Mapping RAM: %zu bytes.\n",sizeof(config.remoteMappings));
}
'''
if __name__=='__main__':
 headers=arduino_json_headers()
 with tempfile.TemporaryDirectory() as d:
  temp=Path(d)
  for name,content in HEADERS.items():(temp/name).write_text(content)
  (temp/'main.cpp').write_text(MAIN)
  subprocess.run(['c++','-std=c++17','-I'+str(temp),'-I'+str(ROOT/'include'),'-I'+str(headers),str(ROOT/'src/remoteMapping.cpp'),str(temp/'main.cpp'),'-o',str(temp/'test')],check=True)
  subprocess.run([str(temp/'test')],check=True)
