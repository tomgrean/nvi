/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994, 1995
 *	Keith Bostic.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_shell.c,v 10.27 1995/11/27 11:27:40 bostic Exp $ (Berkeley) $Date: 1995/11/27 11:27:40 $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <bitstring.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/common.h"

static const char *sigmsg __P((int));

/*
 * ex_shell -- :sh[ell]
 *	Invoke the program named in the SHELL environment variable
 *	with the argument -i.
 *
 * PUBLIC: int ex_shell __P((SCR *, EXCMD *));
 */
int
ex_shell(sp, cmdp)
	SCR *sp;
	EXCMD *cmdp;
{
	int rval;
	char buf[MAXPATHLEN];

	(void)snprintf(buf, sizeof(buf), "%s -i", O_STR(sp, O_SHELL));

	/* If we're stil in a vi screen, move out explicitly. */
	rval = ex_exec_proc(sp, cmdp, buf, NULL, !F_ISSET(sp, S_SCR_EXWROTE));

	/*
	 * !!!
	 * Historically, vi didn't require a continue message after the
	 * return of the shell.  Match it.
	 */
	F_SET(sp, S_EX_DONTWAIT);

	return (rval);
}

/*
 * ex_exec_proc --
 *	Run a separate process.
 *
 * PUBLIC: int ex_exec_proc __P((SCR *, EXCMD *, char *, const char *, int));
 */
int
ex_exec_proc(sp, cmdp, cmd, msg, need_newline)
	SCR *sp;
	EXCMD *cmdp;
	char *cmd;
	const char *msg;
	int need_newline;
{
	const char *name;
	pid_t pid;

	/* Enter ex mode. */
	if (F_ISSET(sp, S_VI)) {
		if (sp->gp->scr_screen(sp, S_EX)) {
			ex_emsg(sp, cmdp->cmd->name, EXM_NOCANON);
			return (1);
		}
		F_SET(sp, S_SCR_EX | S_SCR_EXWROTE);
	}

	/* Put out additional newline, message. */
	if (need_newline)
		(void)ex_puts(sp, "\n");
	if (msg != NULL) {
		(void)ex_puts(sp, msg);
		(void)ex_puts(sp, "\n");
	}
	(void)ex_fflush(sp);

	switch (pid = vfork()) {
	case -1:			/* Error. */
		msgq(sp, M_SYSERR, "vfork");
		return (1);
	case 0:				/* Utility. */
		if ((name = strrchr(O_STR(sp, O_SHELL), '/')) == NULL)
			name = O_STR(sp, O_SHELL);
		else
			++name;
		execl(O_STR(sp, O_SHELL), name, "-c", cmd, NULL);
		msgq_str(sp, M_SYSERR, O_STR(sp, O_SHELL), "execl: %s");
		_exit(127);
		/* NOTREACHED */
	default:			/* Parent. */
		return (proc_wait(sp, (long)pid, cmd, 0, 0));
	}
	/* NOTREACHED */
}

/*
 * proc_wait --
 *	Wait for one of the processes.
 *
 * !!!
 * The pid_t type varies in size from a short to a long depending on the
 * system.  It has to be cast into something or the standard promotion
 * rules get you.  I'm using a long based on the belief that nobody is
 * going to make it unsigned and it's unlikely to be a quad.
 *
 * PUBLIC: int proc_wait __P((SCR *, long, const char *, int, int));
 */
int
proc_wait(sp, pid, cmd, silent, okpipe)
	SCR *sp;
	long pid;
	const char *cmd;
	int silent, okpipe;
{
	size_t len;
	int nf, pstat;
	char *p;

	/* Wait for the utility, ignoring interruptions. */
	for (;;) {
		errno = 0;
		if (waitpid((pid_t)pid, &pstat, 0) != -1)
			break;
		if (errno != EINTR) {
			msgq(sp, M_SYSERR, "waitpid");
			return (1);
		}
	}

	/*
	 * Display the utility's exit status.  Ignore SIGPIPE from the
	 * parent-writer, as that only means that the utility chose to
	 * exit before reading all of its input.
	 */
	if (WIFSIGNALED(pstat) && (!okpipe || WTERMSIG(pstat) != SIGPIPE)) {
		for (; isblank(*cmd); ++cmd);
		p = msg_print(sp, cmd, &nf);
		len = strlen(p);
		msgq(sp, M_ERR, "%.*s%s: received signal: %s%s",
		    MIN(len, 20), p, len > 20 ? " ..." : "",
		    sigmsg(WTERMSIG(pstat)),
		    WCOREDUMP(pstat) ? "; core dumped" : "");
		if (nf)
			FREE_SPACE(sp, p, 0);
		return (1);
	}

	if (WIFEXITED(pstat) && WEXITSTATUS(pstat)) {
		/*
		 * Remain silent for "normal" errors when doing shell file
		 * name expansions, they almost certainly indicate nothing
		 * more than a failure to match.
		 *
		 * Remain silent for vi read filter errors.  It's historic
		 * practice.
		 */
		if (!silent) {
			for (; isblank(*cmd); ++cmd);
			p = msg_print(sp, cmd, &nf);
			len = strlen(p);
			msgq(sp, M_ERR, "%.*s%s: exited with status %d",
			    MIN(len, 20), p, len > 20 ? " ..." : "",
			    WEXITSTATUS(pstat));
			if (nf)
				FREE_SPACE(sp, p, 0);
		}
		return (1);
	}
	return (0);
}

/*
 * XXX
 * The sys_siglist[] table in the C library has this information, but there's
 * no portable way to get to it.  (Believe me, I tried.)
 */
typedef struct _sigs {
	int	 number;		/* signal number */
	char	*message;		/* related message */
} SIGS;

SIGS const sigs[] = {
#ifdef SIGABRT
	SIGABRT,	"Abort trap",
#endif
#ifdef SIGALRM
	SIGALRM,	"Alarm clock",
#endif
#ifdef SIGBUS
	SIGBUS,		"Bus error",
#endif
#ifdef SIGCHLD
	SIGCHLD,	"Child exited",
#endif
#ifdef SIGCONT
	SIGCONT,	"Continued",
#endif
#ifdef SIGEMT
	SIGEMT,		"EMT trap",
#endif
#ifdef SIGFPE
	SIGFPE,		"Floating point exception",
#endif
#ifdef SIGHUP
	SIGHUP,		"Hangup",
#endif
#ifdef SIGILL
	SIGILL,		"Illegal instruction",
#endif
#ifdef SIGINFO
	SIGINFO,	"Information request",
#endif
#ifdef SIGINT
	SIGINT,		"Interrupt",
#endif
#ifdef SIGIO
	SIGIO,		"I/O possible",
#endif
#ifdef SIGKILL
	SIGKILL,	"Killed",
#endif
#ifdef SIGPIPE
	SIGPIPE,	"Broken pipe",
#endif
#ifdef SIGPROF
	SIGPROF,	"Profiling timer expired",
#endif
#ifdef SIGQUIT
	SIGQUIT,	"Quit",
#endif
#ifdef SIGSEGV
	SIGSEGV,	"Segmentation fault",
#endif
#ifdef SIGSTOP
	SIGSTOP,	"Suspended (signal)",
#endif
#ifdef SIGSYS
	SIGSYS,		"Bad system call",
#endif
#ifdef SIGTERM
	SIGTERM,	"Terminated",
#endif
#ifdef SIGTRAP
	SIGTRAP,	"Trace/BPT trap",
#endif
#ifdef SIGTSTP
	SIGTSTP,	"Suspended",
#endif
#ifdef SIGTTIN
	SIGTTIN,	"Stopped (tty input)",
#endif
#ifdef SIGTTOU
	SIGTTOU,	"Stopped (tty output)",
#endif
#ifdef SIGURG
	SIGURG,		"Urgent I/O condition",
#endif
#ifdef SIGUSR1
	SIGUSR1,	"User defined signal 1",
#endif
#ifdef SIGUSR2
	SIGUSR2,	"User defined signal 2",
#endif
#ifdef SIGVTALRM
	SIGVTALRM,	"Virtual timer expired",
#endif
#ifdef SIGWINCH
	SIGWINCH,	"Window size changes",
#endif
#ifdef SIGXCPU
	SIGXCPU,	"Cputime limit exceeded",
#endif
#ifdef SIGXFSZ
	SIGXFSZ,	"Filesize limit exceeded",
#endif
};

/*
 * sigmsg --
 * 	Return a pointer to a message describing a signal.
 */
static const char *
sigmsg(signo)
	int signo;
{
	static char buf[40];
	const SIGS *sigp;
	int n;

	for (n = 0,
	    sigp = &sigs[0]; n < sizeof(sigs) / sizeof(sigs[0]); ++n, ++sigp)
		if (sigp->number == signo)
			return (sigp->message);
	(void)snprintf(buf, sizeof(buf), "Unknown signal: %d", signo);
	return (buf);
}
