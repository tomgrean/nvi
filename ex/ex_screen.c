/*-
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_screen.c,v 8.16 1994/08/31 17:17:20 bostic Exp $ (Berkeley) $Date: 1994/08/31 17:17:20 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "compat.h"
#include <db.h>
#include <regex.h>

#include "vi.h"
#include "excmd.h"

/*
 * ex_split --	:s[plit] [file ...]
 *	Split the screen, optionally setting the file list.
 */
int
ex_split(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	return (sp->s_split(sp, cmdp->argc ? cmdp->argv : NULL, cmdp->argc));
}

/*
 * ex_bg --	:bg
 *	Hide the screen.
 */
int
ex_bg(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	return (sp->s_bg(sp));
}

/*
 * ex_fg --	:fg [file]
 *	Show the screen.
 */
int
ex_fg(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	return (sp->s_fg(sp, cmdp->argc ? cmdp->argv[0]->bp : NULL));
}

/*
 * ex_resize --	:resize [+-]rows
 *	Change the screen size.
 */
int
ex_resize(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	enum adjust adj;

	if (!F_ISSET(cmdp, E_COUNT)) {
		ex_message(sp, cmdp, EXM_USAGE);
		return (1);
	}
	if (F_ISSET(cmdp, E_COUNT_NEG))
		adj = A_DECREASE;
	else if (F_ISSET(cmdp, E_COUNT_POS))
		adj = A_INCREASE;
	else
		adj = A_SET;
	return (sp->s_rabs(sp, cmdp->count, adj));
}

/*
 * ex_sdisplay --
 *	Display the list of screens.
 */
int
ex_sdisplay(sp, ep)
	SCR *sp;
	EXF *ep;
{
	SCR *tsp;
	int cnt, col, len, sep;

	if ((tsp = sp->gp->hq.cqh_first) == (void *)&sp->gp->hq) {
		(void)ex_printf(EXCOOKIE,
		    "No backgrounded screens to display.\n");
		return (0);
	}

	col = len = sep = 0;
	for (cnt = 1; tsp != (void *)&sp->gp->hq; tsp = tsp->q.cqe_next) {
		col += len = strlen(tsp->frp->name) + sep;
		if (col >= sp->cols - 1) {
			col = len;
			sep = 0;
			(void)ex_printf(EXCOOKIE, "\n");
		} else if (cnt != 1) {
			sep = 1;
			(void)ex_printf(EXCOOKIE, " ");
		}
		(void)ex_printf(EXCOOKIE, "%s", tsp->frp->name);
		++cnt;
	}
	(void)ex_printf(EXCOOKIE, "\n");
	return (0);
}
