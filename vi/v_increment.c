/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_increment.c,v 5.20 1993/03/26 13:40:28 bostic Exp $ (Berkeley) $Date: 1993/03/26 13:40:28 $";
#endif /* not lint */

#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "vi.h"
#include "vcmd.h"

static char * const fmt[] = {
#define	DEC	0
	"%.*ld",
#define	SDEC	1
	"%+.*ld",
#define	HEXC	2
	"%#0.*lX",
#define	HEXL	3
	"%#0.*lx",
#define	OCTAL	4
	"%#0.*lo",
};

/*
 * v_increment -- [count]#[#+-]
 *	Increment/decrement a keyword number.
 */
int
v_increment(sp, ep, vp, fm, tm, rp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	u_long ulval;
	long lval;
	size_t len, nlen;
	int rval;
	char *ntype, nbuf[60];
	u_char *p, *np;

	/* Do repeat operations. */
	if (vp->character == '#')
		vp->character = sp->inc_lastch;

	/* Get new value. */
	if (vp->flags & VC_C1SET)
		sp->inc_lastval = vp->count;

	if (vp->character != '+' && vp->character != '-') {
		msgq(sp, M_ERROR, "usage: %s.", vp->kp->usage);
		return (1);
	}
	sp->inc_lastch = vp->character;

	/* Figure out the resulting type and number. */
	p = (u_char *)vp->keyword;
	len = vp->klen;
	if (len > 1 && p[0] == '0') {
		if (vp->character == '+') {
			ulval = strtoul(vp->keyword, NULL, 0);
			if (ULONG_MAX - ulval < sp->inc_lastval)
				goto overflow;
			ulval += sp->inc_lastval;
		} else {
			ulval = strtoul(vp->keyword, NULL, 0);
			if (ulval < sp->inc_lastval)
				goto underflow;
			ulval -= sp->inc_lastval;
		}
		ntype = fmt[OCTAL];
		if (len > 2)
			if (p[1] == 'X')
				ntype = fmt[HEXC];
			else if (p[1] == 'x')
				ntype = fmt[HEXL];
		nlen = snprintf(nbuf, sizeof(nbuf), ntype, len, ulval);
	} else {
		if (vp->character == '+') {
			lval = strtol(vp->keyword, NULL, 0);
			if (LONG_MAX - lval < sp->inc_lastval) {
overflow:			msgq(sp, M_ERROR,
				    "Resulting number too large.");
				return (1);
			}
			lval += sp->inc_lastval;
		} else {
			lval = strtol(vp->keyword, NULL, 0);
			if (lval < 0 && -(LONG_MIN - lval) > sp->inc_lastval) {
underflow:			msgq(sp, M_ERROR,
				    "Resulting number too small.");
				return (1);
			}
			lval -= sp->inc_lastval;
		}
		ntype = *vp->keyword == '+' || *vp->keyword == '-' ?
		    fmt[SDEC] : fmt[DEC];
		nlen = snprintf(nbuf, sizeof(nbuf), ntype, len, lval);
	}

	if ((p = file_gline(sp, ep, fm->lno, &len)) == NULL) {
		GETLINE_ERR(sp, fm->lno);
		return (1);
	}

	if ((np = malloc(len + nlen)) == NULL) {
		msgq(sp, M_ERROR, "Error: %s", strerror(errno));
		return (1);
	}
	memmove(np, p, fm->cno);
	memmove(np + fm->cno, nbuf, nlen);
	memmove(np + fm->cno + nlen,
	    p + fm->cno + vp->klen, len - fm->cno - vp->klen);
	p = np;
	len = len - vp->klen + nlen;

	if (file_sline(sp, ep, fm->lno, p, len))
		rval = 1;
	else {
		*rp = *fm;
		rval = 0;
	}
	if (np)
		free(np);
	return (rval);
}
