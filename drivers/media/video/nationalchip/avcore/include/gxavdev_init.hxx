#ifndef	GXAVDEV_INIT_H
#define	GXAVDEV_INIT_H

// NDS: "|3"

#ifdef ECOS_OS
#include <cyg/infra/cyg_type.h>
extern "C" int gxav_init(char* profile);
class cyg_av_init_class {
public:
   cyg_av_init_class(const char* profile = NULL)
	{
		gxav_init((char*)profile);
	}
};
#else
extern int gxav_init(char* profile);
#endif

#endif
