#ifndef lint
#ifndef NOID
static char	elsieid[] = "@(#)ctime.c	4.1";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#ifndef BSD_COMPAT

/*
** On non-BSD systems, this can be a separate function (as is proper).
*/

#include "time.h"
#include "nonstd.h"

char *
ctime(timep)
const time_t *	timep;
{
	return asctime(localtime(timep));
}

#endif /* !defined BSD_COMPAT */
