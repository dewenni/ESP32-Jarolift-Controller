"""Host integration test of the actual position controller, with fake clock/radio/WebUI."""
from pathlib import Path
import subprocess
import tempfile
from host_helpers import arduino_json_headers
ROOT = Path(__file__).resolve().parents[1]
HEADERS = {
    'Arduino.h': '#pragma once\n#include <cstdint>\n#include <cstring>\nuint32_t millis();\n',
    'EspSysUtil.h': '#pragma once\nnamespace EspSysUtil { struct OTA { static OTA &getInstance(); bool active=false; bool isActive() {return active;} }; }\n',
    'config.h': '#pragma once\n#include <positionModel.h>\nstruct Config {struct {bool enable=false; bool reference=true; PositionCalibration calibration[16];} position; struct {bool ch_enable[16]={};} jaro;};\nextern Config config; extern bool setupMode;\n',
    'jarolift.h': '#pragma once\n#include <cstdint>\nenum JaroCmdType {CMD_UP,CMD_DOWN,CMD_STOP,CMD_SHADE};\nvoid mqttSendPosition(uint8_t,int8_t); bool jaroPositionSend(uint8_t,JaroCmdType); bool jaroPositionGroupSend(uint16_t,JaroCmdType); uint32_t jaroPositionSentAt(); bool jaroCommandsPending();\n',
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
uint32_t clockMs=1000, lastTx=0; bool radioOK=true, queued=false;
Config config; bool setupMode=false; Web webUI;
struct Tx {uint16_t mask;JaroCmdType cmd;uint32_t start,first;bool group;};
std::vector<Tx> sent;
std::vector<int> mqttPositions;
void mqttSendPosition(uint8_t,int8_t p){mqttPositions.push_back(p);}
uint32_t millis(){return clockMs;}
EspSysUtil::OTA &EspSysUtil::OTA::getInstance(){static OTA ota;return ota;}
bool transmit(uint16_t mask,JaroCmdType cmd,bool group){
 assert(mask);lastTx=clockMs+101;sent.push_back({mask,cmd,clockMs,lastTx,group});
 clockMs+=340;return radioOK;
}
bool jaroPositionSend(uint8_t ch,JaroCmdType cmd){assert(ch<16);return transmit(uint16_t(1)<<ch,cmd,false);}
bool jaroPositionGroupSend(uint16_t mask,JaroCmdType cmd){return transmit(mask,cmd,true);}
uint32_t jaroPositionSentAt(){return lastTx;}
bool jaroCommandsPending(){return queued;}
void command(const char *text){shutterPositionWebCommand(text);shutterPositionCyclic();}
JsonDocument state(){shutterPositionWebCommand(R"({"a":"status"})");JsonDocument d;assert(!deserializeJson(d,webUI.data));return d;}
void runFor(uint32_t ms){const uint32_t since=clockMs;while(clockMs-since<ms){const uint32_t left=ms-(clockMs-since);clockMs+=left<5?left:5;shutterPositionCyclic();}}
void target(){assert(shutterPositionTarget(0,"25"));runFor(550);}
void begin(){command(R"({"a":"begin","c":0,"token":7})");}
void start(){command(R"({"a":"start","c":0,"token":7})");}
void markAt(uint32_t t){clockMs=t;command(R"({"a":"mark","c":0,"token":7})");}
int main(){
 config.position.enable=true;
 const PositionCalibration calibration{{2400,15800,28600},{12200,26400,29800}};
 for(uint8_t ch=0;ch<16;++ch){config.jaro.ch_enable[ch]=true;config.position.calibration[ch]=calibration;}
 // Received commands share estimates offline; repeated frames do not postpone endpoints.
 shutterPositionRemote(1,CMD_UP);clockMs+=1000;shutterPositionRemote(1,CMD_UP);
 clockMs=1000+calibration.referenceTime(true);shutterPositionCyclic();assert(state()["channels"][0][1]==100 && mqttPositions.back()==100);
 shutterPositionRemote(1,CMD_STOP);runFor(40000);assert(state()["channels"][0][1]==-1);
 shutterPositionCommand(3,CMD_DOWN);runFor(32000);assert(state()["channels"][0][1]==0 && state()["channels"][1][1]==0);
 shutterPositionPublish();assert(mqttPositions.back()==-1);
 auto n=sent.size();
 for(const char *bad : {"", "offline", "-1", "+25", "25.5", " 25", "25x", "101", "9999999999"}) assert(!shutterPositionTarget(0,bad));
 assert(!shutterPositionTarget(16,"25"));assert(sent.size()==n);
 config.position.enable=false;assert(!shutterPositionTarget(0,"25"));config.position.enable=true;
 config.position.calibration[0]={};assert(!shutterPositionTarget(0,"25"));config.position.calibration[0]=calibration;
 queued=true;assert(!shutterPositionTarget(0,"25"));queued=false;
 command(R"({"a":"target","c":0,"v":101})");command(R"({"a":"target","c":0,"v":"25"})");assert(sent.size()==n);
 // Collect first, replace in place, then one reference telegram per direction, including endpoints.
 assert(shutterPositionTarget(0,"20"));assert(shutterPositionTarget(0,"25"));
 assert(shutterPositionTarget(1,"75"));assert(shutterPositionTarget(2,"0"));assert(shutterPositionTarget(3,"100"));
 assert(state()["queue"].size()==4 && state()["queue"][0][1]==25);
 runFor(199);assert(sent.size()==n);runFor(700);
 assert(sent.size()==n+2 && sent[n].group && sent[n].mask==5 && sent[n].cmd==CMD_DOWN);
 assert(sent[n+1].mask==10 && sent[n+1].cmd==CMD_UP);
 assert(state()["moves"].size()==2 && state()["queue"].size()==0);
 begin();assert(state()["notice"]=="busy");
 runFor(45000);assert(!shutterPositionBusy());
 assert(state()["channels"][0][1]==25 && state()["channels"][1][1]==75);
 assert(state()["channels"][2][1]==0 && state()["channels"][3][1]==100);
 // Each member uses its own reference duration; a STOP never includes a still-referencing channel.
 config.position.calibration[0].down[0]=500;config.position.calibration[0].down[1]=1500;config.position.calibration[0].down[2]=2000;
 n=sent.size();assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"25"));runFor(15000);
 assert(sent.size()==n+3 && sent[n].mask==3 && sent.back().cmd==CMD_STOP && sent.back().mask==1);
 assert(state()["moves"].size()==1 && state()["moves"][0][0]==1 && state()["moves"][0][1]==1);
 shutterPositionManual(0xffff);config.position.calibration[0]=calibration;
 // Sixteen simultaneous reference moves, sixteen waiting follow-ups; replacement keeps order.
 for(uint8_t ch=0;ch<16;++ch)assert(shutterPositionTarget(ch,"25"));
 runFor(550);assert(state()["moves"].size()==16);
 for(uint8_t ch=0;ch<16;++ch)assert(shutterPositionTarget(ch,"25"));
 assert(shutterPositionTarget(7,"75"));assert(state()["queue"].size()==16 && state()["queue"][7][1]==75);
 n=sent.size();shutterPositionManual(2);assert(!shutterPositionBusy() && sent.size()==n+1);
 assert(sent.back().group && sent.back().mask==0xffff && sent.back().cmd==CMD_STOP);
 // A mapped remote cancels only affected moves, clears waiting targets, sends no countercommand.
 assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"75"));runFor(900);
 assert(shutterPositionTarget(2,"50"));n=sent.size();shutterPositionRemote(1,CMD_STOP);
 assert(state()["moves"].size()==1 && state()["moves"][0][0]==1 && state()["queue"].size()==0 && sent.size()==n);
 shutterPositionRemote(2,CMD_UP);assert(!shutterPositionBusy() && sent.size()==n);
 // Direct mode: unknown starts rejected; endpoints still grouped and estimated from first frame.
 config.position.reference=false;shutterPositionRemote(0xffff,CMD_STOP);
 assert(!shutterPositionTarget(0,"25"));assert(state()["notice"]=="position_unknown");
 n=sent.size();for(uint8_t ch=0;ch<16;++ch)assert(shutterPositionTarget(ch,"0"));
 runFor(550);assert(sent.size()==n+1 && sent.back().mask==0xffff && !shutterPositionBusy());
 assert(!shutterPositionTarget(0,"25"));runFor(32000);assert(state()["channels"][15][1]==0);
 // Sixteen direct starts are staggered, STOPs group; every STOP reaches its modeled target within 1 s.
 n=sent.size();for(uint8_t ch=0;ch<16;++ch)assert(shutterPositionTarget(ch,"25"));
 runFor(6000);assert(state()["moves"].size()==16);runFor(12000);assert(!shutterPositionBusy());
 unsigned stops=0;
 for(uint8_t ch=0;ch<16;++ch){
   const Tx *up=nullptr,*stop=nullptr;
   for(size_t i=n;i<sent.size();++i)if(sent[i].mask&(uint16_t(1)<<ch)){
     if(sent[i].cmd==CMD_UP)up=&sent[i];if(sent[i].cmd==CMD_STOP)stop=&sent[i];
   }
   assert(up && stop);int32_t error=stop->first-(up->first+calibration.timeTo(25,true));
   assert(error>=-1000 && error<=1000);assert(state()["channels"][ch][1]==25);
 }
 for(size_t i=n;i<sent.size();++i)if(sent[i].cmd==CMD_STOP)++stops;
 assert(stops<8); // No serialized 16-STOP burst.
 // WebUI-like bursts of short direct runs: do not stall the fifth start on an extra 500 ms reserve.
 // Establish 50% without assuming a measured position; complete the actual controller move first.
 for(uint8_t ch=0;ch<16;++ch)assert(shutterPositionTarget(ch,"50"));runFor(18000);
 n=sent.size();for(uint8_t ch=0;ch<6;++ch)assert(shutterPositionTarget(ch,"58"));
 clockMs+=200;for(uint8_t ch=0;ch<6;++ch)shutterPositionCyclic();
 assert(sent.size()==n+6 && state()["moves"].size()==6 && state()["queue"].size()==0);
 runFor(6000);assert(!shutterPositionBusy());
 for(uint8_t ch=0;ch<6;++ch){
   const Tx *up=nullptr,*stop=nullptr;
   for(size_t i=n;i<sent.size();++i)if(sent[i].mask&(uint16_t(1)<<ch)){
     if(sent[i].cmd==CMD_UP)up=&sent[i];if(sent[i].cmd==CMD_STOP)stop=&sent[i];
   }
   assert(up && stop);const auto &c=config.position.calibration[ch];
   int32_t error=stop->first-(up->first+c.timeTo(58,true)-c.timeTo(50,true));
   assert(error>=-1000 && error<=1000);
 }
 // A very short newcomer can shift the next group forward after its blocking start.
 // All affected STOPs must still remain within the accepted 1 s timing tolerance.
 shutterPositionCommand(3,CMD_UP);runFor(31000);
 n=sent.size();assert(shutterPositionTarget(0,"90"));clockMs+=200;shutterPositionCyclic();
 clockMs+=1000;assert(shutterPositionTarget(1,"99"));clockMs+=200;shutterPositionCyclic();
 assert(sent.size()==n+2 && sent.back().cmd==CMD_DOWN && state()["moves"].size()==2);runFor(3000);
 assert(!shutterPositionBusy());
 for(uint8_t ch=0;ch<2;++ch){
   const Tx *down=nullptr,*stop=nullptr;
   for(size_t i=n;i<sent.size();++i)if(sent[i].mask&(uint16_t(1)<<ch)){
     if(sent[i].cmd==CMD_DOWN)down=&sent[i];if(sent[i].cmd==CMD_STOP)stop=&sent[i];
   }
   assert(down && stop);int32_t error=stop->first-(down->first+calibration.timeTo(ch==0?90:99,false));
   assert(error>=-1000 && error<=1000);
 }
 // Restore the known 25% starting position for the following same-channel checks.
 shutterPositionCommand(0xffff,CMD_DOWN);runFor(32000);
 for(uint8_t ch=0;ch<16;++ch)assert(shutterPositionTarget(ch,"25"));runFor(18000);
 // Same-channel follow-up starts only after STOP and recalculates from the completed estimate.
 n=sent.size();assert(shutterPositionTarget(0,"50"));runFor(550);assert(shutterPositionTarget(0,"75"));
 runFor(16000);assert(!shutterPositionBusy() && state()["channels"][0][1]==75);
 assert(sent.size()==n+4 && sent[n].cmd==CMD_UP && sent[n+1].cmd==CMD_STOP && sent[n+2].cmd==CMD_UP);
 n=sent.size();assert(shutterPositionTarget(0,"75"));runFor(250);assert(sent.size()==n && !shutterPositionBusy());
 // STOP wins over a new start near its deadline.
 assert(shutterPositionTarget(0,"50"));runFor(550);const uint32_t stopAt=lastTx+calibration.timeTo(50,false)-calibration.timeTo(75,false)-110;
 clockMs=stopAt-400;assert(shutterPositionTarget(1,"50"));runFor(250);n=sent.size();
 shutterPositionCyclic();assert(sent.size()==n);runFor(2000);
 assert(sent[n].cmd==CMD_STOP && sent[n+1].cmd==CMD_UP);shutterPositionManual(0xffff);
 // Very short direct moves cannot block other STOPs; stop immediately after the start returns.
 shutterPositionCommand(1,CMD_DOWN);runFor(32000);
 config.position.calibration[0]={{1,501,2000},{500,1500,2000}};
 n=sent.size();assert(shutterPositionTarget(0,"1"));runFor(1000);
 assert(!shutterPositionBusy() && sent.size()==n+2 && sent.back().cmd==CMD_STOP);
 assert(int32_t(sent.back().first-(sent[n].first+11))<1000);config.position.calibration[0]=calibration;
 // Exact 1 s boundary: 0/1000 ms share a group; 1001 ms remains separate.
 shutterPositionCommand(7,CMD_UP);runFor(31000);
 const uint32_t durations[3]={5000,5660,5321};
 for(uint8_t ch=0;ch<3;++ch)config.position.calibration[ch].down[0]=durations[ch]*2;
 n=sent.size();for(uint8_t ch=0;ch<3;++ch)assert(shutterPositionTarget(ch,"75"));
 clockMs+=200;shutterPositionCyclic();shutterPositionCyclic();shutterPositionCyclic();runFor(8000);
 assert(!shutterPositionBusy() && sent.size()==n+5);
 assert(sent[n+3].cmd==CMD_STOP && sent[n+3].mask==3);
 assert(sent[n+4].cmd==CMD_STOP && sent[n+4].mask==4);
 for(uint8_t ch=0;ch<3;++ch)config.position.calibration[ch]=calibration;
 // Opposite directions share STOP too; a reference/endpoint channel is never included.
 shutterPositionCommand(1,CMD_DOWN);shutterPositionCommand(6,CMD_UP);runFor(32000);
 n=sent.size();assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"50"));
 // Opening 25% takes 9100 ms; closing 50% starts 340 ms later and takes 8760 ms.
 config.position.calibration[1].down[0]=8760;
 assert(shutterPositionTarget(2,"100"));runFor(12000);
 assert(sent[n].cmd==CMD_UP && sent[n+1].cmd==CMD_DOWN);
 bool shared=false;for(size_t i=n;i<sent.size();++i)if(sent[i].cmd==CMD_STOP){assert(sent[i].mask==3);shared=true;}
 assert(shared && !shutterPositionBusy());config.position.calibration[1]=calibration;
 // Revalidation skips disabled/uncalibrated pending channels.
 config.position.reference=true;assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"25"));
 config.jaro.ch_enable[0]=false;config.position.calibration[1]={};n=sent.size();runFor(250);
 assert(!shutterPositionBusy() && sent.size()==n);config.jaro.ch_enable[0]=true;config.position.calibration[1]=calibration;
 // Failures and global state changes clear every move and queued target.
 for(uint8_t mode=0;mode<3;++mode){
   assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"75"));runFor(900);
   assert(shutterPositionTarget(2,"25"));
   if(mode==0)config.position.enable=false;if(mode==1)setupMode=true;if(mode==2)EspSysUtil::OTA::getInstance().active=true;
   shutterPositionCyclic();assert(!shutterPositionBusy() && sent.back().cmd==CMD_STOP && sent.back().mask==3);
   config.position.enable=true;setupMode=false;EspSysUtil::OTA::getInstance().active=false;
 }
 // A failed second reference group aborts the already-started first group as well.
 assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"75"));
 clockMs+=200;shutterPositionCyclic();radioOK=false;shutterPositionCyclic();radioOK=true;
 assert(!shutterPositionBusy() && state()["notice"]=="send_failed" && sent.back().cmd==CMD_STOP && sent.back().mask==1);
 radioOK=false;target();assert(!shutterPositionBusy() && state()["notice"]=="send_failed");radioOK=true;
 target();runFor(33000);assert(state()["moves"][0][1]==int(PositionMove::Target));
 assert(shutterPositionTarget(0,"75"));radioOK=false;runFor(10000);radioOK=true;
 assert(!shutterPositionBusy() && state()["queue"].size()==0 && state()["channels"][0][1]==-1 && state()["notice"]=="send_failed");
 // Calibration remains exclusive, checks ownership/lease and commits measured values only.
 begin();assert(state()["cal"]["phase"]==1 && !shutterPositionTarget(1,"75"));start();n=sent.size();
 command(R"({"a":"cancel","c":0,"token":8})");assert(shutterPositionBusy() && sent.size()==n);
 runFor(6001);assert(!shutterPositionBusy() && sent.back().cmd==CMD_STOP && state()["notice"]=="timeout");
 begin();start();uint32_t t=lastTx;
 markAt(t+2400);markAt(t+15800);markAt(t+28600);assert(state()["cal"]["phase"]==3);
 start();t=lastTx;markAt(t+12200);markAt(t+26400);markAt(t+29800);assert(state()["cal"]["phase"]==5);
 command(R"({"a":"save","c":0,"token":7})");assert(!shutterPositionBusy() && config.position.calibration[0].up[2]==28600 && state()["notice"]=="saved");
 shutterPositionInvalidate(0);assert(!shutterPositionReady(0));config.position.calibration[0]=calibration;
 // Group planning, collection and STOP timing also cross the millis() wrap.
 clockMs=UINT32_MAX-500;config.position.reference=false;shutterPositionCommand(3,CMD_DOWN);runFor(32000);
 clockMs=UINT32_MAX-500;assert(shutterPositionTarget(0,"25"));assert(shutterPositionTarget(1,"25"));runFor(15000);
 assert(!shutterPositionBusy() && state()["channels"][0][1]==25 && state()["channels"][1][1]==25);
 puts("PASS: parallel controller with blocking radio: grouped references/endpoints/STOPs, 16 moves/queued targets, 1 s timing, priority, short moves, wrap, cancellation, calibration and failures");
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
