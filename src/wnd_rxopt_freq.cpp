/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *        Option sub-dialog for RX frequency configuration         *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowFrequencyRxDialog():   *
 * - updates cfg-><frequencies | hop_time>                         *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include <Commctrl.h>

#include "librtl_433.h"
#include "wnd_rxopt_freq.h"

#include "../res/gui_win_resources.h"

static rtl_433_t	*rtl = NULL;
static int tmp_hoptime;

static HMENU hPopup;
static HWND hDlg, hFreqList, hHopCap, hHopEdt, hHopBtn;

static void freqList_init() {
	LVCOLUMN tmp_lvcolumn;
	tmp_lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	tmp_lvcolumn.pszText = "Frequency";
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.cx = OPT_FREQ_COLWIDTH;
	ListView_InsertColumn(hFreqList, 0, &tmp_lvcolumn);
}

static void RefreshWndElements() {
	int num_entries = ListView_GetItemCount(hFreqList);

	// caption:
	char caption[40];
	sprintf_s(caption, sizeof(caption), "Selected frequencies (%lu):", num_entries);
	SetDlgItemText(hDlg, IDC_RX_FREQ_CAPTION, caption);

	sprintf_s(caption, sizeof(caption), "Hopping interval: %lus.", tmp_hoptime);
	SetDlgItemText(hDlg, IDC_RX_FREQ_HOPCAP, caption);

	// Hopping settings
	EnableWindow(hHopCap, num_entries > 1);
	EnableWindow(hHopEdt, num_entries > 1);
	EnableWindow(hHopBtn, num_entries > 1);
}

static void freqList_insertFreq(unsigned long freq) {
	int num_entries = ListView_GetItemCount(hFreqList);
	int trg_idx = 0;

	for (; trg_idx < num_entries; trg_idx++) {
		LVITEM tmp_lvitem;
		tmp_lvitem.iItem = trg_idx;
		tmp_lvitem.iSubItem = 0;
		tmp_lvitem.lParam = 0;
		tmp_lvitem.mask = LVIF_PARAM;
		ListView_GetItem(hFreqList, &tmp_lvitem);
		if (freq == tmp_lvitem.lParam) return;
		if (freq < tmp_lvitem.lParam) break;
	}

	char nice_freq[20];
	if (freq >= 1E9)		snprintf(nice_freq, sizeof(nice_freq), "%.3fGHz", (double)freq / 1E9);
	else if (freq >= 1E6)	snprintf(nice_freq, sizeof(nice_freq), "%.3fMHz", (double)freq / 1E6);
	else if (freq >= 1E3)	snprintf(nice_freq, sizeof(nice_freq), "%.3fkHz", (double)freq / 1E3);
	else					snprintf(nice_freq, sizeof(nice_freq), "%f", (double)freq);
	char freqstr[150];
	sprintf_s(freqstr, sizeof(freqstr), "%lu (%s)", freq, nice_freq);

	LVITEM tmp_lvitem;
	tmp_lvitem.iItem = trg_idx;
	tmp_lvitem.iSubItem = 0;
	tmp_lvitem.pszText = freqstr;
	tmp_lvitem.mask = LVIF_TEXT | LVIF_PARAM;
	tmp_lvitem.lParam = freq;
	ListView_InsertItem(hFreqList, &tmp_lvitem);
}

static void saveToCfg() {
	int num_entries = min(ListView_GetItemCount(hFreqList), MAX_FREQS);

	for (int a = 0; a < num_entries; a++) {
		LVITEM tmp_lvitem;
		tmp_lvitem.iItem = a;
		tmp_lvitem.iSubItem = 0;
		tmp_lvitem.lParam = 0;
		tmp_lvitem.mask = LVIF_PARAM;
		ListView_GetItem(hFreqList, &tmp_lvitem);
		rtl->cfg->frequency[a] = (unsigned long) tmp_lvitem.lParam;
	}
	rtl->cfg->frequencies = num_entries;
	rtl->cfg->hop_time = tmp_hoptime;
}

static int freqList_clear(int a) {
	int n_list_entries = ListView_GetItemCount(hFreqList);
	if (a < 0 || a >= n_list_entries) return 0;
	ListView_DeleteItem(hFreqList, a);
	return 1;
}

static void freqList_clear() {
	ListView_DeleteAllItems(hFreqList);
	//int n_list_entries = ListView_GetItemCount(hFreqList);
	//for (int a = n_list_entries - 1; a >= 0; a--) {
	//	freqList_clear(a);
	//}
}

#define SEL_ALLOC_STEP 20
static int *selected_indices = NULL; // array to temporarily hold the selection state of the list view entries
static int selected_indices_cap = 0;

static BOOL freqList_getSelectedEntries(int **sel_ents, int *n_sel, int *n_tot) {
	*n_tot = ListView_GetItemCount(hFreqList);
	if (!selected_indices_cap || *n_tot > selected_indices_cap) {
		selected_indices_cap = SEL_ALLOC_STEP * ((*n_tot / SEL_ALLOC_STEP) + 1);
		selected_indices = (int*)realloc(selected_indices, (selected_indices_cap * sizeof(int)));
	}
	if (!selected_indices) return FALSE;

	INT n_stored = 0;
	for (int a = 0; a < *n_tot; a++) {
		if (ListView_GetItemState(hFreqList, a, LVIS_SELECTED) == LVIS_SELECTED) {
			selected_indices[n_stored++] = a;
		}
	}

	*n_sel = n_stored;
	*sel_ents = selected_indices;
	return TRUE;
}

static BOOL onKey(LPNMLVKEYDOWN ctx) {
	if (!ctx) return FALSE;
	if (ctx->hdr.hwndFrom == hFreqList) {
		if (ctx->wVKey == VK_DELETE) {
			int *sel_ents = NULL;
			int n_sel;
			int n_tot;
			if (freqList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
				for (int a = n_sel - 1; a >= 0; a--) {
					freqList_clear(sel_ents[a]);
				}
				RefreshWndElements();
			}
			return TRUE;
		}
	}
	return FALSE;
}

static BOOL onRightClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	if (ctx_in->hwndFrom == hFreqList) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;

		int *sel_ents = NULL;
		int n_sel;
		int n_tot;
		if (freqList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
			//INT clkidx = ctx->iItem;
			EnableMenuItem(hPopup, ID_FRQLIST_DELSEL, MF_BYCOMMAND | (n_sel > 0 ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(hPopup, ID_FRQLIST_DELALL, MF_BYCOMMAND | (n_tot > 0 ? MF_ENABLED : MF_GRAYED));

			POINT pt = ctx->ptAction;
			RECT list_pos;
			GetWindowRect(hFreqList, &list_pos);
			DWORD cmd = TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, list_pos.left + pt.x, list_pos.top + pt.y, 0, hDlg, 0);
			if (cmd == ID_FRQLIST_DELSEL) {
				for (int a = n_sel - 1; a >= 0; a--) {
					freqList_clear(sel_ents[a]);
				}
				RefreshWndElements();
			}
			else if (cmd == ID_FRQLIST_DELALL) {
				freqList_clear();
				RefreshWndElements();
			}

		}
		return TRUE;
	}
	return FALSE;
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_RX_FREQ_ADD: {
		BOOL trans = FALSE;
		unsigned long freq = GetDlgItemInt(hDlg, IDC_RX_FREQ_EDIT, &trans, FALSE);
		if (trans) {
			freqList_insertFreq(freq);
			RefreshWndElements();
		}
		break;
	}

	case IDC_RX_FREQ_CLEAR:
		freqList_clear();
		RefreshWndElements();
		break;

	case IDC_RX_FREQ_HOPBTN: {
		BOOL trans = FALSE;
		unsigned long ht = GetDlgItemInt(hDlg, IDC_RX_FREQ_HOPEDT, &trans, FALSE);
		if (trans && ht) tmp_hoptime = ht;
		RefreshWndElements();
		break;
	}

	case IDC_RX_FREQ_OK:
		saveToCfg();
		EndDialog(hDlg, TRUE);
		break;

	case IDC_RX_FREQ_CANCEL:
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

	hFreqList = GetDlgItem(hDlg, IDC_RX_FREQ_LIST);

	hHopCap = GetDlgItem(hDlg, IDC_RX_FREQ_HOPCAP);
	hHopEdt = GetDlgItem(hDlg, IDC_RX_FREQ_HOPEDT);
	hHopBtn = GetDlgItem(hDlg, IDC_RX_FREQ_HOPBTN);

	freqList_init();
	for (int a = 0; a <rtl->cfg->frequencies; a++) {
		freqList_insertFreq(rtl->cfg->frequency[a]);
	}

	RefreshWndElements();
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

int ShowFrequencyRxDialog(rtl_433_t *rtl_obj,HWND hParent, HMENU hPopupMenu) {
	hPopup = hPopupMenu;
	int r = -1;
	if (rtl_obj) {
		rtl = rtl_obj;
		if (!rtl->cfg->frequencies) {
			rtl->cfg->frequencies = 1;
			rtl->cfg->frequency[0] = DEFAULT_FREQUENCY;
		}
		tmp_hoptime = rtl->cfg->hop_time;
		if (!tmp_hoptime) tmp_hoptime = DEFAULT_HOP_TIME;
		r = (int) DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_RX_FREQ), hParent, (DLGPROC)DialogHandler);
	}
	return r;
}
