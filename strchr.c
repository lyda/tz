#

/*LINTLIBRARY*/

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "@(#)strchr.c	7.3";
#endif

/*
** For the benefit of BSD folks.
** This is written from the manual description,
** so there's no guarantee that it works the same as the "real thing."
*/

char *	
strchr(string, c)
register char *	string;
register int	c;
{
	do if (*string == c)
		return string;
			while (*string++ != '\0');
	return NULL;
}
