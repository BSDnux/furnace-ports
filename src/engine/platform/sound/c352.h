// license:BSD-3-Clause
// C352 emulation based on MAME, copyright R. Belmont and superctr.
// Standalone Furnace adaptation. See c352.cpp for the full license.
#ifndef FURNACE_C352_CORE_H
#define FURNACE_C352_CORE_H

#include <cstddef>
#include <cstdint>

class C352Core {
public:
  enum Flag {
    BUSY=0x8000, KEYON=0x4000, KEYOFF=0x2000, LOOPTRG=0x1000,
    LOOPHIST=0x0800, FM=0x0400, PHASERL=0x0200, PHASEFL=0x0100,
    PHASEFR=0x0080, LDIR=0x0040, LINK=0x0020, NOISE=0x0010,
    MULAW=0x0008, FILTER=0x0004, LOOP=0x0002, REVERSE=0x0001
  };
  struct Voice {
    uint32_t pos, counter;
    int16_t sample, last_sample;
    uint16_t vol_f, vol_r;
    uint8_t curr_vol[4];
    uint16_t freq, flags, wave_bank, wave_start, wave_end, wave_loop;
  };
  static constexpr size_t ROM_SIZE=0x1000000;
  Voice voices[32];
  int16_t mulaw[256];
  int16_t output[4];
  int32_t voiceOutput[32][4]; // Per-voice contributions before the final /8.
  uint16_t noise, control;

  C352Core();
  void setROM(const uint8_t* data, size_t size);
  void reset();
  uint16_t read(uint32_t address) const;
  void write(uint32_t address, uint16_t value, uint16_t mask=0xffff);
  void mute(unsigned voice, bool muted);
  void tick();

private:
  const uint8_t* rom;
  size_t romSize;
  bool muted[32];
  void fetch(Voice& voice);
};
#endif
