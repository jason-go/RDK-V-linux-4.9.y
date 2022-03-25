#include "kernelcalls.h"
#include "taurus_regs.h"
#include "sirius_demux.h"
#include "gxav_bitops.h"
#include "gxdemux.h"

struct demux_gse_ops taurus_gse_ops = {
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

int taurus_gse_init(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	if (!gx_request_mem_region(TAURUS_GSE_BASE_ADDR, sizeof(struct sirius_reg_gse))) {
		gxlog_e(LOG_DEMUX, "request gse mem region failed!\n");
		return -1;
	}

	dev->gse_reg = gx_ioremap(TAURUS_GSE_BASE_ADDR, sizeof(struct sirius_reg_gse));
	if (!dev->gse_reg) {
		gxlog_e(LOG_DEMUX, "ioremap gse space failed!\n");
		return -1;
	}
	dev->gse_ops = &taurus_gse_ops;
	dev->max_gse = MAX_GSE_UNIT;
	dev->max_gse_slot = MAX_GSE_SLOT;
	dev->gse_work_mode = GSE_MODE_IDLE;
	dev->thread_slot_finish = 0xffffffff;
	gx_spin_lock_init(&(dev->gse_spin_lock));

	return 0;
}

int taurus_gse_cleanup(struct gxav_device *device, struct gxav_module_inode *inode)
{
	struct dmxdev *dev = &gxdmxdev[0];
	dev->gse_ops = NULL;
	dev->max_gse = 0;
	dev->max_gse_slot = 0;
	gx_iounmap(dev->gse_reg);
	gx_release_mem_region(TAURUS_GSE_BASE_ADDR, sizeof(struct sirius_reg_gse));

	return 0;
}
