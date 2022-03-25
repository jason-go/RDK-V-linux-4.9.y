#ifndef __TAURUS_OTP_REG_H__
#define __TAURUS_OTP_REG_H__

#include "sirius_otp_reg.h"

#if defined (CPU_SCPU)
typedef union {
	unsigned int value;
	struct {
		unsigned         : 3;
		unsigned address : 11;
		unsigned rd      : 1;
		unsigned wr      : 1;
	} bits;
} rTAURUS_OTP_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned           : 8;
		unsigned busy      : 1;
		unsigned rd_valid  : 1;
		unsigned ready     : 1;
		unsigned           : 1;
		unsigned rw_fail   : 1;
	} bits;
} rTAURUS_OTP_STATUS;

typedef struct {
	unsigned int       reserved[20];
	rTAURUS_OTP_CTRL   otp_ctrl;
	rTAURUS_OTP_STATUS otp_status;
	unsigned int       otp_write_data;
	unsigned int       otp_read_data;
	unsigned int       otp_flag[4];
} TaurusOTPReg;

#else
typedef union {
    unsigned int value;
    struct {
        unsigned             : 3;
        unsigned address     : 11;
        unsigned rd          : 1;
        unsigned wr          : 1;
        unsigned wr_data     : 8;
    } bits;
} rTAURUS_OTP_CTRL;

typedef union {
    unsigned int value;
    struct {
        unsigned rd_data     : 8;
        unsigned busy        : 1;
        unsigned rd_valid    : 1;
        unsigned ready       : 1;
        unsigned             : 1;
        unsigned rw_fail     : 1;
    } bits;
} rTAURUS_OTP_STATUS;

typedef union {
    unsigned int value;
    struct {
        unsigned enable      : 1;
        unsigned update      : 1;
    } bits;
} rOTP_OSC_CLK;

typedef union {
    unsigned int value;
    struct {
        unsigned enable      : 1;
    } bits;
} rOTP_DELAY_SW;

typedef union {
    unsigned int value;
    struct {
        unsigned sw0         : 12;
        unsigned             : 4;
        unsigned sw1         : 12;
    } bits;
} rOTP_SW_CFG;

typedef union {
    unsigned int value;
    struct {
        unsigned flag        : 2;
    } bits;
} rOTP_BOOT_FLAG;

typedef struct {
	unsigned int     reserved_0[32];

	rTAURUS_OTP_CTRL otp_ctrl;
	unsigned int     otp_cfg;
	rTAURUS_OTP_STATUS otp_status;
	unsigned int     otp_cfg_addr[4];
	rOTP_OSC_CLK     otp_osc_clk_ctrl;
	unsigned int     otp_osc_clk_staus;
	unsigned int     reserved_5[7];

	unsigned int     clk_detect_ctrl;
	unsigned int     reserved_6[3];

	rOTP_DELAY_SW    otp_delay_sw_ctrl;
	rOTP_SW_CFG      otp_tsp_pgm_pgavdd;
	rOTP_SW_CFG      otp_tpaen_tpgm;
	rOTP_SW_CFG      otp_thp_pgavdd_pgm;
	rOTP_SW_CFG      otp_trd_tsr_rd;
	rOTP_SW_CFG      otp_tsq_tpaen;
	rOTP_SW_CFG      otp_thr_rd;
	unsigned int     otp_pll_cfg_sel;
	unsigned int     reserved_7[4];

	rOTP_BOOT_FLAG   otp_uart_boot_disable;
	rOTP_BOOT_FLAG   otp_acpu_secu_boot;
	rOTP_BOOT_FLAG   otp_secu_boot_level;
	rOTP_BOOT_FLAG   otp_secu_boot_in_ca_switch;
	unsigned int     otp_chip_boot_version;
	unsigned int     otp_market_id;
	unsigned int     otp_pub_id[2];
	unsigned int     otp_chip_name[3];
} TaurusOTPReg;
#endif

#endif
