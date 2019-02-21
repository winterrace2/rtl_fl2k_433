/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *    Main window element: ListView for incoming (RX) messages     *
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
#include "wnd_main_rxlist.h"
#include "gui_win_resources.h"

static INT ListView_GetSelectedItem(HWND hList) {
	if (ListView_GetSelectedCount(hList) != 1) return -2;
	int total_items = ListView_GetItemCount(hList);
	for (int a = 0; a < total_items; a++) {
		if (ListView_GetItemState(hList, a, LVIS_SELECTED) == LVIS_SELECTED) return a;
	}
	return -1;
}

rxlist::rxlist(HWND hLogWnd, HWND hParent, HMENU popup_menu){
	this->hList = hLogWnd;
	this->hParWnd = hParent;
	this->hPopup = popup_menu;

	// Spalten initialisieren:
	LVCOLUMN tmp_lvcolumn;
	tmp_lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	tmp_lvcolumn.pszText = "time received";
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.cx = RX_COLUMN_TIME_WIDTH;
	ListView_InsertColumn(this->hList, 0, &tmp_lvcolumn);
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.pszText = "mod";
	tmp_lvcolumn.cx = RX_COLUMN_MOD_WIDTH;
	ListView_InsertColumn(this->hList, 1, &tmp_lvcolumn);
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.pszText = "RX message";
	tmp_lvcolumn.cx = RX_COLUMN_TEXT_WIDTH;
	ListView_InsertColumn(this->hList, 2, &tmp_lvcolumn);
	Clear();
}

rxlist::~rxlist(){
	Clear();
}

static COLORREF green = RGB(160, 255, 160);
static COLORREF white = RGB(255, 255, 255);

VOID rxlist::setActiveStyle(BOOL active) {
	ListView_SetBkColor(this->hList, (active ? green : white));
	ListView_SetTextBkColor(this->hList, (active ? green : white));
	RedrawWindow(this->hList, NULL, NULL, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

VOID rxlist::Clear(){
	int cnt = this->GetRxEntryCount();
	while (cnt > 0) {
		this->RemoveRxEntry(cnt-1);
		cnt--;
	}
}

HWND rxlist::getWndHandle() {
	return this->hList;
}

pPulseDatCompact rxlist::getPulses(int obj_idx) {
	rx_entry *entry = this->GetRxEntry(obj_idx);
	if (entry) {
		return entry->getPulses();
	}
	return NULL;
}

data_t *rxlist::getData(int obj_idx) {
	rx_entry *entry = this->GetRxEntry(obj_idx);
	if (entry) {
		return entry->getData();
	}
	return NULL;
}

VOID rxlist::AddRxEntry(rx_entry *entry){
	char crntline[100];

	if (entry) {
		char *time = entry->getTime();
		if (time) {

			char *tp = entry->getType();
			char *md = entry->getModel();
			int id = entry->getDevId();

			if (tp && *tp && md && *md) sprintf_s(crntline, "[%03lu] %s: %s", id, tp, md);
			else if (md && *md) sprintf_s(crntline, "[%03lu] %s", id, md);
			else if (tp && *tp) sprintf_s(crntline, "[%03lu] %s", id, tp);
			else if(id)         sprintf_s(crntline, "[%03lu]", id);
			else                sprintf_s(crntline, "[n/a]");

			// Add line
			int idx = this->GetRxEntryCount();
			BOOL autoscroll = ListView_IsItemVisible(this->hList, idx-1); // if the last item was visible before, we activate autoscrolling
			LVITEM tmp_lvitem;
			tmp_lvitem.iSubItem = 0;
			tmp_lvitem.iItem = idx;
			tmp_lvitem.pszText = time;
			tmp_lvitem.mask = LVIF_TEXT | LVIF_PARAM;
			tmp_lvitem.lParam = (LPARAM) entry;
			ListView_InsertItem(this->hList, &tmp_lvitem);

			sigmod sm = entry->getModulation();
			ListView_SetItemText(this->hList, idx, 1, (sm == SIGNAL_MODULATION_OOK ? "OOK":(sm==SIGNAL_MODULATION_FSK?"FSK":"n/a")));
			ListView_SetItemText(this->hList, idx, 2, crntline);
			
			if (autoscroll) ListView_EnsureVisible(this->hList, idx, FALSE); // conditional autoscrolling (ensure line is visible)
		}
	}
}

INT rxlist::GetRxEntryCount() {
	return ListView_GetItemCount(this->hList);
}

INT rxlist::GetSelectedItem() {
	return ListView_GetSelectedItem(this->hList);
}

rx_entry *rxlist::GetRxEntry(int obj_idx) {
	rx_entry *result = NULL;
	if (obj_idx >= 0) {
		int cnt = this->GetRxEntryCount();
		if (obj_idx < cnt) {
			LVITEM tmp_lvitem;
			tmp_lvitem.iSubItem = 0;
			tmp_lvitem.iItem = obj_idx;
			tmp_lvitem.mask = LVIF_PARAM;
			tmp_lvitem.lParam = NULL;
			ListView_GetItem(this->hList, &tmp_lvitem);
			if (tmp_lvitem.lParam) {
				result = (rx_entry*)tmp_lvitem.lParam;
			}
		}
	}
	return result;
}

INT rxlist::RemoveRxEntry(int obj_idx) {
	rx_entry *entry = this->GetRxEntry(obj_idx);
	if (entry) {
		delete entry;
		ListView_DeleteItem(this->hList, obj_idx);
		return 1;
	}
	return 0;
}

DWORD rxlist::getRightClickCommand(LPNMITEMACTIVATE dat, INT *selidx) {
	DWORD cmd = 0;
	if (dat && selidx) {
		*selidx = dat->iItem;
		POINT pt = dat->ptAction;
		INT cnt = this->GetRxEntryCount();
		BOOL objsel = (*selidx >= 0 && *selidx < cnt);
		BOOL mod_unknown = FALSE;
		if (objsel) {
			rx_entry *e = this->GetRxEntry(*selidx);
			mod_unknown = (e->getModulation() == SIGNAL_MODULATION_UNK);
		}
		EnableMenuItem(this->hPopup, 0, MF_BYPOSITION | (objsel ? MF_ENABLED : MF_DISABLED)); // popup including ID_RXLIST_SIGTX and ID_RXLIST_PLSTX
		EnableMenuItem(this->hPopup, 1, MF_BYPOSITION | (objsel ? MF_ENABLED : MF_DISABLED)); // popup including ID_RXLIST_SAVESIG and ID_RXLIST_SAVEPLS
		EnableMenuItem(this->hPopup, ID_RXLIST_DEL,    MF_BYCOMMAND | (objsel ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(this->hPopup, ID_RXLIST_DELALL, MF_BYCOMMAND | (cnt    ? MF_ENABLED : MF_GRAYED));

		RECT list_pos;
		GetWindowRect(this->hList, &list_pos);
		cmd = TrackPopupMenu(this->hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, list_pos.left + pt.x, list_pos.top + pt.y, 0, this->hParWnd, 0);
	}
	return cmd;
}

