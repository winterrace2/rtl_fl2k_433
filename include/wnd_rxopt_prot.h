/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *        Option sub-dialog for radio protocol selection           *
 *                                                                 *
 *    coded in 2018/19 by winterrace (github.com/winterrace)       *
 *                                   (github.com/winterrace2)      *
 *                                                                 *
 * Basic actions performed by a call to ShowProtocolDialog():      *
 * - updates cfg->active_prots                                     *
 *                                                                 *
 * If the dialog is cancelled, no changes are made                 *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License as  *
 * published by the Free Software Foundation; either version 2 of  *
 * the License, or (at your option) any later version.             *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef WND_RXOPT_PROT_INCLUDED
	#define WND_RXOPT_PROT_INCLUDED

	#include "librtl_433.h"

	#define OPT_PROT_NAMECOL_WIDTH  300
	#define OPT_PROT_PARAM_WIDTH    100
	#define OPT_PROT_STATECOL_WIDTH 80

	int ShowProtocolDialog(rtl_433_t *rl, HWND hParent, HMENU hPopupMenu);

#endif // WND_RXOPT_PROT_INCLUDED
