/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *        Option sub-dialog for TX frequency configuration         *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowFrequencyTxDialog():   *
 * - updates fl2k->cfg.<carrier | samp_rate>                       *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include <Commctrl.h>
#include <stdio.h>

#include "libfl2k_433.h"
#include "wnd_txopt_freq.h"
#include "logwrap.h"

#include "../res/gui_win_resources.h"

static fl2k_433_t		*fl2k = NULL;

static HWND		hDlg, hSlSrate, hSlMult, hSlDiv, hSlFrac, hSlCarr, hDspMult, hDspDiv, hDspFrac, hFreqList, hWarn;

static int tmp_mult, tmp_div, tmp_frac;
static uint32_t tmp_carr;

BOOL manmode;

pFl2kCfg fl2kcfgs = NULL;
uint32_t fl2kcfgs_n = 0;

pFl2kCfg fl2kcfgs_rest = NULL;
uint32_t fl2kcfgs_rest_n = 0;

VOID FreqList_Init(){
	LVCOLUMN tmp_lvcolumn;
	tmp_lvcolumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	tmp_lvcolumn.pszText = "Harmonic";
	tmp_lvcolumn.fmt = LVCFMT_LEFT;
	tmp_lvcolumn.cx = 110;
	ListView_InsertColumn(hFreqList, 0, &tmp_lvcolumn);
	tmp_lvcolumn.pszText = "Center";
	tmp_lvcolumn.cx = 90;
	ListView_InsertColumn(hFreqList, 1, &tmp_lvcolumn);
	tmp_lvcolumn.pszText = "Signal low";
	tmp_lvcolumn.cx = 90;
	ListView_InsertColumn(hFreqList, 2, &tmp_lvcolumn);
	tmp_lvcolumn.pszText = "Signal high";
	tmp_lvcolumn.cx = 90;
	ListView_InsertColumn(hFreqList, 3, &tmp_lvcolumn);

	LVITEM tmp_lvitem;
	tmp_lvitem.iSubItem = 0;
	tmp_lvitem.mask = LVIF_TEXT;
	char tmp[20] = "Base frequency";
	for (int a = 1; a <= MAX_DSP_HARMONIC; a++) {
		if (a > 1) {
			char *pf = (a == 2 ? "nd" : (a == 3 ? "rd" : "th"));
			sprintf_s(tmp, sizeof(tmp), "%lu%s harmonic", a, pf);
		}
		tmp_lvitem.iItem = a-1;
		tmp_lvitem.pszText = tmp;
		ListView_InsertItem(hFreqList, &tmp_lvitem);
	}
}

VOID FreqList_Update(uint32_t samprate, uint32_t carrier) {
	char tmp[20] = "";
	for (int a = 1; a <= MAX_DSP_HARMONIC; a++) {
		double c  =  ((double)a * (double)samprate) / 1000000.0;
		double fl = (((double)a * (double)samprate) - (double)carrier) / 1000000.0;
		double fh = (((double)a * (double)samprate) + (double)carrier) / 1000000.0;
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", c);
		ListView_SetItemText(hFreqList, a-1, 1, tmp);
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", fl);
		ListView_SetItemText(hFreqList, a-1, 2, tmp);
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", fh);
		ListView_SetItemText(hFreqList, a-1, 3, tmp);
	}
}

VOID tmpSettingsToAutoSlider() {
	int idx = -1;
	for (uint32_t a = 0; idx < 0 && a < fl2kcfgs_n; a++) {
		if (fl2kcfgs[a].mult != tmp_mult) continue;
		if (fl2kcfgs[a].div != tmp_div) continue;
		if (fl2kcfgs[a].frac != tmp_frac) continue;
		idx = a;
	}
	if (idx < 0) {
		// Combination not found. Should be a redundant one, so search for the sample rate there
		int ridx = -1;
		for (uint32_t a = 0; ridx < 0 && a < fl2kcfgs_rest_n; a++) {
			if (fl2kcfgs_rest[a].mult != tmp_mult) continue;
			if (fl2kcfgs_rest[a].div != tmp_div) continue;
			if (fl2kcfgs_rest[a].frac != tmp_frac) continue;
			ridx = a;
		}
		if (ridx < 0) {
			Gui_fprintf(stderr, "tmpSettingsToAutoSlider(): Unexpected condition 1.\n");
			return; // should never happen
		}
		for (uint32_t a = 0; idx < 0 && a < fl2kcfgs_n; a++) {
			if (fl2kcfgs[a].sample_clock != fl2kcfgs_rest[ridx].sample_clock) continue;
			idx = a;
		}
		if (ridx < 0) {
			Gui_fprintf(stderr, "tmpSettingsToAutoSlider(): Unexpected condition 2.\n");
			return; // should never happen
		}
	}

	SendMessage(hSlSrate, TBM_SETPOS, true, idx);
}

void tmpSettingsToManualSliders() {
	SendMessage(hSlMult, TBM_SETPOS, true, tmp_mult);
	SendMessage(hSlDiv, TBM_SETPOS, true, tmp_div);
	SendMessage(hSlFrac, TBM_SETPOS, true, tmp_frac);
}

VOID InitConfAuto() {
	SendMessage(hSlSrate, TBM_SETRANGEMIN, false, 0);
	SendMessage(hSlSrate, TBM_SETRANGEMAX, false, fl2kcfgs_n-1);
	SendMessage(hSlSrate, TBM_SETLINESIZE, 0, 1);
	SendMessage(hSlSrate, TBM_SETPAGESIZE, 0, 20);
}

VOID InitConfMan() {
	SendMessage(hSlMult, TBM_SETRANGEMIN, false, 3);
	SendMessage(hSlMult, TBM_SETRANGEMAX, false, 6);
	SendMessage(hSlMult, TBM_SETLINESIZE, 0, 1);
	SendMessage(hSlMult, TBM_SETPAGESIZE, 0, 1);

	SendMessage(hSlDiv, TBM_SETRANGEMIN, false, 2);
	SendMessage(hSlDiv, TBM_SETRANGEMAX, false, 63);
	SendMessage(hSlDiv, TBM_SETLINESIZE, 0, 1);
	SendMessage(hSlDiv, TBM_SETPAGESIZE, 0, 5);

	SendMessage(hSlFrac, TBM_SETRANGEMIN, false, 1);
	SendMessage(hSlFrac, TBM_SETRANGEMAX, false, 15);
	SendMessage(hSlFrac, TBM_SETLINESIZE, 0, 1);
	SendMessage(hSlFrac, TBM_SETPAGESIZE, 0, 3);
}

VOID InitSlider_Carr(uint32_t max_freq) {
	SendMessage(hSlCarr, TBM_SETRANGEMIN, false, (100000 / CARRIER_RESOLUTION)); // discuss: is this OK as minumum carrier value? or should we choose factor 2?5?10? of the OOK signal frequency?
	SendMessage(hSlCarr, TBM_SETRANGEMAX, true, (max_freq / CARRIER_RESOLUTION));
	SendMessage(hSlCarr, TBM_SETLINESIZE, 0, 1);
	SendMessage(hSlCarr, TBM_SETPAGESIZE, 0, 100);
}

static uint32_t getSampleRate(int mult, int div, int frac, BOOL *hint) {
	// Search for the sample rate in the regular FL2K configs
	if (hint) *hint = FALSE;
	for (uint32_t a = 0; a < fl2kcfgs_n; a++) {
		if (fl2kcfgs[a].mult != mult) continue;
		if (fl2kcfgs[a].div != div) continue;
		if (fl2kcfgs[a].frac != frac) continue;
		return fl2kcfgs[a].sample_clock;
	}
	// Sample rate not found. Should be a redundant one, so search for the settings there
	if (hint) *hint = TRUE;
	for (uint32_t a = 0; a < fl2kcfgs_rest_n; a++) {
		if (fl2kcfgs_rest[a].mult != mult) continue;
		if (fl2kcfgs_rest[a].div != div) continue;
		if (fl2kcfgs_rest[a].frac != frac) continue;
		return fl2kcfgs_rest[a].sample_clock;
	}
	Gui_fprintf(stderr, "getSampleRate(): Unexpected condition 2.\n");
	return 0;
}

static void RefreshWndElements() {
	char tmp[1000];

	// Refresh current sample rate
	BOOL hint_redundantonly = FALSE;
	uint32_t samprate = getSampleRate(tmp_mult, tmp_div, tmp_frac, &hint_redundantonly);
	sprintf_s(tmp, sizeof(tmp), "%lu", samprate);
	SetDlgItemText(hDlg, IDC_TX_FREQ_SAMPRATE, tmp);
	sprintf_s(tmp, sizeof(tmp), "(%.02f MS/s)", ((double)samprate/1000000.0));
	SetDlgItemText(hDlg, IDC_TX_FREQ_SRATEDSP, tmp);

	// Refresh displayed (textual) values for manual settings
	sprintf_s(tmp, sizeof(tmp), "PLL multiplier: %lu", tmp_mult);
	SetDlgItemText(hDlg, IDC_TX_FREQ_MULT_DSP, tmp);

	sprintf_s(tmp, sizeof(tmp), "div: %lu", tmp_div);
	SetDlgItemText(hDlg, IDC_TX_FREQ_DIV_DSP, tmp);

	sprintf_s(tmp, sizeof(tmp), "frac: %lu", tmp_frac);
	SetDlgItemText(hDlg, IDC_TX_FREQ_FRAC_DSP, tmp);

	// Display warnings / information about current setting
	if (hint_redundantonly) {
		// find the config preferred by OSMO-FL2K
		pFl2kCfg ref = NULL;
		for (uint32_t a = 0; !ref && a < fl2kcfgs_n; a++) {
			if (fl2kcfgs[a].sample_clock == samprate) ref = &fl2kcfgs[a];
		}
		if (ref) sprintf_s(tmp, sizeof(tmp), "Info: for %lu samples/sec OSMO-FL2K will use alternate config [%lu;%lu;%lu].", samprate, ref->mult, ref->div, ref->frac);
		else sprintf_s(tmp, sizeof(tmp), "Info: for %lu samples/sec OSMO-FL2K will use alternate config [n/a].", samprate); // should not occur
		SetWindowText(hWarn, tmp);
	}
	else SetWindowText(hWarn, (samprate > 157000000 ? "Warning: This will not run stable on most USB3 host controllers." : ""));

	// Check carrier bounds against Nyquist and adjust carrier range, if necessary
	INT crnt_maxcarr = (INT) SendMessage(hSlCarr, TBM_GETRANGEMAX, 0, 0);
	INT new_maxcarr = samprate / (2*CARRIER_RESOLUTION);
	if (crnt_maxcarr != new_maxcarr) {
		InitSlider_Carr(new_maxcarr*CARRIER_RESOLUTION);
		INT pos = (INT)SendMessage(hSlCarr, TBM_GETPOS, 0, 0);
		//SendMessage(hSlCarr, TBM_SETPOS, 0, 0);
		tmp_carr = pos * CARRIER_RESOLUTION;
	}
	double samplesPerCycle = (double)samprate / (double)tmp_carr;

	// Refresh (textual) carrier frequency
	sprintf_s(tmp, sizeof(tmp), "%.02f MHz", ((double)tmp_carr / 1000000.0));
	SetDlgItemText(hDlg, IDC_TX_FREQ_CARR_DSP, tmp);

	// Refresh carrier info text
	sprintf_s(tmp, sizeof(tmp), "(%.1f sample points per sine wave at the current sample rate):", samplesPerCycle);
	SetDlgItemText(hDlg, IDC_TX_FREQ_CARR_STAT, tmp);

	// Adjust the sliders of the inactive mode to the current settings (which might have been changed by the active mode)
	if (manmode)	tmpSettingsToAutoSlider(); // In manual mode adjust the auto slider
	else			tmpSettingsToManualSliders(); // In auto mode adjust the manual sliders

	// En-/disable window elements regarding to the current mode
	
	EnableWindow(hSlSrate,	(manmode ? FALSE : TRUE));
	EnableWindow(hSlMult,	(manmode ? TRUE : FALSE));
	EnableWindow(hSlDiv,	(manmode ? TRUE : FALSE));
	EnableWindow(hSlFrac,	(manmode ? TRUE : FALSE));
	EnableWindow(hDspMult, (manmode ? TRUE : FALSE));
	EnableWindow(hDspDiv, (manmode ? TRUE : FALSE));
	EnableWindow(hDspFrac, (manmode ? TRUE : FALSE));

	// Refresh the info element about harmonic frequencies
	FreqList_Update(samprate, tmp_carr);

}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_TX_FREQ_RADAUTO:
	case IDC_TX_FREQ_RADMAN:
	{
		BOOL man_on = (IsDlgButtonChecked(hDlg, IDC_TX_FREQ_RADMAN) == BST_CHECKED);
		if (man_on != manmode) {
			manmode = man_on;
			RefreshWndElements();
		}
		break;
	}

	case IDC_TX_FREQ_OK: {
		fl2k->cfg.carrier = tmp_carr;
		BOOL redundant_cfg = FALSE;
		fl2k->cfg.samp_rate = getSampleRate(tmp_mult, tmp_div, tmp_frac, &redundant_cfg);
		if (redundant_cfg) {
			// treatment of alternate configurations for the same sample rate might be added here
			// but this would require an adaptation/extension of libosmo-fl2k
		}
		EndDialog(hDlg, TRUE);
		break;
	}

	case IDC_TX_FREQ_CANCEL:
		EndDialog(hDlg, TRUE);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static VOID onHorScroll(HWND hElem) {
	if (hElem == hSlSrate) {
		INT pos = (INT)SendMessage(hSlSrate, TBM_GETPOS, 0, 0);
		if (pos >= 0 && pos < (int)fl2kcfgs_n) {
			if (tmp_mult != fl2kcfgs[pos].mult || tmp_div != fl2kcfgs[pos].div || tmp_frac != fl2kcfgs[pos].frac) {
				tmp_mult = fl2kcfgs[pos].mult;
				tmp_div = fl2kcfgs[pos].div;
				tmp_frac = fl2kcfgs[pos].frac;
				RefreshWndElements(); // will also update the manual mode sliders
			}
		}
	}
	else if (hElem == hSlMult) {
		INT pos = (INT)SendMessage(hSlMult, TBM_GETPOS, 0, 0);
		if (pos >= FL2K_MULT_MIN && pos <= FL2K_MULT_MAX && tmp_mult != pos) {
			tmp_mult = pos;
			RefreshWndElements(); // will also update the auto mode slider
		}
	}
	else if (hElem == hSlDiv) {
		INT pos = (INT)SendMessage(hSlDiv, TBM_GETPOS, 0, 0);
		if (pos >= FL2K_DIV_MIN && pos <= FL2K_DIV_MAX && tmp_div != pos) {
			tmp_div = pos;
			RefreshWndElements(); // will also update the auto mode slider
		}
	}
	else if (hElem == hSlFrac) {
		INT pos = (INT)SendMessage(hSlFrac, TBM_GETPOS, 0, 0);
		if (pos >= FL2K_FRAC_MIN && pos <= FL2K_FRAC_MAX && tmp_frac != pos) {
			tmp_frac = pos;
			RefreshWndElements(); // will also update the auto mode slider
		}
	}
	else if (hElem == hSlCarr) {
		INT pos = (INT)SendMessage(hSlCarr, TBM_GETPOS, 0, 0);
		if (tmp_carr != (pos * CARRIER_RESOLUTION)) {
			tmp_carr = pos * CARRIER_RESOLUTION;
			RefreshWndElements();
		}
	}
}

static VOID onInit(HWND hwndDlg) {
	hDlg = hwndDlg;
	HICON hIcon = LoadIcon(GetModuleHandle(0), (const char *)IDI_ICON1);
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	hSlSrate = GetDlgItem(hDlg, IDC_TX_FREQ_SRATE_SL);
	hSlMult = GetDlgItem(hDlg, IDC_TX_FREQ_MULT_SL);
	hSlDiv = GetDlgItem(hDlg, IDC_TX_FREQ_DIV_SL);
	hSlFrac = GetDlgItem(hDlg, IDC_TX_FREQ_FRAC_SL);
	hDspMult = GetDlgItem(hDlg, IDC_TX_FREQ_MULT_DSP);
	hDspDiv = GetDlgItem(hDlg, IDC_TX_FREQ_DIV_DSP);
	hDspFrac = GetDlgItem(hDlg, IDC_TX_FREQ_FRAC_DSP);
	hSlCarr = GetDlgItem(hDlg, IDC_TX_FREQ_CARR_SL);
	hWarn = GetDlgItem(hDlg, IDC_TX_FREQ_WARNING);

	hFreqList = GetDlgItem(hDlg, IDC_TX_FREQ_HARM);

	InitSlider_Carr(2 * tmp_carr); // max value: just to get us started
	SendMessage(hSlCarr, TBM_SETPOS, true, tmp_carr / CARRIER_RESOLUTION);

	CheckRadioButton(hDlg, IDC_TX_FREQ_RADAUTO, IDC_TX_FREQ_RADMAN, (manmode ? IDC_TX_FREQ_RADMAN : IDC_TX_FREQ_RADAUTO));
	InitConfAuto();
	InitConfMan();
	// Set the sliders of the active mode to the current position
	if (manmode) tmpSettingsToManualSliders();
	else tmpSettingsToAutoSlider();

	// Init Freq List
	FreqList_Init();
	// Refresh window elements (will also refresh the sliders of the inactive mode)
	RefreshWndElements();
}

static INT_PTR CALLBACK DialogHandler(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

	case WM_HSCROLL:
		onHorScroll((HWND)lParam);
		break;

	case WM_COMMAND:
		onCommand(wParam, lParam);
		break;

	case WM_INITDIALOG:
		onInit(hwndDlg);
		break;

	case WM_CTLCOLORSTATIC: // highlight static control IDC_TX_FREQ_WARNING by using blue color
		if ((HWND)lParam != hWarn) return NULL;
		SetTextColor((HDC)wParam, RGB(0, 0, 255));
		SetBkColor((HDC)wParam, GetSysColor(COLOR_MENU));
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (INT_PTR)CreateSolidBrush(GetSysColor(COLOR_MENU));

	case WM_CLOSE:
		EndDialog(hDlg, TRUE);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

int ShowFrequencyTxDialog(fl2k_433_t *fl2k_obj, HWND hParent) {
	int r = -1;
	if (fl2k_obj) {
		fl2k = fl2k_obj;

		getCfgTables(&fl2kcfgs, &fl2kcfgs_n, &fl2kcfgs_rest, &fl2kcfgs_rest_n);

		if (fl2kcfgs && fl2kcfgs_n > 0) {
			// Search active Cfg
			pFl2kCfg cfg = NULL;
			for (uint32_t a = 0; !cfg && a < fl2kcfgs_n; a++) {
				if (fl2k->cfg.samp_rate == fl2kcfgs[a].sample_clock) cfg = &fl2kcfgs[a];
			}
			tmp_mult = (cfg ? cfg->mult:FL2K_DEFAULT_MULT);
			tmp_div = (cfg ? cfg->div : FL2K_DEFAULT_DIV);
			tmp_frac = (cfg ? cfg->frac : FL2K_DEFAULT_FRAC);
			tmp_carr = fl2k->cfg.carrier;
			manmode = FALSE;
			r = (int)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_TX_FREQ), hParent, (DLGPROC)DialogHandler);
		}

	}
	return r;
}
