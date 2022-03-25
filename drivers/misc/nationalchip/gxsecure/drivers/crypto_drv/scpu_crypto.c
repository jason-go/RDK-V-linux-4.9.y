#include "sirius_crypto_reg.h"
#include "sirius_crypto.h"

//#define DEBUG_CTRL
struct crypto_isr_info scpu_isr_info = {
	.run_flag = 0,
	.err_flag = 0,
};

struct fetch_crypto_info {
	struct crypto_isr_info *isr_info;
};

int32_t scpu_crypto_set_ready(void *vreg, void *priv)
{
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct fetch_crypto_info *p = (struct fetch_crypto_info *)priv;

#ifdef DEBUG_CTRL
	gx_fw_debug_print(0x3000);
	gx_fw_debug_print(reg->dctrl.value);
	gx_fw_debug_print(0x3001);
#endif

	p->isr_info->run_flag = 1;
	reg->dctrl.bits.start = 1;

	return GXSE_SUCCESS;
}

int32_t scpu_crypto_wait_finish(void *vreg, void *priv, uint32_t is_query)
{
	struct fetch_crypto_info *p = (struct fetch_crypto_info *)priv;
	(void) vreg;
	(void) is_query;

	if (p->isr_info->err_flag) {
		CHECK_ISR_TIMEOUT(GXSE_MOD_CRYPTO, p->isr_info->run_flag, TFM_MASK_DONE, 5, 0);
		p->isr_info->err_flag = 0;
	} else
		while(p->isr_info->run_flag);

	return GXSE_SUCCESS;
}

int32_t scpu_crypto_init(GxSeModuleHwObj *obj)
{
	struct fetch_crypto_info *p = (struct fetch_crypto_info *)obj->priv;

	p->isr_info = &scpu_isr_info;
	p->isr_info->reg = (volatile SiriusCryptoReg *)obj->reg;

	return GXSE_SUCCESS;
}

int gx_crypto_isr(int irq, void *pdata)
{
	volatile SiriusCryptoReg *reg = scpu_isr_info.reg;

	if (reg->crypto_int.bits.crypto_done && reg->crypto_int_en.bits.crypto_done) {
		reg->crypto_int.bits.crypto_done = 1;
		scpu_isr_info.run_flag = 0;
	}

	if (reg->crypto_int.bits.crypto_key_err && reg->crypto_int_en.bits.crypto_key_err) {
		reg->crypto_int.bits.crypto_key_err = 1;
		scpu_isr_info.run_flag = 0;
		scpu_isr_info.err_flag = 1;
	}
	return 0;
}
