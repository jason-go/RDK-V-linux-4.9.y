#ifndef __GX3211_MTC_REG__
#define __GX3211_MTC_REG__

#include "taurus_m2m_reg.h"

typedef union {
	unsigned int value;
	struct {
		unsigned encrypt      : 1;
		unsigned decrypt      : 1;
		unsigned              : 6;
		unsigned alg          : 4;
		unsigned opt          : 4;
		unsigned residue      : 4;
		unsigned short_msg    : 4;
	} bits;
} rMTC_NDS_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned                  : 2;
		unsigned key_update_ready : 1;
		unsigned key_sel_enable   : 1;
		unsigned key_sel          : 3;
		unsigned key_write_done   : 1;
		unsigned ctrl_write_en    : 1;
		unsigned ctrl_write_sel   : 3;
	} bits;
} rMTC_NDS_GEN;

typedef union {
	unsigned int value;
	struct {
		unsigned cw_en            : 1;
		unsigned cw_round_1       : 1;
		unsigned cw_round_2       : 1;
		unsigned cw_round_3       : 1;
		unsigned cw_round_3_type  : 2;
		unsigned cw_round_3_key   : 2;
		unsigned cw_aes_HL_sel    : 1;
		unsigned multi_key_en     : 1;
		unsigned cw_round_0       : 1;
		unsigned cw_round_0_valid : 1;
		unsigned cw_round_4       : 1;
		unsigned cw_round_5       : 1;
	} bits;
} rMTC_CW_SEL;

typedef union {
	unsigned int value;
	struct {
		unsigned enable           : 1;
		unsigned DSK_ready        : 1;
		unsigned DCK_ready        : 1;
		unsigned DCW_ready        : 1;
		unsigned ca_key_length    : 1;
		unsigned otp_key_length   : 1;
		unsigned VCW_ready        : 1;
		unsigned MCW_ready        : 1;
	} bits;
} rMTC_CA_MODE;

typedef struct {
	rMTC_CTRL1      ctrl1;                      // 0x00
	rMTC_KEY        key0_2[3];                  // 0x04
	unsigned int    m2mr_addr;                  // 0x1c
	unsigned int    m2mw_addr;                  // 0x20
	unsigned int    data_len;                   // 0x24
	rMTC_CTRL2      ctrl2;                      // 0x28
	rMTC_KEY        key3;                       // 0x2c
	unsigned int    counter[4];                 // 0x34
	unsigned int    iv1[4];                     // 0x44
	unsigned int    reserved_0[3];              // 0x54
	rMTC_RST        rst_ctrl;                   // 0x60
	rMTC_KEY_SEL    key_sel;                    // 0x64
	unsigned int    reserved_1[(0x9c-0x64)/4-1];// 0x68~0x9c
	rMTC_NDS_CTRL   nds_ctrl;                   // 0x9c
	rMTC_NDS_GEN    nds_gen;                    // 0xa0
	unsigned int    nds_key_write_addr;         // 0xa4
	unsigned int    nds_key_full;               // 0xa8
	unsigned int    reserved_2[(0x160-0xa8)/4-1];// 0xac~0x160
	rMTC_INT        isr_en;                     // 0x160
	rMTC_INT        isr;                        // 0x164
	unsigned int    reserved_4[6];              // 0x168~0x180
	unsigned int    iv2[4];                     // 0x180
	unsigned int    reserved_6[28];             // 0x190~0x200
} Gx3211M2MReg;

#endif
