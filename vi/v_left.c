/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_left.c,v 5.4 1992/05/27 10:36:21 bostic Exp $ (Berkeley) $Date: 1992/05/27 10:36:21 $";
#endif /* not lint */

#include <sys/types.h>
#include <stdio.h>

#include "vi.h"
#include "options.h"
#include "vcmd.h"
#include "extern.h"

/*
 * v_left -- [count]^H, [count]h
 *	Move left by columns.
 */
int
v_left(vp, fm, tm, rp)
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	u_long cnt;

	cnt = vp->flags & VC_C1SET ? vp->count : 1;

	if (fm->cno == 0) {
		bell();
		if (ISSET(O_VERBOSE))
			msg("Already in the first column.");
		return (1);
	}

	rp->lno = fm->lno;
	if (fm->cno > cnt)
		rp->cno = fm->cno - cnt;
	else
		rp->cno = 0;
	return (0);
}

/*
 * v_first -- ^, _
 *	Move to the first non-blank column on this line.
 */
v_first(vp, fm, tm, rp)
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	rp->lno = fm->lno;
	return (v_nonblank(rp));
}

/*
 * v_ncol -- [count]|
 *	Move to column count, or the first non-blank column on this line.
 */
v_ncol(vp, fm, tm, rp)
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	if (vp->flags & VC_C1SET) {
		rp->lno = fm->lno;
		fm->cno = vp->count;
		return (0);
	}
	rp->lno = fm->lno;
	return (v_nonblank(rp));
}

/*
 * v_zero -- 0
 *	Move to the first column on this line.
 */
v_zero(vp, fm, tm, rp)
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	rp->lno = fm->lno;
	rp->cno = 0;
	return (0);
}
