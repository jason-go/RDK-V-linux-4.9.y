#ifndef __SIRIUS_MISC_CHIP_H__
#define __SIRIUS_MISC_CHIP_H__

#ifdef CPU_ACPU
int32_t sirius_misc_get_CSSN(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
int32_t sirius_misc_get_chipname(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);

#else
int32_t sirius_misc_chip_init(GxSeModuleHwObj *obj);
int32_t sirius_misc_scpu_reset(GxSeModuleHwObj *obj);
int32_t sirius_misc_get_BIV(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
int32_t sirius_misc_send_m2m_soft_key(GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
int32_t sirius_misc_send_secure_key(GxSeModuleHwObj *obj, GxSeScpuKeyType type, uint8_t *buf, uint32_t size);
#endif

#endif
