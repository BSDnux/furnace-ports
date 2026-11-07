// license:BSD-3-Clause
// copyright-holders:Ville Linde
/* Ricoh RF5C400 emulator */

#ifndef _RF5C400_H
#define _RF5C400_H

#pragma once

namespace rf5c400
{
  typedef unsigned char       uint8_t;
  typedef signed char         s8;
  typedef unsigned short     uint16_t;
  typedef signed short       s16;
  typedef unsigned int       uint32_t;
  typedef signed int         s32;
  typedef unsigned int       uint64_t;
  typedef signed int      offs_t;
}

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************


// ======================> rf5c400_device

using namespace rf5c400;
class rf5c400_device
{
public:
	rf5c400_device();

	uint16_t rf5c400_r(offs_t offset, uint16_t mem_mask = ~0);
	void rf5c400_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	void device_start(uint8_t *ext_mem);
	void device_reset();

	void sound_stream_update(s16 **outputs, s16 **channel_outputs, uint32_t samples);

protected:
	rf5c400_device(int output_bits);

private:
	static constexpr unsigned NUM_CHANNELS = 32;
	struct rf5c400_channel
	{
		rf5c400_channel() { }

		uint16_t startH;
		uint16_t startL;
		uint16_t freq;
		uint16_t endL;
		uint16_t endHloopH;
		uint16_t loopL;
		uint16_t pan;
		uint16_t effect;
		uint16_t volume;

		uint16_t attack;
		uint16_t decay;
		uint16_t release;

		uint16_t cutoff;

		uint64_t pos;
		uint64_t step;
		uint16_t keyon;

		uint8_t env_phase;
		double env_level;
		double env_step;
		double env_scale;

	};

	class envelope_tables
	{
	public:
		envelope_tables();
		void init(uint32_t clock);
		double ar(rf5c400_channel const &chan) const { return m_ar[decode80(chan.attack >> 8)]; }
		double dr(rf5c400_channel const &chan) const { return m_dr[decode80(chan.decay >> 8)]; }
		double rr(rf5c400_channel const &chan) const { return m_rr[decode80(chan.release >> 8)]; }
	private:
		static constexpr uint8_t decode80(uint8_t val) { return (val & 0x80) ? ((val & 0x7f) + 0x1f) : val; }
		double m_ar[0x9f];
		double m_dr[0x9f];
		double m_rr[0x9f];
	};

	uint8_t decode80(uint8_t val);

	envelope_tables m_env_tables;

	rf5c400_channel m_channels[32];

	uint16_t m_rf5c400_status;
	uint32_t m_ext_mem_address;
	uint16_t m_ext_mem_data;
	uint16_t m_req_channel;
};

#endif // _RF5C400_H
