#ifndef __GX_MISC_H__
#define __GX_MISC_H__

typedef enum {
	GXSE_HEVC_SUPPORT_8BIT  = ( 1 << 0),            ///< 支持8bit
	GXSE_HEVC_SUPPORT_10BIT = ( 1 << 1),            ///< 支持10bit
} GxSecureHEVCStatus;

int gxsecure_otp_read(unsigned int addr);
int gxsecure_otp_write(unsigned int addr, unsigned char byte);
int gxsecure_otp_read_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size);
int gxsecure_otp_write_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size);

int gxsecure_otp_get_macrovision_status(void);
int gxsecure_otp_get_hdr_status(void);
int gxsecure_otp_get_hevc_status(void);

#endif

