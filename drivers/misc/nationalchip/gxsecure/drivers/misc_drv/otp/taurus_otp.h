#ifndef __TAURUS_OTP_H__
#define __TAURUS_OTP_H__

int32_t taurus_misc_otp_read(GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size);
int32_t taurus_misc_otp_write(GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size);

#endif
