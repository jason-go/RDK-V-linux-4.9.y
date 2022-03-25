#ifndef __SIRIUS_M2M_REG_H__
#define __SIRIUS_M2M_REG_H__

typedef union {
	uint32_t value;
	struct {
		unsigned little_endian    : 1;
		unsigned key_clr          : 1;
		unsigned                  : 6;
		unsigned channel_en       : 8;
		unsigned ts_mode          : 8;
		unsigned fixed_brst       : 1;
		unsigned mixed_brst       : 1;
		unsigned addr_align_brst  : 1;
	} bits;
} rM2M_CTRL;

typedef union {
	uint32_t value;
	struct {
		unsigned addr : 4;
	} bits;
} rM2M_RK;

typedef union {
	uint32_t value;
	struct {
		unsigned index : 3;
	} bits;
} rM2M_IV;

typedef union {
	uint32_t value;
	struct {
		unsigned key_update      : 1;
		unsigned two_key_on      : 1;
		unsigned one_key_odd     : 1;
		unsigned key_switch_clr  : 1;
		unsigned                 : 4;
		unsigned key1_sel        : 5;
		unsigned                 : 3;
		unsigned key0_sel        : 5;
		unsigned                 : 3;
		unsigned opt_mode        : 3;
		unsigned alg             : 4;
		unsigned enc             : 1;
	} bits;
} rM2M_ENC_CTRL;

typedef union {
	uint32_t value;
	struct {
		unsigned src_header      : 8;
		unsigned dst_header      : 8;
	} bits;
} rM2M_HEADER_UPD;

typedef union {
	uint32_t value;
	struct {
		unsigned src_desc        : 8;
		unsigned dst_desc        : 8;
	} bits;
} rM2M_DESC_UPD;

typedef union {
	uint32_t value;
	struct {
		unsigned status          : 8;
	} bits;
} rM2M_STATUS;

typedef union {
	uint32_t value;
	struct {
		unsigned dma_done        : 1;
		unsigned sync_byte_error : 1;
		unsigned tdes_same_key   : 1;
		unsigned almost_empty    : 1;
		unsigned almost_full     : 1;
		unsigned space_err       : 1;
	} bits;
} rM2M_INT;

typedef struct {
	uint32_t      src_chain_ptr;
	uint32_t      dst_chain_ptr;
	uint32_t      weight;
	rM2M_ENC_CTRL ctrl;
	uint32_t      src_update_size;
	uint32_t      dst_update_size;
	uint32_t      src_remainder;
	uint32_t      dst_remainder;
	uint32_t      data_len;
	uint32_t      reserved[55];
} rM2M_CHANNEL;

typedef struct {
	rM2M_CTRL            ctrl;
	uint32_t             reserved_0[63];

	rM2M_RK              rk;                       // 0x100
	uint32_t             reserved_1[19];

	uint32_t             soft_key[8];
	uint32_t             soft_key_update;
	uint32_t             reserved_rk[3];
	uint32_t             record_key[4];
	uint32_t             record_key_update;
	uint32_t             reserved_2[27];

	rM2M_IV              iv_mask;                   // 0x200
	uint32_t             iv[4];
	uint32_t             iv_update;
	uint32_t             reserved_3[890];

	rM2M_CHANNEL         channel[8];               //0x1000
	uint32_t             reserved_4[512];

	rM2M_HEADER_UPD      header_update;            // 0x2000
	rM2M_DESC_UPD        desc_update;
	uint32_t             reserved_5[62];

	uint32_t             key_switch_threshold;     // 0x2100
	uint32_t             sync_byte_err_threshold;
	uint32_t             almost_empty_threshold;
	uint32_t             almost_full_threshold;
	uint32_t             reserved_6[60];

	rM2M_STATUS          space_err;                // 0x2200
	rM2M_STATUS          almost_full;
	rM2M_STATUS          almost_empty;
	rM2M_STATUS          key_err;
	rM2M_STATUS          sync_err;
	rM2M_STATUS          dma_done;
	uint32_t             reserved_7[58];

	rM2M_INT             m2m_int_en;               // 0x2300
	rM2M_INT             m2m_int;
} SiriusM2MReg;

#endif
