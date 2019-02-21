/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                 Main window element: System Log                 *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_MAIN_LOG_INCLUDED
	#define WND_MAIN_LOG_INCLUDED

	#define LOG_TRG_STDERR 1
	#define LOG_TRG_STDOUT 2

	#define LOG_COLUMN_TIME_WIDTH 80	
	#define LOG_COLUMN_MOD_WIDTH 80	
	#define LOG_COLUMN_TEXT_WIDTH 700

	#define	LOG_MAX_LINE_CHARS		160 // Maximum length of a line in characters. Longer lines will be wrapped (preferably before the last word)
	#define LOG_MAX_LOOKBACKCHARS	24  // Maximum amount of lookback characters when looking for a non-alpha character (otherwise the wrap will occur within the word).

	#if (LOG_MAX_LOOKBACKCHARS >= LOG_MAX_LINE_CHARS)
	#error "LOG_MAX_LOOKBACKCHARS needs to be significantly smaller than LOG_MAX_LINE_CHARS!"
	#endif

class loglist {
	public:
		loglist(HWND hLogWnd, HWND hParent, HMENU popup_menu);	// Konstruktor
		~loglist();			// Destruktor
		
		VOID AddLogEntry(LPSTR module, INT trg, LPSTR str, BOOL notime=FALSE);	// Eine Lognachricht anhängen und ausgeben
		VOID Clear();									// Logpuffer leeren
		HWND getWndHandle();
		BOOL SaveLog();									// Log in eine Textdatei speichern
		VOID toString(INT id, CHAR *buf, INT cap);
		DWORD getRightClickCommand(LPNMITEMACTIVATE dat, INT *selidx);

	private:
		char crntline[LOG_MAX_LINE_CHARS+10];
		int crntline_len;
		HWND hList;	// Handle des Logfensters
		HWND hParWnd;
		HMENU hPopup;
};

#endif // WND_MAIN_LOG_INCLUDED