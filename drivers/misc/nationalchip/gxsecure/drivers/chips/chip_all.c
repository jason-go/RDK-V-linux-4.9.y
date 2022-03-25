#include "sirius.h"
#include "taurus.h"
#include "gx3211.h"
#include "gx3113c.h"
#include "gx6605s.h"
#include "gemini.h"
#include "cygnus.h"
#include "chip_all.h"

int gxse_chip_register_all(void)
{
	uint32_t chipid = gx_osdep_chip_probe();

	switch (chipid) {
	case GXSECURE_CHIPID_GX3211:
		return gxse_chip_register_gx3211();

	case GXSECURE_CHIPID_GX3113C:
		return gxse_chip_register_gx3113c();

	case GXSECURE_CHIPID_GX6605S:
		return gxse_chip_register_gx6605s();

	case GXSECURE_CHIPID_SIRIUS:
		return gxse_chip_register_sirius();

	case GXSECURE_CHIPID_TAURUS:
		return gxse_chip_register_taurus();

	case GXSECURE_CHIPID_GEMINI:
		return gxse_chip_register_gemini();

	case GXSECURE_CHIPID_CYGNUS:
		return gxse_chip_register_cygnus();

	default:
		break;
	}
    return GXSE_ERR_GENERIC;
}

