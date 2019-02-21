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

#include <windows.h>
#include "wndelements.h"

static VOID GetChildPos(HWND hChild, HWND hParent, RECT *r) {
	POINT p0 = { 0,0 };

	GetWindowRect(hChild, r);
	ClientToScreen(hParent, &p0);
	r->left -= p0.x;
	r->top -= p0.y;
	r->right -= p0.x;
	r->bottom -= p0.y;
}

WndElem::WndElem(DWORD i, HWND w, LPRECT r) {
	this->ctlid = i;
	this->hWnd = w;
	this->initrect = *r;
	this->next = NULL;
}

WndElem::~WndElem() {
	if (this->next) delete this->next;
}

VOID WndElem::add(WndElem *e) {
	if (this->next) this->next->add(e);
	else this->next = e;
}

WndElem *WndElem::get(DWORD cid) {
	if (this->ctlid == cid) {
		return this;
	}
	else if(this->next){
		return next->get(cid);
	}
	else return NULL;
}

WndElems::WndElems(HWND hPar) {
	this->hParent = hPar;
	this->elems = NULL;
}

HWND WndElems::add(DWORD ctlid) {
	HWND h = GetDlgItem(this->hParent, ctlid);
	RECT r;
	GetChildPos(h, this->hParent, &r);
	WndElem *e = new WndElem(ctlid, h, &r);
	if (this->elems) this->elems->add(e);
	else this->elems = e;

	return h;
}

WndElem *WndElems::get(DWORD ctlid) {
	if (this->elems) return elems->get(ctlid);
	else return NULL;
}

WndElems::~WndElems() {
	if (this->elems) delete this->elems;
}

BOOL WndElems::show(DWORD ctlid, int nShow) {
	WndElem *e = this->get(ctlid);
	if (e) {
		return ShowWindow(e->hWnd, nShow);
	}
	return FALSE;
}

LRESULT WndElems::sendMsg(DWORD ctlid, UINT Msg, WPARAM wParam, LPARAM lParam) {
	WndElem *e = this->get(ctlid);
	if (e) {
		return SendMessage(e->hWnd, Msg, wParam, lParam);
	}
	return NULL;
}

BOOL WndElems::enable(DWORD ctlid, BOOL bEnable) {
	WndElem *e = this->get(ctlid);
	if (e) {
		return EnableWindow(e->hWnd, bEnable);
	}
	return NULL;
}

HWND WndElems::getHandle(DWORD ctlid) {
	WndElem *e = this->get(ctlid);
	if (e) return e->hWnd;
	else return NULL;
}
