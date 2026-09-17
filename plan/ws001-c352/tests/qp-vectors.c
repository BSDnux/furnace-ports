// Comparison-only harness. GPL-2.0-or-later, like the included QuattroPlay source.
#include <assert.h>
#include <stdio.h>
#include "qp-source.c"
int main(void) {
  C352 c={0};
  uint8_t wave[4]={0x40,0x40,0x40,0x40};
  assert(C352_init(&c,25401600)==88200);
  c.wave=wave; c.wave_mask=3;
  C352_write(&c,0,0xffff); C352_write(&c,1,0xffff);
  C352_write(&c,2,0xffff); C352_write(&c,6,0);
  C352_write(&c,3,0x4004); C352_write(&c,0x202,0);
  C352_update(&c);
  assert(c.v[0].sample==16384); // MAME zeroes sample on non-loop terminal.
  assert(c.v[0].curr_vol[0]==255); // MAME ramps to 1, even with FILTER.
  assert(c.out[0]==4177920.0); // Unscaled double accumulator, not MAME int16 output.
  C352_write(&c,0x200,0x1234); C352_write(&c,0x201,0x5678);
  assert(c.control1==0x1234 && c.control2==0x5678);
  assert(C352_read(&c,0x200)==0); // Unlike MAME control readback.
  C352_write(&c,3,0x4006); C352_write(&c,0x202,0); C352_update(&c);
  C352_write(&c,3,0x8106); C352_update(&c);
  assert(c.out[0]>0); // Phase flag is latched, unlike MAME live flags.
  assert(c.mulaw_table[128]==-32 && c.mulaw_table[255]==-31264);
  puts("PASS: 8 QuattroPlay comparison assertions (clock/terminal/ramp/output/control/latch/mu-law)");
  return 0;
}
