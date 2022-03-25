#ifndef __GX6605S_CHIP_REG_H__
#define __GX6605S_CHIP_REG_H__

#include "sirius_chip_reg.h"

typedef struct {
	unsigned int     reserved_0[0x13C / 4];
	unsigned int     pinmax_port_a;                       //0x13C
	unsigned int     pinmax_port_b;                       //0x140
	unsigned int     reserved_1[(0x190 - 0x140) / 4 -1];
	unsigned int     chipname   [3];                      //0x190
	unsigned int     reserved_2 [(0x560-0x19c)/4];
	unsigned int     CSSN[2];                             //0x560
} Gx6605sChipReg;

#endif
