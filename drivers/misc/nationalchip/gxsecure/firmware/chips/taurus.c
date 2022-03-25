#include "gxse_core.h"

extern GxSeModule sirius_mbox_scpu_module;
extern GxSeModule taurus_misc_scpu_module;
extern GxSeModule sirius_klm_scpu_dynamic_module;
extern GxSeModule taurus_crypto_scpu_dynamic_module;
extern GxSeModule taurus_crypto_module;

int gxse_chip_register_taurus(void)
{
#ifdef CFG_GXSE_FIRMWARE
	gxse_module_register(&sirius_mbox_scpu_module);
#endif

#ifdef CFG_GXSE_MISC
	gxse_module_register(&taurus_misc_scpu_module);
#endif

#ifdef CFG_GXSE_KLM
	gxse_module_register(&sirius_klm_scpu_dynamic_module);
#endif

#ifdef CFG_GXSE_CRYPTO
#ifdef CFG_GXSE_CRYPTO_FIFO
	gxse_module_register(&taurus_crypto_scpu_dynamic_module);
#endif
#ifdef CFG_GXSE_CRYPTO_FIFO_BOOT
	gxse_module_register(&taurus_crypto_module);
#endif
#endif

	return 0;
}

