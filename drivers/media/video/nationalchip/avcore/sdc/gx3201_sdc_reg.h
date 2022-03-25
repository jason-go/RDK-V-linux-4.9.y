#ifndef __GX3201_SDC_REG__
#define __GX3201_SDC_REG__

#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gxav_common.h"

#define GXAV_SDC_MAX		  (16)

#define CHECK_CHANNEL(channel)  if((channel < 0) ||(channel >= GXAV_SDC_MAX)) return -1;

struct sdc_regs {
	unsigned int rBUFn_PEAK_VALUE[GXAV_SDC_MAX];
	unsigned int rBUFn_COUNTER[GXAV_SDC_MAX];
	unsigned int rBUFn_CAP[GXAV_SDC_MAX];
	unsigned int rBUFn_EMPTY_GATE[GXAV_SDC_MAX];
	unsigned int rBUFn_FULL_GATE[GXAV_SDC_MAX];
	unsigned int rBUFn_ALMOST_EMPTY_GATE[GXAV_SDC_MAX];
	unsigned int rBUFn_ALMOST_FULL_GATE[GXAV_SDC_MAX];

	unsigned int rBUFn_RDADDR[GXAV_SDC_MAX];
	unsigned int rBUFn_WRADDR[GXAV_SDC_MAX];

	unsigned int rBUFn_STATUS[GXAV_SDC_MAX];
	gx_spin_lock_t spin_lock[GXAV_SDC_MAX];
};


#endif
