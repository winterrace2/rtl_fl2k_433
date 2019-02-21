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

#ifndef WND_TEXTREADER_INCLUDED
#define WND_TEXTREADER_INCLUDED

LPSTR ShowTextReaderDialog(HWND hParent, LPSTR caption, LPSTR preset, DWORD maxbuf);

#endif // WND_TEXTREADER_INCLUDED
