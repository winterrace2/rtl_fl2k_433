/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *        Option sub-dialog for radio protocol selection           *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowProtocolDialog():      *
 * - updates cfg->active_prots                                     *
 *                                                                 *
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
#include "wnd_textreader.h"
#include "wnd_rxopt_prot.h"

#include "../res/gui_win_resources.h"

#define NOPAR_MARKER "<unused>" // this value is used in the "Parameter" column if the protocol does not support parameters. Right click handler checks for this content to decide if context menu allows parameter editing...

static rtl_433_t *rtl = NULL;
static HMENU hPopup;
static HWND	hDlg, hProtList;
static int dev_num; // number of r_devices compiled into librtl_433 (including those with disabled == 3)

// A list of states for all protocols (r_device) compiled into librtl_433
typedef struct _protstate {
	BOOL active; // activation flag
	LPSTR param; // parameter string
} protstate, *pprotstate;

static protstate *tmpprots = NULL; // If allocated, this list has dev_num entries, i.e. including those protocols with disabled == 3 (which cannot be activated here)

// pre-defined sets of protocols (that can be loaded via prot_LoadConstellation()) 
typedef enum {
	PROTS_DEFAULTS = 0,
	PROTS_ALL = 1,
	PROTS_NONE = 2
} ProtocolConstellation;

// Gets a protocol id (1-based) from the listview entry param. This is used since the list lacks the hidden protocols and thus has no linear scale.
static INT getProtocolIdFromListIdx(INT listidx) {
	int n_list_entries = ListView_GetItemCount(hProtList);
	if (listidx < 0 || listidx >= n_list_entries) return -1;

	LVITEM tmp_lvitem;
	tmp_lvitem.iSubItem = 0;
	tmp_lvitem.iItem = listidx;
	tmp_lvitem.mask = LVIF_PARAM;
	tmp_lvitem.lParam = NULL;
	ListView_GetItem(hProtList, &tmp_lvitem);
	if (tmp_lvitem.lParam) {
		return (INT) tmp_lvitem.lParam;
	}
	return -1;
}

// loads a pre-defined set of protocols
static VOID prot_LoadConstellation(ProtocolConstellation c) {
	for (int a = 0; a < dev_num; a++) {
		// Default constellations use no params, so remove them
		if (tmpprots[a].param) {
			free(tmpprots[a].param);
			tmpprots[a].param = NULL;
		}
		// Load default activation masks
		r_device *devptr = NULL;
		if (!getDev(a, &devptr) || devptr->disabled >= 2) { //todo: depending of what will rtl_433 be using the value 2 vor, this should be changed to 3
			// todo: warning
			tmpprots[a].active = FALSE;
		}
		else {
			tmpprots[a].active = (c == PROTS_NONE ? FALSE : (c == PROTS_ALL ? TRUE : (devptr->disabled==0)));
		}
	}
}

// Initializes the protocol listview. Only fills the first column. Column 2 and 3 are filled and refreshed by protList_refreshEntry
static VOID protList_init() {
	LVCOLUMN tmp_lvcolumn;
	tmp_lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	tmp_lvcolumn.pszText = "Protocol";
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.cx = OPT_PROT_NAMECOL_WIDTH;
	ListView_InsertColumn(hProtList, 0, &tmp_lvcolumn);
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.pszText = "Parameter";
	tmp_lvcolumn.cx = OPT_PROT_PARAM_WIDTH;
	ListView_InsertColumn(hProtList, 1, &tmp_lvcolumn);
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.pszText = "State";
	tmp_lvcolumn.cx = OPT_PROT_STATECOL_WIDTH;
	ListView_InsertColumn(hProtList, 2, &tmp_lvcolumn);

	for (int a = 0; a < dev_num; a++) {
		char protname[150];

		r_device *devptr = NULL;
		if (!getDev(a, &devptr)) {
			sprintf_s(protname, sizeof(protname), "[%03lu] - error, could not access device information", a);
		}
		else if (devptr->disabled < 2){ // don't create entries for permanently disabled (==3) devices. Remark: current meaning of value 2 is unclear (unused) in rtl_433
			sprintf_s(protname, sizeof(protname), "[%03lu] %s", a + 1, devptr->name);
			LVITEM tmp_lvitem;
			tmp_lvitem.iItem = a;
			tmp_lvitem.iSubItem = 0;
			tmp_lvitem.pszText = protname;
			tmp_lvitem.mask = LVIF_TEXT | LVIF_PARAM;
			tmp_lvitem.lParam = (a + 1); // this will finally be devptr->protocol_num;
			ListView_InsertItem(hProtList, &tmp_lvitem);
		}
	}
}

// refreshes a line in the protocol listview
static BOOL protList_refreshEntry(INT listidx) {
	int n_list_entries = ListView_GetItemCount(hProtList);
	if (listidx >= n_list_entries) return FALSE;

	int prot_id = getProtocolIdFromListIdx(listidx);
	if (prot_id < 1) return FALSE;

	r_device *devptr = NULL;
	if (!getDev(prot_id - 1, &devptr)) return FALSE;

	// update param
	LPSTR param = NOPAR_MARKER;
	if (devptr->create_fn && prot_id <= dev_num && tmpprots) {
		param = (tmpprots[prot_id-1].param ? tmpprots[prot_id-1].param : "");
	}
	ListView_SetItemText(hProtList, listidx, 1, param);

	// update activation state
	LPSTR state = "n/a";
	if (tmpprots && prot_id <= dev_num) state = (tmpprots[prot_id - 1].active ? "enabled" : "disabled");
	ListView_SetItemText(hProtList, listidx, 2, state);

	return TRUE;
}

// refreshes all lines in the protocol listview
static VOID protList_refreshAll() {
	int n_list_entries = ListView_GetItemCount(hProtList);

	for (int listidx = 0; listidx < n_list_entries; listidx++) {
		protList_refreshEntry(listidx);
	}
}

static int *selected_indices = NULL; // array to temporarily hold the selection state of the list view entries
static int selected_indices_cap = 0; // current capacity of this array
#define SEL_ALLOC_STEP 20            // step size defining how much the array may grow when it runs full

// get all selected entries in the protocol listview
static BOOL protList_getSelectedEntries(INT **sel_ents, INT *n_sel, INT *n_tot) {
	*n_tot = ListView_GetItemCount(hProtList);
	if (!selected_indices_cap || *n_tot > selected_indices_cap) {
		selected_indices_cap = SEL_ALLOC_STEP * ((*n_tot / SEL_ALLOC_STEP) + 1);
		selected_indices = (int*)realloc(selected_indices, (selected_indices_cap * sizeof(int)));
	}
	if (!selected_indices) return FALSE;

	INT n_stored = 0;
	for (int a = 0; a < *n_tot; a++) {
		if (ListView_GetItemState(hProtList, a, LVIS_SELECTED) == LVIS_SELECTED) {
			selected_indices[n_stored++] = a;
		}
	}

	*n_sel = n_stored;
	*sel_ents = selected_indices;
	return TRUE;
}

// get activation state of a protocol given its list index (!= protocol id)
static BOOL protState_get(INT listidx) {
	int prot_id = getProtocolIdFromListIdx(listidx); // translate to protocol id
	if (prot_id >= 1 && prot_id <= dev_num) {
		return (tmpprots[prot_id-1].active);
	}
	return FALSE;
};

// set activation state of a protocol given its list index (!= protocol id)
static VOID protState_set(INT listidx, BOOL v) {
	int prot_id = getProtocolIdFromListIdx(listidx);  // translate to protocol id
	if (prot_id >= 1 && prot_id <= dev_num) {
		tmpprots[prot_id-1].active = v;
	}
};

// updates all dynamic protocol gui elements except the protocol list
static VOID updateProtGuiElements() {
	// count and print number of selected protocols
	int n_list_entries = ListView_GetItemCount(hProtList);
	int num_sel = 0;
	for (int a = 0; a < n_list_entries; a++) {
		if (protState_get(a)) num_sel++;
	}
	char caption[40];
	sprintf_s(caption, sizeof(caption), "Active protocols (%lu):", num_sel);
	SetDlgItemText(hDlg, IDC_RX_PROT_CAPTION, caption);
	// that's it for now...
}

// event handler for key presses in the protocol listview
static BOOL onKey(LPNMLVKEYDOWN ctx) {
	if (!ctx) return FALSE;
	if (ctx->hdr.hwndFrom == hProtList) {
		if (ctx->wVKey == VK_SPACE) {
			int *sel_ents = NULL;
			int n_sel;
			int n_tot;
			if (protList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
				if (n_sel == 1) {
					protState_set(sel_ents[0], !protState_get(sel_ents[0]));
					protList_refreshEntry(sel_ents[0]);
					updateProtGuiElements();
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}

// event handler for right clicks in the protocol listview
static BOOL onRightClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	if (ctx_in->hwndFrom == hProtList) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;

		int *sel_ents = NULL;
		int n_sel;
		int n_tot;
		if (protList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
			BOOL par_allowed = FALSE;
			if (n_sel == 1) {
				CHAR parbuf[16] = "";
				ListView_GetItemText(hProtList, *sel_ents, 1, parbuf, sizeof(parbuf));
				if (memcmp(parbuf, NOPAR_MARKER, strlen(NOPAR_MARKER)) != 0) par_allowed = TRUE;
			}

			EnableMenuItem(hPopup, ID_PRTLIST_TOGGLE, MF_BYCOMMAND | (n_sel == 1 ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(hPopup, ID_PRTLIST_ENABLE, MF_BYCOMMAND | (n_sel > 0 ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(hPopup, ID_PRTLIST_DISABLE, MF_BYCOMMAND | (n_sel > 0 ? MF_ENABLED : MF_GRAYED));
			EnableMenuItem(hPopup, ID_PRTLIST_PAREDT, MF_BYCOMMAND | (par_allowed ? MF_ENABLED : MF_GRAYED));

			POINT pt = ctx->ptAction;
			RECT list_pos;
			GetWindowRect(hProtList, &list_pos);
			DWORD cmd = TrackPopupMenu(hPopup, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON, list_pos.left + pt.x, list_pos.top + pt.y, 0, hDlg, 0);
			if (cmd == ID_PRTLIST_TOGGLE) {
				protState_set(sel_ents[0], !protState_get(sel_ents[0]));
				protList_refreshAll();
				updateProtGuiElements();
			}
			else if (cmd == ID_PRTLIST_ENABLE) {
				for (int a = 0; a < n_sel; a++) {
					protState_set(sel_ents[a], 1);
				}
				protList_refreshAll();
				updateProtGuiElements();
			}
			else if (cmd == ID_PRTLIST_DISABLE) {
				for (int a = 0; a < n_sel; a++) {
					protState_set(sel_ents[a], 0);
				}
				protList_refreshAll();
				updateProtGuiElements();
			}
			else if (cmd == ID_PRTLIST_PAREDT) {
				if(n_sel == 1 && par_allowed && sel_ents[0] < dev_num){
					LPSTR crnt_pars = tmpprots[sel_ents[0]].param; // might be NULL
					LPSTR edited = ShowTextReaderDialog(hDlg, "Edit protocol parameters", crnt_pars, 2000);
					if (edited) {
						size_t cap_avail = (tmpprots[sel_ents[0]].param ? strlen(tmpprots[sel_ents[0]].param) + 1 : 0);
						size_t cap_reqd = strlen(edited) + 1;
						if (cap_avail < cap_reqd) tmpprots[sel_ents[0]].param = (LPSTR) realloc(tmpprots[sel_ents[0]].param, cap_reqd);
						if (tmpprots[sel_ents[0]].param) strcpy_s(tmpprots[sel_ents[0]].param, cap_reqd, edited);
						protList_refreshEntry(sel_ents[0]);
					}
				}
			}
		}
		return TRUE;
	}
	return FALSE;
}

// event handler for double clicks in the protocol listview
static BOOL onDoubleClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	if (ctx_in->hwndFrom == hProtList) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;

		int idx_clicked = ctx->iItem;
		int *sel_ents = NULL;
		int n_sel;
		int n_tot;
		if (protList_getSelectedEntries(&sel_ents, &n_sel, &n_tot)) {
			if (n_sel == 1 && sel_ents[0] == idx_clicked) {
				protState_set(sel_ents[0], !protState_get(sel_ents[0]));
			}
			protList_refreshEntry(sel_ents[0]);
			updateProtGuiElements();
		}
		return TRUE;
	}
	return FALSE;
}

// event handler for GUI commands
static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_RX_PROT_SELALL:
		prot_LoadConstellation(PROTS_ALL);
		protList_refreshAll();
		updateProtGuiElements();
		break;

	case IDC_RX_PROT_SELNONE:
		prot_LoadConstellation(PROTS_NONE);
		protList_refreshAll();
		updateProtGuiElements();
		break;

	case IDC_RX_PROT_DEFAULTS:
		prot_LoadConstellation(PROTS_DEFAULTS);
		protList_refreshAll();
		updateProtGuiElements();
		break;

	case IDC_RX_PROT_OK: {
		list_ensure_size(&rtl->cfg->active_prots, dev_num);
		int a;
		// 1) for all existing entries of rtl->cfg->active_prots: refresh with new setting
		for (a = 0; a < rtl->cfg->active_prots.len && a < dev_num; a++) {
			// if current protocol was enabled, place its params (if specified) or an empty param string to mark it as enabled
			if (tmpprots[a].active) {
				size_t reqd = (tmpprots[a].param ? strlen(tmpprots[a].param) + 1 : 1);
				size_t avail = (rtl->cfg->active_prots.elems[a] ? strlen((char*) rtl->cfg->active_prots.elems[a]) + 1 : 0);
				if (reqd > avail) rtl->cfg->active_prots.elems[a] = realloc(rtl->cfg->active_prots.elems[a], reqd);
				if (rtl->cfg->active_prots.elems[a]) {
					strcpy_s((char*) rtl->cfg->active_prots.elems[a], reqd, (tmpprots[a].param ? tmpprots[a].param : ""));
				}
			}
			// if current protocol was not enabled, set rtl->cfg->active_prots entry to NULL
			else if (rtl->cfg->active_prots.elems[a]) {
				free(rtl->cfg->active_prots.elems[a]);
				rtl->cfg->active_prots.elems[a] = NULL;
			}
		}
		// 2) if rtl->cfg->active_prots was initially empty (or not filled completely) append (remaining) rtl->cfg->active_prots entries
		for (; a < dev_num; a++) {
			char *par = NULL;
			if (tmpprots[a].active) {
				par = (tmpprots[a].param ? strdup(tmpprots[a].param) : strdup(""));
			}
			list_push(&rtl->cfg->active_prots, par);
		}
		EndDialog(hDlg, TRUE);
		break;
	}

	case IDC_RX_PROT_CANCEL:
		EndDialog(hDlg, TRUE);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

// Event handler for GUI initialization
static VOID onInit(HWND hwndDlg) {
	hDlg = hwndDlg;
	HICON hIcon = LoadIcon(GetModuleHandle(0), (const char *)IDI_ICON1);
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	hProtList = GetDlgItem(hDlg, IDC_RX_PROT_LIST);
	protList_init();
	protList_refreshAll();
	updateProtGuiElements();
}

// Dialog handler
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

int ShowProtocolDialog(rtl_433_t *rtl_obj, HWND hParent, HMENU hPopupMenu) {
	hPopup = hPopupMenu;
	int r = -1;
	if (rtl_obj) {
		rtl = rtl_obj;
		dev_num = getDevCount();
		// allocate temporary arrays for protocol selection (on first call only)
		if (!tmpprots) tmpprots = (protstate*) calloc(dev_num, sizeof(protstate));
		if (!selected_indices) selected_indices = (int*)calloc(dev_num, sizeof(int));

		if (tmpprots && selected_indices) {
			
			// if no custom selection has been made since the start of the program, load default values from dev_pointers
			if (!rtl->cfg->active_prots.len) prot_LoadConstellation(PROTS_DEFAULTS);
			
			// else take protocol configuration from librtl_433 configuration
			else {
				for (int a = 0; a < dev_num; a++) {
					// set activation state
					tmpprots[a].active = (a < rtl->cfg->active_prots.len ? (rtl->cfg->active_prots.elems[a] != NULL) : FALSE);
					// set parameter
					LPSTR src   = (LPSTR) rtl->cfg->active_prots.elems[a];
					size_t   src_l = (src?strlen(src)+1 : 0);
					LPSTR trg   = tmpprots[a].param;
					size_t   trg_l = (trg?strlen(trg)+1 : 0);
					// a) no param
					if (!src || !src[0]) {
						if (trg) {
							free(trg);
							tmpprots[a].param = NULL;
						}
					}
					// b) non-fitting (longer) param
					else if (src && src_l > trg_l) {
						tmpprots[a].param = trg = (LPSTR) realloc(trg, src_l);
						if(trg) strcpy_s(trg, src_l, src);
					}
					// c) fitting param
					else {
						if(trg) strcpy_s(trg, trg_l, src);
					}
				}
			}

			// Open the dialog
			r = (int)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_RX_PROT), hParent, (DLGPROC)DialogHandler);
		}
	}
	return r;
}
