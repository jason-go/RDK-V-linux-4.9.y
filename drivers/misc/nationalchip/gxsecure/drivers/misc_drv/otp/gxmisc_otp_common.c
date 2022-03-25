#include "gxse_core.h"
#include "../gxmisc_virdev.h"

int32_t gx_misc_otp_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModuleHwObjMiscOps *ops = (GxSeModuleHwObjMiscOps *)obj->ops;
	(void) size;

	switch (cmd) {
	case SECURE_OTP_WRITE:
		{
			GxSecureOtpBuf *otp = (GxSecureOtpBuf *)param;
			if (ops->misc_otp.otp_write)
				ret = ops->misc_otp.otp_write(obj, otp->addr, otp->buf, otp->size);
		}
		break;

	case SECURE_OTP_READ:
		{
			GxSecureOtpBuf *otp = (GxSecureOtpBuf *)param;
			if (ops->misc_otp.otp_read)
				ret = ops->misc_otp.otp_read(obj, otp->addr, otp->buf, otp->size);
		}
		break;

	default:
		break;
	}

	return ret;
}
