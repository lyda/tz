#ifndef lint
#ifndef NOID
static char	elsieid[] = "@(#)dysize.c	4.1";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#ifdef BSD_COMPAT

#include "tzfile.h"

dysize(y)
{
	/*
	** The 4.[0123]BSD version of dysize behaves as if the return statement
	** below read
	**	return ((y % 4) == 0) ? DAYSPERLYEAR : DAYSPERNYEAR;
	** but since we'd rather be right than (strictly) compatible. . .
	*/
	return isleap(y) ? DAYSPERLYEAR : DAYSPERNYEAR;
}

#endif /* defined BSD_COMPAT */
