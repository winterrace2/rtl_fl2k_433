/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *               rtl_fl2k_433 main window (TX part)                *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_MAINTX_INCLUDED
	#define WND_MAINTX_INCLUDED

#include "librtl_433.h"
#include "rxlist_entry.h"

BOOL TxGui_prepare();
VOID TxGui_onInit(HWND hMainWnd, HMENU poptx);
VOID TxGui_cleanup();
BOOL TxGui_onCommand(WPARAM wParam, LPARAM lParam);
BOOL TxGui_onChangedItem(LPNMLISTVIEW ctx);
BOOL TxGui_onKey(LPNMLVKEYDOWN ctx);
BOOL TxGui_onRightClick(LPNMHDR ctx_in);
BOOL TxGui_onDoubleClick(LPNMHDR ctx_in);
VOID TxGui_onSizeChange(int add_w, int add_h, double fac_w, double fac_h, BOOL redraw);
BOOL TxGui_shutdownRequest();
VOID TxGui_shutdownConfirm();
BOOL TxGui_isActive();
VOID QueueRxPulseToTx(rx_entry *entry, BOOL entirepulse = FALSE);

#endif // WND_MAINTX_INCLUDED
