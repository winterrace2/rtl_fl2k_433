/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *        Option sub-dialog for TX frequency configuration         *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowFrequencyTxDialog():   *
 * - updates fl2k->cfg.<carrier | samp_rate>                       *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_TXOPT_FREQ_INCLUDED
	#define WND_TXOPT_FREQ_INCLUDED

	#include "libfl2k_433.h"

	#define FL2K_MULT_MIN 3
	#define FL2K_MULT_MAX 6

	#define FL2K_DIV_MIN 2
	#define FL2K_DIV_MAX 63

	#define FL2K_FRAC_MIN 1
	#define FL2K_FRAC_MAX 15

	#define MAX_DSP_HARMONIC 20

	#define FL2K_DEFAULT_MULT 6
	#define FL2K_DEFAULT_DIV 2
	#define FL2K_DEFAULT_FRAC 1

	#define FL2K_PRESET1_SRATE 85555554
	#define	FL2K_PRESET1_CARRIER1 6183693
	#define	FL2K_PRESET1_CARRIER2 0
	
	#define FL2K_PRESET2_SRATE 42777770
	#define	FL2K_PRESET2_CARRIER1 6183693
	#define	FL2K_PRESET2_CARRIER2 0

	#define FL2K_PRESET3_SRATE 21388878
	#define	FL2K_PRESET3_CARRIER1 6183693
	#define	FL2K_PRESET3_CARRIER2 0

	#define FL2K_PRESET4_SRATE 85555554
	#define	FL2K_PRESET4_CARRIER1 6153693
	#define	FL2K_PRESET4_CARRIER2 6213693

	#define FL2K_PRESET5_SRATE 85555554
	#define	FL2K_PRESET5_CARRIER1 6213693
	#define	FL2K_PRESET5_CARRIER2 6153693

	#define CARRIER_RESOLUTION 10000 // Initialize Slider with 10kHz steps

	int ShowFrequencyTxDialog(fl2k_433_t *fl2k, HWND hParent);

#endif // WND_TXOPT_FREQ_INCLUDED
