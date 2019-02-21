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

#include <windows.h>
#include <stdio.h>
#include <string.h>

#include <memory.h>
#include <malloc.h>

#include "txlist_entry.h"

// this will copy msg and description.
// take care that msg->buf must not be freed by the caller, it will be freed by the tx_entry desctructor

tx_entry::tx_entry(char *description, mod_type mod, char *buf, uint32_t len, uint32_t samp_rate) {
	strcpy_s(this->description, sizeof(this->description), description);
	this->msg.mod = mod;
	this->msg.buf = buf;
	this->msg.len = len;
	this->msg.samp_rate = samp_rate;
	this->msg.next = NULL;
	memset(this->time_sent, 0, sizeof(this->time_sent));
}

tx_entry::~tx_entry() {
	if (this->msg.buf) free(this->msg.buf);
}

void tx_entry::setSentTime(char *timestr) {
	strcpy_s(this->time_sent, sizeof(this->time_sent), timestr ? timestr : "");
}

char *tx_entry::getSentTime() {
	return (this->time_sent[0] ? this->time_sent : NULL);
}

char *tx_entry::getDescription() {
	return this->description;
}

TxMsg *tx_entry::getMessage() {
	return &this->msg;
}
