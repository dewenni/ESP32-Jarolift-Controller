// Host test: c++ -std=c++17 -Iinclude tests/test_position_model.cpp -o /tmp/position-test && /tmp/position-test
#include <positionModel.h>
#include <cassert>
#include <cstdio>
#include <limits>
#include <initializer_list>

int main() {
  PositionCalibration c{{2400,15800,28600},{12200,26400,29800}};
  assert(c.valid());
  assert(c.timeTo(0,true)==0 && c.timeTo(100,true)==28600);
  assert(c.timeTo(50,true)==15800 && c.timeTo(25,true)==9100);
  assert(c.timeTo(100,false)==0 && c.timeTo(0,false)==29800);
  assert(c.timeTo(50,false)==12200 && c.timeTo(75,false)==6100);
  // Directional mappings are monotonic; slat phase belongs only to full closure.
  for(int i=1;i<=100;++i) {
    assert(c.timeTo(i,true)>=c.timeTo(i-1,true));
    assert(c.timeTo(i,false)<=c.timeTo(i-1,false));
  }
  assert(c.timeTo(1,false)<c.down[1] && c.down[1]<c.timeTo(0,false));
  PositionMove m;
  m.begin(3,25,c,1000);assert(!m.referenceUp() && m.channel==3);
  assert(m.next(1000+c.referenceTime(false)-1)==PositionMove::None);
  assert(m.next(1000+c.referenceTime(false))==PositionMove::None && m.phase==PositionMove::Settle);
  assert(m.next(m.since+599)==PositionMove::None);
  assert(m.next(m.since+600)==PositionMove::MoveToTarget);
  m.targetStarted(c,40000);assert(m.targetUp() && m.duration==9100);
  assert(m.next(48989)==PositionMove::None);
  assert(m.next(48990)==PositionMove::Stop);m.cancel();assert(!m.active());
  m.begin(0,75,c,0);assert(m.referenceUp() && !m.targetUp());
  m.begin(0,50,c,0);assert(m.referenceUp());
  for(auto endpoint:{0,100}) {
    m.begin(1,endpoint,c,0);
    assert(m.next(c.referenceTime(endpoint==100))==PositionMove::Complete && !m.active());
  }
  // Direct travel uses the directional curve difference, including endpoint slat phases.
  m.beginDirect(2,75,25,c,1000);
  assert(m.phase==PositionMove::Target && m.targetUp() && m.duration==13100);
  assert(m.next(13989)==PositionMove::None && m.next(13990)==PositionMove::Stop);
  m.beginDirect(2,25,75,c,1000);
  assert(!m.targetUp() && m.duration==13200);
  m.beginDirect(2,25,0,c,0);assert(m.duration==9100 && m.targetUp());
  m.beginDirect(2,75,100,c,0);assert(m.duration==6100 && !m.targetUp());
  m.beginDirect(2,51,50,c,UINT32_MAX-20);
  assert(m.next(uint32_t(m.since+m.duration-110))==PositionMove::Stop);
  // Timing survives the millis() wrap and can be cancelled in every phase.
  m.begin(1,25,c,UINT32_MAX-10000);
  assert(m.next(uint32_t(m.since+m.duration))==PositionMove::None && m.phase==PositionMove::Settle);
  m.cancel();assert(m.next(UINT32_MAX)==PositionMove::None);
  auto bad=c;bad.down[0]=UINT32_MAX;assert(!bad.valid());
  bad=c;bad.up[2]=180001;assert(!bad.valid());
  bad=c;bad.up[1]=bad.up[0];assert(!bad.valid());
  assert(!PositionCalibration{}.valid());
  puts("PASS: asymmetric curves, slat times, referencing, reversal pause, STOP lead, endpoints, cancellation, wrap and invalid calibration");
}
