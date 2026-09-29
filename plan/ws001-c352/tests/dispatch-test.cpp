// Link the real headless Furnace objects, retaining CLI globals from main.cpp.
// The application entry point is never called; all ROM data is synthetic.
#define main furnace_original_main
#include "../../../src/main.cpp"
#undef main
#include "../../../src/engine/platform/c352.h"
#include <array>
#include <memory>
#include <stdexcept>
#include <iostream>

namespace c352test {
unsigned checks=0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) throw std::runtime_error(message);
}
void equal(long long actual, long long expected, const char* message) {
  ++checks;
  if (actual!=expected) {
    std::cerr << message << ": got " << actual << ", expected " << expected << '\n';
    throw std::runtime_error(message);
  }
}
void nearFreq(int actual, int expected, const char* message) {
  ++checks;
  if (std::abs(actual-expected)>1) {
    std::cerr << message << ": got " << actual << ", expected " << expected << " +/- 1\n";
    throw std::runtime_error(message);
  }
}
using Audio=std::array<std::vector<short>,4>;
struct Fixture {
  std::unique_ptr<DivEngine> engine;
  std::unique_ptr<DivPlatformC352> chip;
  DivInstrument* instrument;
  explicit Fixture(bool quad=false): engine(new DivEngine), chip(new DivPlatformC352) {
    engine->song.compatFlags.oldCenterRate=false;
    instrument=new DivInstrument;
    instrument->type=DIV_INS_AMIGA;
    engine->song.ins.push_back(instrument);
    engine->song.insLen=1;
    DivConfig flags;
    flags.set("quadOutput",quad);
    equal(chip->init(engine.get(),32,44100,flags),32,"32 voices initialized");
  }
  ~Fixture() {
    chip->quit();
    chip.reset();
    engine->song.unload();
  }
  int sample(int size, bool loop=false, DivSampleDepth depth=DIV_SAMPLE_DEPTH_8BIT, unsigned char value=32) {
    auto* s=new DivSample;
    s->depth=depth;
    check(s->init(size),"sample allocation");
    s->centerRate=22050;
    s->loop=loop;
    s->loopStart=0;
    s->loopEnd=size;
    if (depth==DIV_SAMPLE_DEPTH_C219) std::fill(s->dataC219,s->dataC219+size,value);
    else std::fill(s->data8,s->data8+size,static_cast<signed char>(value));
    engine->song.sample.push_back(s);
    engine->song.sampleLen=static_cast<int>(engine->song.sample.size());
    return engine->song.sampleLen-1;
  }
  void render() { chip->renderSamples(0); chip->notifyPitchTable(); }
  int command(DivDispatchCmds c, int voice=0, int a=0, int b=0) {
    return chip->dispatch(DivCommand(c,static_cast<unsigned char>(voice),a,b));
  }
  void note(int sampleIndex, int voice=0, int pitch=DIV_NOTE_RAW_FLAG|0xffff) {
    instrument->amiga.initSample=sampleIndex;
    command(DIV_CMD_INSTRUMENT,voice,0);
    command(DIV_CMD_NOTE_ON,voice,pitch);
  }
  Audio audio(size_t count=1) {
    Audio out;
    short* buffers[4];
    for (int i=0; i<4; i++) { out[i].resize(count); buffers[i]=out[i].data(); }
    chip->acquire(buffers,count);
    return out;
  }
  unsigned reg(unsigned addr) {
    return reinterpret_cast<const unsigned short*>(chip->getRegisterPool())[addr];
  }
  size_t base(int sampleIndex) {
    for (auto& entry: chip->getMemCompo(0)->entries) if (entry.asset==sampleIndex) return entry.begin;
    throw std::runtime_error("sample has no memory entry");
  }
};

void layout() {
  Fixture f;
  f.sample(1,false,DIV_SAMPLE_DEPTH_8BIT,0x7f);
  f.sample(65535);
  f.sample(65536,true);
  const int tooLong=f.sample(65536);
  const int empty=f.sample(0);
  const int invalid=f.sample(8,true);
  f.engine->song.sample[invalid]->loopEnd=9;
  const int backward=f.sample(8,true);
  f.engine->song.sample[backward]->loopMode=DIV_SAMPLE_LOOP_BACKWARD;
  const int disabled=f.sample(8);
  f.engine->song.sample[disabled]->renderOn[0][0]=false;
  const int encoded=f.sample(256,true,DIV_SAMPLE_DEPTH_C219);
  for (int i=0; i<256; i++) f.engine->song.sample[encoded]->dataC219[i]=i;
  f.render();
  equal(f.chip->getSampleMemCapacity(),0x1000000,"16 MiB capacity");
  equal(f.base(0),0,"one-byte sample base");
  equal(f.base(1),0x10000,"65535 plus guard starts in new bank");
  equal(f.base(2),0x20000,"65536 loop occupies full bank");
  equal(f.base(encoded),0x30000,"encoded sample after full bank");
  equal(f.chip->getSampleMemUsage(),0x30100,"memory usage includes guard/alignment");
  const auto* rom=static_cast<const unsigned char*>(f.chip->getSampleMem());
  equal(rom[0],0x7f,"single audio byte retained");
  equal(rom[1],0,"single-byte guard");
  equal(rom[0x1fffe],32,"last audio byte retained at bank end");
  equal(rom[0x1ffff],0,"guard at bank end");
  for (int i=0; i<256; i++) equal(rom[0x30000+i],i,"C219 byte order unchanged");
  for (int i: {tooLong,empty,invalid,backward,disabled}) check(!f.chip->isSampleLoaded(0,i),"invalid or disabled sample rejected");
  check(!f.chip->isSampleLoaded(0,-1),"negative sample rejected");
  check(!f.chip->isSampleLoaded(1,0),"invalid memory index rejected");
  f.note(0); f.chip->tick();
  // FILTER isolates the one-byte guard test from interpolation.
  f.chip->poke(3,C352Core::BUSY|C352Core::FILTER);
  auto one=f.audio();
  equal(one[0][0],15,"one-byte sample is audible before guard");
  equal(f.reg(6),1,"non-loop end points at guard");
  auto end=f.audio();
  equal(end[0][0],0,"guard terminates sample");
  check(!(f.reg(3)&C352Core::BUSY),"guard clears BUSY");
  f.note(1); f.chip->tick(); f.audio();
  equal(f.reg(4),1,"bank for maximum non-loop");
  equal(f.reg(6),0xffff,"maximum non-loop guard register");
  f.note(2); f.chip->tick(); f.audio();
  equal(f.reg(6),0xffff,"loop end is inclusive");
  check(f.reg(3)&C352Core::LOOP,"full-bank loop flag");
  f.note(tooLong); f.chip->tick(); f.audio();
  check(!(f.reg(3)&C352Core::BUSY),"unloaded note stops previous voice");
  f.note(encoded); f.chip->tick(); f.audio();
  check(f.reg(3)&C352Core::MULAW,"C219 sample selects C352 decoder");
  // Re-rendering cannot leave old loaded flags or ROM contents behind.
  f.engine->song.sample[0]->renderOn[0][0]=false;
  f.render();
  check(!f.chip->isSampleLoaded(0,0),"rerender clears loaded flag");
}

void fullROM() {
  Fixture f;
  for (int i=0; i<256; i++) f.sample(65536,true);
  int overflow=f.sample(1);
  f.render();
  equal(f.chip->getSampleMemUsage(),0x1000000,"exact ROM capacity used");
  check(f.chip->isSampleLoaded(0,255),"last bank loaded");
  check(!f.chip->isSampleLoaded(0,overflow),"ROM overflow rejected");
  equal(f.base(255),0xff0000,"last ROM bank address");
  f.note(255,31); f.chip->tick(); f.audio();
  equal(f.reg(31*8+4),0xff,"voice31 plays last ROM bank");
  equal(f.reg(31*8+6),0xffff,"last ROM address is playable");
}

void playback() {
  Fixture f;
  f.sample(32,true);
  auto* sample=f.engine->song.sample[0];
  sample->loopStart=5; sample->loopEnd=20;
  f.render();
  f.chip->toggleRegisterDump(true);
  f.note(0,0); f.note(0,31);
  f.command(DIV_CMD_SAMPLE_POS,0,3);
  f.chip->tick();
  const auto& writes=f.chip->getRegisterWrites();
  unsigned executes=0;
  int lastData=-1, flags=-1;
  for (size_t i=0; i<writes.size(); i++) {
    if (writes[i].addr==0x202) ++executes;
    if (writes[i].addr<8 && writes[i].addr!=3) lastData=static_cast<int>(i);
    if (writes[i].addr==3) { flags=static_cast<int>(i); equal(writes[i].val,C352Core::KEYON|C352Core::LOOP,"only pending key-on and loop"); }
  }
  equal(executes,1,"one key execution for simultaneous voices");
  equal(writes.back().addr,0x202,"key execution is last write");
  check(lastData<flags,"voice data precedes key flag");
  f.audio();
  equal(f.reg(5),3,"sample offset changes start only");
  equal(f.reg(7),5,"loop start remains absolute within sample");
  equal(f.reg(6),19,"half-open loop end converted");
  check(f.reg(31*8+3)&C352Core::BUSY,"voice31 starts");
  f.audio(64);
  check(f.reg(3)&C352Core::LOOPHIST,"loop executes during acquisition");
  f.command(DIV_CMD_NOTE_OFF); f.note(0); f.chip->tick(); f.audio();
  check(f.reg(3)&C352Core::BUSY,"same-tick note-off then note-on retriggers");
  check(!(f.reg(3)&C352Core::KEYOFF),"retrigger has no stale KEYOFF");
  f.command(DIV_CMD_NOTE_OFF,0); f.command(DIV_CMD_NOTE_OFF,31); f.chip->tick();
  auto silent=f.audio(2);
  equal(silent[0][1],0,"note-off silences both voices");
  check(!(f.reg(3)&C352Core::BUSY),"note-off clears busy");
  sample->loopMode=DIV_SAMPLE_LOOP_PINGPONG;
  f.render(); f.note(0); f.chip->tick(); f.audio();
  check(f.reg(3)&C352Core::REVERSE,"ping-pong reverse flag");
  sample->loopStart=19;
  f.render(); f.note(0); f.chip->tick(); f.audio();
  check(!(f.reg(3)&C352Core::REVERSE),"single-point ping-pong normalized");
  f.command(DIV_CMD_SAMPLE_POS,0,20); f.chip->tick(); f.audio();
  check(!(f.reg(3)&C352Core::BUSY),"offset at loop end rejects note");
  f.note(-1); f.chip->tick(); f.audio();
  check(!f.chip->getChanState(0)->active,"invalid sample deactivates channel");
  f.chip->poke(0x200,0xabcd); f.chip->poke(0x201,0xffff); f.chip->poke(0x100,0xffff); f.chip->poke(0xffffffff,0xffff);
  f.audio();
  equal(f.reg(0x200),0xabcd,"16-bit register preserved");
  equal(f.reg(0x201),0,"unused control ignored");
  equal(f.reg(0x100),0,"unused voice address ignored");
  equal(f.chip->getRegisterPoolSize(),0x203,"word register count");
  equal(f.chip->getRegisterPoolDepth(),16,"word register depth");
  const auto& dump=f.chip->getRegisterWrites();
  check(std::any_of(dump.begin(),dump.end(),[](const DivRegWrite& w) { return w.addr==0x200 && w.val==0xabcd; }),"dump preserves full word");
  f.chip->reset(); f.audio();
  for (unsigned i=0; i<0x203; i++) equal(f.reg(i),0,"reset clears register pool");
}

void panAndMix() {
  Fixture quad(true), stereo;
  for (Fixture* f: {&quad,&stereo}) {
    f->sample(2,true); f->render(); f->note(0);
    for (int i=0; i<4; i++) f->command(DIV_CMD_SURROUND_PANNING,0,i,25*(i+1));
    f->chip->tick();
  }
  equal(quad.chip->getOutputCount(),4,"quad output count");
  equal(stereo.chip->getOutputCount(),2,"default stereo output count");
  auto q=quad.audio(512), s=stereo.audio(512);
  for (size_t i=0; i<512; i++) {
    equal(s[0][i],(int(q[0][i])+q[2][i])/2,"front/rear left mean");
    equal(s[1][i],(int(q[1][i])+q[3][i])/2,"front/rear right mean");
  }
  for (int i=0; i<4; i++) equal(q[i].back(),100*(i+1),"quad FL FR RL RR order");
  equal(s[0].back(),200,"100,200,300,400 stereo left");
  equal(s[1].back(),300,"100,200,300,400 stereo right");
  quad.command(DIV_CMD_VOLUME,0,128); quad.command(DIV_CMD_PANNING,0,255,0); quad.chip->tick(); quad.audio(512);
  equal(quad.reg(0),0x8000,"volume times left pan");
  equal(quad.reg(1),0x8000,"normal pan applies to rear");
  quad.chip->muteChannel(0,true);
  auto muted=quad.audio(8);
  for (int o=0; o<4; o++) for (short v: muted[o]) equal(v,0,"mute suppresses all outputs");
  check(quad.reg(3)&C352Core::BUSY,"mute retains playback state");
  quad.chip->muteChannel(0,false);
  check(quad.audio()[0][0]!=0,"unmute resumes without retrigger");
  for (Fixture* f: {&quad,&stereo}) {
    std::fill(f->engine->song.sample[0]->data8,f->engine->song.sample[0]->data8+2,-1);
    f->render(); f->chip->reset(); f->note(0);
    const int pans[]={1,9,9,17};
    for (int i=0; i<4; i++) f->command(DIV_CMD_SURROUND_PANNING,0,i,pans[i]);
    f->chip->tick();
  }
  q=quad.audio(512); s=stereo.audio(512);
  equal(q[0].back(),-1,"negative FL"); equal(q[1].back(),-2,"negative FR");
  equal(q[2].back(),-2,"negative RL"); equal(q[3].back(),-3,"negative RR");
  equal(s[0].back(),-1,"negative odd left sum truncates toward zero");
  equal(s[1].back(),-2,"negative odd right sum truncates toward zero");
  // Oscilloscope receives the average before the core's final divide-by-eight.
  auto* osc=quad.chip->getOscBuffer(0);
  const unsigned short pos=osc->needle>>16;
  quad.audio();
  equal(osc->data[pos],-9,"per-voice scope average");
}

void noiseMute() {
  Fixture baseline(true), muted(true);
  for (Fixture* f: {&baseline,&muted}) {
    for (int v=0; v<2; v++) {
      f->chip->poke(v*8,v==0 && f==&baseline ? 0 : 0xffff);
      f->chip->poke(v*8+1,0);
      f->chip->poke(v*8+2,0xffff);
      f->chip->poke(v*8+3,C352Core::KEYON|C352Core::NOISE|C352Core::FILTER);
    }
    f->chip->poke(0x202,0);
  }
  muted.chip->muteChannel(0,true);
  auto a=baseline.audio(512), b=muted.audio(512);
  check(std::any_of(a[0].begin(),a[0].end(),[](short v) { return v!=0; }),"noise is audible");
  for (int o=0; o<4; o++) for (int i=0; i<512; i++) equal(a[o][i],b[o][i],"muted voice still advances shared noise");
  auto* osc=muted.chip->getOscBuffer(0);
  unsigned short pos=osc->needle>>16;
  muted.audio();
  equal(osc->data[pos],0,"muted scope is silent");
}

void pitchAndMacros() {
  Fixture f;
  f.sample(32,true); f.render();
  for (bool linear: {true,false}) {
    f.engine->song.compatFlags.linearPitch=linear;
    f.chip->notifyPitchTable(); f.chip->reset();
    for (int octave=0; octave<3; octave++) {
      f.note(0,0,96+12*octave); f.chip->tick(); f.audio();
      nearFreq(f.reg(2),8192<<octave,"sample pitch follows octave and center rate");
    }
    f.note(0,0,DIV_NOTE_RAW_FLAG|0x1234); f.chip->tick(); f.audio();
    equal(f.reg(2),0x1234,"raw pitch bypasses table");
    f.note(0,0,DIV_NOTE_RAW_FLAG|0x1ffff); f.chip->tick(); f.audio();
    equal(f.reg(2),0xffff,"frequency saturates at 16 bits");
  }
  f.engine->song.compatFlags.linearPitch=true;
  f.engine->song.compatFlags.oldCenterRate=true;
  f.chip->notifyPitchTable(); f.chip->reset();
  f.note(0,0,108); f.chip->tick(); f.audio();
  equal(f.reg(2),16402,"legacy center reference uses 8363 Hz");
  f.engine->song.compatFlags.oldCenterRate=false;
  f.engine->song.sample[0]->centerRate=44100;
  f.chip->notifyPitchTable(0); f.chip->reset();
  f.note(0,0,108); f.chip->tick(); f.audio();
  nearFreq(f.reg(2),32768,"centerRate change updates pitch table");
  // 1.5 times the default clock stays below Furnace's 40 MHz ceiling.
  DivConfig flags; flags.set("customClock",38102400);
  f.chip->setFlags(flags); f.chip->forceIns(); f.chip->tick(); f.audio();
  nearFreq(f.reg(2),21845,"custom clock uses divider 288");
  equal(f.chip->getMaxFreq(0),65535,"reported frequency ceiling");
  // AMIGA fallback note map is the p002 route; dedicated instrument ID is p003.
  f.instrument->amiga.useNoteMap=true;
  f.instrument->amiga.noteMap[96].map=0;
  f.instrument->amiga.noteMap[96].freq=108;
  f.note(0,0,96); f.chip->tick(); f.audio();
  nearFreq(f.reg(2),21845,"note map selects playback note");
  f.command(DIV_CMD_LEGATO,0,108); f.chip->tick(); f.audio();
  nearFreq(f.reg(2),43691,"legato preserves sample-note offset");
  f.instrument->amiga.useNoteMap=false;
  f.instrument->std.volMacro.len=1; f.instrument->std.volMacro.val[0]=32;
  f.instrument->std.panLMacro.len=1; f.instrument->std.panLMacro.val[0]=127;
  f.instrument->std.panRMacro.len=1; f.instrument->std.panRMacro.val[0]=0;
  f.note(0,0,108); f.chip->tick(); f.audio();
  equal(f.reg(0),0x7f00,"AMIGA volume/pan macros use 64/127 scales");
  equal(f.reg(1),0x7f00,"pan macros apply to rear outputs");
  f.command(DIV_CMD_NOTE_OFF); f.chip->tick(); f.audio();
  check(!(f.reg(3)&C352Core::BUSY),"macro instrument stops");
}
}

int main() {
  try {
    c352test::layout();
    c352test::fullROM();
    c352test::playback();
    c352test::panAndMix();
    c352test::noiseMute();
    c352test::pitchAndMacros();
    std::cout << "PASS: C352 real-dispatch integration (" << c352test::checks << " assertions)\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "FAIL: C352 dispatch: " << ex.what() << '\n';
    return 1;
  }
}
