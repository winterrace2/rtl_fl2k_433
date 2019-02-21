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

#include <windows.h>
#include "commctrl.h"
#include "stdio.h"
#include "wnd_main_rxscope.h"

// static resources for all instances
static HFONT font_legend = NULL;
static HFONT font_splash = NULL;
static HBRUSH hbrBackground = NULL;
static HBRUSH hbrLineOutside = NULL;
static HBRUSH hbrLineInside = NULL;
static HBRUSH hbrFillOutside = NULL;
static HBRUSH hbrFillInside = NULL;

// list of all ScopeWnd instances (needed for window procedure handling). In the current practice (we only create 1 instance for 1 GUI), we won't really need > 1 instances
typedef struct _ScopeInstance{
	HWND hWnd;
	ScopeWnd *obj;
	_ScopeInstance *next;
} ScopeInstance, *pScopeInstance;
static pScopeInstance instances = NULL;

static VOID addInstance(HWND w, ScopeWnd *o) {
	ScopeInstance *newobj = new ScopeInstance();
	if (newobj) {
		newobj->hWnd = w;
		newobj->obj = o;
		newobj->next = NULL;
		pScopeInstance *crnt = &instances;
		while (*crnt) {
			crnt = &(*crnt)->next;
		}
		(*crnt) = newobj;
	}
}

static VOID removeInstance(ScopeWnd *o) {
	pScopeInstance prev = NULL;
	pScopeInstance crnt = instances;
	while (crnt) {
		if (crnt->obj == o) break;
		prev = crnt;
		crnt = crnt->next;
	}
	if (!crnt) return; // Instance not found
	if (prev) prev->next = crnt->next; 
	else instances = crnt->next;
	delete crnt;
}

static ScopeWnd *findInstance(HWND w) {
	pScopeInstance crnt = instances;
	while (crnt) {
		if (crnt->hWnd == w) return crnt->obj;
		crnt = crnt->next;
	}
	return NULL;
}

static VOID InitGfxResources() {
	if(!font_legend) font_legend = CreateFont(8, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "MS Sans Serif");
	if(!font_splash) font_splash = CreateFont(40, 0, 0, 0, FW_DONTCARE, TRUE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, NULL);
	if(!hbrBackground) hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	if(!hbrLineOutside) hbrLineOutside = CreateSolidBrush(RGB(128, 128, 128));
	if(!hbrLineInside) hbrLineInside = CreateSolidBrush(RGB(0, 128, 0));
	if(!hbrFillOutside) hbrFillOutside = CreateSolidBrush(RGB(224, 224, 224));
	if(!hbrFillInside) hbrFillInside = CreateSolidBrush(RGB(192, 255, 192));
}

LRESULT CALLBACK NewGfxWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	ScopeWnd *sc = findInstance(hwndDlg);
	if(!sc) return FALSE;
	return sc->WndProc(hwndDlg, uMsg, wParam, lParam);
}

void ScopeWnd::refreshBox() {
	HDC hdc = GetDC(this->hGfxOut);
	if (!hdc) return;

	InvalidateRect(this->hGfxOut, &this->boxpos, FALSE);
	ReleaseDC(this->hGfxOut, hdc);
	return;
}

ScopeWnd::ScopeWnd(HWND hBox) {
	InitGfxResources();
	addInstance(hBox, this);
	this->hGfxOut = hBox;
	this->src = NULL;
	this->zoom = 1.0;
	this->offset = 0.0;
	OldGfxWndProc = (WNDPROC) SetWindowLongPtr(this->hGfxOut, GWLP_WNDPROC, (LONG_PTR) NewGfxWndProc);
	this->resizeBox(1);
}

ScopeWnd::~ScopeWnd() {
	removeInstance(this);
}

void ScopeWnd::resizeBox(int redraw) {
	// get current dimensions
	GetClientRect(this->hGfxOut, &this->boxpos);
	if(redraw) this->refreshBox();
}

void ScopeWnd::setSource(pPulseDatCompact pdat) {
	this->src = pdat;
	this->refreshBox();
}

int ScopeWnd::isSourceSet() {
	return (this->src != NULL);
}

void ScopeWnd::setZoom(double z) {
	this->zoom = z;
}

double ScopeWnd::getZoom() {
	return this->zoom;
}

void ScopeWnd::setOffset(double o) {
	this->offset = o;
}

double ScopeWnd::getOffset() {
	return this->offset;
}

pulse_state ScopeWnd::getSrcStateAt(double relpos) {
	if(!this->src || relpos < 0.0 || relpos > 1.0) return STATE_LOW_OUTSIDE;
	int s = 0;
	for (unsigned int a = 0; a < src->num_pulses; a++) {
		s += src->pulse[a];
		double endpulse = (double)s / (double)src->num_samples;
		if (endpulse >= relpos) {
			return ((src->segment_len > 0 && a >= src->segment_startidx && a < (src->segment_startidx + src->segment_len)) ? STATE_HIGH_INSIDE :STATE_HIGH_OUTSIDE);
		}
		s += src->gap[a];
		double endgap = (double)s / (double)src->num_samples;
		if (endgap >= relpos) {
			return ((src->segment_len > 0 && a >= src->segment_startidx && a < (src->segment_startidx + src->segment_len)) ? STATE_LOW_INSIDE : STATE_LOW_OUTSIDE);
		}
	}
	return STATE_LOW_OUTSIDE;
}

LRESULT CALLBACK ScopeWnd::WndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (hwndDlg != hGfxOut) return NULL;	// should never occur

	if (uMsg == WM_PAINT) {
		// 1) prepare painting
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwndDlg, &ps);


		// 2) Draw the signal
		if (this->src && this->src->num_pulses > 0 && this->zoom >= 1.0 && this->offset <= 1.0 && this->offset >= 0.0) {
			RECT rDest = ps.rcPaint; // the area which needs to be updated

			int bar_low = this->boxpos.bottom - 10;
			int bar_hi = 24;

			double wnd_left = this->offset;	// signal window which is currently visible (can reach beyond end of the signal)
			double wnd_right = this->offset + (1.0 / this->zoom);
			// determine the signal state left of the drawing area
			int prev_idx = max(rDest.left-1, 0);
			double wnd_relpos = (double)prev_idx / ((double) this->boxpos.right); // relative position of the pixel column within the entire gfx box
			pulse_state prevstate = this->getSrcStateAt(wnd_left + (wnd_relpos*(wnd_right-wnd_left))); // signal value (1/0) at this position

			// draw the signal within the drawing area (rDest.top/.bottom is currently not used to further limit the drawing activity)
			RECT line_hor;
			RECT line_vert;
			RECT filling;
			RECT background;
			pulse_state crntstate;
			for (int x = rDest.left; x < rDest.right; x++) { // pixel columns left and right to the drawing area are not painted (rDest might be a fraction of boxpos)
				wnd_relpos = (double)x / ((double) this->boxpos.right);	// relative position of the pixel column within the entire gfx box
				crntstate = this->getSrcStateAt(wnd_left + (wnd_relpos*(wnd_right-wnd_left))); // signal value (1/0) at this position
				if (crntstate == prevstate) continue; // only draw a pulse after reaching its end
				// draw horizontal line
				line_hor.left = prev_idx;								
				line_hor.right = x;										
				line_hor.top = (prevstate == STATE_HIGH_OUTSIDE || prevstate == STATE_HIGH_INSIDE ? bar_hi : bar_low);
				line_hor.bottom = (prevstate == STATE_HIGH_OUTSIDE || prevstate == STATE_HIGH_INSIDE ? bar_hi+1 : bar_low+1);
				FillRect(hdc, &line_hor, (prevstate == STATE_HIGH_INSIDE || prevstate == STATE_LOW_INSIDE ? hbrLineInside : hbrLineOutside));
				// draw filling
				filling.left = prev_idx;
				filling.right = x-1;
				filling.top = (prevstate == STATE_HIGH_OUTSIDE || prevstate == STATE_HIGH_INSIDE ? bar_hi+1 : bar_low + 1);
				filling.bottom = rDest.bottom;
				FillRect(hdc, &filling, (prevstate == STATE_HIGH_INSIDE || prevstate == STATE_LOW_INSIDE ? hbrFillInside : hbrFillOutside));
				// draw background above low pulses
				if (prevstate == STATE_LOW_OUTSIDE || prevstate == STATE_LOW_INSIDE) {
					background.left = prev_idx;
					background.right = x;
					background.top = bar_hi;
					background.bottom = bar_low;
					FillRect(hdc, &background, hbrBackground);
				}
				// draw vertical line
				if (x > 0) {
					line_vert.left = x - 1;
					line_vert.right = x;
					line_vert.top = bar_hi;
					line_vert.bottom = bar_low+1;
					FillRect(hdc, &line_vert, (prevstate == STATE_HIGH_INSIDE || prevstate == STATE_LOW_INSIDE ? hbrLineInside : hbrLineOutside));
					line_vert.left = x - 1;
					line_vert.right = x;
					line_vert.top = bar_low + 1;
					line_vert.bottom = rDest.bottom;
					FillRect(hdc, &line_vert, (prevstate == STATE_HIGH_INSIDE || prevstate == STATE_LOW_INSIDE ? hbrFillInside : hbrFillOutside));
				}
				// proceed
				prevstate = crntstate;
				prev_idx = x;
			}
			// draw the last pulse (even if not completely visible)
			line_hor.left = prev_idx; // horizontal line
			line_hor.right = rDest.right;
			line_hor.top = (prevstate == STATE_HIGH_OUTSIDE || prevstate == STATE_HIGH_INSIDE ? bar_hi : bar_low);
			line_hor.bottom = (prevstate == STATE_HIGH_OUTSIDE || prevstate == STATE_HIGH_INSIDE ? bar_hi + 1 : bar_low + 1);
			FillRect(hdc, &line_hor, (prevstate == STATE_HIGH_INSIDE || prevstate == STATE_LOW_INSIDE ? hbrLineInside : hbrLineOutside));
			filling.left = prev_idx; // filling
			filling.right = rDest.right;
			filling.top = (prevstate == STATE_HIGH_OUTSIDE || prevstate == STATE_HIGH_INSIDE ? bar_hi + 1 : bar_low + 1);
			filling.bottom = rDest.bottom;
			FillRect(hdc, &filling, (prevstate == STATE_HIGH_INSIDE || prevstate == STATE_LOW_INSIDE ? hbrFillInside : hbrFillOutside));
			if (prevstate == STATE_LOW_OUTSIDE || prevstate == STATE_LOW_INSIDE) { // background above low pulses
				background.left = prev_idx;
				background.right = rDest.right;
				background.top = bar_hi;
				background.bottom = bar_low;
				FillRect(hdc, &background, hbrBackground);
			}

			// draw the legend
			RECT legend = this->boxpos;
			legend.bottom = bar_hi;
			RECT updrct;
			if(IntersectRect(&updrct, &legend, &ps.rcPaint)){
				char test[60] = "";
				int smp_start = (int)(wnd_left*src->num_samples);
				int smp_end = min((int)(wnd_right*src->num_samples), (int)src->num_samples);
				double frac = 100.0 * (double)(smp_end - smp_start) / (double)src->num_samples;
				sprintf_s(test, sizeof(test), "Displaying samples %lu - %lu (%.02f%%)", smp_start, smp_end, frac);
				SelectObject(hdc, font_legend);
				SetTextColor(hdc, RGB(0, 0, 0));
				SetBkMode(hdc, TRANSPARENT);
				FillRect(hdc, &updrct, hbrBackground);
				DrawText(hdc, test, -1, &legend, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
			}
		}
		// 3) Or draw splash text
		else {
			RECT updrct;
			if (IntersectRect(&updrct, &this->boxpos, &ps.rcPaint)) {
				SelectObject(hdc, font_splash);
				SetTextColor(hdc, RGB(240, 240, 240));
				SetBkMode(hdc, TRANSPARENT);
				FillRect(hdc, &updrct, hbrBackground);
				DrawText(hdc, "rtl_fl2k_433       rtl_fl2k_433       rtl_fl2k_433", -1, &this->boxpos, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX);
			}
		}

		// 4) end painting
		EndPaint(hwndDlg, &ps);
		return 0;
	}

	// All other windows messages are processed by the original window procedure
	return CallWindowProc(this->OldGfxWndProc, hwndDlg, uMsg, wParam, lParam);
}
