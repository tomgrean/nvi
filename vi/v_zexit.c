/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_zexit.c,v 5.15 1993/03/26 13:40:25 bostic Exp $ (Berkeley) $Date: 1993/03/26 13:40:25 $";
#endif /* not lint */

#include <sys/types.h>

#include <curses.h>
#include <string.h>

#include "vi.h"
#include "excmd.h"
#include "vcmd.h"

/*
 * v_exit -- ZZ
 *	Save the file and exit.
 */
int
v_exit(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	EXCMDARG cmd;

	SETCMDARG(cmd, C_WQ, 0, OOBLNO, OOBLNO, 0, NULL);
	return (ex_wq(sp, ep, &cmd));
}
