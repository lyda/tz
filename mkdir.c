#

/*LINTLIBRARY*/

#include "stdio.h"

#ifdef OJBECTID
static char	sccsid[] = "@(#)mkdir.c	7.2";
#endif

extern FILE *	popen();

mkdir(name)
char *	name;
{
	register FILE *	fp;
	register int	c;
	register int	oops;

	if ((fp = popen("sh", "w")) == NULL)
		return -1;
	(void) fputs("mkdir 2>&- '", fp);
	if (name != NULL)
		while ((c = *name++) != '\0')
			if (c == '\'')
				(void) fputs("'\\''", fp);
			else	(void) fputc(c, fp);
	(void) fputs("'\n", fp);
	oops = ferror(fp);
	return (pclose(fp) == 0 && !oops) ? 0 : -1;
}
