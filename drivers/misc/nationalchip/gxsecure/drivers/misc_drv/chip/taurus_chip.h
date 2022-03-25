#ifndef __TAURUS_MISC_CHIP_H__
#define __TAURUS_MISC_CHIP_H__

#ifdef CPU_ACPU
int32_t taurus_misc_get_CSSN(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
int32_t taurus_misc_get_chipname(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
#endif

#endif
