#ifndef __GXSE_GX6605S_H__
#define __GXSE_GX6605S_H__
#include "gxse_core.h"

int gxse_device_register_gx6605s_klm_generic(void);
int gxse_device_register_gx6605s_crypto_dma(void);
int gxse_device_register_gx6605s_misc_firewall(void);
int gxse_device_register_gx6605s_misc_sci(void);
int gxse_device_register_gx6605s_misc_otp(void);
int gxse_device_register_gx6605s_misc_chip(void);

int gxse_device_register_gx6605s_klm(void);
int gxse_device_register_gx6605s_crypto(void);
int gxse_device_register_gx6605s_misc(void);

int gxse_chip_register_gx6605s(void);
#endif
