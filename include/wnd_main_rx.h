/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *               rtl_fl2k_433 main window (RX part)                *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_MAINRX_INCLUDED
	#define WND_MAINRX_INCLUDED

#include "librtl_433.h"

#define SCOPE_ZOOM_MIN 100	// Minimal Zoom is 1.00
#define SCOPE_ZOOM_MAX 2000 // Maximal Zoom is 20.00

#define SCOPE_OFFSET_MIN   0  // Minimal Offset is 0.000
#define SCOPE_OFFSET_MAX 1000 // Maximal Offset is 1.000

BOOL RxGui_prepare();
VOID RxGui_onInit(HWND hMainWnd, HMENU hMainMenu, HMENU poprx, HMENU popdet, HMENU popfreq, HMENU popflex, HMENU popprot);
VOID RxGui_cleanup();
BOOL RxGui_onCommand(WPARAM wParam, LPARAM lParam);
BOOL RxGui_onChangedItem(LPNMLISTVIEW ctx);
BOOL RxGui_onKey(LPNMLVKEYDOWN ctx);
BOOL RxGui_onRightClick(LPNMHDR ctx_in);
BOOL RxGui_onDoubleClick(LPNMHDR ctx_in);
VOID RxGui_onSizeChange(int add_w, int add_h, double fac_w, double fac_h, BOOL redraw);
VOID RxGui_onHorScroll(HWND hElem);
BOOL RxGui_shutdownRequest();
VOID RxGui_shutdownConfirm();
BOOL RxGui_isActive();
BOOL RxGui_passFile(LPSTR path);

#endif // WND_MAINRX_INCLUDED
