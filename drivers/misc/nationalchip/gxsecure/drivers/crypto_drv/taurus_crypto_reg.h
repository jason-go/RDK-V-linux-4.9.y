#ifndef __TAURUS_CRYPTO_REG_H__
#define __TAURUS_CRYPTO_REG_H__

#include "sirius_crypto_reg.h"

/*************************** Sirius crypto reg ***************************/
typedef union {
	unsigned int value;
	struct {
		unsigned start       : 1;
		unsigned sob         : 1;
		unsigned             : 2;
		unsigned di_update   : 1;
		unsigned do_update   : 1;
		unsigned di_words    : 1;
		unsigned             : 1;
		unsigned data_src    : 4;
		unsigned rslt_des    : 4;
		unsigned key_sel     : 5;
		unsigned cw_half     : 1;
		unsigned cw_sel      : 1;
		unsigned cw_ready    : 1;
		unsigned key_clr     : 1;
		unsigned             : 3;
		unsigned cipher      : 2;
		unsigned             : 1;
		unsigned xor_en      : 1;
	} bits;
} rCEM_TAURUS_DYNAMIC_CTRL;

/*************************** common crypto reg ***************************/

typedef struct {
	unsigned int         data_in[4];
	unsigned int         data_out[4];
	unsigned int         reserved_0[56];

	rCEM_BOOT_CTRL       ctrl;                     // 0x100
	rCEM_TAURUS_DYNAMIC_CTRL    dctrl;
	rCEM_STATUS          status;
	unsigned int         reserved_1[61];

	unsigned int         IV[4];                    // 0x200
	unsigned int         soft_key[4];
	unsigned int         reserved_2[56];

	unsigned int         data_length;              // 0x300
	unsigned int         reserved_3[63];

	rCEM_INT             crypto_int_en;            // 0x400
	rCEM_INT             crypto_int;
} TaurusCryptoReg;

#endif

