/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *               rtl_fl2k_433 main window (RX part)                *
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
#include <stdlib.h>

#include "librtl_433.h"
#include "rtl-sdr.h"
#include "configure.h"
#include "wnd_main.h"
#include "wnd_main_rx.h"
#include "wnd_main_rxlist.h"
#include "wnd_main_rxdetails.h"
#include "wnd_main_rxscope.h"
#include "wnd_rxopt_siggrab.h"
#include "wnd_rxopt_strgrab.h"
#include "wnd_rxopt_prot.h"
#include "wnd_rxopt_flex.h"
#include "wnd_rxopt_freq.h"
#include "wnd_rxopt_dev.h"
#include "wnd_rxopt_out.h"
#include "wnd_rxopt_exp.h"
#include "wndelements.h"
#include "logwrap.h"
#include "wnd_main_tx.h"

#include "../res/gui_win_resources.h"

#define REG_PATH "Software\\rtl_fl2k_433" // todo: move to more appropriate place?

static rtl_433_t	*rx = NULL;
static rxlist		*lv_rx = NULL;
static rxdetails	*lv_rxdet = NULL;
static ScopeWnd		*scw = NULL;
static HANDLE		hRXthread = INVALID_HANDLE_VALUE; // thread to perform RX reception
static HANDLE		hCRthread = INVALID_HANDLE_VALUE; // thread to coordinate program closing

static BOOL	tmp_outputstate_kv;
static BOOL	tmp_outputstate_csv;
static BOOL	tmp_outputstate_json;
static BOOL	tmp_outputstate_udp;
static BOOL	tmp_output_tofile;

static HMENU mainmenu, popup_rxlist, popup_details, popup_rxflex, popup_rxfreq, popup_rxprot;
static HWND hwndMainDlg;

static WndElems *ctllist = NULL; // list of information about (RX-related) window elements, used for resizing etc.

static void RefreshScope() {
	char lbl[20];
	int z = (int)(scw->getZoom()*100.0);
	sprintf_s(lbl, sizeof(lbl), "Zoom: %lu%%", z);
	SetDlgItemText(hwndMainDlg, IDC_MAIN_RX_SCOPE_LBLZ, lbl);
	scw->refreshBox();
}

static void RefreshRxListInfo() {
	// Refresh number of messages
	char tmp[40];
	sprintf_s(tmp, sizeof(tmp), "Messages received: %lu", lv_rx->GetRxEntryCount());
	SetDlgItemText(hwndMainDlg, IDC_MAIN_RX_COUNTER, tmp);

	// Refresh selected message display
	int sel_id = lv_rx->GetSelectedItem();
	if (sel_id >= 0) {
		// Display details
		if (lv_rxdet) {
			lv_rxdet->DisplayData(lv_rx->getData(sel_id));
		}
		// Display scope
		if (scw) {
			scw->setSource(lv_rx->getPulses(sel_id));
			if (ctllist) {
				ctllist->enable(IDC_MAIN_RX_SCOPE_ZOOM, (scw->isSourceSet()));
				ctllist->enable(IDC_MAIN_RX_SCOPE_OFFSET, (scw->isSourceSet()));
			}
		}
	}
	else {
		if (lv_rxdet) lv_rxdet->DisplayData(NULL);
		if (scw) scw->setSource(NULL);
	}
}

static void RefreshRxCfgElements() {
	if (!rx) return;

	// Check menu items (active RX mode and dump mode)
	BOOL md_a = FALSE, md_A = FALSE, md_S = FALSE, md_w = FALSE;
	if (rx) {
		if (rx->cfg->analyze_am)                 md_a = TRUE;
		else if (rx->cfg->analyze_pulses)        md_A = TRUE;

		if (rx->cfg->grab_mode != GRAB_DISABLED) md_S = TRUE;
		if (rx->cfg->out_filename[0])            md_w = TRUE;
	}
	CheckMenuItem(mainmenu, ID_RX_MODE_ANALYZE,  MF_BYCOMMAND | (md_a ? MF_CHECKED: MF_UNCHECKED));
	CheckMenuItem(mainmenu, ID_RX_MODE_PLSANLZ,  MF_BYCOMMAND | (md_A ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(mainmenu, ID_RX_DUMP_SIGS,     MF_BYCOMMAND | (md_S ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(mainmenu, ID_RX_DUMP_STREAM ,  MF_BYCOMMAND | (md_w ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(mainmenu, ID_RX_DSP_HIRES,     MF_BYCOMMAND | (rx->cfg->report_time_hires ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(mainmenu, ID_RX_DSP_META,      MF_BYCOMMAND | (rx->cfg->report_meta ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(mainmenu, ID_RX_DSP_BITS,      MF_BYCOMMAND | (rx->cfg->verbose_bits ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(mainmenu, ID_RX_DSP_UNKNOWN,   MF_BYCOMMAND | (rx->cfg->report_unknown ? MF_CHECKED : MF_UNCHECKED));

	// Disable Menu items while RX is running
	DWORD state = (hRXthread == INVALID_HANDLE_VALUE ? MF_ENABLED : MF_DISABLED);
	HMENU hMenuFile = GetSubMenu(mainmenu, 0);
	HMENU hMenuRx = GetSubMenu(mainmenu, 1);
	EnableMenuItem(mainmenu, ID_RX_FILE,         MF_BYCOMMAND | state);
	EnableMenuItem(hMenuFile, 1, MF_BYPOSITION | state); // popup including ID_RX_FILE_HIST0 - ID_RX_FILE_HIST9
	EnableMenuItem(hMenuRx,   2, MF_BYPOSITION | state); // popup including ID_RX_MODE_ANALYZE and ID_RX_MODE_PLSANLZ
	EnableMenuItem(hMenuRx,   3, MF_BYPOSITION | state); // popup including ID_RX_DUMP_SIGS and ID_RX_DUMP_STREAM
	EnableMenuItem(mainmenu, ID_RX_PROT,         MF_BYCOMMAND | state);
	EnableMenuItem(mainmenu, ID_RX_FLEXPROT,     MF_BYCOMMAND | state);
	EnableMenuItem(mainmenu, ID_RX_OUT,          MF_BYCOMMAND | state);
	EnableMenuItem(mainmenu, ID_RX_DSP_BITS,     MF_BYCOMMAND | state);

	// En/disable GUI elements
	if (ctllist) {
		// disable conflicting configurations while RX is running
		ctllist->enable(IDC_MAIN_RX_SDR_BTN, hRXthread == INVALID_HANDLE_VALUE);
		ctllist->enable(IDC_MAIN_RX_FREQ_BTN, hRXthread == INVALID_HANDLE_VALUE);

		// disable start button unless a rtl-sdr query string is configured
		ctllist->enable(IDC_MAIN_RX_STARTSTOP, rx->cfg->dev_query[0]!=0);
	}

	// Update 'unknown signals' checkbox
	CheckDlgButton(hwndMainDlg, IDC_MAIN_RX_CHKUNK, (rx->cfg->report_unknown ? BST_CHECKED : BST_UNCHECKED));
}

static void UpdateFreqCaption() {
	char nice_freq[20];
	unsigned long freq = (rx && rx->cfg->frequencies > 0 ? rx->cfg->frequency[0] : DEFAULT_FREQUENCY);
	if (freq >= 1E9)		snprintf(nice_freq, sizeof(nice_freq), "%.3fGHz", (double)freq / 1E9);
	else if (freq >= 1E6)	snprintf(nice_freq, sizeof(nice_freq), "%.3fMHz", (double)freq / 1E6);
	else if (freq >= 1E3)	snprintf(nice_freq, sizeof(nice_freq), "%.3fkHz", (double)freq / 1E3);
	else					snprintf(nice_freq, sizeof(nice_freq), "%f", (double)freq);
	char freqset[60];
	if (rx && rx->cfg->frequencies > 1)	sprintf_s(freqset, sizeof(freqset), "%s and %lu more", nice_freq, rx->cfg->frequencies - 1);
	else								sprintf_s(freqset, sizeof(freqset), "%s", nice_freq);
	SetDlgItemText(hwndMainDlg, IDC_MAIN_RX_FREQ_DSP, freqset);
}

static void GuiNotificationHandler(data_ext_t *datext) {
	char *s_time = NULL;
	char *s_type = NULL;
	char *s_model = NULL;
	int s_prot = 0;

	data_t *d = &datext->data;
	while (d) {
		if (!strcmp(d->key, "time") && d->type == DATA_STRING)
			s_time = (char*)d->value;
		else if (!strcmp(d->key, "protocol") && d->type == DATA_INT)
			s_prot = *((int*)d->value);
		else if (!strcmp(d->key, "model") && d->type == DATA_STRING)
			s_model = (char*)d->value;
		else if (!strcmp(d->key, "type") && d->type == DATA_STRING)
			s_type = (char*)d->value;
		d = d->next;
	}

	if (!s_time || !(*s_time)) return;
	if (!(datext->ext.mod >= OOK_DEMOD_MIN_VAL && datext->ext.mod <= OOK_DEMOD_MAX_VAL) && !(datext->ext.mod >= FSK_DEMOD_MIN_VAL && datext->ext.mod <= FSK_DEMOD_MAX_VAL) && !(datext->ext.mod >= UNKNOWN_OOK && datext->ext.mod <= UNKNOWN_FSK)) return;

	rx_entry *ge = new rx_entry();
	ge->setDevId(s_prot);
	ge->setTime(s_time);
	if (s_type) ge->setType(s_type);
	if (s_model) ge->setModel(s_model);
	ge->setModulation(((datext->ext.mod >= FSK_DEMOD_MIN_VAL && datext->ext.mod <= FSK_DEMOD_MAX_VAL) || datext->ext.mod == UNKNOWN_FSK) ? SIGNAL_MODULATION_FSK : SIGNAL_MODULATION_OOK);
	if (datext->ext.pulses/* && datext->ext.samprate*/) {
		if (!ge->copyPulses(datext->ext.pulses, datext->ext.pulseexc_startidx, datext->ext.pulseexc_len)) Gui_fprintf(stderr, "Internal error: pulses could not be copied.\n");
	}
	// copy data list (actually, it only copies the pointer and increases the usage counter)
	if (!ge->copyData(&datext->data)) Gui_fprintf(stderr, "Internal error: data could not be copied.\n");

	// Add item to RX listview
	lv_rx->AddRxEntry(ge);
	RefreshRxListInfo();
}

static DWORD WINAPI RX_Start(LPVOID param) {
	if (rx) {
		BOOL from_file = (rx->cfg->in_files.len > 0);
		if (!from_file) {
			lv_rx->setActiveStyle(TRUE);
			Gui_fprintf(stderr, "Started RX via SDR.\n");
		}
		start(rx, NULL);
		if (!from_file) {
			lv_rx->setActiveStyle(FALSE);
			Gui_fprintf(stderr, "Stopped RX via SDR.\n");
		}
		// after start will return, rx->cfg will basically remain and is reusable. Options which we only intended to be used once we reset here:
		if (rx->cfg->test_data[0]) memset(rx->cfg->test_data, 0, sizeof(rx->cfg->test_data)); // clear test data
		if (rx->cfg->in_files.len) clear_infiles(rx->cfg); // clear input file(s) because on next start it would force rtl_433 into file mode again.
	}
	// Close Thread
	SetDlgItemText(hwndMainDlg, IDC_MAIN_RX_STARTSTOP, "Start rtl_433");
	hRXthread = INVALID_HANDLE_VALUE;
	RefreshRxCfgElements(); // this will re-enable menu items like ID_RX_FILE
	return 1;
}

static BOOL RtlQuickLoad() {
	// Quick-Load only if there's exactly 1 RTL-SDR
	if (!rx || rtlsdr_get_device_count() != 1) return FALSE;

	// Get display name to update window element
	char vendor[256] = "n/a";
	char product[256] = "n/a";
	char serial[256] = "n/a";
	if (!rtlsdr_get_device_usb_strings(0, vendor, product, serial)) {
		int cap = (int) (strlen(vendor) + strlen(product) + strlen(serial) + 20);
		char *dspname = (char*)malloc(cap);
		if (dspname) {
			sprintf_s(dspname, cap, "%s %s, SN: %s", vendor, product, serial);
			SetDlgItemText(hwndMainDlg, IDC_MAIN_RX_SDR_DSP, dspname);
			free(dspname);
		}
	}

	// configure this device
	strcpy_s(rx->cfg->dev_query, sizeof(rx->cfg->dev_query), "0");
	return TRUE;
}

static VOID InitSlider_ScopeZoom() {
	HWND hSlid = (ctllist?ctllist->getHandle(IDC_MAIN_RX_SCOPE_ZOOM):NULL);
	if (hSlid) {
		SendMessage(hSlid, TBM_SETRANGEMIN, false, SCOPE_ZOOM_MIN);
		SendMessage(hSlid, TBM_SETRANGEMAX, true, SCOPE_ZOOM_MAX);
		SendMessage(hSlid, TBM_SETLINESIZE, 0, 1);  // 0.01
		SendMessage(hSlid, TBM_SETPAGESIZE, 0, 10); // 0.1
		SendMessage(hSlid, TBM_SETPOS, true, (int)(scw->getZoom()*100.0));
	}

}

static VOID InitSlider_ScopeOffset() {
	HWND hSlid = (ctllist ? ctllist->getHandle(IDC_MAIN_RX_SCOPE_OFFSET) : NULL);
	if (hSlid) {
		SendMessage(hSlid, TBM_SETRANGEMIN, false, SCOPE_OFFSET_MIN);
		SendMessage(hSlid, TBM_SETRANGEMAX, true, SCOPE_OFFSET_MAX);
		SendMessage(hSlid, TBM_SETLINESIZE, 0, 1);  // 0.001
		SendMessage(hSlid, TBM_SETPAGESIZE, 0, 10); // 0.01
		SendMessage(hSlid, TBM_SETPOS, true, (int)(scw->getOffset()*1000.0));
	}
}

static VOID SaveRxPulse(rx_entry *entry, BOOL entirepulse = FALSE) {
	sigmod mod = entry->getModulation();
	if (mod != SIGNAL_MODULATION_OOK && mod != SIGNAL_MODULATION_FSK) {
		Gui_fprintf(stderr, "Unknown modulation. Abort saving...\n");
		return;
	}

	char fpath[MAX_PATH] = "";
	OPENFILENAME ofntemp = { sizeof(OPENFILENAME), hwndMainDlg, 0, "OOK/FSK pulse data (text) (*.ook)\0*.ook\0VCD logic (text) (*.vcd)\0*.vcd\0\0", 0, 0, 0, (char*)fpath, sizeof(fpath), 0,0,0, "Save file as:", OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_READONLY | OFN_HIDEREADONLY, 0, 0, "ook", 0, 0, 0 };
	if (!GetSaveFileName(&ofntemp)) return;
	LPSTR ext = &fpath[ofntemp.nFileExtension];

	FILE *file = NULL;
	file = fopen(fpath, "wb");
	if (!file) {
		Gui_fprintf(stderr, "Failed to open output file\n");
		return;
	}

	PulseDatCompact *src = entry->getPulses();
	if (src) {
		// Temporarily build an uncompressed pulse_data_t object
		pulse_data_t *trg = (pulse_data_t *)calloc(1, sizeof(pulse_data_t));
		if (trg) {
			// Fill (required) elements of the pulse_data_t object
			trg->sample_rate = src->sample_rate;
			unsigned int s = (entirepulse || !src->segment_len ? 0 : src->segment_startidx);
			unsigned int e = (entirepulse || !src->segment_len ? src->num_pulses : min(src->segment_startidx + src->segment_len, src->num_pulses));
			trg->num_pulses = min(e - s, (sizeof(trg->pulse) / sizeof(int)));
			memcpy(trg->pulse, src->pulse, trg->num_pulses*sizeof(int));
			memcpy(trg->gap, src->gap, trg->num_pulses*sizeof(int));
			trg->freq1_hz = (float)src->freq1_hz;
			trg->freq2_hz = (float)src->freq2_hz;
			trg->fsk_f2_est = (mod == SIGNAL_MODULATION_FSK ? 1 : 0); // 1 is just used as exemplary non-null value to let librtl_433 recognize FSK
			trg->rssi_db = src->rssi_db;
			trg->snr_db = src->snr_db;
			trg->noise_db = src->noise_db;

			// Save this pulse_data_t object to output file in requested format
			if (!_stricmp(ext, "ook")) {
				pulse_data_print_pulse_header(file);
				pulse_data_dump(file, trg);
			}
			else if (!_stricmp(ext, "vcd")) {
				pulse_data_print_vcd_header(file, src->sample_rate);
				pulse_data_print_vcd(file, trg, '\'');
			}
			else {
				Gui_fprintf(stderr, "Unknown output format\n");
			}
			free(trg);
		}
	}
	fclose(file);
}

static BOOL SaveToFile(LPSTR txt, HWND hParent, LPSTR filter, LPSTR defext) {
	BOOL result = FALSE;
	if (txt) {
		char fpath[MAX_PATH] = "";
		OPENFILENAME ofntemp = { sizeof(OPENFILENAME), hParent, 0, filter, 0, 0, 0, (char*)fpath, sizeof(fpath), 0,0,0, "Save file as:", OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_READONLY | OFN_HIDEREADONLY, 0, 0, defext, 0, 0, 0 };
		if (GetSaveFileName(&ofntemp)) {
			HANDLE fhandle = CreateFile(fpath, GENERIC_READ + GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
			if (fhandle != INVALID_HANDLE_VALUE) {
				DWORD bytes_written;
				WriteFile(fhandle, txt, (DWORD) strlen(txt), &bytes_written, 0);
				CloseHandle(fhandle);
				result = TRUE;
			}
		}
	}
	return result;
}

BOOL RxGui_passFile(LPSTR path) {
	if (!rx) return FALSE;
	if (hRXthread != INVALID_HANDLE_VALUE) return FALSE; // Do nothing if RX is (still) active (however, in such cases the users should not be able to reach here anyways)

	add_infile(rx->cfg, strdup(path));
	SetDlgItemText(hwndMainDlg, IDC_MAIN_RX_STARTSTOP, "Stop rtl_433");
	hRXthread = CreateThread(0, 0, &RX_Start, 0, 0, 0);
	RefreshRxCfgElements(); // disables ID_RX_FILE menu item while librtl_433 is active
	return TRUE;
}

VOID RxGui_onHorScroll(HWND hElem) {
	HWND hSlidOffs = (ctllist ? ctllist->getHandle(IDC_MAIN_RX_SCOPE_OFFSET):NULL);
	HWND hSlidZoom = (ctllist ? ctllist->getHandle(IDC_MAIN_RX_SCOPE_ZOOM) : NULL);

	if (hElem == hSlidOffs) {
		INT pos = (INT)SendMessage(hSlidOffs, TBM_GETPOS, 0, 0);
		if (pos >= SCOPE_OFFSET_MIN && pos <= SCOPE_OFFSET_MAX && scw->getOffset() != ((double)pos / 1000.0)) {
			scw->setOffset((double)pos / 1000.0);
			RefreshScope();
		}
	}
	else if (hElem == hSlidZoom) {
		INT pos = (INT)SendMessage(hSlidZoom, TBM_GETPOS, 0, 0);
		if (pos >= SCOPE_ZOOM_MIN && pos <= SCOPE_ZOOM_MAX && pos != (int)(100.0*scw->getZoom())) {
			scw->setZoom((double)pos / 100.0);
			RefreshScope();
		}
	}
}

BOOL RxGui_onKey(LPNMLVKEYDOWN ctx) {
	if (!ctx) return FALSE;
	if (lv_rx && ctx->hdr.hwndFrom == lv_rx->getWndHandle()) {
		if (ctx->wVKey == VK_DELETE) {
			int sel_id = lv_rx->GetSelectedItem();
			lv_rx->RemoveRxEntry(sel_id);
			RefreshRxListInfo();
			return TRUE;
		}
	}
	return FALSE;
}

BOOL RxGui_onRightClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	if (lv_rx && ctx_in->hwndFrom == lv_rx->getWndHandle()) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;

		INT selidx;
		DWORD cmd = lv_rx->getRightClickCommand(ctx, &selidx);
		switch (cmd) {
		case ID_RXLIST_DEL:
			lv_rx->RemoveRxEntry(selidx);
			RefreshRxListInfo();
			break;
		case ID_RXLIST_DELALL:
			lv_rx->Clear();
			RefreshRxListInfo();
			break;
		case ID_RXLIST_SAVESIG:
		case ID_RXLIST_SAVEPLS: {
			rx_entry *e = lv_rx->GetRxEntry(selidx);
			if (e) SaveRxPulse(e, (cmd == ID_RXLIST_SAVEPLS));
			break;
		}
		case ID_RXLIST_SIGTX:
		case ID_RXLIST_PLSTX: {
			rx_entry *e = lv_rx->GetRxEntry(selidx);
			QueueRxPulseToTx(e, (cmd == ID_RXLIST_PLSTX));
			break;
		}
		default:
			break;
		}
		return TRUE;
	}
	else if (lv_rxdet && ctx_in->hwndFrom == lv_rxdet->getWndHandle()) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;

		INT selidx;
		DWORD cmd = lv_rxdet->getRightClickCommand(ctx, &selidx);
		switch (cmd) {
		case ID_RDLIST_COPY:
			lv_rxdet->toString(selidx, copybuf, sizeof(copybuf));
			CopyToClipboard(copybuf);
			break;
		case ID_RDLIST_COPYALL:
			lv_rxdet->toString(-1, copybuf, sizeof(copybuf));
			CopyToClipboard(copybuf);
			break;
		case ID_RDLIST_SAVEALL:
			lv_rxdet->toString(-1, copybuf, sizeof(copybuf));
			SaveToFile(copybuf, hwndMainDlg, "Text files (*.txt)\0*.txt\0\0", "txt");
			break;
		default:
			break;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL RxGui_onDoubleClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	// if (lv_rx && ctx_in->hwndFrom == lv_rx->getWndHandle()) {
	// LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;
	// ...
	// return TRUE;
	//}
	return FALSE;
}

BOOL RxGui_onChangedItem(LPNMLISTVIEW ctx) {
	if (!ctx) return FALSE;
	if (lv_rx && ctx->hdr.hwndFrom == lv_rx->getWndHandle()) { // Iconstates geändert (also auch evtl. Selections -> rx info aktualisieren)
		if (ctx->uChanged & LVIF_STATE) {
			unsigned long states_changed = (ctx->uOldState ^ ctx->uNewState);
			if ((states_changed & LVIS_SELECTED)) {
				RefreshRxListInfo();
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL RxGui_onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_MAIN_RX_STARTSTOP:
		if (!rx) break;
		if (hRXthread == INVALID_HANDLE_VALUE) {
			SetDlgItemText(hwndMainDlg, IDC_MAIN_RX_STARTSTOP, "Stop rtl_433");
			hRXthread = CreateThread(0, 0, &RX_Start, 0, 0, 0);
			RefreshRxCfgElements(); // disables ID_RX_FILE menu item while RX is active
		}
		else {
			stop_signal(rx);
		}
		break;

	case IDC_MAIN_RX_SDR_BTN: {
		if (!rx) break;
		char *devname = NULL;
		if (ShowDeviceRxDialog(rx, hwndMainDlg, &devname) > 0) {
			SetDlgItemText(hwndMainDlg, IDC_MAIN_RX_SDR_DSP, (devname && *devname ? devname : ""));
			RefreshRxCfgElements();
		}
		break;
	}

	case ID_RX_DSP_UNKNOWN:
	case IDC_MAIN_RX_CHKUNK:
		if (!rx) break;
		rx->cfg->report_unknown = (rx->cfg->report_unknown?0:1);
		RefreshRxCfgElements();
		break;

	case ID_RX_PROT:
		if (!rx) break;
		ShowProtocolDialog(rx, hwndMainDlg, popup_rxprot);
		break;

	case ID_RX_FLEXPROT:
		if (!rx) break;
		ShowFlexDialog(rx, hwndMainDlg, popup_rxflex);
		break;

	case ID_RX_MODE_ANALYZE: //-a
		if (!rx) break;
		rx->cfg->analyze_am = (rx->cfg->analyze_am ? 0 : 1);
		rx->cfg->analyze_pulses = 0;
		RefreshRxCfgElements();
		break;

	case ID_RX_MODE_PLSANLZ: // -A
		if (!rx) break;
		rx->cfg->analyze_pulses = (rx->cfg->analyze_pulses ? 0 : 1);
		rx->cfg->analyze_am = 0;
		RefreshRxCfgElements();
		break;

	case ID_RX_DUMP_SIGS: // -S
		if (!rx) break;
		if (ShowSignalGrabberDialog(rx, hwndMainDlg) > 0) {
			RefreshRxCfgElements();
		}
		break;

	case ID_RX_DUMP_STREAM: // -w / -W
		if (!rx) break;
		if (ShowStreamGrabberDialog(rx, hwndMainDlg) > 0) {
			RefreshRxCfgElements();
		}
		break;

	case IDC_MAIN_RX_FREQ_BTN:
		if (!rx) break;
		ShowFrequencyRxDialog(rx, hwndMainDlg, popup_rxfreq);
		UpdateFreqCaption();
		break;

	case ID_RX_OUT:
		if (!rx) break;
		ShowOutputDialog(rx, hwndMainDlg);
		break;

	case ID_RX_EXP:
		if (!rx) break;
		ShowExpertDialog(rx, hwndMainDlg);
		break;

	case ID_RX_DSP_HIRES:
		if (!rx) break;
		rx->cfg->report_time_hires = (rx->cfg->report_time_hires ? 0 : 1);
		RefreshRxCfgElements();
		break;

	case ID_RX_DSP_META:
		rx->cfg->report_meta = (rx->cfg->report_meta ? 0 : 1);
		RefreshRxCfgElements();
		break;

	case ID_RX_DSP_BITS:
		rx->cfg->verbose_bits = (rx->cfg->verbose_bits ? 0 : 1);
		RefreshRxCfgElements();
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

VOID RxGui_onSizeChange(int add_w, int add_h, double fac_w, double fac_h, BOOL redraw) {
	if (!ctllist) return;

	WndElem *e = ctllist->elems;
	while (e) {
		switch (e->ctlid) {
			// everything completely above the RX listview just scales horizontally
			case IDC_MAIN_RX_SDR_DSP:
			case IDC_MAIN_RX_SDR_BTN:
			case IDC_MAIN_RX_FREQ_LBL:
			case IDC_MAIN_RX_FREQ_DSP:
			case IDC_MAIN_RX_FREQ_BTN:
			case IDC_MAIN_RX_STARTSTOP:
			case IDC_MAIN_RX_COUNTER:
			case IDC_MAIN_RX_CHKUNK:
			case IDC_MAIN_RX_STAT02:
				MoveWindow(e->hWnd, (int)(fac_w*e->initrect.left), e->initrect.top, (int)(fac_w*(e->initrect.right - e->initrect.left)), e->initrect.bottom - e->initrect.top, redraw);
				break;

			// everything completely below the RX listview scales horizontally with a vertical shift
			case IDC_MAIN_RX_STAT03:
			case IDC_MAIN_RX_DETLIST:
			case IDC_MAIN_RX_SCOPE:
			case IDC_MAIN_RX_SCOPE_LBLZ:
			case IDC_MAIN_RX_SCOPE_LBLP:
			case IDC_MAIN_RX_SCOPE_ZOOM:
			case IDC_MAIN_RX_SCOPE_OFFSET:
				MoveWindow(e->hWnd, (int)(fac_w*e->initrect.left), e->initrect.top + add_h, (int)(fac_w*(e->initrect.right - e->initrect.left)), e->initrect.bottom - e->initrect.top, redraw);
				if (e->ctlid == IDC_MAIN_RX_SCOPE && scw) scw->resizeBox(0);
				break;

			// The RX listview and surrounding objects scale horizontally with a vertical enlargement
			case IDC_MAIN_RX_STAT01:
			case IDC_MAIN_RX_LIST:
				MoveWindow(e->hWnd, (int)(fac_w*e->initrect.left), e->initrect.top, (int)(fac_w*(e->initrect.right - e->initrect.left)), e->initrect.bottom - e->initrect.top + add_h, redraw);
				break;

			default:
				break;
		}

		e = e->next;
	}

}

VOID RxGui_onInit(HWND hMainWnd, HMENU hMainMenu, HMENU poprx, HMENU popdet, HMENU popfreq, HMENU popflex, HMENU popprot) {
	hwndMainDlg = hMainWnd;
	mainmenu = hMainMenu;
	popup_rxlist = poprx;
	popup_details = popdet;
	popup_rxfreq = popfreq;
	popup_rxflex = popflex;
	popup_rxprot = popprot;

	ctllist = new WndElems(hwndMainDlg);

	HWND hRxList = NULL;
	HWND hRxDet = NULL;
	HWND hRxScp = NULL;

	if (ctllist) {
		ctllist->add(IDC_MAIN_RX_SDR_DSP);
		ctllist->add(IDC_MAIN_RX_SDR_BTN);
		ctllist->add(IDC_MAIN_RX_FREQ_LBL);
		ctllist->add(IDC_MAIN_RX_FREQ_DSP);
		ctllist->add(IDC_MAIN_RX_FREQ_BTN);
		ctllist->add(IDC_MAIN_RX_STARTSTOP);
		ctllist->add(IDC_MAIN_RX_COUNTER);
		ctllist->add(IDC_MAIN_RX_CHKUNK);
		hRxList = ctllist->add(IDC_MAIN_RX_LIST);
		hRxDet = ctllist->add(IDC_MAIN_RX_DETLIST);
		hRxScp = ctllist->add(IDC_MAIN_RX_SCOPE);
		ctllist->add(IDC_MAIN_RX_SCOPE_LBLZ);
		ctllist->add(IDC_MAIN_RX_SCOPE_LBLP);
		ctllist->add(IDC_MAIN_RX_SCOPE_ZOOM);
		ctllist->add(IDC_MAIN_RX_SCOPE_OFFSET);
		ctllist->add(IDC_MAIN_RX_STAT01);
		ctllist->add(IDC_MAIN_RX_STAT02);
		ctllist->add(IDC_MAIN_RX_STAT03);
	}

	// Init RX
	rtl_433_init(&rx);
	if (rx >= 0) {
		rx->cfg->new_model_keys = 1; // override current default

		// Load settings (command line and/or config file)
		if (configure_librtl433(rx->cfg, __argc, __argv, 1) != CFG_SUCCESS_GO_ON) {
			MessageBox(hwndMainDlg, "Command line processing threw some text output (see log), possibly not all options have been parsed.", "Information", MB_OK | MB_ICONINFORMATION);
		} // todo: load important librtl_433 settings from registry (if command line was unused)

		// Enable callback
		rx->cfg->outputs_configured |= OUTPUT_EXT;
		rx->cfg->output_extcallback = (void*)GuiNotificationHandler;
		rx->cfg->report_protocol = 1;
		rx->cfg->report_unknown = 1;

		// Pre-select an RTL-SDR if we find exactly one:
		RtlQuickLoad();
	}
	else Gui_fprintf(stderr, "Error: Could not create librtl_433 instance.\n");
	if(hRxList) lv_rx = new rxlist(hRxList, hwndMainDlg, popup_rxlist);
	if(hRxDet) lv_rxdet = new rxdetails(hRxDet, hwndMainDlg, popup_details);
	if(hRxScp) scw = new ScopeWnd(hRxScp);
	InitSlider_ScopeZoom();
	InitSlider_ScopeOffset();

	RefreshRxCfgElements();
	UpdateFreqCaption();
}

BOOL RxGui_prepare() {
	// ensure that there is not more 1 instance of this GUI at this point in time
	if (ctllist || lv_rx || lv_rxdet || scw || rx) return FALSE;
	return TRUE;
}

BOOL RxGui_shutdownRequest() {
	if (hRXthread != INVALID_HANDLE_VALUE) {
		if (MessageBox(hwndMainDlg, "RX (reception) is still active. Cancel it? ('no' returns to program)", "rtl_fl2k_433", MB_YESNO | MB_ICONQUESTION) != IDYES) {
			return FALSE;
		}
	}
	return TRUE;
}

VOID RxGui_shutdownConfirm() {
	if (rx) stop_signal(rx);
}

BOOL RxGui_isActive(){
	return (hRXthread != INVALID_HANDLE_VALUE);
}


VOID RxGui_cleanup() {
	// RxGui_saveSettings(); // still unused cause of librtl_433s own loading features (command line / config file)

	if (lv_rx) {
		delete lv_rx;
		lv_rx = NULL;
	}
	if (lv_rxdet) {
		delete lv_rxdet;
		lv_rxdet = NULL;
	}
	if (scw) {
		delete scw;
		scw = NULL;
	}
	if (rx) {
		rtl_433_destroy(rx);
		rx = NULL;
	}
	if (ctllist) {
		delete ctllist;
		ctllist = NULL;
	}
}

/* still unused cause of librtl_433s own loading features (command line / config file)

VOID RxGui_loadSettings() {
}

VOID RxGui_saveSettings() {
} */