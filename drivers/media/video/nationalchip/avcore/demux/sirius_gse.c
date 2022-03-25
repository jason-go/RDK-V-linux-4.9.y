#include "kernelcalls.h"
#include "sirius_regs.h"
#include "sirius_demux.h"
#include "gxav_bitops.h"
#include "gxdemux.h"

void sirius_gse_clr_slot_cfg(void *vreg, unsigned int slotid)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;

	REG_SET_VAL(&(reg->almost_full_gate[slotid]), 0);
	REG_SET_VAL(&(reg->slots[slotid].protocol_type), 0);
	REG_SET_VAL(&(reg->slots[slotid].label_type), 0);

	REG_SET_VAL(&(reg->filters.data_buf[slotid].buf_len), 0);
	REG_SET_VAL(&(reg->filters.data_buf[slotid].start_addr), 0);
	REG_SET_VAL(&(reg->filters.data_buf[slotid].phy_write_ptr), 0);
	REG_SET_VAL(&(reg->filters.data_buf[slotid].logic_write_ptr), 0);
	REG_SET_VAL(&(reg->filters.data_buf[slotid].logic_read_ptr), 0);

	REG_SET_VAL(&(reg->filters.status_buf[slotid].buf_len), 0);
	REG_SET_VAL(&(reg->filters.status_buf[slotid].start_addr), 0);
	REG_SET_VAL(&(reg->filters.status_buf[slotid].phy_write_ptr), 0);
	REG_SET_VAL(&(reg->filters.status_buf[slotid].phy_read_ptr), 0);
}

void sirius_gse_reg_init(void *vreg)
{
	int i;
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;

	REG_SET_VAL(&(reg->slot_en), 0);
	REG_SET_VAL(&(reg->int_pdu_finish_en), 0);
	REG_SET_VAL(&(reg->int_almost_full_en), 0);
	REG_CLR_BIT(&(reg->gse_ctrl), BIT_GSE_PLAY_RECORD);

	for (i = 0; i < MAX_GSE_SLOT; i++)
		sirius_gse_clr_slot_cfg(vreg, i);
}

void sirius_gse_set_sync_gate(void *vreg)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;
	REG_SET_FIELD(&(reg->gse_ctrl), GSE_LOSS_GATE, 0x3, BIT_GSE_LOSS_GATE);
	REG_SET_FIELD(&(reg->gse_ctrl), GSE_LOCK_GATE, 0x3, BIT_GSE_LOCK_GATE);
	REG_SET_FIELD(&(reg->int_over_time), GSE_OVERTIME_GATE, 1000, BIT_GSE_OVERTIME_GATE);
}

void sirius_gse_set_input_source(void *vreg, unsigned int source)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;
	REG_SET_FIELD(&(reg->gse_ctrl), GSE_GS_IN_SEL, source, BIT_GSE_GS_IN_SEL);
}

void sirius_gse_set_stream_mode(void *vreg, unsigned int mode)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;
	REG_CLR_BIT(&(reg->gse_ctrl), BIT_GSE_SERIAL_IN);
	REG_CLR_BIT(&(reg->gse_ctrl), BIT_GSE_SERIAL_2_4_BIT);
	REG_SET_BIT(&(reg->gse_ctrl), BIT_GSE_BIG_ENDIAN);
	REG_SET_BIT(&(reg->gse_ctrl), BIT_GSE_CLOCK_MOD);
	if (mode & 0x1)
		REG_SET_BIT(&(reg->gse_ctrl), BIT_GSE_SERIAL_IN);
	if (mode & 0x1 << 1)
		REG_SET_BIT(&(reg->gse_ctrl), BIT_GSE_SERIAL_2_4_BIT);
}

int sirius_gse_query_tslock(void *vreg)
{
	unsigned int status;
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;

	status = REG_GET_VAL(&(reg->int_over_time));
	if (status & GSE_LOCK_INT)
		return TS_SYNC_LOCKED;

	return TS_SYNC_UNLOCKED;
}

void sirius_gse_enable_slot(void *vreg, unsigned int slotid, int work_mode, int mode)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;

	REG_SET_BIT(&(reg->slot_buf_full_stop), slotid);
	REG_SET_BIT(&(reg->int_buf_full_en), slotid);
	REG_SET_BIT(&(reg->int_almost_full_en), slotid);
	REG_CLR_BIT(&(reg->gse_ctrl), BIT_GSE_WORK_EN);
	REG_SET_BIT(&(reg->gse_ctrl), BIT_GSE_WORK_EN);

	if (work_mode == GSE_MODE_PDU) {
		REG_SET_BIT(&(reg->int_pdu_finish_en), slotid);
		REG_SET_BIT(&(reg->int_slot_close_finish_en), slotid);
		REG_SET_BIT(&(reg->slot_en), slotid);
	} else {
		REG_SET_BIT(&(reg->int_over_time), BIT_GSE_RECORD_FINISH_EN);
		REG_SET_FIELD(&(reg->gse_ctrl), GSE_RECORD_BUF_SEL, slotid, BIT_GSE_RECORD_BUF_SEL);
		if (mode == DVR_RECORD_ALL)
			REG_SET_BIT(&(reg->gse_ctrl), BIT_GSE_RECORD_MODE);
		else
			REG_CLR_BIT(&(reg->gse_ctrl), BIT_GSE_RECORD_MODE);
		REG_SET_BIT(&(reg->gse_ctrl), BIT_GSE_PLAY_RECORD);
	}
}

void sirius_gse_disable_slot(void *vreg, unsigned int slotid, int work_mode)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;

	if (work_mode == GSE_MODE_PDU) {
		REG_CLR_BIT(&(reg->int_pdu_finish_en), slotid);
		REG_CLR_BIT(&(reg->slot_en), slotid);
	} else {
		REG_CLR_BIT(&(reg->gse_ctrl), BIT_GSE_PLAY_RECORD);
		REG_CLR_BIT(&(reg->gse_ctrl), BIT_GSE_RECORD_MODE);
		REG_SET_FIELD(&(reg->gse_ctrl), GSE_RECORD_BUF_SEL, 0, BIT_GSE_RECORD_BUF_SEL);
	}

	REG_CLR_BIT(&(reg->int_buf_full_en), slotid);
	REG_CLR_BIT(&(reg->slot_buf_full_stop), slotid);
	REG_CLR_BIT(&(reg->int_almost_full_en), slotid);
	REG_CLR_BIT(&(reg->int_over_time), BIT_GSE_RECORD_FINISH_EN);
}

void sirius_gse_wait_finish(void *vreg, unsigned int slotid, volatile unsigned int *slot_finish)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse*)vreg;

	REG_SET_FIELD(&(reg->int_over_time), GSE_OVERTIME_GATE, 10, BIT_GSE_OVERTIME_GATE);
	while (!(*slot_finish & 0x1<<slotid)) {
		gxlog_d(LOG_DEMUX, "gse wait finish!\n");
		gx_msleep(100);
	}
	REG_SET_FIELD(&(reg->int_over_time), GSE_OVERTIME_GATE, 1000, BIT_GSE_OVERTIME_GATE);
	REG_CLR_BIT(&(reg->int_slot_close_finish_en), slotid);
}

void sirius_gse_set_status_rptr(void *vreg, unsigned int slotid, unsigned int rptr)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse *)vreg;
	REG_SET_VAL(&(reg->filters.status_buf[slotid].phy_read_ptr), rptr);
}

unsigned int sirius_gse_get_status_rptr(void *vreg, unsigned int slotid)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse *)vreg;
	return REG_GET_VAL(&(reg->filters.status_buf[slotid].phy_read_ptr));
}

unsigned int sirius_gse_get_status_wptr(void *vreg, unsigned int slotid)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse *)vreg;
	return REG_GET_VAL(&(reg->filters.status_buf[slotid].phy_write_ptr));
}

void sirius_gse_set_addr(void *vreg, struct dmx_gse_slot *slot)
{
	int id = slot->id;
	struct sirius_reg_gse *reg = (struct sirius_reg_gse *)vreg;

	REG_SET_VAL(&(reg->filters.data_buf[id].buf_len), slot->hw_buffer_size);
	REG_SET_VAL(&(reg->filters.data_buf[id].start_addr), slot->paddr_data);
	REG_SET_VAL(&(reg->filters.data_buf[id].phy_write_ptr), 0);
	REG_SET_VAL(&(reg->filters.data_buf[id].logic_write_ptr), 0);
	REG_SET_VAL(&(reg->filters.data_buf[id].logic_read_ptr), 0);
	REG_SET_VAL(&(reg->almost_full_gate[id]), slot->almost_full_gate);

	REG_SET_VAL(&(reg->filters.status_buf[id].buf_len), BUFFER_GSE_STATUS_SIZE);
	REG_SET_VAL(&(reg->filters.status_buf[id].start_addr), slot->paddr_status);
	REG_SET_VAL(&(reg->filters.status_buf[id].phy_write_ptr), 0);
	REG_SET_VAL(&(reg->filters.status_buf[id].phy_read_ptr), 0);
}

void sirius_gse_all_pass_en(void *vreg, unsigned int slotid)
{
	struct sirius_reg_gse *reg = (struct sirius_reg_gse *)vreg;

	REG_SET_FIELD(&(reg->slots[slotid].protocol_type), GSE_PROTOCOL_TYPE_MASK, 0xffff, BIT_GSE_PROTOCOL_TYPE_MASK);
	REG_SET_FIELD(&(reg->slots[slotid].label_type), GSE_LABEL_TYPE_MASK, 0x3, BIT_GSE_LABEL_TYPE_MASK);
	REG_SET_FIELD(&(reg->slots[slotid].label_mask.label_mask_h), GSE_LABEL_MASK_H, 0xffff, BIT_GSE_LABEL_MASK_H);
	REG_SET_FIELD(&(reg->slots[slotid].label_mask.label_mask_l), GSE_LABEL_MASK_L, 0xffffffff, BIT_GSE_LABEL_MASK_L);
}

void sirius_gse_set_label(void *vreg, struct dmx_gse_slot *slot)
{
	int id = slot->id;
	struct sirius_reg_gse *reg = (struct sirius_reg_gse *)vreg;
	unsigned int label_mask_h = 0, label_mask_l = 0;
	unsigned int label_h = slot->label.h & GSE_LABEL_H;
	unsigned int label_l = slot->label.l;

	if (slot->label_type == GSE_LABEL_NOT_USED) {
		REG_SET_FIELD(&(reg->slots[id].label_type), GSE_LABEL_TYPE, 0, BIT_GSE_LABEL_TYPE);
		REG_SET_FIELD(&(reg->slots[id].label_type), GSE_LABEL_TYPE_MASK, 0x3, BIT_GSE_LABEL_TYPE_MASK);
		return;
	}

	REG_SET_FIELD(&(reg->slots[id].label_type), GSE_LABEL_TYPE, slot->label_type, BIT_GSE_LABEL_TYPE);
	REG_SET_FIELD(&(reg->slots[id].label_type), GSE_LABEL_TYPE_MASK, 0, BIT_GSE_LABEL_TYPE_MASK);

	REG_SET_FIELD(&(reg->slots[id].label_val.label_h), GSE_LABEL_H, label_h, BIT_GSE_LABEL_H);
	REG_SET_FIELD(&(reg->slots[id].label_val.label_l), GSE_LABEL_L, label_l, BIT_GSE_LABEL_L);
	if (!label_h && !label_l) {
		label_mask_h = 0xffff;
		label_mask_l = 0xffffffff;
	}
	REG_SET_FIELD(&(reg->slots[id].label_mask.label_mask_h), GSE_LABEL_MASK_H, label_mask_h, BIT_GSE_LABEL_MASK_H);
	REG_SET_FIELD(&(reg->slots[id].label_mask.label_mask_l), GSE_LABEL_MASK_L, label_mask_l, BIT_GSE_LABEL_MASK_L);
}

void sirius_gse_set_protocol(void *vreg, struct dmx_gse_slot *slot)
{
	int id = slot->id;
	unsigned int mask = 0;
	struct sirius_reg_gse *reg = (struct sirius_reg_gse *)vreg;

	if (!slot->protocol)
		mask = 0xffff;
	REG_SET_FIELD(&(reg->slots[id].protocol_type), GSE_PROTOCOL_TYPE, slot->protocol, BIT_GSE_PROTOCOL_TYPE);
	REG_SET_FIELD(&(reg->slots[id].protocol_type), GSE_PROTOCOL_TYPE_MASK, mask, BIT_GSE_PROTOCOL_TYPE_MASK);
}

struct demux_gse_ops sirius_gse_ops = {
	.reg_init          = sirius_gse_reg_init,
	.set_sync_gate     = sirius_gse_set_sync_gate,
	.set_input_source  = sirius_gse_set_input_source,
	.set_stream_mode   = sirius_gse_set_stream_mode,
	.query_tslock      = sirius_gse_query_tslock,

	.enable_slot       = sirius_gse_enable_slot,
	.disable_slot      = sirius_gse_disable_slot,
	.wait_finish       = sirius_gse_wait_finish,
	.clr_slot_cfg      = sirius_gse_clr_slot_cfg,
	.set_addr          = sirius_gse_set_addr,
	.all_pass_en       = sirius_gse_all_pass_en,
	.set_label         = sirius_gse_set_label,
	.set_protocol      = sirius_gse_set_protocol,

	.set_status_rptr   = sirius_gse_set_status_rptr,
	.get_status_rptr   = sirius_gse_get_status_rptr,
	.get_status_wptr   = sirius_gse_get_status_wptr,

	.full_isr          = sirius_gse_full_isr,
	.slot_close_isr    = sirius_gse_slot_finish_isr,
	.record_finish_isr = sirius_gse_record_finish_isr,
	.al_isr            = sirius_gse_almost_full_isr,
	.pdu_isr           = sirius_gse_pdu_isr,
	.al_isr_en         = sirius_gse_pdu_isr_en,
	.pdu_isr_en        = sirius_gse_al_isr_en,
};

struct sirius_reg_gse *gse_reg;
int sirius_gse_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = NULL;
	unsigned int i=0, addr = SIRIUS_DEMUX_BASE_GSE;

	if (!gx_request_mem_region(addr, sizeof(struct sirius_reg_gse))) {
		gxlog_e(LOG_DEMUX, "request gse mem region failed!\n");
		return -1;
	}

	gse_reg = gx_ioremap(addr, sizeof(struct sirius_reg_gse));
	if (!gse_reg) {
		gxlog_e(LOG_DEMUX, "ioremap gse space failed!\n");
		return -1;
	}

	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->gse_reg = gse_reg;
		dev->gse_ops = &sirius_gse_ops;
		dev->max_gse = MAX_GSE_UNIT;
		dev->max_gse_slot = MAX_GSE_SLOT;
		dev->thread_slot_finish = 0xffffffff;
		dev->gse_work_mode = GSE_MODE_IDLE;
		gx_spin_lock_init(&(dev->gse_spin_lock));
	}

	return 0;
}

int sirius_gse_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	unsigned int i=0;
	struct dmxdev *dev = NULL;

	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->gse_ops = NULL;
		dev->max_gse = 0;
		dev->max_gse_slot = 0;
	}
	gx_iounmap(gse_reg);
	gx_release_mem_region(SIRIUS_DEMUX_BASE_GSE, sizeof(struct sirius_reg_gse));

	return 0;
}
