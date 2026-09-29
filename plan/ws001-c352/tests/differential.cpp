#include "oracle.hpp"
#include "src/engine/platform/sound/c352.h"
#include <algorithm>
static uint32_t rng=0xc3521234;
static uint32_t randomValue() { rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
int main() {
  c352_device reference;
  C352Core actual;
  reference.init_table(); reference.device_reset();
  for (auto& b: reference.rom) b=randomValue();
  actual.setROM(reference.rom.data(),reference.rom.size());
  size_t checks=0;
  auto equal=[&](int64_t a,int64_t b,const char* field,int frame,int voice) {
    ++checks;
    if (a!=b) {
      std::cerr<<field<<" frame="<<frame<<" voice="<<voice<<" actual="<<a<<" expected="<<b<<'\n';
      throw std::runtime_error("MAME differential mismatch");
    }
  };
  try {
    for (int frame=0; frame<8192; frame++) {
      if (frame%1024==0) { reference.device_reset(); actual.reset(); }
      // A reproducible sequence of full/partial writes, retriggers and live
      // changes, exercising all voices with positive/negative PCM and mu-law.
      for (int j=0; j<8; j++) {
        unsigned address=randomValue()%0x204;
        uint16_t value=randomValue();
        uint16_t mask=(j==0) ? 0xff00 : (j==1) ? 0x00ff : 0xffff;
        reference.write(address,value,mask); actual.write(address,value,mask);
      }
      if (frame%3==0) { reference.write(0x202,0); actual.write(0x202,0); }
      reference.step(); actual.tick(); reference.reads.clear();
      for (int i=0; i<32; i++) {
        const auto& a=actual.voices[i]; const auto& b=reference.m_c352_v[i];
        equal(a.pos,b.pos,"position",frame,i); equal(a.counter,b.counter,"counter",frame,i);
        equal(a.sample,b.sample,"sample",frame,i); equal(a.last_sample,b.last_sample,"last_sample",frame,i);
        for (int o=0; o<4; o++) equal(a.curr_vol[o],b.curr_vol[o],"ramp",frame,i);
      }
      for (int i=0; i<0x204; i++) equal(actual.read(i),reference.read(i),"register",frame,i);
      equal(actual.noise,reference.m_random,"noise",frame,-1);
      for (int o=0; o<4; o++) equal(actual.output[o],reference.sink.out[o],"output",frame,o);
    }
    // Muting suppresses only the contribution, never the shared noise clock.
    actual.reset(); reference.device_reset();
    actual.write(2,0xffff); reference.write(2,0xffff);
    actual.write(3,0x4014); reference.write(3,0x4014);
    actual.write(0x202,0); reference.write(0x202,0);
    actual.mute(0,true);
    for (int i=0; i<100; i++) {
      actual.tick(); reference.step();
      equal(actual.noise,reference.m_random,"muted noise",i,0);
      for (int o=0; o<4; o++) equal(actual.output[o],0,"muted output",i,o);
    }
    actual.mute(32,true); actual.write(0xffffffff,0xffff);
    equal(actual.read(0xffffffff),0,"invalid address",0,0);
    std::cout<<"PASS: "<<checks<<" MAME differential and mute assertions\n";
  } catch (const std::exception&) { return 1; }
}
