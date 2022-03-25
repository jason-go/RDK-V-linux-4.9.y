#ifndef __GXSE_GX3113C_H__
#define __GXSE_GX3113C_H__
#include "gxse_core.h"

int gxse_device_register_gx3113c_misc_firewall(void);
int gxse_device_register_gx3113c_misc_sci(void);
int gxse_device_register_gx3113c_misc_otp(void);
int gxse_device_register_gx3113c_misc_rng(void);
int gxse_device_register_gx3113c_misc_chip(void);
int gxse_device_register_gx3113c_klm_generic(void);

int gxse_device_register_gx3113c_klm(void);
int gxse_device_register_gx3113c_misc(void);
int gxse_device_register_gx3113c_crypto(void);

int gxse_chip_register_gx3113c(void);
#endif
