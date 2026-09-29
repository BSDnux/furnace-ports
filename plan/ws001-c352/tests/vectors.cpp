// Specification vectors, shared by the pinned reference and production core adapter.
#include <algorithm>
#include <string>
#ifdef C352_IMPLEMENTATION
#include "core-adapter.hpp"
#else
#include "oracle.hpp"
#endif
static int checks=0;
static void eq(long long actual,long long expected,const char* label) {
  ++checks;
  if(actual!=expected) throw std::runtime_error(std::string(label)+": got "+std::to_string(actual)+", expected "+std::to_string(expected));
}
static void boot(c352_device& c) {
  c.device_reset(); c.init_table(); c.reads.clear();
  std::fill(c.rom.begin(),c.rom.end(),0);
}
static void key(c352_device& c,int voice=0,int flags=4,int start=0,int end=3,int loop=0,int bank=0,int freq=0xffff) {
  int b=voice*8;
  c.write(b,0xffff); c.write(b+1,0xffff); c.write(b+2,freq);
  c.write(b+4,bank); c.write(b+5,start); c.write(b+6,end); c.write(b+7,loop);
  c.write(b+3,0x4000|flags); c.write(0x202,0);
}
int main() { try {
  c352_device c;
  boot(c); c.step();
  for(int x:c.sink.out) eq(x,0,"reset silence");
  eq(c.m_random,0x1234,"reset noise seed"); eq(c.read(0x200),0,"reset control");
  for(int i=0;i<256;++i) eq(c.m_mulawtab[i],c219Table[i],"all 256 C219/C352 values");
  eq(c.m_mulawtab[0x80],-32,"negative zero code"); eq(c.m_mulawtab[0xff],-31264,"mu-law negative max");
  c.write(0xf8,0x1234); c.write(0xf8,0xab00,0xff00); eq(c.read(0xf8),0xab34,"masked word write voice31");
  c.write(0x200,0x1234); c.write(0x201,0xffff); eq(c.read(0x200),0x1234,"control read"); eq(c.read(0x201),0,"unmapped control2");
  c.write(3,0x4004); c.write(0x202,0,0xff); eq(c.read(3),0x4004,"partial execute ignored");
  c.write(0x202,0); eq(c.read(3),0x8004,"keyon committed"); eq(c.m_c352_v[0].counter,0xffff,"initial fraction");
  boot(c); c.rom[0]=0x7f; key(c); c.step();
  eq(c.m_c352_v[0].sample,32512,"linear +127"); eq(c.m_c352_v[0].counter,0xfffe,"first fraction");
  eq(c.m_c352_v[0].curr_vol[0],1,"FILTER still ramps in MAME");
  for(int x:c.sink.out) eq(x,15,"first sample four outputs");
  c.rom[1]=0x80; c.step(); eq(c.m_c352_v[0].sample,-32768,"linear -128");
  boot(c); c.rom[0]=0x40; key(c,0,4,0,1); c.step(); c.step();
  eq(c.m_c352_v[0].sample,0,"terminal sample cleared"); eq(c.read(3),0x2004,"terminal flags"); c.step();
  for(int x:c.sink.out) eq(x,0,"terminal silence");
  boot(c); c.rom[0]=0x7f; key(c,0,4,0,0); c.step(); eq(c.sink.out[0],0,"start=end is silent MAME");
  boot(c); c.rom[0]=0x40; key(c,0,0,0,3,0,0,0x8000); c.step();
  eq(c.m_c352_v[0].counter,0x7fff,"half rate fraction"); eq(c.sink.out[0],3,"first interpolation");
  c.step(); eq(c.reads.size(),1,"half rate fetch count at two frames");
  boot(c); key(c,0,4,0,3,0,0,0); c.step(); eq(c.reads.size(),0,"freq zero no fetch"); eq(c.sink.out[0],0,"freq zero silence");
  boot(c); key(c,0,6,0,2,1); for(int i=0;i<5;++i)c.step();
  const int forward[]={0,1,2,1,2}; for(int i=0;i<5;++i)eq(c.reads[i],forward[i],"forward loop addresses"); eq(c.read(3)&0x8800,0x8800,"loop busy/history");
  boot(c); key(c,0,5,3,0); for(int i=0;i<4;++i)c.step();
  for(int i=0;i<4;++i) { eq(c.reads[i],3-i,"reverse addresses"); }
  eq(c.read(3),0x2005,"reverse terminal");
  boot(c); key(c,0,7,0,2,0); for(int i=0;i<6;++i)c.step();
  const int ping[]={0,1,2,1,0,1}; for(int i=0;i<6;++i)eq(c.reads[i],ping[i],"ping pong addresses");
  boot(c); key(c,0,0x26,2,2,7,1); c.step(); eq(c.m_c352_v[0].pos,0x20007,"LINK start becomes bank");
  boot(c); key(c,0,4,0xffff,1,0,0xff); c.step(); c.step();
  eq(c.reads[0],0xffffff,"ROM last byte"); eq(c.reads[1],0,"24-bit ROM wrap");
  boot(c); key(c,0,4,0,2,0,0x100); c.step(); eq(c.reads[0],0,"bank upper bits masked by ROM");
  boot(c); key(c,0,0x14); c.step(); eq(c.m_random,0x091a,"noise step1"); c.step(); eq(c.m_random,0x048d,"noise step2"); c.step(); eq(c.m_random,0xfdb0,"noise step3"); eq(c.reads.size(),0,"noise no ROM access");
  boot(c); key(c); c.write(3,0x6004); c.write(0x202,0); eq(c.read(3),4,"keyoff wins simultaneous keyon");
  boot(c); c.rom[0]=0x40; key(c); c.step(); c.write(3,0xa004); c.write(0x202,0); c.step(); eq(c.sink.out[0],0,"explicit stop");
  boot(c); c.rom[0]=0x40; key(c,0,6,0,0); c.step();
  c.write(0,0x8000); c.write(1,0x0040);
  for(int i=0;i<255;++i)c.step();
  eq(c.sink.out[0],1024,"front-left pan"); eq(c.sink.out[1],0,"front-right mute"); eq(c.sink.out[2],0,"rear-left mute"); eq(c.sink.out[3],512,"rear-right pan");
  c.write(3,0x8186); c.step(); eq(c.sink.out[0],-1024,"live PHASEFL"); eq(c.sink.out[3],-512,"PHASEFR also rear-right");
  c.write(0,0xffff); c.write(1,0xffff); c.write(3,0x8206); for(int i=0;i<255;++i)c.step();
  eq(c.sink.out[2],-2040,"PHASERL"); eq(c.sink.out[0],2040,"left phase restored");
  boot(c); c.rom[0]=0x40; key(c,31,6,0,0); c.step(); eq(c.sink.out[0],8,"voice31 active"); eq(c.m_c352_v[0].flags,0,"voice0 unchanged");
  boot(c); c.rom[0]=0x7f; for(int i=0;i<32;++i)key(c,i,6,0,0); c.step(); eq(c.sink.out[0],508,"32 voices first frame");
  for(int i=0;i<254;++i) { c.step(); }
  eq(c.sink.out[0],-1532,"32 voice signed16 output wrap");
  boot(c); c.rom[0]=0x80; key(c,0,0x0e,0,0); for(int i=0;i<255;++i)c.step(); eq(c.sink.out[0],-4,"mu-law signed output");
  std::cout<<"PASS: "<<checks<<" numerical assertions; reset/register/key/frequency/PCM/mu-law/loop/bank/noise/pan/phase/32-voice vectors\n";
  return 0;
} catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; } }
