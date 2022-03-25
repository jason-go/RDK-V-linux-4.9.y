#include "avcore.h"
#include "kernelcalls.h"
#include "gxav.h"
#include "gxav_bitops.h"
#include "sdc_module.h"
#include "profile.h"
#include "firewall.h"

#define device_get_sdc(dev)  \
	((dev != NULL) ? dev->modules_inode[GXAV_MOD_SDC].priv : NULL)

#define module_get_sdc(module) \
	((module && (module)->inode) ? module->inode->priv : NULL)

#define IS_MODULE_SDC_NULL() \
	(module_sdc == NULL)

#if 0
#define IS_SDC_FIND_CHANNEL(channel) \
	do{                                                                      \
		int i = 0;                                                       \
		GXAV_ASSERT(channel != NULL);                                    \
		for (i = 0; i < GXAV_MAX_CHANNEL; i++) {                         \
			if (channel == &module_sdc->channel[i])                  \
			break;                                                   \
		}                                                                \
		if (i == GXAV_MAX_CHANNEL) {                                     \
			gx_printf("%s(),can't find the channel   \ n",__func__); \
			return -1;                                               \
		}                                                                \
	}while(0);
#else
#define IS_SDC_FIND_CHANNEL(channel) do {if(channel == NULL) return -1;}while(0)
#endif

#define IS_SDC_TYPE_ERROR(type) \
	(type & GXAV_PTS_FIFO && type & GXAV_NO_PTS_FIFO)

#define IS_SDC_NO_PTS_FIFO(type) \
	(type & GXAV_NO_PTS_FIFO)

#define IS_SDC_RETURN_ERROR(Ret) \
	if(Ret < 0) {\
		GXAV_DBG("%s[%d]:%s,####Error####,Ret : %d\n", __FILE__, __LINE__, __FUNCTION__,Ret);\
		return Ret;	\
	}

#define PTS_FIFO       (0x9)

struct gxav_channel *gxav_channel_apply(struct gxav_device *dev, void *buffer, unsigned int len, GxAvChannelType type)
{
	int i = 0,ret = 0;
	signed char j = -1;
	unsigned int id = 0;
	struct gxav_channel *channel = NULL;
	struct gxav_sdc *module_sdc = device_get_sdc(dev);

	if(IS_SDC_TYPE_ERROR(type)) {
		gx_printf("%s()::type error \n",__func__);
		return NULL;
	}
	for (i = 0; i < GXAV_MAX_CHANNEL; i++) {
		id = 1UL << i;
		if ((module_sdc->sdc_status & id) == 0) {
			module_sdc->channel_status |= id;
			module_sdc->sdc_status |= id;

			GXAV_DBG("%s(), module_sdc->channel_status = 0x%x,channel->id=%d\n",
					__func__,module_sdc->channel_status,i);

			if (GXAV_PTS_FIFO & type) {
				for (j = i; j < GXAV_MAX_CHANNEL; j++) {
					id = 1UL << j;
					if ((module_sdc->sdc_status & id) == 0) {
						module_sdc->sdc_status |= id;
						break;
					}
				}
				if (j == GXAV_MAX_CHANNEL) {
					gx_printf("%s(),####ERROR####,AV type fifo had no idle channel\n",__func__);
					module_sdc->channel_status &= ~(1UL << i);
					module_sdc->sdc_status &= ~(1UL << i);
					return NULL;
				}
				GXAV_DBG("%s(), avtype module_sdc->sdc_status = 0x%x,pts_channel->id=%d\n",
						__func__,module_sdc->sdc_status,j);
				gx_memset(&module_sdc->channel[j], 0 , sizeof(struct gxav_channel));
				module_sdc->channel[j].channel_id = j;
				module_sdc->channel[j].pts_channel_id = -1;
				module_sdc->channel[j].type = PTS_FIFO;
				module_sdc->channel[j].flag = RW;
			}

			//config the channel info
			gx_memset(&module_sdc->channel[i], 0 , sizeof(struct gxav_channel));
			channel = &module_sdc->channel[i];
			channel->type = type;
			channel->size = len;
			channel->channel_id = i;
			channel->pts_channel_id = j;

			channel->flag = RW;
			channel->inpin_id = channel->outpin_id = -1;
			channel->incallback = channel->outcallback = NULL;
			channel->indata = channel->outdata = NULL;
			channel->dev = dev;

			GXAV_DBG("%s(), channel->channel_id = %d ,channel->pts_channel_id = %d\n", __func__,
					channel->channel_id,channel->pts_channel_id);

			if(buffer == NULL) {
				channel->buffer = gx_page_malloc(channel->size);
				channel->malloced = 1;
			}
			else
				channel->buffer = buffer;

			if (channel->buffer == NULL) {
				gx_printf("%s()::channel->buffer == NULL \n",__func__);
				goto err;
			}

			gx_dcache_inv_range((unsigned int)channel->buffer, (unsigned)channel->buffer + channel->size);

			ret = gx_sdc_apply(channel, type);
			if(ret < 0) {
				gx_printf("%s(),channel(%d) apply from hardware sdc controller fail\n",
						__func__, channel->channel_id);
				goto err;
			}

			channel->start_addr = channel->last_start_addr = (unsigned int)channel->buffer;
			gx_spin_lock_init(&channel->spin_lock);

			GXAV_DBG("%s(),channel(0x%x),channel->type = %d,channel_status=0x%x,sdc_status=0x%x\n",
					__func__, (unsigned int)channel,
					channel->type, module_sdc->channel_status, module_sdc->sdc_status);
			break;
		}
	}

	return channel;

err:
	gx_sdc_free(channel, channel->type);

	module_sdc->channel_status &= ~(1UL << channel->channel_id);
	module_sdc->sdc_status &= ~(1UL << channel->channel_id);

	if(channel->type & GXAV_PTS_FIFO && channel->pts_channel_id != -1)
		module_sdc->sdc_status &= ~(1UL << channel->pts_channel_id);

	gx_memset(channel, 0, sizeof(struct gxav_channel));
	channel->pts_channel_id = -1;
	channel->channel_id = -1;
	return NULL;
}

int gxav_channel_free(struct gxav_device *dev, struct gxav_channel *channel)
{
	struct gxav_sdc *module_sdc = device_get_sdc(dev);
	int ret = 0, i=0;

	IS_SDC_FIND_CHANNEL(channel);

	if (channel->inpin_id != -1 || channel->outpin_id != -1)
		return -1;

	ret = gx_sdc_free(channel, channel->type);
	IS_SDC_RETURN_ERROR(ret);

	module_sdc->channel_status &= ~(1UL << channel->channel_id);
	module_sdc->sdc_status &= ~(1UL << channel->channel_id);

	if(channel->malloced)
		gx_page_free(channel->buffer, channel->size);

	GXAV_DBG("%s(), module_sdc->channel_status = 0x%x,channel->id=%d\n",
			__func__, module_sdc->channel_status, channel->channel_id);

	GXAV_DBG("%s(),channel(0x%x),channel->type = %d,pts_channel->id=%d\n",
			__func__, (unsigned int)channel, channel->type, channel->pts_channel_id);

	if((channel->type & GXAV_PTS_FIFO) && channel->pts_channel_id != -1) {
		module_sdc->sdc_status &= ~(1UL << channel->pts_channel_id);
		gx_memset(&(module_sdc->channel[channel->pts_channel_id]), 0, sizeof(struct gxav_channel));
		module_sdc->channel[channel->pts_channel_id].channel_id = -1;
		module_sdc->channel[channel->pts_channel_id].pts_channel_id = -1;
	}

	gx_memset(channel, 0, sizeof(struct gxav_channel));
	channel->pts_channel_id = -1;
	channel->channel_id = -1;

	for (i = 0; i < GXAV_MAX_CHANNEL; i++) {
		if (module_sdc->channel[i].flag & R)
			break;
	}

	GXAV_DBG("%s(),channel_status=0x%x,sdc_status=0x%x\n",
			__func__, module_sdc->channel_status, module_sdc->sdc_status);

	return 0;
}

int gxav_channel_reset(struct gxav_device *dev, struct gxav_channel *channel)
{
	int ret = 0;
	unsigned long flags;

	IS_SDC_FIND_CHANNEL(channel);

	ret = gx_sdc_reset(channel);
	IS_SDC_RETURN_ERROR(ret);

	gx_spin_lock_irqsave(&channel->spin_lock, flags);
	channel->in = channel->out = 0;
	channel->in_pos = channel->out_pos = 0;
	channel->pts_pos = 0;
	channel->ptstimes = 0;
	gx_spin_unlock_irqrestore(&channel->spin_lock, flags);

	channel->start_addr = (unsigned int)channel->buffer;
	channel->last_start_addr = (unsigned int)channel->buffer;

	return 0;
}

int gxav_channel_rollback(struct gxav_device *dev, struct gxav_channel *channel, int size)
{
	int ret = 0;
	int pts_cnt = 0;
	unsigned int tmp_pts_addr = 0;
	unsigned int pts_pos_tmp  = 0;
	unsigned long flags;

	IS_SDC_FIND_CHANNEL(channel);

	if(size < 0) {
		size = gx_sdc_length_get(channel);
		pts_cnt = gx_sdc_pts_length(channel);
	}
	else if (size == 0) {
		memset(channel->buffer, 0, channel->size);
		gx_dcache_clean_range((unsigned int)channel->buffer, (unsigned)channel->buffer + channel->size);
		return 0;
	}

	gx_spin_lock_irqsave(&channel->spin_lock, flags);
	if(channel->in_pos >= size) {
		channel->in_pos -= size;
	} else {
		channel->in_pos = channel->in_pos + channel->size - size;
	}
	channel->last_start_addr = channel->in_pos + (unsigned int)channel->buffer;

	if(channel->pts_pos >= pts_cnt) {
		channel->pts_pos -= pts_cnt;
	} else {
		channel->pts_pos = channel->pts_pos + channel->pts_size - pts_cnt;
	}

	if(channel->pts_pos >= 8) {
		pts_pos_tmp = channel->pts_pos - 8;
	} else {
		pts_pos_tmp = channel->pts_pos + channel->pts_size - 8;
	}

	tmp_pts_addr = *(unsigned int*)(&channel->pts_buffer[pts_pos_tmp]);
	CHANGE_ENDIAN_32(tmp_pts_addr);
	channel->ptstimes = tmp_pts_addr >> 28;
	gx_spin_unlock_irqrestore(&channel->spin_lock, flags);

	ret = gx_sdc_flush(channel);

	IS_SDC_RETURN_ERROR(ret);

	return 0;
}


int gxav_channel_put(struct gxav_device *dev, struct gxav_channel *channel,
		void* (*copy) (void *, const void *, int),
		unsigned char *data, unsigned int len, int pts)
{
	int ret = 0, in_pos = 0;
	unsigned char *buffer_addr_start;
	unsigned int first_len, hwfree_len;
	unsigned long flags;

	IS_SDC_FIND_CHANNEL(channel);

	if (channel->flag & W) {

		gx_spin_lock_irqsave(&channel->spin_lock, flags);
		in_pos = channel->in_pos % channel->size;

		copy = ((copy == NULL) ? gx_sdc_memcpy : copy);

		buffer_addr_start = channel->buffer + in_pos;

		hwfree_len = gx_sdc_freesize_get(channel);

		len = GX_MIN(len, hwfree_len);

		first_len = GX_MIN(len, channel->size - in_pos);

		gx_spin_unlock_irqrestore(&channel->spin_lock, flags);

		if (data != NULL) {
			copy(buffer_addr_start, data, first_len);

			if (first_len < len) {
				copy(channel->buffer, data + first_len, len - first_len);
			}
		}

		gx_spin_lock_irqsave(&channel->spin_lock, flags);
		channel->in_pos += len;
		channel->in_pos %= channel->size;
		channel->in += len;
		gx_spin_unlock_irqrestore(&channel->spin_lock, flags);

		gx_dcache_clean_range((unsigned int)channel->buffer, (unsigned)channel->buffer + channel->size);

		ret = gx_sdc_inc(channel, len);
		IS_SDC_RETURN_ERROR(ret);

		if (pts != -1) {
			GXAV_ASSERT(gx_sdc_pts_set != NULL);
			channel->start_addr = (unsigned int)buffer_addr_start;
		}

		if(channel->last_start_addr > channel->start_addr) {
			channel->ptstimes ++;
			channel->ptstimes &= 0x0f;
		}

		channel->last_start_addr = channel->start_addr;

		return len;
	}

	return -1;
}

int gxav_channel_put_pts(struct gxav_device *dev, struct gxav_channel *channel,int pts)
{
	int ret;
	static unsigned int last_pts[GXAV_MAX_CHANNEL] = {0};

	IS_SDC_FIND_CHANNEL(channel);

	if(IS_SDC_NO_PTS_FIFO(channel->type))
		return -1;

	if(pts == -1)
		return -1;

	if(pts != last_pts[channel->channel_id]) {
		last_pts[channel->channel_id] = pts;
		ret = gx_sdc_pts_set(channel, channel->start_addr, pts);
	} else {
		return 8;
	}

	return ret;
}

int gxav_channel_get_pts(struct gxav_device *dev, struct gxav_channel *channel, int *pts)
{
	IS_SDC_FIND_CHANNEL(channel);

	if(IS_SDC_NO_PTS_FIFO(channel->type)) {
		if (pts != NULL)
			*pts = -1;
		return -1;
	}

	return gx_sdc_pts_get(channel, pts);
}

int gxav_channel_get(struct gxav_device *dev, struct gxav_channel *channel,
		void *(*copy) (void *, const void *, int),
		unsigned char *data, unsigned int len, int peek)
{
	IS_SDC_FIND_CHANNEL(channel);

	if (channel->flag & R) {
		unsigned int first_len;
		unsigned int hw_len;
		unsigned int out_pos;
		unsigned long flags = 0;
		int ret;

		copy = ((copy == NULL) ? gx_sdc_memcpy : copy);

		hw_len = gx_sdc_length_get(channel);

		len = GX_MIN(len, hw_len);
		if(len <= 0){
			return 0;
		}

		gx_spin_lock_irqsave(&channel->spin_lock, flags);
		out_pos = channel->out_pos;
		if(!peek) {
			channel->out_pos += len;
			channel->out_pos %= channel->size;
			channel->out += len;
		}
		first_len = GX_MIN(len, channel->size - out_pos);
		gx_spin_unlock_irqrestore(&channel->spin_lock, flags);

		gx_dcache_inv_range((unsigned int)channel->buffer, (unsigned)channel->buffer + channel->size);
		if (data != NULL) {
			copy(data, channel->buffer + out_pos, first_len);
			if(first_len < len){
				copy(data+first_len, channel->buffer, len - first_len);
			}
		}

		if(!peek) {
			ret = gx_sdc_dec(channel, len);
			IS_SDC_RETURN_ERROR(ret);
		}

		return len ;
	}

	return -1;
}

int gxav_channel_skip(struct gxav_device *dev, struct gxav_channel *channel,unsigned int len)
{
	IS_SDC_FIND_CHANNEL(channel);

	if (channel->flag & R) {
		unsigned int first_len;
		unsigned int hw_len;
		unsigned int out_pos;
		unsigned long flags = 0;
		int ret;

		hw_len = gx_sdc_length_get(channel);

		len = GX_MIN(len, hw_len);
		if(len <= 0){
			return 0;
		}

		gx_spin_lock_irqsave(&channel->spin_lock, flags);
		out_pos = channel->out_pos;
		channel->out_pos += len;
		channel->out_pos %= channel->size;
		channel->out += len;
		first_len = GX_MIN(len, channel->size - out_pos);
		gx_spin_unlock_irqrestore(&channel->spin_lock, flags);

		GXAV_DBG("%s(),channel id=0x%x,skip len=%d\n",__func__, channel->channel_id,len);

		ret = gx_sdc_dec(channel, len);
		IS_SDC_RETURN_ERROR(ret);

		return len ;
	}

	return -1;
}

int gxav_channel_gate_set(struct gxav_device *dev, struct gxav_channel *channel, void *info)
{
	int ret = 0;

	IS_SDC_FIND_CHANNEL(channel);

	ret = gx_sdc_gate_set(channel, info);
	IS_SDC_RETURN_ERROR(ret);

	return 0;
}

int gxav_channel_freesize_get(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return gx_sdc_freesize_get(channel);
}

int gxav_channel_length_get(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return gx_sdc_length_get(channel);
}

int gxav_channel_cap_get(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return gx_sdc_cap_get(channel);
}

int gxav_channel_pts_cap(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return gx_sdc_pts_cap(channel);
}

int gxav_channel_pts_freesize(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return gx_sdc_pts_freesize(channel);
}

int gxav_channel_pts_length(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return gx_sdc_pts_length(channel);
}

int gxav_channel_pts_isfull(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return gx_sdc_pts_isfull(channel);
}

int gxav_channel_flag_get(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return channel->flag;
}

int gxav_channel_flag_set(struct gxav_device *dev, struct gxav_channel *channel, int flag)
{
	IS_SDC_FIND_CHANNEL(channel);

	channel->flag = flag;

	return 0;
}

int gxav_channel_type_get(struct gxav_device *dev, struct gxav_channel *channel)
{
	IS_SDC_FIND_CHANNEL(channel);

	return channel->type;
}

int gxav_channel_id_get(struct gxav_channel *channel)
{
	GXAV_ASSERT(channel != NULL);

	return (int)channel->channel_id;
}

int gxav_channel_pts_id_get(struct gxav_channel *channel)
{
	GXAV_ASSERT(channel != NULL);

	return (int)channel->pts_channel_id;
}

int gxav_channel_collation(struct gxav_device *dev, struct gxav_channel *channel)
{
	return -1;
}

struct gxav_channel *gxav_channel_get_channel(struct gxav_device *dev,unsigned char channel_id)
{
	int i;
	struct gxav_sdc *module_sdc = device_get_sdc(dev);
	struct gxav_channel *channel = NULL;

	for (i = 0; i < GXAV_MAX_CHANNEL; i++) {
		channel = &module_sdc->channel[i];
		GXAV_DBG("%s(),channel->channel_id = %d\n",__func__,channel->channel_id);
		if (channel_id == channel->channel_id && channel->channel_id != -1) {
			GXAV_DBG("%s(),channel %d,channel_id %d\n",__func__,i,channel->channel_id);
			return channel;
		}
	}

	return NULL;
}

int gxav_channel_get_phys(struct gxav_channel *channel, unsigned int *start_addr, unsigned int *end_addr,
		unsigned char *buf_id)
{
	unsigned char filter_num = channel->channel_id + 1;

	GXAV_ASSERT(channel != NULL);

	*start_addr = gx_virt_to_phys((unsigned int)channel->buffer);
	*end_addr = gx_virt_to_phys((unsigned int)(channel->buffer + channel->size - 1));
	*buf_id = filter_num;
	GXAV_DBG("%s(): channel->buffer = %p,channel->size = 0x%x\n",__func__,channel->buffer,channel->size);
	GXAV_DBG("%s(): start_addr = 0x%x,end_addr = 0x%x,buf_id=0x%x\n",__func__,*start_addr,*end_addr,*buf_id);

	return 0;
}

int gxav_channel_get_ptsbuffer(struct gxav_channel *channel, unsigned int *start_addr, unsigned int *end_addr, unsigned char *buf_id)
{
	unsigned char filter_num = channel->pts_channel_id+1;

	GXAV_ASSERT(channel != NULL);

	if(start_addr != NULL)
		*start_addr = (unsigned int )(gx_dma_to_phys((unsigned int)channel->pts_buffer));
	if(end_addr != NULL)
		*end_addr = (unsigned int )(gx_dma_to_phys((unsigned int)channel->pts_buffer)) + channel->pts_size - 1;

	if(buf_id != NULL)
		*buf_id = filter_num;

	GXAV_DBG("%s(): channel->pts_buffer = %p,channel->pts_size = 0x%x\n",__func__,channel->pts_buffer,channel->pts_size);
	GXAV_DBG("%s(): start_addr = 0x%x,end_addr = 0x%x,buf_id=0x%x\n",__func__,*start_addr,*end_addr,*buf_id);

	return 0;
}

void gxav_sdc_free_all(struct gxav_module *module)
{
	int i;
	unsigned int id = 0;
	struct gxav_sdc *module_sdc = module_get_sdc(module);

	if (IS_MODULE_SDC_NULL()) {
		gx_printf("%s,module_sdc NULL\n",__func__);
		return;
	}
	for (i = 0; i < GXAV_MAX_CHANNEL; i++) {
		id = 1UL << i;
		GXAV_DBG("%s(), module_sdc->channel_status = 0x%x\n",__func__,module_sdc->channel_status);
		if (module_sdc->channel_status & id) {
			module_sdc->channel[i].inpin_id = -1;
			module_sdc->channel[i].outpin_id = -1;
			gxav_channel_free(gxav_module_getdev(module), &module_sdc->channel[i]);
		}
	}
}

