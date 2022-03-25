#include "kernelcalls.h"
#include "gxav_sdc_propertytypes.h"
#include "avcore.h"
#include "gxav_bitops.h"
#include "gxav_event_type.h"
#include "pts_fetcher.h"

#include "sdc_hal.h"

struct gxav_sdc module_sdc;

static int PTS_LEN = 16*1024;
static int PTS_ONCE_LEN = 8;
static PtsFetcher _channel_ptsfetcher[GXAV_MAX_CHANNEL];

#define CHECK_RET(ret) \
	if(ret < 0)\
	{\
		gx_printf("(%s) return Error!!!\n",__func__);\
		return -1;\
	}

static void gx_sdc_reset_ptsfetcher(struct gxav_channel *channel)
{
#if defined CONFIG_AV_MODULE_AUDIO_DEC || defined CONFIG_AV_MODULE_VIDEO_DEC
	if (channel->type & GXAV_PTS_FIFO) {
		RingBufInfo esbuf_info, ptsbuf_info;

		esbuf_info.id    = channel->channel_id;
		esbuf_info.addr  = gx_virt_to_phys((unsigned int)(channel->buffer));
		esbuf_info.size  = channel->size;
		ptsbuf_info.id   = channel->pts_channel_id;
		ptsbuf_info.addr = (unsigned int)(channel->pts_buffer);
		ptsbuf_info.size = channel->pts_size;
		ptsfetcher_init(&_channel_ptsfetcher[channel->channel_id], &esbuf_info, &ptsbuf_info);
	}
#endif
}

int gx_sdc_apply(struct gxav_channel *channel, GxAvChannelType type)
{
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	channel->flag = RW;
	channel->type = type;
	channel->almost_empty_gate = (channel->size >> 2);
	channel->almost_full_gate  = ((channel->size * 3) >> 2);

	ret = gxav_sdc_apply(channel->channel_id, channel->size);
	CHECK_RET(ret);

	ret = gxav_sdc_algate_set(channel->channel_id,
			channel->almost_empty_gate,(channel->size - channel->almost_full_gate));
	CHECK_RET(ret);

	if (GXAV_PTS_FIFO & type)
	{
		channel->pts_buffer = gx_dma_malloc(PTS_LEN * sizeof(char));
		if(channel->pts_buffer == NULL)
		{
			gx_printf("%s(),pts buffer page malloc fail\n",__func__);
			channel->pts_buffer = NULL;
			return -1;
		}

		gx_memset(channel->pts_buffer, 0, (PTS_LEN * sizeof(char)));
		channel->pts_size = PTS_LEN;
		channel->pts_pos = 0;
		SDC_DBG("channel->pts_buffer = %p,channel->pts_size = 0x%x\n", channel->pts_buffer, channel->pts_size);

		ret = gxav_sdc_apply(channel->pts_channel_id,channel->pts_size);
		CHECK_RET(ret);

		ret = gxav_sdc_algate_set(channel->pts_channel_id, channel->pts_size - 32, PTS_ONCE_LEN);
		CHECK_RET(ret);

		//pts fetcher
		gx_sdc_reset_ptsfetcher(channel);
	}

	return 0;
}

int gx_sdc_free(struct gxav_channel *channel, GxAvChannelType type)
{
	int ret = 0;

	GXAV_ASSERT(channel != NULL);
	GXAV_ASSERT(gxav_sdc_free != NULL);

	ret = gxav_sdc_free(channel->channel_id);
	CHECK_RET(ret);

	if (channel->type & GXAV_PTS_FIFO)
	{
		ret = gxav_sdc_free(channel->pts_channel_id);
		CHECK_RET(ret);

		SDC_DBG("channel->pts_buffer = %p,channel->pts_size = 0x%x\n",channel->pts_buffer, channel->pts_size);
		gx_dma_free(channel->pts_buffer, channel->pts_size);
		channel->pts_buffer = NULL;
		channel->pts_size = 0;
		channel->pts_pos = 0;
	}

	return 0;
}

int gx_sdc_reset(struct gxav_channel *channel)
{
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_buffer_reset(channel->channel_id);
	CHECK_RET(ret);

	SDC_DBG("channel id=%d\n",channel->channel_id);

	if (GXAV_PTS_FIFO & channel->type)
	{
		ret = gxav_sdc_buffer_reset(channel->pts_channel_id);
		CHECK_RET(ret);

		//pts fetcher
		gx_sdc_reset_ptsfetcher(channel);

		SDC_DBG("pts channel id=%d\n",channel->pts_channel_id);
	}

	return 0;
}

int gx_sdc_flush(struct gxav_channel *channel)
{
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_buffer_flush(channel->channel_id);
	CHECK_RET(ret);

	SDC_DBG("channel id = %d\n",channel->channel_id);

	if (GXAV_PTS_FIFO & channel->type)
	{
		ret = gxav_sdc_buffer_flush(channel->pts_channel_id);
		CHECK_RET(ret);

		SDC_DBG("pts channel id=%d\n",channel->pts_channel_id);
	}

	return 0;
}

int gx_sdc_gate_set(struct gxav_channel *channel, void *info)
{
	struct gxav_gate_info *gate_info = info;
	int ret = 0;

	GXAV_ASSERT(channel != NULL);
	GXAV_ASSERT(info != NULL);

	if (gate_info->almost_empty >= channel->size)
	{
		gx_printf("%s(),####WARRING#####,almost_empty gate=0x%x exceed cap size=0x%x.\n",
			__func__,gate_info->almost_empty,channel->size);
		gate_info->almost_empty = 0;
	}

	if (gate_info->almost_full > channel->size)
	{
		gx_printf("%s(),####WARRING#####,almost_full gate=0x%x exceed cap size=0x%x.\n",
			__func__,gate_info->almost_full,channel->size);
		gate_info->almost_full = 0;
	}

	SDC_DBG("almost_empty=%d\n",gate_info->almost_empty);
	SDC_DBG("almost_full=%d\n", gate_info->almost_full);

	ret = gxav_sdc_algate_set(channel->channel_id,
			    gate_info->almost_empty,
			    gate_info->almost_full);
	CHECK_RET(ret);

	channel->almost_empty_gate = gate_info->almost_empty;
	channel->almost_full_gate  = gate_info->almost_full;

	return 0;
}

int gx_sdc_pts_set(struct gxav_channel *channel, unsigned int start_addr, int pts)
{
	unsigned int start = 0,pts_len = 0;
	unsigned int counter = 0;
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	start = (unsigned int)channel->pts_buffer;
	pts_len = channel->pts_size;

	ret = gxav_sdc_length_get(channel->pts_channel_id,&counter);
	CHECK_RET(ret);

	//pts sdc free size
	if((pts_len - counter) <= PTS_ONCE_LEN)
	{
		SDC_DBG("####WARRING####,pts channnel id(%d) already full now\n",channel->pts_channel_id);
		return 0;
	}

	SDC_DBG("last_start_addr : 0x%x,start_addr : 0x%x,pts_times : %d,pts : 0x%x\n",
		channel->last_start_addr,start_addr,channel->ptstimes,pts);

	//CPU 读 PTS, 大小端不取反，写sdram
	// 3201 不需要折回次数
	GX_SET_VAL((start + channel->pts_pos), gx_virt_to_phys(start_addr));
	channel->pts_pos = ((channel->pts_pos + 4) % pts_len);

	GX_SET_VAL((start + channel->pts_pos),pts);
	channel->pts_pos = ((channel->pts_pos + 4) % pts_len);

	//inc pts sdc
	ret = gxav_sdc_length_set(channel->pts_channel_id, PTS_ONCE_LEN ,GXAV_SDC_WRITE);
	CHECK_RET(ret);

	return PTS_ONCE_LEN;
}

int gx_sdc_pts_get(struct gxav_channel *channel, int* pts)
{
	unsigned int rd_addr;
	GXAV_ASSERT(channel != NULL);

	gxav_sdc_rwaddr_get(channel->channel_id, &rd_addr, NULL);
#if defined CONFIG_AV_MODULE_AUDIO_DEC || defined CONFIG_AV_MODULE_VIDEO_DEC
	ptsfetcher_fetch(&_channel_ptsfetcher[channel->channel_id], rd_addr, (unsigned int *)pts);
#endif
	return 0;
}

int gx_sdc_inc(struct gxav_channel *channel, int len)
{
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_length_set(channel->channel_id,len, GXAV_SDC_WRITE);
	CHECK_RET(ret);

	return 0;
}

int gx_sdc_dec(struct gxav_channel *channel, int len)
{
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_length_set(channel->channel_id,len,GXAV_SDC_READ);
	CHECK_RET(ret);

	return 0;
}

int gx_sdc_freesize_get(struct gxav_channel *channel)
{
	unsigned int len = 0;
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_length_get(channel->channel_id,&len);
	CHECK_RET(ret);

	return channel->size - len - 1;
}

int gx_sdc_length_get(struct gxav_channel *channel)
{
	unsigned int len = 0;
	unsigned int ptslen = 0;
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_length_get(channel->channel_id,&len);
	CHECK_RET(ret);

	if(channel->pts_channel_id > 0){
		gxav_sdc_length_get(channel->pts_channel_id, &ptslen);
		if(ptslen + PTS_ONCE_LEN >= PTS_LEN)
			len = channel->size;
	}

	return len;
}

int gx_sdc_cap_get(struct gxav_channel *channel)
{
	GXAV_ASSERT(channel != NULL);
	return channel->size;
}

int gx_sdc_pts_cap(struct gxav_channel *channel)
{
	GXAV_ASSERT(channel != NULL);
	return channel->pts_size;
}

int gx_sdc_pts_freesize(struct gxav_channel *channel)
{
	unsigned int len = 0;
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_length_get(channel->pts_channel_id,&len);
	CHECK_RET(ret);

	return channel->pts_size - len - 1;
}

int gx_sdc_pts_length(struct gxav_channel *channel)
{
	unsigned int len = 0;
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_length_get(channel->pts_channel_id,&len);

	if(ret < 0)
		goto err;

	return len;

err:
	gx_printf("(%s) return Error!!!\n",__func__);
	return -1;
}

int gx_sdc_pts_isfull(struct gxav_channel *channel)
{
	unsigned int freelen = 0;
	int ret = 0;

	GXAV_ASSERT(channel != NULL);

	ret = gxav_sdc_free_get(channel->pts_channel_id, &freelen);
	if (ret < 0)
		goto err;

	return (freelen <= PTS_ONCE_LEN) ? 1 : 0;

err:
	gx_printf("(%s) return Error!!!\n",__func__);
	return 0;
}

int gx_sdc_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	SDC_DBG("init\n");

	gx_memset(&module_sdc, 0, sizeof(module_sdc));

	inode->priv = &module_sdc;

    return gxav_sdc_init(inode->interface);
}

int gx_sdc_open(struct gxav_module *module)
{
	SDC_DBG("open\n");

    return gxav_sdc_open(module);
}

int gx_sdc_close(struct gxav_module *module)
{
	SDC_DBG("close\n");

	if (CHIP_IS_GX3113C  == 1) {
		PTS_LEN = 4*1024;
	}

	gxav_sdc_free_all(module);

    return gxav_sdc_close();
}

int gx_sdc_uninit(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	SDC_DBG("uninit\n");

	gx_sdc_close(inode->module[0]);

    return gxav_sdc_uninit();
}

int gx_sdc_callback(int channel_id, unsigned int len,  unsigned int dataflow, GxAvSdcOPMode mode)
{
	int ret = 0;

	if(mode == GXAV_SDC_READ) {
		if(module_sdc.channel[channel_id].outcallback) {
			ret = module_sdc.channel[channel_id].outcallback( \
				channel_id, len, dataflow,\
				module_sdc.channel[channel_id].outdata);
		}
	}
	else {
		if(module_sdc.channel[channel_id].incallback) {
			ret = module_sdc.channel[channel_id].incallback( \
				channel_id, len, dataflow,\
				module_sdc.channel[channel_id].indata);
		}
	}

	return ret;
}

unsigned long gx_sdc_spin_lock_irqsave(int channel_id)
{
	unsigned long flags = 0;
	gx_spin_lock_irqsave(&module_sdc.channel[channel_id].spin_lock, flags);

	return flags;
}

void gx_sdc_spin_unlock_irqrestore(int channel_id, unsigned long flags)
{
	gx_spin_unlock_irqrestore(&module_sdc.channel[channel_id].spin_lock, flags);
}

void* gx_sdc_get_outdata(int channel_id)
{
	return module_sdc.channel[channel_id].outdata;
}

void* gx_sdc_get_indata(int channel_id)
{
	return module_sdc.channel[channel_id].indata;
}



