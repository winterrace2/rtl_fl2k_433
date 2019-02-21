/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *    Main window element: Scope to visualize message pulse data   *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_MAIN_RX_SCOPE_INCLUDED
	#define WND_MAIN_RX_SCOPE_INCLUDED
	#include "rxlist_entry.h"

typedef enum {
	STATE_LOW_OUTSIDE  = 0,
	STATE_LOW_INSIDE   = 1,
	STATE_HIGH_INSIDE  = 2,
	STATE_HIGH_OUTSIDE = 3
} pulse_state;

class ScopeWnd {
public:
	ScopeWnd(HWND hBox);					// specify handle of the picture element
	~ScopeWnd();

	void refreshBox();						// trigger redrawing
	void resizeBox(int redraw);				// adjust to current dimensions and repaint
	void setSource(pPulseDatCompact pdat);	// change the data source and redraw
	int  isSourceSet();						// returns > 0 if a source is currently set
	void setZoom(double z);					// change the current zoom (z >= 1.0)
	void setOffset(double o);				// change the current offset (0.0 <= o <= 1.0)
	double getZoom();						// get the current zoom
	double getOffset();						// get the current offset
	LRESULT CALLBACK WndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	HWND hGfxOut;							// handle of the picture element
	pPulseDatCompact src;					// data source
	double zoom;							// current zoom (zoom >= 1.0)
	double offset;							// current offset (0.0 <= offset <= 1.0)
	RECT boxpos;							// Position der Gfx-Box
	WNDPROC OldGfxWndProc;					// Address of original WNDPROC
	pulse_state getSrcStateAt(double relpos);		// Gets value of signal at position relpos (0.0 <= relpos <= 1.0)
};

#endif // WND_MAIN_RX_SCOPE_INCLUDED
