#ifndef lint
#ifndef NOID
static char	elsieid[] = "@(#)timemk.c	4.1";
#endif /* !defined NOID */
#endif /* !defined lint */

/*LINTLIBRARY*/

#ifdef STD_INSPIRED

/*
** Code provided by Robert Elz, who writes:
**	The "best" way to do mktime I think is based on an idea of Bob
**	Kridle's (so its said...) from a long time ago. (mtxinu!kridle now).
**	It does a binary search of the time_t space.  Since time_t's are
**	just 32 bits, its a max of 32 iterations (even at 64 bits it
**	would still be very reasonable).
**
** This code does handle "out of bounds" values in the way described
** for "mktime" in the October, 1986 draft of the proposed ANSI C Standard;
** though this is an accident of the implementation and *cannot* be made to
** work correctly for the purposes there described.
**
** A warning applies if you try to use these functions with a version of
** "localtime" that has overflow problems (such as System V Release 2.0
** or 4.3 BSD localtime).
** If you're not using GMT and feed a value to localtime
** that's near the minimum (or maximum) possible time_t value, localtime
** may return a struct that represents a time near the maximum (or minimum)
** possible time_t value (because of overflow).  If such a returned struct tm
** is fed to timelocal, it will not return the value originally feed to
** localtime.
*/

#include "time.h"
#include "tzfile.h"
#include "nonstd.h"

#ifndef WRONG
#define WRONG	(-1)
#endif /* !defined WRONG */

extern struct tm *	offtime P((const time_t * clock, long offset));

static time_t
timemk(timeptr, funcp, offset)
const struct tm *	timeptr;
struct tm * (*		funcp)();
long			offset;
{
	register int	direction;
	register int	bits;
	time_t		t;
	struct tm	yourtm, mytm;

	yourtm = *timeptr;
	/*
	** Correct the tm supplied, in case some of its values are
	** out of range.
	*/
	while (yourtm.tm_sec > SECSPERMIN)	/* could be leap sec */
		++yourtm.tm_min, yourtm.tm_sec -= SECSPERMIN;
	while (yourtm.tm_sec < 0)
		--yourtm.tm_min, yourtm.tm_sec += SECSPERMIN;
	while (yourtm.tm_min >= MINSPERHOUR)
		++yourtm.tm_hour, yourtm.tm_min -= MINSPERHOUR;
	while (yourtm.tm_min < 0)
		--yourtm.tm_hour, yourtm.tm_min += MINSPERHOUR;
	while (yourtm.tm_hour >= HOURSPERDAY)
		++yourtm.tm_mday, yourtm.tm_hour -= HOURSPERDAY;
	while (yourtm.tm_hour < 0)
		--yourtm.tm_mday, yourtm.tm_hour += HOURSPERDAY;
	while (yourtm.tm_mday > 31)		/* trust me [kre] */
		++yourtm.tm_mon, yourtm.tm_mday -= 31;
	while (yourtm.tm_mday <= 0)
		--yourtm.tm_mon, yourtm.tm_mday += 31;
	while (yourtm.tm_mon >= MONSPERYEAR)
		++yourtm.tm_year, yourtm.tm_mon -= MONSPERYEAR;
	while (yourtm.tm_mon < 0)
		--yourtm.tm_year, yourtm.tm_mon += MONSPERYEAR;
	/*
	** Calculate the number of magnitude bits in a time_t
	** (this works regardless of whether time_t is
	** signed or unsigned, though lint complains if unsigned).
	*/
	for (bits = 0, t = 1; t > 0; ++bits, t <<= 1)
		;
	/*
	** If time_t is signed, then 0 is the median value,
	** if time_t is unsigned, then 1 << bits is median.
	*/
	t = (t < 0) ? 0 : ((time_t) 1 << bits);
	for ( ; ; ) {
		mytm = (funcp == offtime) ?
			*((*funcp)(&t, offset)) : *((*funcp)(&t));
		if ((direction = (mytm.tm_year - yourtm.tm_year)) == 0 &&
			(direction = (mytm.tm_mon - yourtm.tm_mon)) == 0 &&
			(direction = (mytm.tm_mday - yourtm.tm_mday)) == 0 &&
			(direction = (mytm.tm_hour - yourtm.tm_hour)) == 0 &&
			(direction = (mytm.tm_min - yourtm.tm_min)) == 0)
				direction = mytm.tm_sec - yourtm.tm_sec;
		if (direction == 0) {
			*timeptr = mytm;
			return t;
		}
		if (bits-- < 0) {
			*timeptr = yourtm;	/* restore "original" value */
			if (yourtm.tm_mday == 31) {
				timeptr->tm_mday = 1;
				++(timeptr->tm_mon);
				t = timemk(timeptr, funcp, offset);
				if (t != WRONG)
					return t;
				*timeptr = yourtm;
			} else if (yourtm.tm_mon == TM_FEBRUARY &&
				yourtm.tm_mday > 28) {
					timeptr->tm_mday -= 28;
					++(timeptr->tm_mon);
					t = timemk(timeptr, funcp, offset);
					if (t != WRONG)
						return t;
					*timeptr = yourtm;
			}
			return WRONG;
		}
		if (bits < 0)
			--t;
		else if (direction > 0)
			t -= (time_t) 1 << bits;
		else	t += (time_t) 1 << bits;
	}
}

time_t
timelocal(timeptr)
const struct tm *	timeptr;
{
	return timemk(timeptr, localtime, 0L);
}

time_t
timegm(timeptr)
const struct tm *	timeptr;
{
	return timemk(timeptr, gmtime, 0L);
}

time_t
timeoff(timeptr, offset)
const struct tm *	timeptr;
long			offset;
{
	return timemk(timeptr, offtime, offset);
}

#endif /* defined STD_INSPIRED */
