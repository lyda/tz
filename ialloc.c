#ifndef lint
#ifndef NOID
static char	elsieid[] = "@(#)ialloc.c	8.24";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#include "private.h"

#ifdef MAL
#define NULLMAL(x)	((x) == NULL || (x) == MAL)
#endif /* defined MAL */
#ifndef MAL
#define NULLMAL(x)	((x) == NULL)
#endif /* !defined MAL */

#define nonzero(n)	(((n) == 0) ? 1 : (n))

char *	icalloc P((int nelem, int elsize));
char *	icatalloc P((char * old, const char * new));
char *	icpyalloc P((const char * string));
char *	imalloc P((int n));
void *	irealloc P((void * pointer, int size));
void	ifree P((char * pointer));

char *
imalloc(n)
const int	n;
{
#ifdef MAL
	register char *	result;

	result = malloc((alloc_size_T) nonzero(n));
	return NULLMAL(result) ? NULL : result;
#endif /* defined MAL */
#ifndef MAL
	return malloc((alloc_size_T) nonzero(n));
#endif /* !defined MAL */
}

char *
icalloc(nelem, elsize)
int	nelem;
int	elsize;
{
	if (nelem == 0 || elsize == 0)
		nelem = elsize = 1;
	return calloc((alloc_size_T) nelem, (alloc_size_T) elsize);
}

void *
irealloc(pointer, size)
void * const	pointer;
const int	size;
{
	if (NULLMAL(pointer))
		return imalloc(size);
	return realloc((genericptr_T) pointer, (alloc_size_T) nonzero(size));
}

char *
icatalloc(old, new)
char * const		old;
const char * const	new;
{
	register char *	result;
	register int	oldsize, newsize;

	newsize = NULLMAL(new) ? 0 : strlen(new);
	if (NULLMAL(old))
		oldsize = 0;
	else if (newsize == 0)
		return old;
	else	oldsize = strlen(old);
	if ((result = irealloc(old, oldsize + newsize + 1)) != NULL)
		if (!NULLMAL(new))
			(void) strcpy(result + oldsize, new);
	return result;
}

char *
icpyalloc(string)
const char * const	string;
{
	return icatalloc((char *) NULL, string);
}

void
ifree(p)
char * const	p;
{
	if (!NULLMAL(p))
		(void) free(p);
}

void
icfree(p)
char * const	p;
{
	if (!NULLMAL(p))
		(void) free(p);
}
