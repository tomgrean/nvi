/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994, 1995
 *	Keith Bostic.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_search.c,v 10.7 1995/09/21 12:08:38 bostic Exp $ (Berkeley) $Date: 1995/09/21 12:08:38 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "vi.h"

static int v_exaddr __P((SCR *, VICMD *, dir_t));
static int v_search __P((SCR *, VICMD *, char *, u_int, dir_t));

/*
 * v_srch -- [count]?RE[? offset]
 *	Ex address search backward.
 *
 * PUBLIC: int v_searchb __P((SCR *, VICMD *));
 */
int
v_searchb(sp, vp)
	SCR *sp;
	VICMD *vp;
{
	return (v_exaddr(sp, vp, BACKWARD));
}

/*
 * v_searchf -- [count]/RE[/ offset]
 *	Ex address search forward.
 *
 * PUBLIC: int v_searchf __P((SCR *, VICMD *));
 */
int
v_searchf(sp, vp)
	SCR *sp;
	VICMD *vp;
{
	return (v_exaddr(sp, vp, FORWARD));
}

/*
 * v_exaddr --
 *	Do a vi search (which is really an ex address).
 */
static int
v_exaddr(sp, vp, dir)
	SCR *sp;
	VICMD *vp;
	dir_t dir;
{
	static EXCMDLIST fake = { "search" };
	EXCMD *cmdp;
	GS *gp;
	TEXT *tp;
	recno_t s_lno;
	size_t len, s_cno, tlen;
	int err, nb, type;
	char *p, *t, buf[20];

	/*
	 * !!!
	 * If using the search command as a motion, any addressing components
	 * are lost, i.e. y/ptrn/+2, when repeated, is the same as y/ptrn/.
	 */
	if (F_ISSET(vp, VC_ISDOT))
		return (v_search(sp, vp,
		    NULL, SEARCH_PARSE | SEARCH_MSG | SEARCH_SET, dir));

	/* Get the search pattern. */
	if (v_tcmd(sp, vp, dir == BACKWARD ? CH_BSEARCH : CH_FSEARCH,
	    TXT_BS | TXT_CR | TXT_ESCAPE | TXT_PROMPT))
		return (1);

	/*
	 * If the user backspaced over the prompt, do nothing.  If the user
	 * entered <escape> or <carriage-return>, the length is 1 and the
	 * right thing will happen, i.e. the prompt will be used as a command
	 * character.
	 */
	tp = sp->tiq.cqh_first;
	if (tp->term == TERM_BS)
		return (1);

	/* Build a fake ex command structure. */
	gp = sp->gp;
	gp->excmd.cp = tp->lb;
	gp->excmd.clen = tp->len;
	F_SET(&gp->excmd, E_VISEARCH);

	/* Save the current line/column. */
	s_lno = sp->lno;
	s_cno = sp->cno;

	/*
	 * !!!
	 * Historically, vi / and ? commands were full-blown ex addresses,
	 * including ';' delimiters, trailing <blank>'s, multiple search
	 * strings (separated by semi-colons) and, finally, full-blown z
	 * commands after the / and ? search strings.  (If the search was
	 * being used as a motion, the trailing z command was ignored.
	 * Aslo, we do some argument checking on the z command, to be sure
	 * that it's not some other random command.) For multiple search
	 * strings, leading <blank>'s at the second and subsequent strings
	 * were eaten as well.  This has some unintended side-effects: the
	 * command /ptrn/;3 is legal and results in moving to line 3.  It
	 * should have been illegal, but it's too late now.
	 *
	 * !!!
	 * Historically, if any part of the search command failed, the cursor
	 * remained unmodified (even if ; was used).  We have to play games
	 * because the underlying ex parser thinks we're modifying the cursor
	 * as we go, but I think we're compatible with historic practice.
	 *
	 * !!!
	 * Historically, the command "/STRING/;   " failed, apparently it
	 * confused the parser.  We're not that compatible.
	 */
	cmdp = &gp->excmd;
	if (ex_range(sp, cmdp, &err))
		return (1);
	if (err)
		goto err2;

	/* Copy out the new cursor position and make sure it's okay. */
	switch (cmdp->addrcnt) {
	case 1:
		vp->m_stop = cmdp->addr1;
		break;
	case 2:
		vp->m_stop = cmdp->addr2;
		break;
	}
	if (!file_eline(sp, vp->m_stop.lno)) {
		ex_badaddr(sp, &fake,
		    vp->m_stop.lno == 0 ? A_ZERO : A_EOF, NUM_OK);
		goto err2;
	}

	/*
	 * !!!
	 * Historic practice is that a trailing 'z' was ignored if it was a
	 * motion command.  Should probably be an error, but not worth the
	 * effort.
	 */
	if (ISMOTION(vp))
		return (v_correct(sp, vp, F_ISSET(cmdp, E_DELTA)));
		
	/*
	 * !!!
	 * Historically, if it wasn't a motion command, a delta in the search
	 * pattern turns it into a first nonblank movement.
	 */
	nb = F_ISSET(cmdp, E_DELTA);

	/* Check for the 'z' command. */
	if ((len = cmdp->clen) != 0) {
		p = cmdp->cp;
		if (*p != 'z')
			goto err1;

		/* No blanks, just like the z command. */
		for (t = p + 1, tlen = len - 1; tlen > 0; ++t, --tlen)
			if (!isdigit(*t))
				break;
		if (tlen &&
		    (*t == '-' || *t == '.' || *t == '+' || *t == '^')) {
			++t;
			--tlen;
			type = 1;
		} else
			type = 0;
		if (tlen)
			goto err1;

		/* The z command will do the nonblank for us. */
		nb = 0;

		/* Default to z+. */
		if (!type &&
		    v_event_push(sp, NULL, "+", 1, CH_NOMAP | CH_QUOTED))
			return (1);

		/* Push the user's command. */
		if (v_event_push(sp, NULL, p, len, CH_NOMAP | CH_QUOTED))
			return (1);

		/* Push line number so get correct z display. */
		tlen = snprintf(buf,
		    sizeof(buf), "%lu", (u_long)vp->m_stop.lno);
		if (v_event_push(sp, NULL, buf, tlen, CH_NOMAP | CH_QUOTED))
			return (1);
		 
		/* Don't refresh until after 'z' happens. */
		F_SET(VIP(sp), VIP_SKIPREFRESH);
	}

	/* Non-motion commands move to the end of the range. */
	vp->m_final = vp->m_stop;
	if (nb) {
		F_CLR(vp, VM_RCM_MASK);
		F_SET(vp, VM_RCM_SETFNB);
	}
	return (0);

err1:	msgq(sp, M_ERR,
	    "188|Characters after search string, line offset and/or z command");
err2:	vp->m_final.lno = s_lno;
	vp->m_final.cno = s_cno;
	return (1);
}

/*
 * v_searchN -- N
 *	Reverse last search.
 *
 * PUBLIC: int v_searchN __P((SCR *, VICMD *));
 */
int
v_searchN(sp, vp)
	SCR *sp;
	VICMD *vp;
{
	dir_t dir;

	switch (sp->searchdir) {
	case BACKWARD:
		dir = FORWARD;
		break;
	case FORWARD:
		dir = BACKWARD;
		break;
	default:
		dir = sp->searchdir;
		break;
	}
	return (v_search(sp, vp, NULL, SEARCH_PARSE, dir));
}

/*
 * v_searchn -- n
 *	Repeat last search.
 *
 * PUBLIC: int v_searchn __P((SCR *, VICMD *));
 */
int
v_searchn(sp, vp)
	SCR *sp;
	VICMD *vp;
{
	return (v_search(sp, vp, NULL, SEARCH_PARSE, sp->searchdir));
}

/*
 * v_searchw -- [count]^A
 *	Search for the word under the cursor.
 *
 * PUBLIC: int v_searchw __P((SCR *, VICMD *));
 */
int
v_searchw(sp, vp)
	SCR *sp;
	VICMD *vp;
{
	size_t blen, len;
	int rval;
	char *bp;

	len = VIP(sp)->klen + sizeof(RE_WSTART) + sizeof(RE_WSTOP);
	GET_SPACE_RET(sp, bp, blen, len);
	(void)snprintf(bp, blen, "%s%s%s", RE_WSTART, VIP(sp)->keyw, RE_WSTOP);

	rval = v_search(sp, vp, bp, SEARCH_SET, FORWARD);

	FREE_SPACE(sp, bp, blen);
	return (rval);
}

/*
 * v_search --
 *	The search commands.
 */
static int
v_search(sp, vp, ptrn, flags, dir)
	SCR *sp;
	VICMD *vp;
	u_int flags;
	char *ptrn;
	dir_t dir;
{
	switch (dir) {
	case BACKWARD:
		if (b_search(sp, &vp->m_start, &vp->m_stop, ptrn, NULL,
		    flags | SEARCH_MSG | (ISMOTION(vp) ? SEARCH_EOL : 0)))
			return (1);
		break;
	case FORWARD:
		if (f_search(sp, &vp->m_start, &vp->m_stop, ptrn, NULL,
		    flags | SEARCH_MSG | (ISMOTION(vp) ? SEARCH_EOL : 0)))
			return (1);
		break;
	case NOTSET:
		msgq(sp, M_ERR, "189|No previous search pattern");
		return (1);
	default:
		abort();
	}

	/* Correct motion commands, otherwise, simply move to the location. */
	if (ISMOTION(vp)) {
		if (v_correct(sp, vp, 0))
			return(1);
	} else
		vp->m_final = vp->m_stop;
	return (0);
}

/*
 * v_correct --
 *	Handle command with a search as the motion.
 *
 * !!!
 * Historically, commands didn't affect the line searched to/from if the
 * motion command was a search and the final position was the start/end
 * of the line.  There were some special cases and vi was not consistent;
 * it was fairly easy to confuse it.  For example, given the two lines:
 *
 *	abcdefghi
 *	ABCDEFGHI
 *
 * placing the cursor on the 'A' and doing y?$ would so confuse it that 'h'
 * 'k' and put would no longer work correctly.  In any case, we try to do
 * the right thing, but it's not going to exactly match historic practice.
 *
 * PUBLIC: int v_correct __P((SCR *, VICMD *, int));
 */
int
v_correct(sp, vp, isdelta)
	SCR *sp;
	VICMD *vp;
	int isdelta;
{
	dir_t dir;
	MARK m;
	size_t len;

	/*
	 * !!!
	 * We may have wrapped if wrapscan was set, and we may have returned
	 * to the position where the cursor started.  Historic vi didn't cope
	 * with this well.  Yank wouldn't beep, but the first put after the
	 * yank would move the cursor right one column (without adding any
	 * text) and the second would put a copy of the current line.  The
	 * change and delete commands would beep, but would leave the cursor
	 * on the colon command line.  I believe that there are macros that
	 * depend on delete, at least, failing.  For now, commands that use
	 * search as a motion component fail when the search returns to the
	 * original cursor position.
	 */
	if (vp->m_start.lno == vp->m_stop.lno &&
	    vp->m_start.cno == vp->m_stop.cno) {
		msgq(sp, M_BERR, "190|Search wrapped to original position");
		return (1);
	}

	/*
	 * !!!
	 * Searches become line mode operations if there was a delta specified
	 * to the search pattern.
	 */
	if (isdelta)
		F_SET(vp, VM_LMODE);

	/*
	 * If the motion is in the reverse direction, switch the start and
	 * stop MARK's so that it's in a forward direction.  (There's no
	 * reason for this other than to make the tests below easier.  The
	 * code in vi.c:vi() would have done the switch.)  Both forward
	 * and backward motions can happen for any kind of search command
	 * because of the wrapscan option.
	 */
	if (vp->m_start.lno > vp->m_stop.lno ||
	    vp->m_start.lno == vp->m_stop.lno &&
	    vp->m_start.cno > vp->m_stop.cno) {
		m = vp->m_start;
		vp->m_start = vp->m_stop;
		vp->m_stop = m;
		dir = BACKWARD;
	} else
		dir = FORWARD;

	/*
	 * BACKWARD:
	 *	Delete and yank commands move to the end of the range.
	 *	Ignore others.
	 *
	 * FORWARD:
	 *	Delete and yank commands don't move.  Ignore others.
	 */
	vp->m_final = vp->m_start;

	/*
	 * !!!
	 * Delta'd searches don't correct based on column positions.
	 */
	if (isdelta)
		return (0);

	/*
	 * !!!
	 * Backward searches starting at column 0, and forward searches ending
	 * at column 0 are corrected to the last column of the previous line.
	 * Otherwise, adjust the starting/ending point to the character before
	 * the current one (this is safe because we know the search had to move
	 * to succeed).
	 *
	 * Searches become line mode operations if they start at column 0 and
	 * end at column 0 of another line.
	 */
	if (vp->m_start.lno < vp->m_stop.lno && vp->m_stop.cno == 0) {
		if (file_gline(sp, --vp->m_stop.lno, &len) == NULL) {
			FILE_LERR(sp, vp->m_stop.lno);
			return (1);
		}
		if (vp->m_start.cno == 0)
			F_SET(vp, VM_LMODE);
		vp->m_stop.cno = len ? len - 1 : 0;
	} else
		--vp->m_stop.cno;

	return (0);
}
