#ifndef __GXSE_TAURUS_H__
#define __GXSE_TAURUS_H__
#include "gxse_core.h"

int gxse_device_register_taurus_klm_scpu(void);
int gxse_device_register_taurus_misc_otp(void);
int gxse_device_register_taurus_misc_rng(void);
int gxse_device_register_taurus_misc_chip(void);
int gxse_device_register_taurus_misc_firewall(void);
int gxse_device_register_taurus_misc_sci(void);
int gxse_device_register_taurus_crypto_dma(void);
int gxse_device_register_taurus_crypto_fifo(void);
int gxse_device_register_taurus_secure_generic(void);

int gxse_device_register_taurus_klm(void);
int gxse_device_register_taurus_misc(void);
int gxse_device_register_taurus_crypto(void);
int gxse_device_register_taurus_secure(void);

int gxse_chip_register_taurus(void);
#endif
