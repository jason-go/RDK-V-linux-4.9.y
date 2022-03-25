#ifndef __AV_SECURE_HAL_H__
#define __AV_SECURE_HAL_H__

#include "gxav_m2m.h"
#include "firewall.h"
#include "external/gxm2m.h"
#include "external/gxfirewall.h"
#include "external/gxmisc.h"

typedef enum {
	GXAV_HEVC_SUPPORT_8BIT  = GXSE_HEVC_SUPPORT_8BIT  ,
	GXAV_HEVC_SUPPORT_10BIT = GXSE_HEVC_SUPPORT_10BIT ,
} GxAVHEVCStatus;

int gxav_secure_encrypt(GxTfmCrypto *param);
int gxav_secure_decrypt(GxTfmCrypto *param);
int gxav_secure_otp_read(unsigned int addr);
int gxav_secure_otp_write(unsigned int addr, unsigned char byte);
int gxav_secure_otp_read_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size);
int gxav_secure_otp_write_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size);
int gxav_secure_get_macrovision_status(void);
int gxav_secure_get_hdr_status(void);
int gxav_secure_get_hevc_status(void);
int gxav_secure_config_filter(int addr, int size, int master_rd_permission, int master_wr_permission);
int gxav_secure_get_protect_buffer(void);
int gxav_secure_query_access_align(void);
int gxav_secure_query_protect_align(void);

int gxav_secure_set_hdcp_state(int state);
int gxav_secure_get_hdcp_state(void);

int gxav_secure_register_read(unsigned int base, unsigned int offset);
int gxav_secure_register_write(unsigned int base, unsigned int offset, unsigned value);

#endif
