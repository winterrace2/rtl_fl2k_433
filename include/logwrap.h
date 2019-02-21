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
 
#ifndef LOGWRAP_INCLUDED
	#define LOGWRAP_INCLUDED

	int initLogRedirects();

#ifdef __cplusplus
	extern "C" {
#endif
		int Gui_fprintf(FILE *stream, const char* aFormat, ...);
#ifdef __cplusplus
	}
#endif

#endif // LOGWRAP_INCLUDED
