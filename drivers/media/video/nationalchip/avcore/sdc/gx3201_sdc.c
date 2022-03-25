#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gxav_common.h"

#include "sdc_hal.h"
#include "sdc_module.h"
#include "gx3201_sdc_reg.h"

#define EVENT_FIFO_FULL(i)   (1UL << i)
#define EVENT_FIFO_EMPTY(i)  (1UL << (i + 16))

#define BUF_STATUS_EMPTY 1
#define BUF_STATUS_FULL  2
#define BUF_STATUS_ALMOST_EMPTY 4
#define BUF_STATUS_ALMOST_FULL 8

#define BUF_EVENT_EMPTY 1
#define BUF_EVENT_FULL  2
#define BUF_EVENT_ALMOST_EMPTY 4
#define BUF_EVENT_ALMOST_FULL 8

static struct sdc_regs gx3201_sdc_hal, *gx3201_sdc_reg;
static void* gx3201_sdc_mod;
unsigned int gx3201_sdc_peak[GXAV_SDC_MAX];

#define SDC_CHANNEL_LOCK_INIT(id)   gx_spin_lock_init(&gx3201_sdc_reg->spin_lock[id]);
#define SDC_CHANNEL_LOCK(id)        gx_spin_lock_irqsave(&gx3201_sdc_reg->spin_lock[id], flags);
#define SDC_CHANNEL_UNLOCK(id)      gx_spin_unlock_irqrestore(&gx3201_sdc_reg->spin_lock[id], flags);
#define SDC_CHANNEL_LOCK_UNINIT(id) (void)0

static int gx3201_sdc_length_get(int channel, unsigned int *len)
{
	CHECK_CHANNEL(channel);

	if(len) {
		unsigned long flags;
		SDC_CHANNEL_LOCK(channel);
		if(gx3201_sdc_reg->rBUFn_WRADDR[channel] >= gx3201_sdc_reg->rBUFn_RDADDR[channel])
			*len = gx3201_sdc_reg->rBUFn_WRADDR[channel] - gx3201_sdc_reg->rBUFn_RDADDR[channel];
		else
			*len = gx3201_sdc_reg->rBUFn_WRADDR[channel] - gx3201_sdc_reg->rBUFn_RDADDR[channel] +
			gx3201_sdc_reg->rBUFn_CAP[channel];
		SDC_CHANNEL_UNLOCK(channel);
	}

	return 0;
}

static int gx3201_sdc_free_get(int channel, unsigned int *len)
{
	CHECK_CHANNEL(channel);

	if(len){
		unsigned int len1;
		gx3201_sdc_length_get(channel, &len1);
		*len = gx3201_sdc_reg->rBUFn_CAP[channel] - len1 - 1;
	}

	return 0;
}

static int channel_reset(int channel)
{
	unsigned long flags;
	CHECK_CHANNEL(channel);

	SDC_CHANNEL_LOCK(channel);
	gx3201_sdc_peak[channel] = 0;
	gx3201_sdc_reg->rBUFn_PEAK_VALUE[channel] = 0;
	gx3201_sdc_reg->rBUFn_COUNTER[channel] = 0;
	gx3201_sdc_reg->rBUFn_RDADDR[channel] = 0;
	gx3201_sdc_reg->rBUFn_WRADDR[channel] = 0;
	gx3201_sdc_reg->rBUFn_STATUS[channel] = 0;
	SDC_CHANNEL_UNLOCK(channel);

	gx_wake_event(gx3201_sdc_mod, EVENT_FIFO_EMPTY(channel));

	return 0;
}

static int channel_flush(int channel)
{
	unsigned long flags;
	CHECK_CHANNEL(channel);

	SDC_CHANNEL_LOCK(channel);
	gx3201_sdc_peak[channel] = 0;
	gx3201_sdc_reg->rBUFn_PEAK_VALUE[channel] = 0;
	gx3201_sdc_reg->rBUFn_COUNTER[channel] = 0;
	gx3201_sdc_reg->rBUFn_WRADDR[channel] = gx3201_sdc_reg->rBUFn_RDADDR[channel];
	gx3201_sdc_reg->rBUFn_STATUS[channel] = 0;
	SDC_CHANNEL_UNLOCK(channel);

	gx_wake_event(gx3201_sdc_mod, EVENT_FIFO_EMPTY(channel));

	return 0;
}

static int gx3201_sdc_apply(int channel,unsigned int size)
{
	CHECK_CHANNEL(channel);

	gx3201_sdc_reg->rBUFn_CAP[channel] = size;
	gx3201_sdc_reg->rBUFn_EMPTY_GATE[channel] = 0;
	gx3201_sdc_reg->rBUFn_FULL_GATE[channel] = size;
	gx3201_sdc_reg->rBUFn_ALMOST_EMPTY_GATE[channel] = size/4;
	gx3201_sdc_reg->rBUFn_ALMOST_FULL_GATE[channel] = size*3/4;

	SDC_CHANNEL_LOCK_INIT(channel);

	return channel_reset(channel);
}

static int gx3201_sdc_free(int channel)
{
	return 0;
}

static int gx3201_sdc_gate_set(int channel, unsigned int empty,unsigned int full)
{
	CHECK_CHANNEL(channel);

	gx3201_sdc_reg->rBUFn_EMPTY_GATE[channel] = empty;
	gx3201_sdc_reg->rBUFn_FULL_GATE[channel] = full;

	return 0;
}

static int gx3201_sdc_gate_get(int channel, unsigned int *empty,unsigned int *full)
{
	CHECK_CHANNEL(channel);

	if(empty)
		*empty = gx3201_sdc_reg->rBUFn_EMPTY_GATE[channel] ;

	if(full)
		*full = gx3201_sdc_reg->rBUFn_FULL_GATE[channel];

	return 0;
}

static int gx3201_sdc_algate_set(int channel, unsigned int almost_empty,unsigned int almost_full)
{
	CHECK_CHANNEL(channel);

	gx3201_sdc_reg->rBUFn_ALMOST_EMPTY_GATE[channel] = almost_empty;
	gx3201_sdc_reg->rBUFn_ALMOST_FULL_GATE[channel] = almost_full;

	return 0;
}

static int gx3201_sdc_algate_get(int channel, unsigned int *almost_empty,unsigned int *almost_full)
{
	CHECK_CHANNEL(channel);

	if(almost_empty)
		*almost_empty = gx3201_sdc_reg->rBUFn_ALMOST_EMPTY_GATE[channel];

	if(almost_full)
		*almost_full  = gx3201_sdc_reg->rBUFn_ALMOST_FULL_GATE[channel];

	return 0;
}

static void gx3201_sdc_interrupt(int channel)
{
	unsigned long flags;
	unsigned int counter = 0;
	unsigned int almost_empty = 0, almost_full = 0;

	gx3201_sdc_algate_get(channel, &almost_empty, &almost_full);
	gx3201_sdc_length_get(channel, &counter);

	SDC_CHANNEL_LOCK(channel);
	if(counter >= almost_full)//full
	{
		if((gx3201_sdc_reg->rBUFn_STATUS[channel] & BUF_STATUS_ALMOST_FULL) == 0)
		{
			gx_wake_event(gx3201_sdc_mod, EVENT_FIFO_FULL(channel));
			gx3201_sdc_reg->rBUFn_STATUS[channel] |= (BUF_STATUS_ALMOST_FULL);
		}
	}
	else
	{
		gx3201_sdc_reg->rBUFn_STATUS[channel] &= (~BUF_STATUS_ALMOST_FULL);
	}

	if(counter <= almost_empty)//empty
	{
		if((gx3201_sdc_reg->rBUFn_STATUS[channel] & BUF_STATUS_ALMOST_EMPTY) == 0)
		{
			gx_wake_event(gx3201_sdc_mod, EVENT_FIFO_EMPTY(channel));
			gx3201_sdc_reg->rBUFn_STATUS[channel] |= (BUF_STATUS_ALMOST_EMPTY);
		}
	}
	else
	{
		gx3201_sdc_reg->rBUFn_STATUS[channel] &= (~BUF_STATUS_ALMOST_EMPTY);
	}
	SDC_CHANNEL_UNLOCK(channel);
}

static int gx3201_sdc_length_set(int channel, unsigned int len, GxAvSdcOPMode mode)
{
	unsigned int free_size, used_size, act_size=0;
	unsigned int dataflow=0;

	CHECK_CHANNEL(channel);

	if(len > 0)
	{
		unsigned long flags;
		gx3201_sdc_free_get(channel, &free_size);
		gx3201_sdc_length_get(channel, &used_size);

		SDC_CHANNEL_LOCK(channel);
		if( mode == GXAV_SDC_WRITE)
		{
			act_size = GX_MIN(free_size, len);

			dataflow = (act_size < len) ? 1 : 0;

			gx3201_sdc_reg->rBUFn_WRADDR[channel]  += act_size;
			gx3201_sdc_reg->rBUFn_COUNTER[channel] += act_size;
		}
		else
		{
			act_size = GX_MIN(used_size, len);

			dataflow = (act_size == used_size) ? 1 : 0;

			gx3201_sdc_reg->rBUFn_COUNTER[channel] -= act_size;
			gx3201_sdc_reg->rBUFn_RDADDR[channel]  += act_size;
		}

		gx3201_sdc_reg->rBUFn_RDADDR[channel] %= gx3201_sdc_reg->rBUFn_CAP[channel];
		gx3201_sdc_reg->rBUFn_WRADDR[channel] %= gx3201_sdc_reg->rBUFn_CAP[channel];

		if(gx3201_sdc_reg->rBUFn_COUNTER[channel] > gx3201_sdc_reg->rBUFn_PEAK_VALUE[channel]) {
			gx3201_sdc_reg->rBUFn_PEAK_VALUE[channel] = gx3201_sdc_reg->rBUFn_COUNTER[channel];
			gx3201_sdc_peak[channel] = gx3201_sdc_reg->rBUFn_COUNTER[channel];
		}
		SDC_CHANNEL_UNLOCK(channel);

		gx3201_sdc_interrupt(channel);
		gx_sdc_callback(channel, act_size, dataflow, mode);
	}

	return 0;
}

static int gx3201_sdc_rwaddr_get(int channel,unsigned int *rd_addr,unsigned int *wr_addr)
{
	unsigned long flags;
	CHECK_CHANNEL(channel);

	SDC_CHANNEL_LOCK(channel);

	if(rd_addr)
		*rd_addr = gx3201_sdc_reg->rBUFn_RDADDR[channel] % gx3201_sdc_reg->rBUFn_CAP[channel];

	if(wr_addr)
		*wr_addr = gx3201_sdc_reg->rBUFn_WRADDR[channel] % gx3201_sdc_reg->rBUFn_CAP[channel];

	SDC_CHANNEL_UNLOCK(channel);

	return 0;
}

static int gx3201_sdc_Rptr_Set(int channel,unsigned int rd_ptr)
{
	unsigned long flags;
	unsigned int last_ptr, length;

	CHECK_CHANNEL(channel);

	SDC_CHANNEL_LOCK(channel);

	last_ptr = gx3201_sdc_reg->rBUFn_RDADDR[channel] % gx3201_sdc_reg->rBUFn_CAP[channel];
	length = (rd_ptr+gx3201_sdc_reg->rBUFn_CAP[channel]-last_ptr)%gx3201_sdc_reg->rBUFn_CAP[channel];

	SDC_CHANNEL_UNLOCK(channel);

	gx3201_sdc_length_set(channel, length, GXAV_SDC_READ);

	return 0;
}

static int gx3201_sdc_Wptr_Set(int channel,unsigned int wr_ptr)
{
	unsigned long flags;
	unsigned int last_ptr, length;

	CHECK_CHANNEL(channel);

	SDC_CHANNEL_LOCK(channel);

	last_ptr = gx3201_sdc_reg->rBUFn_WRADDR[channel] % gx3201_sdc_reg->rBUFn_CAP[channel];
	length = (wr_ptr+gx3201_sdc_reg->rBUFn_CAP[channel]-last_ptr)%gx3201_sdc_reg->rBUFn_CAP[channel];

	SDC_CHANNEL_UNLOCK(channel);

	gx3201_sdc_length_set(channel, length, GXAV_SDC_WRITE);

	return 0;
}

static int gx3201_sdc_buffer_flush(int channel)
{
	return channel_flush(channel);
}

static int gx3201_sdc_buffer_reset(int channel)
{
	return channel_reset(channel);
}

static int gx3201_sdc_init(void)
{
	gx3201_sdc_reg = &gx3201_sdc_hal;

	gx_memset(gx3201_sdc_reg, 0 ,sizeof(gx3201_sdc_hal));

	return 0;
}

static int gx3201_sdc_uninit(void)
{
	gx3201_sdc_reg = NULL;

	return 0;
}

static int gx3201_sdc_open(void* priv)
{
	gx3201_sdc_mod = priv;

	return 0;
}

static int gx3201_sdc_close(void)
{
	return 0;
}

struct sdc_ops gx3201_sdc_ops = {
	.init           = gx3201_sdc_init,
	.uninit         = gx3201_sdc_uninit,
	.open           = gx3201_sdc_open,
	.close          = gx3201_sdc_close,
	.apply          = gx3201_sdc_apply,
	.free           = gx3201_sdc_free,
	.gate_set       = gx3201_sdc_gate_set,
	.gate_get       = gx3201_sdc_gate_get,
	.algate_set     = gx3201_sdc_algate_set,
	.algate_get     = gx3201_sdc_algate_get,
	.length_set     = gx3201_sdc_length_set,
	.length_get     = gx3201_sdc_length_get,
	.free_get       = gx3201_sdc_free_get,
	.buffer_flush   = gx3201_sdc_buffer_flush,
	.buffer_reset   = gx3201_sdc_buffer_reset,
	.rwaddr_get     = gx3201_sdc_rwaddr_get,
	.Rptr_Set       = gx3201_sdc_Rptr_Set,
	.Wptr_Set       = gx3201_sdc_Wptr_Set,
};

struct gxav_module_ops gx3201_sdc_module = {
	.module_type = GXAV_MOD_SDC,
	.count       = 1,
	.irqs        = {-1},
	.irq_flags   = {-1},
	.event_mask  = 0xffffffff,
	.init        = gx_sdc_init,
	.cleanup     = gx_sdc_uninit,
	.open        = gx_sdc_open,
	.close       = gx_sdc_close,
	.priv        = &gx3201_sdc_ops,
};

