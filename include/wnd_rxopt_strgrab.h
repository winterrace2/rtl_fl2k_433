/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *         Sub-dialog to configure dumping of sample stream        *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_RXOPT_STRGRAB_INCLUDED
	#define WND_RXOPT_STRGRAB_INCLUDED

	#include "librtl_433.h"

	int ShowStreamGrabberDialog(rtl_433_t *rl, HWND hParent);

#endif // WND_RXOPT_STRGRAB_INCLUDED
