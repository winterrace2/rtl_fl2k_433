/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                 Option sub-dialog for RX output                 *
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
#include "wnd_rxopt_out.h"

#include "../res/gui_win_resources.h"

static rtl_433_t *rtl = NULL;

static char tmp_outpath_kv[MAX_PATHLEN];   // path or "-" for stdout
static char tmp_outpath_csv[MAX_PATHLEN];  // path or "-" for stdout
static char tmp_outpath_json[MAX_PATHLEN]; // path or "-" for stdout
static char tmp_outpath_samp[MAX_PATHLEN]; // path or "-" for stdout
static BOOL tmp_sampout_configured;
static char tmp_out_udp_host[100];
static unsigned short tmp_out_udp_port;
static unsigned char tmp_outputs_configured;
static unsigned char tmp_ovrflags_configured;

static HWND	hDlg, hOutEdtHost, hOutEdtPort, hOkBtn;
static HWND	hOutDspKv, hOutDspCsv, hOutDspJson;
static HWND	hOutBtnKv, hOutBtnCsv, hOutBtnJson;
static HWND hOutStdoKv, hOutStdoCsv, hOutStdoJson;
static HWND	hOutOvrKv, hOutOvrCsv, hOutOvrJson;

static void RefreshWndElements() {
	SetDlgItemText(hDlg, IDC_RX_OUT_PATH_KV, (tmp_outpath_kv[0] == '-' ? "<STDOUT>" : tmp_outpath_kv));
	SetDlgItemText(hDlg, IDC_RX_OUT_PATH_CSV, (tmp_outpath_csv[0] == '-' ? "<STDOUT>" : tmp_outpath_csv));
	SetDlgItemText(hDlg, IDC_RX_OUT_PATH_JSON, (tmp_outpath_json[0] == '-' ? "<STDOUT>" : tmp_outpath_json));

	CheckDlgButton(hDlg, IDC_RX_OUT_CHK_KV, (tmp_outputs_configured & OUTPUT_KV ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_OUT_CHK_CSV, (tmp_outputs_configured & OUTPUT_CSV ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_OUT_CHK_JSON, (tmp_outputs_configured & OUTPUT_JSON ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_OUT_CHK_UDP, (tmp_outputs_configured & OUTPUT_UDP ? BST_CHECKED : BST_UNCHECKED));
	
	CheckDlgButton(hDlg, IDC_RX_OUT_STDO_KV,   (tmp_outpath_kv[0] == '-' ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_OUT_STDO_CSV,  (tmp_outpath_csv[0] == '-' ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_OUT_STDO_JSON, (tmp_outpath_json[0] == '-' ? BST_CHECKED : BST_UNCHECKED));
	
	CheckDlgButton(hDlg, IDC_RX_OUT_OVR_KV,    (tmp_ovrflags_configured & OVR_SUBJ_DEC_KV   ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_OUT_OVR_CSV,   (tmp_ovrflags_configured & OVR_SUBJ_DEC_CSV  ? BST_CHECKED : BST_UNCHECKED));
	CheckDlgButton(hDlg, IDC_RX_OUT_OVR_JSON,  (tmp_ovrflags_configured & OVR_SUBJ_DEC_JSON ? BST_CHECKED : BST_UNCHECKED));

	EnableWindow(hOutDspKv, (tmp_outputs_configured & OUTPUT_KV ? TRUE : FALSE));
	EnableWindow(hOutDspCsv, (tmp_outputs_configured & OUTPUT_CSV ? TRUE : FALSE));
	EnableWindow(hOutDspJson, (tmp_outputs_configured & OUTPUT_JSON ? TRUE : FALSE));
	EnableWindow(hOutEdtHost, (tmp_outputs_configured & OUTPUT_UDP ? TRUE : FALSE));
	EnableWindow(hOutEdtPort, (tmp_outputs_configured & OUTPUT_UDP ? TRUE : FALSE));

	EnableWindow(hOutBtnKv, (tmp_outputs_configured & OUTPUT_KV ? TRUE : FALSE));
	EnableWindow(hOutBtnCsv, (tmp_outputs_configured & OUTPUT_CSV ? TRUE : FALSE));
	EnableWindow(hOutBtnJson, (tmp_outputs_configured & OUTPUT_JSON ? TRUE : FALSE));
	
	EnableWindow(hOutStdoKv,   (tmp_outputs_configured & OUTPUT_KV ? TRUE : FALSE));
	EnableWindow(hOutStdoCsv,  (tmp_outputs_configured & OUTPUT_CSV ? TRUE : FALSE));
	EnableWindow(hOutStdoJson, (tmp_outputs_configured & OUTPUT_JSON ? TRUE : FALSE));

	EnableWindow(hOutOvrKv,		((tmp_outputs_configured & OUTPUT_KV)	&& tmp_outpath_kv[0]	!= '-' ? TRUE : FALSE));
	EnableWindow(hOutOvrCsv,	((tmp_outputs_configured & OUTPUT_CSV)	&& tmp_outpath_csv[0]	!= '-' ? TRUE : FALSE));
	EnableWindow(hOutOvrJson,	((tmp_outputs_configured & OUTPUT_JSON)	&& tmp_outpath_json[0]	!= '-' ? TRUE : FALSE));

	EnableWindow(hOkBtn, ((!tmp_sampout_configured || tmp_outpath_samp[0]) && (!(tmp_outputs_configured & OUTPUT_KV) || tmp_outpath_kv[0]) && (!(tmp_outputs_configured & OUTPUT_CSV) || tmp_outpath_csv[0]) && (!(tmp_outputs_configured & OUTPUT_JSON) || tmp_outpath_json[0]) && (!(tmp_outputs_configured & OUTPUT_UDP) || (tmp_out_udp_host[0] && tmp_out_udp_port > 0)) ? TRUE : FALSE));
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
	case IDC_RX_OUT_CHK_KV:
		tmp_outputs_configured ^= OUTPUT_KV;
		RefreshWndElements();
		break;
	case IDC_RX_OUT_BTN_KV:
		if (GetSaveFilePath(tmp_outpath_kv, sizeof(tmp_outpath_kv), "Text files (*.txt)\0*.txt\0\0", "Select target text file:", "txt")) tmp_outputs_configured |= OUTPUT_KV;
		RefreshWndElements();
		break;
	case IDC_RX_OUT_STDO_KV:
		if (tmp_outpath_kv[0] != '-') {
			strcpy_s(tmp_outpath_kv, sizeof(tmp_outpath_kv), "-");
			tmp_outputs_configured |= OUTPUT_KV;
		}
		else tmp_outpath_kv[0] = 0;
		RefreshWndElements();
		break;

	case IDC_RX_OUT_CHK_CSV: // if CSV output is active, disable it...
		tmp_outputs_configured ^= OUTPUT_CSV;
		RefreshWndElements();
		break;
	case IDC_RX_OUT_BTN_CSV:
		if (GetSaveFilePath(tmp_outpath_csv, sizeof(tmp_outpath_csv), "CSV files (*.csv)\0*.csv\0\0", "Select target CSV file:", "csv")) tmp_outputs_configured |= OUTPUT_CSV;
		RefreshWndElements();
		break;
	case IDC_RX_OUT_STDO_CSV:
		if (tmp_outpath_csv[0] != '-') {
			strcpy_s(tmp_outpath_csv, sizeof(tmp_outpath_csv), "-");
			tmp_outputs_configured |= OUTPUT_CSV;
		}
		else tmp_outpath_csv[0] = 0;
		RefreshWndElements();
		break;


	case IDC_RX_OUT_CHK_JSON: // if JSON output is active, disable it...
		tmp_outputs_configured ^= OUTPUT_JSON;
		RefreshWndElements();
		break;
	case IDC_RX_OUT_BTN_JSON:
		if (GetSaveFilePath(tmp_outpath_json, sizeof(tmp_outpath_json), "JSON files (*.json)\0*.json\0\0", "Select target JSON file:", "json")) tmp_outputs_configured |= OUTPUT_JSON;
		RefreshWndElements(); // checkmark is re removed here since file selection was cancelled
		break;
	case IDC_RX_OUT_STDO_JSON:
		if (tmp_outpath_json[0] != '-') {
			strcpy_s(tmp_outpath_json, sizeof(tmp_outpath_json), "-");
			tmp_outputs_configured |= OUTPUT_JSON;
		}
		else tmp_outpath_json[0] = 0;
		RefreshWndElements();
		break;

	case IDC_RX_OUT_CHK_UDP:
		if (tmp_outputs_configured & OUTPUT_UDP) tmp_outputs_configured ^= OUTPUT_UDP; // if UDP output is active, disable it...
		else tmp_outputs_configured |= OUTPUT_UDP; // else enable it
		RefreshWndElements();
		break;

	case IDC_RX_OUT_UDP_HOST:
		GetDlgItemText(hDlg, IDC_RX_OUT_UDP_HOST, tmp_out_udp_host, sizeof(tmp_out_udp_host) - 1);
		RefreshWndElements();
		break;

	case IDC_RX_OUT_UDP_PORT: {
		BOOL trans = false;
		DWORD port = GetDlgItemInt(hDlg, IDC_RX_OUT_UDP_PORT, &trans, FALSE);
		tmp_out_udp_port = (unsigned short)((trans && port < 65536) ? port : 0);
		RefreshWndElements();
		break;
	}

	case IDC_RX_OUT_OVR_KV:
		if (tmp_outputs_configured & OUTPUT_KV) {
			tmp_ovrflags_configured ^= OVR_SUBJ_DEC_KV;
			RefreshWndElements();
		}
		break;

	case IDC_RX_OUT_OVR_CSV:
		if (tmp_outputs_configured & OUTPUT_CSV) {
			tmp_ovrflags_configured ^= OVR_SUBJ_DEC_CSV;
			RefreshWndElements();
		}
		break;

	case IDC_RX_OUT_OVR_JSON:
		if (tmp_outputs_configured & OUTPUT_JSON) {
			tmp_ovrflags_configured ^= OVR_SUBJ_DEC_JSON;
			RefreshWndElements();
		}
		break;

	case IDC_RX_OUT_BTN_OK: {
		rtl->cfg->outputs_configured = tmp_outputs_configured;
		strcpy_s(rtl->cfg->output_path_kv, sizeof(rtl->cfg->output_path_kv), tmp_outpath_kv);
		strcpy_s(rtl->cfg->output_path_csv, sizeof(rtl->cfg->output_path_csv), tmp_outpath_csv);
		strcpy_s(rtl->cfg->output_path_json, sizeof(rtl->cfg->output_path_json), tmp_outpath_json);
		strcpy_s(rtl->cfg->out_filename, sizeof(rtl->cfg->out_filename), (tmp_sampout_configured ? tmp_outpath_samp : ""));
		rtl->cfg->overwrite_modes = tmp_ovrflags_configured;
		EndDialog(hDlg, TRUE);
		break;
	}

	case IDC_RX_OUT_BTN_CANCEL:
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

	hOutDspKv = GetDlgItem(hDlg, IDC_RX_OUT_PATH_KV);
	hOutDspCsv = GetDlgItem(hDlg, IDC_RX_OUT_PATH_CSV);
	hOutDspJson = GetDlgItem(hDlg, IDC_RX_OUT_PATH_JSON);

	hOutBtnKv = GetDlgItem(hDlg, IDC_RX_OUT_BTN_KV);
	hOutBtnCsv = GetDlgItem(hDlg, IDC_RX_OUT_BTN_CSV);
	hOutBtnJson = GetDlgItem(hDlg, IDC_RX_OUT_BTN_JSON);

	hOutStdoKv = GetDlgItem(hDlg, IDC_RX_OUT_STDO_KV);
	hOutStdoCsv = GetDlgItem(hDlg, IDC_RX_OUT_STDO_CSV);
	hOutStdoJson = GetDlgItem(hDlg, IDC_RX_OUT_STDO_JSON);

	hOutOvrKv = GetDlgItem(hDlg, IDC_RX_OUT_OVR_KV);
	hOutOvrCsv = GetDlgItem(hDlg, IDC_RX_OUT_OVR_CSV);
	hOutOvrJson = GetDlgItem(hDlg, IDC_RX_OUT_OVR_JSON);

	hOutEdtHost = GetDlgItem(hDlg, IDC_RX_OUT_UDP_HOST);
	hOutEdtPort = GetDlgItem(hDlg, IDC_RX_OUT_UDP_PORT);
	SendMessage(hOutEdtPort, EM_LIMITTEXT, 5, 0);

	hOkBtn = GetDlgItem(hDlg, IDC_RX_OUT_BTN_OK);

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

int ShowOutputDialog(rtl_433_t *rtl_obj, HWND hParent) {
	int r = -1;
	if (rtl_obj) {
		rtl = rtl_obj;
		tmp_outputs_configured = rtl->cfg->outputs_configured;
		strcpy_s(tmp_outpath_kv, sizeof(tmp_outpath_kv), rtl->cfg->output_path_kv);
		strcpy_s(tmp_outpath_csv, sizeof(tmp_outpath_csv), rtl->cfg->output_path_csv);
		strcpy_s(tmp_outpath_json, sizeof(tmp_outpath_json), rtl->cfg->output_path_json);
		strcpy_s(tmp_outpath_samp, sizeof(tmp_outpath_samp), rtl->cfg->out_filename);
		tmp_sampout_configured = (rtl->cfg->out_filename[0] != 0);
		tmp_ovrflags_configured = rtl->cfg->overwrite_modes;
		r = (int) DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_RX_OUTPUT), hParent, (DLGPROC)DialogHandler);
	}
	return r;
}
