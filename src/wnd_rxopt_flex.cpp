/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *       Option sub-dialog for flex protocol configuration         *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowFlexDialog():          *
 * - updates cfg->flex_specs                                       *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "librtl_433.h"
#include "wnd_rxopt_flex.h"
#include "wnd_textreader.h"

#include "../res/gui_win_resources.h"


static rtl_433_t *rtl = NULL;
static HWND		hDlg, hFlexList;
static HMENU hPopup;

static void flexList_addElement(LPSTR spec_src) {
	if (spec_src) {
		char *spec = strdup(spec_src);
		LVITEM tmp_lvitem;
		tmp_lvitem.iItem = ListView_GetItemCount(hFlexList);
		tmp_lvitem.iSubItem = 0;
		tmp_lvitem.pszText = spec;
		tmp_lvitem.lParam = (LPARAM)spec;
		tmp_lvitem.mask = LVIF_TEXT | LVIF_PARAM;
		ListView_InsertItem(hFlexList, &tmp_lvitem);
	}
}

static void flexList_init() {
	// create columns
	LVCOLUMN tmp_lvcolumn;
	tmp_lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	tmp_lvcolumn.pszText = "Flex spec";
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.cx = OPT_FLEX_SPEC_WIDTH;
	ListView_InsertColumn(hFlexList, 0, &tmp_lvcolumn);

	//fill from config
	for (int a = 0; a < rtl->cfg->flex_specs.len; a++) {
		flexList_addElement((char*)rtl->cfg->flex_specs.elems[a]);
	}

}
static char *flexList_getSpec(int a) {
	int n_list_entries = ListView_GetItemCount(hFlexList);
	if (a < 0 || a >= n_list_entries) return NULL;

	LVITEM tmp_lvitem;
	tmp_lvitem.iItem = a;
	tmp_lvitem.iSubItem = 0;
	tmp_lvitem.lParam = NULL;
	tmp_lvitem.mask = LVIF_PARAM;
	ListView_GetItem(hFlexList, &tmp_lvitem);

	return (char*)tmp_lvitem.lParam;
}

static int flexList_editSpec(int a, char *newspec) {
	int n_list_entries = ListView_GetItemCount(hFlexList);
	if (a < 0 || a >= n_list_entries) return 0;

	LPSTR spec = strdup(newspec);
	LPSTR oldspec = flexList_getSpec(a);
	
	LVITEM tmp_lvitem;
	tmp_lvitem.iItem = a;
	tmp_lvitem.iSubItem = 0;
	tmp_lvitem.pszText = spec;
	tmp_lvitem.lParam = (LPARAM)spec;
	tmp_lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	ListView_SetItem(hFlexList, &tmp_lvitem);
	
	if (oldspec) free(oldspec);
	return 1;
}

static int flexList_clear(int a) {
	int n_list_entries = ListView_GetItemCount(hFlexList);
	if (a < 0 || a >= n_list_entries) return 0;
	LPSTR spec = flexList_getSpec(a);
	ListView_DeleteItem(hFlexList, a);
	if (spec) free(spec);
	return 1;
}

static void flexList_clear() {
	int n_list_entries = ListView_GetItemCount(hFlexList);
	for (int a = n_list_entries - 1; a >= 0; a--) {
		flexList_clear(a);
	}
}

#define SEL_ALLOC_STEP 20
static int *selected_indices = NULL; // array to temporarily hold the selection state of the list view entries
static int selected_indices_cap = 0;

static BOOL flexList_getSelectedEntries(int **sel_ents, int *n_sel, int *n_tot) {
	*n_tot = ListView_GetItemCount(hFlexList);
	if (!selected_indices_cap || *n_tot > selected_indices_cap) {
		selected_indices_cap = SEL_ALLOC_STEP * ((*n_tot / SEL_ALLOC_STEP) + 1);
		selected_indices = (int*)realloc(selected_indices, (selected_indices_cap * sizeof(int)));
	}
	if (!selected_indices) return FALSE;

	INT n_stored = 0;
	for (int a = 0; a < *n_tot; a++) {
		if (ListView_GetItemState(hFlexList, a, LVIS_SELECTED) == LVIS_SELECTED) {
			selected_indices[n_stored++] = a;
		}
	}

	*n_sel = n_stored;
	*sel_ents = selected_indices;
	return TRUE;
}

static void updateFlexGuiElements() {
	int n_list_entries = ListView_GetItemCount(hFlexList);

	char caption[40];
	sprintf_s(caption, sizeof(caption), "Active flex protocols (%lu):", n_list_entries);
	SetDlgItemText(hDlg, IDC_RX_FLEX_CAPTION, caption);
}

static VOID EditFlexEntry(int idx) {
	LPSTR spec = flexList_getSpec(idx);
	LPSTR edited = ShowTextReaderDialog(hDlg, "Flex protocol specification", spec, 2000);
	if (edited) {
		if (strlen(edited) > 0) flexList_editSpec(idx, edited);
		else {
			if (MessageBox(hDlg, "Clearing the spec means deleting it. Are you sure?", "Attention:", MB_OKCANCEL | MB_ICONINFORMATION) == IDOK) {
				flexList_clear(idx);
				updateFlexGuiElements();
			}
		}
	}
}

static BOOL onKey(LPNMLVKEYDOWN ctx) {
	if (!ctx) return FALSE;
	if (ctx->hdr.hwndFrom == hFlexList) {
		if (ctx->wVKey == VK_DELETE) {
			int *sel_ents = NULL;
			int n_sel;
			int n_tot;
			if (flexList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
				for (int a = n_sel - 1; a >= 0; a--) {
					flexList_clear(sel_ents[a]);
				}
				updateFlexGuiElements();
			}
			return TRUE;
		}
		else if (ctx->wVKey == VK_SPACE) {
			int *sel_ents = NULL;
			int n_sel;
			int n_tot;
			if (flexList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
				if (n_sel == 1) EditFlexEntry(sel_ents[0]);
			}
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL onRightClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	if (ctx_in->hwndFrom == hFlexList) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;

		int *sel_ents = NULL;
		int n_sel;
		int n_tot;
		if (flexList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
			EnableMenuItem(hPopup, ID_FLXLIST_EDIT, MF_BYCOMMAND | (n_sel == 1 ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(hPopup, ID_FLXLIST_DELSEL, MF_BYCOMMAND | (n_sel > 0 ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(hPopup, ID_FLXLIST_DELALL, MF_BYCOMMAND | (n_tot > 0 ? MF_ENABLED : MF_GRAYED));

			POINT pt = ctx->ptAction;
			RECT list_pos;
			GetWindowRect(hFlexList, &list_pos);
			DWORD cmd = TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, list_pos.left + pt.x, list_pos.top + pt.y, 0, hDlg, 0);
			if (cmd == ID_FLXLIST_EDIT) {
				EditFlexEntry(sel_ents[0]);
			}
			else if (cmd == ID_FLXLIST_DELSEL) {
				for (int a = n_sel - 1; a >= 0; a--) {
					flexList_clear(sel_ents[a]);
				}
				updateFlexGuiElements();
			}
			else if (cmd == ID_FLXLIST_DELALL) {
				flexList_clear();
				updateFlexGuiElements();
			}
		}
		return TRUE;
	}
	return FALSE;
}

static BOOL onDoubleClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	if (ctx_in->hwndFrom == hFlexList) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;


		int idx_clicked = ctx->iItem;
		int *sel_ents = NULL;
		int n_sel;
		int n_tot;
		if (flexList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
			if (n_sel == 1 && sel_ents[0] == idx_clicked) {
				EditFlexEntry(sel_ents[0]);
			}
		}
		return TRUE;
	}
	return FALSE;
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_RX_FLEX_ADD: {
		LPSTR spec = ShowTextReaderDialog(hDlg, "Flex protocol specification", NULL, 2000);
		if (spec) flexList_addElement(spec);
		updateFlexGuiElements();
		break;
	}

	case IDC_RX_FLEX_CLEAR:
		flexList_clear();
		updateFlexGuiElements();
		break;

	case IDC_RX_FLEX_OK: {
		int n_list_entries = ListView_GetItemCount(hFlexList); // current number of list entries
		list_clear(&rtl->cfg->flex_specs, free);
		list_ensure_size(&rtl->cfg->flex_specs, n_list_entries);
		for (int a = 0; a < n_list_entries; a++) {
			char *spec = flexList_getSpec(a);
			list_push(&rtl->cfg->flex_specs, strdup(spec));
		}
		EndDialog(hDlg, TRUE);
		break;
	}

	case IDC_RX_FLEX_CANCEL:
		EndDialog(hDlg, TRUE);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static VOID onInit(HWND hwndDlg) {
	hDlg = hwndDlg;
	HICON hIcon = LoadIcon(GetModuleHandle(0), (const char *)IDI_ICON1);
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	hFlexList = GetDlgItem(hDlg, IDC_RX_FLEX_LIST);
	flexList_init();
	updateFlexGuiElements();
}

static INT_PTR CALLBACK DialogHandler(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

	case WM_NOTIFY: {
		switch (((LPNMHDR)lParam)->code) {

		case LVN_KEYDOWN:
			onKey((LPNMLVKEYDOWN) lParam);
			break;

		case NM_RCLICK:
			onRightClick((LPNMHDR) lParam);
			break;

		case NM_DBLCLK:
			onDoubleClick((LPNMHDR)lParam);
			break;

		default:
			break;
		}
		break;
	}
	
	case WM_COMMAND:
		onCommand(wParam, lParam);
		break;

	case WM_INITDIALOG:
		onInit(hwndDlg);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, TRUE);
		break;

	default:
		return false;
	}
	return true;
}

int ShowFlexDialog(rtl_433_t *rtl_obj, HWND hParent, HMENU hPopupMenu) {
	hPopup = hPopupMenu;
	int r = -1;
	if (rtl_obj) {
		rtl = rtl_obj;
		r = (int)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_RX_FLEX), hParent, (DLGPROC)DialogHandler);
	}
	return r;
}
