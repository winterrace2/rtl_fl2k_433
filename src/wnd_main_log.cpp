/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                 Main window element: System Log                 *
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
#include "gui_win_resources.h"

#include "wnd_main_log.h"

loglist::loglist(HWND hLogWnd, HWND hParent, HMENU popup_menu){
	this->hList = hLogWnd;
	this->hParWnd = hParent;
	this->hPopup = popup_menu;

	// Spalten initialisieren:
	LVCOLUMN tmp_lvcolumn;
	tmp_lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	tmp_lvcolumn.pszText = "Log time";
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.cx = LOG_COLUMN_TIME_WIDTH;
	ListView_InsertColumn(hList, 0, &tmp_lvcolumn);
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.pszText = "Module";
	tmp_lvcolumn.cx = LOG_COLUMN_MOD_WIDTH;
	ListView_InsertColumn(hList, 1, &tmp_lvcolumn);
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.pszText = "Log text";
	tmp_lvcolumn.cx = LOG_COLUMN_TEXT_WIDTH;
	ListView_InsertColumn(hList, 2, &tmp_lvcolumn);
	Clear();
}

loglist::~loglist(){
	Clear();
}

HWND loglist::getWndHandle() {
	return this->hList;
}

VOID loglist::Clear(){
	ListView_DeleteAllItems(hList);
	crntline_len = 0;
	memset(crntline, 0, sizeof(crntline)); // just for sake of a nice initialization. Should not be necessary here
}

BOOL loglist::SaveLog(){
	char fpath[MAX_PATH]="";
	char line[LOG_MAX_LINE_CHARS+10];
	DWORD bytes_written;

	// 1.) Dateipfad zum Speichern abfragen
	OPENFILENAME ofntemp = {sizeof(OPENFILENAME), hList, 0, "Text files (*.txt)\0*.txt\0\0", 0, 0, 0, (char*) fpath, sizeof(fpath), 0,0,0, "Save file as:", OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_READONLY | OFN_HIDEREADONLY, 0, 0, "txt", 0, 0, 0};
	if(GetSaveFileName(&ofntemp) == 0) return FALSE;

	// 2.) Log zeilenweise in Logdatei speichern
	HANDLE fhandle = CreateFile (fpath,GENERIC_READ + GENERIC_WRITE ,0,0,CREATE_ALWAYS,0,0);
	if(fhandle == INVALID_HANDLE_VALUE) return FALSE;

	int lines = ListView_GetItemCount(hList);
	for(int a = 0; a < lines; a++){
		memset(line,0,sizeof(line));
		ListView_GetItemText(hList, a,0, line, sizeof(line)-1);
		// Print date (if there)
		if(strlen(line)>0){
			WriteFile(fhandle, "==========", 10, &bytes_written, 0);
			WriteFile(fhandle, line, (DWORD) strlen(line), &bytes_written, 0);
			WriteFile(fhandle, "==========\r\n", 12, &bytes_written, 0);
			memset(line,0,sizeof(line));
		}
		// Print module
		ListView_GetItemText(hList, a, 1, line, sizeof(line) - 1);
		if (strlen(line) > 0) {
			WriteFile(fhandle, "[", 1, &bytes_written, 0);
			WriteFile(fhandle, line, (DWORD)strlen(line), &bytes_written, 0);
			WriteFile(fhandle, "] ", 2, &bytes_written, 0);
		}
		// Print log text
		ListView_GetItemText(hList, a,2, line, sizeof(line)-1);
		WriteFile(fhandle, line, (DWORD) strlen(line), &bytes_written, 0);
		WriteFile(fhandle, "\r\n", 2, &bytes_written, 0);
	}

	CloseHandle(fhandle);
	return TRUE;
}

VOID loglist::toString(INT id, CHAR *buf, INT cap) {
	buf[0] = 0;
	int buf_l = 0;
	int buf_c = cap - 1;

	INT start_entry = (id >= 0 ? id : 0);
	INT num_entries = (id >= 0 ? 1 : ListView_GetItemCount(this->hList));

	for (int idx = start_entry; idx < (start_entry + num_entries) && ((buf_c - buf_l)>3); idx++) {
		LVITEM tmp_lvitem;
		tmp_lvitem.iSubItem = 0;
		tmp_lvitem.iItem = idx;
		tmp_lvitem.pszText = &buf[buf_l];
		tmp_lvitem.cchTextMax = buf_c - buf_l;
		tmp_lvitem.mask = LVIF_TEXT;
		INT read = (INT)SendMessage(this->hList, LVM_GETITEMTEXT, idx, (LPARAM)&tmp_lvitem);
		buf_l += read;
		if ((buf_c - buf_l) >= 2)
			buf_l += sprintf_s(&buf[buf_l], buf_c - buf_l, "\t");
		tmp_lvitem.iSubItem = 1;
		tmp_lvitem.pszText = &buf[buf_l];
		tmp_lvitem.cchTextMax = buf_c - buf_l;
		read = (INT)SendMessage(this->hList, LVM_GETITEMTEXT, idx, (LPARAM)&tmp_lvitem);
		buf_l += read;
		if ((buf_c - buf_l) >= 2)
			buf_l += sprintf_s(&buf[buf_l], buf_c - buf_l, "\t");
		tmp_lvitem.iSubItem = 2;
		tmp_lvitem.pszText = &buf[buf_l];
		tmp_lvitem.cchTextMax = buf_c - buf_l;
		read = (INT)SendMessage(this->hList, LVM_GETITEMTEXT, idx, (LPARAM)&tmp_lvitem);
		buf_l += read;
		if ((buf_c - buf_l) >= 3)
			buf_l += sprintf_s(&buf[buf_l], buf_c - buf_l, "\r\n");
	}
}


VOID loglist::AddLogEntry(LPSTR module, INT trg, LPSTR str, BOOL notime){
	// info: log target (trg, i.e. stdout/stderr) is not displayed yet

	// 1) create time stamp
	char timestr[20];
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf_s(timestr, sizeof(timestr), "%02lu:%02lu:%02lu.%03lu", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	LPSTR time = (notime ? "" : timestr);

	// 2) parse string (extract single lines) and output line terminations
	LPSTR line_beg = str;
	while (line_beg && line_beg[0]) {
		int breakwidth = 0;
		int e;
		for (e=0; line_beg[e] && (crntline_len < LOG_MAX_LINE_CHARS); e++) {
			if (line_beg[e] == '\r' && line_beg[e + 1] == '\n') {	// \r\n closes the line
				breakwidth = 2; 
				break;
			}
			else if (line_beg[e] == '\n') {							// \n closes the line
				breakwidth = 1;						
				break;
			}
			else crntline[crntline_len++] = line_beg[e];			// Copy character to line buffer
		}

		BOOL do_output = (breakwidth > 0 || crntline_len >= LOG_MAX_LINE_CHARS); // Plan to output the current line if it is confirmed (\n) or full

		// If a full line is to be printed which got interrupted within a word, try to shorten the line until the beginning of this word
		if (do_output && !breakwidth && IsCharAlpha(line_beg[e]) && IsCharAlpha(line_beg[e-1])) {
			int shorten = 0;
			for (int a = 1; a <= LOG_MAX_LOOKBACKCHARS; a++) {
				if (!IsCharAlpha(line_beg[e-(1+a)])) {
					shorten = a;
					break;
				}
			}
			if (shorten) {
				crntline_len -= shorten;
				e -= shorten;
			}
		}

		// If crntline is confirmed or full, add to ListView
		if (do_output) {
			crntline[crntline_len] = 0; // terminate line
			int idx = ListView_GetItemCount(hList);
			LVITEM tmp_lvitem;
			tmp_lvitem.iSubItem = 0;
			tmp_lvitem.iItem = idx;
			tmp_lvitem.pszText = time;
			tmp_lvitem.mask = LVIF_TEXT;
			ListView_InsertItem(hList, &tmp_lvitem);
			ListView_SetItemText(hList, idx, 1, module);
			ListView_SetItemText(hList, idx, 2, crntline);
			crntline_len = 0;
		} // else: if crntline is unconfirmed but no more input data there, we're ready for now
		
		// Für nächsten Durchlauf vorbereiten
		line_beg = &line_beg[e+breakwidth];
		time = ""; // leave timestamp cell unfilled if new line is due to a line feed or exceeded max line length
	}

	// 3) scroll down in log:
	ListView_EnsureVisible(hList, ListView_GetItemCount(hList)-1, FALSE);
}

DWORD loglist::getRightClickCommand(LPNMITEMACTIVATE dat, INT *selidx) {
	DWORD cmd = 0;
	if (dat && selidx) {
		*selidx = dat->iItem;
		POINT pt = dat->ptAction;
		INT num_entries = ListView_GetItemCount(this->hList);
		BOOL objsel = (*selidx >= 0 && *selidx < num_entries);
		EnableMenuItem(this->hPopup, ID_RDLIST_COPY, MF_BYCOMMAND | (objsel ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(this->hPopup, ID_RDLIST_COPYALL, MF_BYCOMMAND | (num_entries ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(this->hPopup, ID_RDLIST_SAVEALL, MF_BYCOMMAND | (num_entries ? MF_ENABLED : MF_GRAYED));

		RECT list_pos;
		GetWindowRect(this->hList, &list_pos);
		cmd = TrackPopupMenu(this->hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, list_pos.left + pt.x, list_pos.top + pt.y, 0, this->hParWnd, 0);
	}
	return cmd;
}

