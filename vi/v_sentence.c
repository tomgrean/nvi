/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_sentence.c,v 5.2 1992/05/27 10:37:57 bostic Exp $ (Berkeley) $Date: 1992/05/27 10:37:57 $";
#endif /* not lint */

#include <sys/types.h>
#include <stddef.h>

#include "vi.h"
#include "vcmd.h"
#include "extern.h"

static char *ptrn = "[\\.?!][ \t]|[\\.?!]$";

/*
 * v_fsentence -- [count])
 *	Move forward count sentences.
 */
int
v_fsentence(vp, fm, tm, rp)
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	u_long cnt;

	for (cnt = vp->flags & VC_C1SET ? vp->count : 1; cnt--;)
		if ((fm = f_search(fm, ptrn, NULL, 0)) == NULL)
			return (1);
return (0);
/*
	return (v_fword(vp, fm, rp));
*/
}

/*
 * v_bsentence -- [count])
 *	Move forward count sentences.
 */
int
v_bsentence(vp, fm, tm, rp)
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	u_long cnt;

	for (cnt = vp->flags & VC_C1SET ? vp->count : 1; cnt--;)
		if ((fm = b_search(fm, ptrn, NULL, 0)) == NULL)
			return (1);
/*
	return (v_fword(vp, fm, rp));
*/
	return (0);
}
