"""Exercise the actual edge-capture code with synthetic clean and noisy pulses."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/'lib/JaroliftController/JaroliftController.cpp').read_text()
header=(ROOT/'lib/JaroliftController/JaroliftController.h').read_text()
def function(name):
 start=source.index('void IRAM_ATTR JaroliftController::'+name+'() {');p=source.index('{',start);depth=1;end=p+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end].replace('unsigned long','uint32_t')
fields=header[header.index('  // Empfangspuffer'):header.index('  // Hardware-Modul')].replace('unsigned long','uint32_t')
main=r'''
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cassert>
#include <vector>
#include <random>
#define IRAM_ATTR
using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
static unsigned shortWidth=400;
static uint32_t clockUs=0;static int pinState=0;
uint32_t micros(){return clockUs;}int digitalRead(int){return pinState;}
class JaroliftController {public:
FIELDS
 struct{int gdo2=0;}gpio_;
 static constexpr int kDebounce=200;
 void finishRxFrame();void measureRxEdge();
 JaroliftController(){for(auto&x:lowBuf_)x=0;for(auto&x:hiBuf_)x=0;pbWrite_=0;rxDataReady_=false;}
 void edge(uint32_t at,int state){clockUs=at;pinState=state;measureRxEdge();assert(pbWrite_<=75);}
 void tick(uint32_t at){clockUs=at;if(!rxDataReady_ && rxCapturing_ && uint32_t(micros()-rxLastPulse_)>3500)finishRxFrame();}
 void release(){pbWrite_=0;rxCapturing_=false;lineUp_=lineDown_=rxLastPulse_=0;rxUndo_={};rxDataReady_=false;}
};
FUNCTIONS
struct Interval{int state;unsigned us;};
std::vector<Interval> frame(unsigned bits){
 std::vector<Interval> v{{0,3950}};
 for(unsigned i=0;i<bits;++i){bool one=((i*37+19)%7)<3;v.push_back({1,one?shortWidth:800u});v.push_back({0,one?800u:shortWidth});}
 v.push_back({1,400});return v;
}
JaroliftController run(unsigned bits,int spikeInterval=-1,unsigned spikeWidth=70,bool loopDuring=false,uint32_t start=100000){
 JaroliftController r;uint32_t t=start;auto v=frame(bits);
 for(unsigned i=0;i<v.size();++i){
  r.edge(t,v[i].state);
  if(int(i)==spikeInterval){r.edge(t+230,!v[i].state);if(loopDuring)r.tick(t+235);r.edge(t+230+spikeWidth,v[i].state);}
  t+=v[i].us;
 }
 r.edge(t,0);r.tick(t+3600);return r;
}
void same(const JaroliftController&a,const JaroliftController&b){
 assert(a.rxDataReady_ && b.rxDataReady_);assert(a.pbWrite_==b.pbWrite_);
 for(unsigned i=0;i<a.pbWrite_;++i){assert(a.lowBuf_[i]==b.lowBuf_[i]);if(i)assert(a.hiBuf_[i]==b.hiBuf_[i]);}
}
int main(){
 for(unsigned width:{290u,360u,400u,420u})for(unsigned n:{64u,72u}){
  shortWidth=width;
  auto good=run(n);assert(good.pbWrite_==n+1);
  for(unsigned i=0;i<frame(n).size();++i)for(unsigned w:{1u,40u,100u,199u}){
   // Spike must finish before the real transition.
   if(230+w>=frame(n)[i].us)continue;
   same(good,run(n,i,w,true));
  }
 }
 // Repeated disturbances within the same physical pulse.
 JaroliftController r;r.edge(100000,0);r.edge(103950,1);
 r.edge(104180,0);r.edge(104200,1);r.edge(104450,0);r.edge(104470,1);r.edge(104750,0);
 assert(r.hiBuf_[1]==800);assert(r.lineUp_==103950);
 // False sync must restore both sync and the first HIGH slot.
 r=JaroliftController{};r.lowBuf_[0]=123;r.hiBuf_[1]=456;r.edge(100000,0);r.edge(103950,1);r.edge(104000,0);
 assert(!r.rxCapturing_ && r.pbWrite_==0 && r.lowBuf_[0]==123 && r.hiBuf_[1]==456);r.tick(108000);assert(!r.rxDataReady_);
 // Do not keep a valid partial HIGH when the full HIGH is too long.
 r=JaroliftController{};r.edge(100000,0);r.edge(103950,1);r.edge(104250,0);assert(r.hiBuf_[1]==300);r.edge(104300,1);assert(r.hiBuf_[1]==0);r.edge(105150,0);assert(r.hiBuf_[1]==0);
 // The frame is immutable while waiting for the loop to decode it.
 auto good=run(72);auto frozen=good;good.edge(clockUs+100,1);good.edge(clockUs+4000,0);same(good,frozen);
 // No rollback state may survive release; test a subsequent clean frame.
 good.release();r=run(72);same(r,run(72));
 // micros() rollover uses the original 32-bit ESP arithmetic.
 same(run(72),run(72,-1,70,false,0xffff0000u));
 std::mt19937 rng(123);r=JaroliftController{};uint32_t t=100000;
 for(unsigned i=0;i<100000;++i){t+=1+rng()%5000;r.edge(t,i%2);if(r.rxDataReady_)r.release();}
 printf("PASS: clean 64/72-bit frames, HIGH/LOW and sync spikes, repeated spikes, final-bit rollback, stale HIGH invalidation, frozen frames, timer rollover and 100000 noise edges. Undo RAM: %zu bytes.\n",sizeof(r.rxUndo_));
}
'''
code=main.replace('FIELDS',fields).replace('FUNCTIONS',function('finishRxFrame')+'\n'+function('measureRxEdge'))
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++17','-O1','-g','-fsanitize=address,undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
