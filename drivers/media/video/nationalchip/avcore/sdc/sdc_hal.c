#include "kernelcalls.h"
#include "gxav_sdc_propertytypes.h"
#include "gxav_bitops.h"
#include "gxav_event_type.h"
#include "sdc_hal.h"

static struct sdc_ops* _sdc_ops = NULL;

int gxav_sdc_init(struct gxav_module_ops* ops)
{
	_sdc_ops = ops->priv;

	if (_sdc_ops && _sdc_ops->init)
		return _sdc_ops->init();

	return 0;
}

int gxav_sdc_uninit(void)
{
	if (_sdc_ops && _sdc_ops->uninit)
		return _sdc_ops->uninit();
	_sdc_ops = NULL;

	return 0;
}

int gxav_sdc_open(void* priv)
{
    if (_sdc_ops && _sdc_ops->open) {
        return _sdc_ops->open(priv);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_sdc_close(void)
{
    if (_sdc_ops && _sdc_ops->close) {
        return _sdc_ops->close();
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_apply(int channel_id, unsigned int size)
{
    if (_sdc_ops && _sdc_ops->apply) {
        return _sdc_ops->apply(channel_id, size);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_free(int channel_id)
{
    if (_sdc_ops && _sdc_ops->free) {
        return _sdc_ops->free(channel_id);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_gate_set(int channel_id, unsigned int empty, unsigned int full)
{
    if (_sdc_ops && _sdc_ops->gate_set) {
        return _sdc_ops->gate_set(channel_id, empty, full);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_gate_get(int channel_id, unsigned int *empty, unsigned int *full)
{
    if (_sdc_ops && _sdc_ops->gate_get) {
        return _sdc_ops->gate_get(channel_id, empty, full);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_algate_set(int channel_id, unsigned int almost_empty, unsigned int almost_full)
{
    if (_sdc_ops && _sdc_ops->algate_set) {
        return _sdc_ops->algate_set(channel_id, almost_empty, almost_full);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_algate_get(int channel_id, unsigned int *almost_empty, unsigned int *almost_full)
{
    if (_sdc_ops && _sdc_ops->algate_get) {
        return _sdc_ops->algate_get(channel_id, almost_empty, almost_full);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_length_set(int channel_id, unsigned int len, GxAvSdcOPMode mode)
{
    if (_sdc_ops && _sdc_ops->length_set) {
        return _sdc_ops->length_set(channel_id, len, mode);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_length_get(int channel_id, unsigned int *len)
{
    if (_sdc_ops && _sdc_ops->length_get) {
        return _sdc_ops->length_get(channel_id, len);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_free_get(int channel_id, unsigned int *len)
{
    if (_sdc_ops && _sdc_ops->free_get) {
        return _sdc_ops->free_get(channel_id, len);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_buffer_reset(int channel_id)
{
    if (_sdc_ops && _sdc_ops->buffer_reset) {
        return _sdc_ops->buffer_reset(channel_id);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_buffer_flush(int channel_id)
{
    if (_sdc_ops && _sdc_ops->buffer_flush) {
        return _sdc_ops->buffer_flush(channel_id);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_sdc_rwaddr_get(int channel_id, unsigned int *rd_addr,unsigned int *wr_addr)
{
    if (_sdc_ops && _sdc_ops->rwaddr_get) {
        return _sdc_ops->rwaddr_get(channel_id, rd_addr, wr_addr);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_Rptr_Set(int channel_id, unsigned int rd_ptr)
{
    if (_sdc_ops && _sdc_ops->Rptr_Set) {
        return _sdc_ops->Rptr_Set(channel_id, rd_ptr);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}

int gxav_sdc_Wptr_Set(int channel_id, unsigned int wr_ptr)
{
    if (_sdc_ops && _sdc_ops->Wptr_Set) {
        return _sdc_ops->Wptr_Set(channel_id, wr_ptr);
    } else {
        gx_printf("%s(),sdc ops NULL\n",__func__);
        return -1;
    }

}


