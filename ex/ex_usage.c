/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_usage.c,v 5.7 1993/03/26 13:39:17 bostic Exp $ (Berkeley) $Date: 1993/03/26 13:39:17 $";
#endif /* not lint */

#include <string.h>

#include "vi.h"
#include "excmd.h"
#include "vcmd.h"

/*
 * ex_usage -- :usage cmd
 *	Display ex usage strings.
 */
int
ex_usage(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	EXCMDLIST *cp;
	size_t len;
	u_char *p;
	
	for (cp = cmds, p = cmdp->argv[0], len = USTRLEN(p);
	    cp->name && memcmp(p, cp->name, len); ++cp);
	if (cp->name == NULL) {
		msgq(sp, M_ERROR, "The %.*s command is unknown.", len, p);
		return (1);
	}
	(void)fprintf(sp->stdfp, "Usage: %s", cp->usage);
	return (0);
}

/*
 * ex_viusage -- :viusage key
 *	Display vi usage strings.
 */
int
ex_viusage(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	VIKEYS *kp;

	u_char key;

	key = *(u_char *)cmdp->argv[0];
	if (key > MAXVIKEY)
		goto nokey;

	kp = &vikeys[key];
	if (kp->func == NULL) {
nokey:		msgq(sp, M_ERROR,
		    "The %s key has no current meaning", charname(sp, key));
		return (1);
	}

	(void)fprintf(sp->stdfp, "Usage: %s", kp->usage);
	return (0);
}
