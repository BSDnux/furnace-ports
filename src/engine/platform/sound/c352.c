/*
    C352 chip emulator for QuattroPlay
    written by ctr with help from MAME source code (written by R.Belmont)

    modified by BSDnux for furance
    perhaps internal registers are readable at 0x100-0x1ff
*/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "c352.h"

int C352_init(C352 *c, uint32_t clk)
{
    c->mute_mask=0;
    c->rate = clk/288;

    memset(c->voice,0,sizeof(C352_Voice)*C352_VOICES);
    memset(c->out,0,4*sizeof(int16_t));

    c->control1 = 0;
    c->control2 = 0;
    c->random = 0x1234;

    c352_set_mulaw_type(c,C352_MULAW_TYPE_C352);

    return c->rate;
}

// Generated tables verified with Wii Virtual Console emulators
// (Starblade, Knuckle Heads)
void C352_set_mulaw_type(C352 *c,int mulaw_type)
{
    int i=0, j=0;
    c->mulaw_type = mulaw_type;
    switch(mulaw_type)
    {
    default:
    case C352_MULAW_TYPE_C352:
        for(i=0;i<128;i++)
        {
            c->mulaw_table[i] = j<<5;
            if(i < 16)
                j += 1;
            else if(i < 24)
                j += 2;
            else if(i < 48)
                j += 4;
            else if(i < 100)
                j += 8;
            else
                j += 16;
        }
        for(i=128;i<256;i++)
            c->mulaw_table[i] = (~c->mulaw_table[i-128])&0xffe0;
        break;
    case C352_MULAW_TYPE_C140:
        for(i=0;i<256;i++)
        {
            j=(int8_t)i;
            int8_t s1 = j&7;
            int8_t s2 = abs(j>>3)&31;

            c->mulaw_table[i] = 0x80<<s1 & 0xff00;
            c->mulaw_table[i] += s2<<(s1 ? s1+3 : 4);

            if(j<0)
                c->mulaw_table[i] = -c->mulaw_table[i];
        }
        break;
    }
}

uint16_t C352RegMap[8] = {
    offsetof(C352_Voice,vol_f),
    offsetof(C352_Voice,vol_r),
    offsetof(C352_Voice,freq),
    offsetof(C352_Voice,flags),
    offsetof(C352_Voice,sample_bank),
    offsetof(C352_Voice,sample_start),
    offsetof(C352_Voice,sample_end),
    offsetof(C352_Voice,sample_loop),
};

void C352_write(C352 *c, uint16_t addr, uint16_t data)
{

    int i;

    if(addr < 0x100)
    {
        *(uint16_t*)((void*)&c->voice[addr/8]+C352RegMap[addr%8]) = data;
        if((addr%8) == C352_FLAGS && !(data & C352_FLG_KEYON))
        ;
    }
    else if(addr == 0x200)
        c->control1 = data;
    else if(addr == 0x201)
        c->control2 = data;
    else if(addr == 0x202) // execute keyons/keyoffs
    {
        for(i=0;i<C352_VOICES;i++)
        {
            if(c->voice[i].flags & C352_FLG_KEYON)
            {
                c->voice[i].pos = (c->voice[i].sample_bank<<16) | c->voice[i].sample_start;

				c->voice[i].counter = 0xffff;

                c->voice[i].flags |= C352_FLG_BUSY;
                c->voice[i].flags &= ~(C352_FLG_KEYON|C352_FLG_LOOPHIST);

                c->voice[i].latch_flags = c->voice[i].flags;


                c->voice[i].curr_vol[0] = c->voice[i].curr_vol[1] = 0;
                c->voice[i].curr_vol[2] = c->voice[i].curr_vol[3] = 0;
            }
            if(c->voice[i].flags & C352_FLG_KEYOFF)
            {
                c->voice[i].flags &= ~(C352_FLG_BUSY|C352_FLG_KEYOFF);

                c->voice[i].counter = 0xffff;
            }
        }
    }
}

uint16_t C352_read(C352 *c, uint16_t addr)
{
    if(addr < 0x100)
        return *(uint16_t*)((void*)&c->voice[addr/8]+C352RegMap[addr%8]);
    else
        return 0;
}


static inline void C352_fetch_sample(C352 *c, int i)
{
    C352_Voice *v = &c->voice[i];
	v->last_sample = v->sample;

    if(~v->flags & C352_FLG_BUSY)
    {
        v->sample = 0;
    }
    else if(v->flags & C352_FLG_NOISE)
    {
        c->random = (c->random>>1) ^ ((-(c->random&1)) & 0xfff6);
        v->sample = c->random;
	}
	else
	{
		int8_t s;

		s = (int8_t)c->sample_mem[v->pos&c->sample_mask];

		if(v->flags & C352_FLG_MULAW)
            v->sample = c->mulaw_table[s&0xff];
		else
			v->sample = s<<8;

		uint16_t pos = v->pos&0xffff;

        if((v->flags & C352_FLG_LOOP) && v->flags & C352_FLG_REVERSE)
        {
			// backwards>forwards
            if((v->flags & C352_FLG_LDIR) && pos == v->sample_loop)
                v->flags &= ~C352_FLG_LDIR;
			// forwards>backwards
            else if(!(v->flags & C352_FLG_LDIR) && pos == v->sample_end)
                v->flags |= C352_FLG_LDIR;

			v->pos += (v->flags&C352_FLG_LDIR) ? -1 : 1;
        }
        else if(pos == v->sample_end)
        {
            if((v->flags & C352_FLG_LINK) && (v->flags & C352_FLG_LOOP))
            {
                v->pos = (v->sample_start<<16) | v->sample_loop;
                v->flags |= C352_FLG_LOOPHIST;
            }
            else if(v->flags & C352_FLG_LOOP)
            {
                v->pos = (v->pos&0xff0000) | v->sample_loop;
                v->flags |= C352_FLG_LOOPHIST;
            }
            else
            {
                v->flags |= C352_FLG_KEYOFF;
                v->flags &= ~C352_FLG_BUSY;
            }
        }
		else
		{
			v->pos += (v->flags&C352_FLG_REVERSE) ? -1 : 1;
		}
	}
}


static inline void C352_update_volume(C352 *c,int i,int ch,uint8_t v)
{
    // disabling filter also disables volume ramp?
    if(c->voice[i].latch_flags & C352_FLG_FILTER)
        c->voice[i].curr_vol[ch] = v;

    // do volume ramping to prevent clicks
    int16_t vol_delta = c->voice[i].curr_vol[ch] - v;
    if(vol_delta != 0)
        c->voice[i].curr_vol[ch] += (vol_delta>0) ? -1 : 1;
}

static inline int16_t C352_update_voice(C352 *c, int i)
{
    C352_Voice *v = &c->voice[i];

    uint32_t next_counter = v->counter + v->freq;

	if(next_counter & 0x10000)
        C352_fetch_sample(c,i);

	if((next_counter^v->counter) & 0x18000)
    {
        C352_update_volume(c,i,0,c->voice[i].vol_f>>8);
        C352_update_volume(c,i,1,c->voice[i].vol_f&0xff);
        C352_update_volume(c,i,2,c->voice[i].vol_r>>8);
        C352_update_volume(c,i,3,c->voice[i].vol_r&0xff);
    }

	v->counter = next_counter&0xffff;

	int32_t temp = v->sample;
	// Interpolate samples
    if((v->latch_flags & C352_FLG_FILTER) == 0)
         temp = v->last_sample + (v->counter*(v->sample-v->last_sample)>>16);

    return temp;
}

void C352_update(C352 *c)
{
    int i;
    int16_t s;
    uint16_t flags;

    c->out[0]=c->out[1]=c->out[2]=c->out[3]=0;

    for(i=0;i<C352_VOICES;i++)
    {
        s = C352_update_voice(c,i);

        if(!(c->mute_mask & 1<<i))
        {
            flags = c->voice[i].latch_flags;

            // Left
            c->out[0] += (flags & C352_FLG_PHASEFL) ? -s * (c->voice[i].curr_vol[0])
                                                    :  s * (c->voice[i].curr_vol[0]);
            c->out[2] += (flags & C352_FLG_PHASERL) ? -s * (c->voice[i].curr_vol[2])
                                                    :  s * (c->voice[i].curr_vol[2]);

            // Right
            c->out[1] += (flags & C352_FLG_PHASEFR) ? -s * (c->voice[i].curr_vol[1])
                                                    :  s * (c->voice[i].curr_vol[1]);
            c->out[3] += (flags & C352_FLG_PHASEFR) ? -s * (c->voice[i].curr_vol[3])
                                                    :  s * (c->voice[i].curr_vol[3]);
        }
    }
}
