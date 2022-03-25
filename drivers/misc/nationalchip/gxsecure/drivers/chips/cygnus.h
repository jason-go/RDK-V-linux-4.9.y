#ifndef __GXSE_CYGNUS_H__
#define __GXSE_CYGNUS_H__
#include "gxse_core.h"

int gxse_device_register_cygnus_klm_scpu(void);
int gxse_device_register_cygnus_misc_otp(void);
int gxse_device_register_cygnus_misc_rng(void);
int gxse_device_register_cygnus_misc_chip(void);
int gxse_device_register_cygnus_misc_firewall(void);
int gxse_device_register_cygnus_misc_sci(void);
int gxse_device_register_cygnus_crypto_dma(void);
int gxse_device_register_cygnus_crypto_fifo(void);
int gxse_device_register_cygnus_secure_generic(void);

int gxse_device_register_cygnus_klm(void);
int gxse_device_register_cygnus_misc(void);
int gxse_device_register_cygnus_crypto(void);
int gxse_device_register_cygnus_secure(void);

int gxse_chip_register_cygnus(void);
#endif
