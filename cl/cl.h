/*-
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994, 1995
 *	Keith Bostic.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	$Id: cl.h,v 10.1 1995/06/08 19:00:31 bostic Exp $ (Berkeley) $Date: 1995/06/08 19:00:31 $
 */

typedef struct _cl_private {
	CHAR_T	 ibuf[256];	/* Input keys. */
	int	 icnt;		/* Input key count. */

	char	*VB;		/* Visual bell termcap string. */

	int	 eof_count;	/* EOF count. */

				/* Busy state. */
	enum { BUSY_OFF=0, BUSY_ON, BUSY_SILENT } busy_state;
	int	 busy_ch;	/* Busy character. */
	struct timeval
		 busy_tv;	/* Busy timer. */
	size_t	 busy_fx;	/* Busy character x coordinate. */
	size_t	 busy_y, busy_x;/* Busy saved screen coordinates. */

	CHAR_T	*lline;		/* Last line retrieved or copied. */
	size_t	 lline_len;	/* Length of last line. */
	size_t	 lline_blen;	/* Length of last line buffer. */

	struct termios exterm;	/* Terminal values ex restores. */
	char	*CE;		/* Clear to EOL termcap string. */
	char	*CM;		/* Cursor movement termcap string. */
	char	*UP;		/* Cursor up termcap string. */
	char	*SE, *SO;	/* Inverse video termcap strings. */

#define	INDX_CONT	0	/* Signal array offsets. */
#define	INDX_HUP	1
#define	INDX_INT	2
#define	INDX_TERM	3
#define	INDX_WINCH	4
#define	INDX_MAX	5	/* Original signal information. */
	struct sigaction oact[INDX_MAX];

#define	CL_INIT_EX	0x0001	/* Ex curses/termcap initialized. */
#define	CL_INIT_VI	0x0002	/* Vi curses/termcap initialized. */
#define	CL_LLINE	0x0004	/* If cursor on the screen's last line. */
#define	CL_LLINE_IV	0x0008	/* If the last line is inverse video. */
#define	CL_SIGCONT	0x0020	/* SIGCONT arrived. */
#define	CL_SIGHUP	0x0040	/* SIGHUP arrived. */
#define	CL_SIGINT	0x0080	/* SIGINT arrived. */
#define	CL_SIGTERM	0x0100	/* SIGTERM arrived. */
#define	CL_SIGWINCH	0x0200	/* SIGWINCH arrived. */
	u_int16_t flags;
} CL_PRIVATE;

extern GS *__global_list;	/* GLOBAL: List of screens. */

#define	CLP(sp)		((CL_PRIVATE *)((sp)->gp->cl_private))
#define	G_CLP		((CL_PRIVATE *)__global_list->cl_private)

typedef enum { INP_OK=0, INP_EOF, INP_ERR, INP_INTR, INP_TIMEOUT } input_t;

/* The screen line relative to a specific window. */
#define	RLNO(sp, lno)	(sp)->woff + (lno)

/*
 * search.c:f_search() is called from ex/ex_tag.c:ex_tagfirst(), which
 * runs before the screen really exists, and tries to access the screen.
 * Make sure we don't step on anything.
 *
 * If the terminal isn't initialized, there's nothing to do.
 */
#define	VI_INIT_IGNORE(sp)						\
	if (CLP(sp) == NULL ||						\
	    F_ISSET(sp, S_VI) && !F_ISSET(CLP(sp), CL_INIT_VI))		\
		return (0);
/* A similar check when running in ex mode. */
#define	EX_INIT_IGNORE(sp)						\
	if (CLP(sp) == NULL ||						\
	    F_ISSET(sp, S_EX) && !F_ISSET(CLP(sp), CL_INIT_EX))		\
		return (0);
/* Some functions are noop's in ex mode. */
#define	EX_NOOP(sp)							\
	if (F_ISSET(sp, S_EX))						\
		return (0);
/* Some functions should never be called in vi mode. */
#define	VI_ABORT(sp)							\
	if (F_ISSET(sp, S_VI))						\
		abort();
/* Some functions should never be called in ex mode. */
#define	EX_ABORT(sp)							\
	if (F_ISSET(sp, S_EX))						\
		abort();

#include "cl_extern.h"
