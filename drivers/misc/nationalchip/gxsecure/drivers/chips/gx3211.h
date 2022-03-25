#ifndef __GXSE_GX3211_H__
#define __GXSE_GX3211_H__
#include "gxse_core.h"

int gxse_device_register_gx3211_misc_otp(void);
int gxse_device_register_gx3211_misc_sci(void);
int gxse_device_register_gx3211_misc_rng(void);
int gxse_device_register_gx3211_misc_chip(void);
int gxse_device_register_gx3211_misc_firewall(void);
int gxse_device_register_gx3211_klm_generic(void);
int gxse_device_register_gx3211_crypto_dma(void);

int gxse_device_register_gx3211_klm(void);
int gxse_device_register_gx3211_misc(void);
int gxse_device_register_gx3211_crypto(void);

int gxse_chip_register_gx3211(void);
#endif
