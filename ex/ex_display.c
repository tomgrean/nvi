/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994, 1995
 *	Keith Bostic.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_display.c,v 10.8 1995/09/21 12:07:07 bostic Exp $ (Berkeley) $Date: 1995/09/21 12:07:07 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../common/common.h"
#include "tag.h"

static int	bdisplay __P((SCR *));
static void	db __P((SCR *, CB *, CHAR_T *));

/*
 * ex_display -- :display b[uffers] | s[creens] | t[ags]
 *
 *	Display buffers, tags or screens.
 *
 * PUBLIC: int ex_display __P((SCR *, EXCMD *));
 */
int
ex_display(sp, cmdp)
	SCR *sp;
	EXCMD *cmdp;
{
	switch (cmdp->argv[0]->bp[0]) {
	case 'b':
#undef	ARG
#define	ARG	"buffers"
		if (cmdp->argv[0]->len >= sizeof(ARG) ||
		    memcmp(cmdp->argv[0]->bp, ARG, cmdp->argv[0]->len))
			break;
		return (bdisplay(sp));
	case 's':
#undef	ARG
#define	ARG	"screens"
		if (cmdp->argv[0]->len >= sizeof(ARG) ||
		    memcmp(cmdp->argv[0]->bp, ARG, cmdp->argv[0]->len))
			break;
		return (ex_sdisplay(sp));
	case 't':
#undef	ARG
#define	ARG	"tags"
		if (cmdp->argv[0]->len >= sizeof(ARG) ||
		    memcmp(cmdp->argv[0]->bp, ARG, cmdp->argv[0]->len))
			break;
		return (ex_tagdisplay(sp));
	}
	ex_message(sp, cmdp->cmd->usage, EXM_USAGE);
	return (1);
}

/*
 * bdisplay --
 *
 *	Display buffers.
 */
static int
bdisplay(sp)
	SCR *sp;
{
	CB *cbp;

	if (sp->gp->cutq.lh_first == NULL && sp->gp->dcbp == NULL) {
		msgq(sp, M_INFO, "123|No cut buffers to display");
		return (0);
	}

	/* Display regular cut buffers. */
	for (cbp = sp->gp->cutq.lh_first; cbp != NULL; cbp = cbp->q.le_next) {
		if (isdigit(cbp->name))
			continue;
		if (cbp->textq.cqh_first != (void *)&cbp->textq)
			db(sp, cbp, NULL);
		if (INTERRUPTED(sp))
			return (0);
	}
	/* Display numbered buffers. */
	for (cbp = sp->gp->cutq.lh_first; cbp != NULL; cbp = cbp->q.le_next) {
		if (!isdigit(cbp->name))
			continue;
		if (cbp->textq.cqh_first != (void *)&cbp->textq)
			db(sp, cbp, NULL);
		if (INTERRUPTED(sp))
			return (0);
	}
	/* Display default buffer. */
	if ((cbp = sp->gp->dcbp) != NULL)
		db(sp, cbp, "default buffer");
	return (0);
}

/*
 * db --
 *	Display a buffer.
 */
static void
db(sp, cbp, name)
	SCR *sp;
	CB *cbp;
	CHAR_T *name;
{
	CHAR_T *p;
	GS *gp;
	TEXT *tp;
	size_t len;

	gp = sp->gp;
	(void)ex_printf(sp, "********** %s%s\n",
	    name == NULL ? KEY_NAME(sp, cbp->name) : name,
	    F_ISSET(cbp, CB_LMODE) ? " (line mode)" : " (character mode)");
	for (tp = cbp->textq.cqh_first;
	    tp != (void *)&cbp->textq; tp = tp->q.cqe_next) {
		for (len = tp->len, p = tp->lb; len--; ++p) {
			(void)ex_puts(sp, KEY_NAME(sp, *p));
			if (INTERRUPTED(sp))
				return;
		}
		(void)ex_puts(sp, "\n");
	}
}
