/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *    Implementation of the ListView listing RX signal details     *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_MAIN_RXDETAILS_INCLUDED
	#define WND_MAIN_RXDETAILS_INCLUDED

	#define RX_COLUMN_KEY 140
	#define RX_COLUMN_VALUE 220

class rxdetails {
	public:
		rxdetails(HWND hLogWnd, HWND hParent, HMENU popup_menu);
		~rxdetails();

		VOID DisplayData(data_t *data);
		HWND getWndHandle();
		DWORD getRightClickCommand(LPNMITEMACTIVATE dat, INT *selidx);
		VOID toString(INT id, CHAR *buf, INT cap);

	private:
		HWND hList;	// Handle of log window
		HWND hParWnd;
		HMENU hPopup;
};

#endif // WND_MAIN_RXDETAILS_INCLUDED