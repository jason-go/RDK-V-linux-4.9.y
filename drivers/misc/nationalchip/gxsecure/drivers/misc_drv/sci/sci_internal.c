#include "gxmisc_sci_common.h"
#include "sci_chiptest.h"

static void dbg_dump(char *hdr, uint8_t *buf, int size)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 3)
	int len, i, j, c;
	char line[128];
	if (size == 0)
		return;
	gx_printf("[%s] total len = %d\n", hdr, size);
	for (i = 0; i < size; i += 16) {
		len = size - i;
		if (len > 16)
			len = 16;
		sprintf(line, "%08x ", i);
		for (j = 0; j < 16; j++) {
			if (j < len)
				sprintf(line, "%s %02x", line, buf[i+j]);
			else
				sprintf(line, "%s   ", line);
		}
		sprintf(line, "%s ", line);
		for (j = 0; j < len; j++) {
			c = buf[i+j];
			if (c < ' ' || c > '~')
				c = '.';
			sprintf(line, "%s%c", line, c);
		}
		gx_printf("%s\n", line);
	}
#endif
}

static int32_t _sci_printf_regs(volatile SCIReg *reg)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 3)
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_CTRL1:        %08x\n", reg->ctrl1.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_CTRL2:        %08x\n", reg->ctrl2.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_CTRL3:        %08x\n", reg->ctrl3.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_STATUS:       %08x\n", reg->isr.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_INTEN:        %08x\n", reg->isr_en.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_EGT:          %08x\n", reg->EGT);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_TGT:          %08x\n", reg->TGT);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_WDT:          %08x\n", reg->WDT);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_TWDT:         %08x\n", reg->TWDT);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_CFG_ACK2:     %08x\n", reg->ack2_cfg.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_CFG_CLDRST:   %08x\n", reg->cldrst_cfg.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_CFG_HOTRST:   %08x\n", reg->hotrst_cfg.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_CFG_PARITY:   %08x\n", reg->parity_cfg.value);
	gxlog_i(GXSE_LOG_MOD_MISC_SCI, "rSCI_CFG_IO:       %08x\n", reg->io_cfg.value);
#endif
	return 0;
}

static int32_t _sci_check_status(volatile SCIReg *reg, uint32_t status)
{
	if ((status & SCI_INTSTATUS_SCIIN) == 0) {
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "smartcard plugged out while reading/writing data!\n");
		return GXSE_ERR_GENERIC;
	}

	if (status & SCI_INTSTATUS_CARD_NOACK) {
		reg->isr_en.bits.nack = 1;
		return GXSE_ERR_GENERIC;
	}

	if (status & SCI_INTSTATUS_REPEAT_ERROR) {
		reg->isr_en.bits.repeat_error = 1;
		return GXSE_ERR_GENERIC;
	}

	if (status & SCI_INTSTATUS_RX_FIFO_OVERFLOW) {
		reg->ctrl1.bits.scirst = 1;
		reg->ctrl1.bits.scirst = 0;
		reg->isr_en.bits.rxfifo_overflow = 1;
		return GXSE_ERR_GENERIC;
	}

	return GXSE_SUCCESS;
}

static void _sci_fifo_reset(volatile SCIReg *reg, struct sci_priv *p)
{
	unsigned long spin_flag;
	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	p->rptr = 0;
	p->wptr = 0;
	p->rdata_num = 0;
	p->wdata_num = 0;
	p->write_end = false;
	p->read_end  = false;
	p->card_status &= SCI_INTSTATUS_SCIIN;
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	reg->ctrl1.bits.fiforst = 1;
	while(reg->isr.bits.data_size);
}

static int32_t _sci_isr_check_error(volatile SCIReg *reg, struct sci_priv *p, uint32_t status)
{
	unsigned long spin_flag;

	if (status & SCI_ISR_BIT_NACK) {
		reg->isr_en.bits.nack = 0;
		reg->isr.bits.nack = 1;
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "---- SCI_STATUS_CARD_NACK [%08x] ----\n", status);
		p->card_status |= SCI_INTSTATUS_CARD_NOACK;
	}

	if (status & SCI_ISR_BIT_REPEAT_ERROR) {
		reg->isr_en.bits.repeat_error = 0;
		reg->isr.bits.repeat_error = 1;
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "---- SCI_STATUS_REPEAT_ERROR [%08x] ----\n", status);
		p->card_status |= SCI_INTSTATUS_REPEAT_ERROR;
	}

	/* check rx overflow first */
	if (reg->isr.bits.rxfifo_overflow) {
		reg->isr_en.bits.rxfifo_overflow = 0;
		reg->isr.bits.rxfifo_overflow = 1;
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "---- SCI_INTSTATUS_RX_FIFO_OVERFLOW [%08x] ----\n", status);
		p->card_status |= SCI_INTSTATUS_RX_FIFO_OVERFLOW;

		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->wptr = 0;
		p->wdata_num = 0;
		p->write_end = true;
		p->rdata_num = 0;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		gx_wake_event(&p->wr_queue, p->write_end);
	}
	return GXSE_SUCCESS;
}

static int32_t _sci_isr_check_inout(volatile SCIReg *reg, struct sci_priv *p, uint32_t status)
{
	unsigned long spin_flag;
	/* NOTE: here's one hardware bug (when card inserted/plugged out, rSCI_STATUS[0]
	   & rSCI_STATUS[1] MAY be set simultaneously, and two interrupts triggered).
	   One interrupt should be ignored by checking rSCI_STATUS[10] */
	if (status & (SCI_ISR_BIT_SCIIN | SCI_ISR_BIT_SCIOUT)) {/* rSCI_STATUS[0] */
		reg->isr.bits.sci_in = 1;
		reg->isr.bits.sci_out = 1;

		/* call SCOUT callback in DSR if card plugged out */
		if (reg->isr.bits.card_in == 0) {
			//0 out, 1 in
			/* reset the module first, always */
			/* NOTE: maybe some card-in interrupts will be triggered for verilog bugs */
			gxlog_i(GXSE_LOG_MOD_MISC_SCI, "---- SCI_INTSTATUS_SCIOUT [%08x] ----\n", status);
			reg->isr_en.bits.sci_in = 1;
			reg->isr_en.bits.sci_out = 1;
			reg->ctrl1.bits.scirst = 1;
			reg->ctrl1.bits.scirst = 0;
			reg->io_cfg.bits.din_hold_en = 1;
			reg->ctrl1.bits.deact = 1;
			reg->io_cfg.bits.card_in_sw = 0;
			p->card_status &= ~SCI_INTSTATUS_SCIIN;

			/* to prompt card-out error for read/write operations */
			if (p->status == SCI_STATUS_WRITE) {
				gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
				p->wptr = 0;
				p->wdata_num = 0;
				p->write_end = true;
				gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
				gx_wake_event(&p->wr_queue, p->write_end);

			} else if (p->status == SCI_STATUS_READ) {
				gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
				p->read_end = SCI_READ_VALID;
				gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
				gx_select_wake(&p->rd_queue, p->read_end);
			}

   		} else {
			gxlog_i(GXSE_LOG_MOD_MISC_SCI, "---- SCI_INTSTATUS_SCIIN [%08x] ----\n", status);
			_sci_fifo_reset(reg, p);
			reg->isr_en.value = 0xb67;
			reg->io_cfg.bits.din_hold_en = 0;
			reg->io_cfg.bits.card_in_sw = 1;
			p->card_status |= SCI_INTSTATUS_SCIIN;
		}
	}

	if (reg->isr.bits.card_in_diff) {
		gxlog_i(GXSE_LOG_MOD_MISC_SCI, "---- SCI_INTSTATUS_SCI_DIFF [%08x] ----\n", status);
		reg->io_cfg.bits.card_in_sw = reg->isr.bits.card_in;
	}
	return GXSE_SUCCESS;
}

static int32_t _sci_isr_rx_console(volatile SCIReg *reg, struct sci_priv *p, uint32_t status)
{
	uint8_t *rpos = NULL;
	uint32_t len = 0;
	uint32_t isr_en = reg->isr_en.value;
	unsigned long spin_flag;

	/* receiving the data even if some error occured */
	/* NOTE: if status[9] & status[2] set simultaneously (32~64 bytes),check status[9] first */
	if (status & SCI_ISR_BIT_RECV_OVER) {
		reg->isr.bits.rcv_over = 1;
		len = reg->isr.bits.data_size;
		gxlog_d(GXSE_LOG_MOD_MISC_SCI, "---- SCI_STATUS_RECEIVE_OVER [%08x %d %d] ----\n", status, p->rdata_num, len);

#ifdef CFG_GXSE_CHIP_TEST
		if (g_sci_flag & SCI_TEST_FLAG_RX_OVERFLOW)
			return GXSE_SUCCESS;
#endif

		if (len == 0 && p->rdata_num == 0)
			return GXSE_SUCCESS;

		/* for COLD/HOT RESET, status[SCI_STATUS_RSTFINISH] will be set meanwhile */
		/* copy data into temp buffer */

		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		rpos = p->rx_buffer + p->rdata_num;
		p->rdata_num += len;
		while (len-- > 0) {
			if (p->rdata_num < SCI_BUFFER_MAX_SIZE)
				*rpos++ = (uint8_t)(reg->data & 0xff);
		}

		/* In normal transmite situation(not include get atr), Some card would ack 0x60 when command is long enougth,
		 * in this case, it should setup a read opearation actively at once. */
		if (p->rx_buffer[0] == 0x60) {
			//改版后的智能卡控制器会一直处于接收模式，不需要重新打开接收模式
			if (reg->ctrl1.bits.rcv == 0)
				reg->ctrl1.bits.rcv = 1;
		} else
			p->read_end |= SCI_READ_VALID;

		/* ONLY when all data received, send signal to wake up pended rx task */
		if (p->rdata_num > 1)
			p->read_end |= SCI_READ_FINISH | SCI_READ_VALID;

		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		gx_select_wake(&p->rd_queue, p->read_end);

	} else if ((status & isr_en & SCI_ISR_BIT_RXFIFO_STATUS)) {
		reg->isr.bits.rxfifo_status = 1;
		len = reg->isr.bits.data_size;
		gxlog_d(GXSE_LOG_MOD_MISC_SCI, "---- SCI_STATUS_RX_FIFO_LEVEL_STATUS [%08x %d] ----\n", status, len);

#ifdef CFG_GXSE_CHIP_TEST
		if (g_sci_flag & SCI_TEST_FLAG_RX_OVERFLOW)
			return GXSE_SUCCESS;
#endif
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		rpos = p->rx_buffer + p->rdata_num;
		while (len-- > 0) {
			if (p->rdata_num < SCI_BUFFER_MAX_SIZE)
				*rpos++ = (uint8_t)(reg->data & 0xff);
			else {
				gxlog_e(GXSE_LOG_MOD_MISC_SCI, "---- sci d!!! fifolen = %d ----\n", p->rdata_num);
				break;
			}
			p->rdata_num++;
		}
		if (p->flags & SCI_FLAG_BOOST_MODE)
			p->read_end = SCI_READ_VALID;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		if (p->flags & SCI_FLAG_BOOST_MODE)
			gx_select_wake(&p->rd_queue, p->read_end);
	}
	return GXSE_SUCCESS;
}

static int32_t _sci_isr_tx_console(volatile SCIReg *reg, struct sci_priv *p, uint32_t status)
{
	unsigned long spin_flag;
	uint8_t *wpos = p->rx_buffer;
	uint32_t len = 0, i = 0;

	if (status & SCI_ISR_BIT_TXFIFO_STATUS) {

		gxlog_d(GXSE_LOG_MOD_MISC_SCI, "---- SCI_STATUS_TX_FIFO_LEVEL_STATUS [%08x] ----\n", status);
		reg->isr.bits.txfifo_status = 1;

		wpos += p->wptr;

		while (1) {
			len = reg->isr.bits.data_size;

			//when request data, and fifo was empty yet,so we know dsr was blocked.
			//mark read finish,ignore this io option!
			if (len == 0) {
				gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
				p->read_end = SCI_READ_VALID | SCI_READ_FINISH;
				gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
				gx_select_wake(&p->rd_queue, p->read_end);
				break;
			}

			len = (len >= p->fifolen) ? 0: (p->fifolen-len);
			len = (p->wdata_num >= len) ? len : p->wdata_num;
			for (i = 0; i < len; i++)
				reg->data = *wpos++;

			gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
			p->wptr     += len;
			p->wdata_num -= len;
			if (p->wdata_num == 0){
				reg->isr_en.bits.txfifo_status = 0;
				reg->isr_en.bits.txfifo_empty = 1;
				gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
				break;
			}
			gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		}

	} else if (status & SCI_ISR_BIT_TXFIFO_EMPTY) {
		gxlog_d(GXSE_LOG_MOD_MISC_SCI, "---- SCI_STATUS_TX_FIFO_EMPTY [%08x] ----\n", status);
		reg->isr.bits.txfifo_empty = 1;
		if (reg->ctrl1.bits.rcv)
			reg->isr_en.bits.rxfifo_status = 1;
		reg->isr_en.bits.txfifo_empty = 0;

		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->wptr = 0;
		p->wdata_num = 0;
		p->write_end = true;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		gx_wake_event(&p->wr_queue, p->write_end);
	}
	return GXSE_SUCCESS;
}

int32_t sci_dsr(GxSeModuleHwObj *obj)
{
	struct sci_priv *p = (struct sci_priv *)obj->priv;
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;
	uint32_t status = reg->isr.value;

	_sci_isr_tx_console(reg, p, status);
	_sci_isr_rx_console(reg, p, status);
	_sci_isr_check_error(reg, p, status);
	_sci_isr_check_inout(reg, p, status);

	return GXSE_SUCCESS;
}

int32_t sci_isr(GxSeModuleHwObj *obj)
{
#ifndef ECOS_OS
	sci_dsr(obj);
#endif
	return GXSE_SUCCESS;
}

int32_t sci_read(GxSeModuleHwObj *obj, uint8_t *kbuf, uint32_t klen)
{
	unsigned long spin_flag;
	unsigned char *buf = kbuf;
	uint32_t  pos = 0, len = 0, timeout = 0;
	struct sci_priv *p = (struct sci_priv *)obj->priv;
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;

	if (NULL == kbuf || klen == 0) {
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "Param is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	// Discrad 0x60
	if (p->rptr == 0) {
		while (p->rx_buffer[pos++] == 0x60)
			p->rptr += 1;
		pos = 0;
	}

redo:
	timeout++;
	if (_sci_check_status(reg, p->card_status) < 0) {
		_sci_fifo_reset(reg, p);
		goto out;
	}

	if (timeout > SCI_RX_TIMEOUT) {
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "rx timeout.\n");
		goto out;
	}

	if (!p->read_end)
		goto redo;

	len = min((p->rdata_num - p->rptr), (klen - pos));
	memcpy(buf + pos, p->rx_buffer + p->rptr, len);
	p->rptr += len;
	pos += len;

	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	if (p->read_end & SCI_READ_FINISH) {
		if (p->rptr == p->rdata_num) {
			p->read_end = false;
			gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
			goto out;
		}
	}
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	if (pos < klen)
		goto redo;

out:
	dbg_dump("read", buf, pos);
	if (p->status != SCI_STATUS_INIT)
		p->status = SCI_STATUS_IDLE;
	return pos;
}

int32_t sci_write(GxSeModuleHwObj *obj, const uint8_t *kbuf, uint32_t klen)
{
	unsigned long spin_flag;
	unsigned char *buf = (void *)kbuf;
	uint32_t len = klen, i;
	struct sci_priv *p = (struct sci_priv *)obj->priv;
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;

	if (NULL == kbuf || klen == 0 || klen > SCI_BUFFER_MAX_SIZE) {
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "Param is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if ((p->card_status & SCI_INTSTATUS_SCIIN) == 0) {
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "No smartcard inserted while resetting.\n");
		return GXSE_ERR_GENERIC;
	}

	dbg_dump("write", (void *)buf, len);
	_sci_fifo_reset(reg, p);

#ifdef CFG_GXSE_CHIP_TEST
	if (g_sci_flag & SCI_TEST_FLAG_RX_OVERFLOW) {
		GxMiscSCIHWOps *ops = (GxMiscSCIHWOps *)GXSE_OBJ_HWOPS(obj);
		if (g_sci_flag & SCI_TEST_FLAG_RX_GATE) {
			if (len == 5) {
				if (ops->set_rxfifo_gate)
					ops->set_rxfifo_gate(obj->reg, p, buf[4] < 32 ? 32 : buf[4] - 1);
			} else {
				if (ops->set_rxfifo_gate)
					ops->set_rxfifo_gate(obj->reg, p, 32);
			}
		}
	}
#endif

	p->status = SCI_STATUS_WRITE;
	reg->isr_en.bits.txfifo_empty = 1;
	reg->isr_en.bits.rxfifo_status = 0;
	if (len > p->fifolen) {
		reg->isr_en.bits.txfifo_status = 1;
		p->wdata_num = len - p->fifolen;
		memset(p->rx_buffer, 0, SCI_BUFFER_MAX_SIZE);
		memcpy(p->rx_buffer, buf + p->fifolen, p->wdata_num);
		len = p->fifolen;
	}

	for (i = 0; i < len; i++)
		reg->data = *buf++;

	reg->ctrl1.bits.TE = 1;

	/* wait until tx finished (signaled via INT) */
	if (gx_wait_event(&p->wr_queue, p->write_end, SCI_EVENT_RW_MASK) == 0) {
		gx_mask_event(&p->wr_queue, SCI_EVENT_RW_MASK);
	}

	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	p->write_end = false;
	p->status = SCI_STATUS_IDLE;
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);

	_sci_check_status(reg, p->card_status);

	return 0;
}

int32_t sci_set_param(GxSeModuleHwObj *obj, GxSciParam *param)
{
	struct sci_priv *p = (struct sci_priv *)obj->priv;
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;
	GxMiscSCIHWOps *ops = (GxMiscSCIHWOps *)GXSE_OBJ_HWOPS(obj);

	if (param->flags & SCI_FLAG_PARAM_RPT) {
		reg->ctrl3.bits.repeat_time = 1;
		reg->ctrl2.bits.repeat_dis = param->repeat;
	}

	if (param->flags & SCI_FLAG_PARAM_STL)
		reg->ctrl2.bits.stoplen = param->stoplen;

	if (param->flags & SCI_FLAG_PARAM_PAR) {
		if (param->flags & SCI_FLAG_AUTO_PARITY)
			reg->ctrl2.bits.auto_parity = 0;
		else
			reg->ctrl2.bits.auto_parity = 1;
	}

	if (param->flags & SCI_FLAG_PARAM_CON) {
		reg->ctrl2.bits.io_conv = param->convention;
		if (reg->ctrl2.bits.auto_parity)
			reg->ctrl2.bits.parity = param->convention;

#ifdef CFG_GXSE_CHIP_TEST
		if (g_sci_flag & SCI_TEST_FLAG_ERROR_PARITY) {
			reg->ctrl2.bits.io_conv = param->convention ? 0 : 1;
			reg->ctrl2.bits.parity = reg->ctrl2.bits.io_conv;
			g_sci_flag &= ~(SCI_TEST_FLAG_ERROR_PARITY);
		}
#endif
	}

	if (param->flags & SCI_FLAG_PARAM_VCC)
		reg->ctrl2.bits.vcc_pol = param->vcc ? 0 : 1;

	if (param->flags & SCI_FLAG_PARAM_DET)
		reg->ctrl2.bits.detect_pol = param->detect ? 0 : 1;

	if (param->flags & SCI_FLAG_PARAM_CLK) {
		if (((p->sys_clk * 10/2/(param->clock))%10) >= 5)
			reg->ctrl3.bits.clkdiv = (p->sys_clk/2/param->clock);
		else
			reg->ctrl3.bits.clkdiv = (p->sys_clk/2/param->clock - 1);
	}

	if (param->flags & SCI_FLAG_PARAM_ETU) {
		if (param->flags & SCI_FLAG_AUTO_ETU)
			reg->ctrl2.bits.auto_ETU = 0;
		else {
			reg->ctrl3.bits.ETU = param->ETU ? param->ETU: SCI_DEFAULT_ETU;
			reg->ctrl2.bits.auto_ETU = 1;
		}
	}

	if (p->status == SCI_STATUS_INIT) {
		if (param->flags & SCI_FLAG_BOOST_MODE) {
			if (ops->set_rxfifo_gate)
				ops->set_rxfifo_gate(obj->reg, p, 1);
			p->flags |= SCI_FLAG_BOOST_MODE;
			p->status = SCI_STATUS_IDLE;
		} else {
			if (ops->set_rxfifo_gate)
				ops->set_rxfifo_gate(obj->reg, p, 32);
			p->flags &= ~SCI_FLAG_BOOST_MODE;
		}
	}

	if (param->flags & SCI_FLAG_PARAM_EGT)
		reg->EGT = param->EGT;

	if (param->flags & SCI_FLAG_PARAM_TGT)
		reg->TGT = param->TGT;

	if (param->flags & SCI_FLAG_PARAM_WDT)
		reg->WDT = param->WDT;

	if (param->flags & SCI_FLAG_PARAM_TWDT) {
		reg->TWDT = param->TWDT;

#ifdef CFG_GXSE_CHIP_TEST
		if (g_sci_flag & SCI_TEST_FLAG_RX_OVERTIME)
			reg->TWDT = reg->WDT * 2;
#endif
	}

	return GXSE_SUCCESS;
}

int32_t sci_get_param(GxSeModuleHwObj *obj, GxSciParam *param)
{
	struct sci_priv *p = (struct sci_priv *)obj->priv;
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;

	if (param->flags & SCI_FLAG_PARAM_RPT)
		param->repeat = reg->ctrl2.bits.repeat_dis;

	if (param->flags & SCI_FLAG_PARAM_STL)
		param->stoplen = reg->ctrl2.bits.stoplen;

	if (param->flags & SCI_FLAG_PARAM_CON)
		param->convention = reg->ctrl2.bits.io_conv;

	if (param->flags & SCI_FLAG_PARAM_PAR) {
		param->parity = reg->ctrl2.bits.parity;
		if (reg->ctrl2.bits.auto_parity)
			param->flags |= SCI_FLAG_AUTO_PARITY;
	}

	if (param->flags & SCI_FLAG_PARAM_VCC)
		param->vcc = reg->ctrl2.bits.vcc_pol ? GXSCI_LOW_LEVEL : GXSCI_HIGH_LEVEL;

	if (param->flags & SCI_FLAG_PARAM_DET)
		param->detect = reg->ctrl2.bits.detect_pol ? GXSCI_LOW_LEVEL : GXSCI_HIGH_LEVEL;

	if (param->flags & SCI_FLAG_PARAM_CLK)
		param->clock = p->sys_clk/2/(reg->ctrl3.bits.clkdiv+1);

	if (param->flags & SCI_FLAG_PARAM_ETU) {
		if (reg->ctrl2.bits.auto_ETU)
			param->flags |= SCI_FLAG_AUTO_ETU;
		param->ETU = reg->ctrl3.bits.ETU;
	}

	if (param->flags & SCI_FLAG_PARAM_EGT)
		param->EGT = reg->EGT;

	if (param->flags & SCI_FLAG_PARAM_TGT)
		param->TGT = reg->TGT;

	if (param->flags & SCI_FLAG_PARAM_WDT)
		param->WDT = reg->WDT;

	if (param->flags & SCI_FLAG_PARAM_TWDT)
		param->TWDT = reg->TWDT;

	return GXSE_SUCCESS;
}

int32_t sci_print_reg(GxSeModuleHwObj *obj)
{
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;
	return _sci_printf_regs(reg);
}

int32_t sci_ICC_reset(GxSeModuleHwObj *obj)
{
	struct sci_priv *p = (struct sci_priv *)obj->priv;
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;

	if ((p->card_status & SCI_INTSTATUS_SCIIN) == 0) {
		gxlog_e(GXSE_LOG_MOD_MISC_SCI, "No smartcard inserted while resetting.\n");
		return GXSE_ERR_GENERIC;
	}

	_sci_fifo_reset(reg, p);
	p->status = SCI_STATUS_INIT;
	reg->ctrl1.bits.cldrst = 1; // cold reset | hot reset
	while (!reg->isr.bits.reset_finish);

	return GXSE_SUCCESS;
}

int32_t sci_ICC_poweroff(GxSeModuleHwObj *obj)
{
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;

	reg->ctrl2.bits.vcc_en = 0;

	return GXSE_SUCCESS;
}

int32_t sci_get_status(GxSeModuleHwObj *obj, GxSciCardStatus *status)
{
	struct sci_priv *p = (struct sci_priv *)obj->priv;

	*status = GXSCI_CARD_OUT;
	if (p->card_status & SCI_INTSTATUS_SCIIN)
		*status = GXSCI_CARD_IN;

	return GXSE_SUCCESS;
}

int32_t sci_init(GxSeModuleHwObj *obj)
{
	struct sci_priv *p = (struct sci_priv *)obj->priv;
	GxMiscSCIHWOps *ops = (GxMiscSCIHWOps *)GXSE_OBJ_HWOPS(obj);

	memset(p, 0, sizeof(struct sci_priv));

#ifdef CFG_GXSE_CHIP_TEST
	gxse_chiptest_sci_init((uint32_t)obj->reg);
#endif

	if (ops->init)
		ops->init(obj->reg, p);

	if (ops->set_clk_fb)
		ops->set_clk_fb(obj->reg, p, 1);

	if (ops->set_rxfifo_gate)
		ops->set_rxfifo_gate(obj->reg, p, 32);

	if (ops->set_txfifo_gate)
		ops->set_txfifo_gate(obj->reg, p, 32);

	gx_spin_lock_init(&p->spin_lock);
	gx_select_init(&p->rd_queue);
	gx_event_init(&p->wr_queue);
	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;

	return GXSE_SUCCESS;
}

int32_t sci_open(GxSeModuleHwObj *obj)
{
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;
	reg->isr_en.value = 0xb67;
	return GXSE_SUCCESS;
}

int32_t sci_poll(GxSeModuleHwObj *obj, int32_t which, void **r_wait, void **w_wait)
{
	unsigned long spin_flag;
	int32_t ret = GXSE_POLL_NONE;
	struct sci_priv *p = (struct sci_priv *)obj->priv;
	volatile SCIReg *reg = (volatile SCIReg *)obj->reg;

	if (which & GXSE_POLL_R) {
		if (r_wait)
			*r_wait = &p->rd_queue;

		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		if (p->status != SCI_STATUS_INIT)
			p->status = SCI_STATUS_READ;
		if (p->read_end || ((p->card_status & SCI_INTSTATUS_SCIIN) == 0) || reg->isr.bits.card_in == 0)
			ret |= GXSE_POLL_R;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	}
	if (which & GXSE_POLL_W)
		ret |= GXSE_POLL_W;
	return ret;
}

int32_t sci_setup(GxSeModuleHwObj *obj, GxSeModuleResource *res)
{
	struct sci_priv *p = (struct sci_priv *)obj->priv;

	p->sys_clk = res->clk;

	return GXSE_SUCCESS;
}
