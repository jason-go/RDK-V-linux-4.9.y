#ifndef __GXSE_SIRIUS_H__
#define __GXSE_SIRIUS_H__
#include "gxse_core.h"

int gxse_device_register_sirius_secure_generic(void);
int gxse_device_register_sirius_secure_tee(void);
int gxse_device_register_sirius_klm_generic(void);
int gxse_device_register_sirius_klm_irdeto(void);
int gxse_device_register_sirius_klm_scpu(void);
int gxse_device_register_sirius_crypto_fifo(void);
int gxse_device_register_sirius_crypto_dma(void);
int gxse_device_register_sirius_misc_firewall(void);
int gxse_device_register_sirius_misc_otp(void);
int gxse_device_register_sirius_misc_chip(void);
int gxse_device_register_sirius_misc_rng(void);
int gxse_device_register_sirius_misc_dgst(void);
int gxse_device_register_sirius_misc_akcipher(void);
int gxse_device_register_sirius_misc_sci(void);
int gxse_device_register_sirius_misc(void);
int gxse_device_register_sirius_crypto(void);
int gxse_device_register_sirius_klm(void);
int gxse_device_register_sirius_secure(void);
int gxse_chip_register_sirius(void);

#endif
