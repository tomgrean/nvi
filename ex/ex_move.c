/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_move.c,v 5.21 1993/02/28 14:00:41 bostic Exp $ (Berkeley) $Date: 1993/02/28 14:00:41 $";
#endif /* not lint */

#include <sys/types.h>

#include <limits.h>
#include <stdio.h>

#include "vi.h"
#include "excmd.h"
#include "screen.h"

enum which {COPY, MOVE};
static int cm __P((EXF *, EXCMDARG *, enum which));

/*
 * ex_copy -- :[line [,line]] co[py] line [flags]
 *	Copy selected lines.
 */
int
ex_copy(ep, cmdp)
	EXF *ep;
	EXCMDARG *cmdp;
{
	return (cm(ep, cmdp, COPY));
}

/*
 * ex_move -- :[line [,line]] co[py] line
 *	Move selected lines.
 */
int
ex_move(ep, cmdp)
	EXF *ep;
	EXCMDARG *cmdp;
{
	return (cm(ep, cmdp, MOVE));
}

static int
cm(ep, cmdp, cmd)
	EXF *ep;
	EXCMDARG *cmdp;
	enum which cmd;
{
	MARK fm1, fm2, m, tm;
	recno_t lline;

	fm1 = cmdp->addr1;
	fm2 = cmdp->addr2;
	tm.lno = cmdp->lineno;
	tm.cno = 0;

	/* Make sure the destination is valid. */
	if (cmd == MOVE && tm.lno >= fm1.lno && tm.lno < fm2.lno) {
		ep->msg(ep, M_ERROR,
		    "Destination line is inside move range.");
		return (1);
	}

	/* Save the text to a cut buffer. */
	cut(ep, DEFCB, &fm1, &fm2, 1);

	/* If we're not copying, delete the old text & adjust tm. */
	if (cmd == MOVE) {
		delete(ep, &fm1, &fm2, 1);
		if (tm.lno >= fm1.lno)
			tm.lno -= fm2.lno - fm1.lno;
	}

	/* Add the new text. */
	m.lno = SCRLNO(ep);
	m.cno = SCRCNO(ep);
	(void)put(ep, DEFCB, &tm, &m, 1);

	if (SCRLNO(ep) < 1)
		SCRLNO(ep) = 1;
	else {
		lline = file_lline(ep);
		if (SCRLNO(ep) > lline)
			SCRLNO(ep) = lline;
	}

	/* Reporting. */
	if ((ep->rptlines = fm2.lno - fm1.lno) == 0)
		ep->rptlines = 1;
	ep->rptlabel = (cmd == MOVE ? "moved" : "copied");

	FF_SET(ep, F_AUTOPRINT);
	return (0);
}
