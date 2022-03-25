#ifndef __TAURUS_MTC_REG__
#define __TAURUS_MTC_REG__

typedef union {
	unsigned int value;
	struct {
		unsigned tri_des             : 1;
		unsigned encrypt             : 1;
		unsigned key_rdy             : 1;
		unsigned aes_mode            : 2;
		unsigned work_mode           : 2;
		unsigned opt                 : 3;
		unsigned M                   : 6;
		unsigned big_endian          : 1;
		unsigned gx3211_key_sel      : 3;
		unsigned des_key_low         : 1;
		unsigned residue             : 2;
		unsigned short_msg           : 2;
		unsigned des_key_multi_low   : 1;
		unsigned memory_key_read_en  : 1;
		unsigned memory_key_low      : 1;
		unsigned des_iv_sel          : 1;
		unsigned flash_encrypt       : 1;
	} bits;
} rMTC_CTRL1;

typedef union {
	unsigned int value;
	struct {
		unsigned key_sel           : 4;
	} bits;
} rMTC_KEY_SEL;

typedef struct {
	unsigned int low;
	unsigned int high;
} rMTC_KEY;

typedef union {
	unsigned int value;
	struct {
		unsigned des_ready           : 1;
		unsigned aes_ready           : 1;
		unsigned multi2_ready        : 1;
		unsigned sms4_ready          : 1;
		unsigned nds_ready           : 1;
		unsigned memory_ready        : 1;
	} bits;
} rMTC_CTRL2;

typedef union {
	unsigned int value;
	struct {
		unsigned des_rst           : 1;
		unsigned aes_rst           : 1;
		unsigned sms4_rst          : 1;
		unsigned multi2_rst        : 1;
		unsigned memory_rst        : 1;
		unsigned ctr_rst           : 1;
		unsigned sdc_rst           : 1;
	} bits;
} rMTC_RST;

typedef union {
	unsigned int value;
	struct {
		unsigned write_busy        : 1;
		unsigned read_busy         : 1;
	} bits;
} rMTC_FLASH_SIG;

typedef union {
	unsigned int value;
	struct {
		unsigned data_finish            : 1;
		unsigned k3_finish              : 1;
		unsigned aes_round_finish       : 1;
		unsigned tdes_round_finish      : 1;
		unsigned err_operate_nds        : 1;
		unsigned m2m_operate_illegal    : 1;
		unsigned memory_read_finish     : 1;
		unsigned otp_flash_finish : 1;
	} bits;
} rMTC_INT;

typedef struct {
	rMTC_CTRL1      ctrl1;                   // 0x00
	rMTC_KEY        key0_2[3];               // 0x04
	unsigned int    m2mr_addr;               // 0x1c
	unsigned int    m2mw_addr;               // 0x20
	unsigned int    data_len;                // 0x24
	rMTC_CTRL2      ctrl2;                   // 0x28
	rMTC_KEY        key3;                    // 0x2c
	unsigned int    counter[4];              // 0x34
	unsigned int    iv1[4];                  // 0x44
	unsigned int    reserved_0[3];           // 0x54
	rMTC_RST        rst_ctrl;                // 0x60
	rMTC_KEY_SEL    key_sel;                 // 0x64
	unsigned int    reserved_7[(0xc0-0x64)/4-1];// 0x68~0xc0
	rMTC_FLASH_SIG  flash_sig;               // 0xc0
	unsigned int    flash_write_data;        // 0xc4
	unsigned int    flash_read_data;         // 0xc8
	unsigned int    reserved_2[37];          // 0xcc~0x160
	rMTC_INT        isr_en;                  // 0x160
	rMTC_INT        isr;                     // 0x164
	unsigned int    reserved_4[6];           // 0x168~0x180
	unsigned int    iv2[4];                  // 0x180
} TaurusM2MReg;

#endif
