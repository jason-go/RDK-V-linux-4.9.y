#ifndef __GXSE_PEGASUS_H__
#define __GXSE_PEGASUS_H__
#include "gxse_core.h"

int gxse_device_register_pegasus_klm_generic(void);
int gxse_device_register_pegasus_crypto_dma(void);
int gxse_device_register_pegasus_misc_firewall(void);
int gxse_device_register_pegasus_misc_sci(void);
int gxse_device_register_pegasus_misc_otp(void);
int gxse_device_register_pegasus_misc_chip(void);

int gxse_device_register_pegasus_klm(void);
int gxse_device_register_pegasus_crypto(void);
int gxse_device_register_pegasus_misc(void);

int gxse_chip_register_pegasus(void);
#endif
