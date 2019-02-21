/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  A class to manage window elements (to better handle resizing)  *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef INCLUDE_WNDELEMENTS
#define INCLUDE_WNDELEMENTS

class WndElem {
public:
	DWORD ctlid;
	HWND hWnd;
	RECT initrect;
	WndElem *next;

	WndElem(DWORD i, HWND w, LPRECT r);
	~WndElem();
	VOID add(WndElem *e);
	WndElem *get(DWORD ctlid);
};

class WndElems {
private:
	HWND hParent;

public:
	WndElem *elems;

	WndElems(HWND hPar);
	~WndElems();
	HWND add(DWORD ctlid);
	WndElem *get(DWORD ctlid);
	BOOL show(DWORD ctlid, INT nShow);
	LRESULT sendMsg(DWORD ctlid, UINT Msg, WPARAM wParam, LPARAM lParam);
	BOOL enable(DWORD ctlid, BOOL bEnable);
	HWND getHandle(DWORD ctlid);
};

#endif