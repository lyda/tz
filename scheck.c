#

/*LINTLIBRARY*/

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "@(#)scheck.c	7.9";
#endif

#include "ctype.h"

#ifndef alloc_t
#define alloc_t	unsigned
#endif

#ifndef MAL
#define MAL	NULL
#endif

extern char *	malloc();

char *
scheck(string, format)
char *	string;
char *	format;
{
	register char *	fbuf;
	register char *	fp;
	register char *	tp;
	register int	c;
	register char *	result;
	char		dummy;

	result = "";
	if (string == NULL || format == NULL)
		return result;
	fbuf = malloc((alloc_t) (2 * strlen(format) + 4));
	if (fbuf == MAL)
		return result;
	fp = format;
	tp = fbuf;
	while ((*tp++ = c = *fp++) != '\0') {
		if (c != '%')
			continue;
		if (*fp == '%') {
			*tp++ = *fp++;
			continue;
		}
		*tp++ = '*';
		if (*fp == '*')
			++fp;
		while (isascii(*fp) && isdigit(*fp))
			*tp++ = *fp++;
		if (*fp == 'l' || *fp == 'h')
			*tp++ = *fp++;
		else if (*fp == '[')
			do *tp++ = *fp++;
				while (*fp != '\0' && *fp != ']');
		if ((*tp++ = *fp++) == '\0')
			break;
	}
	*(tp - 1) = '%';
	*tp++ = 'c';
	*tp++ = '\0';
	if (sscanf(string, fbuf, &dummy) != 1)
		result = format;
	free(fbuf);
	return result;
}
