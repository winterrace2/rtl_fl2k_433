/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *         Sub-dialog to configure dumping of sample stream        *
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

#include "librtl_433.h"
#include "wnd_rxopt_siggrab.h"

#include "../res/gui_win_resources.h"

static rtl_433_t *rtl = NULL;

static char tmp_strgrab_file[MAX_PATHLEN];   // file or "-" for working stdout
static BOOL tmp_strgrab_configured;
static unsigned char tmp_ovrflags_configured;

static HWND	hDlg, hFileLbl, hFileDsp, hFileBtnSet, hFileBtnSout, hOutOverwrite, hOkBtn;

static void RefreshWndElements() {
	SetDlgItemText(hDlg, IDC_RX_STRGRAB_FILE, (tmp_strgrab_file[0]=='-' ? "<stdout>" : tmp_strgrab_file));
	CheckDlgButton(hDlg, IDC_RX_STRGRAB_CHK, (tmp_strgrab_configured ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_STRGRAB_STDOUT, (tmp_strgrab_file[0] == '-' ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_STRGRAB_OVR, (tmp_ovrflags_configured & OVR_SUBJ_SAMPLES ? BST_CHECKED : BST_UNCHECKED));
	EnableWindow(hFileLbl, tmp_strgrab_configured);
	EnableWindow(hFileDsp, tmp_strgrab_configured);
	EnableWindow(hFileBtnSet, tmp_strgrab_configured);
	EnableWindow(hFileBtnSout, tmp_strgrab_configured);
	EnableWindow(hOutOverwrite, tmp_strgrab_configured);
	EnableWindow(hOkBtn, !tmp_strgrab_configured || tmp_strgrab_file[0]);
}

static BOOL GetSaveFilePath(LPSTR buf, INT cap, LPSTR filter, LPSTR title, LPSTR defext) {
	char fpath[MAX_PATH] = "";
	OPENFILENAME ofntemp = { sizeof(OPENFILENAME), hDlg, 0, filter, 0, 0, 0, fpath, sizeof(fpath), 0,0,0, title, OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_READONLY | OFN_HIDEREADONLY, 0, 0, defext, 0, 0, 0 };
	if (!GetSaveFileName(&ofntemp)) return FALSE;
	strcpy_s(buf, cap, fpath);
	return TRUE;
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_RX_STRGRAB_CHK:
		tmp_strgrab_configured = !tmp_strgrab_configured;
		RefreshWndElements();
		break;

	case IDC_RX_STRGRAB_FILE_BWS:
		if (tmp_strgrab_configured) {
			if (GetSaveFilePath(tmp_strgrab_file, sizeof(tmp_strgrab_file), "Dump files (*.dump)\0*.dump\0\0", "Select target sample file:", "dump")) {
				RefreshWndElements();
			}
		}
		break;

	case IDC_RX_STRGRAB_STDOUT:
		if (tmp_strgrab_configured) {
			if (tmp_strgrab_file[0] != '-') {
				strcpy_s(tmp_strgrab_file, sizeof(tmp_strgrab_file), "-");
			}
			else {
				tmp_strgrab_file[0] = 0;
			}
			RefreshWndElements();
		}
		break;

	case IDC_RX_STRGRAB_OVR:
		if (tmp_strgrab_configured) {
			tmp_ovrflags_configured ^= OVR_SUBJ_SAMPLES;
			RefreshWndElements();
		}
		break;


	case IDC_RX_STRGRAB_OK: {
		strcpy_s(rtl->cfg->out_filename, sizeof(rtl->cfg->out_filename), (tmp_strgrab_configured ? tmp_strgrab_file : ""));
		rtl->cfg->overwrite_modes = tmp_ovrflags_configured;
		EndDialog(hDlg, TRUE);
		break;
	}

	case IDC_RX_STRGRAB_CANCEL:
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

	hFileLbl = GetDlgItem(hDlg, IDC_RX_STRGRAB_FILE_LBL);
	hFileDsp = GetDlgItem(hDlg, IDC_RX_STRGRAB_FILE);
	hFileBtnSet = GetDlgItem(hDlg, IDC_RX_STRGRAB_FILE_BWS);
	hFileBtnSout = GetDlgItem(hDlg, IDC_RX_STRGRAB_STDOUT);
	hOutOverwrite = GetDlgItem(hDlg, IDC_RX_STRGRAB_OVR);
	hOkBtn = GetDlgItem(hDlg, IDC_RX_STRGRAB_OK);

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

int ShowStreamGrabberDialog(rtl_433_t *rtl_obj, HWND hParent) {
	int r = -1;
	if (rtl_obj) {
		rtl = rtl_obj;
		tmp_strgrab_configured = (rtl->cfg->out_filename[0] != 0);
		strcpy_s(tmp_strgrab_file, sizeof(tmp_strgrab_file), rtl->cfg->out_filename);
		tmp_ovrflags_configured = rtl->cfg->overwrite_modes;
		r = (int)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_RX_STRGRAB), hParent, (DLGPROC)DialogHandler);
	}
	return r;
}
