/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_delete.c,v 5.18 1993/03/26 13:40:22 bostic Exp $ (Berkeley) $Date: 1993/03/26 13:40:22 $";
#endif /* not lint */

#include <sys/param.h>

#include "vi.h"
#include "vcmd.h"

/*
 * v_Delete -- [buffer][count]D
 *	Delete line command.
 */
int
v_Delete(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	size_t len;

	if (file_gline(sp, ep, fm->lno, &len) == NULL) {
		if (file_lline(sp, ep) == 0)
			return (0);
		GETLINE_ERR(sp, fm->lno);
		return (1);
	}

	if (len == 0)
		return (0);

	tm->lno = fm->lno;
	tm->cno = len;

	if (cut(sp, ep, VICB(vp), fm, tm, 0) || delete(sp, ep, fm, tm, 0))
		return (1);

	rp->lno = fm->lno;
	rp->cno = fm->cno ? fm->cno - 1 : 0;
	return (0);
}

/*
 * v_delete -- [buffer][count]d[count]motion
 *	Delete a range of text.
 */
int
v_delete(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	recno_t nlines;
	size_t len;
	int lmode;
	
	lmode = vp->flags & VC_LMODE;
	if (cut(sp, ep, VICB(vp), fm, tm, lmode) ||
	    delete(sp, ep, fm, tm, lmode))
		return (1);

	/*
	 * If deleting lines, leave the cursor at the lowest line deleted,
	 * otherwise, leave it where it started.  Always correct for EOF.
	 */
	nlines = file_lline(sp, ep);
	if (lmode) {
		rp->lno = MIN(fm->lno, tm->lno);
		if (rp->lno > nlines)
			rp->lno = nlines ? nlines : 1;
		rp->cno = fm->cno;
		return (0);
	}

	*rp = *fm;
	if (rp->lno >= nlines)
		if (nlines == 0) {
			rp->lno = 1;
			rp->cno = 0;
		} else {
			rp->lno = nlines;
			if (file_gline(sp, ep, nlines, &len) == NULL) {
				GETLINE_ERR(sp, rp->lno);
				return (1);
			}
			rp->cno = len ? len - 1 : 0;
		}
	return (0);
}
