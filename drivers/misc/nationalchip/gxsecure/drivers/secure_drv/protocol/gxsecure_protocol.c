#include "gxsecure_protocol.h"

static GxSecureProtocol *gx_secure_protocol_list[MBOX_PROTOCOL_MAX] = {0};
static void gx_secure_protocol_probe(void)
{
	static int init = 0;
	if (init == 0) {
#ifdef CFG_MBOX_PROTOCOL_S5H
		gx_secure_protocol_list[MBOX_PROTOCOL_S5H]     = &secure_protocol_S5H;
#endif
#ifdef CFG_MBOX_PROTOCOL_IFCP
		gx_secure_protocol_list[MBOX_PROTOCOL_IFCP]    = &secure_protocol_IFCP;
#endif
#ifdef CFG_MBOX_PROTOCOL_GENERIC
		gx_secure_protocol_list[MBOX_PROTOCOL_GENERIC] = &secure_protocol_generic;
#endif
		init = 1;
	}
}

int32_t gx_secure_protocol_read(GxSeModuleHwObj *obj, GxSecureUserData *param, uint32_t protocol)
{
	GxSecureProtocol *p = NULL;
	gx_secure_protocol_probe();
	if (protocol > MBOX_PROTOCOL_MAX || NULL == gx_secure_protocol_list[protocol]) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Don't supported the protocol, %d\n", protocol);
		return GXSE_ERR_GENERIC;
	}

	p = gx_secure_protocol_list[protocol];
	if (p->rx)
		return p->rx(obj, param);

	return GXSE_ERR_GENERIC;
}

int32_t gx_secure_protocol_write(GxSeModuleHwObj *obj, GxSecureUserData *param, uint32_t protocol)
{
	GxSecureProtocol *p = NULL;
	gx_secure_protocol_probe();
	if (protocol > MBOX_PROTOCOL_MAX || NULL == gx_secure_protocol_list[protocol]) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "Don't supported the protocol, %d\n", protocol);
		return GXSE_ERR_GENERIC;
	}

	p = gx_secure_protocol_list[protocol];
	if (p->tx)
		return p->tx(obj, param);

	return GXSE_ERR_GENERIC;
}
