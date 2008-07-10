/* util.c - 10.7.2008 - 10.7.2008 Ari & Tero Roponen */
#include <regex.h>
#include <stdio.h>
#include "util.h"

/* Print lines from STR that match REGEXP.
   WHERE and PAGE specify the current filename and page number.
   If WHERE is NULL, don't display matches.
   Return 0 if some matches were found, else 1. */
int grep_from_str (char *regexp, char *str, char *where, unsigned int page)
{
	regex_t re;
	regmatch_t match;
	int ret = 1;
	char *beg, *end;

	if (regcomp (&re, regexp, REG_EXTENDED))
	{
		perror ("regcomp");
		return 1;
	}

	while (! regexec (&re, str, 1, &match, 0))
	{
		ret = 0;	/* Found match. */
		if (! where)
			break;

		beg = str + match.rm_so;
		end = str +  match.rm_eo;

		/* try to find line beginning and end. */
		while (beg > str && beg[-1] != '\n')
			beg--;
		while (*end && *end != '\n')
			end++;

		printf ("%s:%d: %.*s\n", where, page, end - beg, beg);
		str = end;
	}

	regfree (&re);
	return ret;
}
