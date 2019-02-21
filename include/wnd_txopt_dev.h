/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *          Option sub-dialog for FL2K device selection            *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowDeviceTxDialog():      *
 * - updates fl2k->cfg.dev_index                                  *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_TXOPT_DEV_INCLUDED
	#define WND_TXOPT_DEV_INCLUDED

	#include "libfl2k_433.h"
	int ShowDeviceTxDialog(fl2k_433_t *fl, HWND hParent, char **trgdev_name);

#endif // WND_TXOPT_DEV_INCLUDED
