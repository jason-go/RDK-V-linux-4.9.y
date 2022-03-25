#ifndef _GXAV_M2M_H_
#define _GXAV_M2M_H_

#include "gxav_firmware.h"
#include "gxav_dvr_propertytypes.h"

#ifdef __cplusplus
extern "C"{
#endif

int gxm2m_firmware_copy(unsigned int dst, unsigned int src, unsigned int size, GxAvFirmwareType type);
int gxm2m_decrypt(void *dst, const void *src, int size);
int gxm2m_encrypt(void *dst, const void *src, int size);
int gxm2m_dvr_config(GxDvrProperty_CryptConfig *config);

#ifdef __cplusplus
}
#endif

#endif

