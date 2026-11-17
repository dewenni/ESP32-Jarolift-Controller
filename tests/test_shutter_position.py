"""Host integration test of the actual position controller, with fake clock/radio/WebUI."""
from pathlib import Path
import subprocess
import tempfile
from host_helpers import arduino_json_headers
ROOT = Path(__file__).resolve().parents[1]
HEADERS = {
    'Arduino.h': '#pragma once\n#include <cstdint>\n#include <cstring>\nuint32_t millis();\n',
    'EspSysUtil.h': '#pragma once\nnamespace EspSysUtil { struct OTA { static OTA &getInstance(); bool active=false; bool isActive() {return active;} }; }\n',
    'config.h': '#pragma once\n#include <positionModel.h>\nstruct Config {struct {bool enable=false; PositionCalibration calibration[16];} position; struct {bool ch_enable[16]={};} jaro;};\nextern Config config; extern bool setupMode;\n',
    'jarolift.h': '#pragma once\n#include <cstdint>\nenum JaroCmdType {CMD_UP,CMD_DOWN,CMD_STOP,CMD_SHADE};\nvoid mqttSendPosition(uint8_t,int8_t); bool jaroPositionSend(uint8_t,JaroCmdType); uint32_t jaroPositionSentAt(); bool jaroCommandsPending();\n',
    'webUI.h': '#pragma once\n#include <ArduinoJson.h>\n#include <string>\nstruct Web {std::string data; void wsUpdateWebJSON(JsonDocument &doc) {data.clear();serializeJson(doc,data);}};extern Web webUI;\n',
}
MAIN = r'''
#include <Arduino.h>
#include <EspSysUtil.h>
#include <config.h>
#include <jarolift.h>
#include <webUI.h>
#include <shutterPosition.h>
#include <cassert>
#include <vector>
#include <cstdio>
uint32_t clockMs=1000; bool radioOK=true, queued=false;
Config config; bool setupMode=false; Web webUI;
std::vector<JaroCmdType> sent;
std::vector<int> mqttPositions;
void mqttSendPosition(uint8_t,int8_t p){mqttPositions.push_back(p);}
uint32_t millis(){return clockMs;}
EspSysUtil::OTA &EspSysUtil::OTA::getInstance(){static OTA ota;return ota;}
bool jaroPositionSend(uint8_t ch,JaroCmdType cmd){assert(ch<16);sent.push_back(cmd);return radioOK;}
uint32_t jaroPositionSentAt(){return clockMs;}
bool jaroCommandsPending(){return queued;}
void command(const char *text){shutterPositionWebCommand(text);shutterPositionCyclic();}
JsonDocument state(){JsonDocument d;assert(!deserializeJson(d,webUI.data));return d;}
void target(){command(R"({"a":"target","c":0,"v":25})");}
void begin(){command(R"({"a":"begin","c":0,"token":7})");}
void start(){command(R"({"a":"start","c":0,"token":7})");}
void markAt(uint32_t t){clockMs=t;command(R"({"a":"mark","c":0,"token":7})");}
int main(){
 config.position.enable=true;config.jaro.ch_enable[0]=true;
 const PositionCalibration calibration{{2400,15800,28600},{12200,26400,29800}};
 config.position.calibration[0]=calibration;
 // Received frames use the same state even offline; duplicates cannot postpone completion.
 shutterPositionRemote(1,CMD_UP);clockMs+=1000;shutterPositionRemote(1,CMD_UP);
 clockMs=1000+calibration.referenceTime(true);shutterPositionCyclic();assert(state()["channels"][0][1]==100 && mqttPositions.back()==100);
 shutterPositionRemote(1,CMD_DOWN);shutterPositionRemote(1,CMD_STOP);clockMs+=40000;shutterPositionCyclic();assert(state()["channels"][0][1]==-1 && mqttPositions.back()==-1);
 shutterPositionCommand(1,CMD_DOWN);clockMs+=calibration.referenceTime(false);shutterPositionCyclic();assert(state()["channels"][0][1]==0 && mqttPositions.back()==0);
 shutterPositionRemote(1,CMD_SHADE);shutterPositionCyclic();assert(state()["channels"][0][1]==-1);
 // One group mask updates every channel; an unrelated remote does not stop an auto move.
 config.jaro.ch_enable[1]=true;config.position.calibration[1]=calibration;
 shutterPositionCommand(3,CMD_UP);clockMs+=calibration.referenceTime(true);shutterPositionCyclic();assert(state()["channels"][0][1]==100 && state()["channels"][1][1]==100);
 shutterPositionPublish();assert(mqttPositions.back()==100);
 clockMs=1000;
 auto n=sent.size();
 assert(shutterPositionReady(0) && !shutterPositionReady(16));
 n=sent.size();
 for(const char *bad : {"", "offline", "-1", "+25", "25.5", " 25", "25x", "101", "9999999999"}) assert(!shutterPositionTarget(0,bad));
 assert(!shutterPositionTarget(16,"25"));assert(sent.size()==n);
 config.position.enable=false;assert(!shutterPositionTarget(0,"25"));config.position.enable=true;
 auto savedCalibration=config.position.calibration[0];config.position.calibration[0]=PositionCalibration{};assert(!shutterPositionTarget(0,"25"));config.position.calibration[0]=savedCalibration;
 assert(shutterPositionTarget(0,"25"));shutterPositionCyclic();assert(shutterPositionBusy() && sent.back()==CMD_DOWN);
 assert(state()["remaining"]==32);
 clockMs+=1000;shutterPositionCyclic();assert(state()["remaining"]==31);
 n=sent.size();begin();assert(sent.size()==n && state()["notice"]=="busy");
 clockMs=1000+calibration.referenceTime(false);shutterPositionCyclic();assert(state()["phase"]==int(PositionMove::Settle) && state()["remaining"]==0);
 clockMs+=600;shutterPositionCyclic();assert(sent.back()==CMD_UP);
 clockMs+=8990;shutterPositionCyclic();assert(sent.back()==CMD_STOP && !shutterPositionBusy());assert(state()["channels"][0][1]==25 && mqttPositions.back()==25);
 target();n=sent.size();shutterPositionRemote(1);shutterPositionCyclic();assert(!shutterPositionBusy() && sent.size()==n);assert(state()["channels"][0][1]==-1);
 target();shutterPositionManual(1);assert(!shutterPositionBusy() && sent.back()==CMD_STOP);
 radioOK=false;target();assert(!shutterPositionBusy() && state()["notice"]=="send_failed");radioOK=true;
 queued=true;target();assert(!shutterPositionBusy());queued=false;
 n=sent.size();command(R"({"a":"target","c":0,"v":101})");command(R"({"a":"target","c":16,"v":20})");command(R"({"a":"target","c":0,"v":"25"})");assert(sent.size()==n);
 begin();assert(state()["cal"]["phase"]==1);start();assert(sent.back()==CMD_UP);
 n=sent.size();command(R"({"a":"cancel","c":0,"token":8})");assert(shutterPositionBusy() && sent.size()==n);
 clockMs+=6001;shutterPositionCyclic();assert(!shutterPositionBusy() && sent.back()==CMD_STOP && state()["notice"]=="timeout");assert(config.position.calibration[0].up[2]==28600);
 // Guided calibration, no arbitrary client-provided times are accepted.
 begin();start();uint32_t t=clockMs;
 markAt(t+2400);markAt(t+15800);markAt(t+28600);assert(state()["cal"]["phase"]==3);
 start();t=clockMs;markAt(t+12200);markAt(t+26400);markAt(t+29800);assert(state()["cal"]["phase"]==5);
 command(R"({"a":"save","c":0,"token":7})");assert(!shutterPositionBusy() && config.position.calibration[0].valid() && state()["notice"]=="saved");
 target();config.position.enable=false;shutterPositionCyclic();assert(!shutterPositionBusy() && sent.back()==CMD_STOP);
 config.position.enable=true;target();EspSysUtil::OTA::getInstance().active=true;shutterPositionCyclic();assert(!shutterPositionBusy() && sent.back()==CMD_STOP);EspSysUtil::OTA::getInstance().active=false;
 shutterPositionInvalidate(0);assert(!config.position.calibration[0].valid());

 // Endpoint commands do not own the position FIFO or wait for motor travel.
 for(uint8_t ch=0;ch<16;++ch){config.jaro.ch_enable[ch]=true;config.position.calibration[ch]=calibration;}
 n=sent.size();
 for(uint8_t ch=0;ch<16;++ch)assert(shutterPositionTarget(ch,"0"));
 shutterPositionCyclic();assert(sent.size()==n+16 && !shutterPositionBusy() && state()["queue"].size()==0);
 clockMs+=40000;shutterPositionCyclic();assert(state()["channels"][15][1]==0);
 assert(shutterPositionTarget(0,"100"));assert(!shutterPositionBusy());
 clockMs+=40000;shutterPositionCyclic();assert(state()["channels"][0][1]==100);
 // Intermediate moves retain serialization, including queued endpoints, to protect STOP timing.
 assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"0"));assert(shutterPositionTarget(2,"100"));
 shutterPositionCyclic();assert(state()["queue"].size()==2);
 clockMs+=40000;shutterPositionCyclic();clockMs+=600;shutterPositionCyclic();clockMs+=40000;shutterPositionCyclic();
 assert(!shutterPositionBusy() && state()["queue"].size()==0 && sent.back()==CMD_UP);
 // Fixed shared FIFO still holds 16 intermediate goals, replacing a channel in place.
 assert(shutterPositionTarget(0,"25"));
 for(uint8_t ch=0;ch<16;++ch)assert(shutterPositionTarget(ch,"25"));
 assert(shutterPositionTarget(7,"75"));shutterPositionCyclic();
 assert(state()["queue"].size()==16 && state()["queue"][7][1]==75);
 shutterPositionManual(1);shutterPositionCyclic();assert(!shutterPositionBusy());
 // Normal manual commands discard the complete queue and stop the active automatic run.
 assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"75"));
 shutterPositionManual(2);shutterPositionCyclic();assert(!shutterPositionBusy() && state()["queue"].size()==0 && sent.back()==CMD_STOP);
 // Remote operation cancels pending automation, but unrelated ongoing motion is not overridden.
 assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"75"));
 n=sent.size();shutterPositionRemote(2,CMD_UP);shutterPositionCyclic();
 assert(shutterPositionBusy() && state()["queue"].size()==0 && sent.size()==n);
 shutterPositionRemote(1,CMD_STOP);shutterPositionCyclic();assert(!shutterPositionBusy());
 // No queue survives disable/setup/OTA or a radio failure at STOP.
 for(uint8_t mode=0;mode<3;++mode){
   assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"75"));
   if(mode==0)config.position.enable=false;
   if(mode==1)setupMode=true;
   if(mode==2)EspSysUtil::OTA::getInstance().active=true;
   shutterPositionCyclic();assert(!shutterPositionBusy());
   config.position.enable=true;setupMode=false;EspSysUtil::OTA::getInstance().active=false;
   shutterPositionCyclic();assert(state()["queue"].size()==0);
 }
 assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"75"));
 clockMs+=40000;shutterPositionCyclic();clockMs+=600;shutterPositionCyclic();
 radioOK=false;clockMs+=40000;shutterPositionCyclic();radioOK=true;
 assert(!shutterPositionBusy() && state()["queue"].size()==0 && state()["notice"]=="send_failed");
 begin();assert(!shutterPositionTarget(1,"75"));command(R"({"a":"cancel","c":0,"token":7})");
 puts("PASS: actual controller: target sequence, busy/invalid requests, radio failure, manual/remote abort, lease/ownership, calibration commit, disable, OTA, invalidation");
}
'''
if __name__ == '__main__':
    headers = arduino_json_headers()
    with tempfile.TemporaryDirectory() as directory:
        temp=Path(directory)
        for name, content in HEADERS.items():
            (temp/name).write_text(content)
        (temp/'main.cpp').write_text(MAIN)
        subprocess.run(['c++','-std=c++17','-I'+str(temp),'-I'+str(ROOT/'include'),'-I'+str(headers),str(ROOT/'src/shutterPosition.cpp'),str(temp/'main.cpp'),'-o',str(temp/'test')],check=True)
        subprocess.run([str(temp/'test')],check=True)
