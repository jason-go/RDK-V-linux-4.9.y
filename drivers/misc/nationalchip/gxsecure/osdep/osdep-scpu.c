#include "kernelcalls.h"

unsigned int gx_osdep_chip_probe(void)
{
#ifdef CONFIG_SIRIUS
	return GXSECURE_CHIPID_SIRIUS;
#elif defined CONFIG_TAURUS
	return GXSECURE_CHIPID_TAURUS;
#endif
}

unsigned int gx_osdep_chip_sub_probe(void)
{
	return 0;
}

void gx_osdep_cache_sync(const void *start, unsigned int size, int direction)
{
}

