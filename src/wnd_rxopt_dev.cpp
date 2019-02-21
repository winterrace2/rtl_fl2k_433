/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *        Option sub-dialog for RTL-SDR device selection           *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowDeviceDialog():        *
 * - updates cfg-> <dev_query|dev_gain|ppm_error>                  *
 * - can provide a pointer to the device name (param trgdev_name)  *
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

#include "wnd_rxopt_dev.h"
#include "rtl-sdr.h"

#include "../res/gui_win_resources.h"

#define MAX_GAIN_VALUES 50
#define MAX_RTLSDR_DEVICES 10

#define NUM_SRATE_PRESETS 11
unsigned long sr_presets[NUM_SRATE_PRESETS] = {
	3200000,
	2800000,
	2560000,
	2400000,
	2048000,
	1920000,
	1800000,
	1400000,
	1024000,
	 900001,
	 250000
};
#define DEFAULT_SRATE_IDX 10 // // rtl_433 default sample rate is 250000

typedef struct _RtlDevState {
	char name[3*256];
	unsigned long crnt_srate_idx;
	unsigned char auto_gain;
	int valid_gain_values[MAX_GAIN_VALUES];
	int num_gain_values;
	int crnt_gain_index;
	int crnt_ppm;
} RtlDevState, *pRtlDevState;

static RtlDevState devs[MAX_RTLSDR_DEVICES]; // information about all recognized devices
static int num_devs; // number of recognized devices
static int crnt_dev; // index of currently selected device, starting from 1 (0, if no device is selected)

static rtl_433_t		*rtl = 0;

static HWND		hDlg, hComboRxDevs, hSampRate, hRxGainDsp, hRxGainSlider, hRxPpmEdt, hRadGAut, hRadGMan, hBtnOk;

static void DevInfo_Refresh() {
	num_devs = 0;
	crnt_dev = 0;

	uint16_t device_count = rtlsdr_get_device_count();
	for (int a = 0; a < device_count && num_devs < MAX_RTLSDR_DEVICES; a++) {
		char vendor[256], product[256], serial[256];
		rtlsdr_get_device_usb_strings(a, vendor, product, serial);
		sprintf_s(devs[num_devs].name, "%s %s, SN: %s", vendor, product, serial);

		rtlsdr_dev_t *dev = 0;
		if (rtlsdr_open(&dev, a) >= 0) {
			int gain_no = rtlsdr_get_tuner_gains(dev, NULL);
			if (gain_no > 0) {
				if (gain_no <= MAX_GAIN_VALUES) {
					devs[num_devs].num_gain_values = rtlsdr_get_tuner_gains(dev, devs[num_devs].valid_gain_values);
					devs[num_devs].crnt_gain_index = 0;
					devs[num_devs].auto_gain = 1;
					devs[num_devs].crnt_ppm = 0;
					devs[num_devs].crnt_srate_idx = DEFAULT_SRATE_IDX;
					num_devs++;
				}
				//else TODO: warning?
			}
			rtlsdr_close(dev);
		}
	}
}

static void Gui_RebuildDevlist() {
	// Update Combo Box:
	char caption[40];
	sprintf_s(caption, sizeof(caption), "%s%lu device%s found%s", (num_devs > 1 ? "Select from " : ""), num_devs, (num_devs != 1 ? "s" : ""), (num_devs > 0 ? ":" : "."));
	SendMessage(hComboRxDevs, CB_RESETCONTENT, 0, 0);
	SendMessage(hComboRxDevs, CB_ADDSTRING, 0, (LPARAM)caption);
	for (int a = 0; a < num_devs; a++) {
		SendMessage(hComboRxDevs, CB_ADDSTRING, 0, (LPARAM)devs[a].name);
	}
	SendMessage(hComboRxDevs, CB_SETCURSEL, crnt_dev, 0);
}

static void Gui_ReconfigureGainPpm() {
	if (crnt_dev > 0 && crnt_dev <= num_devs){
		// Update Gain Slider:
		SendMessage(hRxGainSlider, TBM_SETRANGEMIN, false, 0);
		SendMessage(hRxGainSlider, TBM_SETRANGEMAX, false, (devs[crnt_dev-1].num_gain_values > 1 ? devs[crnt_dev-1].num_gain_values - 1 : 0));
		SendMessage(hRxGainSlider, TBM_SETPOS, true, devs[crnt_dev-1].crnt_gain_index);
		SendMessage(hRxGainSlider, TBM_SETLINESIZE, 0, 1);
		SendMessage(hRxGainSlider, TBM_SETPAGESIZE, 0, 5);
		CheckRadioButton(hDlg, IDC_RX_DEV_RAD_GAINAUTO, IDC_RX_DEV_RAD_GAINMAN, (devs[crnt_dev-1].auto_gain? IDC_RX_DEV_RAD_GAINAUTO : IDC_RX_DEV_RAD_GAINMAN));
		SetDlgItemInt(hDlg, IDC_RX_DEV_PPM_EDT, devs[crnt_dev-1].crnt_ppm, TRUE);
	}
	ShowWindow(hRxGainDsp, (crnt_dev > 0 && crnt_dev <= num_devs && devs[crnt_dev-1].auto_gain == 0) ? SW_SHOW : SW_HIDE);
	ShowWindow(hRxGainSlider, (crnt_dev > 0 && crnt_dev <= num_devs && devs[crnt_dev-1].auto_gain == 0) ? SW_SHOW : SW_HIDE);
	EnableWindow(hRadGAut, (crnt_dev > 0 && crnt_dev <= num_devs));
	EnableWindow(hSampRate, (crnt_dev > 0 && crnt_dev <= num_devs));
	EnableWindow(hRadGMan, (crnt_dev > 0 && crnt_dev <= num_devs));
	EnableWindow(hRxPpmEdt, (crnt_dev > 0 && crnt_dev <= num_devs));
	EnableWindow(hBtnOk, (crnt_dev > 0 && crnt_dev <= num_devs));
}

static void Gui_RefreshLabels() {
	if (crnt_dev > 0 && crnt_dev <= num_devs) {
		// Display current gain in dB
		char tmp[20];
		int gain_idx = devs[crnt_dev-1].crnt_gain_index;
		if (gain_idx < 0 || gain_idx >= devs[crnt_dev-1].num_gain_values) gain_idx = 0;
		int gain_int = devs[crnt_dev-1].valid_gain_values[gain_idx];
		sprintf_s(tmp, sizeof(tmp), "%.1f dB", (((double)gain_int) / 10.0));
		SetDlgItemText(hDlg, IDC_RX_DEV_GAIN_DSP, tmp);
	}
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_RX_DEV_LIST: {
		int sel = (int)SendMessage(hComboRxDevs, CB_GETCURSEL, 0, 0);
		if (sel != crnt_dev && sel <= num_devs) {
			crnt_dev = sel;
			Gui_ReconfigureGainPpm();
		}
		break;
	}

	case IDC_RX_DEV_SRATE: {
		int sel = (int)SendMessage(hSampRate, CB_GETCURSEL, 0, 0);
		if (crnt_dev && devs[crnt_dev-1].crnt_srate_idx != sel) {
			devs[crnt_dev - 1].crnt_srate_idx = sel;
		}
		break;
	}

	case IDC_RX_DEV_REFRESH:
		DevInfo_Refresh();			// Refresh structure with RTL-SDR device infos (name, available gain options, ...)	- sets crnt_dev to 0
		crnt_dev = (num_devs == 1 ? 1 : 0); // pre-select the device if only 1 is present
		Gui_RebuildDevlist();		// Refresh combo box (device list)
		Gui_ReconfigureGainPpm();	// Refresh gain and ppm elements (or disable, if no device is selected)
		Gui_RefreshLabels();
		break;

	case IDC_RX_DEV_RAD_GAINAUTO:
	case IDC_RX_DEV_RAD_GAINMAN: {
		if (crnt_dev > 0 && crnt_dev <= num_devs) {
			int new_autogain = (IsDlgButtonChecked(hDlg, IDC_RX_DEV_RAD_GAINAUTO) == BST_CHECKED ? 1 : 0);
			int old_autogain = (devs[crnt_dev-1].auto_gain ? 1 : 0);
			if (new_autogain != old_autogain) {
				devs[crnt_dev-1].auto_gain = new_autogain;
				Gui_ReconfigureGainPpm();
			}
		}
		break;
	}

	case IDC_RX_DEV_PPM_EDT:
		if (crnt_dev > 0 && crnt_dev <= num_devs) {
			BOOL trans = FALSE;
			INT v = GetDlgItemInt(hDlg, IDC_RX_DEV_PPM_EDT, &trans, TRUE);
			if (trans) {
				devs[crnt_dev-1].crnt_ppm = v;
			}
		}
		break;

	case IDC_RX_DEV_OK: {
		if (crnt_dev > 0 && crnt_dev <= num_devs) {
			sprintf_s(rtl->cfg->dev_query, sizeof(rtl->cfg->dev_query), "%lu", crnt_dev - 1);
			rtl->cfg->ppm_error = devs[crnt_dev-1].crnt_ppm;
			rtl->cfg->samp_rate = sr_presets[devs[crnt_dev - 1].crnt_srate_idx];
			sprintf_s(rtl->cfg->gain_str, sizeof(rtl->cfg->gain_str), "%lu", (devs[crnt_dev-1].auto_gain ? 0 : devs[crnt_dev-1].valid_gain_values[devs[crnt_dev-1].crnt_gain_index]));
		}
		else {
			rtl->cfg->dev_query[0] = 0;
			rtl->cfg->samp_rate = sr_presets[DEFAULT_SRATE_IDX];
			rtl->cfg->gain_str[0] = 0;
			rtl->cfg->ppm_error = 0;
		}

		EndDialog(hDlg, 1);
		break;
	}

	case IDC_RX_DEV_CANCEL:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static VOID onHorScroll(HWND hElem) {
	if (hElem == hRxGainSlider) {
		if (crnt_dev > 0 && crnt_dev <= num_devs) {
			INT pos = (INT)SendMessage(hRxGainSlider, TBM_GETPOS, 0, 0);
			if (pos >= 0 && pos < devs[crnt_dev-1].num_gain_values) {
				devs[crnt_dev-1].crnt_gain_index = pos;
				Gui_RefreshLabels();
			}
		}
	}
}

static VOID onInit(HWND hwndDlg) {
	hDlg = hwndDlg;
	HICON hIcon = LoadIcon(GetModuleHandle(0), (const char *)IDI_ICON1);
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	hComboRxDevs = GetDlgItem(hDlg, IDC_RX_DEV_LIST);
	hSampRate = GetDlgItem(hDlg, IDC_RX_DEV_SRATE);
	hRxGainSlider = GetDlgItem(hDlg, IDC_RX_DEV_GAIN_SLIDER);
	hRxGainDsp = GetDlgItem(hDlg, IDC_RX_DEV_GAIN_DSP);
	hRadGAut = GetDlgItem(hDlg, IDC_RX_DEV_RAD_GAINAUTO);
	hRadGMan = GetDlgItem(hDlg, IDC_RX_DEV_RAD_GAINMAN);
	hRxPpmEdt = GetDlgItem(hDlg, IDC_RX_DEV_PPM_EDT);
	hBtnOk = GetDlgItem(hDlg, IDC_RX_DEV_OK);

	char srate_str[40];
	for (int a = 0; a < NUM_SRATE_PRESETS; a++) {
		sprintf_s(srate_str, sizeof(srate_str), "%.6g MSPS", ((double) sr_presets[a]) / 1000000.0);
		SendMessage(hSampRate, CB_ADDSTRING, 0, (LPARAM)srate_str);
	}
	SendMessage(hSampRate, CB_SETCURSEL, devs[crnt_dev-1].crnt_srate_idx, 0);
	
	Gui_RebuildDevlist();		// Refresh combo box (device list), preselect if there's exactly 1 device.			- sets crnt_dev to 1 and selects the entry if only 1 dev is present
	Gui_ReconfigureGainPpm();	// Refresh gain and ppm elements (or disable, if no device is selected)
	Gui_RefreshLabels();
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

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return false;
	}
	return true;
}

BOOL IsCompatible(RtlDevState *dev, long samp_rate, long gain) {
	// 1) check if gain value is valid for this device
	BOOL gain_ok = (gain == 0); // auto gain is OK
	for (int a = 0; !gain_ok && a < dev->num_gain_values; a++) {
		if (gain == dev->valid_gain_values[a]) gain_ok = TRUE;
	}
	if (!gain_ok) return FALSE;

	// 2) check if sample rate is one of our preset values
	BOOL samp_rate_ok = FALSE;
	for (int a = 0; !samp_rate_ok && a < NUM_SRATE_PRESETS; a++) {
		if (samp_rate == sr_presets[a]) samp_rate_ok = TRUE;
	}
	return (gain_ok && samp_rate_ok);
}

int ShowDeviceRxDialog(rtl_433_t *rtl_obj, HWND hParent, char **trgdev_name) {
	int r = -1;
	if (rtl_obj && rtl_obj->cfg) {
		rtl = rtl_obj;

		// refresh structure with RTL-SDR device infos (name, available gain options, ...)	- sets crnt_dev to 0
		DevInfo_Refresh();

		// if possible, pre-select the previous device and load its settings
		if (num_devs > 0) {
			long cfg_gain = atol(rtl->cfg->gain_str);
			long cfg_dev = atol(rtl->cfg->dev_query);
			if ((cfg_gain > 0 || !strcmp(rtl->cfg->gain_str, "0")) && (cfg_dev > 0 || !strcmp(rtl->cfg->dev_query, "0"))) {
				// first check if it matches the device at the same index
				if (cfg_dev < num_devs && IsCompatible(&devs[cfg_dev], rtl->cfg->samp_rate, cfg_gain)) {
					crnt_dev = cfg_dev + 1;
				}
				// Otherwise preselect the fist matching device (maybe the device constellation changed meanwhile)
				for (int a = 0; !crnt_dev && a < num_devs; a++) {
					if (a == cfg_dev) continue; // we already checked this one
					if (IsCompatible(&devs[a], rtl->cfg->samp_rate, cfg_gain)) {
						crnt_dev = a + 1;
					}
				}
			}
			// if we have a preselection, then apply the further settings from the cfg
			if (crnt_dev) {
				devs[crnt_dev-1].crnt_ppm = rtl->cfg->ppm_error;
				devs[crnt_dev-1].auto_gain = (cfg_gain==0);
				for (int a = 0; a < devs[crnt_dev-1].num_gain_values; a++) {
					if (devs[crnt_dev-1].valid_gain_values[a] == cfg_gain) {
						devs[crnt_dev-1].crnt_gain_index = a;
						break;
					}
				}
				for (int a = 0; a < NUM_SRATE_PRESETS; a++) {
					if (rtl->cfg->samp_rate == sr_presets[a]) {
						devs[crnt_dev-1].crnt_srate_idx = a;
						break;
					}
				}
			}
		}

		// if we could not pre-select the last device, preselect the current one (if theres exactly one)
		if(!crnt_dev && num_devs == 1) crnt_dev = 1;

		// load dialog box
		r = (int) DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_RX_DEV), hParent, (DLGPROC)DialogHandler);
		if(trgdev_name) *trgdev_name = (r > 0 && crnt_dev > 0 && crnt_dev <= num_devs ? devs[crnt_dev-1].name : NULL);
	}
	return r;
}
