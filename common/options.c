/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: options.c,v 5.3 1992/02/20 14:14:46 bostic Exp $ (Berkeley) $Date: 1992/02/20 14:14:46 $";
#endif /* not lint */

#include <sys/param.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <paths.h>

#include "config.h"
#include "options.h"
#include "vi.h"
#include "curses.h"

static int opts_abbcmp __P((const void *, const void *));
static int opts_cmp __P((const void *, const void *));
static int opts_print __P((struct _option *));

/*
 * First array slot is the current value, second and third are the low and
 * and high ends of the range.
 */
static long columns[3] = {80, 32, 255};
static long keytime[3] = {2, 0, 50};
static long lines[3] =	{25, 2, 66};
static long report[3] = {5, 1, 127};
static long scroll[3] = {12, 1, 127};
static long shiftwidth[3] = {8, 1, 255};
static long sidescroll[3] = {8, 1, 40};
static long tabstop[3] = {8, 1, 40};
static long taglength[3] = {0, 0, 30};
static long window[3] = {24, 1, 24};
static long wrapmargin[3] = {0, 0, 255};

/* START_OPTION_DEF */
OPTIONS opts[] = {
#define	O_AUTOINDENT	0
	"autoindent",	NULL,		OPT_0BOOL,
#define	O_AUTOPRINT	1
	"autoprint",	NULL,		OPT_1BOOL,
#define	O_AUTOTAB	2
	"autotab",	NULL,		OPT_1BOOL,
#define	O_AUTOWRITE	3
	"autowrite",	NULL,		OPT_0BOOL,
#define	O_BEAUTIFY	4
	"beautify",	NULL,		OPT_0BOOL,
#define	O_CC		5
	"cc",		"cc -c",	OPT_STR,
#define	O_CHARATTR	6
	"charattr",	NULL,		OPT_0BOOL|OPT_REDRAW,
#define	O_COLUMNS	7
	"columns",	&columns,	OPT_NOSAVE|OPT_NUM|OPT_REDRAW,
#define	O_DIGRAPH	8
	"digraph",	NULL,		OPT_0BOOL,
#define	O_DIRECTORY	9
	"directory",	_PATH_TMP,	OPT_NOSAVE|OPT_STR,
#define	O_EDCOMPATIBLE	10
	"edcompatible",	NULL,		OPT_0BOOL,
#define	O_EQUALPRG	11
	"equalprg",	"fmt",		OPT_STR,
#define	O_ERRORBELLS	12
	"errorbells",	NULL,		OPT_1BOOL,
#define	O_EXRC		13
	"exrc",		NULL,		OPT_0BOOL,
#define	O_EXREFRESH	14
	"exrefresh",	NULL,		OPT_1BOOL,
#define	O_FLASH		15
	"flash",	NULL,		OPT_1BOOL,
#define	O_FLIPCASE	16
	"flipcase",	NULL,		OPT_STR,
#define	O_HIDEFORMAT	17
	"hideformat",	NULL,		OPT_0BOOL|OPT_REDRAW,
#define	O_IGNORECASE	18
	"ignorecase",	NULL,		OPT_0BOOL,
#define	O_INPUTMODE	19
	"inputmode",	NULL,		OPT_0BOOL,
#define	O_KEYTIME	20
	"keytime",	&keytime,	OPT_NUM,
#define	O_KEYWORDPRG	21
	"keywordprg",	"ref",		OPT_STR,
#define	O_LINES		22
	"lines",	&lines,		OPT_NOSAVE|OPT_NUM|OPT_REDRAW,
#define	O_LIST		23
	"list",		NULL,		OPT_0BOOL|OPT_REDRAW,
#define	O_MAGIC		24
	"magic",	NULL,		OPT_1BOOL,
#define	O_MAKE		25
	"make",		"make",		OPT_STR,
#define	O_MESG		26
	"mesg",		NULL,		OPT_1BOOL,
#define	O_MODELINE	27
	"modeline",	NULL,		OPT_0BOOL,
#define	O_MORE		28
	"more",		NULL,		OPT_1BOOL,
#define	O_NUMBER	29
	"number",	NULL,		OPT_0BOOL|OPT_REDRAW,
#define	O_PARAGRAPHS	30
	"paragraphs",	"PPppIPLPQP",	OPT_STR,
#define	O_PROMPT	31
	"prompt",	NULL,		OPT_1BOOL,
#define	O_READONLY	32
	"readonly",	NULL,		OPT_0BOOL,
#define	O_REPORT	33
	"report",	&report,	OPT_NUM,
#define	O_RULER		34
	"ruler",	NULL,		OPT_0BOOL,
#define	O_SCROLL	35
	"scroll",	&scroll,	OPT_NUM,
#define	O_SECTIONS	36
	"sections",	"NHSHSSSEse",	OPT_STR,
#define	O_SHELL		37
	"shell",	_PATH_BSHELL,	OPT_STR,
#define	O_SHIFTWIDTH	38
	"shiftwidth",	&shiftwidth,	OPT_NUM,
#define	O_SHOWMATCH	39
	"showmatch",	NULL,		OPT_0BOOL,
#define	O_SHOWMODE	40
	"showmode",	NULL,		OPT_0BOOL,
#define	O_SIDESCROLL	41
	"sidescroll",	&sidescroll,	OPT_NUM,
#define	O_SYNC		42
	"sync",		NULL,		OPT_0BOOL,
#define	O_TABSTOP	43
	"tabstop",	&tabstop,	OPT_NUM|OPT_REDRAW,
#define	O_TAGLENGTH	44
	"taglength",	&taglength,	OPT_NUM,
#define	O_TERM		45
	"term",		"unknown",	OPT_NOSAVE|OPT_STR,
#define	O_TERSE		46
	"terse",	NULL,		OPT_0BOOL,
#define	O_TIMEOUT	47
	"timeout",	NULL,		OPT_0BOOL,
#define	O_VBELL		48
	"vbell",	NULL,		OPT_0BOOL,
#define	O_WARN		49
	"warn",		NULL,		OPT_1BOOL,
#define	O_WINDOW	50
	"window",	&window,	OPT_NUM|OPT_REDRAW,
#define	O_WRAPMARGIN	51
	"wrapmargin",	&wrapmargin,	OPT_NUM,
#define	O_WRAPSCAN	52
	"wrapscan",	NULL,		OPT_1BOOL,
#define	O_WRITEANY	53
	"writeany",	NULL,		OPT_0BOOL,
	NULL,
};
#define	O_OPTIONCOUNT	54
/* END_OPTION_DEF */

typedef struct abbrev {
        char *name;
        int offset;
} ABBREV;

static ABBREV abbrev[] = {
	"ai",	O_AUTOINDENT,
	"ap",	O_AUTOPRINT,
	"at",	O_AUTOTAB,
	"aw",	O_AUTOWRITE,
	"bf",	O_BEAUTIFY,
	"ca",	O_CHARATTR,
	"cc",	O_CC,
	"co",	O_COLUMNS,
	"dig",	O_DIGRAPH,
	"dir",	O_DIRECTORY,
	"eb",	O_ERRORBELLS,
	"ed",	O_EDCOMPATIBLE,
	"ep",	O_EQUALPRG,
	"er",	O_EXREFRESH,
	"fc",	O_FLIPCASE,
	"fl",	O_VBELL,
	"hf",	O_HIDEFORMAT,
	"ic",	O_IGNORECASE,
	"im",	O_INPUTMODE,
	"kp",	O_KEYWORDPRG,
	"kt",	O_KEYTIME,
	"li",	O_LIST,
	"ls",	O_LINES,
	"ma",	O_MAGIC,
	"me",	O_MESG,
	"mk",	O_MAKE,
	"ml",	O_MODELINE,
	"ml",	O_MODELINE,
	"mo",	O_MORE,
	"nu",	O_NUMBER,
	"pa",	O_PARAGRAPHS,
	"pr",	O_PROMPT,
	"re",	O_REPORT,
	"ro",	O_READONLY,
	"ru",	O_RULER,
	"sc",	O_SCROLL,
	"se",	O_SECTIONS,
	"sh",	O_SHELL,
	"sm",	O_SHOWMATCH,
	"ss",	O_SIDESCROLL,
	"sw",	O_SHIFTWIDTH,
	"sy",	O_SYNC,
	"te",	O_TERM,
	"tl",	O_TAGLENGTH,
	"to",	O_KEYTIME,
	"tr",	O_TERSE,
	"ts",	O_TABSTOP,
	"vb",	O_VBELL,
	"wa",	O_WARN,
	"wi",	O_WINDOW,
	"wm",	O_WRAPMARGIN,
	"wr",	O_WRITEANY,
	"ws",	O_WRAPSCAN,
	NULL,
};

/*
 * opts_init --
 *	Initialize some of the options.  Since the user isn't really "setting"
 *	these variables, we don't set their OPT_SET bits.
 */
void
opts_init()
{
	char *val;

	if (val = getenv("COLUMNS"))
		COLS = atoi(val);
	LVAL(O_COLUMNS) = COLS;

	if (val = getenv("LINES"))
		LINES = atoi(val);
	LVAL(O_LINES) = LINES;
	LVAL(O_SCROLL) = LINES / 2 - 1;
	LVAL(O_WINDOW) = LINES - 1;

	if (val = getenv("SHELL")) {
		PVAL(O_SHELL) = strdup(val);
		FSET(O_SHELL, OPT_ALLOCATED);
	}

	/* Disable the vbell option if we don't know how to do a vbell. */
	if (!has_VB) {
		FSET(O_FLASH, OPT_NOSET);
		FSET(O_VBELL, OPT_NOSET);
	}

/* XXX -- THIS CODE IS PROBABLY WRONG */
#ifndef NO_DIGRAPH
# ifdef CS_LATIN1
	for (i = 0, val = PVAL(O_FLIPCASE); i < 32; i++)
	{
		/* leave out the multiply/divide symbols */
		if (i == 23)
			continue;

		/* add lower/uppercase pair */
		*val++ = i + 0xe0;
		*val++ = i + 0xc0;
	}
	*val = '\0';
# endif /* CS_LATIN1 */

	/* initialize the ctype package */
	_ct_init(PVAL(O_FLIPCASE));
#else
	_ct_init("");
#endif /* not NO_DIGRAPH */
}

/*
 * opts_set --
 *	Change the values of one or more options.
 */
void
opts_set(arg)
	char *arg;
{
	ABBREV atmp, *ap;
	OPTIONS otmp, *op;
	long value;
	int ch, needredraw, off;
	char *ep, *equals, *name;
	
	/*
	 * Reset the upper limit of "window" option to lines - 1.
	 * XXX -- Why are we doing this?
	 */
	LVAL(O_WINDOW) = LINES - 1;

	for (needredraw = 0;;) {
		/* Skip leading white space. */
		while ((ch = *arg) && isspace(ch))
			++arg;
		if (*arg == '\0')
			break;
		name = arg;

		/* Find equals sign or end of set, skipping backquoted chars. */
		for (equals = NULL; (ch = *arg) && !isspace(ch); ++arg)
			switch(ch) {
			case '=':
				equals = arg;
				break;
			case '\\':
				/* Historic vi just used the backslash. */
				if (arg[1] == '\0')
					break;
				++arg;
				break;
			}

		off = 0;
		op = NULL;
		if (equals)
			*equals++ = '\0';

		/* Check list of abbreviations. */
		atmp.name = name;
		if ((ap = bsearch(&atmp, abbrev,
		    sizeof(abbrev) / sizeof(ABBREV) - 1,
		    sizeof(ABBREV), opts_abbcmp)) != NULL) {
			op = opts + ap->offset;
			goto found;
		}

		/* Check list of options. */
		otmp.name = name;
		if ((op = bsearch(&otmp, opts,
		    sizeof(opts) / sizeof(OPTIONS) - 1,
		    sizeof(OPTIONS), opts_cmp)) != NULL)
			goto found;

		/* Try the name without any leading "no". */
		if (name[0] == 'n' && name[1] == 'o') {
			off = 1;
			name += 2;
		} else
			goto found;

		/* Check list of abbreviations. */
		atmp.name = name;
		if ((ap = bsearch(&atmp, abbrev,
		    sizeof(abbrev) / sizeof(ABBREV) - 1,
		    sizeof(ABBREV), opts_abbcmp)) != NULL) {
			op = opts + ap->offset;
			goto found;
		}

		/* Check list of options. */
		otmp.name = name;
		op = bsearch(&otmp, opts,
		    sizeof(opts) / sizeof(OPTIONS) - 1,
		    sizeof(OPTIONS), opts_cmp);

found:		if (op == NULL || off && !ISFSETP(op, OPT_0BOOL|OPT_1BOOL)) {
			msg("no option %s: 'set all' gives all option values",
			    name);
			continue;
		}

		/* Set name, value. */
		if (ISFSETP(op, OPT_NOSET)) {
			msg("%s: may not be set", name);
			continue;
		}

		switch (op->flags & OPT_TYPE) {
		case OPT_0BOOL:
		case OPT_1BOOL:
			if (equals)
				msg("set: option [no]%s is a boolean", name);
			else {
				FUNSETP(op, OPT_0BOOL | OPT_1BOOL);
				FSETP(op,
				    (off ? OPT_0BOOL : OPT_1BOOL) | OPT_SET);
				needredraw |= ISFSETP(op, OPT_REDRAW);
			}
			break;
		case OPT_NUM:
			if (!equals) {
				msg("set: option [no]%s requires a value",
				    name);
				break;
			}
			value = strtol(equals, &ep, 10);
			if (*ep && !isspace(*ep)) {
				msg("set %s: illegal number %s", name, equals);
				break;
			}
			if (value < MINLVALP(op)) {
				msg("set %s: min value is %ld",
				    name, MINLVALP(op));
				break;
			}
			if (value > MAXLVALP(op)) {
				msg("set %s: max value is %ld",
				    name, MAXLVALP(op));
				break;
			}
			LVALP(op) = value;
			FSETP(op, OPT_SET);
			needredraw |= ISFSETP(op, OPT_REDRAW);
			break;
		case OPT_STR:
			if (!equals) {
				msg("set: option [no]%s requires a value",
				    name);
				break;
			}
			if (ISFSETP(op, OPT_ALLOCATED))
				free(PVALP(op));
			PVALP(op) = strdup(equals);
			FSETP(op, OPT_ALLOCATED | OPT_SET);
			needredraw |= ISFSETP(op, OPT_REDRAW);
			break;
		}
	}

	/* Special processing. */

	/* If "readonly" then set the READONLY flag for this file */
	if (ISSET(O_READONLY))
		setflag(file, READONLY);

/* XXX -- REMOVE THIS AT SOME POINT, I THINK ITS WRONG */
#ifndef NO_DIGRAPH
	/* re-initialize the ctype package */
	_ct_init(PVAL(O_FLIPCASE));
#endif
	/*
	 * XXX -- why, just use O_LINES/O_COLUMNS
	 * Copy O_LINES and O_COLUMNS into LINES and COLS.
	 */
	LINES = LVAL(O_LINES);
	COLS = LVAL(O_COLUMNS);

	if (needredraw)
		redraw(MARK_UNSET, FALSE);
}

/*
 * opt_dump --
 *	List the current values of selected options.
 */
void
opts_dump(all)
	int all;
{
	OPTIONS *op;
	int base, b_num, chcnt, cnt, col, colwidth, curlen, endcol, s_num;
	int numcols, numrows, row, termwidth;
	int b_op[O_OPTIONCOUNT], s_op[O_OPTIONCOUNT];
	char nbuf[20];

	/*
	 * Options are output in two groups -- those that fit at least two to
	 * a line and those that don't.  We do output on tab boundaries for no
	 * particular reason.   First get the set of options to list, keeping
	 * track of the length of each.  No error checking, because we know
	 * that O_TERM was set so at least one option has the OPT_SET bit on.
	 * Termwidth is the tab stop before half of the line in the first loop,
	 * and the full line length later on.
	 */
#define	TAB	8		/* XXX */
	colwidth = -1;
	termwidth = (COLS - 1) / 2 & ~(TAB - 1);
	for (b_num = s_num = 0, op = opts; op->name; ++op) {
		if (!all && !ISFSETP(op, OPT_SET))
			continue;
		cnt = op - opts;
		curlen = strlen(op->name);
		switch (op->flags & OPT_TYPE) {
		case OPT_0BOOL:
			curlen += 2;
			break;
		case OPT_1BOOL:
			break;
		case OPT_NUM:
			curlen +=
			    snprintf(nbuf, sizeof(nbuf), "%ld", LVAL(cnt));
			break;
		case OPT_STR:
			curlen += strlen(PVAL(cnt)) + 3;
			break;
		}
		if (curlen < termwidth) {
			if (colwidth < curlen)
				colwidth = curlen;
			s_op[s_num++] = cnt;
		} else
			b_op[b_num++] = cnt;
	}

	colwidth = (colwidth + TAB) & ~(TAB - 1);
	termwidth = COLS - 1;
	numcols = termwidth / colwidth;
	if (s_num > numcols) {
		numrows = s_num / numcols;
		if (s_num % numcols)
			++numrows;
	} else
		numrows = 1;

	for (row = 0; row < numrows; ++row) {
		endcol = colwidth;
		for (base = row, chcnt = col = 0; col < numcols; ++col) {
			chcnt += opts_print(&opts[s_op[base]]);
			if ((base += numrows) >= s_num)
				break;
			while ((cnt = (chcnt + TAB & ~(TAB - 1))) <= endcol) {
				qaddch('\t');
				chcnt = cnt;
			}
			endcol += colwidth;
		}
		if (numrows > 1 || b_num)
			addch('\n');
	}

	for (row = 0; row < b_num; ++row) {
		(void)opts_print(&opts[b_op[row]]);
		addch('\n');
	}
	exrefresh();
}

/*
 * opts_save --
 *	Write the current configuration to a file.
 */
void
opts_save(fd)
	int fd;
{
	OPTIONS *op;
	int len;
	char *p, buf[1024];

	(void)strcpy(buf, "set ");
	for (op = opts; op->name; ++op) {
		if (ISFSETP(op, OPT_NOSAVE))
			continue;
		p = buf + (len = 4);
		switch (op->flags & OPT_TYPE) {
		case OPT_0BOOL:
			*p++ = 'n';
			*p++ = 'o';
			len += 2;
			/* FALLTHROUGH */
		case OPT_1BOOL:
			len += snprintf(p, sizeof(buf) - 10, "%s\n", op->name);
			break;
		case OPT_NUM:
			len += snprintf(p,
			    sizeof(buf) - 10, "%s=%-3d\n", op->name, LVALP(op));
			break;
		case OPT_STR:
			len += snprintf(p, sizeof(buf) - 4,
			    "%s=\"%s\"\n", op->name, PVALP(op));
			break;
		}
		if (write(fd, buf, len) != len) {
			msg("mkexrc: incomplete: %s", strerror(errno));
			return;
		}
	}
}

/*
 * opt_print --
 *	Print out an option.
 */
static int
opts_print(op)
	OPTIONS *op;
{
	int curlen;
	char nbuf[20];

	curlen = 0;
	switch (op->flags & OPT_TYPE) {
	case OPT_0BOOL:
		curlen += 2;
		qaddch('n');
		qaddch('o');
		/* FALLTHROUGH */
	case OPT_1BOOL:
		curlen += strlen(op->name);
		qaddstr(op->name);
		break;
	case OPT_NUM:
		curlen += strlen(op->name);
		qaddstr(op->name);
		curlen += 1;
		qaddch('=');
		curlen += snprintf(nbuf, sizeof(nbuf), "%ld", LVALP(op));
		qaddstr(nbuf);
		break;
	case OPT_STR:
		curlen += strlen(op->name);
		qaddstr(op->name);
		curlen += 1;
		qaddch('=');
		curlen += 1;
		qaddch('"');
		curlen += strlen(PVALP(op));
		qaddstr(PVALP(op));
		curlen += 1;
		qaddch('"');
		break;
	}
	return (curlen);
}

opts_abbcmp(a, b)
        const void *a, *b;
{
        return(strcmp(((ABBREV *)a)->name, ((ABBREV *)b)->name));
}

opts_cmp(a, b)
        const void *a, *b;
{
        return(strcmp(((OPTIONS *)a)->name, ((OPTIONS *)b)->name));
}
