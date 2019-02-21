/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                    sub-dialog for TX warning                    *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowDeviceTxDialog():      *
 * - displays warning message and returns TRUE if user accepted    *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <windows.h>
#include "wnd_txwarn.h"
#include "../res/gui_win_resources.h"

static BOOL accepted = FALSE;
static HWND hDlg;

/* Warning message (taken from https://osmocom.org/projects/osmo-fl2k/wiki as of November 23rd, 2018) */ 

#define WARNING_TITLE "Legal Aspects / Keeping the RF Spectrum Clean"

#define WARNING_MSG "If you are operating a radio transmitter of any sort, particularly a DIY solution or a SDR transmitter, it is assumed that you are familiar with both the legal aspects of radio transmissions, as well as the technical aspects of it. Do not operate a radio transmitter unless you are clear of the legal and regulatory requirements. In most jurisdictions the operation of homebrew / DYI transmitters requires at the very least an amateur radio license.\r\n\r\n" \
"The raw, unfiltered DAC output contains lots of harmonics at multiples of the base frequency. This is what is creatively (ab)used if you use osmo-fl2k to generate a signal much higher than what you could normally achieve with a ~ 165MHz DAC without upconversion. However, this means that the frequency spectrum will contain not only the one desired harmonic, but all the lower harmonics as well as the base frequency.\r\n\r\n" \
"Before transmitting any signals with an FL2000 device, it is strongly suggested that you check the resulting spectrum with a spectrum analyzer, and apply proper filtering to suppress any but the desired transmit frequency.\r\n\r\n" \
"Operating a transmitter with the unfiltered FL2000 DAC output attached to an antenna outside a RF shielding chamber is dangerous. Don't do it!\r\n\r\n" \
"You have the following alternatives to broadcasting over the air:\r\n" \
" · add proper output band pass filtering for your desired TX frequency, or\r\n" \
" · operate transmitter + receiver in a shielded RF box / chamber, or\r\n" \
" · connect transmitter + receiver over coaxial cable (with proper attenuators)"

static BOOL onCommand(WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {

	case IDC_TX_WARN_OK:
		accepted = (IsDlgButtonChecked(hDlg, IDC_TX_WARN_CHK) == BST_CHECKED);
		EndDialog(hDlg, 1);
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

	SetWindowText(hDlg, WARNING_TITLE);
	SetDlgItemText(hDlg, IDC_TX_WARN_TXT, WARNING_MSG);
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
		return FALSE;
	}
	return TRUE;
}

BOOL ShowWarningDialog(HWND hParent){
	if(!accepted)
		DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_TX_WARN), hParent, (DLGPROC)DialogHandler);
	return accepted;
}
