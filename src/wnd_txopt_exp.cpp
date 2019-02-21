/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *       Option sub-dialog for expert settings regarding TX        *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>

#include "libfl2k_433.h"
#include "wnd_txopt_exp.h"

#include "../res/gui_win_resources.h"

static fl2k_433_t	*fl2k= NULL;

static HWND	hDlg;

static int      tmp_verbose;        // -1 = edit control currently carries no valid value
static int      tmp_initime;        // -1 = edit control currently carries no valid value

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_TX_EXP_DBGLVL_EDT: {
		BOOL trans = FALSE;
		int tmp_dlev = GetDlgItemInt(hDlg, IDC_TX_EXP_DBGLVL_EDT, &trans, FALSE);
		tmp_verbose = (trans ? tmp_dlev : -1);
		break;
	}

	case IDC_TX_EXP_INITIME_EDT: {
		BOOL trans = FALSE;
		int tmp_ini = GetDlgItemInt(hDlg, IDC_TX_EXP_INITIME_EDT, &trans, FALSE);
		tmp_initime = (trans ? tmp_ini : -1);
		break;
	}

	case IDC_TX_EXP_BTN_OK: {
		fl2k->cfg.verbose = (tmp_verbose >= 0 ? tmp_verbose : FL2K_433_DEFAULT_VERBOSITY);
		fl2k->cfg.inittime_ms = (tmp_initime >= 0 ? tmp_initime : FL2K_433_DEFAULT_INIT_TIME);
		EndDialog(hDlg, TRUE);
		break;
	}

	case IDC_TX_EXP_BTN_CANCEL:
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

	SetDlgItemInt(hDlg, IDC_TX_EXP_DBGLVL_EDT, tmp_verbose, TRUE);
	SetDlgItemInt(hDlg, IDC_TX_EXP_INITIME_EDT, tmp_initime, TRUE);
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
			EndDialog(hDlg, TRUE);
			break;

		default:
			return false;
	}
	return true;
}

int ShowExpertDialog(fl2k_433_t *fl2k_obj, HWND hParent) {
	int r = -1;
	if (fl2k_obj) {
		fl2k = fl2k_obj;
		tmp_verbose = (fl2k->cfg.verbose     >= 0 ? fl2k->cfg.verbose     : FL2K_433_DEFAULT_VERBOSITY);
		tmp_initime = (fl2k->cfg.inittime_ms >= 0 ? fl2k->cfg.inittime_ms : FL2K_433_DEFAULT_INIT_TIME);
		r = (int) DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_TX_EXPERT), hParent, (DLGPROC)DialogHandler);
	}
	return r;
}
