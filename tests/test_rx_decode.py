"""Exercise capture, real KeeLoq decoding and callbacks with synthetic telegrams."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
lib=ROOT/'lib/JaroliftController'
source=(lib/'JaroliftController.cpp').read_text();header=(lib/'JaroliftController.h').read_text()
def function(name):
 marker='JaroliftController::'+name+'() {';mid=source.index(marker);start=source.rfind('\n',0,mid)+1;p=source.index('{',mid);depth=1;end=p+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end].replace('unsigned long','uint32_t')
fields=header[header.index('  // Empfangspuffer'):header.index('  // Hardware-Modul')].replace('unsigned long','uint32_t')
code=r'''
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cassert>
#include <functional>
#include <vector>
#include <utility>
#define IRAM_ATTR
#define ESP_LOGD(...) ((void)0)
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define bitRead(x,n) (((x)>>(n)) & 1u)
static uint32_t now=100000;static int level=0;
uint32_t micros(){return now;}int digitalRead(int){return level;}
CRYPTO
class JaroliftController {public:
FIELDS
 static constexpr int kDebounce=200;
 struct{int gdo2=0;}gpio_;
 struct{uint32_t masterMSB=0x12345678,masterLSB=0xabcdef01;}config_;
 uint32_t deviceKeyMSB_=0,deviceKeyLSB_=0;
 std::function<void(uint32_t,uint8_t,uint16_t)> remoteCallback;
 void finishRxFrame();void measureRxEdge();void processRxData();void rxKeyGen();uint32_t rxDecode();
 JaroliftController(){for(auto&x:lowBuf_)x=0;for(auto&x:hiBuf_)x=0;pbWrite_=0;rxDataReady_=false;rxLogFrame_=false;steadyCount_=0;}
 void edge(uint32_t at,int state){now=at;level=state;measureRxEdge();}
 void tick(uint32_t at){now=at;if(!rxDataReady_ && rxCapturing_ && uint32_t(now-rxLastPulse_)>3500)finishRxFrame();}
};
FUNCTIONS
std::vector<std::pair<int,unsigned>> pulses(JaroliftController&r,uint32_t serial,uint16_t mask,uint8_t cmd,uint8_t discriminator,unsigned bits){
 r.rxSerial_=serial;r.rxKeyGen();Keeloq key(r.deviceKeyMSB_,r.deviceKeyLSB_);
 const uint32_t decoded=(uint32_t(mask & 255)<<24)|(uint32_t(discriminator)<<16)|0x48aa;
 const uint32_t hop=key.encrypt(decoded);
 uint64_t fixed=(uint64_t(cmd)<<60)|(uint64_t(serial)<<32)|hop;
 std::vector<std::pair<int,unsigned>> v{{0,3961}};
 for(unsigned i=0;i<bits;++i){bool one=i<64?(fixed>>i)&1:(mask>>(i-56))&1;v.push_back({1,one?400:800});v.push_back({0,one?800:400});}
 return v;
}
unsigned decode(uint32_t serial,uint16_t mask,uint8_t cmd,bool idleEnd,bool badDisc=false,int damagedSerialBit=-1,unsigned bits=72){
 JaroliftController r;auto v=pulses(r,serial,mask,cmd,badDisc?(serial^1)&255:serial&255,bits);
 if(damagedSerialBit>=0){unsigned index=1+2*(32+damagedSerialBit);std::swap(v[index].second,v[index+1].second);}
 if(!idleEnd)v.push_back({1,400});
 unsigned called=0;uint16_t actualMask=0;uint8_t actualCmd=0;uint32_t actualSerial=0;
 r.remoteCallback=[&](auto s,auto c,auto m){++called;actualSerial=s;actualCmd=c;actualMask=m;};
 uint32_t at=100000;for(auto [state,width]:v){r.edge(at,state);at+=width;}
 if(!idleEnd)r.edge(at,0);
 r.tick(at+3600);assert(r.rxDataReady_);r.processRxData();
 if(badDisc || damagedSerialBit>=0 || (bits>64 && bits<72)){if(called)printf("unexpected callback: badDisc=%d damagedBit=%d bits=%u idle=%d\n",badDisc,damagedSerialBit,bits,idleEnd);fflush(stdout);assert(!called);return 0;}
 assert(called==1);assert(actualSerial==serial && actualMask==mask && actualCmd==cmd);return actualMask;
}
int main(){
 for(bool idle:{true,false})for(unsigned c=0;c<16;++c)for(uint8_t cmd:{2,4,8})decode(0xc7c0000+c,uint16_t(1)<<c,cmd,idle);
 for(bool idle:{true,false})for(uint16_t group:{0x8889,0x403f,0x003f,0xffff})decode(0xc7c000e,group,8,idle);
 for(bool idle:{true,false})decode(0x4d46401,2,4,idle);
 decode(0xc7c0000,1,8,true,true);
 // One corrupted serial bit derives a different crypto key: must not publish a random mask.
 decode(0x007c0000,1,8,true,false,20);
 decode(0xc7c0000,1,8,false,false,-1,64);
 for(unsigned n:{65u,66u,67u,68u,69u,70u})decode(0xc7c0000,1,8,false,false,-1,n);
 decode(0xc7c0000,1,8,true,false,-1,71);
 puts("PASS: actual capture + KeeLoq decode + callback: all 16 channels, UP/DOWN/STOP, groups, final LOW merging into idle, paired ending, legacy 64-bit frame, truncated groups, discriminator mismatch and corrupted serial.");
}
'''
crypto=(lib/'KeeloqLib.h').read_text();crypto=crypto[crypto.index('class Keeloq'):];crypto+='\n'+(lib/'KeeloqLib.cpp').read_text().replace('#include "KeeloqLib.h"','');crypto=crypto.replace('unsigned long','uint32_t')
code=code.replace('CRYPTO',crypto).replace('FIELDS',fields).replace('FUNCTIONS','\n'.join(function(x) for x in ['finishRxFrame','measureRxEdge','rxKeyGen','rxDecode','processRxData']))
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++17','-O1','-g','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
