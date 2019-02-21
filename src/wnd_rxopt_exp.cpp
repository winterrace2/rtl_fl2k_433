/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *       Option sub-dialog for expert settings regarding RX        *
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

#include "librtl_433.h"
#include "wnd_rxopt_exp.h"

#include "../res/gui_win_resources.h"

static rtl_433_t	*rtl = NULL;

static HWND	hDlg;
static HWND	hEdtSydu, hEdtBamt, hEdtBlev, hEdtDlev, hEdtSVal, hEdtLVal, hBtnOk, hCmbConv, hVerbCmb;

static int      tmp_override_short;      // -1 = no valid value configured, 0 = auto
static int      tmp_override_long;	     // -1 = no valid value configured, 0 = auto
static int      tmp_verbosity;
static int		tmp_duration;		     // -1 = no valid value configured
static int      tmp_byteamt;		     // -1 = no valid value configured
static uint32_t tmp_out_block_size_d512; // 0 = no valid value configured
static int32_t  tmp_level_limit;         // -1 = no valid value configured, 0 = auto
static char tmp_conversion_mode;         // 0...2 = valid, -1 = invalid
static BOOL tmp_chkdur, tmp_chkbam, tmp_chkblev, tmp_chksval, tmp_chklval, tmp_chkstop;

static void RefreshWndElements() {

	CheckDlgButton(hDlg, IDC_RX_EXP_DUR, (tmp_chkdur ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_EXP_BYTEAMT, (tmp_chkbam ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_EXP_BLEV_OVR, (tmp_chkblev ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_EXP_SVAL_OVR, (tmp_chksval ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_EXP_LVAL_OVR, (tmp_chklval ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_EXP_AUTOSTOP, (tmp_chkstop ? BST_CHECKED : BST_UNCHECKED));

	char tmpstr[100] = "";
	sprintf_s(tmpstr, sizeof(tmpstr), "* 512 = %d", (512 * tmp_out_block_size_d512));
	SetDlgItemText(hDlg, IDC_RX_EXP_BSIZE_DSP, tmpstr);

	EnableWindow(hEdtSydu, tmp_chkdur);
	EnableWindow(hEdtBamt, tmp_chkbam);
	EnableWindow(hEdtBlev, tmp_chkblev);
	EnableWindow(hEdtSVal, tmp_chksval);
	EnableWindow(hEdtLVal, tmp_chklval);
	EnableWindow(hBtnOk, (!tmp_chkdur || tmp_duration > 0) && (!tmp_chkbam || tmp_byteamt > 0) && (tmp_out_block_size_d512 > 0) && (!tmp_chkblev || tmp_level_limit > 0) && (!tmp_chksval || tmp_override_short > 0) && (!tmp_chklval || tmp_override_long > 0) && (tmp_conversion_mode >= CONVERT_NATIVE && tmp_conversion_mode <= CONVERT_CUSTOMARY));
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_RX_EXP_DUR:
		tmp_chkdur = !tmp_chkdur;
		RefreshWndElements();
		break;

	case IDC_RX_EXP_DUR_EDT: {
		BOOL trans = FALSE;
		int tmp_dur = GetDlgItemInt(hDlg, IDC_RX_EXP_DUR_EDT, &trans, FALSE);
		tmp_duration = (trans ? tmp_dur : -1);
		RefreshWndElements();
		break;
	}
	case IDC_RX_EXP_BYTEAMT:
		tmp_chkbam = !tmp_chkbam;
		RefreshWndElements();
		break;

	case IDC_RX_EXP_BYTEAMT_EDT: {
		BOOL trans = FALSE;
		int tmp_bam = GetDlgItemInt(hDlg, IDC_RX_EXP_BYTEAMT_EDT, &trans, FALSE);
		tmp_byteamt = (trans ? tmp_bam : -1);
		RefreshWndElements();
		break;
	}
	case IDC_RX_EXP_BSIZE_EDT: {
		BOOL trans = FALSE;
		int tmp_bsize = GetDlgItemInt(hDlg, IDC_RX_EXP_BSIZE_EDT, &trans, FALSE);
		tmp_out_block_size_d512 = (trans ? tmp_bsize : 0);
		RefreshWndElements();
		break;
	}
	case IDC_RX_EXP_CONVMD: {
		int sel = (int)SendMessage(hCmbConv, CB_GETCURSEL, 0, 0);
		if (sel != tmp_conversion_mode) {
			tmp_conversion_mode = sel;
			RefreshWndElements();
		}
		break;
	}
	case IDC_RX_EXP_AUTOSTOP:
		tmp_chkstop = !tmp_chkstop;
		RefreshWndElements();
		break;

	case IDC_RX_EXP_DBGLVL:
		tmp_verbosity = (INT)SendMessage(hVerbCmb, CB_GETCURSEL, 0, 0);
		break;

	case IDC_RX_EXP_BLEV_OVR:
		tmp_chkblev = !tmp_chkblev;
		RefreshWndElements();
		break;

	case IDC_RX_EXP_BLEV_EDT: {
		BOOL trans = FALSE;
		int tmp_lim = GetDlgItemInt(hDlg, IDC_RX_EXP_BLEV_EDT, &trans, FALSE);
		tmp_level_limit = (trans ? tmp_lim : -1);
		RefreshWndElements();
		break;
	}

	case IDC_RX_EXP_SVAL_OVR:
		tmp_chksval = !tmp_chksval;
		RefreshWndElements();
		break;

	case IDC_RX_EXP_SVAL_EDT: {
		BOOL trans = FALSE;
		int tmp_sval = GetDlgItemInt(hDlg, IDC_RX_EXP_SVAL_EDT, &trans, FALSE);
		tmp_override_short = (trans ? tmp_sval : -1);
		RefreshWndElements();
		break;
	}

	case IDC_RX_EXP_LVAL_OVR:
		tmp_chklval = !tmp_chklval;
		RefreshWndElements();
		break;

	case IDC_RX_EXP_LVAL_EDT: {
		BOOL trans = FALSE;
		int tmp_lval = GetDlgItemInt(hDlg, IDC_RX_EXP_LVAL_EDT, &trans, FALSE);
		tmp_override_long = (trans ? tmp_lval : -1);
		RefreshWndElements();
		break;
	}

	case IDC_RX_EXP_BTN_OK: {
		rtl->cfg->duration = (tmp_chkdur  && tmp_duration > 0 ? tmp_duration : 0);
		rtl->cfg->bytes_to_read = (tmp_chkbam  && tmp_byteamt > 0 ? tmp_byteamt : 0);
		rtl->cfg->level_limit = (tmp_chkblev && tmp_level_limit > 0 ? tmp_level_limit : 0);
		rtl->cfg->override_short = (tmp_chksval && tmp_override_short > 0 ? tmp_override_short : 0);
		rtl->cfg->override_long = (tmp_chklval && tmp_override_long > 0 ? tmp_override_long : 0);
		rtl->cfg->verbosity = tmp_verbosity;
		rtl->cfg->out_block_size = (tmp_out_block_size_d512 > 0 ? 512 * tmp_out_block_size_d512 : 0);
		rtl->cfg->stop_after_successful_events_flag = (tmp_chkstop ? 1 : 0);
		rtl->cfg->conversion_mode = ((tmp_conversion_mode >= CONVERT_NATIVE && tmp_conversion_mode <= CONVERT_CUSTOMARY) ? (conversion_mode_t)tmp_conversion_mode : CONVERT_NATIVE);
		EndDialog(hDlg, TRUE);
		break;
	}

	case IDC_RX_EXP_BTN_CANCEL:
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

	hEdtSydu = GetDlgItem(hDlg, IDC_RX_EXP_DUR_EDT);
	hEdtBamt = GetDlgItem(hDlg, IDC_RX_EXP_BYTEAMT_EDT);
	hEdtBlev = GetDlgItem(hDlg, IDC_RX_EXP_BLEV_EDT);
	hEdtSVal = GetDlgItem(hDlg, IDC_RX_EXP_SVAL_EDT);
	hEdtLVal = GetDlgItem(hDlg, IDC_RX_EXP_LVAL_EDT);
	hBtnOk = GetDlgItem(hDlg, IDC_RX_EXP_BTN_OK);
	hCmbConv = GetDlgItem(hDlg, IDC_RX_EXP_CONVMD);
	hVerbCmb = GetDlgItem(hDlg, IDC_RX_EXP_DBGLVL);

	SendMessage(hCmbConv, CB_ADDSTRING, 0, (LPARAM)"Native");
	SendMessage(hCmbConv, CB_ADDSTRING, 0, (LPARAM)"SI");
	SendMessage(hCmbConv, CB_ADDSTRING, 0, (LPARAM)"Customary");
	SendMessage(hCmbConv, CB_SETCURSEL, tmp_conversion_mode, 0);

	SendMessage(hVerbCmb, CB_ADDSTRING, 0, (LPARAM)"0 (normal)");
	SendMessage(hVerbCmb, CB_ADDSTRING, 0, (LPARAM)"1 (verbose)");
	SendMessage(hVerbCmb, CB_ADDSTRING, 0, (LPARAM)"2 (verbose decoders)");
	SendMessage(hVerbCmb, CB_ADDSTRING, 0, (LPARAM)"3 (debug decoders)");
	SendMessage(hVerbCmb, CB_ADDSTRING, 0, (LPARAM)"4 (trace decoding)");
	SendMessage(hVerbCmb, CB_SETCURSEL, tmp_verbosity, 0);

	if (tmp_out_block_size_d512 > 0) SetDlgItemInt(hDlg, IDC_RX_EXP_BSIZE_EDT, tmp_out_block_size_d512, FALSE);
	if (tmp_duration > 0)            SetDlgItemInt(hDlg, IDC_RX_EXP_DUR_EDT, tmp_duration, FALSE);
	if (tmp_byteamt > 0)             SetDlgItemInt(hDlg, IDC_RX_EXP_BYTEAMT_EDT, tmp_byteamt, FALSE);
	if (tmp_level_limit > 0)         SetDlgItemInt(hDlg, IDC_RX_EXP_BLEV_EDT, tmp_level_limit, FALSE);
	if (tmp_override_short > 0)      SetDlgItemInt(hDlg, IDC_RX_EXP_SVAL_EDT, tmp_override_short, FALSE);
	if (tmp_override_long > 0)       SetDlgItemInt(hDlg, IDC_RX_EXP_LVAL_EDT, tmp_override_long, FALSE);

	RefreshWndElements();
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

int ShowExpertDialog(rtl_433_t *rtl_obj, HWND hParent) {
	int r = -1;
	if (rtl_obj) {
		rtl = rtl_obj;

		tmp_chkdur					= (rtl->cfg->duration > 0);
		tmp_duration				= (rtl->cfg->duration > 0 ? rtl->cfg->duration : -1);
		tmp_chkbam                  = (rtl->cfg->bytes_to_read > 0);
		tmp_byteamt                 = (rtl->cfg->bytes_to_read > 0 ? rtl->cfg->bytes_to_read: -1);
		tmp_chkblev					= (rtl->cfg->level_limit > 0);
		tmp_level_limit				= (rtl->cfg->level_limit > 0 ? rtl->cfg->level_limit : -1);
		tmp_chksval					= (rtl->cfg->override_short > 0);
		tmp_override_short			= (rtl->cfg->override_short > 0 ? rtl->cfg->override_short : -1);
		tmp_chklval					= (rtl->cfg->override_long > 0);
		tmp_override_long			= (rtl->cfg->override_long > 0 ? rtl->cfg->override_long : -1);
		tmp_verbosity				= (rtl->cfg->verbosity < 0 ||rtl->cfg->verbosity > 4 ? 4: rtl->cfg->verbosity);
		tmp_out_block_size_d512		= (rtl->cfg->out_block_size > 0 && rtl->cfg->out_block_size % 512 == 0 ? rtl->cfg->out_block_size / 512 : 0);
		tmp_chkstop					= (rtl->cfg->stop_after_successful_events_flag > 0);
		tmp_conversion_mode			= ((rtl->cfg->conversion_mode >= CONVERT_NATIVE && rtl->cfg->conversion_mode <= CONVERT_CUSTOMARY) ? rtl->cfg->conversion_mode: -1);

		r = (int) DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_RX_EXPERT), hParent, (DLGPROC)DialogHandler);
	}
	return r;
}
