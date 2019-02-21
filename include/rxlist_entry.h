/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *              Definition of entries in the RX list               *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef RXLIST_ENTRY_INCLUDED
	#define RXLIST_ENTRY_INCLUDED

	#include "librtl_433.h"
	#include "pulse_detect.h"

// Relevant modulation types
typedef enum {
	SIGNAL_MODULATION_UNK = 0,
	SIGNAL_MODULATION_OOK = 1,
	SIGNAL_MODULATION_FSK = 2
} sigmod;


typedef struct _PulseDatCompact {
	unsigned int num_pulses;
	int *pulse;	// Contains widths of pulses	(high)
	int *gap;	// Width of gaps between pulses (low)
	unsigned int segment_startidx;
	unsigned int segment_len;
	unsigned int samplerate;
	unsigned int num_samples;
	unsigned int frequency;
} PulseDatCompact, *pPulseDatCompact;

class rx_entry {
	private:
		int devid;
		char time[32];
		char type[32];
		char model[64];
		PulseDatCompact pdat;
		sigmod mod;
		data_t *data;

	public:
		rx_entry();
		~rx_entry();
		void setTime(char *time);
		void setType(char *time);
		void setModel(char *time);
		void setDevId(int id);
		void setModulation(sigmod sm);
		char *getTime();
		char *getType();
		char *getModel();
		sigmod getModulation();
		int getDevId();
		int copyData(data_t *data);
		data_t *getData(void);
		int copyPulses(const pulse_data_t *pulses, unsigned freq, unsigned samprate, unsigned pulseexc_startidx, unsigned pulseexc_len);
		pPulseDatCompact getPulses();
};

#endif // RXLIST_ENTRY_INCLUDED
