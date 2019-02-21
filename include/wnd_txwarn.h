/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                    sub-dialog for TX warning                    *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowDeviceTxDialog():      *
 * - displays warning message and returns TRUE if user accepted    *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_TXWARN_INCLUDED
	#define WND_TXWARN_INCLUDED

	BOOL ShowWarningDialog(HWND hParent);

#endif // WND_TXWARN_INCLUDED
