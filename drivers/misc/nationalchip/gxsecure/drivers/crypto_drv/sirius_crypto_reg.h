#ifndef __SIRIUS_CRYPTO_REG_H__
#define __SIRIUS_CRYPTO_REG_H__

/*************************** Sirius crypto reg ***************************/
#if defined (CPU_SCPU)
typedef union {
	unsigned int value;
	struct {
		unsigned start       : 1;
		unsigned sob         : 1;
		unsigned             : 2;
		unsigned di_update   : 1;
		unsigned do_update   : 1;
		unsigned             : 10;
		unsigned key_sel     : 2;
		unsigned             : 2;
		unsigned CSGK2_clr   : 1;  // for sirius
		unsigned SIK_clr     : 1;  // for sirius
		unsigned             : 2;
		unsigned opt_mode    : 1;
		unsigned             : 2;
		unsigned cipher      : 1;
	} bits;
} rCEM_BOOT_CTRL;

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
		unsigned key_sel     : 4;
		unsigned key_clr     : 1;
		unsigned cw_half     : 1;
		unsigned cw_sel      : 1;
		unsigned cw_ready    : 1;
		unsigned             : 4;
		unsigned cipher      : 2;
		unsigned             : 1;
		unsigned xor_en      : 1;
	} bits;
} rCEM_DYNAMIC_CTRL;

#else
typedef union {
	unsigned int value;
	struct {
		unsigned start       : 1;
		unsigned             : 3;
		unsigned di_update   : 1;
		unsigned do_update   : 1;
		unsigned             : 2;
		unsigned rslt_des    : 2;
		unsigned             : 2;
		unsigned sob         : 1;
		unsigned             : 3;
		unsigned key_sel     : 4;
		unsigned reg_clr     : 1;
		unsigned ccck_clr    : 1;
		unsigned ik_clr      : 1;
		unsigned bck_clr     : 1;
		unsigned bcm_enc_en  : 1;
		unsigned im_enc_en   : 1;
		unsigned otp_key_en  : 1;
		unsigned             : 1;
		unsigned opt_mode    : 1;
		unsigned cipher      : 1;
		unsigned             : 1;
		unsigned enc         : 1;
	} bits;
} rCEM_CTRL;
#endif

/*************************** common crypto reg ***************************/

typedef union {
	unsigned int value;
	struct {
		unsigned status_busy : 1;
	} bits;
} rCEM_STATUS;

typedef union {
	unsigned int value;
	struct {
		unsigned crypto_done    :1;
		unsigned crypto_key_err :1;
	} bits;
} rCEM_INT;

typedef struct {
#if defined (CPU_SCPU)
	unsigned int         data_in[4];
	unsigned int         data_out[4];
	unsigned int         reserved_0[56];

	rCEM_BOOT_CTRL       ctrl;                     // 0x100
	rCEM_DYNAMIC_CTRL    dctrl;
	rCEM_STATUS          status;
	unsigned int         reserved_1[61];

	unsigned int         IV[4];                    // 0x200
	unsigned int         soft_key[4];
	unsigned int         reserved_2[56];

	unsigned int         data_length;              // 0x300
	unsigned int         reserved_3[63];

	rCEM_INT             crypto_int_en;            // 0x400
	rCEM_INT             crypto_int;

#else
	unsigned int         data_in[4];
	unsigned int         data_out[4];
	unsigned int         reserved_0[76];

	unsigned int         chip_cfg_status[4];       // 0x150
	unsigned int         chip_cfg_mask[4];
	unsigned int         reserved_1[36];

	unsigned int         data_length;              // 0x200
	unsigned int         reserved_2[63];

	rCEM_CTRL            ctrl;                     // 0x300
	rCEM_STATUS          status;
	unsigned int         reserved_3[82];

	unsigned int         IV[4];                    // 0x450
	unsigned int         soft_key[4];
	unsigned int         reserved_4[100];

	rCEM_INT             crypto_int_en;            // 0x600
	rCEM_INT             crypto_int;
#endif
} SiriusCryptoReg;

#endif

