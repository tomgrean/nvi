/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	$Id: ex.h,v 8.74 1994/08/31 17:17:34 bostic Exp $ (Berkeley) $Date: 1994/08/31 17:17:34 $
 */

#define	PROMPTCHAR	':'		/* Prompt character. */

/* Ex command structure. */
typedef struct _excmdlist {
	char	*name;			/* Command name. */
					/* Underlying function. */
	int (*fn) __P((SCR *, EXF *, EXCMDARG *));

#define	E_ADDR1		0x0000001	/* One address. */
#define	E_ADDR2		0x0000002	/* Two address. */
#define	E_ADDR2_ALL	0x0000004	/* Zero/two addresses; zero == all. */
#define	E_ADDR2_NONE	0x0000008	/* Zero/two addresses; zero == none. */
#define	E_ADDRDEF	0x0000010	/* Default addresses used. */
#define	E_AUTOPRINT	0x0000020	/* Command always sets autoprint. */
#define	E_BUFFER	0x0000040	/* Buffer name supplied. */
#define	E_COUNT		0x0000080	/* Count supplied. */
#define	E_COUNT_NEG	0x0000100	/* Count was signed negative. */
#define	E_COUNT_POS	0x0000200	/* Count was signed positive. */
#define	E_FORCE		0x0000400	/*  ! */

#define	E_F_CARAT	0x0000800	/*  ^ flag. */
#define	E_F_DASH	0x0001000	/*  - flag. */
#define	E_F_DOT		0x0002000	/*  . flag. */
#define	E_F_EQUAL	0x0004000	/*  = flag. */
#define	E_F_HASH	0x0008000	/*  # flag. */
#define	E_F_LIST	0x0010000	/*  l flag. */
#define	E_F_PLUS	0x0020000	/*  + flag. */
#define	E_F_PRINT	0x0040000	/*  p flag. */

#define	E_F_PRCLEAR	0x0080000	/* Clear the print (#, l, p) flags. */
#define	E_MODIFY	0x0100000	/* File name expansion modified arg. */
#define	E_NOGLOBAL	0x0200000	/* Not in a global. */
#define	E_NOPERM	0x0400000	/* Permission denied for now. */
#define	E_NORC		0x0800000	/* Not from a .exrc or EXINIT. */
#define	E_ZERO		0x1000000	/* 0 is a legal addr1. */
#define	E_ZERODEF	0x2000000	/* 0 is default addr1 of empty files. */
	u_int32_t flags;
	char	 *syntax;		/* Syntax script. */
	char	*usage;			/* Usage line. */
	char	*help;			/* Help line. */
} EXCMDLIST;
#define	MAXCMDNAMELEN	12		/* Longest command name. */
extern EXCMDLIST const cmds[];		/* List of ex commands. */

/*
 * Structure passed around to functions implementing ex commands.
 * There are several commands in vi that build one of these and
 * call ex directly.  See vi/v_ex.c for details.
 */
struct _excmdarg {
	EXCMDLIST const *cmd;	/* Command entry in command table. */
	CHAR_T	  buffer;	/* Named buffer. */
	recno_t	  lineno;	/* Line number. */
	long	  count;	/* Signed, specified count. */
	long	  flagoff;	/* Signed, flag offset parsed by command. */
	int	  addrcnt;	/* Number of addresses (0, 1 or 2). */
	MARK	  addr1;	/* 1st address. */
	MARK	  addr2;	/* 2nd address. */
	ARGS	**argv;		/* Array of arguments. */
	int	  argc;		/* Count of arguments. */
	u_int32_t flags;	/* Selected flags from EXCMDLIST. */
};

/* Global ranges. */
typedef struct _range	RANGE;
struct _range {
	CIRCLEQ_ENTRY(_range) q;	/* Linked list of ranges. */
	recno_t start, stop;		/* Start/stop of the range. */
};

/* Cd paths. */
typedef struct _cdpath	CDPATH;
struct _cdpath {
	TAILQ_ENTRY(_cdpath) q;		/* Linked list of cd paths. */
	char *path;			/* Path. */
};

/* Ex private, per-screen memory. */
typedef struct _ex_private {
	ARGS   **args;			/* Arguments. */
	int	 argscnt;		/* Argument count. */
	int	 argsoff;		/* Offset into arguments. */

	CHAR_T	 at_lbuf;		/* Last executed at buffer's name. */
	int	 at_lbuf_set;		/* If at_lbuf is set. */

	char	*ibp;			/* Line input buffer. */
	size_t	 ibp_len;		/* Line input buffer length. */

	u_int32_t fdef;			/* Default command flags. */

	CHAR_T	*lastbcomm;		/* Last bang command. */

	struct termios leave_term;	/* ex_[sr]leave tty state. */
	/* XXX: Should be struct timespec's, but time_t is more portable. */
	time_t leave_atime;		/* ex_[sr]leave old access time. */
	time_t leave_mtime;		/* ex_[sr]leave old mod time. */

	TAILQ_HEAD(_tagh, _tag) tagq;	/* Tag list (stack). */
	TAILQ_HEAD(_tagfh, _tagf) tagfq;/* Tag file list. */
	char	*tlast;			/* Saved last tag. */

	TAILQ_HEAD(_cdh, _cdpath) cdq;	/* Cd path list. */

					/* Linked list of ranges. */
	CIRCLEQ_HEAD(_rangeh, _range) rangeq;
	recno_t  range_lno;		/* Range set line number. */

#define	EX_ABSMARK	0x01		/* Set the absolute mark. */
#define	EX_AUTOPRINT	0x02		/* Autoprint flag. */
	u_int8_t flags;
} EX_PRIVATE;
#define	EXP(sp)	((EX_PRIVATE *)((sp)->ex_private))

/*
 * !!!
 * Historically, .exrc files and EXINIT variables could only use ^V
 * as an escape character, neither ^Q or a user specified character
 * worked.  We enforce that here, just in case someone depends on it.
 */
#define	IS_ESCAPE(sp, ch)						\
	(F_ISSET(sp, S_VLITONLY) ?					\
	    (ch) == CH_LITERAL : KEY_VAL(sp, ch) == K_VLNEXT)

/*
 * Filter actions:
 *
 *	FILTER		Filter text through the utility.
 *	FILTER_READ	Read from the utility into the file.
 *	FILTER_WRITE	Write to the utility, display its output.
 */
enum filtertype { FILTER, FILTER_READ, FILTER_WRITE };
int	filtercmd __P((SCR *, EXF *,
	    MARK *, MARK *, MARK *, char *, enum filtertype));

/* Argument expansion routines. */
int	argv_init __P((SCR *, EXF *, EXCMDARG *));
int	argv_exp0 __P((SCR *, EXF *, EXCMDARG *, char *, size_t));
int	argv_exp1 __P((SCR *, EXF *, EXCMDARG *, char *, size_t, int));
int	argv_exp2 __P((SCR *, EXF *, EXCMDARG *, char *, size_t, int));
int	argv_exp3 __P((SCR *, EXF *, EXCMDARG *, char *, size_t));
int	argv_free __P((SCR *));

/* Ex common messages. */
enum exmtype { EXM_INTERRUPTED, EXM_NOPREVRE, EXM_USAGE };
void	ex_message __P((SCR *, EXCMDARG *, enum exmtype));

/* Ex function prototypes. */
int	ex __P((SCR *, EXF *));
int	ex_cfile __P((SCR *, EXF *, char *, int));
int	ex_cmd __P((SCR *, EXF *, char *, size_t, int));
int	ex_cdalloc __P((SCR *, char *));
int	ex_cdfree __P((SCR *));
int	ex_end __P((SCR *));
int	ex_exec_proc __P((SCR *, char *, char *, char *));
int	ex_gb __P((SCR *, EXF *, TEXTH *, int, u_int));
int	ex_getline __P((SCR *, FILE *, size_t *));
int	ex_icmd __P((SCR *, EXF *, char *, size_t, int));
int	ex_init __P((SCR *, EXF *));
int	ex_is_abbrev __P((char *, size_t));
int	ex_is_unmap __P((char *, size_t));
int	ex_ldisplay __P((SCR *, CHAR_T *, size_t, size_t, u_int));
int	ex_ncheck __P((SCR *, int));
int	ex_optchange __P((SCR *, int));
int	ex_print __P((SCR *, EXF *, MARK *, MARK *, int));
int	ex_readfp __P((SCR *, EXF *, char *, FILE *, MARK *, recno_t *, int));
void	ex_refresh __P((SCR *, EXF *));
void	ex_rleave __P((SCR *));
int	ex_screen_copy __P((SCR *, SCR *));
int	ex_screen_end __P((SCR *));
int	ex_sdisplay __P((SCR *, EXF *));
int	ex_sleave __P((SCR *));
int	ex_suspend __P((SCR *));
int	ex_tdisplay __P((SCR *, EXF *));
int	ex_writefp __P((SCR *, EXF *,
	    char *, FILE *, MARK *, MARK *, u_long *, u_long *));
void	global_insdel __P((SCR *, EXF *, enum operation, recno_t));
int	proc_wait __P((SCR *, long, const char *, int));
int	sscr_end __P((SCR *));
int	sscr_exec __P((SCR *, EXF *, recno_t));
int	sscr_input __P((SCR *));

int	abbr_save __P((SCR *, FILE *));
int	map_save __P((SCR *, FILE *));

#define	EXPROTO(name)	int name __P((SCR *, EXF *, EXCMDARG *))
EXPROTO(ex_abbr);
EXPROTO(ex_append);
EXPROTO(ex_args);
EXPROTO(ex_at);
EXPROTO(ex_bang);
EXPROTO(ex_bg);
EXPROTO(ex_cd);
EXPROTO(ex_change);
EXPROTO(ex_color);
EXPROTO(ex_copy);
EXPROTO(ex_debug);
EXPROTO(ex_delete);
EXPROTO(ex_digraph);
EXPROTO(ex_display);
EXPROTO(ex_edit);
EXPROTO(ex_equal);
EXPROTO(ex_fg);
EXPROTO(ex_file);
EXPROTO(ex_global);
EXPROTO(ex_help);
EXPROTO(ex_insert);
EXPROTO(ex_join);
EXPROTO(ex_list);
EXPROTO(ex_map);
EXPROTO(ex_mark);
EXPROTO(ex_mkexrc);
EXPROTO(ex_move);
EXPROTO(ex_next);
EXPROTO(ex_number);
EXPROTO(ex_open);
EXPROTO(ex_pr);
EXPROTO(ex_preserve);
EXPROTO(ex_prev);
EXPROTO(ex_put);
EXPROTO(ex_quit);
EXPROTO(ex_read);
EXPROTO(ex_recover);
EXPROTO(ex_resize);
EXPROTO(ex_rew);
EXPROTO(ex_script);
EXPROTO(ex_set);
EXPROTO(ex_shell);
EXPROTO(ex_shiftl);
EXPROTO(ex_shiftr);
EXPROTO(ex_source);
EXPROTO(ex_split);
EXPROTO(ex_stop);
EXPROTO(ex_subagain);
EXPROTO(ex_substitute);
EXPROTO(ex_subtilde);
EXPROTO(ex_tagpop);
EXPROTO(ex_tagpush);
EXPROTO(ex_tagtop);
EXPROTO(ex_unabbr);
EXPROTO(ex_undo);
EXPROTO(ex_unmap);
EXPROTO(ex_usage);
EXPROTO(ex_validate);
EXPROTO(ex_version);
EXPROTO(ex_vglobal);
EXPROTO(ex_visual);
EXPROTO(ex_viusage);
EXPROTO(ex_wn);
EXPROTO(ex_wq);
EXPROTO(ex_write);
EXPROTO(ex_xit);
EXPROTO(ex_yank);
EXPROTO(ex_z);
