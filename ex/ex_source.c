/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994, 1995
 *	Keith Bostic.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_source.c,v 10.4 1995/09/21 12:07:33 bostic Exp $ (Berkeley) $Date: 1995/09/21 12:07:33 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <bitstring.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/common.h"

/*
 * ex_source -- :source file
 *	Execute ex commands from a file.
 *
 * PUBLIC: int ex_source __P((SCR *, EXCMD *));
 */
int
ex_source(sp, cmdp)
	SCR *sp;
	EXCMD *cmdp;
{
	struct stat sb;
	int fd, len, nf;
	char *bp, *name, *p;

	name = cmdp->argv[0]->bp;
	if ((fd = open(name, O_RDONLY, 0)) < 0 || fstat(fd, &sb))
		goto err;

	/*
	 * XXX
	 * I'd like to test to see if the file is too large to malloc.  Since
	 * we don't know what size or type off_t's or size_t's are, what the
	 * largest unsigned integral type is, or what random insanity the local
	 * C compiler will perpetrate, doing the comparison in a portable way
	 * is flatly impossible.  So, put an fairly unreasonable limit on it,
	 * I don't want to be dropping core here.
	 */
#define	MEGABYTE	1048576
	if (sb.st_size > MEGABYTE) {
		errno = ENOMEM;
		goto err;
	}
	MALLOC(sp, bp, char *, (size_t)sb.st_size);
	if (bp == NULL) {
		(void)close(fd);
		return (1);
	}

	/* Read the file into memory. */
	len = read(fd, bp, (int)sb.st_size);
	(void)close(fd);
	if (len == -1 || len != sb.st_size) {
		if (len != sb.st_size)
			errno = EIO;

err:		p = msg_print(sp, name, &nf);
		msgq(sp, M_SYSERR, "%s", p);
		if (nf)
			FREE_SPACE(sp, p, 0);
		if (bp != NULL)
			free(bp);
		return (1);
	}

	/* Put it on the ex queue. */
	return (ex_run_str(sp, name, bp, (size_t)sb.st_size, 1));
}
