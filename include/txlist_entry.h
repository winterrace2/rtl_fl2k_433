/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *              Definition of entries in the TX list               *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TXLIST_ENTRY_INCLUDED
	#define TXLIST_ENTRY_INCLUDED

	#include "libfl2k_433.h"

class tx_entry {
	private:
		TxMsg msg;
		char description[200];
		char time_sent[20];

	public:
		tx_entry(char *description, mod_type mod, char *buf, uint32_t len, uint32_t samp_rate);
		~tx_entry();

		void setSentTime(char *timestr);
		char *getSentTime();
		char *getDescription();
		TxMsg *getMessage();
};

#endif // RXLIST_ENTRY_INCLUDED
