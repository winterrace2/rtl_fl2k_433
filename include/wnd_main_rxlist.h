/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *    Main window element: ListView for incoming (RX) messages     *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_MAIN_RXLIST_INCLUDED
	#define WND_MAIN_RXLIST_INCLUDED

	#define RX_COLUMN_TIME_WIDTH 110
	#define RX_COLUMN_MOD_WIDTH 40
	#define RX_COLUMN_TEXT_WIDTH 250

	#include "rxlist_entry.h"

class rxlist {
	public:
		rxlist(HWND hLogWnd, HWND hParent, HMENU popup_menu);	// Konstruktor
		~rxlist();				// Destruktor

		VOID AddRxEntry(rx_entry *entry);	// Ein RX-Telegramm anhängen
		INT RemoveRxEntry(int obj_idx);
		INT GetRxEntryCount();
		INT GetSelectedItem();
		VOID Clear();						// RX-Liste leeren
		HWND getWndHandle();
		VOID setActiveStyle(BOOL active);
		data_t *rxlist::getData(int obj_idx);
		pPulseDatCompact getPulses(int obj_idx);
		DWORD getRightClickCommand(LPNMITEMACTIVATE dat, INT *selidx);
		rx_entry *GetRxEntry(int obj_idx);

	private:
		HWND hList;	// Handle des Logfensters
		HWND hParWnd;
		HMENU hPopup;
};

#endif // WND_MAIN_RXLIST_INCLUDED