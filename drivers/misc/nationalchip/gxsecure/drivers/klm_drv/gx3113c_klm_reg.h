#ifndef __GX3113C_AKLM_REG_H__
#define __GX3113C_AKLM_REG_H__
#include "gx3211_klm_reg.h"

typedef struct {
	rMTC_CTRL1      ctrl1;                        // 0x00
	unsigned int    reserved11[(0x1c-0x00)/4-1];  // 0x04~0x1c
	unsigned int    m2mr_addr;                    // 0x1c
	unsigned int    m2mw_addr;                    // 0x20
	unsigned int    data_len;                     // 0x24
	rMTC_CTRL2      ctrl2;                        // 0x28
	unsigned int    reserved22[(0x34-0x28)/4-1];  // 0x2c~34
	unsigned int    counter[4];                   // 0x34
	unsigned int    iv1[4];                       // 0x44
	unsigned int    reserved_0[2];                // 0x54
	rMTC_CA_MODE    ca_mode;                      // 0x5c

	unsigned int    ca_DCK[6];                    // 0x60
	unsigned int    ca_DCW[4];                    // 0x78
	unsigned int    reserved_1[2];                // 0x88~0x90
	unsigned int    ca_addr;                      // 0x90
	unsigned int    reserved_2[(0x100-0x90)/4-1]; // 0x94~0x100
	unsigned int    ca_DSK[6];                    // 0x100
	unsigned int    reserved_4[2];                // 0x118~0x120
	rMTC_CW_SEL     ca_cw_select;                 // 0x120
	unsigned int    ca_nonce[4];                  // 0x124
	unsigned int    ca_DA[4];                     // 0x134
	unsigned int    reserved_5[3];                // 0x144~0x150
	unsigned int    ca_k3_gen;                    // 0x150
	unsigned int    reserved_6[3];                // 0x154~0x160
	rMTC_INT        isr_en;                       // 0x160
	rMTC_INT        isr;                          // 0x164
	unsigned int    reserved_7[6];                // 0x168~0x180
	unsigned int    iv2[4];                       // 0x180
} Gx3113CKLMReg;

#endif
