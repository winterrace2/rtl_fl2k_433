/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *    Interfacing of (3rd party) libraries used by rtl_fl2k_433    *
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
#include <cstdarg>
#include <stdio.h>
#include "logwrap.h"
#include "librtl_433.h"
#include "rtl-sdr.h"
#include "osmo-fl2k.h"
#include "libfl2k_433.h"
#include "wnd_main_log.h"
#include "wnd_main.h"

extern loglist	*lv_log;

#define MAX_PRINT_LEN 5000  // maximum length of single print call is for rtl_433 usage instructions
static char gui_printbuf[MAX_PRINT_LEN];

int Gui_fprintf(FILE *stream, const char* aFormat, ...) {
	va_list argptr;
	int rv = 0;

	va_start(argptr, aFormat);

	static char printbuf[512]; // local default buffer (stack). Sufficient size for most use cases.
	char *buf = printbuf; // we use the local buffer if possible
	rv = vsnprintf(NULL, 0, aFormat, argptr) + 1; // test how much space we really need
	int needed_cap = rv;
	if (needed_cap >= 0) {
		int need_more_mem = (needed_cap > sizeof(printbuf) ? 1 : 0); // if we need more space, we...
		if (need_more_mem) buf = (char*) calloc(1, needed_cap + 10); // ...allocate our buffer dynamically on the heap
		rv = vsprintf(buf, aFormat, argptr);
		// call the callback function
		if (rv >= 0) lv_log->AddLogEntry("rtl_fl2k_433", (stream == stderr ? LOG_TRG_STDERR : LOG_TRG_STDOUT), buf);
		// free the dynamic buffer (if used)
		if (need_more_mem) free(buf);
	}
	va_end(argptr);

	if (rv < 0) rv = -1;
	return rv;
}

static void wrapPrint_rtl433(char target, char *text, void *ctx) {
	if (lv_log) lv_log->AddLogEntry("librtl_433", target, text);
}

static void wrapPrint_librtlsdr(char target, char *text, void *ctx) {
	if (lv_log) lv_log->AddLogEntry("librtlsdr", target, text);
}

static void wrapPrint_libosmofl2k(char target, char *text, void *ctx) {
	if (lv_log) lv_log->AddLogEntry("libosmo-fl2k", target, text);
}

static void wrapPrint_libfl2k_433(char target, char *text, void *ctx) {
	if (lv_log) lv_log->AddLogEntry("libfl2k_433", target, text);
}

static void wrapPrint_getopt(char target, char *text, void *ctx) {
	if (lv_log) lv_log->AddLogEntry("getopt_win", target, text);
}

int initLogRedirects() {
	void (WINAPI *redirfn_p)(std_print_wrapper, void*);
	int r = 1;
	int r_tmp;

	// interfacing librtl_433
#ifdef librtl_433_STATIC
	rtl433_print_redirection(wrapPrint_rtl433, NULL);
#else
	r_tmp = 1;
	HMODULE hDllRtl433 = GetModuleHandle("librtl_433.dll");
	if (hDllRtl433 != NULL) {
		redirfn_p = (void (WINAPI*)(std_print_wrapper, void*)) GetProcAddress(hDllRtl433, "rtl433_print_redirection");
		if (redirfn_p) redirfn_p(wrapPrint_rtl433, NULL);
		else r_tmp = 0;
	}
	else r_tmp = 0;
	if (!r_tmp) {
		Gui_fprintf(stderr, "Print wrapper could not be registered for librtl_433, so no output messages from that library will be visible in this log.");
		r = 0;
	}

#endif

	// interfacing librtlsdr
#ifdef rtlsdr_STATIC
	rtlsdr_print_redirection(wrapPrint_librtlsdr, NULL);
#else
	r_tmp = 1;
	HMODULE hDllRtl = GetModuleHandle("rtlsdr.dll");
	if (hDllRtl != NULL) {
		redirfn_p = (void (WINAPI*)(std_print_wrapper, void*)) GetProcAddress(hDllRtl, "rtlsdr_print_redirection");
		if (redirfn_p) redirfn_p(wrapPrint_librtlsdr, NULL);
		else r_tmp = 0;
	}
	else r_tmp = 0;
	if (!r_tmp) {
		Gui_fprintf(stderr, "Print wrapper could not be registered for rtlsdr, so no output messages from that library will be visible in this log.");
		r = 0;
	}
#endif

	// interfacing getopt
#ifdef STATIC_GETOPT
	getopt_print_redirection(wrapPrint_getopt, NULL);
#else
	r_tmp = 1;
	HMODULE hDllGetopt = GetModuleHandle("getopt.dll");
	if (!hDllGetopt) hDllGetopt = GetModuleHandle("getoptd.dll");
	if (hDllGetopt != NULL) {
		redirfn_p = (void (WINAPI*)(std_print_wrapper, void*)) GetProcAddress(hDllGetopt, "getopt_print_redirection");
		if (redirfn_p) redirfn_p(wrapPrint_getopt, NULL);
		else r_tmp = 0;
	}
	else r_tmp = 0;
	if (!r_tmp) {
		Gui_fprintf(stderr, "Print wrapper could not be registered for getopt, so no output messages from that library will be visible in this log.");
		r = 0;
	}
#endif

	// interfacing osmo-fl2k
#ifdef libosmofl2k_STATIC
	fl2k_print_redirection(wrapPrint_libosmofl2k, NULL);
#else
	r_tmp = 1;
	HMODULE hDllFl2k = GetModuleHandle("osmo-fl2k.dll");
	if (hDllFl2k != NULL) {
		redirfn_p = (void (WINAPI*)(std_print_wrapper, void*)) GetProcAddress(hDllFl2k, "fl2k_print_redirection");
		if (redirfn_p) redirfn_p(wrapPrint_libosmofl2k, NULL);
		else r_tmp = 0;
	}
	else r_tmp = 0;
	if (!r_tmp) {
		Gui_fprintf(stderr, "Print wrapper could not be registered for osmo-fl2k, so no output messages from that library will be visible in this log.");
		r = 0;
	}
#endif

	// interfacing libfl2k_433.
#ifdef libfl2k_433_STATIC
	fl2k433_print_redirection(wrapPrint_libfl2k_433, NULL);
#else
	r_tmp = 1;
	HMODULE hDllFl2k433 = GetModuleHandle("libfl2k_433.dll");
	if (hDllFl2k433 != NULL) {
		redirfn_p = (void (WINAPI*)(std_print_wrapper, void*)) GetProcAddress(hDllFl2k433, "fl2k433_print_redirection");
		if (redirfn_p) redirfn_p(wrapPrint_libfl2k_433, NULL);
		else r_tmp = 0;
	}
	else r_tmp = 0;
	if (!r_tmp) {
		Gui_fprintf(stderr, "Print wrapper could not be registered for libfl2k_433, so no output messages from that library will be visible in this log.");
		r = 0;
	}
#endif

	return r;
}

