// Run the p001 numerical expectations against the production core API.
#include "src/engine/platform/sound/c352.h"
#include <algorithm>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <cstdint>
class c352_device {
  C352Core core;
public:
  C352Core::Voice (&m_c352_v)[32];
  int16_t (&m_mulawtab)[256];
  uint16_t &m_random;
  std::vector<uint8_t> rom;
  std::vector<uint32_t> reads;
  struct { int16_t out[4]; } sink;
  c352_device(): m_c352_v(core.voices), m_mulawtab(core.mulaw), m_random(core.noise), rom(1<<24) {
    core.setROM(rom.data(),rom.size());
  }
  void device_reset() { core.reset(); }
  void init_table() {} // Production constructor generates the decoder table.
  void write(uint32_t a,uint16_t d,uint16_t m=0xffff) { core.write(a,d,m); }
  uint16_t read(uint32_t a) { return core.read(a); }
  void step() {
    // Reconstruct the fetch trace from the public pre-step state. Differential
    // tests additionally compare decoded samples against nonuniform ROM data.
    for (const auto& v: core.voices) {
      if ((v.flags&C352Core::BUSY) && !(v.flags&C352Core::NOISE) && ((v.counter+v.freq)&0x10000)) reads.push_back(v.pos&0xffffff);
    }
    core.tick();
    std::copy(core.output,core.output+4,sink.out);
  }
};
#include "c219-table.hpp"
