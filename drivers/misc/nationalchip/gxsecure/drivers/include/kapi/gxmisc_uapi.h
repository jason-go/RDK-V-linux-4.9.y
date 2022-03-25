#ifndef _GX_MISC_UAPI_H_
#define _GX_MISC_UAPI_H_

unsigned int gxsecure_rng_request(void);
unsigned char hw_get_random_byte(void);
int gxsecure_otp_read(unsigned int addr);
int gxsecure_otp_write(unsigned int addr, unsigned char byte);
int gxsecure_otp_read_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size);
int gxsecure_otp_write_buf(unsigned char *buf, unsigned int buf_len, unsigned int addr, unsigned int size);
int gxsecure_otp_get_chipname(unsigned char *buf, unsigned int size);
int gxsecure_otp_get_publicid(unsigned char *buf, unsigned int size);
int gxsecure_otp_get_marketid(unsigned char *buf, unsigned int size);
/*
* 获取macrovision功能的使能状态
* return val
*  0: 未使能
*  1: 使能
*  < 0: 功能出错
*/
int gxsecure_otp_get_macrovision_status(void);

/*
* 获取HDR功能的使能状态
* return val
*  0: 未使能
*  1: 使能
*  < 0: 功能出错
*/
int gxsecure_otp_get_hdr_status(void);
/*
* 获取HEVC支持的状态
* return val
*  >=0: 获取状态成功,例如 status = HEVC_SUPPORT_8BIT | HEVC_SUPPORT_10BIT
*  < 0: 获取状态失败
*/
int gxsecure_otp_get_hevc_status(void);
unsigned int gx_ta_marketid(void);
int gx_ta_decrypt(unsigned char *src, unsigned char*dst, unsigned int len);

#endif
