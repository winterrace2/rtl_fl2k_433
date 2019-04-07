/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *    Main window element: ListView for outgoing (TX) messages     *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 
#ifndef WND_MAIN_TXLIST_INCLUDED
	#define WND_MAIN_TXLIST_INCLUDED

	#define TX_COLUMN_DSCR_WIDTH 260
	#define TX_COLUMN_MOD_WIDTH   40
	#define TX_COLUMN_SENT_WIDTH  90

	#include "txlist_entry.h"

class txlist {
	public:
		txlist(HWND hLogWnd, HWND hParent, HMENU popup_menu);	// Konstruktor
		~txlist();				// Destruktor

		VOID AddTxEntry(tx_entry *entry);	// Ein RX-Telegramm anhängen
		INT RemoveTxEntry(int obj_idx);
		INT CountAllEntries();
		INT CountUnsentEntries();
		BOOL hasEntryBeenSent(int obj_idx);
		INT GetSelectedItem();
		VOID Clear();						// RX-Liste leeren
		VOID refreshLine(int obj_idx);
		HWND getWndHandle();
		VOID setActiveStyle(BOOL active);
		char *getStrView(int obj_idx, char *buf, int cap);
		DWORD getRightClickCommand(LPNMITEMACTIVATE dat, INT *selidx);
		tx_entry *GetTxEntry(INT obj_idx);
		tx_entry *GetFirstUnsentEntry(INT *obj_idx);

private:
		HWND hList;	// Handle des Logfensters
		HWND hParWnd;
		HMENU hPopup;
		VOID refreshLine(int idx, tx_entry *entry);
};

#endif // WND_MAIN_TXLIST_INCLUDED