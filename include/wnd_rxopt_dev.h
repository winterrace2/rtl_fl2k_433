/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *        Option sub-dialog for RTL-SDR device selection           *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowDeviceDialog():        *
 * - updates cfg-> <dev_query|dev_gain|ppm_error>                  *
 * - can provide a pointer to the device name (param trgdev_name)  *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_RXOPT_DEV_INCLUDED
	#define WND_RXOPT_DEV_INCLUDED

	#include "librtl_433.h"

	int ShowDeviceRxDialog(rtl_433_t *rl, HWND hParent, char **trgdev_name);

#endif // WND_RXOPT_DEV_INCLUDED
