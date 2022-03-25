#include "gxsecure_virdev.h"
#include "sirius_mbox.h"
#include "sirius_mbox_reg.h"

#define TAURUS_MBOX_SIGLEN   (0)

static int32_t taurus_mbox_get_siglen(void *vreg, void *priv)
{
	(void) priv;
	(void) vreg;
	return TAURUS_MBOX_SIGLEN;
}

static int32_t taurus_mbox_check_param(void *vreg, void *priv, void *param, uint32_t cmd)
{
	struct mbox_priv *p = (struct mbox_priv *)priv;
	(void) vreg;

	switch (cmd) {
	case SECURE_SEND_LOADER:
		{
			GxSecureLoader *loader = (GxSecureLoader *)param;

			if (NULL == loader->loader || loader->size == 0)
				return GXSE_ERR_PARAM;

			if (p->status >= FW_APP_START)
				return GXSE_ERR_GENERIC;

			if (loader->dst_addr != 0x0 && loader->dst_addr != 0x40000 && loader->dst_addr != 0x80000) {
				if (((loader->dst_addr >> 28) & 0xF) != 0x1) {
					gxlog_e(GXSE_MOD_SECURE_MBOX, "firmware load addr error.\n");
					return -2;
				}
				p->fw_load_addr = loader->dst_addr;
				p->fw_load_size = loader->size;
				loader->dst_addr = 0x40000;
			} else if (loader->dst_addr == 0x0)
				loader->dst_addr = 0x80000;
		}
		break;

	case SECURE_SEND_ROOTKEY:
	case SECURE_SEND_KN:
	case SECURE_SEND_CW:
	case SECURE_SEND_IRDETO_CMD:
	case SECURE_GET_RESP:
	case SECURE_SEND_USER_KEY:
		break;

	default:
		gxlog_e(GXSE_LOG_MOD_SECURE, "Not support the cmd.\n");
		return GXSE_ERR_GENERIC;
	};

	return GXSE_SUCCESS;
}

static GxSeModuleDevOps taurus_mbox_devops = {
	.init            = sirius_mbox_init,
	.isr             = sirius_mbox_isr,
	.dsr             = sirius_mbox_dsr,
	.read            = gx_secure_module_read,
	.write           = gx_secure_module_write,
};

static GxSecureOps taurus_mbox_ops = {
	.check_param        = taurus_mbox_check_param,
	.get_status         = sirius_mbox_get_status,
	.get_blocklen       = sirius_mbox_get_blocklen,
	.get_siglen         = taurus_mbox_get_siglen,

	.firmware_is_alive  = sirius_mbox_firmware_is_alive,
	.tx                 = sirius_mbox_tx,
	.rx                 = sirius_mbox_rx,
	.trx                = sirius_mbox_trx,
	.set_BIV            = sirius_mbox_set_BIV,
	.get_BIV            = sirius_mbox_get_BIV,
};

static GxSeModuleHwObjSecureOps taurus_mbox_obj_ops = {
	.devops = &taurus_mbox_devops,
	.hwops = &taurus_mbox_ops,

	.load_firmware   = gx_secure_object_load_firmware,
	.get_status      = gx_secure_object_get_status,
	.set_BIV         = gx_secure_object_set_BIV,
	.get_BIV         = gx_secure_object_get_BIV,
	.send_rootkey    = gx_secure_object_send_rootkey,
	.send_kn         = gx_secure_object_send_kn,
	.send_cw         = gx_secure_object_send_cw,
	.send_user_key   = gx_secure_object_send_user_key,
	.send_irdeto_cmd = gx_secure_object_send_irdeto_cmd,
	.get_resp        = gx_secure_object_get_resp,
};

static GxSeModuleHwObj taurus_mbox_obj = {
	.type   = GXSE_HWOBJ_TYPE_FIRMWARE,
	.ops    = &taurus_mbox_obj_ops,
	.priv   = &sirius_mbox_priv,
};

GxSeModule taurus_mbox_module = {
	.id   = GXSE_MOD_SECURE_MBOX,
	.ops  = &secure_dev_ops,
	.hwobj= &taurus_mbox_obj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_SECURE,
		.reg_len   = sizeof(SiriusMboxReg),
		.irqs      = {GXACPU_IRQ_TAURUS_SECURE, -1},
		.irq_names  = {"secure"},
	},
};
