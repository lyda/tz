#

#include "stdio.h"

#ifdef OBJECTID
static char	sccsid[] = "@(#)tzcomp.c	2.1";
#endif

#include "tzfile.h"
#include "ctype.h"

#ifndef alloc_t
#define alloc_t	unsigned
#endif

#ifndef MAL
#define MAL	NULL
#endif

#ifndef BUFSIZ
#define BUFSIZ	1024
#endif

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#ifdef lint
#define scheck(string, format)	(format)
#endif
#ifndef lint
extern char *	scheck();
#endif

extern char *	calloc();
extern char *	malloc();
extern char *	optarg;
extern int	optind;
extern FILE *	popen();
extern char *	realloc();
extern char *	sprintf();
extern char *	strcat();
extern char *	strchr();
extern char *	strcpy();

static int	errors;
static char *	filename;
static char **	getfields();
static int	linenum;
static char *	progname;
static long	rpytime();
static long	tadd();

#define	SECS_PER_MIN	60L
#define MINS_PER_HOUR	60L
#define HOURS_PER_DAY	24L
#define DAYS_PER_YEAR	365L	/* Except in leap years */
#define	SECS_PER_HOUR	(SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY	(SECS_PER_HOUR * HOURS_PER_DAY)
#define SECS_PER_YEAR	(SECS_PER_DAY * DAYS_PER_YEAR)

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

/*
** Values a la localtime(3)
*/

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

/*
** Line codes.
*/

#define LC_RULE		0
#define LC_ZONE		1
#define LC_LINK		2

/*
** Which fields are which on a Zone line.
*/

#define ZF_NAME		1
#define ZF_GMTOFF	2
#define ZF_RULE		3
#define ZF_FORMAT	4
#define ZONE_FIELDS	5

/*
** Which files are which on a Rule line.
*/

#define	RF_NAME		1
#define RF_LOYEAR	2
#define RF_HIYEAR	3
#define RF_COMMAND	4
#define RF_MONTH	5
#define RF_DAY		6
#define RF_TOD		7
#define RF_STDOFF	8
#define RF_ABBRVAR	9
#define RULE_FIELDS	10

/*
** Which fields are which on a Link line.
*/

#define LF_FROM		1
#define LF_TO		2
#define LINK_FIELDS	3

struct rule {
	char *	r_filename;
	int	r_linenum;
	char *	r_name;

	long	r_loyear;	/* for example, 1986 */
	long	r_hiyear;	/* for example, 1986 */
	char *	r_yrtype;

	long	r_month;	/* 0..11 */

	int	r_dycode;	/* see below */
	long	r_dayofmonth;
	long	r_wday;

	long	r_tod;		/* time from midnight */
	int	r_todisstd;	/* above is standard time if TRUE */
				/* above is wall clock time if FALSE */
	long	r_stdoff;	/* offset from standard time */
	char *	r_abbrvar;	/* variable part of time zone abbreviation */

	int	r_type;	/* used when creating output files */
};

/*
**	r_dycode		r_dayofmonth	r_wday
*/
#define DC_DOM		0	/* 1..31 */	/* unused */
#define DC_DOWGEQ	1	/* 1..31 */	/* 0..6 (Sun..Sat) */
#define DC_DOWLEQ	2	/* 1..31 */	/* 0..6 (Sun..Sat) */

static struct rule *	rules;
static int		nrules;	/* number of rules */

struct zone {
	char *		z_filename;
	int		z_linenum;

	char *		z_name;
	long		z_gmtoff;
	char *		z_rule;
	char *		z_format;

	struct rule *	z_rules;
	int		z_nrules;
};

static struct zone *	zones;
static int		nzones;	/* number of zones */

struct link {
	char *		l_filename;
	int		l_linenum;
	char *		l_from;
	char *		l_to;
};

static struct link *	links;
static int		nlinks;

struct lookup {
	char *		l_word;
	long		l_value;
};

static struct lookup *	byword();

static struct lookup	line_codes[] = {
	"Rule",		LC_RULE,
	"Zone",		LC_ZONE,
	"Link",		LC_LINK,
	NULL,		0
};

static struct lookup	mon_names[] = {
	"January",	TM_JANUARY,
	"February",	TM_FEBRUARY,
	"March",	TM_MARCH,
	"April",	TM_APRIL,
	"May",		TM_MAY,
	"June",		TM_JUNE,
	"July",		TM_JULY,
	"August",	TM_AUGUST,
	"September",	TM_SEPTEMBER,
	"October",	TM_OCTOBER,
	"November",	TM_NOVEMBER,
	"December",	TM_DECEMBER,
	NULL,		0
};

static struct lookup	wday_names[] = {
	"Sunday",	TM_SUNDAY,
	"Monday",	TM_MONDAY,
	"Tuesday",	TM_TUESDAY,
	"Wednesday",	TM_WEDNESDAY,
	"Thursday",	TM_THURSDAY,
	"Friday",	TM_FRIDAY,
	"Saturday",	TM_SATURDAY,
	NULL,		0
};

static struct lookup	lasts[] = {
	"last-Sunday",		TM_SUNDAY,
	"last-Monday",		TM_MONDAY,
	"last-Tuesday",		TM_TUESDAY,
	"last-Wednesday",	TM_WEDNESDAY,
	"last-Thursday",	TM_THURSDAY,
	"last-Friday",		TM_FRIDAY,
	"last-Saturday",	TM_SATURDAY,
	NULL,			0
};

static long	mon_lengths[] = {	/* ". . .knuckles are 31. . ." */
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static struct tzhead	h;
static long		ats[TZ_MAX_TIMES];
static unsigned char	types[TZ_MAX_TIMES];
static struct ttinfo	ttis[TZ_MAX_TYPES];
static char		chars[TZ_MAX_CHARS];

/*
** Exits.
*/

static
tameexit()
{
	exit(0);
}

static
wild2exit(part1, part2)
char *	part1;
char *	part2;
{
	register char *	between;

	if (part1 == NULL)
		part1 = "";
	if (part2 == NULL)
		part2 = "";
	between = (*part1 == '\0' || *part2 == '\0') ? "" : " ";
	(void) fprintf(stderr, "%s: wild %s%s%s\n",
		progname, part1, between, part2);
	exit(1);
}

static
wildexit(string)
char *	string;
{
	wild2exit(string, (char *) NULL);
}

static
wildrexit(string)
char *	string;
{
	wild2exit("result from", string);
}

/*
** Memory allocation.
*/

static char *
emalloc(size)
{
	register char *	cp;

	if ((cp = malloc((alloc_t) size)) == NULL || cp == MAL)
		wildrexit("malloc");
	return cp;
}

static char *
erealloc(ptr, size)
char *	ptr;
{
	register char *	cp;

	if ((cp = realloc(ptr, (alloc_t) size)) == NULL)
		wildrexit("realloc");
	return cp;
}

static char *
eallocpy(old)
char *	old;
{
	register char *	new;

	if (old == NULL)
		old = "";
	new = emalloc(strlen(old) + 1);
	(void) strcpy(new, old);
	return new;
}

static
usage()
{
	(void) fprintf(stderr,
	"%s: usage is %s [ -l localtime ] [ -d directory ] [ filename ... ]\n",
		progname, progname);
	exit(1);
}

static char *	localtime = NULL;
static char *	directory = NULL;

main(argc, argv)
int	argc;
char *	argv[];
{
	register int	i;
	register int	c;

	progname = argv[0];
	while ((c = getopt(argc, argv, "d:l:")) != EOF)
		switch (c) {
			default:
				usage();
			case 'd':
				if (directory == NULL)
					directory = optarg;
				else	wildexit("multiple command line -d's");
				break;
			case 'l':
				if (localtime == NULL)
					localtime = optarg;
				else	wildexit("multiple command line -l's");
		}
	if (directory == NULL)
		directory = TZDIR;
	if (optind == argc - 1 && strcmp(argv[optind], "=") == 0)
		usage();	/* usage message by request */
	zones = (struct zone *) emalloc(0);
	rules = (struct rule *) emalloc(0);
	links = (struct link *) emalloc(0);
	for (i = optind; i < argc; ++i)
		infile(argv[i]);
	if (errors)
		wildexit("input data");
	associate();
	for (i = 0; i < nzones; ++i)
		outzone(&zones[i]);
	/*
	** We'll take the easy way out on this last part.
	*/
	if (chdir(directory) != 0)
		wild2exit("result from chdir to", directory);
	for (i = 0; i < nlinks; ++i) {
		(void) unlink(links[i].l_to);
		if (link(links[i].l_from, links[i].l_to) != 0)
			wild2exit("result creating", links[i].l_to);
	}
	if (localtime != NULL) {
		(void) unlink(TZDEFAULT);
		if (link(localtime, TZDEFAULT) != 0)
			wild2exit("result creating", TZDEFAULT);
	}
	tameexit();
}

/*
** Associate sets of rules with zones.
*/

/*
** Sort by rule name, and by magnitude of standard time offset for rules of
** the same name.  The second sort gets standard time entries to the start
** of the dsinfo table (and we want a standard time entry at the start of
** the table, since the first entry gets used for times not covered by the
** rules).
*/

static
rcomp(cp1, cp2)
char *	cp1;
char *	cp2;
{
	register struct rule *	rp1;
	register struct rule *	rp2;
	register long		l1, l2;
	register int		diff;

	rp1 = (struct rule *) cp1;
	rp2 = (struct rule *) cp2;
	if ((diff = strcmp(rp1->r_name, rp2->r_name)) != 0)
		return diff;
	if ((l1 = rp1->r_stdoff) < 0)
		l1 = -l1;
	if ((l2 = rp2->r_stdoff) < 0)
		l2 = -l2;
	if (l1 > l2)
		return 1;
	else if (l1 < l2)
		return -1;
	else	return 0;
}

static
associate()
{
	register struct zone *	zp;
	register struct rule *	rp;
	register int		base, out;
	register int		i;

	if (nrules != 0)
		(void) qsort((char *) rules, nrules, sizeof *rules, rcomp);
	base = 0;
	for (i = 0; i < nzones; ++i) {
		zp = &zones[i];
		zp->z_rules = NULL;
		zp->z_nrules = 0;
	}
	while (base < nrules) {
		rp = &rules[base];
		for (out = base + 1; out < nrules; ++out)
			if (strcmp(rp->r_name, rules[out].r_name) != 0)
				break;
		for (i = 0; i < nzones; ++i) {
			zp = &zones[i];
			if (strcmp(zp->z_rule, rp->r_name) != 0)
				continue;
			zp->z_rules = rp;
			zp->z_nrules = out - base;
		}
		base = out;
	}
	for (i = 0; i < nzones; ++i) {
		zp = &zones[i];
		if (*zp->z_rule != '\0' && zp->z_nrules == 0) {
			filename = zp->z_filename;
			linenum = zp->z_linenum;
			error("unruly zone");
		}
	}
	if (errors)
		wildexit("unruly zone(s)");
}

static
error(string)
char *	string;
{
	(void) fprintf(stderr, "%s: file \"%s\", line %d: wild %s\n",
		progname, filename, linenum, string);
	++errors;
}

static
infile(name)
char *	name;
{
	register FILE *			fp;
	register char **		fields;
	register char *			cp;
	register struct lookup *	lp;
	register int			nfields;
	char				buf[BUFSIZ];

	if (strcmp(name, "-") == 0) {
		name = "standard input";
		fp = stdin;
	} else if ((fp = fopen(name, "r")) == NULL)
		wild2exit("result opening", name);
	filename = eallocpy(name);
	for (linenum = 1; ; ++linenum) {
		if (fgets(buf, sizeof buf, fp) != buf)
			break;
		cp = strchr(buf, '\n');
		if (cp == NULL) {
			error("long line");
			wildexit("input data");
		}
		*cp = '\0';
		if ((fields = getfields(buf)) == NULL)
			wildrexit("getfields");
		nfields = 0;
		while (fields[nfields] != NULL) {
			if (ciequal(fields[nfields], "-"))
				fields[nfields] = "";
			++nfields;
		}
		if (nfields > 0)	/* non-blank line */
			if ((lp = byword(fields[0], line_codes)) == NULL)
				error("input line of unknown type");
			else switch (lp->l_value) {
				case LC_RULE:
					inrule(fields, nfields);
					break;
				case LC_ZONE:
					inzone(fields, nfields);
					break;
				case LC_LINK:
					inlink(fields, nfields);
					break;
				default:	/* "cannot happen" */
					wildrexit("lookup");
			}
		free((char *) fields);
	}
	if (ferror(fp))
		wild2exit("result reading", filename);
	if (fp != stdin && fclose(fp))
		wild2exit("result closing", filename);
}

/*
** Convert a string of one of the forms
**	h	-h 	hh:mm	-hh:mm	hh:mm:ss	-hh:mm:ss
** into a number of seconds. 
** Call error with errstring and return zero on errors.
*/

static long
getoff(string, errstring)
char *	string;
char *	errstring;
{
	long	hh, mm, ss, sign;

	if (*string == '-') {
		sign = -1;
		++string;
	} else	sign = 1;
	if (sscanf(string, scheck(string, "%ld"), &hh) == 1)
		mm = ss = 0;
	else if (sscanf(string, scheck(string, "%ld:%ld"), &hh, &mm) == 2)
		ss = 0;
	else if (sscanf(string, scheck(string, "%ld:%ld:%ld"),
		&hh, &mm, &ss) != 3) {
			error(errstring);
			return 0;
	}
	if (hh < 0 || hh >= HOURS_PER_DAY ||
		mm < 0 || mm >= MINS_PER_HOUR ||
		ss < 0 || ss >= SECS_PER_MIN) {
			error(errstring);
			return 0;
	}
	return (long) sign * (((hh * MINS_PER_HOUR) + mm) * SECS_PER_MIN + ss);
}

static
inrule(fields, nfields)
char **	fields;
{
	register struct lookup *	lp;
	register char *			cp;
	struct rule			r;

	if (nfields != RULE_FIELDS) {
		error("number of fields on Rule line");
		return;
	}
	r.r_filename = filename;
	r.r_linenum = linenum;
	if ((lp = byword(fields[RF_MONTH], mon_names)) == NULL) {
		error("month name");
		return;
	}
	r.r_month = lp->l_value;
	r.r_todisstd = FALSE;
	cp = fields[RF_TOD];
	if (strlen(cp) > 0) {
		cp += strlen(cp) - 1;
		switch (lowerit(*cp)) {
			case 's':
				r.r_todisstd = TRUE;
				*cp = '\0';
				break;
			case 'w':
				r.r_todisstd = FALSE;
				*cp = '\0';
				break;
		}
	}
	r.r_tod = getoff(fields[RF_TOD], "time of day");
	r.r_stdoff = getoff(fields[RF_STDOFF], "Standard Time offset");
	/*
	** Year work.
	*/
	cp = fields[RF_LOYEAR];
	if (sscanf(cp, scheck(cp, "%ld"), &r.r_loyear) != 1 ||
		r.r_loyear <= 0) {
			error("low year");
			return;
	}
	cp = fields[RF_HIYEAR];
	if (*cp == '\0' || ciequal(cp, "only"))
		r.r_hiyear = r.r_loyear;
	else if (sscanf(cp, scheck(cp, "%ld"), &r.r_hiyear) != 1 ||
		r.r_hiyear <= 0) {
			error("high year");
			return;
	}
	if (r.r_loyear > r.r_hiyear) {
		error("low year (greater than high year)");
		return;
	}
	if (*fields[RF_COMMAND] == '\0')
		r.r_yrtype = NULL;
	else {
		if (r.r_loyear == r.r_hiyear) {
			error("typed single year");
			return;
		}
		r.r_yrtype = eallocpy(fields[RF_COMMAND]);
	}
	/*
	** Day work.
	** Accept things such as:
	**	1
	**	last-Sunday
	**	Sun<=20
	**	Sun>=7
	*/
	cp = fields[RF_DAY];
	if ((lp = byword(cp, lasts)) != NULL) {
		r.r_dycode = DC_DOWLEQ;
		r.r_wday = lp->l_value;
		r.r_dayofmonth = mon_lengths[r.r_month];
		if (r.r_month == TM_FEBRUARY)
			++r.r_dayofmonth;
	} else {
		if ((cp = strchr(fields[RF_DAY], '<')) != 0)
			r.r_dycode = DC_DOWLEQ;
		else if ((cp = strchr(fields[RF_DAY], '>')) != 0)
			r.r_dycode = DC_DOWGEQ;
		else {
			cp = fields[RF_DAY];
			r.r_dycode = DC_DOM;
		}
		if (r.r_dycode != DC_DOM) {
			*cp++ = 0;
			if (*cp++ != '=') {
				error("day of month");
				return;
			}
			if ((lp = byword(fields[RF_DAY], wday_names)) == NULL) {
				error("weekday name");
				return;
			}
			r.r_wday = lp->l_value;
		}
		if (sscanf(cp, scheck(cp, "%ld"), &r.r_dayofmonth) != 1 ||
			r.r_dayofmonth <= 0 ||
			(r.r_dayofmonth > mon_lengths[r.r_month] &&
			r.r_month != TM_FEBRUARY && r.r_dayofmonth != 29)) {
				error("day of month");
				return;
		}
	}
	if (*fields[RF_NAME] == '\0') {
		error("nameless rule");
		return;
	}
	r.r_name = eallocpy(fields[RF_NAME]);
	r.r_abbrvar = eallocpy(fields[RF_ABBRVAR]);
	rules = (struct rule *) erealloc((char *) rules,
		(nrules + 1) * sizeof *rules);
	rules[nrules++] = r;
}

static
inzone(fields, nfields)
char **	fields;
{
	register char *	cp;
	register int	i;
	struct zone	z;
	char		buf[132];

	if (nfields != ZONE_FIELDS) {
		error("number of fields on Zone line");
		return;
	}
	z.z_filename = filename;
	z.z_linenum = linenum;
	for (i = 0; i < nzones; ++i)
		if (strcmp(zones[i].z_name, fields[ZF_NAME]) == 0) {
			(void) sprintf(buf,
				"duplicate zone name %s (file \"%s\", line %d)",
				fields[ZF_NAME],
				zones[i].z_filename,
				zones[i].z_linenum);
			error(buf);
			return;
		}
	z.z_gmtoff = getoff(fields[ZF_GMTOFF], "GMT offset");
	if ((cp = strchr(fields[ZF_FORMAT], '%')) != 0) {
		if (*++cp != 's' || strchr(cp, '%') != 0) {
			error("format");
			return;
		}
	}
	z.z_name = eallocpy(fields[ZF_NAME]);
	z.z_rule = eallocpy(fields[ZF_RULE]);
	z.z_format = eallocpy(fields[ZF_FORMAT]);
	zones = (struct zone *) erealloc((char *) zones,
		(nzones + 1) * sizeof *zones);
	zones[nzones++] = z;
}

static
inlink(fields, nfields)
char **	fields;
{
	struct link	l;

	if (nfields != LINK_FIELDS) {
		error("number of fields on Link line");
		return;
	}
	if (*fields[LF_FROM] == '\0') {
		error("blank FROM field on Link line");
		return;
	}
	if (*fields[LF_TO] == '\0') {
		error("blank TO field on Link line");
		return;
	}
	l.l_filename = filename;
	l.l_linenum = linenum;
	l.l_from = eallocpy(fields[LF_FROM]);
	l.l_to = eallocpy(fields[LF_TO]);
	links = (struct link *) erealloc((char *) links,
		(nlinks + 1) * sizeof *links);
	links[nlinks++] = l;
}

static
writezone(name)
char *	name;
{
	register FILE *	fp;
	register int	i;
	char		fullname[BUFSIZ];

	if (strlen(directory) + 1 + strlen(name) >= BUFSIZ)
		wild2exit("long directory/file", filename);
	(void) sprintf(fullname, "%s/%s", directory, name);
	if ((fp = fopen(fullname, "w")) == NULL) {
		mkdirs(fullname);
		if ((fp = fopen(fullname, "w")) == NULL)
			wild2exit("result creating", fullname);
	}
	if (fwrite((char *) &h, sizeof h, 1, fp) != 1)
		goto wreck;
	if ((i = h.tzh_timecnt) != 0) {
		if (fwrite((char *) ats, sizeof ats[0], i, fp) != i)
			goto wreck;
		if (fwrite((char *) types, sizeof types[0], i, fp) != i)
			goto wreck;
	}
	if ((i = h.tzh_typecnt) != 0)
		if (fwrite((char *) ttis, sizeof ttis[0], i, fp) != i)
			goto wreck;
	if ((i = h.tzh_charcnt) != 0)
		if (fwrite(chars, sizeof chars[0], i, fp) != i)
			goto wreck;
	if (fclose(fp))
		wild2exit("result closing", fullname);
	return;
wreck:
	wild2exit("result writing", fullname);
}

struct temp {
	long		t_time;
	struct rule *	t_rp;
};

static struct temp	temps[TZ_MAX_TIMES];
static int		ntemps;

static
tcomp(cp1, cp2)
char *	cp1;
char *	cp2;
{
	register struct temp *	tp1;
	register struct temp *	tp2;
	register char *		cp;
	register long		diff;

	tp1 = (struct temp *) cp1;
	tp2 = (struct temp *) cp2;
	if (tp1->t_time > 0 && tp2->t_time <= 0)
		return 1;
	if (tp1->t_time <= 0 && tp2->t_time > 0)
		return -1;
	if ((diff = tp1->t_time - tp2->t_time) > 0)
		return 1;
	else if (diff < 0)
		return -1;
	/*
	** Oops!
	*/
	if (tp1->t_rp->r_type == tp2->t_rp->r_type)
		cp = "duplicate rule?!";
	else	cp = "inconsistent rules?!";
	filename = tp1->t_rp->r_filename;
	linenum = tp1->t_rp->r_linenum;
	error(cp);
	filename = tp2->t_rp->r_filename;
	linenum = tp2->t_rp->r_linenum;
	error(cp);
	wildexit(cp);
	/*NOTREACHED*/
}

static
addrule(rp, y)
register struct rule *	rp;
long			y;
{
	if (ntemps >= TZ_MAX_TIMES) {
		error("too many transitions?!");
		wildexit("large number of transitions");
	}
	temps[ntemps].t_time = rpytime(rp, y);
	temps[ntemps].t_rp = rp;
	++ntemps;
}

static
outzone(zp)
register struct zone *	zp;
{
	register struct rule *		rp;
	register int			i, j;
	register long			y;
	register long			gmtoff;
	char				buf[BUFSIZ];

	h.tzh_timecnt = 0;
	h.tzh_typecnt = 0;
	h.tzh_charcnt = 0;
	if (zp->z_nrules == 0) {	/* Piece of cake! */
		h.tzh_timecnt = 0;
		h.tzh_typecnt = 1;
		ttis[0].tt_gmtoff = zp->z_gmtoff;
		ttis[0].tt_isdst = 0;
		ttis[0].tt_abbrind = 0;
		newabbr(zp->z_format);
		writezone(zp->z_name);
		return;
	}
	/*
	** See what the different local time types are.
	** Plug the indices into the rules.
	*/
	for (i = 0; i < zp->z_nrules; ++i) {
		rp = &zp->z_rules[i];
		(void) sprintf(buf, zp->z_format, rp->r_abbrvar);
		gmtoff = tadd(zp->z_gmtoff, rp->r_stdoff);
		for (j = 0; j < h.tzh_typecnt; ++j) {
			if (gmtoff == ttis[j].tt_gmtoff &&
				strcmp(buf, &chars[ttis[j].tt_abbrind]) == 0)
					break;
		}
		if (j >= h.tzh_typecnt) {
			if (h.tzh_typecnt >= TZ_MAX_TYPES) {
				filename = zp->z_filename;
				linenum = zp->z_linenum;
				error("large number of local time types");
				wildexit("input data");
			}
			ttis[j].tt_gmtoff = gmtoff;
			ttis[j].tt_isdst = rp->r_stdoff != 0;
			ttis[j].tt_abbrind = h.tzh_charcnt;
			newabbr(buf);
			++h.tzh_typecnt;
		}
		rp->r_type = j;
	}
	/*
	** Now. . .finally. . .generate some useable data!
	*/
	ntemps = 0;
	for (i = 0; i < zp->z_nrules; ++i) {
		rp = &zp->z_rules[i];
		filename = rp->r_filename;
		linenum = rp->r_linenum;
		if (rp->r_yrtype != NULL && *rp->r_yrtype != '\0')
			hard(rp);
		else for (y = rp->r_loyear; y <= rp->r_hiyear; ++y)
			addrule(rp, y);
	}
	h.tzh_timecnt = ntemps;
	(void) qsort((char *) temps, ntemps, sizeof *temps, tcomp);
	for (i = 0; i < ntemps; ++i) {
		rp = temps[i].t_rp;
		filename = rp->r_filename;
		linenum = rp->r_linenum;
		types[i] = rp->r_type;
		ats[i] = tadd(temps[i].t_time, -zp->z_gmtoff);
		if (!rp->r_todisstd) {
			/*
			** Credit to munnari!kre for pointing out the need for
			** the following.  (This can still mess up on the
			** earliest rule; who's got the solution?  It can also
			** mess up if a time switch results in a day switch;
			** this is left as an exercise for the reader.)
			*/
			if (i == 0) {
				/*
				** Kludge--not guaranteed to work.
				*/
				if (ntemps > 1)
					ats[0] = tadd(ats[0],
						-temps[1].t_rp->r_stdoff);
			} else	ats[i] = tadd(ats[i],
				-temps[i - 1].t_rp->r_stdoff);
		}
	}
	writezone(zp->z_name);
	return;
}

static
hard(rp)
register struct rule *	rp;
{
	register FILE *	fp;
	register int	n;
	long		y;
	char		buf[BUFSIZ];
	char		command[BUFSIZ];

	(void) sprintf(command, "years %ld %ld %s",
		rp->r_loyear, rp->r_hiyear, rp->r_yrtype);
	if ((fp = popen(command, "r")) == NULL)
		wild2exit("result opening pipe to", command);
	for (n = 0; fgets(buf, sizeof buf, fp) == buf; ++n) {
		if (strchr(buf, '\n') == 0)
			wildrexit(command);
		*strchr(buf, '\n') = '\0';
		if (sscanf(buf, scheck(buf, "%ld"), &y) != 1)
			wildrexit(command);
		if (y < rp->r_loyear || y > rp->r_hiyear)
			wildrexit(command);
		addrule(rp, y);
	}
	if (ferror(fp))
		wild2exit("result reading from", command);
	if (pclose(fp))
		wild2exit("result closing pipe to", command);
	if (n == 0) {
		error("no year in range matches type");
		wildexit("input data");
	}
}

static
lowerit(a)
{
	return (isascii(a) && isupper(a)) ? tolower(a) : a;
}

static
ciequal(ap, bp)		/* case-insensitive equality */
register char *	ap;
register char *	bp;
{
	while (lowerit(*ap) == lowerit(*bp++))
		if (*ap++ == '\0')
			return TRUE;
	return FALSE;
}

static
isabbr(abbr, word)
register char *		abbr;
register char *		word;
{
	if (lowerit(*abbr) != lowerit(*word))
		return FALSE;
	++word;
	while (*++abbr != '\0')
		do if (*word == '\0')
			return FALSE;
				while (lowerit(*word++) != lowerit(*abbr));
	return TRUE;
}

static struct lookup *
byword(word, table)
register char *			word;
register struct lookup *	table;
{
	register struct lookup *	foundlp;
	register struct lookup *	lp;

	if (word == NULL || table == NULL)
		return NULL;
	foundlp = NULL;
	for (lp = table; lp->l_word != NULL; ++lp)
		if (ciequal(word, lp->l_word))		/* "exact" match */
			return lp;
		else if (!isabbr(word, lp->l_word))
			continue;
		else if (foundlp == NULL)
			foundlp = lp;
		else	return NULL;		/* two inexact matches */
	return foundlp;
}

static char **
getfields(cp)
register char *	cp;
{
	register char *		dp;
	register char **	array;
	register int		nsubs;

	if (cp == NULL)
		return NULL;
	array = (char **) emalloc((strlen(cp) + 1) * sizeof *array);
	nsubs = 0;
	for ( ; ; ) {
		while (isascii(*cp) && isspace(*cp))
			++cp;
		if (*cp == '\0' || *cp == '#')
			break;
		array[nsubs++] = dp = cp;
		do {
			if ((*dp = *cp++) != '"')
				++dp;
			else while ((*dp = *cp++) != '"')
				if (*dp != '\0')
					++dp;
				else	error("Odd number of quotation marks");
		} while (*cp != '\0' && *cp != '#' &&
			(!isascii(*cp) || !isspace(*cp)));
		if (isascii(*cp) && isspace(*cp))
			++cp;
		*dp++ = '\0';
	}
	array[nsubs] = NULL;
	return array;
}

static long
tadd(t1, t2)
long	t1;
long	t2;
{
	register long	t;

	t = t1 + t2;
	if (t1 > 0 && t2 > 0 && t <= 0 || t1 < 0 && t2 < 0 && t >= 0) {
		error("time overflow");
		wildexit("time overflow");
	}
	return t;
}

static
isleap(y)
long	y;
{
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

static long
rpytime(rp, wantedy)
register struct rule *	rp;
register long		wantedy;
{
	register long	i, y, wday, t, m;
	register long	dayoff;			/* with a nod to Margaret O. */

	dayoff = 0;
	m = TM_JANUARY;
	y = EPOCH_YEAR;
	while (wantedy != y) {
		if (wantedy > y) {
			i = DAYS_PER_YEAR;
			if (isleap(y))
				++i;
			++y;
		} else {
			--y;
			i = -DAYS_PER_YEAR;
			if (isleap(y))
				--i;
		}
		dayoff = tadd(dayoff, i);
	}
	while (m != rp->r_month) {
		i = mon_lengths[m];
		if (m == TM_FEBRUARY && isleap(y))
			++i;
		dayoff = tadd(dayoff, i);
		++m;
	}
	i = rp->r_dayofmonth;
	if (m == TM_FEBRUARY && i == 29 && !isleap(y)) {
		if (rp->r_dycode == DC_DOWLEQ)
			--i;
		else {
			error("use of 2/29 in non leap-year");
			for ( ; ; )
				wildexit("data");
		}
	}
	--i;
	dayoff = tadd(dayoff, i);
	if (rp->r_dycode == DC_DOWGEQ || rp->r_dycode == DC_DOWLEQ) {
		wday = EPOCH_WDAY;
		/*
		** Don't trust mod of negative numbers.
		*/
		if (dayoff >= 0)
			wday = (wday + dayoff) % 7;
		else {
			wday -= ((-dayoff) % 7);
			if (wday < 0)
				wday += 7;
		}
		while (wday != rp->r_wday) {
			if (rp->r_dycode == DC_DOWGEQ)
				i = 1;
			else	i = -1;
			dayoff = tadd(dayoff, i);
			wday = (wday + i + 7) % 7;
		}
	}
	t = dayoff * SECS_PER_DAY;
	/*
	** Cheap overflow check.
	*/
	if (t / SECS_PER_DAY != dayoff)
		error("time overflow");
	return tadd(t, rp->r_tod);
}

static
newabbr(string)
char *	string;
{
	register int	i;

	i = strlen(string);
	if (h.tzh_charcnt + i >= TZ_MAX_CHARS)
		error("long time zone abbreviations");
	(void) strcpy(&chars[h.tzh_charcnt], string);
	h.tzh_charcnt += i + 1;
}

static
mkdirs(name)
char *	name;
{
	register char *	cp;

	if ((cp = name) == NULL || *cp == '\0')
		return;
	while ((cp = strchr(cp + 1, '/')) != 0) {
		*cp = '\0';
		(void) mkdir(name);
		*cp = '/';
	}
}
