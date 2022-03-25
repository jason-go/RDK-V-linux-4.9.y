#include "gxdemux.h"
#include "gx3211_regs.h"
#include "taurus_regs.h"
#include "sirius_regs.h"
#include "sirius_pfilter.h"
#include "profile.h"
#include "gxav_bitops.h"

static struct dvr_pfilter sirius_pfilter[MAX_PFILTER_UNIT];

static void sirius_pfilter_cfg_done(struct dvr_pfilter *pfilter)
{
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
	REG_SET_BIT(&reg->ctrl1, BIT_PFILTER_CTRL1_OUTPUT_EN);
	REG_CLR_BIT(&reg->ctrl1, BIT_PFILTER_CTRL1_CFG_REQ);
	pfilter->cfg_done = 1;
}

static void sirius_pfilter_cfg_req(struct dvr_pfilter *pfilter)
{
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
	pfilter->cfg_done = 0;
	REG_CLR_BIT(&reg->ctrl1, BIT_PFILTER_ISR_ALLOW_CFG);
	REG_CLR_BIT(&reg->ctrl1, BIT_PFILTER_CTRL1_OUTPUT_EN);
	REG_CLR_BIT(&reg->ctrl0, BIT_PFILTER_CTRL0_SYNC_EN);
	REG_SET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_ALLOW_CFG);
	REG_SET_BIT(&reg->ctrl1, BIT_PFILTER_CTRL1_CFG_REQ);
	while(!REG_GET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_ALLOW_CFG))
		gx_msleep(5);

	REG_SET_BIT(&reg->ctrl0, BIT_PFILTER_CTRL0_SYNC_EN);

	// TODO FPGA need clk reverse
	// REG_SET_BIT(&reg->ctrl1, BIT_PFILTER_CTRL1_CLK_REVERSE);
	/* set buf gate to avoid unexpected interrupt */
	REG_SET_VAL(&reg->buf_full_gate, 0xffffffff);
	REG_SET_VAL(&reg->buf_almost_full_gate, 0xffffffff);
	REG_SET_VAL(&reg->buf_rptr, 0x0);
	REG_SET_VAL(&reg->buf_wptr_logic, 0);
	REG_SET_BIT(&reg->pfilter_int, BIT_PFILTER_ISR_ALLOW_CFG);
}

int sirius_pfilter_open(struct dvr_pfilter *pfilter)
{
#define BUFFER_LIST_SIZE        0x100
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
	pfilter->list_start_addr = (unsigned int)gx_page_malloc(BUFFER_LIST_SIZE);
	gx_memset((void*)pfilter->list_start_addr, 0, BUFFER_LIST_SIZE);
	gx_dcache_clean_range(pfilter->list_start_addr, pfilter->list_start_addr+BUFFER_LIST_SIZE);

	reg->buf_header = gx_virt_to_phys(pfilter->list_start_addr);
	REG_SET_FIELD(&reg->ctrl0, MSK_PFILTER_CTRL0_SYNC_GATE, 0x3, BIT_PFILTER_CTRL0_SYNC_GATE);
	REG_SET_FIELD(&reg->ctrl0, MSK_PFILTER_CTRL0_LOSS_GATE, 0x3, BIT_PFILTER_CTRL0_LOSS_GATE);
	REG_SET_BIT(&reg->ctrl0, BIT_PFILTER_CTRL0_SYNC_EN);
	reg->cfg_req_timeout_gate = 0xffff;
	reg->stream_timeout_gate  = 0xffffff;
	REG_SET_FIELD(&reg->ctrl1, MSK_PFILTER_CTRL1_INT_EN, 0x3b, BIT_PFILTER_CTRL1_INT_EN);

	return 0;
}

int sirius_pfilter_close(struct dvr_pfilter *pfilter)
{
	sirius_pfilter_cfg_req(pfilter);
	gx_page_free((unsigned char *)pfilter->list_start_addr, BUFFER_LIST_SIZE);
	return 0;
}

void sirius_pfilter_start(struct dvr_pfilter *pfilter)
{
	if (pfilter->cfg_done == 0)
		sirius_pfilter_cfg_done(pfilter);
}

void sirius_pfilter_stop(struct dvr_pfilter *pfilter)
{
	if (pfilter->cfg_done)
		sirius_pfilter_cfg_req(pfilter);
}

void sirius_pfilter_set_ctrl(struct dvr_pfilter *pfilter)
{
	int mode;
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;

	if (reg->pid_en_h || reg->pid_en_l)
		mode = 0;
	else
		mode = pfilter->mode+1;
	REG_SET_FIELD(&reg->ctrl0, MSK_PFILTER_CTRL0_STORE_MODE, mode, BIT_PFILTER_CTRL0_STORE_MODE);
	REG_SET_FIELD(&reg->ctrl0, MSK_PFILTER_CTRL0_TS_SEL, pfilter->source, BIT_PFILTER_CTRL0_TS_SEL);
	REG_SET_FIELD(&reg->ctrl0, MSK_PFILTER_CTRL0_INPUT_MODE, pfilter->input_mode, BIT_PFILTER_CTRL0_INPUT_MODE);
	REG_SET_FIELD(&reg->ctrl0, MSK_PFILTER_CTRL0_NOSYNC, pfilter->nosync_en, BIT_PFILTER_CTRL0_NOSYNC);
	REG_SET_FIELD(&reg->ctrl0, MSK_PFILTER_CTRL0_NOVALID, pfilter->novalid_en, BIT_PFILTER_CTRL0_NOVALID);
}

void sirius_pfilter_set_gate(struct dvr_pfilter *pfilter)
{
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
	reg->buf_full_gate        = pfilter->hw_buffer_size;
	reg->buf_almost_full_gate = pfilter->almost_full_gate;
}

void sirius_pfilter_add_pid(struct dvr_pfilter *pfilter, int pid_handle, int pid)
{
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
	reg->pid[pid_handle/2] &= ~(0x1fff << ((pid_handle%2)*16));
	reg->pid[pid_handle/2] |= (pid << ((pid_handle%2)*16));
	if (pid_handle >= 32)
		reg->pid_en_h |= 0x1<<(pid_handle-32);
	else
		reg->pid_en_l |= 0x1<<(pid_handle);
}

void sirius_pfilter_del_pid(struct dvr_pfilter *pfilter, int pid_handle)
{
	volatile struct sirius_reg_pid_filter *reg = (volatile struct sirius_reg_pid_filter *)pfilter->reg;
	if (pid_handle > 32)
		reg->pid_en_h &= ~(0x1<<(pid_handle-32));
	else
		reg->pid_en_l &= ~(0x1<<(pid_handle));
	reg->pid[pid_handle/2] &= ~(0x1fff << ((pid_handle%2)*16));
}

static struct dvr_pfilter *sirius_pfilter_get(int id)
{
	return (id < MAX_PFILTER_UNIT) ? &sirius_pfilter[id] : NULL;
}

struct demux_pfilter_ops sirius_pfilter_ops = {
	.open            = sirius_pfilter_open,
	.close           = sirius_pfilter_close,
	.start           = sirius_pfilter_start,
	.stop            = sirius_pfilter_stop,
	.set_ctrl        = sirius_pfilter_set_ctrl,
	.set_gate        = sirius_pfilter_set_gate,
	.add_pid         = sirius_pfilter_add_pid,
	.del_pid         = sirius_pfilter_del_pid,
	.irq_entry       = sirius_pfilter_irq_entry,
	.tsw_isr         = sirius_pfilter_dealwith,

	.get_pfilter     = sirius_pfilter_get,
};

int sirius_pfilter_init(void)
{
	int i;
	struct dmxdev *dev;
	int addr[3] = {SIRIUS_DEMUX_BASE_PID_FILTER0, SIRIUS_DEMUX_BASE_PID_FILTER1, SIRIUS_DEMUX_BASE_PID_FILTER2};

	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->pfilter_ops     = &sirius_pfilter_ops;
		dev->max_pfilter     = MAX_PFILTER_UNIT;
		dev->max_pfilter_pid = MAX_PFILTER_PID;
	}

	for (i=0; i<MAX_PFILTER_UNIT; i++) {
		gx_memset(&sirius_pfilter[i], 0, sizeof(struct dvr_pfilter));
		sirius_pfilter[i].id = i;
		sirius_pfilter[i].max_pid_num = MAX_PFILTER_PID;
		sirius_pfilter[i].pid_mask = 0;
		sirius_pfilter[i].base_addr = addr[i];
		sirius_pfilter[i].cfg_done = 1;
		if (!gx_request_mem_region(addr[i], sizeof(struct sirius_reg_pid_filter))) {
			gxlog_e(LOG_DEMUX, "request pfilter mem region failed!\n");
			return -1;
		}
		sirius_pfilter[i].reg = gx_ioremap(addr[i], sizeof(struct sirius_reg_pid_filter));
		if (!sirius_pfilter[i].reg) {
			gxlog_e(LOG_DEMUX, "ioremap pfilter space failed!\n");
			return -1;
		}
	}
	return dvr_pfilter_init();
}

int sirius_pfilter_deinit(void)
{
	int i;
	struct dmxdev *dev;

	dvr_pfilter_deinit();
	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->pfilter_ops     = NULL;
		dev->max_pfilter     = 0;
		dev->max_pfilter_pid = 0;
	}

	for (i=0; i<MAX_PFILTER_UNIT; i++) {
		gx_iounmap(sirius_pfilter[i].reg);
		gx_release_mem_region(sirius_pfilter[i].base_addr, sizeof(struct sirius_reg_pid_filter));
	}
	return 0;
}
