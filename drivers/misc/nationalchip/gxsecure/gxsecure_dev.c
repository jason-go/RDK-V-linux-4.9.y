#include "common_dev.h"
#include "gxse_core.h"

static void probe_chip(void)
{
#ifdef CONFIG_SIRIUS
	PROBE_CHIP(sirius);
#endif
#if defined (CONFIG_TAURUS) || defined(CONFIG_GEMINI)
	PROBE_CHIP(taurus);
#endif
#ifdef CONFIG_GX3211
	PROBE_CHIP(gx3211);
#endif
#ifdef CONFIG_GX3113C
	PROBE_CHIP(gx3113c);
#endif
#ifdef CONFIG_GX6605S
	PROBE_CHIP(gx6605s);
#endif
#ifdef CONFIG_PEGASUS
	PROBE_CHIP(pegasus);
#endif
}

static int __init gxsecure_probe(void)
{
	probe_chip();

#ifdef CFG_GXSE_KLM
	klm_probe();
#endif

#ifdef CFG_GXSE_MISC
	misc_probe();
#endif

#ifdef CFG_GXSE_CRYPTO
	crypto_probe();
#endif

#ifdef CFG_GXSE_FIRMWARE
	secure_probe();
#endif

#ifdef CFG_GXSE_FIREWALL
	firewall_probe();
#endif

	return 0;
}

static void __exit gxsecure_remove(void)
{
#ifdef CFG_GXSE_KLM
	klm_remove();
#endif

#ifdef CFG_GXSE_MISC
	misc_remove();
#endif

#ifdef CFG_GXSE_CRYPTO
	crypto_remove();
#endif

#ifdef CFG_GXSE_FIRMWARE
	secure_remove();
#endif

#ifdef CFG_GXSE_FIREWALL
	firewall_remove();
#endif
}

module_init(gxsecure_probe);
module_exit(gxsecure_remove);

MODULE_DESCRIPTION("support for NationalChip gxsecure modules.");
MODULE_AUTHOR("NationalChip Co., Ltd.");
MODULE_LICENSE("GPL");

