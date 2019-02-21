/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *       Option sub-dialog for expert settings regarding TX        *
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

#ifndef WND_TXOPT_EXP_INCLUDED
	#define WND_TXOPT_EXP_INCLUDED

	#include "libfl2k_433.h"

	int ShowExpertDialog(fl2k_433_t *fl, HWND hParent);

#endif // WND_TXOPT_EXP_INCLUDED
