#ifndef __TAURUS_CHIP_REG_H__
#define __TAURUS_CHIP_REG_H__

#include "sirius_chip_reg.h"

#if defined (CPU_SCPU)
typedef struct {
	rSCPU_CTRL        scpu_ctrl;
	rSCPU_STA         scpu_status;
	rSCPU_SECURE_CTRL secure_ctrl;
	unsigned int      reserved_0;

	unsigned int      biv[4];
	rBIV_STA          biv_status;
	unsigned int      reserved_1[3];

	unsigned int      public_id0;
	unsigned int      public_id1;
	rSECURE_STA       secure_status;
	unsigned int      reserved_2[13];

	unsigned int      secure_key[4];
	rSECURE_KEY_CTRL  key_ctrl;
	unsigned int      reserved_4[3];

	unsigned int      m2m_soft_key[8];
	unsigned int      reserved_5[16];

	rSCPU_STA         key_err_int_en;
	rSCPU_STA         key_err_int;
} TaurusChipReg;

#else

typedef struct {
	unsigned int     reserved_0 [0x190/4];
	unsigned int     chipname   [3];                      //0x190
	unsigned int     reserved_1 [(0x560-0x19c)/4];
	unsigned int     CSSN[2];                             //0x560
} TaurusChipReg;

#endif

#endif
