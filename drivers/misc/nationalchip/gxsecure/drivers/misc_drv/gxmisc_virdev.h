#ifndef __GXMISC_VIRDEV_H__
#define __GXMISC_VIRDEV_H__

#include "gxse_core.h"
#include "gxse_hwobj_misc.h"

extern GxSeModuleOps misc_dev_ops;

int32_t gx_misc_fw_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size);
int32_t gx_misc_sci_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size);
int32_t gx_misc_otp_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size);
int32_t gx_misc_rng_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size);
int32_t gx_misc_chip_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size);
int32_t gx_misc_timer_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size);
int32_t gx_misc_sensor_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size);

#endif
