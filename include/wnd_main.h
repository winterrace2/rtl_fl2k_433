/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *              rtl_fl2k_433 main window (core part)               *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_MAIN_INCLUDED
	#define WND_MAIN_INCLUDED

extern char copybuf[4096]; // todo: move to more appropriate place?

int GuiMain();
BOOL CopyToClipboard(LPSTR txt); // todo: move to more appropriate place?

#endif // WND_MAIN_INCLUDED
