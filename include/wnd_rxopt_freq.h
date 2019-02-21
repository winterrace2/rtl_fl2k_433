/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *        Option sub-dialog for RX frequency configuration         *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowFrequencyRxDialog():   *
 * - updates cfg-><frequencies | hop_time>                         *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_RXOPT_FREQ_INCLUDED
	#define WND_RXOPT_FREQ_INCLUDED

	#include "librtl_433.h"

	#define OPT_FREQ_COLWIDTH 200

	int ShowFrequencyRxDialog(rtl_433_t *rl, HWND hParent, HMENU hPopupMenu);

#endif // WND_RXOPT_FREQ_INCLUDED
