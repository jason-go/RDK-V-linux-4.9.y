#include "kernelcalls.h"
#include "gxav_common.h"

static struct av_profile {
	int as_type;
}_av_profile = {0};


#ifndef LINUX_OS
static unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base)
{
	unsigned long result = 0,value;

	if (*cp == '0') {
		cp++;
		if ((*cp == 'x') && isxdigit(cp[1])) {
			base = 16;
			cp++;
		}
		if (!base) {
			base = 8;
		}
	}
	if (!base) {
		base = 10;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
					? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}
#endif
#ifdef UCOS_OS
static char *strsep(char **stringp, const char *delim)
{
	char *s;
	const char *spanp;
	int c, sc;
	char *tok;
	if ((s = *stringp)== NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc =*spanp++) == c) {
				if (c == 0)
					s = NULL;
				else
					s[-1] = 0;
				*stringp = s;
				return (tok);
			}
		} while (sc != 0);
	}
	/* NOTREACHED */
}
#endif

static int av_profile_parse(char* profile, struct av_profile* av_profile)
{
	char *p = profile;
	char *field =NULL;

	if (p == NULL){
		gx_printf("av_profile = NULL\n");
		return 0;
	}

	gx_printf("av_profile = %s\n", profile);

	if(!(field = strsep(&p,"|")))
		goto syntax_error;

	if(!(field = strsep(&p,":")))
		goto syntax_error;
	av_profile->as_type = simple_strtoul(field, NULL, 0);
	gx_printf("av_profile.as_type = %d\n", av_profile->as_type);

	return 0;

syntax_error:
	return -1;
}

int gxav_profile_init(char* profile)
{
	return av_profile_parse(profile, &_av_profile);
}

