#ifndef __SCPU_CRYPTO_H__
#define __SCPU_CRYPTO_H__

#include "gxcrypto_virdev.h"
#include "sirius_crypto_reg.h"

struct  crypto_isr_info{
	volatile SiriusCryptoReg *reg;
	volatile int run_flag;
	volatile int err_flag;
};

extern struct crypto_isr_info scpu_isr_info;
int32_t scpu_crypto_set_ready(void *vreg, void *priv);
int32_t scpu_crypto_wait_finish(void *vreg, void *priv, uint32_t is_query);
int32_t scpu_crypto_init(GxSeModuleHwObj *obj);

#endif
