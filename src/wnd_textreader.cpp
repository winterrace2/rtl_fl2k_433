/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *          Sub-dialog to let the user enter some text             *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * If the user entered some text, the dialog returns a pointer     *
 * If the user cancelled, a NULL pointer is returned               *
 *                                                                 *
 * The calling code has to ensure an instant evaluatation or copy  *
 * the resulting string before the next call to this dialog since  *
 * it uses a local buffer which can be changed or re-allocated     *
 * during the next call of this dialog                             *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include <malloc.h>

#include "wnd_textreader.h"

#include "../res/gui_win_resources.h"

static HWND hDlg;
static char *buf_ptr = NULL;
static DWORD buf_cap = 0;
static LPSTR wnd_caption = NULL;

static INT_PTR CALLBACK EditDialogHandler(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {

	case WM_COMMAND:
		switch (LOWORD(wParam)) {

		case IDC_TEXTREADER_OK: {
			GetDlgItemText(hDlg, IDC_TEXTREADER_EDIT, buf_ptr, buf_cap - 1);
			EndDialog(hDlg, TRUE);
			break;
		}

		case IDC_TEXTREADER_CANCEL:
			EndDialog(hDlg, TRUE);
			break;
		}
		break;

	case WM_INITDIALOG: {
		hDlg = hwndDlg;
		HICON hIcon = LoadIcon(GetModuleHandle(0), (const char *)IDI_ICON1);
		SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		if (wnd_caption) SetWindowText(hDlg, wnd_caption);
		SetDlgItemText(hDlg, IDC_TEXTREADER_EDIT, buf_ptr);
		break;
	}

	case WM_CLOSE:
		EndDialog(hDlg, TRUE);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

LPSTR ShowTextReaderDialog(HWND hParent, LPSTR caption, LPSTR preset, DWORD maxbuf) {
	if (!buf_ptr || buf_cap < maxbuf) {
		buf_ptr = (char*) realloc(buf_ptr, maxbuf);
		buf_cap = maxbuf;
	}
	LPSTR result = NULL;
	if (buf_ptr) {
		strcpy_s(buf_ptr, buf_cap, (preset ? preset : ""));
		wnd_caption = caption;
		int confirmed = (int)DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_TEXTREADER), hParent, (DLGPROC)EditDialogHandler);
		if(confirmed) result = buf_ptr;
	}
	return result;
}
