// Exercise production GUI/engine objects without starting the application or audio device.
// Include SDL first so its main macro cannot rename the application's retained entry point.
#include <SDL.h>
#undef main
#define main furnace_original_main
#include "../../../src/main.cpp"
#undef main
#include "../../../src/engine/platform/c352.h"
#include "../../../src/engine/platform/c140.h"
#include "../../../src/gui/guiConst.h"
#include "../../../src/gui/debug.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

extern nlohmann::json serializeInstrument(DivInstrument* ins);

namespace {
unsigned checks=0;
void check(bool ok, const char* message) {
  ++checks;
  if (!ok) throw std::runtime_error(message);
}
struct Engine {
  std::unique_ptr<DivEngine> e{new DivEngine};
  ~Engine() { e->quitDispatch(); e->song.unload(); }
};
void bootstrap(DivEngine& e) {
  // load() registers the real system table before validating the format.
  // A deliberately invalid buffer avoids preInit() touching the user's config/log.
  check(!e.load(new unsigned char[32](),32,"invalid.fur"),"invalid bootstrap rejected");
}
void setup(DivEngine& e, DivSystem sys) {
  e.song.systemLen=1;
  e.song.system[0]=sys;
  e.song.initDefaultSystemChans();
  e.song.recalcChans();
}
DivSample* sample(DivEngine& e, DivSampleDepth depth, DivSampleLoopMode mode) {
  auto* s=new DivSample;
  s->depth=depth;
  check(s->init(32),"sample allocation");
  s->name="synthetic";
  s->centerRate=22050;
  s->loop=true;
  s->loopStart=4;
  s->loopEnd=28;
  s->loopMode=mode;
  auto* bytes=static_cast<unsigned char*>(s->getCurBuf());
  for (int i=0; i<32; ++i) bytes[i]=(i*13+7)&255;
  e.song.sample.push_back(s);
  e.song.sampleLen=int(e.song.sample.size());
  return s;
}
void setMacro(DivInstrumentMacro& m, int first, int last) {
  m.len=2; m.loop=0; m.val[0]=first; m.val[1]=last;
}
void verifyMacro(const DivInstrumentMacro& m, int first, int last) {
  check(m.len==2 && m.loop==0 && m.val[0]==first && m.val[1]==last,"macro round-trip");
}
void registry(DivEngine& e) {
  check(DIV_INS_C352==68,"stable instrument ID");
  check(DivEngine::systemFromFileFur(0xe8)==DIV_SYSTEM_C352,"C352 file map");
  const auto* d=DivEngine::getSystemDef(DIV_SYSTEM_C352);
  check(d && d->id==0xe8,"C352 definition");
  check(d->channels==32 && d->minChans==32 && d->maxChans==32,"32 fixed channels");
  check(d->vgmVersion==0,"VGM remains unsupported");
  check(e.minVGMVersion(DIV_SYSTEM_C352)==0,"VGM export UI sees unsupported version");
  check(d->sampleFormatMask==((1U<<DIV_SAMPLE_DEPTH_8BIT)|(1U<<DIV_SAMPLE_DEPTH_C219)),"sample format mask");
  int occurrences=0;
  for (int i=0; i<DIV_SYSTEM_MAX; ++i) {
    const auto* other=DivEngine::getSystemDef(static_cast<DivSystem>(i));
    if (other && other->id==0xe8) ++occurrences;
  }
  check(occurrences==1,"unique file ID");
  for (int i=0; i<32; ++i) {
    auto c=d->getChanDef(i);
    check(c.type==DIV_CH_PCM && c.insType[0]==DIV_INS_C352 && c.insType[1]==DIV_INS_AMIGA,"channel instrument types");
  }
  setup(e,DIV_SYSTEM_C352);
  auto& types=e.getPossibleInsTypes();
  check(std::count(types.begin(),types.end(),DIV_INS_C352)==1,"dedicated instrument chooser");
  check(std::count(types.begin(),types.end(),DIV_INS_AMIGA)==1,"generic sample fallback");
  check(DivEngine::systemFromFileFur(0xce)==DIV_SYSTEM_C140,"C140 ID unchanged");
  check(DivEngine::systemFromFileFur(0xcf)==DIV_SYSTEM_C219,"C219 ID unchanged");
  for (const int* list: {availableSystems,chipsSample}) {
    int count=0;
    for (const int* p=list; *p; ++p) if (*p==DIV_SYSTEM_C352) ++count;
    check(count==1,"compiled GUI system chooser contains C352 once");
  }
  check(std::string(insTypes[DIV_INS_C352][0])=="C352","compiled instrument chooser name at type 68");
}
void debugFrame(DivDispatch* chip) {
  ImGui::CreateContext();
  auto& io=ImGui::GetIO();
  io.IniFilename=nullptr;
  io.DisplaySize=ImVec2(640,480);
  io.DeltaTime=1.0f/60.0f;
  unsigned char* pixels;
  int width,height;
  io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
  ImGui::NewFrame();
  ImGui::SetNextWindowSize(ImVec2(600,440));
  ImGui::Begin("C352 debug smoke test");
  putDispatchChip(chip,DIV_SYSTEM_C352);
  putDispatchChan(chip->getChanState(0),0,DIV_SYSTEM_C352);
  ImGui::End();
  ImGui::Render();
  check(ImGui::GetDrawData()->TotalVtxCount>0,"production debug UI renders an ImGui frame");
  ImGui::DestroyContext();
}
void roundTrip(DivSystem sys, DivInstrumentType type, unsigned id, unsigned chans) {
  std::vector<unsigned char> saved;
  {
    Engine source;
    auto& e=*source.e;
    bootstrap(e);
    if (sys==DIV_SYSTEM_C352) registry(e);
    setup(e,sys);
    e.song.systemFlags[0].set("customClock",38102400);
    e.song.systemFlags[0].set("quadOutput",true);
    auto* ins=new DivInstrument;
    ins->type=type;
    ins->name="C352 integration fixture";
    ins->amiga.initSample=0;
    ins->amiga.useNoteMap=true;
    for (int i=0; i<180; ++i) {
      ins->amiga.noteMap[i].map=i&1;
      ins->amiga.noteMap[i].freq=i+1;
    }
    setMacro(ins->std.volMacro,255,128);
    setMacro(ins->std.panLMacro,255,32);
    setMacro(ins->std.panRMacro,64,200);
    setMacro(ins->std.arpMacro,0,12);
    setMacro(ins->std.pitchMacro,-17,31);
    setMacro(ins->std.phaseResetMacro,0,1);
    e.song.ins.push_back(ins);
    e.song.insLen=1;
    sample(e,DIV_SAMPLE_DEPTH_8BIT,DIV_SAMPLE_LOOP_FORWARD);
    sample(e,sys==DIV_SYSTEM_C140?DIV_SAMPLE_DEPTH_MULAW:DIV_SAMPLE_DEPTH_C219,DIV_SAMPLE_LOOP_PINGPONG);
    const auto j=serializeInstrument(ins);
    check(j.contains("amiga"),"JSON sample feature");
    check(j["amiga"]["sampleMap"].size()==180,"JSON note map");
    check(j["amiga"]["sampleMap"][61]["map"]==1,"JSON mapped sample");
    // Standalone instrument feature classification must retain its sample map.
    SafeWriter iw;
    iw.init();
    ins->putInsData2(&iw,true,&e.song);
    SafeReader ir(iw.getFinalBuf(),iw.size());
    // These structures are too large for the default Windows thread stack.
    std::unique_ptr<DivInstrument> imported(new DivInstrument);
    std::unique_ptr<DivSong> importedSong(new DivSong);
    check(imported->readInsData(ir,DIV_ENGINE_VERSION,importedSong.get())==DIV_DATA_SUCCESS,"standalone instrument read");
    check(imported->type==type && imported->amiga.useNoteMap && imported->amiga.noteMap[61].map==1,"standalone instrument sample map");
    check(importedSong->sampleLen==2,"standalone instrument embeds its referenced samples");
    importedSong->unload();
    iw.finish();
    auto* w=e.saveFur();
    check(w!=nullptr,"saveFur succeeded");
    saved.assign(w->getFinalBuf(),w->getFinalBuf()+w->size());
    w->finish(); delete w;
    // Persist fixtures as inspection/reproduction artifacts in the ignored build directory.
    std::ofstream file("fixture-"+std::to_string(id)+".fur",std::ios::binary);
    file.write(reinterpret_cast<const char*>(saved.data()),saved.size());
    check(bool(file),"fixture write");
  }
  Engine restored;
  auto& e=*restored.e;
  auto* data=new unsigned char[saved.size()];
  std::copy(saved.begin(),saved.end(),data);
  check(e.load(data,saved.size(),"fixture.fur"),"load into separate engine");
  check(e.song.systemLen==1 && e.song.system[0]==sys,"system round-trip");
  check(DivEngine::systemToFileFur(e.song.system[0])==id,"file ID round-trip");
  check(e.song.chans==int(chans) && e.song.systemChans[0]==chans,"channel count round-trip");
  check(e.song.systemFlags[0].getInt("customClock",0)==38102400,"clock round-trip");
  check(e.song.systemFlags[0].getBool("quadOutput",false),"quadOutput round-trip");
  check(e.song.insLen==1 && e.song.ins[0]->type==type,"instrument type round-trip");
  auto& ins=*e.song.ins[0];
  check(ins.amiga.initSample==0 && ins.amiga.useNoteMap,"initial sample and map enabled");
  for (int i=0; i<180; ++i) check(ins.amiga.noteMap[i].map==(i&1) && ins.amiga.noteMap[i].freq==i+1,"note map round-trip");
  verifyMacro(ins.std.volMacro,255,128);
  verifyMacro(ins.std.panLMacro,255,32);
  verifyMacro(ins.std.panRMacro,64,200);
  verifyMacro(ins.std.arpMacro,0,12);
  verifyMacro(ins.std.pitchMacro,-17,31);
  verifyMacro(ins.std.phaseResetMacro,0,1);
  check(e.song.sampleLen==2,"sample count round-trip");
  for (int n=0; n<2; ++n) {
    auto& s=*e.song.sample[n];
    check(s.depth==(n?(sys==DIV_SYSTEM_C140?DIV_SAMPLE_DEPTH_MULAW:DIV_SAMPLE_DEPTH_C219):DIV_SAMPLE_DEPTH_8BIT),"sample depth round-trip");
    check(s.samples==32 && s.loop && s.loopStart==4 && s.loopEnd==28,"sample size and loop round-trip");
    check(s.loopMode==(n?DIV_SAMPLE_LOOP_PINGPONG:DIV_SAMPLE_LOOP_FORWARD),"loop mode round-trip");
    auto* bytes=static_cast<unsigned char*>(s.getCurBuf());
    for (int i=0; i<32; ++i) check(bytes[i]==((i*13+7)&255),"sample bytes round-trip");
  }
  e.initDispatch();
  auto* chip=e.getDispatch(0);
  if (sys==DIV_SYSTEM_C352) {
    check(dynamic_cast<DivPlatformC352*>(chip)!=nullptr,"production C352 dispatch factory");
    check(chip->getOutputCount()==4 && chip->rate==132300,"restored clock and quad dispatch");
    check(chip->getRegisterPoolDepth()==16 && chip->getRegisterPoolSize()==0x203 && chip->getRegisterSheet()!=nullptr,"common register viewer data");
    debugFrame(chip);
  } else {
    check(dynamic_cast<DivPlatformC140*>(chip)!=nullptr,"C140/C219 factory unchanged");
  }
  // Unreferenced sample is removed; samples used only through the map must survive.
  sample(e,DIV_SAMPLE_DEPTH_8BIT,DIV_SAMPLE_LOOP_FORWARD);
  e.delUnusedSamples();
  check(e.song.sampleLen==2,"sample map deletion protection");
  check(chip->isSampleLoaded(0,0) && chip->isSampleLoaded(0,1),"restored samples render to ROM");
  if (sys==DIV_SYSTEM_C352) {
    chip->dispatch(DivCommand(DIV_CMD_INSTRUMENT,0,0));
    chip->dispatch(DivCommand(DIV_CMD_NOTE_ON,0,61));
    chip->tick(true);
    short audio[4][16]={};
    short* bufs[]={audio[0],audio[1],audio[2],audio[3]};
    chip->acquire(bufs,16);
    const auto* regs=reinterpret_cast<const unsigned short*>(chip->getRegisterPool());
    check((regs[3]&C352Core::MULAW)!=0,"dedicated instrument resolves compressed mapped sample");
    check(regs[0]==0xff40,"dedicated volume/panning use 255 scale");
    e.song.systemFlags[0].clear();
    e.updateSysFlags(0,false,false);
    check(chip->getOutputCount()==2 && chip->rate==88200,"default stereo and clock");
    // Start a new stereo container, then enable quad through the production flag path.
    e.quitDispatch();
    e.initDispatch();
    e.song.systemFlags[0].set("quadOutput",true);
    e.updateSysFlags(0,false,true);
    check(e.getDispatch(0)->getOutputCount()==4,"live stereo-to-quad change");
    // Exercise the container's existing lazy allocation for newly enabled outputs.
    DivDispatchContainer container;
    DivConfig flags;
    container.init(DIV_SYSTEM_C352,&e,32,44100,flags);
    container.setRates(44100);
    container.acquire(16);
    flags.set("quadOutput",true);
    container.dispatch->setFlags(flags);
    container.setRates(44100);
    container.acquire(16);
    for (int i=0; i<4; ++i) check(container.bb[i] && container.bbIn[i] && container.bbOut[i],"newly enabled output buffers allocated");
    container.quit();
  }
}
}
int main(int, char**) {
  try {
    roundTrip(DIV_SYSTEM_C352,DIV_INS_C352,0xe8,32);
    roundTrip(DIV_SYSTEM_C140,DIV_INS_C140,0xce,24);
    roundTrip(DIV_SYSTEM_C219,DIV_INS_C219,0xcf,16);
    std::cout << "PASS: " << checks << " registry, persistence, asset and dispatch assertions\n";
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "FAIL after " << checks << " assertions: " << ex.what() << '\n';
    return 1;
  }
}
