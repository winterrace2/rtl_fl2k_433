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

static HWND		hDlg, hSlSrate, hSlMult, hSlDiv, hSlFrac, hSlCarr1, hSlCarr2, hDspMult, hDspDiv, hDspFrac, hFreqList, hWarn;

typedef struct _FreqSettings {
	int mult;
	int div;
	int frac;
	uint32_t carr1;
	uint32_t carr2;
} FreqSettings, *pFreqSettings;

static FreqSettings tempset;

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
	tmp_lvcolumn.cx = 100;
	ListView_InsertColumn(hFreqList, 0, &tmp_lvcolumn);
	tmp_lvcolumn.pszText = "Center";
	tmp_lvcolumn.cx = 80;
	ListView_InsertColumn(hFreqList, 1, &tmp_lvcolumn);
	tmp_lvcolumn.pszText = "-Primary";
	tmp_lvcolumn.cx = 80;
	ListView_InsertColumn(hFreqList, 2, &tmp_lvcolumn);
	tmp_lvcolumn.pszText = "+Primary";
	tmp_lvcolumn.cx = 80;
	ListView_InsertColumn(hFreqList, 3, &tmp_lvcolumn);
	tmp_lvcolumn.pszText = "-Secondary";
	tmp_lvcolumn.cx = 80;
	ListView_InsertColumn(hFreqList, 4, &tmp_lvcolumn);
	tmp_lvcolumn.pszText = "+Secondary";
	tmp_lvcolumn.cx = 80;
	ListView_InsertColumn(hFreqList, 5, &tmp_lvcolumn);

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

VOID FreqList_Update(uint32_t samprate, uint32_t carrier1, uint32_t carrier2) {
	char tmp[20] = "";
	for (int a = 1; a <= MAX_DSP_HARMONIC; a++) {
		double c  =  ((double)a * (double)samprate) / 1000000.0;
		double pl = (((double)a * (double)samprate) - (double)carrier1) / 1000000.0;
		double ph = (((double)a * (double)samprate) + (double)carrier1) / 1000000.0;
		double sl = (carrier2>0?(((double)a * (double)samprate) - (double)carrier2) / 1000000.0:0.0);
		double sh = (carrier2>0?(((double)a * (double)samprate) + (double)carrier2) / 1000000.0:0.0);
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", c);
		ListView_SetItemText(hFreqList, a-1, 1, tmp);
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", pl);
		ListView_SetItemText(hFreqList, a-1, 2, (carrier1?tmp:""));
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", ph);
		ListView_SetItemText(hFreqList, a-1, 3, (carrier1?tmp:""));
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", sl);
		ListView_SetItemText(hFreqList, a - 1, 4, (carrier2?tmp:""));
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", sh);
		ListView_SetItemText(hFreqList, a - 1, 5, (carrier2?tmp:""));
	}
}

VOID tmpSettingsToAutoSlider() {
	int idx = -1;
	for (uint32_t a = 0; idx < 0 && a < fl2kcfgs_n; a++) {
		if (fl2kcfgs[a].mult != tempset.mult) continue;
		if (fl2kcfgs[a].div != tempset.div) continue;
		if (fl2kcfgs[a].frac != tempset.frac) continue;
		idx = a;
	}
	if (idx < 0) {
		// Combination not found. Should be a redundant one, so search for the sample rate there
		int ridx = -1;
		for (uint32_t a = 0; ridx < 0 && a < fl2kcfgs_rest_n; a++) {
			if (fl2kcfgs_rest[a].mult != tempset.mult) continue;
			if (fl2kcfgs_rest[a].div != tempset.div) continue;
			if (fl2kcfgs_rest[a].frac != tempset.frac) continue;
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
	SendMessage(hSlMult, TBM_SETPOS, true, tempset.mult);
	SendMessage(hSlDiv, TBM_SETPOS, true, tempset.div);
	SendMessage(hSlFrac, TBM_SETPOS, true, tempset.frac);
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

VOID InitSlider_Carr(HWND hSlid, uint32_t max_freq) {
	SendMessage(hSlid, TBM_SETRANGEMIN, false, 0);
	SendMessage(hSlid, TBM_SETRANGEMAX, true, (max_freq / CARRIER_RESOLUTION));
	SendMessage(hSlid, TBM_SETLINESIZE, 0, 1);
	SendMessage(hSlid, TBM_SETPAGESIZE, 0, 100);
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

static uint32_t adjustCarrSlider(HWND hSlider, DWORD ctl_carr, DWORD ctl_stat, uint32_t samprate, uint32_t oldcarr) {
	char tmp[100];
	uint32_t newcarr = oldcarr;

	// Check carrier bounds against Nyquist and adjust carrier range, if necessary
	INT crnt_maxcarr = (INT)SendMessage(hSlider, TBM_GETRANGEMAX, 0, 0);
	INT new_maxcarr = samprate / (2 * CARRIER_RESOLUTION);
	if (crnt_maxcarr != new_maxcarr) {
		InitSlider_Carr(hSlider, new_maxcarr*CARRIER_RESOLUTION);
		INT pos = (INT)SendMessage(hSlider, TBM_GETPOS, 0, 0);
		//SendMessage(hSlider, TBM_SETPOS, 0, 0);
		newcarr = pos * CARRIER_RESOLUTION;
	}

	if (newcarr > 0) {
		double samplesPerCycle = (double)samprate / (double)newcarr;

		// Refresh (textual) carrier frequency
		sprintf_s(tmp, sizeof(tmp), "%.02f MHz", ((double)newcarr / 1000000.0));
		SetDlgItemText(hDlg, ctl_carr, tmp);

		// Refresh carrier info text
		sprintf_s(tmp, sizeof(tmp), "(%.1f sample points per sine wave at the current sample rate):", samplesPerCycle);
		SetDlgItemText(hDlg, ctl_stat, tmp);
	}
	else {
		SetDlgItemText(hDlg, ctl_carr, "off");
		SetDlgItemText(hDlg, ctl_stat, "This carrier will not be used");
	}

	return newcarr;
}

static void RefreshWndElements(BOOL only_freqwnd) {
	char tmp[1000];

	BOOL hint_redundantonly = FALSE;
	uint32_t samprate = getSampleRate(tempset.mult, tempset.div, tempset.frac, &hint_redundantonly);

	if (!only_freqwnd){
		// Refresh current sample rate
		sprintf_s(tmp, sizeof(tmp), "%lu", samprate);
		SetDlgItemText(hDlg, IDC_TX_FREQ_SAMPRATE, tmp);
		sprintf_s(tmp, sizeof(tmp), "(%.02f MS/s)", ((double)samprate / 1000000.0));
		SetDlgItemText(hDlg, IDC_TX_FREQ_SRATEDSP, tmp);

		// Refresh displayed (textual) values for manual settings
		sprintf_s(tmp, sizeof(tmp), "PLL multiplier: %lu", tempset.mult);
		SetDlgItemText(hDlg, IDC_TX_FREQ_MULT_DSP, tmp);

		sprintf_s(tmp, sizeof(tmp), "div: %lu", tempset.div);
		SetDlgItemText(hDlg, IDC_TX_FREQ_DIV_DSP, tmp);

		sprintf_s(tmp, sizeof(tmp), "frac: %lu", tempset.frac);
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

		// Adjust the sliders of the inactive mode to the current settings (which might have been changed by the active mode)
		if (manmode)	tmpSettingsToAutoSlider(); // In manual mode adjust the auto slider
		else			tmpSettingsToManualSliders(); // In auto mode adjust the manual sliders

		// En-/disable window elements regarding to the current mode
		EnableWindow(hSlSrate,	(manmode ? FALSE : TRUE));
		EnableWindow(hSlMult,	(manmode ? TRUE : FALSE));
		EnableWindow(hSlDiv,	(manmode ? TRUE : FALSE));
		EnableWindow(hSlFrac,	(manmode ? TRUE : FALSE));
		EnableWindow(hDspMult,	(manmode ? TRUE : FALSE));
		EnableWindow(hDspDiv,	(manmode ? TRUE : FALSE));
		EnableWindow(hDspFrac,	(manmode ? TRUE : FALSE));
	}

	// Check carrier bounds against Nyquist and adjust carrier range, if necessary
	tempset.carr1 = adjustCarrSlider(hSlCarr1, IDC_TX_FREQ_CARR1_DSP, IDC_TX_FREQ_CARR1_STAT, samprate, tempset.carr1);
	tempset.carr2 = adjustCarrSlider(hSlCarr2, IDC_TX_FREQ_CARR2_DSP, IDC_TX_FREQ_CARR2_STAT, samprate, tempset.carr2);

	// Refresh the info element about harmonic frequencies
	FreqList_Update(samprate, tempset.carr1, tempset.carr2);
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_TX_FREQ_RADAUTO:
	case IDC_TX_FREQ_RADMAN:
	{
		BOOL man_on = (IsDlgButtonChecked(hDlg, IDC_TX_FREQ_RADMAN) == BST_CHECKED);
		if (man_on != manmode) {
			manmode = man_on;
			RefreshWndElements(FALSE);
		}
		break;
	}

	case IDC_TX_FREQ_OK: {
		fl2k->cfg.carrier1 = tempset.carr1;
		fl2k->cfg.carrier2 = tempset.carr2;
		BOOL redundant_cfg = FALSE;
		fl2k->cfg.samp_rate = getSampleRate(tempset.mult, tempset.div, tempset.frac, &redundant_cfg);
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
			if (tempset.mult != fl2kcfgs[pos].mult || tempset.div != fl2kcfgs[pos].div || tempset.frac != fl2kcfgs[pos].frac) {
				tempset.mult = fl2kcfgs[pos].mult;
				tempset.div = fl2kcfgs[pos].div;
				tempset.frac = fl2kcfgs[pos].frac;
				RefreshWndElements(FALSE); // will also update the manual mode sliders
			}
		}
	}
	else if (hElem == hSlMult) {
		INT pos = (INT)SendMessage(hSlMult, TBM_GETPOS, 0, 0);
		if (pos >= FL2K_MULT_MIN && pos <= FL2K_MULT_MAX && tempset.mult != pos) {
			tempset.mult = pos;
			RefreshWndElements(FALSE); // will also update the auto mode slider
		}
	}
	else if (hElem == hSlDiv) {
		INT pos = (INT)SendMessage(hSlDiv, TBM_GETPOS, 0, 0);
		if (pos >= FL2K_DIV_MIN && pos <= FL2K_DIV_MAX && tempset.div != pos) {
			tempset.div = pos;
			RefreshWndElements(FALSE); // will also update the auto mode slider
		}
	}
	else if (hElem == hSlFrac) {
		INT pos = (INT)SendMessage(hSlFrac, TBM_GETPOS, 0, 0);
		if (pos >= FL2K_FRAC_MIN && pos <= FL2K_FRAC_MAX && tempset.frac != pos) {
			tempset.frac = pos;
			RefreshWndElements(FALSE); // will also update the auto mode slider
		}
	}
	else if (hElem == hSlCarr1) {
		INT pos = (INT)SendMessage(hSlCarr1, TBM_GETPOS, 0, 0);
		if (tempset.carr1 != (pos * CARRIER_RESOLUTION)) {
			tempset.carr1 = pos * CARRIER_RESOLUTION;
			RefreshWndElements(TRUE);
		}
	}
	else if (hElem == hSlCarr2) {
		INT pos = (INT)SendMessage(hSlCarr2, TBM_GETPOS, 0, 0);
		if (tempset.carr2 != (pos * CARRIER_RESOLUTION)) {
			tempset.carr2 = pos * CARRIER_RESOLUTION;
			RefreshWndElements(TRUE);
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
	hSlCarr1 = GetDlgItem(hDlg, IDC_TX_FREQ_CARR1_SL);
	hSlCarr2 = GetDlgItem(hDlg, IDC_TX_FREQ_CARR2_SL);
	hWarn = GetDlgItem(hDlg, IDC_TX_FREQ_WARNING);

	hFreqList = GetDlgItem(hDlg, IDC_TX_FREQ_HARM);

	InitSlider_Carr(hSlCarr1, 2 * tempset.carr1); // max value: just to get us started
	SendMessage(hSlCarr1, TBM_SETPOS, true, tempset.carr1 / CARRIER_RESOLUTION);
	InitSlider_Carr(hSlCarr2, 2 * tempset.carr2); // max value: just to get us started
	SendMessage(hSlCarr2, TBM_SETPOS, true, tempset.carr2 / CARRIER_RESOLUTION);

	CheckRadioButton(hDlg, IDC_TX_FREQ_RADAUTO, IDC_TX_FREQ_RADMAN, (manmode ? IDC_TX_FREQ_RADMAN : IDC_TX_FREQ_RADAUTO));
	InitConfAuto();
	InitConfMan();
	// Set the sliders of the active mode to the current position
	if (manmode) tmpSettingsToManualSliders();
	else tmpSettingsToAutoSlider();

	// Init Freq List
	FreqList_Init();
	// Refresh window elements (will also refresh the sliders of the inactive mode)
	RefreshWndElements(FALSE);
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
			tempset.mult = (cfg ? cfg->mult:FL2K_DEFAULT_MULT);
			tempset.div = (cfg ? cfg->div : FL2K_DEFAULT_DIV);
			tempset.frac = (cfg ? cfg->frac : FL2K_DEFAULT_FRAC);
			tempset.carr1 = fl2k->cfg.carrier1;
			tempset.carr2 = fl2k->cfg.carrier2;
			manmode = FALSE;
			r = (int)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_TX_FREQ), hParent, (DLGPROC)DialogHandler);
		}

	}
	return r;
}
