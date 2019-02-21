/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *    Main window element: ListView for outgoing (TX) messages     *
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
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include "wnd_main_txlist.h"
#include "gui_win_resources.h"

static INT ListView_GetSelectedItem(HWND hList) {
	if (ListView_GetSelectedCount(hList) != 1) return -2;
	int total_items = ListView_GetItemCount(hList);
	for (int a = 0; a < total_items; a++) {
		if (ListView_GetItemState(hList, a, LVIS_SELECTED) == LVIS_SELECTED) return a;
	}
	return -1;
}

txlist::txlist(HWND hLogWnd, HWND hParent, HMENU popup_menu){
	this->hList = hLogWnd;
	this->hParWnd = hParent;
	this->hPopup = popup_menu;

	// Spalten initialisieren:
	LVCOLUMN tmp_lvcolumn;
	tmp_lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	tmp_lvcolumn.pszText = "description";
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.cx = TX_COLUMN_DSCR_WIDTH;
	ListView_InsertColumn(this->hList, 0, &tmp_lvcolumn);
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.pszText = "time sent";
	tmp_lvcolumn.cx = TX_COLUMN_SENT_WIDTH;
	ListView_InsertColumn(this->hList, 1, &tmp_lvcolumn);
	Clear();
}

txlist::~txlist(){
	Clear();
}

static COLORREF red = RGB(255, 160, 160);
static COLORREF white = RGB(255, 255, 255);

VOID txlist::setActiveStyle(BOOL active) {
	ListView_SetBkColor(this->hList, (active ? red : white));
	ListView_SetTextBkColor(this->hList, (active ? red : white));
	RedrawWindow(this->hList, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

VOID txlist::Clear(){
	int cnt = this->CountAllEntries();
	while (cnt > 0) {
		this->RemoveTxEntry(cnt-1);
		cnt--;
	}
}

HWND txlist::getWndHandle() {
	return this->hList;
}

LPSTR PrintSampleNumber(uint32_t size, char *trg, int cap) {
	if (size < 1024) {
		sprintf_s(trg, cap, "%lu samples", size);
		return trg;
	}
	else if (size < (1024 * 1024)) {
		sprintf_s(trg, cap, "%.3f kS", (double)size/1024.0);
		return trg;
	}
	else{
		sprintf_s(trg, cap, "%.3f MS", (double)size / (1024.0 * 1024.0));
		return trg;
	}
}

VOID txlist::AddTxEntry(tx_entry *entry){
	if (entry) {
		// Add line
		int idx = this->CountAllEntries();
		BOOL autoscroll = ListView_IsItemVisible(this->hList, idx - 1); // if the last item was visible before, we activate autoscrolling
		LVITEM tmp_lvitem;
		tmp_lvitem.iSubItem = 0;
		tmp_lvitem.iItem = idx;
		tmp_lvitem.pszText = entry->getDescription();
		tmp_lvitem.mask = LVIF_TEXT | LVIF_PARAM;
		tmp_lvitem.lParam = (LPARAM) entry;
		ListView_InsertItem(this->hList, &tmp_lvitem);
		mod_type sm = entry->getMessage()->mod;
		LPSTR time = entry->getSentTime();
		ListView_SetItemText(this->hList, idx, 1, (time?time:"not yet"));
		if (autoscroll) ListView_EnsureVisible(this->hList, idx, FALSE); // conditional autoscrolling (ensure line is visible)
	}
}

VOID txlist::refreshLine(int obj_idx){
	tx_entry *e = this->GetTxEntry(obj_idx);
	if (!e) return;
	ListView_SetItemText(this->hList, obj_idx, 1, e->getDescription());
	LPSTR time = e->getSentTime();
	ListView_SetItemText(this->hList, obj_idx, 1, (time?time:"not yet"));
}

char *txlist::getStrView(int obj_idx, char *buf, int cap) {
	tx_entry *e = this->GetTxEntry(obj_idx);
	if (!e) return "";

	TxMsg *msg = e->getMessage();
	if (msg) {
		char tmp1[20];
		mod_type sm = msg->mod;
		LPSTR dscr = e->getDescription();
		LPSTR mod = (sm == MODULATION_TYPE_OOK ? "OOK" : (sm == MODULATION_TYPE_FSK ? "FSK" : (sm == MODULATION_TYPE_SINE ? "sine" : "n / a")));
		LPSTR len = PrintSampleNumber(msg->len, tmp1, sizeof(tmp1));
		double ms = (double) msg->len * 1000.0 / (double) msg->samp_rate;
		LPSTR time = e->getSentTime();
		sprintf_s(buf, cap, "%s\r\nModulation: %s\r\nLength: %s @ %lu S/s (=> %.02f ms)\r\nSent: %s", dscr, mod, len, msg->samp_rate, ms, (time ? time : "not yet"));
	}
	else {
		sprintf_s(buf, cap, "Internal error (TX msg pointer is NULL) ");
	}

	return buf;
}

INT txlist::CountAllEntries() {
	return ListView_GetItemCount(this->hList);
}

INT txlist::CountUnsentEntries() {
	INT total = this->CountAllEntries();
	INT unsent = 0;
	for (int a = 0; a < total; a++) {
		if (!this->hasEntryBeenSent(a)) unsent++;
	}
	return unsent;
}

tx_entry *txlist::GetFirstUnsentEntry(INT *obj_idx) {
	INT total = this->CountAllEntries();
	for (int a = 0; a < total; a++) {
		if (!this->hasEntryBeenSent(a)) {
			if (obj_idx) *obj_idx = a;
			return this->GetTxEntry(a);
		}
	}
	return NULL;
}

INT txlist::GetSelectedItem() {
	return ListView_GetSelectedItem(this->hList);
}

tx_entry *txlist::GetTxEntry(INT obj_idx) {
	tx_entry *result = NULL;
	if (obj_idx >= 0) {
		int cnt = this->CountAllEntries();
		if (obj_idx < cnt) {
			LVITEM tmp_lvitem;
			tmp_lvitem.iSubItem = 0;
			tmp_lvitem.iItem = obj_idx;
			tmp_lvitem.mask = LVIF_PARAM;
			tmp_lvitem.lParam = NULL;
			ListView_GetItem(this->hList, &tmp_lvitem);
			if (tmp_lvitem.lParam) {
				result = (tx_entry*)tmp_lvitem.lParam;
			}
		}
	}
	return result;
}

INT txlist::RemoveTxEntry(int obj_idx) {
	tx_entry *entry = this->GetTxEntry(obj_idx);
	if (entry) {
		delete entry;
		ListView_DeleteItem(this->hList, obj_idx);
		return 1;
	}
	return 0;
}

BOOL txlist::hasEntryBeenSent(int obj_idx) {
	tx_entry *etr = this->GetTxEntry(obj_idx);
	return (etr && etr->getSentTime() ? TRUE : FALSE);
}

DWORD txlist::getRightClickCommand(LPNMITEMACTIVATE dat, INT *selidx) {
	DWORD cmd = 0;
	if (dat && selidx) {
		*selidx = dat->iItem;
		POINT pt = dat->ptAction;
		INT cnt = this->CountAllEntries();
		BOOL objsel = (*selidx >= 0 && *selidx < cnt);
		EnableMenuItem(this->hPopup, ID_TXLIST_DEL,    MF_BYCOMMAND |  (objsel ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(this->hPopup, ID_TXLIST_RESEND, MF_BYCOMMAND | ((objsel && this->hasEntryBeenSent(*selidx)) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(this->hPopup, ID_TXLIST_DELALL, MF_BYCOMMAND | (cnt ? MF_ENABLED : MF_GRAYED));

		RECT list_pos;
		GetWindowRect(this->hList, &list_pos);
		cmd = TrackPopupMenu(this->hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, list_pos.left + pt.x, list_pos.top + pt.y, 0, this->hParWnd, 0);
	}
	return cmd;
}
