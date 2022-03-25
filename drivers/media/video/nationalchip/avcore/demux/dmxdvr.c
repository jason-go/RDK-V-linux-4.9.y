#include "kernelcalls.h"
#include "avcore.h"
#include "gxav_bitops.h"
#include "gxdemux.h"
#include "gx3211_regs.h"
#include "sirius_regs.h"
#include "gxav_m2m.h"
#include "fifo.h"

#define BUFFER_TS_TRANS_UNIT 188*48
#if 0
static int dvr_test_get_phy_buffer(struct dvr_phy_buffer *buffer, unsigned int length)
{
	int remain = buffer->size - buffer->offset;
	if (remain > length)
		buffer->data_len = length;
	else
		buffer->data_len = remain;

	return length-buffer->data_len;
}

static void dvr_test_copy_to_phy_buffer(struct dvr_phy_buffer *dst, unsigned char *src, unsigned int length)
{
	int remain;
	if ((remain = dvr_test_get_phy_buffer(dst, length)) > 0) {
		memcpy((unsigned char *)dst->vaddr+dst->offset, src, dst->data_len);
		memcpy((unsigned char *)dst->vaddr, src+dst->data_len, remain);
		dst->offset = dst->data_len;

	} else {
		memcpy((unsigned char *)dst->vaddr+dst->offset, src, length);
		dst->offset = (dst->offset + length) % dst->size;
	}
}

static int dvr_test_protocol_callback(struct dvr_phy_buffer *src, struct dvr_phy_buffer *dst)
{
	int src_remain;
	static unsigned char *test_buf = NULL;
	unsigned int src_len, dst_len;

	if (NULL == src || NULL == dst)
		return 0;

	if (NULL == test_buf)
		test_buf = (unsigned char *)gx_malloc(188*20+4096);

	src_len = src->data_len;
	dst_len = src->data_len; // copy src data
	if ((src_remain = dvr_test_get_phy_buffer(src, src->data_len)) > 0) {
		dvr_test_copy_to_phy_buffer(dst, (unsigned char *)src->vaddr+src->offset, src->data_len);
		dvr_test_copy_to_phy_buffer(dst, (unsigned char *)src->vaddr, src_remain);
		src->offset = src_remain;

	} else {
		dvr_test_copy_to_phy_buffer(dst, (unsigned char *)src->vaddr+src->offset, src->data_len);
		src->offset += src->data_len;
	}

	return src_len;
}
#endif

int dvr_tee_set_tsw_buf(int devid, int tswid, unsigned int start, unsigned int size, unsigned int gate)
{
#ifdef CONFIG_AV_TEE_MODULE_SVP
	unsigned reg_start = 0, reg_size = 0, reg_gate = 0;
	if (devid) {
		reg_start = 0x137f0 + 0x20 * tswid;
		reg_size  = 0x137f4 + 0x20 * tswid;
		reg_gate  = 0x13800 + 0x20 * tswid;
	} else {
		reg_start = 0x37f0 + 0x20 * tswid;
		reg_size  = 0x37f4 + 0x20 * tswid;
		reg_gate  = 0x3800 + 0x20 * tswid;
	}

	gxav_secure_register_write(0x8a200000, reg_start, start);
	gxav_secure_register_write(0x8a200000, reg_size , size);
	gxav_secure_register_write(0x8a200000, reg_gate , gate);
#endif

	return 0;
}

int dvr_tee_tsw_enable(int devid, int dmxid, int tswid)
{
#ifdef CONFIG_AV_TEE_MODULE_SVP
	unsigned int reg_ctrl = devid ? 0x10000 : 0;

	if (tswid < 32) {
		if (dmxid)
			reg_ctrl += 0x3714;
		else
			reg_ctrl += 0x370c;

	} else {
		tswid -= 32;
		if (dmxid)
			reg_ctrl += 0x3710;
		else
			reg_ctrl += 0x3708;
	}
	gxav_secure_register_write(0x8a200000, reg_ctrl, tswid);
#endif

	return 0;
}

int dvr_tee_tsw_disable(int devid, int dmxid, int tswid)
{
#ifdef CONFIG_AV_TEE_MODULE_SVP
	unsigned int reg_ctrl = devid ? 0x10000 : 0;

	if (tswid < 32) {
		if (dmxid)
			reg_ctrl += 0x3714;
		else
			reg_ctrl += 0x370c;

	} else {
		tswid -= 32;
		if (dmxid)
			reg_ctrl += 0x3710;
		else
			reg_ctrl += 0x3708;
	}
	gxav_secure_register_write(0x8a200000, reg_ctrl, tswid<<16);
#endif

	return 0;
}

void dvr_tsr_debug_print(unsigned int dvrid)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
	struct dmx_demux *demux = get_subdemux(dvrid%4);
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_info *info = NULL;

	if (dvrid < demux->dev->max_dvr)
		info = &demux->dvr.tsr_info;
	else
		info = &demux->dvr.tsr23_info;

	if (info->status == DVR_STATE_IDLE)
		gxlog_d(LOG_DVR, "DVR(%d)'TSR is IDLE.\n", dvrid);
	else {
		gxlog_d(LOG_DVR, "DVR(%d)'TSR:\n", dvrid);
		gxlog_d(LOG_DVR, "\tSrc=%d, Dst=%d, status=%d\n",
				info->source, info->dest, info->status);
		gxlog_d(LOG_DVR, "\tSWbuffersize=0x%x, HWbuffersize=0x%x, Gate=0x%x\n",
				info->sw_buffer_size, info->hw_buffer_size, info->almost_full_gate);
	}
#endif
}

void dvr_tsw_debug_print(unsigned int dvrid, unsigned int tswid)
{
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
	struct dmx_demux *demux = get_subdemux(dvrid%4);
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_info *info = NULL;
	struct dmxdev *dev = demux->dev;

	if (dvrid >= dev->max_dvr || tswid >= dev->max_tsw) {
		gxlog_d(LOG_DEMUX, "%s %d: Parameter error.\n", __func__, __LINE__);
		return;
	}

	if ((info = &dvr->tsw_info[tswid]) == NULL) {
		gxlog_d(LOG_DVR, "DVR(%d)'TSW(%d) is NULL.\n", dvrid, tswid);
		return;
	}

	if (info->status == DVR_STATE_IDLE)
		gxlog_d(LOG_DVR, "DVR(%d)'TSW(%d) is IDLE.\n", dvrid, tswid);
	else {
		gxlog_d(LOG_DVR, "DVR(%d)'TSW(%d)\n", dvrid, tswid);
		gxlog_d(LOG_DVR, "\tSrc=%d, Dst=%d, status=%d, mask=0x%llx\n",
				info->source, info->dest, info->status, dev->tsw_mask);
		gxlog_d(LOG_DVR, "\tSWbuffersize=0x%x, HWbuffersize=0x%x, Gate=0x%x\n",
				info->sw_buffer_size, info->hw_buffer_size, info->almost_full_gate);
	}
#endif
}

static struct dvr_info *dvr_query_tsw_by_handle(int dvrid, int handle)
{
	int i;
	struct dmx_demux *dmx = get_subdemux(dvrid%4);
	struct dmxdev  *dev = dmx->dev;
	struct dmx_dvr *dvr = &dmx->dvr;

	for (i = 0; i < dev->max_tsw; i++) {
		if (dev->dvr_handles[i] == handle)
			return &dvr->tsw_info[i];
	}

	return NULL;
}

int gxav_dvr_set_protocol_callback(int dvrid, int handle, pCallback func)
{
	struct dvr_info *info = dvr_query_tsw_by_handle(dvrid, handle);
	if (NULL == info)
		return -1;

	info->protocol_callback = func;
	return 0;
}

static int dvr_tsw_alloc_fixed(int dmxid, int tswid, int handle)
{
	int i;
	struct dmx_demux *dmx = get_subdemux(dmxid%4);
	struct dmxdev  *dev = dmx->dev;
	struct dmx_dvr *dvr = &dmx->dvr;

	if (dev->tsw_mask & (0x1 << tswid)) {
		gxlog_e(LOG_DVR, "have no tsw left!\n");
		return -1;
	}

	dvr->tsw_info[tswid].id = tswid;
	dev->dvr_handles[tswid] = handle;
	SET_MASK(dev->tsw_mask, tswid);

	return tswid;
}

static int dvr_tsw_free_fixed(int dmxid, int tswid, int handle)
{
	struct dmx_demux *dmx = get_subdemux(dmxid%4);
	struct dmxdev  *dev = dmx->dev;
	struct dmx_dvr *dvr = &dmx->dvr;

	CLR_MASK(dev->tsw_mask, tswid);
	dev->dvr_handles[tswid] = -1;

	return 0;
}

static int dvr_t2mi_set_source(int id)
{
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;
	struct dvr_info *info = (id < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;

	if (info->status != DVR_STATE_IDLE)
		return -1;

	if (dvr_ops->t2mi_set_source) {
		dvr_ops->t2mi_set_source(id, dev->reg);
		dvr->flags |= DVR_T2MI_IN_EN;
		return 0;
	}
	return -1;
}

static int dvr_t2mi_clr_source(int id)
{
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	if (dvr_ops->t2mi_clr_source) {
		dvr_ops->t2mi_clr_source(id, dev->reg);
		dvr->flags &= ~DVR_T2MI_IN_EN;
		return 0;
	}
	return -1;
}

static int dvr_is_busy(void)
{
	struct dmx_demux *dmx  = get_subdemux(0);
	struct dmx_dvr *dvr    = NULL;
	int i = 0;

	for (i = 0; i< dmx->dev->max_dvr; i++) {
		dmx = get_subdemux(i%4);
		dvr = &dmx->dvr;
		if (dvr->tsr_info.status == DVR_STATE_BUSY ||
			dvr->tsw_info[0].status == DVR_STATE_BUSY ||
			dvr->tsw_info[1].status == DVR_STATE_BUSY)
			return 1;
	}
	return 0;
}

static int dvr_t2mi_set_dest(int id)
{
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;
	struct dvr_info *info = &dvr->tsw_info[0];

	if (info->status == DVR_STATE_BUSY)
		return -1;

	if (dvr_ops->t2mi_set_dest) {
		dvr_ops->t2mi_set_dest(id, dev->reg);
		dvr->flags |= DVR_T2MI_OUT_EN;
		return 0;
	}
	return -1;
}

static int dvr_t2mi_clr_dest(int id)
{
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	if (dvr_ops->t2mi_clr_dest) {
		dvr_ops->t2mi_clr_dest(id, dev->reg);
		dvr->flags &= ~DVR_T2MI_OUT_EN;
		return 0;
	}
	return -1;
}

static int dvr_tsw_alloc(int id, int handle)
{
	int i;
	struct dmx_demux *dmx = get_subdemux(id%4);
	struct dmxdev  *dev = dmx->dev;
	struct dmx_dvr *dvr = &dmx->dvr;

	i = get_one(dev->tsw_mask, 1,  0, dev->max_tsw);
	if (i < 0) {
		gxlog_e(LOG_DVR, "have no tsw left!\n");
		return -1;
	}

	dvr->tsw_info[i].id = i;
	dev->dvr_handles[i] = handle;
	SET_MASK(dev->tsw_mask, i);

	return i;
}
static int dvr_buffer_status_set(GxDvrProperty_CryptConfig  * config)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct dmx_dvr *dvr = &demux->dvr;
	if (config->flags & DVR_ENCRYPT_ENABLE)
		dvr->flags |= DVR_ENCRYPT_ENABLE;
	return 0;
}

static int dvr_buffer_status_get(void)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct dmx_dvr *dvr = &demux->dvr;
	return dvr->flags & DVR_ENCRYPT_ENABLE;
}

int dvr_buffer_protected(GxAvBufferType type)
{
	if (dvr_buffer_status_get()) {
		return 1;
	}
	return gxav_firewall_buffer_protected(type);
}


static void dvr_tsw_free(int id, int tsw_id)
{
	struct dmx_demux *dmx = get_subdemux(id%4);
	struct dmxdev  *dev = dmx->dev;
	struct dmx_dvr *dvr = &dmx->dvr;

	CLR_MASK(dev->tsw_mask, tsw_id);
	dev->dvr_handles[tsw_id] = -1;
}

static void dvr_set_all_tsr_buf(struct dmxdev *dev, struct dvr_info *info, int id)
{
	int i, sub;
	int mask = info->tsr_share_mask;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	sub = id % 2;
	if (id < MAX_DMX_NORMAL) {
		if (dvr_ops->set_tsr_buf)
			dvr_ops->set_tsr_buf(sub, dev->reg, info->paddr,
					info->hw_buffer_size, info->almost_full_gate);
	} else {
		if (dvr_ops->set_tsw_slot_buf)
			dvr_ops->set_tsw_slot_buf(sub, dev->reg, info->paddr,
					info->hw_buffer_size, info->almost_full_gate);
	}

	for (i = 0; i < MAX_DVR_UNIT; i++) {
		if ((mask & (1<<i)) == 0)
			continue;

		if (CHIP_IS_GX3211 || CHIP_IS_GX6605S)
			return;

		sub = i % dev->max_dvr;
		if (i >= dev->max_dvr) {
			if (dvr_ops->set_tsw_slot_buf)
				dvr_ops->set_tsw_slot_buf(sub, dev->reg, info->paddr,
						info->hw_buffer_size, info->almost_full_gate);
		} else {
			if (dvr_ops->set_tsr_buf)
				dvr_ops->set_tsr_buf(sub, dev->reg, info->paddr,
						info->hw_buffer_size, info->almost_full_gate);
		}
	}
}

static void dvr_clr_all_tsr_buf(struct dmxdev *dev, struct dvr_info *info, int id)
{
	int i, sub;
	int mask = info->tsr_share_mask;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	sub = id % dev->max_dvr;
	if (id < MAX_DMX_NORMAL) {
		if (dvr_ops->clr_tsr_buf)
			dvr_ops->clr_tsr_buf(sub, dev->reg);
	} else {
		if (dvr_ops->clr_tsw_slot_buf)
			dvr_ops->clr_tsw_slot_buf(sub, dev->reg);
	}

	for (i = 0; i < MAX_DVR_UNIT; i++) {
		if ((mask & (1<<i)) == 0)
			continue;

		sub = i % dev->max_dvr;
		if (i >= dev->max_dvr) {
			if (dvr_ops->clr_tsw_slot_buf)
				dvr_ops->clr_tsw_slot_buf(sub, dev->reg);
		} else {
			if (dvr_ops->clr_tsr_buf)
				dvr_ops->clr_tsr_buf(sub, dev->reg);
		}
	}
}

static void dvr_set_sub_tsr_share_status(struct dmxdev *dev, int major, int mask)
{
	int i, sub;
	struct dmx_demux *dmx  = NULL;
	struct dvr_info *info = NULL;

	for (i = 0; i < MAX_DVR_UNIT; i++) {
		if ((mask & (1<<i)) == 0)
			continue;

		sub = i % dev->max_dvr;
		dmx  = get_subdemux(sub);
		info = (i >= dev->max_dvr) ? &dmx->dvr.tsr23_info : &dmx->dvr.tsr_info;
		info->status = DVR_STATE_SHARE_SUB;
		info->tsr_share_major_id = major;
	}
}

static void dvr_clr_sub_tsr_share_status(struct dmxdev *dev, int mask)
{
	int i, sub;
	struct dmx_demux *dmx  = NULL;
	struct dvr_info *info = NULL;

	for (i = 0; i < MAX_DVR_UNIT; i++) {
		if ((mask & (1<<i)) == 0)
			continue;

		sub = i % dev->max_dvr;
		dmx  = get_subdemux(sub);
		info = (i >= dev->max_dvr) ? &dmx->dvr.tsr23_info : &dmx->dvr.tsr_info;
		info->status = DVR_STATE_IDLE;
		info->tsr_share_major_id = 0;
	}
}

static void dvr_set_tsw_buf(struct dmxdev *dev, int id, int tsw_id)
{
	int i = 0;
	struct dmx_demux *dmx = &dev->demuxs[id%2];
	struct dmx_dvr *dvr = &dmx->dvr;
	struct dvr_info *info = &dvr->tsw_info[tsw_id];
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	info->hw_full_count = 0;
	if (dvr_ops->set_dvr_mode)
		dvr_ops->set_dvr_mode(id, dev->reg, dvr->mode);
	if (dvr_ops->set_dvr_source)
		dvr_ops->set_dvr_source(id, tsw_id, dev->reg, info);

	gxdemux_tsw_lock();
	if (dvr_ops->set_tsw_buf)
		dvr_ops->set_tsw_buf(id, tsw_id, dev->reg, info->paddr,
				info->hw_buffer_size, info->almost_full_gate);
	if (dvr_ops->set_tsw_enable)
		dvr_ops->set_tsw_enable(id, tsw_id, dev->reg);
	gxdemux_tsw_unlock();
	dvr_tee_set_tsw_buf(dev->id, tsw_id, info->paddr, info->hw_buffer_size, info->almost_full_gate);
	dvr_tee_tsw_enable(dev->id, dmx->id, tsw_id);
	gxav_firewall_register_buffer(GXAV_BUFFER_DEMUX_TSW, info->paddr, info->hw_buffer_size);

	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
		if (info->flags & DVR_FLAG_DVR_TO_DSP) {
			if (NULL == info->trans_temp_buf)
				info->trans_temp_buf = gx_page_malloc(BUFFER_TS_TRANS_UNIT);
		}
	}

	for (i = 0; i < dev->max_slot; i++) {
		if (!dmx->slots[i])
			continue;
		if (dmx->slots[i]->ts_out_pin == info->dvr_handle) {
			if (id < MAX_DMX_NORMAL) {
				if (dvr_ops->bind_slot_tsw)
					dvr_ops->bind_slot_tsw(dev->reg, dmx->slots[i]->id, tsw_id);
			} else {
				if (dvr_ops->pf_bind_slot_tsw)
					dvr_ops->pf_bind_slot_tsw(dev->reg, dmx->slots[i]->id, tsw_id);
			}
		}
	}
	dvr_tsw_debug_print(dvr->id, tsw_id);
}

static void dvr_clr_tsw_buf(struct dmxdev *dev, int id, int tsw_id)
{
	struct dmx_demux *dmx = &dev->demuxs[id%2];
	struct dmx_dvr *dvr = &dmx->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	dvr_tee_tsw_disable(dev->id, dmx->id, tsw_id);
	gxdemux_tsw_lock();
	if (dvr_ops->set_tsw_disable)
		dvr_ops->set_tsw_disable(id, tsw_id, dev->reg);
	if (dvr_ops->clr_tsw_buf)
		dvr_ops->clr_tsw_buf(id, tsw_id, dev->reg);
	gxdemux_tsw_unlock();
}

static struct dvr_info *dvr_get_tsw_info_by_secure_tsr(int id)
{
	struct dmx_demux *demux = get_subdemux(id%4);
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_info *tsr_info = NULL;
	tsr_info = (id < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;
	if (tsr_info->status == DVR_STATE_BUSY)
		return &dvr->tsw_info[tsr_info->secure_tsw_id];

	return NULL;
}

static struct dvr_info *dvr_get_tsw_info(int modid, int dvr_handle, int dst)
{
	int tsw_id = 0;
	struct dmx_demux *demux = get_subdemux(modid%4);
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_info *info = NULL;

	if (NULL == (info = dvr_query_tsw_by_handle(modid, dvr_handle))) {
		if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
			if (dst == DVR_OUTPUT_DSP) {
				if (NULL == (info = dvr_get_tsw_info_by_secure_tsr(modid))) {
					if ((tsw_id = dvr_tsw_alloc_fixed(modid, modid%2, dvr_handle)) < 0) {
						gxlog_e(LOG_DVR, "DSP TSW alloc failed.\n");
						return NULL;
					}
				} else
					tsw_id = info->id;
			} else {
				if ((tsw_id = dvr_tsw_alloc_fixed(modid, modid%2, dvr_handle)) < 0) {
					gxlog_e(LOG_DVR, "TSW alloc failed.\n");
					return NULL;
				}
			}

		} else {
			if ((tsw_id = dvr_tsw_alloc(modid, dvr_handle)) < 0) {
				gxlog_e(LOG_DVR, "TSW alloc failed.\n");
				return NULL;
			}
		}
		info = &dvr->tsw_info[tsw_id];
	}

	return info;
}

static struct gxfifo *dvr_get_tsw_fifo(int modid, int dvr_handle)
{
	int i, sub = modid%4;
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmx_dvr *dvr = &demux->dvr;

	if (modid < MAX_DMX_TSW_SLOT) {
		for (i = 0; i < demux->dev->max_tsw; i++) {
			if (dvr->tsw_info[i].dvr_handle == dvr_handle) {
				if (dvr->tsw_info[i].flags & DVR_FLAG_PTR_MODE_EN)
					return NULL;
				return &dvr->tsw_info[i].fifo;
			}
		}

#ifdef CONFIG_AV_MODULE_PIDFILTER
	} else if (modid < MAX_DMX_PID_FILTER) {
		return dvr_pfilter_get_tsw_fifo(sub);
#endif
	}

	return NULL;
}

/* TSR Module need 128 align buffer size */
#define BUFFER_TS_HW_SIZE    (188*1024)
#define BUFFER_TSR_GATE      (188*100)
#define BUFFER_TSW_GATE      (188*400)
#define BUFFER_TS_BLOCK      (188*256)

static int dmx_gcd(int a, int b)
{
	if (b)
		return dmx_gcd(b, a % b);
	else
		return a;
}

struct dvr_cmdline_info {
	unsigned int mask;
	unsigned int max;
	struct gx_mem_info mem;
};

static struct dvr_cmdline_info tsr_meminfo = {0};
static struct dvr_cmdline_info tsw_meminfo = {0};

static int dvr_get_cmdline_buf(struct dvr_info *info, int config_tsr)
{
	int id = 0;
	static int init = 0;
	unsigned int blocklen = info->blocklen;
	struct gx_mem_info *ts_info = NULL;
	struct dvr_cmdline_info *mem_info = NULL;

	if (init == 0) {
		gx_mem_info_get("tsrmem", &tsr_meminfo.mem);
		gx_mem_info_get("tswmem", &tsw_meminfo.mem);
		tsw_meminfo.max = tsw_meminfo.mem.size / BUFFER_TS_HW_SIZE;

		if (tsw_meminfo.max > 32)
			tsw_meminfo.max = 32;
		else if (tsw_meminfo.max == 0)
			tsw_meminfo.max = 1;

		tsw_meminfo.mask = 0;
		tsr_meminfo.max = 1;
		tsr_meminfo.mask = 0;
		init = 1;
	}

	ts_info = config_tsr ? &tsr_meminfo.mem : &tsw_meminfo.mem;
	mem_info = config_tsr ? &tsr_meminfo : &tsw_meminfo;

	if ((info->vaddr = (void *)ts_info->start) == NULL)
		return -1;

	if (blocklen) {
		int align = blocklen * 64 / dmx_gcd(blocklen, 64);

		if (ts_info->size < align*2) {
			gxlog_e(LOG_DVR, "DVR hw_buffer the field \"%s\" in cmdline is too small! %x %x\n", config_tsr ? "tsrmem" : "tswmem", ts_info->size, align);
			return -1;
		}

		if (mem_info->mask) {
			gxlog_e(LOG_DVR, "The cmdline buffer is occupied.\n");
			return -1;
		}
		mem_info->mask = 0xffffffff;

		info->hw_buffer_size = ts_info->size - ts_info->size % align;
		info->paddr = ts_info->start;

		gxlog_i(LOG_DVR, "DVR hw_buffer is using the field \"%s\" in cmdline!\n [align] = %x [blocklen] = %x [bufsize] = %x\n",
				config_tsr ? "tsrmem" : "tswmem", align, blocklen, info->hw_buffer_size);

	} else {
		int align = 0, count = 1, i = 0;

		if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSR))
			align = 188 * 1024 / dmx_gcd(188, 1024);
		else
			align = 188 * 64 / dmx_gcd(188, 64);

		if (mem_info->max == 1) {
			info->hw_buffer_size = ts_info->size;

		} else {
			if (info->hw_buffer_size) {
				count = (info->hw_buffer_size - 1) / BUFFER_TS_HW_SIZE + 1;
				info->hw_buffer_size = count * BUFFER_TS_HW_SIZE;
			}
		}

		if ((id = get_one(mem_info->mask, count, 0, mem_info->max)) < 0) {
			gxlog_e(LOG_DVR, "Have no cmdline buffer left.\n");
			return -1;
		}

		info->hw_buffer_size = info->hw_buffer_size / (align) * (align);
		if (info->hw_buffer_size < align * 2) {
			gxlog_e(LOG_DVR, "DVR hw_buffer the field \"%s\" in cmdline is too small! %x %x\n", config_tsr ? "tsrmem" : "tswmem", ts_info->size, align);
			return -1;
		}

		info->paddr = ts_info->start + id*BUFFER_TS_HW_SIZE;
		for (i = id; i < id+count; i++)
			SET_MASK(mem_info->mask, i);

		gxlog_i(LOG_DVR, "DVR hw_buffer is using the field \"%s\" in cmdline!\n [addr] = %x [bufsize] = %x %d\n",
				config_tsr ? "tsrmem" : "tswmem", info->paddr, info->hw_buffer_size, id);
	}

	info->vaddr = (void *)gx_phys_to_virt(info->paddr);
	info->flags |= DVR_FLAG_TSBUF_FROM_CMDLINE;

	return 0;
}

static int dvr_clr_cmdline_buf(struct dvr_info *info, int config_tsr)
{
	int id = 0;
	struct gx_mem_info *ts_info = NULL;
	struct dvr_cmdline_info *mem_info = NULL;

	ts_info = config_tsr ? &tsr_meminfo.mem : &tsw_meminfo.mem;
	mem_info = config_tsr ? &tsr_meminfo : &tsw_meminfo;

	if (ts_info->start == 0)
		return 0;

	if (config_tsr == 0) {
		if (info->hw_buffer_size == BUFFER_TS_HW_SIZE) {
			int i = 0, count = 0;
			count = info->hw_buffer_size / BUFFER_TS_HW_SIZE;
			id = (gx_virt_to_phys((unsigned int)info->vaddr) - ts_info->start) / BUFFER_TS_HW_SIZE;
			for (i = id; i < id+count; i++)
				CLR_MASK(mem_info->mask, i);
		} else {
			mem_info->mask = 0;
		}
	} else
		mem_info->mask = 0;

	return 0;
}

int dvr_tsr_empty(unsigned int id)
{
	int sub = id%4;
	unsigned int empty_len = 188*6;
	struct dmx_demux *demux = get_subdemux(sub);
	struct demux_dvr_ops *dvr_ops = demux->dev->dvr_ops;
	struct gxfifo *fifo = &demux->infifo;
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_info *tsr_info = (id < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;

	// Don't support tsr-empty in firewall work mode.
	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSR)) {
		tsr_info->status = DVR_STATE_IDLE;
		return 0;
	}

	gxdemux_dvr_lock();
	if (dvr_ops->tsr_dealwith)
		dvr_ops->tsr_dealwith(demux->dev, id, dvr->id);

	memset(fifo->data, 0, fifo->size);
	gxfifo_wptr_set(fifo, (gxfifo_wptr(fifo) + empty_len) % fifo->size);

	while (1) {
		if (dvr_ops->tsr_dealwith)
			dvr_ops->tsr_dealwith(demux->dev, id, dvr->id);
		if (GX_ABS(gxfifo_rptr(fifo) - gxfifo_wptr(fifo)) < 188 * 2)
			break;
	}

	tsr_info->status = DVR_STATE_IDLE;
	gxdemux_dvr_unlock();

	return 0;
}

int dvr_open(struct gxav_module *module)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev    *dev   = demux->dev;
	struct dmx_dvr   *dvr   = &demux->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	dvr->id = sub;
	if (!dvr->refcount) {
		if (dvr_ops->tsw_module_enable)
			dvr_ops->tsw_module_enable(module->sub, dev->reg);
		SET_MASK(dev->dvr_mask, sub);
	}
	dvr->refcount++;

	return 0;
}

int dvr_close(struct gxav_module *module)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev    *dev   = demux->dev;
	struct dmx_dvr   *dvr   = &demux->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	dvr->refcount--;
	if (!dvr->refcount) {
		if (dvr_ops->tsw_module_disable)
			dvr_ops->tsw_module_disable(module->sub, dev->reg);
		CLR_MASK(dev->dvr_mask, sub);
	}

	return 0;
}

static int dvr_config_common(struct dvr_info *info, struct dvr_info *config, int config_tsr)
{
	if (NULL == info->vaddr) {
		SET_DEFAULT_VALUE(info->hw_buffer_size, config->hw_buffer_size, BUFFER_TS_HW_SIZE);
		if (dvr_get_cmdline_buf(info, config_tsr) < 0) {
			info->vaddr = gx_page_malloc(info->hw_buffer_size);
			info->flags &= ~DVR_FLAG_TSBUF_FROM_CMDLINE;
		}
	}

	if (NULL == info->vaddr){
		gxlog_e(LOG_DVR, "dvr allocate fifo failed! size = 0x%x\n", info->hw_buffer_size);
		return -1;
	}

	SET_DEFAULT_VALUE(info->sw_buffer_size, config->sw_buffer_size, BUFFER_TS_HW_SIZE);
	if (config_tsr) {
		SET_DEFAULT_VALUE(info->almost_full_gate, config->almost_full_gate, BUFFER_TSR_GATE);
	} else {
		if ((info->hw_buffer_size>>6) > BUFFER_TSW_GATE)
			info->almost_full_gate = BUFFER_TSW_GATE;
		else
			SET_DEFAULT_VALUE(info->almost_full_gate, config->almost_full_gate, info->hw_buffer_size>>6);
	}

	if (info->sw_buffer_size < info->hw_buffer_size)
		info->sw_buffer_size = info->hw_buffer_size;

	if (config_tsr) {
		gxfifo_init(&info->fifo, info->vaddr, info->hw_buffer_size);
		if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
			gxfifo_set_put_func(&info->fifo, gxm2m_decrypt);
			if (dvr_buffer_protected(GXAV_BUFFER_SOFT) == 0) {
				gxdmxdev[0].secure_tsw_buf[info->secure_tsw_id] = info->vaddr;
				gxdmxdev[0].secure_tsw_bufsize[info->secure_tsw_id] = info->hw_buffer_size;
			}
		}

	} else {
		if (info->fifo.data) {
			gxfifo_init(&info->fifo, info->fifo.data, info->sw_buffer_size);

		} else {
/* if dvr 2 dvr enable, don't need tsw_fifo, tsw_isr put data into tsr_fifo directly */
			if (config->dest >= DVR_OUTPUT_MEM) {
				if (info->flags & DVR_FLAG_PTR_MODE_EN)
					gxfifo_init(&info->fifo, info->vaddr, info->hw_buffer_size);
				else
					gxfifo_init(&info->fifo, NULL, info->sw_buffer_size);
			} else
				gxfifo_init(&info->fifo, info->vaddr, info->hw_buffer_size);
		}

		if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
			gxfifo_set_put_func(&info->fifo, gxm2m_encrypt);
			if (dvr_buffer_protected(GXAV_BUFFER_SOFT) == 0) {
				gxdmxdev[0].secure_tsw_buf[info->id] = info->vaddr;
				gxdmxdev[0].secure_tsw_bufsize[info->id] = info->hw_buffer_size;
			}
		}
	}

	gx_memset(info->vaddr, 0, info->hw_buffer_size);
	gx_dcache_clean_range((unsigned int)info->vaddr, (unsigned int)info->vaddr+info->hw_buffer_size);
	info->paddr  = gx_virt_to_phys((unsigned int)info->vaddr);
	info->source = (config->source == DVR_INPUT_MEM_DMX) ? DVR_INPUT_DMX : config->source;
	info->dest   = config->dest;
	info->status = DVR_STATE_BUSY;

	return 0;
}

static int dvr_tsr_config(int id, int dvr_handle, struct dvr_info *config, int dvr_2_dvr)
{
	struct dmx_demux *dmx = get_subdemux(id%4);
	struct dmx_dvr *dvr   = &dmx->dvr;
	struct dvr_info *info = (id < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;

	if (dvr->flags & DVR_T2MI_IN_EN) {
		gxlog_e(LOG_DVR, "Can't open TSR in T2MI in mode.\n");
		return -1;
	}

	if (id >= MAX_DMX_TSW_SLOT || info->status >= DVR_STATE_BUSY)
		return 0;

	SET_DEFAULT_VALUE(info->blocklen, config->blocklen, BUFFER_TS_BLOCK);
	if (info->flags == 0)
		info->hw_buffer_size = config->hw_buffer_size;
	info->sw_buffer_size = config->sw_buffer_size;
	info->almost_full_gate = config->almost_full_gate;
	info->source = config->source;
	info->dest   = config->dest;
	info->flags |= config->flags & DVR_FLAG_PTR_MODE_EN;
	info->dvr_handle = dvr_handle;

	if (!dvr_2_dvr && (config->source >= DVR_INPUT_DVR0 && config->source <= DVR_INPUT_DVR3)) {
		info->flags |= DVR_FLAG_PRIV_TSR_SAME_BUF;
		return 0;
	}
	info->status = DVR_STATE_READY;

	return 0;
}

static int dvr_tsr_run(int id, int dvr_handle)
{
	int tsw_id = 0;
	struct dmx_demux *dmx = get_subdemux(id%4);
	struct dmxdev *dev    = dmx->dev;
	struct dmx_dvr *dvr   = &dmx->dvr;
	struct dvr_info *info = (id < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	if (id >= MAX_DMX_TSW_SLOT)
		return -1;

	if (info->status != DVR_STATE_READY)
		return 0;

	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
		if ((tsw_id = dvr_tsw_alloc_fixed(id, id, dvr_handle)) < 0) {
			gxlog_e(LOG_DVR, "TSW alloc failed.\n");
			return -1;
		}

		info->secure_tsw_id = tsw_id;
		if (dvr_buffer_protected(GXAV_BUFFER_SOFT) == 0) {
			if (gxdmxdev[0].secure_tsw_buf[tsw_id]) {
				info->vaddr = gxdmxdev[0].secure_tsw_buf[tsw_id];
				info->hw_buffer_size = gxdmxdev[0].secure_tsw_bufsize[tsw_id];
			}
		}
	}

	if (dvr_config_common(info, info, 1) < 0) {
		gxlog_e(LOG_DVR, "TSR buffer config failed.\n");
		return -1;
	}

	/* Set user's sub tsr share config */
	if (dev->tsr_share_mask[id]) {
		info->tsr_share_mask = dev->tsr_share_mask[id];
		info->status = DVR_STATE_SHARE_MAJOR;
		dvr_set_sub_tsr_share_status(dev, id, info->tsr_share_mask);
	}

	dvr_set_all_tsr_buf(dev, info, id);
	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
		if (NULL == info->trans_temp_buf)
			info->trans_temp_buf = gx_page_malloc(BUFFER_TS_TRANS_UNIT);
		if (dvr_buffer_protected(GXAV_BUFFER_SOFT) == 0) {
			if (dvr_ops->set_tsw_buf)
				dvr_ops->set_tsw_buf(id, tsw_id, gxdmxdev[0].reg, info->paddr,
						info->hw_buffer_size, info->almost_full_gate);
			dvr_tee_set_tsw_buf(0, tsw_id, info->paddr, info->hw_buffer_size, info->almost_full_gate);
		}
	}
	gxav_firewall_register_buffer(GXAV_BUFFER_DEMUX_TSR, info->paddr, info->hw_buffer_size);
	gx_memcpy(&dmx->infifo, &info->fifo, sizeof(struct gxfifo));
	dvr_tsr_debug_print(dvr->id);
	return 0;
}

static int dvr_2_dvr_config(struct dvr_info *tsw_info)
{
	int ret = 0;
	int dvr_id = tsw_info->dest;
	struct dmx_demux *dmx = get_subdemux(dvr_id%4);
	struct dmxdev *dev    = dmx->dev;
	struct dmx_dvr *dvr   = &dmx->dvr;
	struct dvr_info info = {0};
	struct dvr_info *tsr_info = (dvr_id < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;

	if (tsr_info->hw_buffer_size == 0) {
		info.sw_buffer_size   = tsw_info->sw_buffer_size;
		info.hw_buffer_size   = tsw_info->hw_buffer_size;
		info.almost_full_gate = tsw_info->almost_full_gate;
		tsr_info->hw_buffer_size = tsw_info->hw_buffer_size;
		tsr_info->vaddr = tsw_info->vaddr;
		tsr_info->paddr = tsw_info->paddr;
	} else {
		tsr_info->flags &= ~DVR_FLAG_PRIV_TSR_SAME_BUF;
		tsr_info->flags |= DVR_FLAG_PRIV_TSR_DIFF_BUF; // tsr&tsw use different phys buffer
	}

	info.source = tsr_info->source;
	info.dest   = tsr_info->dest;

	if (dev->max_slot == 32 && dvr_id >= DVR_OUTPUT_DVR2)
		dvr_id = 4 + dvr_id%2;

	if ((ret = dvr_tsr_config(dvr_id, tsr_info->dvr_handle, &info, 1)) < 0)
		return ret;

	return dvr_tsr_run(dvr_id, tsr_info->dvr_handle);
}

static int dvr_tsw_config(int id, int dvr_handle, struct dvr_info *config, unsigned int dvr_mode)
{
	int tsw_id;
	struct dvr_info *info;
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmx_dvr *dvr    = &dmx->dvr;

	if (dvr->flags & DVR_T2MI_OUT_EN)
		return -1;

	if (NULL == (info = dvr_get_tsw_info(id, dvr_handle, config->dest)))
		return -1;

	if (info->status >= DVR_STATE_BUSY)
		return -1;

	if (info->status == DVR_STATE_IDLE)
		info->dvr_handle = dvr_handle;
	dvr->mode = dvr_mode;
	info->source = config->source;
	info->dest   = config->dest;
	info->blocklen = config->blocklen;
	info->sw_buffer_size = config->sw_buffer_size;
	info->hw_buffer_size = config->hw_buffer_size;
	info->almost_full_gate = config->almost_full_gate;
	info->vaddr = config->vaddr;
	info->paddr = config->paddr;
	info->fifo.data = config->fifo.data;
	SET_DEFAULT_VALUE(info->blocklen, config->blocklen, BUFFER_TS_BLOCK);

	if (config->vaddr)
		info->flags |= DVR_FLAG_TSBUF_FROM_USER;
	if (config->dest == DVR_OUTPUT_DSP)
		info->flags |= DVR_FLAG_DVR_TO_DSP;

	info->flags |= config->flags & DVR_FLAG_PTR_MODE_EN;
	info->status = DVR_STATE_READY;

	return 0;
}

static int dvr_tsw_run(int id, int dvr_handle)
{
	int tsw_id;
	struct dvr_info *info = NULL;
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmxdev *dev     = dmx->dev;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	if (NULL == (info = dvr_query_tsw_by_handle(id, dvr_handle))) {
		if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW) == 0)
			return 0;
		if (NULL == (info = dvr_get_tsw_info_by_secure_tsr(id)))
			return 0;
	}

	if (info->status != DVR_STATE_READY)
		return 0;

	tsw_id = info->id;
	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW) && dvr_buffer_protected(GXAV_BUFFER_SOFT) == 0) {
		if (gxdmxdev[0].secure_tsw_buf[tsw_id]) {
			info->vaddr = gxdmxdev[0].secure_tsw_buf[tsw_id];
			info->hw_buffer_size = gxdmxdev[0].secure_tsw_bufsize[tsw_id];
		}
	}

	if (info->flags & DVR_FLAG_TSR_TO_DSP) {
		info->status = DVR_STATE_BUSY;
		return 0;
	}

	if (dvr_config_common(info, info, 0) < 0) {
		gxlog_e(LOG_DVR, "TSW buffer config failed.\n");
		return -1;
	}

	if (info->dest < DVR_OUTPUT_MEM) {
		if (dvr_2_dvr_config(info) < 0)
			return -1;
//		gxav_dvr_set_protocol_callback(id, dvr_handle, dvr_test_protocol_callback);
	}

	dvr_set_tsw_buf(dev, id, tsw_id);

	return 0;
}

static int dvr_tsw_link_fifo(int id, int dvr_handle, struct demux_fifo *fifo)
{
	int tsw_id = 0, link_tsr = 0;
	struct dmx_demux *dmx = get_subdemux(id%4);
	struct dmxdev *dev    = dmx->dev;
	struct dmx_dvr *dvr   = &dmx->dvr;
	struct dvr_info *info = NULL, *tsr_info = NULL, *tsw_info = NULL;

	if (NULL == (tsw_info = dvr_get_tsw_info(id, dvr_handle, DVR_OUTPUT_DSP)))
		return -1;

	tsw_id = tsw_info->id;
	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
		tsr_info = (id < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;
		if (tsr_info->status == DVR_STATE_BUSY) {
			link_tsr = 1;
			info = tsr_info;
			tsw_info->flags |= DVR_FLAG_TSR_TO_DSP;
		}
	}

	info = (NULL == info) ? tsw_info : info;
	if (link_tsr == 0) {
		if (info->status == DVR_STATE_BUSY) {
			gxlog_e(LOG_DVR, "TS channel (%d) is busy\n", tsw_id);
			return -1;
		}

		if ((info->flags & DVR_FLAG_DVR_TO_DSP) == 0) {
			gxlog_e(LOG_DVR, "It's not DSP TSChannel\n");
			return -1;
		}
	}

	info->link_vaddr = gx_phys_to_virt(fifo->buffer_start_addr);
	info->link_paddr = fifo->buffer_start_addr;
	info->link_size = fifo->buffer_end_addr - fifo->buffer_start_addr+1;
	info->link_channel_id = fifo->channel_id;

	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
		struct demux_ops *ops = dev->ops;

		if (ops->link_avbuf)
			ops->link_avbuf(id, dev->reg, 0, info->link_paddr, info->link_size, 0);
		gxav_firewall_register_buffer(GXAV_BUFFER_DEMUX_ES, info->link_paddr, info->link_size);

	} else {
		if (info->link_paddr) {
			info->paddr = info->link_paddr;
			info->vaddr = (void *)info->link_vaddr;
			info->hw_buffer_size = info->link_size;
		}
		info->dvr_handle = dvr_handle;
	}

	if (link_tsr)
		info->flags |= DVR_FLAG_DVR_TO_DSP;
	if (info->status != DVR_STATE_BUSY)
		info->status = DVR_STATE_READY;

	return 0;
}

static int dvr_tsw_unlink_fifo(int id, int dvr_handle, struct demux_fifo *fifo)
{
	struct dmx_demux *dmx = get_subdemux(id%4);
	struct dmx_dvr *dvr   = &dmx->dvr;
	struct dvr_info *info = NULL;

	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
		info = (id < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;
		if (info->status == DVR_STATE_BUSY && info->flags & DVR_FLAG_DVR_TO_DSP)
			info->flags &= ~(DVR_FLAG_DVR_TO_DSP);
	}

	return 0;
}

int dvr_tsr_stop(int id, int dvr_handle)
{
	int sub = id%4;
	struct dvr_info *info;
	struct dmx_demux *dmx  = get_subdemux(sub);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;

	if (id < MAX_DMX_NORMAL)
		info = &dvr->tsr_info;
	else
		info = &dvr->tsr23_info;

	if (dvr->flags & DVR_T2MI_IN_EN)
		dvr_t2mi_clr_source(id);

	if (info->dvr_handle == dvr_handle &&
		(info->status == DVR_STATE_BUSY ||
		 info->status == DVR_STATE_SHARE_MAJOR)) {
		dvr_tsr_empty(id);
		dvr_clr_all_tsr_buf(dev, info, id);

		gxfifo_free(&info->fifo);
		if (info->vaddr) {
			if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSR)) {
				dvr_tsw_free_fixed(sub, info->secure_tsw_id, dvr_handle);
				if (info->trans_temp_buf) {
					gx_page_free(info->trans_temp_buf, BUFFER_TS_TRANS_UNIT);
					info->trans_temp_buf = NULL;
				}

			}

			if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSR) == 0 || dvr_buffer_protected(GXAV_BUFFER_SOFT)) {
				if ((info->flags & DVR_FLAG_TSBUF_FROM_CMDLINE) == 0 &&
					(info->flags & DVR_FLAG_PRIV_TSR_SAME_BUF) == 0)
					gx_page_free(info->vaddr, info->hw_buffer_size);
				else
					dvr_clr_cmdline_buf(info, 1);
			}
			info->vaddr = NULL;
		}
		dvr_clr_sub_tsr_share_status(dev, info->tsr_share_mask);
		info->flags &= DVR_FLAG_TSBUF_FROM_CMDLINE;
	}

	return 0;
}

int dvr_tsw_stop(int id, int dvr_handle)
{
	int i;
	int sub = id%4;
	struct dvr_info *info;
	struct dmx_demux *dmx  = get_subdemux(sub);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	if (dvr->flags & DVR_T2MI_OUT_EN)
		dvr_t2mi_clr_dest(id);

	for (i = 0; i < dev->max_tsw; i++) {
		info = &dvr->tsw_info[i];
		if (info->status >= DVR_STATE_BUSY && info->dvr_handle == dvr_handle) {
			info->status = DVR_STATE_IDLE;
			dvr_clr_tsw_buf(dev, id, i);

			if (info->vaddr) {
				if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
					if (info->trans_temp_buf) {
						gx_page_free(info->trans_temp_buf, BUFFER_TS_TRANS_UNIT);
						info->trans_temp_buf = NULL;
					}
				}

				if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW) == 0 || dvr_buffer_protected(GXAV_BUFFER_SOFT)) {
					if (info->flags & DVR_FLAG_TSBUF_FROM_CMDLINE)
						dvr_clr_cmdline_buf(info, 0);
					else if ((info->flags & DVR_FLAG_DVR_TO_DSP) == 0 &&
							 (info->flags & DVR_FLAG_TSBUF_FROM_USER) == 0)
						gx_page_free(info->vaddr, info->hw_buffer_size);
				}
				info->vaddr = NULL;
			}

			if (info->dest == DVR_OUTPUT_MEM)
				gxfifo_free(&info->fifo);
			gxav_dvr_set_protocol_callback(sub, info->dvr_handle, NULL);
			dvr_tsw_free(sub, i);
			info->flags &= DVR_FLAG_TSBUF_FROM_CMDLINE | DVR_FLAG_DVR_TO_DSP;
		}
	}

	return 0;
}

static int dvr_tsw_attribute(int id, int dvr_handle, GxDvrProperty_Config *config)
{
	struct dvr_info *info = NULL;
	if (NULL == (info = dvr_query_tsw_by_handle(id, dvr_handle)))
		return -1;

	config->dst = info->dest;
	config->dst_buf.sw_buffer_size   = info->sw_buffer_size;
	config->dst_buf.hw_buffer_size   = info->hw_buffer_size;
	config->dst_buf.almost_full_gate = info->almost_full_gate;
	config->dst_buf.buffer_len       = gxfifo_len(&info->fifo);

	return 0;
}

static int dvr_tsr_attribute(int id, int dvr_handle, GxDvrProperty_Config *config)
{
	unsigned int dsp_freelen = 0, tsr_freelen = 0;
	struct dmx_demux *demux = get_subdemux(id%4);
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_info *info = &dvr->tsr_info;
	struct gxfifo *fifo = &demux->infifo;
	struct demux_dvr_ops *dvr_ops = demux->dev->dvr_ops;

	if (info->status == 0)
		return -1;

	if (dvr_ops->tsr_dealwith)
		dvr_ops->tsr_dealwith(demux->dev, id, id);

	config->src = info->source;
	config->src_buf.sw_buffer_size   = info->sw_buffer_size;
	config->src_buf.hw_buffer_size   = info->hw_buffer_size;
	config->src_buf.almost_full_gate = info->almost_full_gate;

	tsr_freelen = gxfifo_freelen(fifo);
	config->src_buf.buffer_freelen = tsr_freelen;
	config->src_buf.buffer_len     = (info->hw_buffer_size - tsr_freelen);

	return 0;
}

int dvr_attribute(int id, int dvr_handle, GxDvrProperty_Config *config)
{
	dvr_tsw_attribute(id, dvr_handle, config);
	dvr_tsr_attribute(id, dvr_handle, config);
	return 0;
}

int dvr_reset(int id, int dvr_handle, GxDvrProperty_Reset *reset)
{
	struct dvr_info *info;
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	if (reset->mod & DVR_MOD_TSR) {
		info = &dvr->tsr_info;
		if (info->status == DVR_STATE_BUSY) {
			gxdemux_dvr_lock();
			if (dvr_ops->reset_tsr_buf)
				dvr_ops->reset_tsr_buf(dmx->id, dev->reg);
			gxfifo_reset(&dmx->infifo);
			gxdemux_dvr_unlock();
		}
	}

	if (reset->mod & DVR_MOD_TSW) {
		info = dvr_query_tsw_by_handle(id, dvr_handle);
		if (NULL == info || info->status != DVR_STATE_BUSY) {
			gxlog_e(LOG_DVR, "TSW channel (%d) is idle\n", id);
			return -1;
		}

		info->status = DVR_STATE_IDLE;
		dvr_tee_tsw_disable(dev->id, dmx->id, info->id);
		gxdemux_tsw_lock();
		if (dvr_ops->set_tsw_disable)
			dvr_ops->set_tsw_disable(id, info->id, dev->reg);
		if (dvr_ops->reset_tsw_buf)
			dvr_ops->reset_tsw_buf(id, info->id, dev->reg);
		gxfifo_reset(&info->fifo);

		info->status = DVR_STATE_BUSY;
		if (dvr_ops->set_tsw_enable)
			dvr_ops->set_tsw_enable(id, info->id, dev->reg);
		gxdemux_tsw_unlock();
		dvr_tee_tsw_enable(dev->id, dmx->id, info->id);
	}

	return 0;
}

int dvr_config(int id, int dvr_handle, GxDvrProperty_Config *config)
{
	struct dvr_info info = {0};
	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {
		if (id >= 2) {
			gxlog_e(LOG_DVR, "DVR %d don't supported in secure mode.\n", id);
			return -1;
		}
	}

	if (config->dst < DVR_OUTPUT_DMX) {
		info.source = config->src;
		info.dest   = config->dst;
		info.flags  = config->flags;
		info.blocklen = config->blocklen;
		info.sw_buffer_size   = config->dst_buf.sw_buffer_size;
		info.hw_buffer_size   = config->dst_buf.hw_buffer_size;
		info.almost_full_gate = config->dst_buf.almost_full_gate;
		if (config->dst_buf.hw_buffer_ptr) {
			info.paddr = gx_virt_to_phys(config->dst_buf.hw_buffer_ptr);
			info.vaddr = (void *)config->dst_buf.hw_buffer_ptr;
		}

		if (config->dst_buf.sw_buffer_ptr)
			info.fifo.data = (void *)config->dst_buf.sw_buffer_ptr;

		if (dvr_tsw_config(id, dvr_handle, &info, config->mode) < 0)
			return -1;

	} else if (config->dst == DVR_OUTPUT_T2MI) {
		if (dvr_t2mi_set_dest(id) < 0)
			return -1;
	}

	if (config->src >= DVR_INPUT_MEM) {
		if (config->src == DVR_INPUT_T2MI) {
			if (config->dst >= DVR_OUTPUT_T2MI) {
				gxlog_e(LOG_DVR, "DVR T2MI source or dst error\n");
				return -1;
			}
			if (dvr_t2mi_set_source(id) < 0)
				return -1;

		} else if (config->src != DVR_INPUT_DMX) {
			info.source = config->src;
			info.dest   = config->dst;
			info.flags  = config->flags;
			info.blocklen = config->blocklen;
			info.sw_buffer_size   = config->src_buf.sw_buffer_size;
			info.hw_buffer_size   = config->src_buf.hw_buffer_size;
			info.almost_full_gate = config->src_buf.almost_full_gate;
			if (dvr_tsr_config(id, dvr_handle, &info, 0) < 0)
				return -1;
		}
	}

	return 0;
}

static int dvr_run(int id, int dvr_handle)
{
	int ret = 0;
	if ((ret = dvr_tsw_run(id, dvr_handle)) < 0)
		return ret;

	if ((ret = dvr_tsr_run(id, dvr_handle)) < 0)
		return ret;

	return 0;
}

static int dvr_stop(int id, int dvr_handle)
{
	int ret = 0;
	if ((ret = dvr_tsw_stop(id, dvr_handle)) < 0)
		return ret;

	if ((ret = dvr_tsr_stop(id, dvr_handle)) < 0)
		return ret;

	return 0;
}

int dvr_tsr_share_config(int id, int dvr_handle, GxDvrProperty_TSRShare *share)
{
	int major;
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct dvr_info *info = &dvr->tsr_info;

	if (info->status) {
		gxlog_e(LOG_DVR, "DVR(%d)'s TSR module is busy.\n", id);
		return -1;
	}

	if (id < dev->max_dvr)
		major = id;
	else
		major = id%2+2;

	dev->tsr_share_mask[id] = share->tsr_share_mask & ~(1<<major);

	return 0;
}

static int dvr_tsr_flow_control_set(int id, GxDvrProperty_TSRFlowControl *ctrl)
{
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct dvr_info *info = &dvr->tsr_info;

	info->flow_control = ctrl->flags;

	return 0;
}

static int dvr_tsr_flow_control_get(int id, GxDvrProperty_TSRFlowControl *ctrl)
{
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct dvr_info *info = &dvr->tsr_info;

	ctrl->flags = info->flow_control;

	return 0;
}

static int dvr_tsr_flow_control(int id, unsigned int size)
{
	int i;
	unsigned int align = 188*4;
	unsigned int avfree, avlen, avcap;
	struct dmx_demux *dmx  = get_subdemux(id%4);
	struct dmxdev *dev     = dmx->dev;
	struct dmx_dvr *dvr    = &dmx->dvr;
	struct dvr_info *info = &dvr->tsr_info;

	if (info->flow_control == 0)
		return size;

	for (i = 0; i < MAX_AVSLOT_FOR_MULTI_TRACK; i++) {
		struct dmx_slot *slot = NULL;
		struct demux_fifo *avfifo = NULL;
		struct gxav_channel *channel = NULL;

		if ((slot = dmx->slots[i]) == NULL)
			continue;

		if (info->flow_control & DVR_FLOW_CONTROL_ES) {
			if (slot->type != DEMUX_SLOT_AUDIO && slot->type != DEMUX_SLOT_VIDEO)
				continue;

			if (slot->avid < 0 || slot->avid >= MAX_AVBUF_NUM)
				continue;

			avfifo = &dev->avfifo[slot->avid];
			if (NULL == avfifo->channel)
				continue;

			channel = (struct gxav_channel *)avfifo->channel;
			gxav_sdc_length_get(channel->channel_id, &avlen);
			avcap = avfifo->buffer_end_addr - avfifo->buffer_start_addr + 1;
			avfree = (avcap * 7 / 8);
			avfree = (avfree > avlen) ? (avfree - avlen) : 0;
			size = GX_MIN(size, avfree);
		}
	}

	return size/align*align;
}

int dvr_normal_read(struct gxav_module *module, void *buf, unsigned int size)
{
	struct gxfifo *fifo = NULL;
	unsigned int fifo_len = 0;
	unsigned int read_len = 0;
	unsigned int align = 188;

	fifo = dvr_get_tsw_fifo(module->sub, module->module_id);
	if (NULL == fifo)
		return 0;

	fifo_len = gxfifo_len(fifo);
	if (fifo_len < align)
		return 0;

	read_len = size/align*align;
	return gxfifo_user_get(fifo, buf, read_len);
}

int dvr_normal_write(struct gxav_module *module, void *buf, unsigned int size)
{
	int sub = module->sub%4;
	struct dmx_demux *demux = get_subdemux(sub);
	struct demux_dvr_ops *dvr_ops = demux->dev->dvr_ops;
	struct gxfifo *fifo = NULL;
	struct dmx_dvr *dvr = NULL;
	struct dvr_info *info = NULL;
	unsigned int fifo_len = 0;
	unsigned int align = 188*4;

	fifo = &demux->infifo;
	dvr = &demux->dvr;
	info = (module->sub < MAX_DMX_NORMAL) ? &dvr->tsr_info : &dvr->tsr23_info;

	if (info->status != DVR_STATE_BUSY && info->status != DVR_STATE_SHARE_MAJOR)
		return 0;

	size = dvr_tsr_flow_control(sub, size);

	gxdemux_dvr_lock();
	if (dvr_ops->tsr_dealwith)
		dvr_ops->tsr_dealwith(demux->dev, module->sub, dvr->id);

	fifo_len = gxfifo_freelen(fifo);
	fifo_len = fifo_len/align*align;
	if (fifo_len < size) {
		gxdemux_dvr_unlock();
		return 0;
	}

	if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSR)) {
		unsigned int cur_len = 0, trans_len;
		if (NULL == info->trans_temp_buf) {
			gxlog_e(LOG_DVR, "TSR's temp buffer is NULL.\n");
			gxdemux_dvr_unlock();
			return -1;
		}

		while (cur_len < size) {
			if (size - cur_len >= BUFFER_TS_TRANS_UNIT)
				trans_len = BUFFER_TS_TRANS_UNIT;
			else
				trans_len = size-cur_len;
			gx_copy_from_user(info->trans_temp_buf, buf+cur_len, trans_len);
			gxfifo_put(fifo, info->trans_temp_buf, trans_len);

			if (info->flags & DVR_FLAG_DVR_TO_DSP) {
				unsigned int wptr = 0, split = 0, left = trans_len;
				gxav_sdc_rwaddr_get(info->link_channel_id, NULL, &wptr);

				if (wptr + trans_len > info->link_size) {
					split = info->link_size - wptr;
					gxm2m_decrypt((void *)info->link_vaddr+wptr, info->trans_temp_buf, split);
					left = trans_len - split;
					wptr = 0;
				}

				gxm2m_decrypt((void *)info->link_vaddr+wptr, info->trans_temp_buf+split, left);
				gxav_sdc_length_set(info->link_channel_id, trans_len, GXAV_SDC_WRITE);
			}
			cur_len += trans_len;
		}
	} else
		gxfifo_user_put(fifo, buf, size);

	if (dvr_ops->tsr_dealwith)
		dvr_ops->tsr_dealwith(demux->dev, module->sub, dvr->id);
	gxdemux_dvr_unlock();

	return size;
}

unsigned int dvr_get_tsw_ptr_todolen(struct dvr_info *info)
{
	struct gxfifo *fifo = NULL;
	int fifolen = 0;
	unsigned int align = 188*4;
	unsigned int rptr = 0, todo = 0, split = 0;

	if (NULL == info)
		return 0;

	fifo = &info->fifo;
	rptr = gxfifo_rptr(fifo);
	split = info->hw_buffer_size - rptr;
	if ((fifolen = gxfifo_len(fifo)) == 0)
		return 0;

	if (fifolen % align == 0)
		fifolen -= align;

	todo = fifolen / align * align;
	return GX_MIN(todo, split);
}

int dvr_shallow_read(struct gxav_module *module, GxDvrProperty_PtrInfo *pinfo)
{
	int i, is_tsr = 0;
	int sub = module->sub%4;
	struct gxfifo *fifo = NULL;
	unsigned int blocklen = pinfo->blocklen;
	unsigned int fifolen = 0;

	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev  *dev = demux->dev;
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_info *info = NULL;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	if (dvr->tsr_info.status) {
		info = &dvr->tsr_info;
		is_tsr = 1;

	} else {
		for (i = 0; i < dev->max_tsw; i++) {
			if (dvr->tsw_info[i].dvr_handle == module->module_id) {
				info = &dvr->tsw_info[i];
				break;
			}
		}
	}

	if (NULL == info || info->status == 0) {
		gxlog_e(LOG_DVR, "Can't find the dvr info!\n");
		return -1;
	}

	if ((info->flags & DVR_FLAG_PTR_MODE_EN) == 0) {
		gxlog_e(LOG_DVR, "The dvr info is not in PTR mode!\n");
		return -1;
	}

	if (is_tsr) {
		if (NULL == (fifo = &demux->infifo)) {
			gxlog_e(LOG_DVR, "The dvr info's fifo is NULL\n");
			return -1;
		}

		if ((fifolen = gxfifo_freelen(fifo)/blocklen*blocklen) < blocklen)
			return 0;

		pinfo->paddr = info->paddr + gxfifo_wptr(fifo);
		pinfo->size  = fifolen;

	} else {
		if (NULL == (fifo = &info->fifo)) {
			gxlog_e(LOG_DVR, "The dvr info's fifo is NULL\n");
			return -1;
		}

		if ((fifolen = dvr_get_tsw_ptr_todolen(info)) < blocklen) {
			if (dvr_ops->tsw_isr) {
				dvr_ops->tsw_isr(dev, 1);
				if ((fifolen = dvr_get_tsw_ptr_todolen(info)) < blocklen)
					return 0;
			}
		}

		pinfo->paddr = info->paddr + gxfifo_rptr(fifo);
		pinfo->size  = GX_MIN(fifolen, blocklen);
	}

	return 0;
}

int dvr_shallow_write(struct gxav_module *module, GxDvrProperty_PtrInfo *pinfo)
{
	int i = 0, is_tsr = 0;
	int sub = module->sub%4;
	struct gxfifo *fifo = NULL;
	unsigned int blocklen = pinfo->blocklen;

	struct dmx_demux *demux = get_subdemux(sub);
	struct dmxdev  *dev = demux->dev;
	struct dmx_dvr *dvr = &demux->dvr;
	struct dvr_info *info = NULL;
	struct demux_dvr_ops *dvr_ops = dev->dvr_ops;

	if (dvr->tsr_info.status) {
		info = &dvr->tsr_info;
		is_tsr = 1;

	} else {
		for (i = 0; i < dev->max_tsw; i++) {
			if (dvr->tsw_info[i].dvr_handle == module->module_id) {
				info = &dvr->tsw_info[i];
				break;
			}
		}
	}

	if (NULL == info || info->status == 0) {
		gxlog_e(LOG_DVR, "Can't find the dvr info!\n");
		return -1;
	}

	if ((info->flags & DVR_FLAG_PTR_MODE_EN) == 0) {
		gxlog_e(LOG_DVR, "The dvr info is not in PTR mode!\n");
		return -1;
	}

	if ((pinfo->size = pinfo->size/blocklen*blocklen) == 0)
		return 0;

	if (is_tsr) {
		if (NULL == (fifo = &demux->infifo)) {
			gxlog_e(LOG_DVR, "The dvr info's fifo is NULL\n");
			return -1;
		}

		gxfifo_shallow_put(fifo, NULL, pinfo->size);
		if (dvr_ops->tsr_dealwith)
			dvr_ops->tsr_dealwith(dev, module->sub, dvr->id);
	} else {
		unsigned int todo = 0;
		unsigned int rptr = 0;

		if (NULL == (fifo = &info->fifo)) {
			gxlog_e(LOG_DVR, "The dvr info's fifo is NULL\n");
			return -1;
		}

		rptr = gxfifo_rptr(fifo);
		if ((todo = dvr_get_tsw_ptr_todolen(info)) < blocklen)
			return 0;

		todo = GX_MIN(todo, blocklen);
		if (info->paddr + rptr != pinfo->paddr) {
			gxlog_e(LOG_DVR, "ptr info error %x %x %x %x\n", info->paddr+rptr,todo, pinfo->paddr,pinfo->size);
			return -1;
		}

		rptr = (info->hw_buffer_size + rptr + todo) % info->hw_buffer_size;
		gxfifo_shallow_get(fifo, NULL, todo);
		if (dvr_ops->tsw_update_rptr)
			dvr_ops->tsw_update_rptr(dev, i, rptr);
	}

	return 0;
}

int dvr_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0;

	gxdemux_lock();
	switch (property_id) {
	case GxAVGenericPropertyID_ModuleLinkChannel:
	case GxAVGenericPropertyID_ModuleUnLinkChannel:
		{
			struct fifo_info *module_set_fifo = (struct fifo_info *)property;
			struct demux_fifo dvr_fifo;

			gxav_channel_get_phys(module_set_fifo->channel,
					&(dvr_fifo.buffer_start_addr),
					&(dvr_fifo.buffer_end_addr),
					&(dvr_fifo.buffer_id));
			dvr_fifo.channel_id = gxav_channel_id_get(module_set_fifo->channel);
			dvr_fifo.pin_id     = module_set_fifo->pin_id;
			dvr_fifo.direction  = module_set_fifo->dir;
			dvr_fifo.channel    = module_set_fifo->channel;

			if (GxAVGenericPropertyID_ModuleLinkChannel == property_id)
				ret = dvr_tsw_link_fifo(module->sub, module->module_id, &dvr_fifo);
			else
				ret = dvr_tsw_unlink_fifo(module->sub, module->module_id, &dvr_fifo);
		}
		break;

	case GxDvrPropertyID_CryptConfig:
		{
			GxDvrProperty_CryptConfig *config = (GxDvrProperty_CryptConfig *) property;

			if (NULL == property || size != sizeof(GxDvrProperty_CryptConfig )) {
				gxlog_e(LOG_DVR, "the dvr module's configure pvr param error!\n");
				goto err;
			}

			if (dvr_is_busy()) {
				gxlog_e(LOG_DVR, "dvr is already running\n");
				goto err;
			}

			if (dvr_buffer_status_set(config) < 0) {
				gxlog_e(LOG_DVR, "dvr buffer status set error\n");
				goto err;
			}

			if (dvr_buffer_protected(GXAV_BUFFER_DEMUX_TSW)) {

				if (gxm2m_dvr_config(config) < 0) {
					gxlog_e(LOG_DVR, "dvr set config error\n");
					goto err;
				}
			}
		}
		break;
	case GxDvrPropertyID_Config:
		{
			GxDvrProperty_Config *config = (GxDvrProperty_Config *) property;

			if (NULL == property || size != sizeof(GxDvrProperty_Config )) {
				gxlog_e(LOG_DVR, "the dvr module's configure param error!\n");
				goto err;
			}

			if (dvr_config(module->sub, module->module_id, config) < 0) {
				gxlog_e(LOG_DVR, "dvr set config error\n");
				goto err;
			}
		}
		break;

	case GxDvrPropertyID_Run:
		if (dvr_run(module->sub, module->module_id) < 0) {
			gxlog_e(LOG_DVR, "dvr run failed.\n");
			goto err;
		}
		break;

	case GxDvrPropertyID_Stop:
		if (dvr_stop(module->sub, module->module_id) < 0) {
			gxlog_e(LOG_DVR, "dvr stop failed.\n");
			goto err;
		}
		break;

	case GxDvrPropertyID_Reset:
		{
			GxDvrProperty_Reset *reset = (GxDvrProperty_Reset *)property;
			if (NULL == property || size != sizeof(GxDvrProperty_Reset)) {
				gxlog_e(LOG_DVR, "the dvr module's reset param error!\n");
				goto err;
			}

			ret = dvr_reset(module->sub, module->module_id, reset);
		}
		break;

	case GxDvrPropertyID_TSRShare:
		{
			GxDvrProperty_TSRShare *share = (GxDvrProperty_TSRShare *)property;
			if (NULL == property || size != sizeof(GxDvrProperty_TSRShare)) {
				gxlog_e(LOG_DVR, "the dvr module's tsr share param error!\n");
				goto err;
			}

			ret = dvr_tsr_share_config(module->sub, module->module_id, share);
		}
		break;

	case GxDvrPropertyID_TSRFlowControl:
		{
			GxDvrProperty_TSRFlowControl *ctrl = (GxDvrProperty_TSRFlowControl *)property;
			if (NULL == property || size != sizeof(GxDvrProperty_TSRFlowControl)) {
				gxlog_e(LOG_DVR, "the dvr module's tsr flow control param error!\n");
				goto err;
			}

			ret = dvr_tsr_flow_control_set(module->sub, ctrl);
		}
		break;

	case GxDvrPropertyID_PtrInfo:
		{
			GxDvrProperty_PtrInfo *pinfo = (GxDvrProperty_PtrInfo *)property;
			if (NULL == property || size != sizeof(GxDvrProperty_PtrInfo)) {
				gxlog_e(LOG_DVR, "SetPtrInfo: the param error!\n");
				goto err;
			}
			ret = dvr_shallow_write(module, pinfo);
		}
		break;

	default:
		gxlog_e(LOG_DVR, "dvr_id(%d)'s property_id(%d) set wrong \n", module->sub, property_id);
		goto err;

	}

	gxdemux_unlock();
	return ret;
err:
	gxdemux_unlock();
	return -1;
}

int dvr_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0;
	if ((NULL == module) || (module->sub < 0) ||
		(module->sub > module->inode->count) || (NULL == property)) {
		gxlog_e(LOG_DVR, "Module is NULL.\n");
		return -3;
	}

	gxdemux_lock();
	switch (property_id) {
	case GxDvrPropertyID_Config:
		{
			GxDvrProperty_Config *config = (GxDvrProperty_Config *) property;

			if (size != sizeof(GxDvrProperty_Config )) {
				gxlog_e(LOG_DVR, "the dvr module's attribute param error!\n");
				goto err;
			}

			if (dvr_attribute(module->sub, module->module_id, config) < 0) {
				gxlog_e(LOG_DVR, "dvr attribute error\n");
				goto err;
			}
		}
		break;

	case GxDvrPropertyID_PtrInfo:
		{
			GxDvrProperty_PtrInfo *pinfo = (GxDvrProperty_PtrInfo *)property;
			if (size != sizeof(GxDvrProperty_PtrInfo)) {
				gxlog_e(LOG_DVR, "GetPtrInfo: the param error!\n");
				goto err;
			}
			ret = dvr_shallow_read(module, pinfo);
		}
		break;

	case GxDvrPropertyID_TSRFlowControl:
		{
			GxDvrProperty_TSRFlowControl *ctrl = (GxDvrProperty_TSRFlowControl *)property;
			if (NULL == property || size != sizeof(GxDvrProperty_TSRFlowControl)) {
				gxlog_e(LOG_DVR, "the dvr module's tsr flow control param error!\n");
				goto err;
			}

			ret = dvr_tsr_flow_control_get(module->sub, ctrl);
		}
		break;

	default:
		gxlog_e(LOG_DVR, "dvr_id(%d)'s property_id(%d) get wrong \n", module->sub, property_id);
		goto err;
	}

	gxdemux_unlock();
	return ret;
err:
	gxdemux_unlock();
	return -1;
}

int dvr_set_entry(struct gxav_module *module, int property_id, void *property, int size)
{
	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count)) {
		gxlog_e(LOG_DVR, "Module is NULL.\n");
		return -3;
	}

	if (module->sub < MAX_DMX_TSW_SLOT)
		return dvr_set_property(module, property_id, property, size);
#ifdef CONFIG_AV_MODULE_PIDFILTER
	else if (module->sub < MAX_DMX_PID_FILTER)
		return dvr_pfilter_set_property(module, property_id, property, size);
#endif
#ifdef CONFIG_AV_MODULE_GSE
	else if (module->sub < MAX_DMX_GSE)
		return dmx_gse_set_property(module, property_id, property, size);
#endif

	return -1;
}

int dvr_get_entry(struct gxav_module *module, int property_id, void *property, int size)
{
	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count)) {
		gxlog_e(LOG_DVR, "Module is NULL.\n");
		return -3;
	}

	if (NULL == property) {
		gxlog_e(LOG_DVR, "the property param is NULL!\n");
		return -1;
	}

	if (module->sub < MAX_DMX_PID_FILTER)
		return dvr_get_property(module, property_id, property, size);
#ifdef CONFIG_AV_MODULE_GSE
	else if (module->sub < MAX_DMX_GSE)
		return dmx_gse_get_property(module, property_id, property, size);
#endif

	return -1;
}

int dvr_read(struct gxav_module *module, void *buf, unsigned int size)
{
	if ((NULL == module) || (module->sub < 0) ||
		(module->sub > module->inode->count) || (NULL == buf)) {
		gxlog_e(LOG_DVR, "Module is NULL.\n");
		return -3;
	}

	if (module->sub < MAX_DMX_PID_FILTER)
		return dvr_normal_read(module, buf, size);

#ifdef CONFIG_AV_MODULE_GSE
	else if (module->sub >= MAX_DMX_PID_FILTER && module->sub < MAX_DMX_GSE)
		return dvr_gse_read(module, buf, size);
#endif

	return -1;
}

int dvr_write(struct gxav_module *module, void *buf, unsigned int size)
{
	if ((NULL == module) || (module->sub < 0) ||
		(module->sub > module->inode->count) || (NULL == buf)) {
		gxlog_e(LOG_DVR, "Module is NULL.\n");
		return -3;
	}

	if (module->sub < MAX_DMX_TSW_SLOT)
		return dvr_normal_write(module, buf, size);

	return -1;
}

struct gxav_module_inode* dvr_irq(struct gxav_module_inode *inode, int irq)
{
	struct dmx_demux *demux = get_subdemux(0);
	struct demux_dvr_ops *dvr_ops = demux->dev->dvr_ops;

	if (dvr_ops->irq_entry)
		dvr_ops->irq_entry(inode, irq);

	return inode;
}

EXPORT_SYMBOL(gxav_dvr_set_protocol_callback);
