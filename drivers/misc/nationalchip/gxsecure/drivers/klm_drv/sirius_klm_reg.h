#ifndef __SIRIUS_AKLM_REG_H__
#define __SIRIUS_AKLM_REG_H__

typedef union {
	unsigned int value;
	struct {
		unsigned mode                : 2;
		unsigned alg                 : 2;
		unsigned dst_cw_half         : 1;
		unsigned dst                 : 1;
		unsigned pvr_stage           : 1;
		unsigned cw_stage            : 1;
		unsigned tdes_klm_half_clear : 1;
	} bits;
} rIKLM_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned tdc_busy            : 1;
		unsigned tdc_write_en        : 1;
		unsigned big_endian          : 1;
		unsigned tdc_size            : 5;
	} bits;
} rTDC_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned big_endian : 1;
	} bits;
} rCIPHER_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned mode                : 2;
		unsigned alg                 : 2;
		unsigned dst_cw_half         : 1;
		unsigned dst                 : 1;
		unsigned aes_cw_high         : 1;
		unsigned tdes_klm_half_clear : 1;
		unsigned cw_stage            : 4;
		unsigned                     : 4;
		unsigned pvr_stage           : 4;
	} bits;
} rGKLM_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned cw1     : 1;
		unsigned cw2     : 1;
		unsigned rk1     : 1;
		unsigned rk2     : 1;
	} bits;
} rGKLM_STATE;

typedef union {
	unsigned int value;
	struct {
		unsigned klm_finish           : 1;
		unsigned tdes_half_klm_finish : 1;
		unsigned tdes_klm_finish      : 1;
		unsigned rcv_key_err          : 1;
		unsigned cw_gen_key_err       : 1;
		unsigned tdes_key_err         : 1;
		unsigned tdc_hash_err         : 1;
		unsigned tdc_cmd_size_err     : 1;

		unsigned gklm_finish          : 1;
		unsigned gklm_half_finish     : 1;
		unsigned gklm_tdes_finish     : 1;
		unsigned gklm_rcv_key_err     : 1;
		unsigned gklm_tdes_key_err    : 1;
	} bits;
} rKLM_INT;

typedef struct {
	unsigned int         start;
	rIKLM_CTRL           ctrl;
	rTDC_CTRL            tdc_ctrl;
	rCIPHER_CTRL         cipher_ctrl;
	unsigned int         reserved_0[60];

	unsigned int         EK1_AK[4];
	unsigned int         EK2_SK[4];
	unsigned int         EK3_CW[4];
	unsigned int         tdc_in[4];
	unsigned int         tdc_iv[4];
	unsigned int         reserved_1[940];

	unsigned int         gklm_start;
	rGKLM_CTRL           gklm_ctrl;
	rGKLM_STATE          gklm_stage_sel;
	unsigned int         reserved_2[61];

	unsigned int         gklm_EK[8][4];
	unsigned int         reserved_3[928];

	rKLM_INT            klm_int_en;
	rKLM_INT            klm_int;
} SiriusKLMReg;

#endif
