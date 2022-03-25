#include "gxdemux.h"
#include "gx3211_regs.h"
#include "taurus_regs.h"
#include "sirius_regs.h"
#include "sirius_pfilter.h"
#include "taurus_pfilter.h"
#include "profile.h"
#include "gxav_bitops.h"

static struct dvr_pfilter taurus_pfilter[TAURUS_MAX_PFILTER_UNIT];

void taurus_pfilter_pid_buf_init(struct dvr_pfilter *pfilter)
{
	volatile struct taurus_reg_pid_filter *reg = (volatile struct taurus_reg_pid_filter *)pfilter->reg;
	if (!pfilter->pid_buf)
		return;

	reg->pid_buf_addr = gx_virt_to_phys(pfilter->pid_buf);
	reg->pid_buf_len  = BUFFER_PID_BUF_SIZE;
	REG_SET_BIT(&reg->ctrl0, BIT_TAURUS_PFILTER_CTRL0_PID_BUF_EN);
}

void taurus_pfilter_pid_buf_deinit(struct dvr_pfilter *pfilter)
{
	volatile struct taurus_reg_pid_filter *reg = (volatile struct taurus_reg_pid_filter *)pfilter->reg;
	reg->pid_buf_addr = 0;
	reg->pid_buf_len  = 0;
	REG_CLR_BIT(&reg->ctrl0, BIT_TAURUS_PFILTER_CTRL0_PID_BUF_EN);
}

void taurus_pfilter_pid_buf_add_pid(struct dvr_pfilter *pfilter, int pid_handle, int pid)
{
	unsigned short *pid_buf = (unsigned short *)pfilter->pid_buf;
	pid_buf[pid_handle] = pid;
	gx_dcache_clean_range(0, 0);
}

void taurus_pfilter_pid_buf_del_pid(struct dvr_pfilter *pfilter, int pid_handle)
{
	unsigned short *pid_buf = (unsigned short *)pfilter->pid_buf;
	pid_buf[pid_handle] = 0;
	gx_dcache_clean_range(0, 0);
}

void taurus_pfilter_set_ctrl(struct dvr_pfilter *pfilter)
{
	int mode;
	volatile struct taurus_reg_pid_filter *reg = (volatile struct taurus_reg_pid_filter *)pfilter->reg;

	if (reg->pid_en_l && pfilter->mode != DVR_RECORD_BY_REVERSE_SLOT)
		mode = 0;
	else
		mode = pfilter->mode+1;
	REG_SET_FIELD(&reg->ctrl0, MSK_TAURUS_PFILTER_CTRL0_STORE_MODE, mode, BIT_TAURUS_PFILTER_CTRL0_STORE_MODE);
	REG_SET_FIELD(&reg->ctrl0, MSK_PFILTER_CTRL0_TS_SEL, pfilter->source, BIT_PFILTER_CTRL0_TS_SEL);
	REG_SET_FIELD(&reg->ctrl0, MSK_TAURUS_PFILTER_CTRL0_INPUT_MODE, pfilter->input_mode, BIT_TAURUS_PFILTER_CTRL0_INPUT_MODE);
	REG_SET_FIELD(&reg->ctrl0, MSK_TAURUS_PFILTER_CTRL0_NOSYNC, pfilter->nosync_en, BIT_TAURUS_PFILTER_CTRL0_NOSYNC);
	REG_SET_FIELD(&reg->ctrl0, MSK_TAURUS_PFILTER_CTRL0_NOVALID, pfilter->novalid_en, BIT_TAURUS_PFILTER_CTRL0_NOVALID);
//ts des en
//	REG_SET_FIELD(&reg->ctrl0, (0x1<<4), 1, 4);
}

static struct dvr_pfilter *taurus_pfilter_get(int id)
{
	return (id < TAURUS_MAX_PFILTER_UNIT) ? &taurus_pfilter[id] : NULL;
}

struct demux_pfilter_ops taurus_pfilter_ops = {
	.open            = sirius_pfilter_open,
	.close           = sirius_pfilter_close,
	.start           = sirius_pfilter_start,
	.stop            = sirius_pfilter_stop,
	.set_ctrl        = taurus_pfilter_set_ctrl,
	.set_gate        = sirius_pfilter_set_gate,
	.add_pid         = sirius_pfilter_add_pid,
	.del_pid         = sirius_pfilter_del_pid,
	.pid_buf_init    = taurus_pfilter_pid_buf_init,
	.pid_buf_deinit  = taurus_pfilter_pid_buf_deinit,
	.pid_buf_add_pid = taurus_pfilter_pid_buf_add_pid,
	.pid_buf_del_pid = taurus_pfilter_pid_buf_del_pid,
	.irq_entry       = sirius_pfilter_irq_entry,
	.tsw_isr         = sirius_pfilter_dealwith,

	.get_pfilter     = taurus_pfilter_get,
};

int taurus_pfilter_init(void)
{
	int i;
	struct dmxdev *dev = NULL;
	int addr = TAURUS_PFILTER_BASE_ADDR;

	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->pfilter_ops     = &taurus_pfilter_ops;
		dev->max_pfilter     = TAURUS_MAX_PFILTER_UNIT;
		dev->max_pfilter_pid = TAURUS_MAX_PFILTER_PID;
	}

	gx_memset(&taurus_pfilter[0], 0, sizeof(struct dvr_pfilter));
	taurus_pfilter[0].id = 0;
	taurus_pfilter[0].max_pid_num = TAURUS_MAX_PFILTER_PID;
	taurus_pfilter[0].pid_mask = 0;
	taurus_pfilter[0].base_addr = addr;
	taurus_pfilter[0].cfg_done = 1;
	taurus_pfilter[0].pid_buf = (unsigned int)gx_page_malloc(BUFFER_PID_BUF_SIZE);
	taurus_pfilter[0].pid_buf_max_unit = BUFFER_PID_BUF_SIZE/2;
	taurus_pfilter[0].pid_buf_mask = 0;
	gx_memset((void *)taurus_pfilter[0].pid_buf, 0, BUFFER_PID_BUF_SIZE);
	gx_dcache_clean_range(0,0);

	if (!gx_request_mem_region(addr, sizeof(struct taurus_reg_pid_filter))) {
		gxlog_e(LOG_DEMUX, "request pfilter mem region failed!\n");
		return -1;
	}
	taurus_pfilter[0].reg = gx_ioremap(addr, sizeof(struct taurus_reg_pid_filter));
	if (!taurus_pfilter[0].reg) {
		gxlog_e(LOG_DEMUX, "ioremap pfilter space failed!\n");
		return -1;
	}
	return dvr_pfilter_init();
}

int taurus_pfilter_deinit(void)
{
	int i;
	struct dmxdev *dev = NULL;

	dvr_pfilter_deinit();
	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->pfilter_ops     = NULL;
		dev->max_pfilter     = 0;
		dev->max_pfilter_pid = 0;
	}
	taurus_pfilter_ops.pid_buf_deinit(&taurus_pfilter[0]);
	gx_page_free((void *)taurus_pfilter[0].pid_buf, BUFFER_PID_BUF_SIZE);
	taurus_pfilter[0].pid_buf = 0;
	taurus_pfilter[0].pid_buf_max_unit = 0;
	taurus_pfilter[0].pid_buf_mask = 0;

	gx_iounmap(taurus_pfilter[0].reg);
	gx_release_mem_region(taurus_pfilter[0].base_addr, sizeof(struct sirius_reg_pid_filter));
	return 0;
}
