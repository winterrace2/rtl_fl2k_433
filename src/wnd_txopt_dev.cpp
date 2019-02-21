/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *          Option sub-dialog for FL2K device selection            *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowDeviceTxDialog():      *
 * - updates fl2k->cfg.dev_index                                   *
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
#include "osmo-fl2k.h"
#include "wnd_txopt_dev.h"

#include "../res/gui_win_resources.h"

#define MAX_FL2K_DEVICES 10

typedef struct _RtlDevState {
	char name[3*256];
	//int crnt_ppm;
} RtlDevState, *pRtlDevState;


static RtlDevState devs[MAX_FL2K_DEVICES];
static int num_devs; // number of recognized devices
static int crnt_dev; // Device index, starting from 1 (0, if no device is selected)

static fl2k_433_t	*fl2k = 0;

static HWND		hDlg, hComboTxDevs;

static void DevInfo_Refresh() {
	num_devs = 0;
	crnt_dev = 0;

	uint16_t device_count = fl2k_get_device_count();
	for (int a = 0; a < device_count && num_devs < MAX_FL2K_DEVICES; a++) {
		const char *product = fl2k_get_device_name(a);
		if (product && *product) {
			sprintf_s(devs[num_devs].name, "%s", (product ? product : "n/a"));
			num_devs++;
		}
	}
}

static void Gui_RebuildDevlist() {
	// Update Combo Box:
	char caption[40];
	sprintf_s(caption, sizeof(caption), "%s%lu device%s found%s", (num_devs > 1 ? "Select from " : ""), num_devs, (num_devs != 1 ? "s" : ""), (num_devs > 0 ? ":" : "."));
	SendMessage(hComboTxDevs, CB_RESETCONTENT, 0, 0);
	SendMessage(hComboTxDevs, CB_ADDSTRING, 0, (LPARAM)caption);
	for (int a = 0; a < num_devs; a++) {
		SendMessage(hComboTxDevs, CB_ADDSTRING, 0, (LPARAM)devs[a].name);
	}
	crnt_dev = (num_devs == 1 ? 1 : 0);
	SendMessage(hComboTxDevs, CB_SETCURSEL, crnt_dev, 0); // 1 device: preselect this. >1 devices: No preselection
}

static void RefreshDev_Fl2k() {
	DevInfo_Refresh(); // Refresh structure with FL2K device infos (name)
	Gui_RebuildDevlist(); // Refresh combo box (device list), preselect if there's exactly 1 device.
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_TX_DEV_LIST: {
		int sel = (int)SendMessage(hComboTxDevs, CB_GETCURSEL, 0, 0);
		if (sel != crnt_dev && sel <= num_devs) {
			crnt_dev = sel;
		}
		break;
	}

	case IDC_TX_DEV_REFRESH:
		RefreshDev_Fl2k();
		break;

	case IDC_TX_DEV_OK: {
		if (crnt_dev > 0 && crnt_dev <= num_devs) {
			fl2k->cfg.dev_index = crnt_dev;
		}
		else {
			fl2k->cfg.dev_index = 0;
		}

		EndDialog(hDlg, 1);
		break;
	}

	case IDC_TX_DEV_CANCEL:
		EndDialog(hDlg, 0);
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

	hComboTxDevs = GetDlgItem(hDlg, IDC_TX_DEV_LIST);
	RefreshDev_Fl2k();
}

static INT_PTR CALLBACK DialogHandler(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

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

int ShowDeviceTxDialog(fl2k_433_t *fl2k_obj, HWND hParent, char **trgdev_name) {
	num_devs = 0;
	crnt_dev = 0;

	int r = -1;
	if (fl2k_obj) {
		fl2k = fl2k_obj;
		r = (int) DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_TX_DEV), hParent, (DLGPROC)DialogHandler);
		if(trgdev_name) *trgdev_name = (r>0 && crnt_dev > 0 && crnt_dev <= num_devs ? devs[crnt_dev-1].name : NULL);
	}
	return r;
}
