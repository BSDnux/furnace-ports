// license:BSD-3-Clause
// Copyright R. Belmont, superctr.
// Standalone Furnace adaptation of the MAME C352 emulation.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "c352.h"
#include <cstring>
#include <algorithm>

constexpr size_t C352Core::ROM_SIZE;

namespace {
// Explicit arithmetic shift and signed wrapping, also valid before C++20.
int32_t shift(int64_t value, unsigned bits) {
  const int64_t divisor=int64_t(1)<<bits;
  return static_cast<int32_t>(value>=0 ? value/divisor : -((-value+divisor-1)/divisor));
}
int16_t wrap(int32_t value) {
  const uint16_t bits=static_cast<uint16_t>(value);
  return bits<0x8000 ? bits : static_cast<int32_t>(bits)-0x10000;
}
uint16_t& reg(C352Core::Voice& v, unsigned index) {
  switch (index) {
    case 0: return v.vol_f;
    case 1: return v.vol_r;
    case 2: return v.freq;
    case 3: return v.flags;
    case 4: return v.wave_bank;
    case 5: return v.wave_start;
    case 6: return v.wave_end;
    default: return v.wave_loop;
  }
}
}

C352Core::C352Core(): rom(nullptr), romSize(0) {
  int value=0;
  for (int i=0; i<128; i++) {
    mulaw[i]=value*32;
    mulaw[i+128]=-mulaw[i]-32;
    value+=i<16 ? 1 : i<24 ? 2 : i<48 ? 4 : i<100 ? 8 : 16;
  }
  reset();
}

void C352Core::setROM(const uint8_t* data, size_t size) {
  rom=data;
  romSize=std::min(size,ROM_SIZE);
}

void C352Core::reset() {
  std::memset(voices,0,sizeof(voices));
  std::memset(output,0,sizeof(output));
  std::memset(voiceOutput,0,sizeof(voiceOutput));
  std::memset(muted,0,sizeof(muted));
  noise=0x1234;
  control=0;
}

uint16_t C352Core::read(uint32_t address) const {
  if (address<0x100) {
    const Voice& v=voices[address/8];
    switch (address&7) {
      case 0: return v.vol_f;
      case 1: return v.vol_r;
      case 2: return v.freq;
      case 3: return v.flags;
      case 4: return v.wave_bank;
      case 5: return v.wave_start;
      case 6: return v.wave_end;
      default: return v.wave_loop;
    }
  }
  return address==0x200 ? control : 0;
}

void C352Core::write(uint32_t address, uint16_t value, uint16_t mask) {
  if (address<0x100) {
    uint16_t& r=reg(voices[address/8],address&7);
    r=(r&~mask)|(value&mask);
  } else if (address==0x200) {
    control=(control&~mask)|(value&mask);
  } else if (address==0x202 && mask==0xffff) {
    for (Voice& v: voices) {
      if (v.flags&KEYON) {
        v.pos=(uint32_t(v.wave_bank)<<16)|v.wave_start;
        v.counter=0xffff;
        v.sample=v.last_sample=0;
        v.flags=(v.flags|BUSY)&~(KEYON|LOOPHIST);
        std::memset(v.curr_vol,0,sizeof(v.curr_vol));
      }
      if (v.flags&KEYOFF) {
        v.flags&=~(BUSY|KEYOFF);
        v.counter=0xffff;
      }
    }
  }
}

void C352Core::mute(unsigned voice, bool muteValue) {
  if (voice<32) muted[voice]=muteValue;
}

void C352Core::fetch(Voice& v) {
  v.last_sample=v.sample;
  if (v.flags&NOISE) {
    noise=(noise>>1)^((noise&1) ? 0xfff6 : 0);
    v.sample=wrap(noise);
    return;
  }
  const uint32_t address=v.pos&0xffffff;
  const uint8_t data=(rom && address<romSize) ? rom[address] : 0;
  v.sample=(v.flags&MULAW) ? mulaw[data] : (data<128 ? int(data) : int(data)-256)*256;
  const uint16_t pos=v.pos;
  if ((v.flags&(LOOP|REVERSE))==(LOOP|REVERSE)) {
    if ((v.flags&LDIR) && pos==v.wave_loop) v.flags&=~LDIR;
    else if (!(v.flags&LDIR) && pos==v.wave_end) v.flags|=LDIR;
    v.pos+=(v.flags&LDIR) ? -1 : 1;
  } else if (pos==v.wave_end) {
    if (v.flags&LOOP) {
      v.pos=(v.flags&LINK) ? (uint32_t(v.wave_start)<<16)|v.wave_loop : (v.pos&0xff0000)|v.wave_loop;
      v.flags|=LOOPHIST;
    } else {
      v.flags=(v.flags|KEYOFF)&~BUSY;
      v.sample=0;
    }
  } else {
    v.pos+=(v.flags&REVERSE) ? -1 : 1;
  }
}

void C352Core::tick() {
  int32_t sum[4]={};
  for (int i=0; i<32; i++) {
    Voice& v=voices[i];
    int32_t sample=0;
    if (v.flags&BUSY) {
      const uint32_t next=v.counter+v.freq;
      if (next&0x10000) fetch(v);
      if ((next^v.counter)&0x18000) {
        const uint8_t target[4]={uint8_t(v.vol_f>>8),uint8_t(v.vol_f),uint8_t(v.vol_r>>8),uint8_t(v.vol_r)};
        for (int o=0; o<4; o++) {
          if (v.curr_vol[o]<target[o]) ++v.curr_vol[o];
          else if (v.curr_vol[o]>target[o]) --v.curr_vol[o];
        }
      }
      v.counter=next&0xffff;
      sample=(v.flags&FILTER) ? v.sample : v.last_sample+shift(int64_t(v.counter)*(v.sample-v.last_sample),16);
    }
    const unsigned phase[4]={PHASEFL,PHASEFR,PHASERL,PHASEFR};
    for (int o=0; o<4; o++) {
      voiceOutput[i][o]=muted[i] ? 0 : shift(int64_t((v.flags&phase[o]) ? -sample : sample)*v.curr_vol[o],8);
      sum[o]+=voiceOutput[i][o];
    }
  }
  for (int o=0; o<4; o++) output[o]=wrap(shift(sum[o],3));
}
