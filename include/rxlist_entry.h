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
	//uint64_t offset;            ///< Offset to first pulse in number of samples from start of stream.
	uint32_t sample_rate;       ///< Sample rate the pulses are recorded with.
	//unsigned start_ago;         ///< Start of first pulse in number of samples ago.
	//unsigned end_ago;           ///< End of last pulse in number of samples ago.
	unsigned int num_pulses;
	int *pulse;	                ///< Width of pulses (high) in number of samples.
	int *gap;	                ///< Width of gaps between pulses (low) in number of samples.
	//int ook_low_estimate;       ///< Estimate for the OOK low level (base noise level) at beginning of package.
	//int ook_high_estimate;      ///< Estimate for the OOK high level at end of package.
	//int fsk_f1_est;             ///< Estimate for the F1 frequency for FSK.
	//int fsk_f2_est;             ///< Estimate for the F2 frequency for FSK.
	float freq1_hz;
	float freq2_hz;
	float rssi_db;
	float snr_db;
	float noise_db;
	unsigned int segment_startidx;
	unsigned int segment_len;
	unsigned int num_samples;
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
		int copyPulses(const pulse_data_t *pulses, unsigned pulseexc_startidx, unsigned pulseexc_len);
		pPulseDatCompact getPulses();
};

#endif // RXLIST_ENTRY_INCLUDED
