/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *               rtl_fl2k_433 main window (TX part)                *
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
#include <shlobj.h>		// Contains SHBrowseForFolder

#include "libfl2k_433.h"

#include "wnd_main.h"
#include "wnd_main_tx.h"
#include "wnd_main_txlist.h"
#include "wnd_txopt_dev.h"
#include "wnd_txopt_freq.h"
#include "wnd_txopt_exp.h"
#include "wnd_txwarn.h"
#include "wndelements.h"
#include "rxlist_entry.h"
#include "logwrap.h"

#include "../res/gui_win_resources.h"

#define REG_PATH "Software\\rtl_fl2k_433" // todo: move to more appropriate place?

typedef enum {
	TXMODE_FILE = 0,
	TXMODE_FL2KDEV = 1,
} TxMode;

static fl2k_433_t	*tx = NULL;
static txlist		*lv_tx = NULL;
static HANDLE		hTQthread = INVALID_HANDLE_VALUE; // thread to monitor TX queue
static HANDLE		hTXthread = INVALID_HANDLE_VALUE; // thread to perform TX activity
static BOOL			tx_autostartstop;
static BOOL			close_request;
static HWND         hwndMainDlg;
static HMENU 		popup_txlist;
static TxMode		txmode = TXMODE_FILE;
static CHAR			out_dir_gui[MAX_PATH]; // holds the FL2K output path selected in the GUI (since populating tx->cfg.out_dir forces libfl2k_433 into file mode)

static WndElems *ctllist = NULL; // list of information about (RX-related) window elements, used for resizing etc.

static VOID loadSettings() {
	if (!tx) return;

	HKEY regkey;
	DWORD disposition;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &regkey, &disposition) != ERROR_SUCCESS) return;
	if (disposition == REG_CREATED_NEW_KEY) {
		RegCloseKey(regkey);
		return;
	}

	DWORD tmp_dw;
	DWORD tmp_size = sizeof(tmp_dw);
	DWORD tmp_type;
	if (RegQueryValueEx(regkey, "TX_samp_rate", NULL, &tmp_type, (LPBYTE)&tmp_dw, &tmp_size) == ERROR_SUCCESS && tmp_type == REG_DWORD && tmp_size == sizeof(tmp_dw)) {
		tx->cfg.samp_rate = tmp_dw;
	}
	tmp_size = sizeof(tmp_dw);
	if (RegQueryValueEx(regkey, "TX_carrier", NULL, &tmp_type, (LPBYTE)&tmp_dw, &tmp_size) == ERROR_SUCCESS && tmp_type == REG_DWORD && tmp_size == sizeof(tmp_dw)) {
		tx->cfg.carrier = tmp_dw;
	}
	tmp_size = sizeof(out_dir_gui);
	if (RegQueryValueEx(regkey, "TX_out_dir", NULL, &tmp_type, (LPBYTE)out_dir_gui, &tmp_size) != ERROR_SUCCESS || tmp_type != REG_SZ) {
		out_dir_gui[0] = 0;
	}
	
	RegCloseKey(regkey);
}

static VOID saveSettings() {
	if (!tx) return;

	HKEY regkey;
	DWORD disposition;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &regkey, &disposition) != ERROR_SUCCESS) return;

	RegSetValueEx(regkey, "TX_samp_rate", 0, REG_DWORD, (LPBYTE)&tx->cfg.samp_rate, sizeof(tx->cfg.samp_rate));
	RegSetValueEx(regkey, "TX_carrier", 0, REG_DWORD, (LPBYTE)&tx->cfg.carrier, sizeof(tx->cfg.carrier));
	RegSetValueEx(regkey, "TX_out_dir", 0, REG_SZ, (LPBYTE)out_dir_gui, (DWORD)strlen(out_dir_gui) + 1);

	RegCloseKey(regkey);
}

static void RefreshTxCfgElements() {
	SetDlgItemText(hwndMainDlg, IDC_MAIN_TX_FILE_DSP, out_dir_gui);

	if (ctllist) {
		ctllist->show(IDC_MAIN_TX_FILE_DSP, (txmode == TXMODE_FILE ? SW_SHOW : SW_HIDE));
		ctllist->show(IDC_MAIN_TX_FILE_BTN, (txmode == TXMODE_FILE ? SW_SHOW : SW_HIDE));
		ctllist->show(IDC_MAIN_TX_SDR_DSP, (txmode == TXMODE_FILE ? SW_HIDE : SW_SHOW));
		ctllist->show(IDC_MAIN_TX_SDR_BTN, (txmode == TXMODE_FILE ? SW_HIDE : SW_SHOW));

		INT freqpreset = (INT)ctllist->sendMsg(IDC_MAIN_TX_FREQ_CMB, CB_GETCURSEL, 0, 0);

		ctllist->enable(IDC_MAIN_TX_MODE,     (hTXthread == INVALID_HANDLE_VALUE));
		ctllist->enable(IDC_MAIN_TX_SDR_DSP,  (hTXthread == INVALID_HANDLE_VALUE));
		ctllist->enable(IDC_MAIN_TX_SDR_BTN,  (hTXthread == INVALID_HANDLE_VALUE));
		ctllist->enable(IDC_MAIN_TX_FILE_DSP, (hTXthread == INVALID_HANDLE_VALUE));
		ctllist->enable(IDC_MAIN_TX_FILE_BTN, (hTXthread == INVALID_HANDLE_VALUE));
		ctllist->enable(IDC_MAIN_TX_FREQ_LBL, (hTXthread == INVALID_HANDLE_VALUE && tx && ((tx->cfg.dev_index > 0) || out_dir_gui[0])));
		ctllist->enable(IDC_MAIN_TX_FREQ_CMB, (hTXthread == INVALID_HANDLE_VALUE && tx && ((tx->cfg.dev_index > 0) || out_dir_gui[0])));
		ctllist->enable(IDC_MAIN_TX_FREQ_BTN, (hTXthread == INVALID_HANDLE_VALUE && tx && ((tx->cfg.dev_index > 0) || out_dir_gui[0]) && freqpreset == 0));
		ctllist->enable(IDC_MAIN_TX_STARTSTOP, (tx && !tx_autostartstop && ((tx->cfg.dev_index > 0) || out_dir_gui[0])));
		ctllist->enable(IDC_MAIN_TX_AUTOSTART, (tx && hTXthread == INVALID_HANDLE_VALUE && ((tx->cfg.dev_index > 0) || out_dir_gui[0])));
	}
}

static DWORD WINAPI TX_Start(LPVOID param) {
	BOOL auto_started = (param!=NULL);

	// show warning (will not open if previously accepted)
	if (ShowWarningDialog(hwndMainDlg)) {
		// start TX
		if (tx) {
			if (txmode != TXMODE_FILE) {
				lv_tx->setActiveStyle(TRUE);
				Gui_fprintf(stderr, "started TX via FL2K.\n");
			}
			strcpy_s(tx->cfg.out_dir, sizeof(tx->cfg.out_dir), (txmode == TXMODE_FILE ? out_dir_gui : ""));
			txstart(tx);
			if (txmode != TXMODE_FILE) {
				lv_tx->setActiveStyle(FALSE);
				Gui_fprintf(stderr, "stopped TX via FL2K.\n");
			}
		}
	}
	// if warning was not accepted, cancel autostart mode (to avoid endless warning loop)
	else if (tx_autostartstop){
		tx_autostartstop = FALSE;
		CheckDlgButton(hwndMainDlg, IDC_MAIN_TX_AUTOSTART, BST_UNCHECKED);
	}

	// close thread
	if (!auto_started) SetDlgItemText(hwndMainDlg, IDC_MAIN_TX_STARTSTOP, "Start fl2k_433");
	hTXthread = INVALID_HANDLE_VALUE;
	RefreshTxCfgElements();// re-enables autostar button
	return 1;
}

static DWORD WINAPI TX_Queue_Manager(LPVOID param) {
	if (!tx || !lv_tx) {
		Gui_fprintf(stderr, "Unexpected internal error in TX_Queue_Manager: No tx or lv_tx object (NULL pointer)\n");
		return 0;
	}

	while (1) {
		// if TX is not active, then... 
		while (lv_tx->CountUnsentEntries() > 0 && !close_request && getState(tx) < FL2K433_RUNNING_FL2K){
			if (tx_autostartstop && hTXthread == INVALID_HANDLE_VALUE){  //... auto-start TX thread (if allowed) or....
				hTXthread = CreateThread(0, 0, &TX_Start, (LPVOID) 1, 0, 0);
				RefreshTxCfgElements();// disables autostart button while active
			}
			Sleep(200); // wait for TX to be started automatically (see above) or manually by the user
		}

		// if TX queue is not free wait until TX is finished
		while (lv_tx->CountUnsentEntries() > 0 && !close_request && getQueueLength(tx) > 0) {
			Sleep(200); // wait for TX to finish current message
		}

		// determine the next unsent TX entry from GUI queue
		INT unsent_index;
		tx_entry *unsent_entry = lv_tx->GetFirstUnsentEntry(&unsent_index);
		// if there is any, add TX message to fl2k_433 queue
		if (unsent_entry && !close_request) {
			// Set timestamp
			char time_str[20];
			SYSTEMTIME st;
			GetLocalTime(&st);
			sprintf_s(time_str, sizeof(time_str), "%02lu:%02lu:%02lu.%03lu", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
			unsent_entry->setSentTime(time_str);
			lv_tx->refreshLine(unsent_index); // Refresh QUI queue (sent time)
			// Queue TX message to fl2k_433
			QueueTxMsg(tx, unsent_entry->getMessage());

		}
		// if the list view queue has no more unsent items the queue manager can halt for now (will be restarted when next message is added to the queue)
		if(!unsent_entry || close_request) break;
	}
	// auto-stop TX (if allowed)
	if (tx_autostartstop && !close_request) {
		// wait for TX to finish queue
		while (tx->txqueue != NULL) { // todo: A timeout should be added here - at least if infinite messages of MODULATION_TYPE_SINE will be productively used one day
			Sleep(200);
		}
		// stop tx
		txstop_signal(tx);
	}

	hTQthread = INVALID_HANDLE_VALUE;
	return 1;
}

static void RefreshTxListInfo() {
	// Refresh number of messages
	INT num_all = lv_tx->CountAllEntries();
	INT num_sent = num_all - lv_tx->CountUnsentEntries();

	char tmp[40];
	sprintf_s(tmp, sizeof(tmp), "Messages queued: %lu", num_all);
	SetDlgItemText(hwndMainDlg, IDC_MAIN_TX_COUNTER_Q, tmp);
	sprintf_s(tmp, sizeof(tmp), "Messages sent: %lu", num_sent);
	SetDlgItemText(hwndMainDlg, IDC_MAIN_TX_COUNTER_S, tmp);

	// Refresh selected message display
	int sel_id = lv_tx->GetSelectedItem();
	char dscr_buf[400];
	SetDlgItemText(hwndMainDlg, IDC_MAIN_TX_DETAILS, lv_tx->getStrView(sel_id, dscr_buf, sizeof(dscr_buf)));
}

VOID QueueRxPulseToTx(rx_entry *entry, BOOL entirepulse) {
	if (!lv_tx || !entry) return;
	pPulseDatCompact pulses = entry->getPulses();
	if (!pulses) return;

	// print description
	char dscr[100];
	char *time = entry->getTime();
	char *tp = entry->getType();
	char *md = entry->getModel();
	if (!time || (!tp && !md)) {
		Gui_fprintf(stderr, "Unexpected internal error in QueueRxPulseToTx: entry has no time property or neither type nor model are set\n");
		return;
	}
	if (tp && *tp && md && *md) sprintf_s(dscr, "%s: %s", tp, md);
	else if (md && *md) sprintf_s(dscr, "%s", md);
	else if (tp && *tp) sprintf_s(dscr, "%s", tp);

	// copy pulses
	unsigned int buf_c = pulses->num_samples;
	unsigned int buf_l = 0;
	char* buf = (char*)malloc(buf_c);
	unsigned int s = (entirepulse || !pulses->segment_len ? 0 : pulses->segment_startidx);
	unsigned int e = (entirepulse || !pulses->segment_len ? pulses->num_pulses : min(pulses->segment_startidx + pulses->segment_len, pulses->num_pulses));
	for (unsigned int a = s; a < e; a++) {
		unsigned int l = min((unsigned int) pulses->pulse[a], buf_c-buf_l);
		memset(&buf[buf_l], 1, l);
		buf_l += l;
		l = min((unsigned int) pulses->gap[a], buf_c-buf_l);
		memset(&buf[buf_l], 0, l);
		buf_l += l;
	}

	// create tx_entry
	tx_entry *te = new tx_entry(dscr, MODULATION_TYPE_OOK, buf, buf_l, pulses->samplerate);
	if (!te) {
		free(buf);
		Gui_fprintf(stderr, "Unexpected internal error in QueueRxPulseToTx: no memory left for allocating new tx_entry\n");
		return;
	}

	lv_tx->AddTxEntry(te);
	RefreshTxListInfo();

	// start TX queue handler if not currently running
	if (hTQthread == INVALID_HANDLE_VALUE) {
		hTQthread = CreateThread(0, 0, &TX_Queue_Manager, 0, 0, 0);
	}
}

static BOOL Fl2kQuickLoad() {
	// Quick-Load only if there's exactly 1 RTL-SDR
	if (!tx || fl2k_get_device_count() != 1) return FALSE;

	// Get display name to update window element
	const char *product = fl2k_get_device_name(0);
	if (product) {
		SetDlgItemText(hwndMainDlg, IDC_MAIN_TX_SDR_DSP, product);
	}

	// configure this device
	tx->cfg.dev_index = 1;
	return TRUE;
}

static VOID RebuildTxFreqCombo(BOOL update_sel = FALSE) {
	HWND hTxFreqCmb = (ctllist ? ctllist->getHandle(IDC_MAIN_TX_FREQ_CMB) : NULL);
	if (hTxFreqCmb) {
		// Save old selection and clear list
		INT oldsel = max((INT)ctllist->sendMsg(IDC_MAIN_TX_FREQ_CMB, CB_GETCURSEL, 0, 0), 0);
		SendMessage(hTxFreqCmb, CB_RESETCONTENT, 0, 0);
		// Manual configuration:
		char freqstr[64];
		if(tx) sprintf_s(freqstr, sizeof(freqstr), "Custom configuration (%.02fMS/s with %.02fMHz carrier)", ((double)tx->cfg.samp_rate / 1000000.0), ((double)tx->cfg.carrier / 1000000.0));
		else   sprintf_s(freqstr, sizeof(freqstr), "Custom configuration");
		SendMessage(hTxFreqCmb, CB_ADDSTRING, 0, (LPARAM)freqstr);
		// Some popular presets:
		SendMessage(hTxFreqCmb, CB_ADDSTRING, 0, (LPARAM)"Preset1: 433.92MHz@85.5MS/s (5th harmonic)");
		SendMessage(hTxFreqCmb, CB_ADDSTRING, 0, (LPARAM)"Preset2: 433.92MHz@42.7MS/s (10th harmonic)");
		SendMessage(hTxFreqCmb, CB_ADDSTRING, 0, (LPARAM)"Preset3: 433.92MHz@21.3MS/s (20th harmonic)");
		// Update or restore selection 
		if (update_sel) {
			if      (tx && tx->cfg.samp_rate == FL2K_PRESET1_SRATE && tx->cfg.carrier == FL2K_PRESET1_CARRIER) SendMessage(hTxFreqCmb, CB_SETCURSEL, 1, 0);
			else if (tx && tx->cfg.samp_rate == FL2K_PRESET2_SRATE && tx->cfg.carrier == FL2K_PRESET2_CARRIER) SendMessage(hTxFreqCmb, CB_SETCURSEL, 2, 0);
			else if (tx && tx->cfg.samp_rate == FL2K_PRESET3_SRATE && tx->cfg.carrier == FL2K_PRESET3_CARRIER) SendMessage(hTxFreqCmb, CB_SETCURSEL, 3, 0);
			else                                                                                               SendMessage(hTxFreqCmb, CB_SETCURSEL, 0, 0);

		}
		else SendMessage(hTxFreqCmb, CB_SETCURSEL, oldsel, 0);
	}
}

static BOOL GetSaveFolder(LPSTR buf, INT cap, LPSTR title) {
	char fpath[MAX_PATH];
	BROWSEINFO BrwsInf = { hwndMainDlg,0,0,title, BIF_RETURNONLYFSDIRS,0,0,0 };
	PIDLIST_ABSOLUTE FldId = SHBrowseForFolder(&BrwsInf);
	if (!FldId || !SHGetPathFromIDList(FldId, fpath)) return FALSE;
	strcpy_s(buf, cap, fpath);
	return TRUE;
}

VOID TxGui_onSizeChange(int add_w, int add_h, double fac_w, double fac_h, BOOL redraw) {
	if (!ctllist) return;

	WndElem *e = ctllist->elems;
	while (e) {
		switch (e->ctlid) {
			// everything completely above the TX listview just scales horizontally
			case IDC_MAIN_TX_MODE:
			case IDC_MAIN_TX_FILE_DSP:
			case IDC_MAIN_TX_FILE_BTN:
			case IDC_MAIN_TX_SDR_DSP:
			case IDC_MAIN_TX_SDR_BTN:
			case IDC_MAIN_TX_FREQ_LBL:
			case IDC_MAIN_TX_FREQ_CMB:
			case IDC_MAIN_TX_FREQ_BTN:
			case IDC_MAIN_TX_STARTSTOP:
			case IDC_MAIN_TX_AUTOSTART:
			case IDC_MAIN_TX_COUNTER_Q:
			case IDC_MAIN_TX_COUNTER_S:
			case IDC_MAIN_TX_STAT02:
				MoveWindow(e->hWnd, (int)(fac_w*e->initrect.left), e->initrect.top, (int)(fac_w*(e->initrect.right - e->initrect.left)), e->initrect.bottom - e->initrect.top, redraw);
				break;

			// everything completely below the TX listview scales horizontally with a vertical shift
			case IDC_MAIN_TX_STAT03:
			case IDC_MAIN_TX_DETAILS:
				MoveWindow(e->hWnd, (int)(fac_w*e->initrect.left), e->initrect.top + add_h, (int)(fac_w*(e->initrect.right - e->initrect.left)), e->initrect.bottom - e->initrect.top, redraw);
				break;

			// The TX listview and surrounding objects scale horizontally with a vertical enlargement
			case IDC_MAIN_TX_LIST:
			case IDC_MAIN_TX_STAT01:
				MoveWindow(e->hWnd, (int)(fac_w*e->initrect.left), e->initrect.top, (int)(fac_w*(e->initrect.right - e->initrect.left)), e->initrect.bottom - e->initrect.top + add_h, redraw);
				break;

			default:
				break;
		}

		e = e->next;
	}

}

VOID TxGui_onInit(HWND hMainWnd, HMENU poptx){
	hwndMainDlg = hMainWnd;
	popup_txlist = poptx;

	ctllist = new WndElems(hwndMainDlg);

	HWND hTxList = NULL;
	HWND hCmbTxMode = NULL;

	if(ctllist){
		ctllist->add(IDC_MAIN_TX_DETAILS);
		hTxList = ctllist->add(IDC_MAIN_TX_LIST);
		hCmbTxMode = ctllist->add(IDC_MAIN_TX_MODE);
		ctllist->add(IDC_MAIN_TX_FILE_DSP);
		ctllist->add(IDC_MAIN_TX_FILE_BTN);
		ctllist->add(IDC_MAIN_TX_SDR_DSP);
		ctllist->add(IDC_MAIN_TX_SDR_BTN);
		ctllist->add(IDC_MAIN_TX_FREQ_LBL);
		ctllist->add(IDC_MAIN_TX_FREQ_CMB);
		ctllist->add(IDC_MAIN_TX_FREQ_BTN);
		ctllist->add(IDC_MAIN_TX_STARTSTOP);
		ctllist->add(IDC_MAIN_TX_AUTOSTART);
		ctllist->add(IDC_MAIN_TX_COUNTER_Q);
		ctllist->add(IDC_MAIN_TX_COUNTER_S);
		ctllist->add(IDC_MAIN_TX_STAT01);
		ctllist->add(IDC_MAIN_TX_STAT02);
		ctllist->add(IDC_MAIN_TX_STAT03);
	}

	// Init TX mode Combo
	if (hCmbTxMode) {
		SendMessage(hCmbTxMode, CB_ADDSTRING, 0, (LPARAM)"Directory:");
		SendMessage(hCmbTxMode, CB_ADDSTRING, 0, (LPARAM)"Device:");
	}

	// Init TX
	fl2k_433_init(&tx);
	if (tx) {
		txmode = (Fl2kQuickLoad() ? TXMODE_FL2KDEV : TXMODE_FILE);
		if(hCmbTxMode) SendMessage(hCmbTxMode, CB_SETCURSEL, (txmode == TXMODE_FL2KDEV ? 1 : 0), 0);
		loadSettings();
		RebuildTxFreqCombo(TRUE);
		RefreshTxCfgElements();
	}
	else Gui_fprintf(stderr, "Error: Could not create libfl2k_433 instance.\n");


	if(hTxList) lv_tx = new txlist(hTxList, hwndMainDlg, popup_txlist);
}

BOOL TxGui_onKey(LPNMLVKEYDOWN ctx) {
	if (!ctx) return FALSE;
	if (lv_tx && ctx->hdr.hwndFrom == lv_tx->getWndHandle()) {
		if (ctx->wVKey == VK_DELETE) {
			int sel_id = lv_tx->GetSelectedItem();
			lv_tx->RemoveTxEntry(sel_id);
			RefreshTxListInfo();
			return TRUE;
		}
	}
	return FALSE;
}

BOOL TxGui_onRightClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	if (lv_tx && ctx_in->hwndFrom == lv_tx->getWndHandle()) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;

		INT selidx;
		DWORD cmd = lv_tx->getRightClickCommand(ctx, &selidx);
		switch (cmd) {
		case ID_TXLIST_DEL:
			lv_tx->RemoveTxEntry(selidx);
			RefreshTxListInfo();
			break;
		case ID_TXLIST_DELALL:
			lv_tx->Clear();
			RefreshTxListInfo();
			break;
		case ID_TXLIST_RESEND: {
			tx_entry *e = lv_tx->GetTxEntry(selidx);
			e->setSentTime(NULL);
			lv_tx->refreshLine(selidx);
			// start TX queue handler if not currently running
			if (hTQthread == INVALID_HANDLE_VALUE) {
				hTQthread = CreateThread(0, 0, &TX_Queue_Manager, 0, 0, 0);
			}
			break;
		}
		default:
			break;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL TxGui_onDoubleClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	// if (lv_tx && ctx_in->hwndFrom == lv_tx->getWndHandle()) {
		// LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;
		// ...
		// return TRUE;
	//}
	return FALSE;
}

BOOL TxGui_onChangedItem(LPNMLISTVIEW ctx) {
	if (!ctx) return FALSE;
	if (lv_tx && ctx->hdr.hwndFrom == lv_tx->getWndHandle()) {  // Iconstates geändert (also auch evtl. Selections -> rx info aktualisieren)
		if (ctx->uChanged & LVIF_STATE) {
			unsigned long states_changed = (ctx->uOldState ^ ctx->uNewState);
			if ((states_changed & LVIS_SELECTED)) {
				RefreshTxListInfo();
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL TxGui_onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

		case IDC_MAIN_TX_MODE:
			if (ctllist) {
				int sel = (int)ctllist->sendMsg(IDC_MAIN_TX_MODE, CB_GETCURSEL, 0, 0);
				if (sel >= TXMODE_FILE && sel <= TXMODE_FL2KDEV && sel != txmode) {
					txmode = (TxMode)sel;
					if (txmode == TXMODE_FL2KDEV && !tx->cfg.dev_index) Fl2kQuickLoad();
					RefreshTxCfgElements();
				}
			}
			break;

		case IDC_MAIN_TX_FILE_BTN: {
			if (!tx) break;
			if (!GetSaveFolder(out_dir_gui, sizeof(out_dir_gui), "Select output directory")) {
				out_dir_gui[0] = 0;
			}
			RefreshTxCfgElements();
			break;
		}

		case ID_TX_DEV:
		case IDC_MAIN_TX_SDR_BTN: {
			if (!tx) break;
			char *devname = NULL;
			if (ShowDeviceTxDialog(tx, hwndMainDlg, &devname) > 0) {
				SetDlgItemText(hwndMainDlg, IDC_MAIN_TX_SDR_DSP, (devname && *devname ? devname : ""));
				RefreshTxCfgElements();
			}
			break;
		}

		case  IDC_MAIN_TX_FREQ_CMB: {
			if (!tx || !ctllist) break;
			INT sel = (INT)ctllist->sendMsg(IDC_MAIN_TX_FREQ_CMB, CB_GETCURSEL, 0, 0);
			if (sel == 1) {
				tx->cfg.samp_rate = FL2K_PRESET1_SRATE;
				tx->cfg.carrier = FL2K_PRESET1_CARRIER;
			}
			else if (sel == 2) {
				tx->cfg.samp_rate = FL2K_PRESET2_SRATE;
				tx->cfg.carrier = FL2K_PRESET2_CARRIER;
			}
			else if (sel == 3) {
				tx->cfg.samp_rate = FL2K_PRESET3_SRATE;
				tx->cfg.carrier = FL2K_PRESET3_CARRIER;
			}
			RebuildTxFreqCombo(); // update custom frequency in combo box
			RefreshTxCfgElements();
			break;
		}

		case ID_TX_FREQ:
		case IDC_MAIN_TX_FREQ_BTN: {
			if (!tx) break;
			ShowFrequencyTxDialog(tx, hwndMainDlg);
			RebuildTxFreqCombo(TRUE); // update custom frequency in combo box
			break;
		}

		case ID_TX_EXP:
			ShowExpertDialog(tx, hwndMainDlg);
			break;

		case IDC_MAIN_TX_STARTSTOP:
			if (!tx) break;
			if (hTXthread == INVALID_HANDLE_VALUE) {
				SetDlgItemText(hwndMainDlg, IDC_MAIN_TX_STARTSTOP, "Stop fl2k_433");
				hTXthread = CreateThread(0, 0, &TX_Start, NULL, 0, 0);
				RefreshTxCfgElements();// disables auto-start button while active
			}
			else {
				txstop_signal(tx);
			}
			break;

		case IDC_MAIN_TX_AUTOSTART:
			tx_autostartstop = (IsDlgButtonChecked(hwndMainDlg, IDC_MAIN_TX_AUTOSTART) == BST_CHECKED);
			RefreshTxCfgElements();
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

BOOL TxGui_prepare() {
	close_request = FALSE;
	tx_autostartstop = FALSE;
	if (ctllist || tx || lv_tx) return FALSE;
	return TRUE;
}

BOOL TxGui_shutdownRequest() {
	if (hTQthread != INVALID_HANDLE_VALUE) {
		if (MessageBox(hwndMainDlg, "TX queue has not been processed completely. Cancel anyways? ('no' returns to program)", "rtl_fl2k_433", MB_YESNO | MB_ICONQUESTION) != IDYES) {
			return FALSE;
		}
	}
	if (hTXthread != INVALID_HANDLE_VALUE) {
		if (MessageBox(hwndMainDlg, "TX (transmitting) is still active. Cancel it? ('no' returns to program)", "rtl_fl2k_433", MB_YESNO | MB_ICONQUESTION) != IDYES) {
			return FALSE;
		}
	}
	return TRUE;
}

VOID TxGui_shutdownConfirm() {
	if (tx) txstop_signal(tx);
	close_request = TRUE; // this will cause the TX queue thread to exit
}

BOOL TxGui_isActive() {
	return (hTXthread != INVALID_HANDLE_VALUE || hTQthread != INVALID_HANDLE_VALUE);
}

VOID TxGui_cleanup() {
	saveSettings();

	if (tx) {
		fl2k_433_destroy(tx);
		tx = NULL;
	}
	if (lv_tx) {
		delete lv_tx;
		lv_tx = NULL;
	}

	if (ctllist) {
		delete ctllist;
		ctllist = NULL;
	}
}
