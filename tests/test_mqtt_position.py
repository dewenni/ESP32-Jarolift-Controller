"""Host test: actual MQTT routing and Discovery payloads, fake radio/network."""
from pathlib import Path
import subprocess
import tempfile
from host_helpers import arduino_json_headers
ROOT = Path(__file__).resolve().parents[1]
HEADERS = {
'mqttBasic.h':'#pragma once\n#include <mqtt.h>\n',
'mqtt.h': r'''#pragma once
#include <ArduinoJson.h>
#include <positionModel.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#define VERSION "test"
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
struct Config {
 struct {bool ha_enable=true;char ha_topic[128]="homeassistant",topic[128]="jaro",ha_device[32]="controller";} mqtt;
 struct {bool ch_enable[16]={},grp_enable[6]={};char ch_name[16][32]={},grp_name[6][32]={};uint16_t grp_mask[6]={};} jaro;
 struct {bool enable=false;PositionCalibration calibration[16];} position;
 struct {bool enable=false;} eth;
};
extern Config config;
struct s_MqttMessage {char topic[512]={},payload[512]={};int len=0;};
bool mqttIsConnected();void mqttPublish(const char*,const char*,bool);void mqttSubscribe(const char*);const char *addTopic(const char*);
inline const auto &mqttGetConnectionConfig(){return config.mqtt;}
namespace EspSysUtil {struct OTA {bool active=false;static OTA&getInstance(){static OTA x;return x;}bool isActive(){return active;}};struct RestartReason{static void saveLocal(const char*){}static const char *get(){return "test";}};}
namespace EspStrUtil {inline void replace_underscores(const char*s,char*d,size_t n){snprintf(d,n,"%s",s);}}
inline void yield(){}inline void delay(int){}struct FakeESP{void restart(){}};inline FakeESP ESP;
''',
'basics.h':'#pragma once\n#include <mqtt.h>\n',
'language.h':'#pragma once\n',
'message.h':'#pragma once\n',
'WiFiMgn.h':'#pragma once\ninline void sendWiFiInfo(){}\n',
'jarolift.h':'''#pragma once
#include <cstdint>
enum JaroCmdType {CMD_UP,CMD_DOWN,CMD_STOP,CMD_SHADE,CMD_SET_SHADE};
enum JaroCmdGrpType {CMD_GRP_UP,CMD_GRP_DOWN,CMD_GRP_STOP,CMD_GRP_SHADE};
void jaroCmd(JaroCmdType,uint8_t);void jaroCmd(JaroCmdGrpType,uint16_t);
''',
'shutterPosition.h':'#pragma once\n#include <cstdint>\nbool shutterPositionReady(uint8_t);bool shutterPositionTarget(uint8_t,const char*);void shutterPositionPublish();\n'
}
MAIN=r'''
#include <mqtt.h>
#include <mqttCustom.h>
#include <mqttDiscovery.h>
#include <jarolift.h>
#include <cassert>
#include <map>
#include <string>
Config config;bool connected=true;int publications=0,targetChannel=-1,manualCalls=0;std::string targetValue;std::map<std::string,std::string> published;
bool mqttIsConnected(){return connected;}
void mqttPublish(const char*t,const char*p,bool){published[t]=p;}
void mqttSubscribe(const char*){}
const char *addTopic(const char*s){static char b[512];snprintf(b,sizeof(b),"%s%s",config.mqtt.topic,s);return b;}
bool shutterPositionReady(uint8_t ch){return ch<16&&config.position.enable&&config.jaro.ch_enable[ch]&&config.position.calibration[ch].valid();}
bool shutterPositionTarget(uint8_t ch,const char*p){targetChannel=ch;targetValue=p;return shutterPositionReady(ch);}
void shutterPositionPublish(){++publications;}
void jaroCmd(JaroCmdType,uint8_t){++manualCalls;}void jaroCmd(JaroCmdGrpType,uint16_t){++manualCalls;}
JsonDocument cover(){JsonDocument d;assert(!deserializeJson(d,published.at("homeassistant/cover/controller/shutter1/config")));return d;}
void command(const char*topic,const char*payload){s_MqttMessage m;snprintf(m.topic,sizeof(m.topic),"%s",topic);snprintf(m.payload,sizeof(m.payload),"%s",payload);processMqttMessage(m);}
int main(){
 config.jaro.ch_enable[0]=config.jaro.ch_enable[1]=true;strcpy(config.jaro.ch_name[0],"Living room");
 config.position.calibration[0]=PositionCalibration{{2400,15800,28600},{12200,26400,29800}};
 mqttCustomConnected();mqttCustomCyclic();auto d=cover();assert(!d["set_pos_t"].is<const char*>());assert(d["cmd_t"]=="jaro/cmd/shutter/1");auto identity=d["uniq_id"].as<std::string>();
 config.position.enable=true;mqttCustomCyclic();d=cover();assert(d["set_pos_t"]=="jaro/cmd/position/1"&&d["pos_t"]=="jaro/status/shutter/1");assert(d["pos_clsd"]==0&&d["pos_open"]==100&&!d["optimistic"].as<bool>());assert(d["uniq_id"]==identity);assert(d["pos_tpl"].as<std::string>().find("None")!=std::string::npos);
 JsonDocument second;assert(!deserializeJson(second,published.at("homeassistant/cover/controller/shutter2/config")));assert(second["set_pos_t"].isNull());
 command("jaro/cmd/position/1","25");assert(targetChannel==0&&targetValue=="25"&&manualCalls==0);
 targetChannel=-1;command("jaro/cmd/position/16","75");assert(targetChannel==15);
 targetChannel=-1;command("jaro/cmd/position/17","25");assert(targetChannel==-1);
 command("jaro/cmd/shutter/1","0");command("jaro/cmd/shutter/1","1");assert(manualCalls==2);
 config.position.calibration[0]=PositionCalibration{};mqttCustomCyclic();assert(cover()["set_pos_t"].isNull());
 config.position.calibration[0]=PositionCalibration{{2400,15800,28600},{12200,26400,29800}};mqttCustomCyclic();assert(cover()["set_pos_t"]=="jaro/cmd/position/1");
 config.position.enable=false;mqttCustomCyclic();assert(cover()["set_pos_t"].isNull());
 config.position.enable=true;connected=false;mqttCustomCyclic();connected=true;mqttCustomConnected();mqttCustomCyclic();assert(cover()["set_pos_t"]=="jaro/cmd/position/1");
 // Maximum configured prefix/device/name lengths must still produce complete JSON.
 memset(config.mqtt.topic,'x',127);config.mqtt.topic[127]=0;memset(config.mqtt.ha_device,'d',31);config.mqtt.ha_device[31]=0;memset(config.jaro.ch_name[0],'n',31);config.jaro.ch_name[0][31]=0;mqttDiscoverySetup(false);
 const std::string key="homeassistant/cover/"+std::string(config.mqtt.ha_device)+"/shutter1/config";JsonDocument large;assert(!deserializeJson(large,published.at(key)));assert(large["set_pos_t"].as<std::string>().size()==142);
 puts("PASS: actual MQTT routing and discovery: eligibility, stable identity, position topics, disable/invalidation/reconnect, legacy commands, maximum field lengths");
}
'''
if __name__=='__main__':
    headers=arduino_json_headers()
    with tempfile.TemporaryDirectory() as directory:
        temp=Path(directory)
        for name,content in HEADERS.items():(temp/name).write_text(content)
        (temp/'main.cpp').write_text(MAIN)
        subprocess.run(['c++','-std=c++17','-Wno-deprecated-declarations','-I'+str(temp),'-I'+str(ROOT/'include'),'-I'+str(headers),str(ROOT/'src/mqttCustom.cpp'),str(ROOT/'src/mqttDiscovery.cpp'),str(temp/'main.cpp'),'-o',str(temp/'test')],check=True)
        subprocess.run([str(temp/'test')],check=True)
