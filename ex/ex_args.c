/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_args.c,v 8.18 1994/05/21 09:37:59 bostic Exp $ (Berkeley) $Date: 1994/05/21 09:37:59 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <errno.h>
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
 * ex_next -- :next [+cmd] [files]
 *	Edit the next file, optionally setting the list of files.
 *
 * !!!
 * The :next command behaved differently from the :rewind command in
 * historic vi.  See nvi/docs/autowrite for details, but the basic
 * idea was that it ignored the force flag if the autowrite flag was
 * set.  This implementation handles them all identically.
 */
int
ex_next(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	ARGS **argv, **pc;
	FREF *frp;
	char *name;

	MODIFY_RET(sp, ep, F_ISSET(cmdp, E_FORCE));

	/*
	 * If the first argument is a plus sign, '+', it's an initial
	 * ex command.
	 */
	argv = cmdp->argv;
	if (cmdp->argc && argv[0]->bp[0] == '+') {
		--cmdp->argc;
		pc = argv++;
	} else
		pc = NULL;

	/* Any other arguments are a replacement file list. */
	if (cmdp->argc) {
		/* Mark all the current files as ignored. */
		for (frp = sp->frefq.cqh_first;
		    frp != (FREF *)&sp->frefq; frp = frp->q.cqe_next)
			F_SET(frp, FR_IGNORE);

		/* Add the new files into the file list. */
		for (argv = cmdp->argv; argv[0]->len != 0; ++argv)
			if (file_add(sp, NULL, argv[0]->bp, 0) == NULL)
				return (1);

		if ((frp = file_first(sp)) == NULL)
			return (1);
	} else if ((frp = file_next(sp, sp->a_frp)) == NULL) {
		msgq(sp, M_ERR, "No more files to edit");
		return (1);
	}

	/*
	 * There's a tricky sequence, where the user edits two files, e.g.
	 * "x" and "y".  While in "x", they do ":e y|:f foo", which changes
	 * the name of that FRP entry.  Then, the :n command finds the file
	 * "y" with a name change.  If the file name has been changed, get
	 * a new FREF for the original file name, and make it be the one that
	 * is displayed in the argument list, not the one with the name change.
	 */
	if (frp->cname != NULL) {
		F_SET(frp, FR_IGNORE);
		name = frp->name == NULL ? frp->tname : frp->name;
		if ((frp = file_add(sp, sp->a_frp, name, 0)) == NULL)
			return (1);
	}
	if (file_init(sp, frp, NULL, F_ISSET(cmdp, E_FORCE)))
		return (1);

	/* Push the initial command onto the stack. */
	if (pc != NULL)
		if (IN_EX_MODE(sp))
			(void)term_push(sp, pc[0]->bp, pc[0]->len, 0);
		else if (IN_VI_MODE(sp)) {
			(void)term_push(sp, "\n", 1, 0);
			(void)term_push(sp, pc[0]->bp, pc[0]->len, 0);
			(void)term_push(sp, ":", 1, 0);
			(void)file_lline(sp, sp->ep, &sp->frp->lno);
			F_SET(sp->frp, FR_CURSORSET);
		}

	sp->a_frp = frp;
	F_SET(sp, S_FSWITCH);
	return (0);
}

/*
 * ex_prev -- :prev
 *	Edit the previous file.
 */
int
ex_prev(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	FREF *frp;
	char *name;

	MODIFY_RET(sp, ep, F_ISSET(cmdp, E_FORCE));

	if ((frp = file_prev(sp, sp->a_frp)) == NULL) {
		msgq(sp, M_ERR, "No previous files to edit");
		return (1);
	}

	/* See comment in ex_next(). */
	if (frp->cname != NULL) {
		F_SET(frp, FR_IGNORE);
		name = frp->name == NULL ? frp->tname : frp->name;
		if ((frp = file_add(sp, frp, name, 0)) == NULL)
			return (1);
	}
	if (file_init(sp, frp, NULL, F_ISSET(cmdp, E_FORCE)))
		return (1);
	sp->a_frp = frp;
	F_SET(sp, S_FSWITCH);
	return (0);
}

/*
 * ex_rew -- :rew
 *	Re-edit the list of files.
 */
int
ex_rew(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	FREF *frp, *tfrp;

	/*
	 * !!!
	 * Historic practice -- you can rewind to the current file.
	 */
	if ((frp = file_first(sp)) == NULL) {
		msgq(sp, M_ERR, "No previous files to rewind");
		return (1);
	}

	MODIFY_RET(sp, ep, F_ISSET(cmdp, E_FORCE));

	/*
	 * !!!
	 * Historic practice, turn off the edited bit.  The :next and :prev
	 * code will discard any name changes, so ignore them here.  Start
	 * at the beginning of the file, too.
	 */
	for (tfrp = sp->frefq.cqh_first;
	    tfrp != (FREF *)&sp->frefq; tfrp = tfrp->q.cqe_next)
		F_CLR(tfrp, FR_CHANGEWRITE | FR_CURSORSET | FR_EDITED);

	if (file_init(sp, frp, NULL, F_ISSET(cmdp, E_FORCE)))
		return (1);
	sp->a_frp = frp;
	F_SET(sp, S_FSWITCH);
	return (0);
}

/*
 * ex_args -- :args
 *	Display the list of files.
 */
int
ex_args(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	FREF *frp;
	int cnt, col, iscur, len, nlen, sep;
	char *name;

	/*
	 * !!!
	 * Ignore files that aren't in the "argument" list unless they are the
	 * one we're currently editing.  I'm not sure this is right, but the
	 * historic vi behavior of not showing the current file if it was the
	 * result of a ":e" command, or if the file name was changed was wrong.
	 * This is actually pretty tricky, don't modify it without thinking it
	 * through.  There have been a lot of problems in here.
	 *
	 * Also, historic practice was to display the original name of the file
	 * even if the user had used a file command to change the file name.
	 * Confusing, at best.  We show both names: the original as that's what
	 * the user will get in a next, prev or rewind, and the new one since
	 * that's what the user is actually editing now.
	 *
	 * When we find the "argument" FREF, i.e. the current location in the
	 * user's argument list, if it's not the same as the current FREF, we
	 * display the current FREF as following the argument in the list.
	 * This means that if the user edits three files, "x", "y" and "z", and
	 * then does a :e command in the file "x" to edit "z", "z" will appear
	 * in the list twice.
	 */
	col = len = sep = 0;
	for (cnt = 1, frp = sp->frefq.cqh_first;
	    frp != (FREF *)&sp->frefq; frp = frp->q.cqe_next) {
		iscur = 0;
		/*
		 * If the last argument FREF structure, and we're editing
		 * it, set the current bit.  Otherwise, we'll display it,
		 * then the file we're editing, and the latter will have
		 * the current bit set.
		 */
		if (frp == sp->a_frp) {
			if (frp == sp->frp && frp->cname == NULL)
				iscur = 1;
		} else if (F_ISSET(frp, FR_IGNORE))
			continue;
		name = frp->name == NULL ? frp->tname : frp->name;
		/*
		 * Mistake.  The user edited a temporary file (vi /tmp), then
		 * switched to another file (:e file).  The argument FREF is
		 * pointing to the temporary file, but it doesn't have a name.
		 * Gracefully recover through the creative use of goto's.
		 */
		if (name == NULL)
			goto testcur;
extra:		nlen = strlen(name);
		col += len = nlen + sep + (iscur ? 2 : 0);
		if (col >= sp->cols - 1) {
			col = len;
			sep = 0;
			(void)ex_printf(EXCOOKIE, "\n");
		} else if (cnt != 1) {
			sep = 1;
			(void)ex_printf(EXCOOKIE, " ");
		}
		++cnt;

		if (iscur)
			(void)ex_printf(EXCOOKIE, "[%s]", name);
		else {
			(void)ex_printf(EXCOOKIE, "%s", name);
testcur:		if (frp == sp->a_frp) {
				if (frp != sp->frp)
					name = FILENAME(sp->frp);
				else
					name = frp->cname;
				iscur = 1;
				goto extra;
			}
		}
	}
	/* This should never happen; left in because it's been known to. */
	if (cnt == 1)
		(void)ex_printf(EXCOOKIE, "No files.\n");
	else
		(void)ex_printf(EXCOOKIE, "\n");
	return (0);
}
