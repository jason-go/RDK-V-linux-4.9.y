#include "gxsecure_virdev.h"
#include "sirius_mbox.h"
#include "sirius_mbox_reg.h"

struct mbox_priv sirius_mbox_priv;

static int32_t _mbox_firmware_has_been_loaded(void *vreg)
{
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)vreg;
	return reg->rcsr.value;
}

static int _mbox_get_sync_mode(int oob)
{
	switch (oob) {
	case MB_OOB_USER_TX:
	case MB_OOB_USER_RX:
	case MB_OOB_ACPU_SET_CW:
	case MB_OOB_ACPU_GET_RESP:
	case MB_OOB_ACPU_NEED_MISC_CW:
	case MB_OOB_ACPU_NEED_MISC_GP:
	case MB_OOB_ACPU_NEED_MISC_PVR:
	case MB_OOB_ACPU_NEED_MISC_SOFT:
	case MB_OOB_ACPU_SET_IRDETO_CMD_END:
		return FW_SYNC_CMD_OVER;

	case MB_OOB_ACPU_SELECT_ROOT_KEY:
	case MB_OOB_ACPU_SET_KN:
	case MB_OOB_ACPU_SET_IRDETO_CMD_INFO:
	case MB_OOB_ACPU_SET_IRDETO_CMD:
	case MB_OOB_SCPU_SEND_RESULT:
	case MB_OOB_CASE:
	case MB_OOB_CASE_FINISH:
	case MB_OOB_USER_CFG_DDR_INFO:
		return FW_SYNC_ACK;

	case MB_OOB_APP_IS_ALIVE:
		return FW_SYNC_ACK_TIMEOUT;

	default:
		break;
	}
	return FW_SYNC_NONE;
}

static int32_t _mbox_sync(void *priv, uint32_t oob)
{
	unsigned long spin_flag;
	struct mbox_priv *p = (struct mbox_priv *)priv;
	int32_t ret = _mbox_get_sync_mode(oob);

	if (oob == MB_OOB_ACPU_START) {
		while(!p->rom_start)
			gx_msleep(1);
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->rom_start = 0;
		p->write_en = SECURE_EVENT_W;
		gx_wake_event(&p->wr_queue, p->write_en);
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		return GXSE_SUCCESS;
	}

	if (ret == FW_SYNC_ACK) {
		while(!p->ack_en)
			gx_msleep(1);
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->ack_en = 0;
		p->write_en = SECURE_EVENT_W;
		gx_wake_event(&p->wr_queue, p->write_en);
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);

	} else if (ret == FW_SYNC_ACK_TIMEOUT) {
		int timeout = 0;
		while (!p->ack_en) {
			if (timeout == 500) // 500ms
				break;
			gx_msleep(1);
			timeout++;
		}
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->ack_en = 0;
		p->write_en = SECURE_EVENT_W;
		gx_wake_event(&p->wr_queue, p->write_en);
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		if (timeout == 500)
			return GXSE_ERR_GENERIC;

	} else if (ret == FW_SYNC_CMD_OVER) {
		while(!p->cmd_over)
			gx_msleep(1);
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->cmd_over = 0;
		p->read_en  = SECURE_EVENT_R;
		p->write_en = SECURE_EVENT_W;
		gx_wake_event(&p->rd_queue, p->read_en);
		gx_wake_event(&p->wr_queue, p->write_en);
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
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

	if (oob == MB_OOB_IFCP)
		gxlog_d(GXSE_LOG_MOD_SECURE, "[MBOX] write info (oob %x) (length %d)\n", oob, len);

	for (i = 0; i < len/4; i++) {
		if (oob == MB_OOB_IFCP)
			gxlog_d(GXSE_LOG_MOD_SECURE, "[MBOX] write %x\n", gx_u32_get_val(buf+4*i, 1));
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
	gxse_common_fifo_get(&p->fifo, data, 4);
	size = ((data[0]&0x1f) + 1) * 4;
	*oob  = (data[1])&0xff;

	if (*oob == MB_OOB_IFCP)
		gxlog_d(GXSE_LOG_MOD_SECURE, "[MBOX] read info (oob %x) (length %d)\n", *oob, size);
	for (i = 0; i < size; i+=4) {
		gxse_common_fifo_get(&p->fifo, &buf[i], 4);
		if (*oob == MB_OOB_IFCP)
			gxlog_d(GXSE_LOG_MOD_SECURE, "[MBOX] read %x\n", *(uint32_t *)&buf[i]);
	}

	*len = size;
	return GXSE_SUCCESS;
}

static int32_t _mbox_poll(void *priv, uint32_t flag)
{
	unsigned long spin_flag;
	int32_t ret = FW_POLL_NONE;
	struct mbox_priv *p = (struct mbox_priv *)priv;

	if (flag & FW_POLL_R) {
		if ((gx_wait_event(&p->rd_queue, p->read_en, SECURE_EVENT_R)) == 0) {
			gx_mask_event(&p->rd_queue, SECURE_EVENT_R);
			gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
			p->read_en = 0;
			gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
			ret |= FW_POLL_R;
		}
	}
	if (flag & FW_POLL_W) {
		if ((gx_wait_event(&p->wr_queue, p->write_en, SECURE_EVENT_W)) == 0) {
			gx_mask_event(&p->wr_queue, SECURE_EVENT_W);
			gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
			p->write_en = 0;
			gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
			ret |= FW_POLL_W;
		}
	}
	return ret;
}

static void _mbox_isr_user(volatile SiriusMboxReg *reg, void *priv, uint32_t oob)
{
	unsigned long spin_flag;
	struct mbox_priv *p = (struct mbox_priv *)priv;
	uint32_t fifo_left = gxse_common_fifo_freelen(&p->fifo);
	uint32_t i, data = 0, data_len = reg->rcsr.bits.data_len + 1;

	// TODO dealwith fifo full
	if (fifo_left < (data_len + 1)*4)
		gxlog_i(GXSE_LOG_MOD_SECURE, "[MBOX] fifo full\n");

	data = reg->rcsr.value&0xffff;
	gxse_common_fifo_put(&p->fifo, (unsigned char *)&data, 4);

	for (i = 0; i < data_len; i++) {
		data = reg->data;
		gxse_common_fifo_put(&p->fifo, (unsigned char *)&data, 4);
	}

	if (oob == MB_OOB_IFCP) {
		if (p->status != FW_IFCP_START)
			p->status = FW_IFCP_START;

	} else {
		if (_mbox_get_sync_mode(oob) == FW_SYNC_ACK)
			_mbox_tx(reg, MB_OOB_ACK, (void *)&i, 4);
	}

	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	if (oob == MB_OOB_SCPU_SEND_RESULT)
		p->cmd_over = 1;
	p->isr_rw_flag = SECURE_EVENT_R | SECURE_EVENT_W;
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
}

static void _mbox_isr_rom(volatile SiriusMboxReg *reg, void *priv, int oob)
{
	uint32_t ack;
	unsigned long spin_flag;
	struct mbox_priv *p = (struct mbox_priv *)priv;

	_mbox_rx(reg, (void *)&ack, 4);
	if (oob == MB_OOB_ROM_RET_ROM_START) {
		gxlog_i(GXSE_LOG_MOD_SECURE, "[MBOX] ROM START!\n");
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->rom_start = 1;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		return;
	}

	if (oob == MB_OOB_ROM_RET_CHECK_ACK)
		gxlog_i(GXSE_LOG_MOD_SECURE, "[MBOX] ROM check %s!\n", ack ? "error" : "ok");

	if (ack)
		p->status = FW_ERROR;
}

static void _mbox_isr_common(volatile SiriusMboxReg *reg, void *priv, int oob)
{
	uint32_t ack;
	unsigned long spin_flag;
	struct mbox_priv *p = (struct mbox_priv *)priv;

	_mbox_rx(reg, (void *)&ack, 4);

#ifdef CFG_MBOX_DEBUG_PRINT
	static int print_flag = -1;
#endif

	if (oob == MB_OOB_APP_START) {

#ifdef CFG_MBOX_DEBUG_PRINT
		if (ack == MBOX_PRINT_START) {
			print_flag = 0;
			printf("SCPU : ");
		}

		if (print_flag >= 0) {
			if (ack == MBOX_PRINT_END) {
				printf("\n");

				print_flag = -1;
			} else if (ack != MBOX_PRINT_START)
				printf ("%c", (unsigned char)ack);
		} else
#endif
			gxlog_i(GXSE_LOG_MOD_SECURE, "[MBOX] APP START! %x\n", ack);

		if (ack == FW_VAL_APP_FORMAL || ack == FW_VAL_APP_CUSTOM) {
			if (p->status != FW_CHIP_SERIALIZED)
				p->status = FW_APP_START;

		} else if (ack == FW_VAL_CHIP_SERIALIZED)
			p->status = FW_CHIP_SERIALIZED;

		else if (ack == FW_VAL_APP_PASSWD)
			p->status = FW_PASSWD;

		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->isr_rw_flag = SECURE_EVENT_W;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);

	} else if (oob == MB_OOB_APP_RESTART) {
		gxlog_i(GXSE_LOG_MOD_SECURE, "[MBOX] APP RESTART!\n");
		p->status = FW_RESET;

	} else if (oob == MB_OOB_ACK) {
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->ack_en = 1;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	}
}

int32_t sirius_mbox_init(GxSeModuleHwObj *obj)
{
	struct mbox_priv *p = (struct mbox_priv *)obj->priv;
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)obj->reg;

	memset(p, 0, sizeof(struct mbox_priv));
	gxse_common_fifo_init(&p->fifo, NULL, 1024);
	gx_spin_lock_init(&p->spin_lock);
	gx_event_init(&p->rd_queue);
	gx_event_init(&p->wr_queue);
	p->status = FW_RUNNING;

	reg->mb_int_en.value = 0x2;

	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;

	return GXSE_SUCCESS;
}

int32_t sirius_mbox_check_param(void *vreg, void *priv, void *param, uint32_t cmd)
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

			if (loader->dst_addr == 0x0)
				loader->dst_addr = 0x90000;
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

int32_t sirius_mbox_get_status(void *vreg, void *priv)
{
	struct mbox_priv *p = (struct mbox_priv *)priv;
	(void) vreg;

#ifdef CFG_MBOX_SET_DDR_INFO
	if (p->status == FW_IFCP_START || p->status == FW_APP_START) {
		uint32_t buf[2] = {0};
		buf[0] = p->fw_load_addr;
		buf[1] = p->fw_load_size;
		sirius_mbox_tx(vreg, priv, MB_OOB_USER_CFG_DDR_INFO,(uint8_t *)buf, 8);
	}
#endif

	return (p->status == FW_IFCP_START) ? FW_APP_START : p->status;
}

int32_t sirius_mbox_get_blocklen(void *vreg, void *priv)
{
	(void) priv;
	(void) vreg;
	return SIRIUS_MBOX_BLOCKLEN;
}

int32_t sirius_mbox_get_siglen(void *vreg, void *priv)
{
	(void) priv;
	(void) vreg;
	return SIRIUS_MBOX_SIGLEN;
}

int32_t sirius_mbox_firmware_is_alive(void *vreg, void *priv)
{
	struct mbox_priv *p = (struct mbox_priv *)priv;

	if (p->status == FW_APP_DEAD)
		return 0;

	if (p->status < FW_APP_START) {
		if (_mbox_firmware_has_been_loaded(vreg) == 0) {
			gxlog_i(GXSE_LOG_MOD_SECURE, "Please load firmware first.\n");
			return 0;
		}

		_mbox_tx(vreg, MB_OOB_APP_IS_ALIVE, NULL, 0);
		if (_mbox_sync(priv, MB_OOB_APP_IS_ALIVE) < 0) {
			p->status = FW_APP_DEAD;
			gxlog_i(GXSE_LOG_MOD_SECURE, "SCPU firmware is dead\n");
			return 0;
		}
		p->status = FW_CHIP_SERIALIZED;
	}

	return 1; //firmware is running
}

int32_t sirius_mbox_tx(void *vreg, void *priv, uint32_t oob, uint8_t *buf, uint32_t size)
{
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)vreg;

	if (size > SIRIUS_MBOX_BLOCKLEN)
		return GXSE_ERR_GENERIC;

	if (oob < MB_OOB_APP_ADDR)
		_mbox_poll(priv, FW_POLL_W);
	_mbox_tx(reg, oob, buf, size);
	_mbox_sync(priv, oob);

	return GXSE_SUCCESS;
}

int32_t sirius_mbox_rx(void *vreg, void *priv, uint32_t *oob, uint8_t *buf, uint32_t size)
{
	int32_t ret = 0;
	(void) vreg;

	if (size > SIRIUS_MBOX_BLOCKLEN || NULL == oob) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "rx param error.\n");
		return GXSE_ERR_GENERIC;
	}

	_mbox_poll(priv, FW_POLL_R);
	if ((ret = _mbox_rx_fifo(priv, oob, buf, &size)) < 0) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "rx fifo error.\n");
		return GXSE_ERR_GENERIC;
	}

	return size;
}

int32_t sirius_mbox_trx(void *vreg, void *priv, uint32_t oob, uint8_t *in, uint32_t inlen, uint8_t *out,uint32_t outlen)
{
	GxSecureResult result = {0};
	int32_t ret = 0;
	uint32_t retlen = 0, retoob = 0;
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)vreg;
	(void) outlen;

	if (inlen > SIRIUS_MBOX_BLOCKLEN)
		return GXSE_ERR_GENERIC;

	_mbox_poll(priv, FW_POLL_W);
	_mbox_tx(reg, oob, in, inlen);
	_mbox_sync(priv, oob);

	_mbox_poll(priv, FW_POLL_R);
	ret = _mbox_rx_fifo(priv, &retoob, (void *)&result, &retlen);
	if (ret < 0 ||
		retoob != MB_OOB_SCPU_SEND_RESULT ||
		retlen != sizeof(GxSecureResult)) {
		gxlog_e(GXSE_LOG_MOD_SECURE, "rx fifo error.\n");
		return GXSE_ERR_GENERIC;
	}

	if (result.type != FW_RET_RW_RESP)
		return (result.type == FW_RET_RW_ERROR) ? GXSE_ERR_GENERIC : GXSE_SUCCESS;

	if (out) {
		if (outlen < result.len) {
			gxlog_e(GXSE_LOG_MOD_SECURE, "get result failed.\n");
			return GXSE_ERR_GENERIC;
		}
		memcpy(out, result.buffer, result.len);
	}
	return result.len;
}

int32_t sirius_mbox_set_BIV(void *vreg, void *priv, uint8_t *buf, uint32_t size)
{
	int32_t i = 0;
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)vreg;
	(void) priv;
	(void) size;

	for (i = 0; i < 4; i++)
		reg->BIV[3-i] = gx_u32_get_val(buf+4*i, 0);

	return GXSE_SUCCESS;
}

int32_t sirius_mbox_get_BIV(void *vreg, void *priv, uint8_t *buf, uint32_t size)
{
	int32_t i = 0;
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)vreg;
	(void) priv;
	(void) size;

	for (i = 0; i < 4; i++) {
		gx_u32_set_val(buf+4*i, &reg->BIV[3-i], 0);
	}

	return GXSE_SUCCESS;
}

int32_t sirius_mbox_dsr(GxSeModuleHwObj *obj)
{
	unsigned long spin_flag;
	struct mbox_priv *p = (struct mbox_priv *)obj->priv;

	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	if (p->isr_rw_flag & SECURE_EVENT_W) {
		p->write_en = SECURE_EVENT_W;
		gx_wake_event(&p->wr_queue, p->write_en);
	}
	if (p->isr_rw_flag & SECURE_EVENT_R) {
		p->read_en  = SECURE_EVENT_R;
		gx_wake_event(&p->rd_queue, p->read_en);
	}
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);

	return GXSE_SUCCESS;
}

int32_t sirius_mbox_isr(GxSeModuleHwObj *obj)
{
	GxMBType oob;
	unsigned long spin_flag;
	struct mbox_priv *p = (struct mbox_priv *)obj->priv;
	volatile SiriusMboxReg *reg = (volatile SiriusMboxReg *)obj->reg;

	oob = reg->rcsr.bits.oob;
	if (oob >= MB_OOB_APP_ADDR)
		_mbox_isr_rom(reg, obj->priv, oob);

	else if ((oob >= MB_OOB_USER_TX &&
		oob <= MB_OOB_CASE_FINISH) ||
		oob == MB_OOB_IFCP ||
		oob == MB_OOB_APP_IS_ALIVE) {
		_mbox_isr_user(reg, obj->priv, oob);
	}

	else if (oob <= MB_OOB_APP_RESTART)
		_mbox_isr_common(reg, obj->priv, oob);

	else {
		gxlog_i(GXSE_LOG_MOD_SECURE, "[MBOX] oob 0x%x undefined!\n", oob);

		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		if (!p->write_en)
			p->isr_rw_flag = SECURE_EVENT_W;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	}
#ifndef ECOS_OS
	sirius_mbox_dsr(obj);
#endif

	return GXSE_SUCCESS;
}

static GxSeModuleDevOps sirius_mbox_devops = {
	.init            = sirius_mbox_init,
	.isr             = sirius_mbox_isr,
	.dsr             = sirius_mbox_dsr,
	.read            = gx_secure_module_read,
	.write           = gx_secure_module_write,
};

static GxSecureOps sirius_mbox_ops = {
	.check_param        = sirius_mbox_check_param,
	.get_status         = sirius_mbox_get_status,
	.get_blocklen       = sirius_mbox_get_blocklen,
	.get_siglen         = sirius_mbox_get_siglen,

	.firmware_is_alive  = sirius_mbox_firmware_is_alive,
	.tx                 = sirius_mbox_tx,
	.rx                 = sirius_mbox_rx,
	.trx                = sirius_mbox_trx,
	.set_BIV            = sirius_mbox_set_BIV,
	.get_BIV            = sirius_mbox_get_BIV,
};

static GxSeModuleHwObjSecureOps sirius_mbox_obj_ops = {
	.devops = &sirius_mbox_devops,
	.hwops = &sirius_mbox_ops,

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

static GxSeModuleHwObj sirius_mbox_obj = {
	.type   = GXSE_HWOBJ_TYPE_FIRMWARE,
	.ops    = &sirius_mbox_obj_ops,
	.priv   = &sirius_mbox_priv,
};

GxSeModule sirius_mbox_module = {
	.id   = GXSE_MOD_SECURE_MBOX,
	.ops  = &secure_dev_ops,
	.hwobj= &sirius_mbox_obj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_SECURE,
		.reg_len   = sizeof(SiriusMboxReg),
		.irqs      = {GXACPU_IRQ_SECURE, -1},
		.irq_names  = {"secure"},
	},
};
