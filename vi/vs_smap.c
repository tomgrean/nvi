/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: vs_smap.c,v 5.5 1993/02/24 13:04:59 bostic Exp $ (Berkeley) $Date: 1993/02/24 13:04:59 $";
#endif /* not lint */

#include <curses.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "vi.h"
#include "options.h"
#include "vcmd.h"

#define	SMAP_PRIVATE
#include "screen.h"

#define	HMAP		(SCRP(ep)->h_smap)	/* Head of line map. */
#define	TMAP		(SCRP(ep)->t_smap)	/* Tail of line map. */

/*
 * scr_sm_fill --
 *	Fill in the screen map, placing the specified line at the
 *	right position.
 */
int
scr_sm_fill(ep, lno, pos)
	EXF *ep;
	recno_t lno;
	enum position pos;
{
	SMAP *p;
	
	switch (pos) {
	case P_TOP:
		for (p = HMAP, p->lno = lno, p->off = 1; p < TMAP; ++p)
			if (scr_sm_next(ep, p, p + 1))
				goto err;
		break;
	case P_MIDDLE:
		p = HMAP + (TMAP - HMAP) / 2;
		for (p->lno = lno, p->off = 1; p < TMAP; ++p)
			if (scr_sm_next(ep, p, p + 1))
				goto err;
		p = HMAP + (TMAP - HMAP) / 2;
		for (; p > HMAP; --p)
			if (scr_sm_prev(ep, p, p - 1))
				goto err;
		break;
	case P_BOTTOM:
		for (p = TMAP, p->lno = lno, p->off = 1; p > HMAP; --p)
			if (scr_sm_prev(ep, p, p - 1))
				goto err;
		break;
	}
	return (0);

err:	msg(ep, M_BELL, "Movement not possible");
	for (p = HMAP; p < TMAP; ++p)
		(void)scr_sm_next(ep, p, p + 1);
	return (1);
}

/*
 * scr_sm_delete --
 *	Delete a line out of the SMAP.
 */
int
scr_sm_delete(ep, lno)
	EXF *ep;
	recno_t lno;
{
	SMAP *p, *t;
	size_t cnt1, cnt2;

	/* Find the line in the map. */
        for (p = HMAP; p->lno != lno; ++p);

	/*
	 * Count the number of screen lines which display any part
	 * of the deleted line.
	 */
	for (cnt1 = 1, t = p + 1; t <= TMAP && t->lno == lno; ++cnt1, ++t);

	/* Delete that number of lines from the screen. */
	MOVE(ep, p - HMAP, 0);
	for (cnt2 = cnt1; cnt2--;)
		deleteln();

	/* Shift the screen map up. */
	memmove(p, p + cnt1, (((TMAP - p) - cnt1) + 1) * sizeof(SMAP));

	/* Decrement the line numbers for the rest of the map. */
	for (t = TMAP - cnt1; p <= t; ++p)
		--p->lno;

	/* Display the new lines. */
	for (p = TMAP - cnt1;;) {
		if (p < TMAP && scr_sm_next(ep, p, p + 1))
			return (1);
		if (scr_line(ep, ++p, NULL, 0, NULL, NULL))
			return (1);
		if (p == TMAP)
			break;
	}
	return (0);
}

/*
 * scr_sm_insert --
 *	Insert a line into the SMAP.
 */
int
scr_sm_insert(ep, lno)
	EXF *ep;
	recno_t lno;
{
	SMAP *p, *t;
	size_t cnt1, cnt2;

	/* Find the line in the map. */
        for (p = HMAP; p->lno != lno; ++p);

	/*
	 * Figure out how many lines needed to display the line.
	 * The lines left on the screen overrides that number.
	 */
	cnt1 = scr_screens(ep, lno, NULL);
	cnt2 = (TMAP - p) + 1;
	if (cnt1 > cnt2)
		cnt1 = cnt2;

	/* Push down that many lines. */
	MOVE(ep, p - HMAP, 0);
	for (cnt2 = cnt1; cnt2--;)
		insertln();

	/*
	 * Clear the last line on the screen, it's going to have been
	 * corrupted.
	 */
	MOVE(ep, SCREENSIZE(ep), 0);
	clrtoeol();

	/* Shift the screen map down. */
	memmove(p + cnt1, p, (((TMAP - p) - cnt1) + 1) * sizeof(SMAP));

	/* Increment the line numbers for the rest of the map. */
	for (t = p + cnt1; t <= TMAP; ++t)
		++t->lno;

	/* Fill in the SMAP for the new lines. */
	for (cnt2 = 1, t = p; cnt2 <= cnt1; ++t, ++cnt2) {
		t->lno = lno;
		t->off = cnt2;
	}

	/* Display the new lines. */
	for (; cnt1--; ++p)
		if (scr_line(ep, p, NULL, 0, NULL, NULL))
			return (1);
	return (0);
}

/*
 * scr_sm_up --
 *	Scroll the SMAP up count logical lines.
 */
int
scr_sm_up(ep, rp, count, cursor_move)
	EXF *ep;
	MARK *rp;
	recno_t count;
	int cursor_move;
{
	SMAP *p, svmap, tmp;
	recno_t last;
	int scrolled;

	/* Set the default return position. */
	rp->lno = SCRLNO(ep);
	rp->cno = SCRCNO(ep);

	/*
	 * There are two forms of this command, one where the cursor follows
	 * the line, and one where it doesn't.  In the latter, we try and keep
	 * the cursor at the same position on the screen, but, if the screen
	 * is small enough and the line length large enough, the cursor can
	 * end up in very strange places.  Probably not worth fixing.
	 *
	 * Find the line in the SMAP.
	 */
	for (p = HMAP;; ++p) {
		if (p > TMAP) {
			msg(ep, M_ERROR,
			    "Line %lu not on the screen.", SCRLNO(ep));
			return (1);
		}
		if (p->lno == SCRLNO(ep))
			break;
	}

	last = file_lline(ep);
	for (svmap = *p, scrolled = 0;; scrolled = 1) {
		if (count == 0)
			break;
		--count;

		/* Decide what would show up on the screen. */
		if (scr_sm_next(ep, TMAP, &tmp))
			return (1);

		/* If the line doesn't exist, we're done. */
		if (tmp.lno > last)
			break;
			
		/* Scroll up one logical line. */
		if (scr_sm_1up(ep))
			return (1);
		
		if (!cursor_move && p > HMAP)
			--p;
	}

	if (cursor_move) {
		/*
		 * If didn't move enough lines, it's an error if we're at the
		 * EOF, else move there.  Otherwise, try and place the cursor
		 * roughly where it was before.
		 */
		if (!scrolled || count) {
			if (SCRLNO(ep) == TMAP->lno) {
				v_eof(ep, NULL);
				return (1);
			}
			p = TMAP;
		}
	} else {
		/* It's an error if we didn't scroll enough. */
		if (!scrolled || count) {
			v_eof(ep, NULL);
			return (1);
		}

		/* If the cursor moved off the screen, move it to the top. */
		if (SCRLNO(ep) < HMAP->lno)
			p = HMAP;
	}
	/*
	 * On a logical movement, we try and keep the cursor as close as
	 * possible to the last position, but also set it up so that the
	 * next "real" movement will return the cursor to the closest position
	 * to the last real movement.
	 */
	if (p->lno != svmap.lno || p->off != svmap.off) {
		rp->lno = p->lno;
		rp->cno = scr_lrelative(ep, p->lno, p->off);
	}
	return (0);
}

/*
 * scr_sm_1up --
 *	Scroll the SMAP up one.
 */
int
scr_sm_1up(ep)
	EXF *ep;
{
	/* Delete the top line of the screen. */
	MOVE(ep, 0, 0);
	deleteln();

	/* Shift the screen map up. */
	memmove(HMAP, HMAP + 1, SCREENSIZE(ep) * sizeof(SMAP));

	/* Decide what to display at the bottom of the screen. */
	if (scr_sm_next(ep, TMAP - 1, TMAP))
		return (1);

	/* Display it. */
	if (scr_line(ep, TMAP, NULL, 0, NULL, NULL))
		return (1);

	return (0);
}

/*
 * scr_sm_down --
 *	Scroll the SMAP down count logical lines.
 */
int
scr_sm_down(ep, rp, count, cursor_move)
	EXF *ep;
	MARK *rp;
	recno_t count;
	int cursor_move;
{
	SMAP *p, svmap;
	int scrolled;

	/* Set the default return position. */
	rp->lno = SCRLNO(ep);
	rp->cno = SCRCNO(ep);

	/*
	 * There are two forms of this command, one where the cursor follows
	 * the line, and one where it doesn't.  In the latter, we try and keep
	 * the cursor at the same position on the screen, but, if the screen
	 * is small enough and the line length large enough, the cursor can
	 * end up in very strange places.  Probably not worth fixing.
	 *
	 * Find the line in the SMAP.
	 */
	for (p = HMAP;; ++p) {
		if (p > TMAP) {
			msg(ep, M_ERROR,
			    "Line %lu not on the screen.", SCRLNO(ep));
			return (1);
		}
		if (p->lno == SCRLNO(ep))
			break;
	}

	for (svmap = *p, scrolled = 0;; scrolled = 1) {
		if (count == 0)
			break;
		--count;

		/* If the line doesn't exist, we're done. */
		if (HMAP->lno == 1 && HMAP->off == 1)
			break;
			
		/* Scroll down one logical line. */
		if (scr_sm_1down(ep))
			return (1);
		
		if (!cursor_move && p < TMAP)
			++p;
	}

	if (cursor_move) {
		/*
		 * If didn't move enough lines, it's an error if we're at the
		 * SOF, else move there.  Otherwise, try and place the cursor
		 * roughly where it was before.
		 */
		if (!scrolled || count) {
			if (SCRLNO(ep) == HMAP->lno) {
				v_sof(ep, NULL);
				return (1);
			}
			p = HMAP;
		}
	} else {
		/* It's an error if we didn't scroll enough. */
		if (!scrolled || count) {
			v_sof(ep, NULL);
			return (1);
		}

		/* If the cursor moved off the screen, move it to the bottom. */
		if (SCRLNO(ep) > TMAP->lno)
			p = TMAP;
	}

	/*
	 * On a logical movement, we try and keep the cursor as close as
	 * possible to the last position, but also set it up so that the
	 * next "real" movement will return the cursor to the closest position
	 * to the last real movement.
	 */
	if (p->lno != svmap.lno || p->off != svmap.off) {
		rp->lno = p->lno;
		rp->cno = scr_lrelative(ep, p->lno, p->off);
	}
	return (0);
}

/*
 * scr_sm_1down --
 *	Scroll the SMAP down one.
 */
int
scr_sm_1down(ep)
	EXF *ep;
{
	/* Clear the bottom line of the screen. */
	MOVE(ep, TEXTSIZE(ep), 0);
	clrtoeol();

	/* Insert a line at the top of the screen. */
	MOVE(ep, 0, 0);
	insertln();

	/* Shift the screen map down. */
	memmove(HMAP + 1, HMAP, SCREENSIZE(ep) * sizeof(SMAP));

	/* Decide what to display at the top of the screen. */
	if (scr_sm_prev(ep, HMAP + 1, HMAP))
		return (1);

	/* Display it. */
	if (scr_line(ep, HMAP, NULL, 0, NULL, NULL))
		return (1);

	return (0);
}

/*
 * scr_sm_next --
 *	Fill in the next entry in the SMAP.
 */
int
scr_sm_next(ep, p, t)
	EXF *ep;
	SMAP *p, *t;
{
	size_t lcnt;

	if (ISSET(O_LEFTRIGHT)) {
		t->lno = p->lno + 1;
		t->off = p->off;
	} else {
		lcnt = scr_screens(ep, p->lno, NULL);
		if (lcnt == p->off) {
			t->lno = p->lno + 1;
			t->off = 1;
		} else {
			t->lno = p->lno;
			t->off = p->off + 1;
		}
	}
	return (0);
}

/*
 * scr_sm_prev --
 *	Fill in the previous entry in the SMAP.
 */
int
scr_sm_prev(ep, p, t)
	EXF *ep;
	SMAP *p, *t;
{
	if (ISSET(O_LEFTRIGHT)) {
		t->lno = p->lno - 1;
		t->off = p->off;
	} else if (p->off != 1) {
		t->lno = p->lno;
		t->off = p->off - 1;
	} else {
		t->lno = p->lno - 1;
		t->off = scr_screens(ep, t->lno, NULL);
	}
	return (t->lno == 0);
}

/*
 * scr_sm_bot --
 *	Return the line number of the last line on the screen.  (The vi
 *	L command.) Here because only the screen routines know what's
 *	really out there.
 */
int
scr_sm_bot(ep, lnop, cnt)
	EXF *ep;
	recno_t *lnop;
	u_long cnt;
{
	SMAP *p;
	recno_t last;
	
	/* Set p to point at the last legal entry in the map. */
	last = file_lline(ep);
	if (TMAP->lno <= last)
		p = TMAP + 1;
	else
		for (p = HMAP; p->lno <= last; ++p);

	/* Step past cnt start-of-lines, stopping at HMAP. */
	for (; cnt; --cnt)
		for (;;) {
			if (--p < HMAP) {
				msg(ep, M_ERROR, "No such line on the screen.");
				return (1);
			}
			if (p->off == 1)
				break;
		}
	*lnop = p->lno;
	return (0);
}

/*
 * scr_mid --
 *	Return the line number of the middle line on the screen.  (The
 *	vi M command.  Note, the middle line number may not be anywhere
 *	near the middle of the screen, because that's how the historic
 *	vi behaved.)  Here because only the screen routines know what's
 *	out there.
 */
int
scr_sm_mid(ep, lnop)
	EXF *ep;
	recno_t *lnop;
{
	recno_t last, down;

	/* Check for less than a full screen of lines. */
	last = file_lline(ep);
	if (TMAP->lno < last)
		last = TMAP->lno;

	down = (last - HMAP->lno + 1) / 2;
	if (down == 0 && HMAP->off != 1) {
		msg(ep, M_ERROR, "No such line on the screen.");
		return (1);
	}
	*lnop = HMAP->lno + down;
	return (0);
}

/*
 * scr_sm_top --
 *	Return the line number of the first line on the screen.  (The vi
 *	L command.  Note, the top line number may not be at the top of the
 *	screen, because we search for a line that starts on the screen.
 *	It works that way because that's how the historic vi behaved.) Here
 *	because only the screen routines know what's really out there.
 */
int
scr_sm_top(ep, lnop, cnt)
	EXF *ep;
	recno_t *lnop;
	u_long cnt;
{
	SMAP *p, *t;
	recno_t last;

	/*
	 * Set t to point at the map entry one past the last legal
	 * entry in the map.
	 */
	last = file_lline(ep);
	if (TMAP->lno <= last)
		t = TMAP + 1;
	else
		for (t = HMAP; t->lno <= last; ++t);

	/* Step past cnt start-of-lines, stopping at t. */
	for (p = HMAP - 1; cnt; --cnt)
		for (;;) {
			if (++p == t) {
				msg(ep, M_ERROR, "No such line on the screen.");
				return (1);
			}
			if (p->off == 1)
				break;
		}
	*lnop = p->lno;
	return (0);
}

/*
 * scr_sm_nlines --
 *	Return the number of screen lines from an SMAP entry to the
 *	start of some file line, less than a maximum value.
 */
recno_t
scr_sm_nlines(ep, from_sp, to_lno, max)
	EXF *ep;
	SMAP *from_sp;
	recno_t to_lno;
	size_t max;
{
	recno_t lno, lcnt;

	if (ISSET(O_LEFTRIGHT))
		if (from_sp->lno > to_lno)
			return (from_sp->lno - to_lno);
		else
			return (to_lno - from_sp->lno);

	if (from_sp->lno == to_lno)
		return (from_sp->off - 1);

	if (from_sp->lno > to_lno) {
		lcnt = from_sp->off - 1;	/* Correct for off-by-one. */
		for (lno = from_sp->lno; --lno >= to_lno && lcnt <= max;)
			lcnt += scr_screens(ep, lno, NULL);
	} else {
		lno = from_sp->lno;
		lcnt = (scr_screens(ep, lno, NULL) - from_sp->off) + 1;
		for (; ++lno < to_lno && lcnt <= max;)
			lcnt += scr_screens(ep, lno, NULL);
	}
	return (lcnt);
}

#ifdef DEBUG
void
gdmap(ep)
	EXF *ep;
{
	scr_sm_dmap(ep, "gdb");
}

void
scr_sm_dmap(ep, msg)
	EXF *ep;
	char *msg;
{
	size_t cnt;
	SMAP *p;

	TRACE("==>  %s\n", msg);
	for (p = HMAP, cnt = 1; p <= TMAP; ++p, ++cnt)
		TRACE("%s<%02lu:%u> ",
		    cnt % 10 == 0 ? "\n" : "", p->lno, p->off);
	TRACE("\n");
}
#endif
