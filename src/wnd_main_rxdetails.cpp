/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *    Implementation of the ListView listing RX signal details     *
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
#include "logwrap.h"
#include "rxlist_entry.h"
#include "wnd_main_rxdetails.h"
#include "gui_win_resources.h"

/* data helper functions */

int print_value(char *trg, int cap, data_type_t type, void *value, char *format);

typedef struct {
	int array_element_size; /* what is the element size when put inside an array? */
	bool array_is_boxed; /* is the element boxed (ie. behind a pointer) when inside an array? */
} data_meta_type_t2;

static data_meta_type_t2 dmt[DATA_COUNT] = {
	{sizeof(data_t*),true}, //  DATA_DATA
	{sizeof(int), false}, //  DATA_INT
	{sizeof(double), false}, //  DATA_DOUBLE
	{sizeof(char*), true}, //  DATA_STRING
	{sizeof(data_array_t*), true}, //  DATA_ARRAY
};

static int print_array_value(char *trg, int cap, data_array_t *array, char *format, int idx)
{
	int element_size = dmt[array->type].array_element_size;
	char *buffer = (char*) malloc(element_size);
	if (!buffer) {
		Gui_fprintf(stderr, "print_array_value: Out of memory.\n");
		return 0;
	}
	int len = 0;

	if (!dmt[array->type].array_is_boxed) {
		memcpy(buffer, (void **)((char *)array->values + element_size * idx), element_size);
		len = print_value(trg, cap, array->type, buffer, format);
	}
	else {
		len = print_value(trg, cap, array->type, *(void **)((char *)array->values + element_size * idx), format);
	}

	free(buffer);
	return len;
}

static int data_convert_array(char *trg, int cap, data_array_t *array, char *format) {
	int len = 0;

	len += max(0, sprintf_s(&trg[len], cap-len, "["));

	for (int c = 0; c < array->num_values; ++c) {
		if (c) len += max(0, sprintf_s(&trg[len], cap-len, ", "));
		len += print_array_value(&trg[len], cap-len, array, format, c);
	}
	len += max(0, sprintf_s(&trg[len], cap-len, "]"));

	return len;
}

int print_value(char *trg, int cap, data_type_t type, void *value, char *format)
{
	if (!trg || cap <= 0) {
		Gui_fprintf(stderr, "print_value: Unexpected parameters (internal error).\n");
		return 0;
	}
	trg[0] = 0;

	switch (type) {
		case DATA_FORMAT:
		case DATA_COUNT:
		case DATA_DATA:
			Gui_fprintf(stderr, "print_value: unexpected data type.\n");
			return 0;
		case DATA_INT:
			return sprintf_s(trg, cap, format ? format : "%d", *(int *)value);
		case DATA_DOUBLE:
			return sprintf_s(trg, cap, format ? format : "%.3f", *(double *)value);
		case DATA_STRING:
			return sprintf_s(trg, cap, format ? format : "%s", (const char*)value);
		case DATA_ARRAY:
			return data_convert_array(trg, cap, (data_array_t *)value, format);
		default:
			Gui_fprintf(stderr, "print_value: unknown type (internal error).\n");
			return 0;
	}
}

static INT ListView_GetSelectedItem(HWND hList) {
	if (ListView_GetSelectedCount(hList) != 1) return -2;
	int total_items = ListView_GetItemCount(hList);
	for (int a = 0; a < total_items; a++) {
		if (ListView_GetItemState(hList, a, LVIS_SELECTED) == LVIS_SELECTED) return a;
	}
	return -1;
}


rxdetails::rxdetails(HWND hWnd, HWND hParent, HMENU popup_menu){
	this->hList = hWnd;
	this->hParWnd = hParent;
	this->hPopup = popup_menu;

	// initialize columns
	LVCOLUMN tmp_lvcolumn;
	tmp_lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	tmp_lvcolumn.pszText = "key";
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.cx = RX_COLUMN_KEY;
	ListView_InsertColumn(this->hList, 0, &tmp_lvcolumn);
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.pszText = "value";
	tmp_lvcolumn.cx = RX_COLUMN_VALUE;
	ListView_InsertColumn(this->hList, 1, &tmp_lvcolumn);
}

rxdetails::~rxdetails(){
	ListView_DeleteAllItems(this->hList);
	// no need to free any data items here since these are not kept by rxdetails (only by rxlist)
}

HWND rxdetails::getWndHandle() {
	return this->hList;
}

static char tmpstr[400];
VOID rxdetails::DisplayData(data_t *data){
	ListView_DeleteAllItems(this->hList);
	data_t *d = data;
	while(d){
		// Add line
		int idx = ListView_GetItemCount(this->hList);
		LVITEM tmp_lvitem;
		tmp_lvitem.iSubItem = 0;
		tmp_lvitem.iItem = idx;
		tmp_lvitem.pszText = (d->pretty_key && d->pretty_key[0] ? d->pretty_key : d->key);
		tmp_lvitem.mask = LVIF_TEXT;
		ListView_InsertItem(this->hList, &tmp_lvitem);
		print_value(tmpstr, sizeof(tmpstr), d->type, d->value, d->format);
		ListView_SetItemText(this->hList, idx, 1, tmpstr);
		d = d->next;
	}
}

DWORD rxdetails::getRightClickCommand(LPNMITEMACTIVATE dat, INT *selidx) {
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

VOID rxdetails::toString(INT id, CHAR *buf, INT cap) {
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
		if ((buf_c - buf_l) >= 3)
			buf_l += sprintf_s(&buf[buf_l], buf_c - buf_l, "\r\n");
	}
}
