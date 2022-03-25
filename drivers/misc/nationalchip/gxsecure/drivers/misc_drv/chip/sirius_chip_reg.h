#ifndef __SIRIUS_MISC_REG_H__
#define __SIRIUS_MISC_REG_H__

#include "sirius_rng_reg.h"

#if defined (CPU_SCPU)
typedef union {
	unsigned int value;
	struct {
		unsigned reset_req : 1;
		unsigned key_clr   : 1;
		unsigned parity_rst_en : 1;
	} bits;
} rSCPU_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned otp_master_key_err   : 1;
		unsigned kds_module_key_err   : 1;
		unsigned acpu_klm_key_err     : 1;
		unsigned acpu_decrypt_key_err : 1;
		unsigned m2m_key_err          : 1;
		unsigned ts_des_key_err       : 1;
		unsigned gp_des_key_err       : 1;
		unsigned otp_general_key_err  : 1;
		unsigned jtag_password_crc_err: 1;

		unsigned scpu_klm_key_err     : 1;
		unsigned                      : 1;
		unsigned scpu_decrypt_key_err : 1;
		unsigned otp_cfg_err          : 1;
	} bits;
} rSCPU_STA;

typedef union {
	unsigned int value;
	struct {
		unsigned rand_en : 1;
	} bits;
} rSCPU_SECURE_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned valid : 1;
	} bits;
} rBIV_STA;

typedef union {
	unsigned int value;
	struct {
		unsigned ifcp_test_switch_flag   : 4;
		unsigned scpu_secure_boot_enable : 4;
	} bits;
} rSECURE_STA;

typedef union {
	unsigned int value;
	struct {
		unsigned start : 1;
		unsigned len   : 1;
		unsigned mode  : 2;
		unsigned done  : 1;
	} bits;
} rSECURE_KEY_CTRL;

typedef struct {
	rSCPU_CTRL        scpu_ctrl;
	rSCPU_STA         scpu_status;
	rSCPU_SECURE_CTRL secure_ctrl;
	unsigned int      reserved_0;

	unsigned int      BIV[4];
	rBIV_STA          BIV_status;
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
} SiriusChipReg;

#else

typedef struct {
	unsigned int     reserved_0 [0x190/4];
	unsigned int     chipname   [3];                  // 0x190
	unsigned int     reserved_1 [(0x330-0x19c)/4];
	unsigned int     CSSN[2];                         // 0x330
} SiriusChipReg;
#endif

#endif
