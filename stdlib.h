#ifndef lint
#ifndef NOID
#ifndef STDLIB_H
#define STDLIB_H
static char	stdlibhid[] = "@(#)stdlib.h	4.1";
#endif /* !defined STDLIB_H */
#endif /* !defined NOID */
#endif /* !defined lint */

#ifdef __STDC__

#ifdef STDLIB_RECURSING
#include "/usr/include/stdlib.h"
#else /* !defined STDLIB_RECURSING */
#define STDLIB_RECURSING
#include <stdlib.h>
#undef STDLIB_RECURSING
#endif /* !defined STDLIB_RECURSING */

#ifndef NULL
/*
** Stupid Turbo C doesn't define NULL in stdlib.h
*/
#include <stdio.h>
#endif /* !defined NULL */

#else /* !defined __STDC__ */

#ifdef unix
/*
** Include sys/param.h so we can figure out if we're BSD or USG
*/
#include "sys/param.h"
#endif /* defined unix */

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS	0
#endif /* !defined EXIT_SUCCESS */

#ifndef EXIT_FAILURE
#define EXIT_FAILURE	0
#endif /* !defined EXIT_FAILURE */

#ifndef NULL
#include <stdio.h>
#endif /* !defined NULL */

/*
** String conversion functions
*/

#include <math.h>

/*
** Memory management funcsions
*/

extern char *	calloc();
extern char *	malloc();
extern char *	realloc();

#ifndef BSD
extern void	free();
#endif /* !defined BSD */

/*
** Communication with the environment
*/

extern char *	getenv();

#ifndef BSD
extern void	exit();
#endif /* !defined BSD */

/*
** Searching and sorting functions
*/

#ifndef BSD
extern void	qsort();
#endif /* !defined BSD */

#endif /* !defined __STDC__ */

/*
** UNIX is a registered trademark of AT&T.
*/
