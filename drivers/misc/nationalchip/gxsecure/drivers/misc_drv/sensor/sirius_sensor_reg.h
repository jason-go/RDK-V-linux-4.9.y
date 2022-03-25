#ifndef __SENSOR_REG_H__
#define __SENSOR_REG_H__

typedef union {
	unsigned int value;
	struct {
		unsigned acpu_otp_master_key_err  : 1;
		unsigned acpu_kds_key_err         : 1;
		unsigned acpu_klm_key_err         : 1;
		unsigned acpu_dec_key_err         : 1;
		unsigned acpu_m2m_key_err         : 1;
		unsigned demux_cw_crc_key_err     : 1;
		unsigned gp_cw_crc_err            : 1;
		unsigned acpu_otp_general_key_err : 1;
		unsigned jtag_password_err        : 1;
		unsigned ifcp_klm_key_err         : 1;
		unsigned                          : 1;
		unsigned ifcp_scpu_dec_key_err    : 1;
	} bits;
} rSENSOR_KEY_RECV_ERR;

typedef union {
	unsigned int value;
	struct {
		unsigned otp_flag_err          : 1;
	} bits;
} rSENSOR_OTP_FLAG_ERR;

typedef union {
	unsigned int value;
	struct {
		unsigned sensor_softer_mode     : 3;
	} bits;
} rSENSOR_SOFTER_MODE;

typedef struct {
	unsigned int          s3_alarm_int[3];
	unsigned int          reserved1[5];
	unsigned int          s3_alarm_int_en[3];
	unsigned int          reserved2[5];

	unsigned int          s22_alarm_int[3];
	unsigned int          reserved3[5];
	unsigned int          s22_alarm_int_en[3];
	unsigned int          reserved4[5];

	unsigned int          s21_alarm_int[4];
	unsigned int          reserved5[12];
	unsigned int          s21_alarm_int_en[4];
	unsigned int          reserved6[12];

	unsigned int          s3_health_int[3];
	unsigned int          reserved7[5];
	unsigned int          s3_health_int_en[3];
	unsigned int          reserved8[5];

	unsigned int          s22_health_int[3];
	unsigned int          reserved9[5];
	unsigned int          s22_health_int_en[3];
	unsigned int          reserved10[5];

	unsigned int          s21_health_int[4];
	unsigned int          reserved11[4];
	unsigned int          s21_health_int_en[4];
	unsigned int          reserved12[20];

	rSENSOR_KEY_RECV_ERR  key_recv_err;
	unsigned int          reserved13[3];
	rSENSOR_KEY_RECV_ERR  key_recv_err_en;
	unsigned int          reserved14[3];

	rSENSOR_OTP_FLAG_ERR  otp_flag_err;
	unsigned int          reserved15[3];
	rSENSOR_OTP_FLAG_ERR  otp_flag_err_en;
	unsigned int          reserved16[51];

	unsigned int          err_counter;
	unsigned int          err_counter_clear;
	unsigned int          reserved17[2];
	rSENSOR_SOFTER_MODE   sensor_softer_mode;
} SiriusSensorReg;

typedef union {
	unsigned int value;
	struct {
		unsigned revision       :6;
		unsigned project        :13;
		unsigned ip_type        :13;
	} bits;
} rSENSOR_VERSION;

typedef struct {
	rSENSOR_VERSION version;
	unsigned int    sensor_alarm;
	unsigned int    health_test_alarm;
	unsigned int    health_test;
	unsigned int    alarm_enable;
} SiriusSensorCtlReg;


#endif
