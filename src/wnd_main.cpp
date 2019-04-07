/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *              rtl_fl2k_433 main window (core part)               *
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
#include <stdbool.h>

#include "wnd_main.h"
#include "wnd_main_rx.h"
#include "wnd_main_tx.h"
#include "wnd_main_log.h"
#include "logwrap.h"

#include "../res/gui_win_resources.h"

#pragma comment(linker,"\"/manifestdependency:type                  = 'win32' \
                                              name                  = 'Microsoft.Windows.Common-Controls' \
                                              version               = '6.0.0.0' \
                                              processorArchitecture = '*' \
                                              publicKeyToken        = '6595b64144ccf1df' \
                                              language              = '*'\"")

#define REG_PATH "Software\\rtl_fl2k_433" // todo: move to more appropriate place?

#define        WM_CLOSECONFIRMED     (WM_USER + 7)

loglist *lv_log = NULL;

static HANDLE hCRthread = INVALID_HANDLE_VALUE; // thread to coordinate program closing
static HWND hwndMainDlg;
static HMENU historymenu = NULL;

// window geometry
static int min_width = 0;   // initial and minimal width of main window
static int min_height = 0;  // initial and minimal height of main window
static RECT min_clientarea; // initial and minimal client area of main window
static RECT initrect_log;

#define INFILE_CACHE_SIZE 10
static CHAR last_infiles[INFILE_CACHE_SIZE][MAX_PATH];
static DWORD history_ids[INFILE_CACHE_SIZE] = { ID_RX_FILE_HIST0, ID_RX_FILE_HIST1, ID_RX_FILE_HIST2, ID_RX_FILE_HIST3, ID_RX_FILE_HIST4, ID_RX_FILE_HIST5, ID_RX_FILE_HIST6, ID_RX_FILE_HIST7, ID_RX_FILE_HIST8,ID_RX_FILE_HIST9 };

static void filehistory_add(LPSTR fpath) {
	// search if path is already in the list
	INT existing_idx = -1;
	for (int a = 0; a < INFILE_CACHE_SIZE && existing_idx < 0; a++) {
		if (!strcmp(last_infiles[a], fpath)) existing_idx = a;
	}
	// if not existing yet, add new element by removing the last one and shifting the rest
	if (existing_idx < 0) {
		for (int a = (INFILE_CACHE_SIZE - 2); a >= 0; a--) {
			strcpy_s(last_infiles[a + 1], MAX_PATH, last_infiles[a]);
		}
		strcpy_s(last_infiles[0], MAX_PATH, fpath);
	}
	// otherwise, reorder the list by shifting the selected one to the top
	else if(existing_idx > 0){ // no need for reorder if existing_idx == 0
		char tmp[MAX_PATH];
		strcpy_s(tmp, sizeof(tmp), last_infiles[existing_idx]);
		for (int a = existing_idx-1; a >= 0; a--) {
			strcpy_s(last_infiles[a + 1], MAX_PATH, last_infiles[a]);
		}
		strcpy_s(last_infiles[0], MAX_PATH, tmp);
	}
}

static void filehistory_refreshmenu() {
	if (!historymenu) return;

	BOOL delres = TRUE;
	while(delres) delres = DeleteMenu(historymenu, 0, MF_BYPOSITION);

	DWORD idx = 0;
	DWORD cnt = 0;
	for (int a = 0; a < INFILE_CACHE_SIZE; a++) {
		if (!last_infiles[a][0]) continue;
		AppendMenu(historymenu, MF_STRING, history_ids[a], last_infiles[a]);
		cnt++;
	}

	if(!cnt) AppendMenu(historymenu, MF_STRING | MF_GRAYED, -1, "<there are no previous files, yet>");
}

static VOID loadSettings() {
	HKEY regkey;
	DWORD disposition;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &regkey, &disposition) != ERROR_SUCCESS) return;
	if (disposition == REG_CREATED_NEW_KEY) {
		RegCloseKey(regkey);
		return;
	}

	// determine number of historic input files
	for (int a = 0; a < INFILE_CACHE_SIZE; a++) {
		DWORD tmp_size = sizeof(last_infiles[a])-1;
		DWORD type;
		char key[20];
		sprintf_s(key, "Infile_%lu", (a + 1));
		if (RegQueryValueEx(regkey, key, NULL, &type, (LPBYTE)last_infiles[a], &tmp_size) == ERROR_SUCCESS && type == REG_SZ) {
			last_infiles[a][tmp_size] = 0;
		}
		else{
			last_infiles[a][0]=0;
			break;
		}
	}
	RegCloseKey(regkey);
}

static VOID saveSettings() {
	HKEY regkey;
	DWORD disposition;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_PATH, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &regkey, &disposition) != ERROR_SUCCESS) return;

	int valid_paths = 0;
	for (int a = 0; a < INFILE_CACHE_SIZE; a++) {
		if (last_infiles[a][0]){
			char key[20];
			sprintf_s(key, "Infile_%lu", (valid_paths + 1));
			RegSetValueEx(regkey, key, 0, REG_SZ, (LPBYTE)last_infiles[a], (DWORD) strlen(last_infiles[a]) + 1);
			valid_paths++;
		}
	}
	RegCloseKey(regkey);
}

static DWORD WINAPI CloseRequest(LPVOID param) {
	// Ask user to cancel RX and TX threads if they are still running
	BOOL cancel_rx = FALSE;
	BOOL cancel_tx = FALSE;

	if (!TxGui_shutdownRequest()) {
		hCRthread = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	if (!RxGui_shutdownRequest()) {
		hCRthread = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	// Request to cancel RX and TX thread if they are still running
	RxGui_shutdownConfirm();
	TxGui_shutdownConfirm();

	// Wait until the threads are finished (timeout after 2 seconds)
	for (int a = 0; a < 25; a++) {
		if (!RxGui_isActive() && !TxGui_isActive()) break;
		Sleep(200);
	}

	BOOL close_ok = TRUE;
	BOOL result = TRUE;

	// If threads are still active ask to quit anyways
	if (RxGui_isActive() || TxGui_isActive()){
		if (MessageBox(hwndMainDlg, "Cancelling failed. Exit anyways? ('no' returns to program)", "rtl_fl2k_433", MB_YESNO | MB_ICONQUESTION) != IDYES) {
			close_ok = FALSE;
			result = FALSE;
		}
	}

	if(close_ok) PostMessage(hwndMainDlg, WM_CLOSECONFIRMED, 0, 0);
	hCRthread = INVALID_HANDLE_VALUE;
	return result;
}

BOOL CopyToClipboard(LPSTR txt){
	BOOL result = FALSE;

	if (txt) {
		INT txt_len = (INT) strlen(txt) + 1; // including termination
		if (OpenClipboard(hwndMainDlg)) { // Open the clipboard
			EmptyClipboard(); // and empty it
			HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, txt_len);
			if (hglbCopy) {
				LPVOID lptstrCopy = GlobalLock(hglbCopy); // Lock the handle
				memcpy(lptstrCopy, txt, txt_len); // and copy the text to the buffer. 
				GlobalUnlock(hglbCopy);
				SetClipboardData(CF_TEXT, hglbCopy); // Place the handle on the clipboard. 
				result = TRUE;
			}
			CloseClipboard();
		}
	}
	return result;
}

static BOOL onKey(LPNMLVKEYDOWN ctx) {
	if (!ctx) return FALSE;
	//if (ctx->hdr.hwndFrom == TODO) {
	//	...
	//	return TRUE;
	//}
	return FALSE;
}

static BOOL onDoubleClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;
	//if (ctx_in->hwndFrom == TODO) {
	//	...
	//	return TRUE;
	//}
	return FALSE;
}

char copybuf[4096];
static BOOL onRightClick(LPNMHDR ctx_in) {
	if (!ctx_in) return FALSE;

	if (lv_log && ctx_in->hwndFrom == lv_log->getWndHandle()) {
		LPNMITEMACTIVATE ctx = (LPNMITEMACTIVATE)ctx_in;

		INT selidx;
		DWORD cmd = lv_log->getRightClickCommand(ctx, &selidx);
		switch (cmd) {
		case ID_LOG_COPY:
			lv_log->toString(selidx, copybuf, sizeof(copybuf));
			CopyToClipboard(copybuf);
			break;
		case ID_LOG_SAVE:
			lv_log->SaveLog();
			break;
		case ID_LOG_CLEAR:
			lv_log->Clear();
			break;
		default:
			break;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL onChangedItem(LPNMLISTVIEW ctx) {
	if (!ctx) return FALSE;
	//if (ctx->hdr.hwndFrom == TODO) {
	//	...
	//	return TRUE;
	//}
	return FALSE;
}

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

		//case ID_LOG_CLEAR:
		//	if (lv_log) lv_log->Clear();
		//	break;

		//case ID_LOG_SAVEL:
		//	if (lv_log) lv_log->SaveLog();
		//	break;

	case ID_RX_FILE: {
		if (RxGui_isActive()) break; // Do nothing if RX is (still) active (however, in such cases the users should not be able to reach here anyways)

		char fpath[MAX_PATH] = "";
		OPENFILENAME ofntemp = { sizeof(OPENFILENAME),hwndMainDlg,GetModuleHandle(0),"All file types (*.*)\0*.*\0\0",0,0,0,fpath,sizeof(fpath),0,0,0,0,OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_READONLY | OFN_HIDEREADONLY,0,0,0,0,0,0 };
		if (!GetOpenFileName(&ofntemp)) break;
		if (RxGui_passFile(fpath)) {
			filehistory_add(fpath);
			filehistory_refreshmenu();
		}
		break;
	}

	case ID_RX_FILE_HIST0:
	case ID_RX_FILE_HIST1:
	case ID_RX_FILE_HIST2:
	case ID_RX_FILE_HIST3:
	case ID_RX_FILE_HIST4:
	case ID_RX_FILE_HIST5:
	case ID_RX_FILE_HIST6:
	case ID_RX_FILE_HIST7:
	case ID_RX_FILE_HIST8:
	case ID_RX_FILE_HIST9: {
		if (RxGui_isActive()) break; // todo: check if this can practically occur. enhance code, if necessary

									 // find list index
		int idx = -1;
		for (int a = 0; a < INFILE_CACHE_SIZE && idx < 0; a++) {
			if (history_ids[a] == LOWORD(wParam)) idx = a;
		}
		if (idx < 0) break;
		// reopen file
		if (RxGui_passFile(last_infiles[idx])) {
			filehistory_add(last_infiles[idx]);
			filehistory_refreshmenu();
		}
		break;
	}

	case ID_HELP_ABOUT:
		MessageBox(hwndMainDlg, "RX TX prototyping tool. Version: v0.2\r\n\r\nBased on / credits to:\r\n - rtlsdr\r\n - osmo-fl2k\r\n - libusb\r\n - rtl_433\r\n\r\nhttps://github.com/winterrace2", "rtl_fl2k_433", MB_OK | MB_ICONINFORMATION);
		break;

	case ID_PROG_CLOSE:
		if (hCRthread == INVALID_HANDLE_VALUE) {
			hCRthread = CreateThread(0, 0, &CloseRequest, 0, 0, 0);
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static VOID GetChildPos(HWND hChild, HWND hParent, RECT *r){
	POINT p0 = { 0,0 };

	GetWindowRect(hChild, r);
	ClientToScreen(hParent, &p0);
	r->left -= p0.x;
	r->top -= p0.y;
	r->right -= p0.x;
	r->bottom -= p0.y;
}

static VOID onSizeChange(int add_w, int add_h, double fac_w, double fac_h, BOOL redraw) {
	if (lv_log) {
		MoveWindow(lv_log->getWndHandle(), (int)(fac_w*initrect_log.left), initrect_log.top + add_h, (int)(fac_w*(initrect_log.right - initrect_log.left)), initrect_log.bottom - initrect_log.top, redraw);
	}
}

static VOID onInit(HWND hwndDlg) {
	hwndMainDlg = hwndDlg;
	HICON hIcon = LoadIcon(GetModuleHandle(0), (const char *)IDI_ICON1);
	SendMessage(hwndMainDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwndMainDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	HMENU mainmenu = GetMenu(hwndMainDlg);
	HMENU mainmenu_file = GetSubMenu(mainmenu, 0);
	historymenu = GetSubMenu(mainmenu_file, 1);

	RECT wrct;
	if (GetWindowRect(hwndMainDlg, &wrct)) {
		min_width = wrct.right - wrct.left;
		min_height = wrct.bottom - wrct.top;
	}
	GetClientRect(hwndMainDlg, &min_clientarea);

	// Init System log
	HMENU popup_menus = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_POPUPMENUS));
	HMENU pop_log = GetSubMenu(popup_menus, 0);
	HMENU pop_rxlst = GetSubMenu(popup_menus, 1);
	HMENU pop_rxdet = GetSubMenu(popup_menus, 2);
	HMENU pop_rxfreq = GetSubMenu(popup_menus, 3);
	HMENU pop_rxflex = GetSubMenu(popup_menus, 4);
	HMENU pop_rxprot = GetSubMenu(popup_menus, 5);
	HMENU pop_txlst = GetSubMenu(popup_menus, 6);

	lv_log = new loglist(GetDlgItem(hwndMainDlg, IDC_MAIN_LOG), hwndMainDlg, pop_log);
	if(lv_log) GetChildPos(lv_log->getWndHandle(), hwndMainDlg, &initrect_log);

	initLogRedirects(); // creates stdout/sterr redirects in librtlsdr, libusb, pthreads, getopt, osmo-fl2k

	loadSettings();
	filehistory_refreshmenu();

	RxGui_onInit(hwndMainDlg, mainmenu, pop_rxlst, pop_rxdet, pop_rxfreq, pop_rxflex, pop_rxprot);
	TxGui_onInit(hwndMainDlg, pop_txlst);
	Gui_fprintf(stdout, "rtl_fl2k_433 has been started\n");
}

static BOOL onSizing(RECT *r, DWORD opt) {
	if (min_width && min_height) {
		if (r->right - r->left < min_width) {
			if (opt == WMSZ_LEFT || opt == WMSZ_TOPLEFT || opt == WMSZ_BOTTOMLEFT) r->left = r->right - min_width;
			else                                                                            r->right = r->left + min_width;
		}
		if (r->bottom - r->top < min_height) {
			if (opt == WMSZ_TOP || opt == WMSZ_TOPLEFT || opt == WMSZ_TOPRIGHT) r->top = r->bottom - min_height;
			else                                                                         r->bottom = r->top + min_height;
		}
		return TRUE;
	}
	return FALSE;
}

static INT_PTR CALLBACK DialogHandler(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

		case WM_SIZING:
			return onSizing((LPRECT)lParam, (DWORD) wParam);

		case WM_SIZE: {
			int w = LOWORD(lParam);
			int h = HIWORD(lParam);

			int extra_w = max(0, w - (min_clientarea.right));  // amount of extra pixels in horizontal dimension compared to inital size
			int extra_h = max(0, h - (min_clientarea.bottom)); // amount of extra pixels in vertical dimension compared to inital size
			double fac_w = (double)w / (double)min_clientarea.right;  // Scale in horizontal dimension compared to inital size (>= 1.0)
			double fac_h = (double)h / (double)min_clientarea.bottom; // Scale in vertical dimension compared to inital size (>= 1.0)

			onSizeChange(extra_w, extra_h, fac_w, fac_h, FALSE);		// resize core elements of main window (no redrawing)
			RxGui_onSizeChange(extra_w, extra_h, fac_w, fac_h, FALSE);	// resize RX-related elements of main window (no redrawing)
			TxGui_onSizeChange(extra_w, extra_h, fac_w, fac_h, FALSE);	// resize TX-related elements of main window (no redrawing)
			RedrawWindow(hwndMainDlg, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW); // redraw entire main window
			return 0;
		}

		case WM_HSCROLL:
			RxGui_onHorScroll((HWND)lParam);
			break;

		case WM_NOTIFY: {
			switch (((LPNMHDR)lParam)->code) {

			case LVN_KEYDOWN:
				if (!TxGui_onKey((LPNMLVKEYDOWN)lParam) && !RxGui_onKey((LPNMLVKEYDOWN)lParam))
					onKey((LPNMLVKEYDOWN)lParam);
				break;

			case NM_RCLICK:
				if (!TxGui_onRightClick((LPNMHDR)lParam) && !RxGui_onRightClick((LPNMHDR)lParam))
					onRightClick((LPNMHDR)lParam);
				break;

			case NM_DBLCLK:
				if (!TxGui_onDoubleClick((LPNMHDR)lParam) && !RxGui_onDoubleClick((LPNMHDR)lParam))
					onDoubleClick((LPNMHDR)lParam);
				break;

			case LVN_ITEMCHANGED:
				if (!TxGui_onChangedItem((LPNMLISTVIEW)lParam) && !RxGui_onChangedItem((LPNMLISTVIEW)lParam))
					onChangedItem((LPNMLISTVIEW)lParam);
				break;

			default:
				break;
			}
			break;
		}
						
		case WM_COMMAND:
			if (!RxGui_onCommand(wParam, lParam) && !TxGui_onCommand(wParam, lParam))
				onCommand(wParam, lParam);
			break;

		case WM_CLOSECONFIRMED:
			EndDialog(hwndMainDlg, TRUE);
			break;

		case WM_INITDIALOG:
			onInit(hwndDlg);
			break;

		case WM_CLOSE:
			if (hCRthread == INVALID_HANDLE_VALUE) {
				hCRthread = CreateThread(0, 0, &CloseRequest, 0, 0, 0);
			}
			break;

		default:
			return false;
	}
	return true;
}

int GuiMain() {
	// ensure that there is not more than 1 instance of this GUI at this point in time
	if (lv_log || !RxGui_prepare() || !TxGui_prepare()) return 0;

	// Initialize Common Controls (for ListView and others...)
	INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS };
	InitCommonControlsEx(&icex);

	// Open the main window
	int r = (int)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_MAINDLG), 0, (DLGPROC)DialogHandler);

	// Cleanup
	saveSettings(); // Save settings to registry
	if (lv_log) {
		delete lv_log;
		lv_log = NULL;
	}
	for (int a = 0; a < INFILE_CACHE_SIZE; a++)
		last_infiles[a][0] = 0;
	RxGui_cleanup();
	TxGui_cleanup();

	return r;
}
