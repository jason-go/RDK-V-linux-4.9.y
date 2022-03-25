#ifndef __GXSE_GEMINI_H__
#define __GXSE_GEMINI_H__
#include "gxse_core.h"

int gxse_device_register_gemini_klm_scpu(void);
int gxse_device_register_gemini_misc_otp(void);
int gxse_device_register_gemini_misc_rng(void);
int gxse_device_register_gemini_misc_chip(void);
int gxse_device_register_gemini_misc_firewall(void);
int gxse_device_register_gemini_misc_sci(void);
int gxse_device_register_gemini_crypto_dma(void);
int gxse_device_register_gemini_crypto_fifo(void);
int gxse_device_register_gemini_secure_generic(void);

int gxse_device_register_gemini_klm(void);
int gxse_device_register_gemini_misc(void);
int gxse_device_register_gemini_crypto(void);
int gxse_device_register_gemini_secure(void);

int gxse_chip_register_gemini(void);
#endif
