/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *            Sub-dialog to configure dumping of signals           *
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
#include <commdlg.h>
#include <shlobj.h>		// Contains SHBrowseForFolder

#include "librtl_433.h"
#include "wnd_rxopt_siggrab.h"

#include "../res/gui_win_resources.h"

static rtl_433_t *rtl = NULL;

static char tmp_siggrab_path[MAX_PATHLEN];   // path or "" for working dir
static BOOL tmp_siggrab_configured;
static unsigned char tmp_ovrflags_configured;

static HWND	hDlg, hPathLbl, hPathDsp, hPathBtnSet, hPathBtnCls, hOutOverwrite;

static void RefreshWndElements() {
	SetDlgItemText(hDlg, IDC_RX_SIGGRAB_PATH, (!tmp_siggrab_path[0] ? "<working dir>" : tmp_siggrab_path));
	CheckDlgButton(hDlg, IDC_RX_SIGGRAB_CHK,    (tmp_siggrab_configured ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_SIGGRAB_OVR,    (tmp_ovrflags_configured & OVR_SUBJ_SIGNALS  ? BST_CHECKED : BST_UNCHECKED));
	EnableWindow(hPathLbl, tmp_siggrab_configured);
	EnableWindow(hPathDsp, tmp_siggrab_configured);
	EnableWindow(hPathBtnSet, tmp_siggrab_configured);
	EnableWindow(hPathBtnCls, tmp_siggrab_configured);
}

static BOOL GetSaveFolder(LPSTR buf, INT cap, LPSTR title) {
	char fpath[MAX_PATH];
	BROWSEINFO BrwsInf = { hDlg,0,0,title, BIF_RETURNONLYFSDIRS,0,0,0 };
	PIDLIST_ABSOLUTE FldId = SHBrowseForFolder(&BrwsInf);
	if (!FldId || !SHGetPathFromIDList(FldId, fpath)) return FALSE;
	strcpy_s(buf, cap, fpath);
	return TRUE;
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_RX_SIGGRAB_CHK:
		tmp_siggrab_configured = !tmp_siggrab_configured;
		RefreshWndElements();
		break;

	case IDC_RX_SIGGRAB_PATH_BWS:
		if (tmp_siggrab_configured && GetSaveFolder(tmp_siggrab_path, sizeof(tmp_siggrab_path), "Select target dir (if cancelled, working dir will be used)")) {
			RefreshWndElements();
		}
		break;

	case IDC_RX_SIGGRAB_PATH_CLEAR:
		if (tmp_siggrab_configured) {
			tmp_siggrab_path[0] = 0;
			RefreshWndElements();
		}
		break;

	case IDC_RX_SIGGRAB_OVR:
		if (tmp_siggrab_configured) {
			tmp_ovrflags_configured ^= OVR_SUBJ_SIGNALS;
			RefreshWndElements();
		}
		break;


	case IDC_RX_SIGGRAB_OK: {
		rtl->cfg->grab_mode = (tmp_siggrab_configured ? GRAB_ALL_DEVICES : GRAB_DISABLED);
		strcpy_s(rtl->cfg->output_path_sigdmp, sizeof(rtl->cfg->output_path_sigdmp), tmp_siggrab_path);
		rtl->cfg->overwrite_modes = tmp_ovrflags_configured;
		EndDialog(hDlg, TRUE);
		break;
	}

	case IDC_RX_SIGGRAB_CANCEL:
		EndDialog(hDlg, FALSE);
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

	hPathLbl = GetDlgItem(hDlg, IDC_RX_SIGGRAB_PATH_LBL);
	hPathDsp = GetDlgItem(hDlg, IDC_RX_SIGGRAB_PATH);
	hPathBtnSet = GetDlgItem(hDlg, IDC_RX_SIGGRAB_PATH_BWS);
	hPathBtnCls = GetDlgItem(hDlg, IDC_RX_SIGGRAB_PATH_CLEAR);
	hOutOverwrite = GetDlgItem(hDlg, IDC_RX_STRGRAB_OVR);
	EnableWindow(hOutOverwrite, tmp_siggrab_configured);

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
			EndDialog(hDlg, FALSE);
			break;

		default:
			return false;
	}
	return true;
}

int ShowSignalGrabberDialog(rtl_433_t *rtl_obj, HWND hParent) {
	int r = -1;
	if (rtl_obj) {
		rtl = rtl_obj;
		tmp_siggrab_configured = (rtl->cfg->grab_mode != GRAB_DISABLED);
		strcpy_s(tmp_siggrab_path, sizeof(tmp_siggrab_path), rtl->cfg->output_path_sigdmp);
		tmp_ovrflags_configured = rtl->cfg->overwrite_modes;
		r = (int) DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_RX_SIGGRAB), hParent, (DLGPROC)DialogHandler);
	}
	return r;
}
