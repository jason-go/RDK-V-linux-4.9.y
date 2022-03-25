#include "gxsecure_virdev.h"
#include "sirius_scpu_mbox.h"
#include "sirius_mbox_reg.h"

static uint8_t s_fifo[1024] = {0};
static struct scpu_mbox_priv sirius_scpu_mbox_priv;
static int _mbox_get_sync_mode(int oob)
{
	switch (oob) {
	case MB_OOB_ACPU_SELECT_ROOT_KEY:
	case MB_OOB_ACPU_SET_KN:
	case MB_OOB_ACPU_SET_IRDETO_CMD_INFO:
	case MB_OOB_ACPU_SET_IRDETO_CMD:
	case MB_OOB_SCPU_SEND_RESULT:
	case MB_OOB_CASE:
	case MB_OOB_CASE_FINISH:
	case MB_OOB_USER_CFG_DDR_INFO:
		return FW_SYNC_ACK;

	default:
		break;
	}
	return FW_SYNC_NONE;
}

static int32_t _mbox_sync(void *priv, uint32_t oob)
{
	struct scpu_mbox_priv *p = (struct scpu_mbox_priv *)priv;
	int32_t ret = _mbox_get_sync_mode(oob);

	if (ret == FW_SYNC_ACK) {
		while(!p->ack_en);
		p->ack_en = 0;
		p->write_en = SECURE_EVENT_W;
	}

	return GXSE_SUCCESS;
}

static void _mbox_tx(volatile SiriusMboxReg *reg, uint32_t oob, uint8_t *buf, uint32_t len)
{
	uint32_t i = 0;

	if (NULL == buf) {
		buf = (void *)&i;
		len = 4;
	}

	while (reg->wcsr.bits.state != FW_ST_IDLE);
	reg->wcsr.value = (oob<<8) | (len/4-1);

	for (i = 0; i < len/4; i++) {
		reg->data = gx_u32_get_val(buf+4*i, 1);
	}
}

static void _mbox_rx(volatile SiriusMboxReg *reg, uint8_t *buf, uint32_t len)
{
	uint32_t i = 0;
	uint32_t *temp = (void *)buf;

	while (reg->rcsr.bits.state != FW_ST_READ);
	for (i = 0; i < len/4; i++)
		temp[i] = reg->data;
}

static int32_t _mbox_rx_fifo(void *priv, uint32_t *oob, uint8_t *buf, uint32_t *len)
{
	uint8_t data[4];
	uint32_t i, size = 0;
	struct mbox_priv *p = (struct mbox_priv *)priv;

	if (gxse_common_fifo_len(&p->fifo) == 0)
		return GXSE_ERR_GENERIC;

	//TODO fifo_peek
	//*len != size
	gxse_common_fifo_get(&p->fifo, data, 4);
	size = ((data[0]&0x1f) + 1) * 4;
	*oob  = (data[1])&0xff;

	for (i = 0; i < size; i+=4)
		gxse_common_fifo_get(&p->fifo, &buf[i], 4);

	*len = size;
	return GXSE_SUCCESS;
}

static void _mbox_isr_user(volatile SiriusMboxReg *reg, void *priv, int oob)
{
	struct scpu_mbox_priv *p = (struct scpu_mbox_priv *)priv;
	uint32_t fifo_left = gxse_common_fifo_freelen(&p->fifo);
	uint32_t i, data = 0, data_len = reg->rcsr.bits.data_len + 1;

	// TODO dealwith fifo full
	if (fifo_left < (data_len + 1)*4)
		gxlog_i(GXSE_LOG_MOD_MBOX, "[MBOX] fifo full\n");

	data = reg->rcsr.value&0xffff;
	gxse_common_fifo_put(&p->fifo, (unsigned char *)&data, 4);

	for (i = 0; i < data_len; i++) {
		data = reg->data;
		gxse_common_fifo_put(&p->fifo, (unsigned char *)&data, 4);
	}

	if (oob != MB_OOB_IFCP) {
		if (_mbox_get_sync_mode(oob) == FW_SYNC_ACK)
			_mbox_tx(reg, MB_OOB_ACK, (void *)&data, 4);
	}

	p->read_en  = SECURE_EVENT_R;
	p->write_en = SECURE_EVENT_W;
}

static void _mbox_isr_common(volatile SiriusMboxReg *reg, void *priv, int oob)
{
	uint32_t ack;
	struct scpu_mbox_priv *p = (struct scpu_mbox_priv *)priv;
	(void) oob;

	_mbox_rx(reg, (void *)&ack, 4);
	p->ack_en = 1;
}

int32_t sirius_scpu_mbox_init(GxSeModuleHwObj *obj)
{
	struct scpu_mbox_priv *p = (struct scpu_mbox_priv *)obj->priv;
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)obj->reg;

	memset(p, 0, sizeof(struct scpu_mbox_priv));
	gxse_common_fifo_init(&p->fifo, s_fifo, 1024);

	p->read_en = 0;
	p->write_en = 0;
	p->reg = reg;
	reg->mb_int_en.value = 0x2;

	return GXSE_SUCCESS;
}

int32_t sirius_scpu_mbox_tx(void *vreg, void *priv, uint32_t oob, uint8_t *buf, uint32_t size)
{
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)vreg;

	if (size > SIRIUS_MBOX_BLOCKLEN)
		return GXSE_ERR_GENERIC;

	_mbox_tx(reg, oob, buf, size);
	_mbox_sync(priv, oob);

	return GXSE_SUCCESS;
}

int32_t sirius_scpu_mbox_rx(void *vreg, void *priv, uint32_t *oob, uint8_t *buf, uint32_t size)
{
	int32_t ret = 0;

	if (size > SIRIUS_MBOX_BLOCKLEN || NULL == oob)
		return GXSE_ERR_GENERIC;

	if ((ret = _mbox_rx_fifo(priv, oob, buf, &size)) < 0)
		return GXSE_ERR_GENERIC;

	return size;
}

int gx_mbox_scpu_isr(int irq, void *pdata)
{
	GxMBType oob;
	struct scpu_mbox_priv *p = (struct scpu_mbox_priv *)&sirius_scpu_mbox_priv;
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)p->reg;

	oob = reg->rcsr.bits.oob;
	if ((oob >= MB_OOB_USER_TX &&
		oob <= MB_OOB_CASE_FINISH) ||
		oob == MB_OOB_IFCP)
		_mbox_isr_user(reg, p, oob);

	else if (oob <= MB_OOB_APP_RESTART || oob == MB_OOB_APP_IS_ALIVE)
		_mbox_isr_common(reg, p, oob);

	else {
		if (!p->write_en)
			p->write_en = SECURE_EVENT_W;
	}

	return 0;
}

static GxSeModuleDevOps sirius_mbox_devops = {
	.init = sirius_scpu_mbox_init,
};

static GxSecureOps sirius_mbox_ops = {
	.tx = sirius_scpu_mbox_tx,
	.rx = sirius_scpu_mbox_rx,
};

static GxSeModuleHwObjSecureOps mbox_obj_ops = {
	.devops = &sirius_mbox_devops,
	.hwops = &sirius_mbox_ops,

	.direct_tx = gx_secure_object_direct_tx,
	.direct_rx = gx_secure_object_direct_rx,
};

static GxSeModuleHwObj sirius_scpu_mbox_obj = {
	.type   = GXSE_HWOBJ_TYPE_FIRMWARE,
	.ops    = &mbox_obj_ops,
	.priv   = &sirius_scpu_mbox_priv,
};

GxSeModule sirius_mbox_scpu_module = {
	.id   = GXSE_MOD_SECURE_MBOX,
	.ops  = &secure_dev_ops,
	.hwobj= &sirius_scpu_mbox_obj,
	.res  = {
		.reg_base  = GXSCPU_BASE_ADDR_MAILBOX,
		.reg_len   = sizeof(SiriusMboxReg),
	},
};
