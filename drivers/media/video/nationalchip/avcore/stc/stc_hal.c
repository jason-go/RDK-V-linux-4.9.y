#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "stc_hal.h"

static struct stc_ops* _stc_ops;

int gxav_stc_init(struct gxav_module_ops* ops)
{
    GXAV_DBG("%s()\n",__func__);

	_stc_ops = ops->priv;

	if (_stc_ops && _stc_ops->init)
		return _stc_ops->init();

	return 0;
}

int gxav_stc_cleanup(void)
{
    GXAV_DBG("%s()\n",__func__);

	if (_stc_ops && _stc_ops->cleanup)
		return _stc_ops->cleanup();
	_stc_ops = NULL;

	return 0;
}

int gxav_stc_open(int id)
{
    if (_stc_ops && _stc_ops->open){
        return _stc_ops->open(id);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_close(int id)
{
    if (_stc_ops && _stc_ops->close){
        return _stc_ops->close(id);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_set_source(int id, GxSTCProperty_Config* config)
{
    if (config == NULL) {
        gx_printf("%s(),stc config error!\n", __func__);
        return -1;
    }

    if (_stc_ops && _stc_ops->set_source){
        return _stc_ops->set_source(id, config);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_get_source(int id, GxSTCProperty_Config* config)
{
    if (config == NULL) {
        gx_printf("%s(),stc config error!\n", __func__);
        return -1;
    }

    if (_stc_ops && _stc_ops->get_source){
        return _stc_ops->get_source(id, config);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_set_resolution(int id, GxSTCProperty_TimeResolution* resolution)
{
    if (resolution == NULL) {
        gx_printf("%s(),stc time resolution error!\n", __func__);
        return -1;
    }

    if (_stc_ops && _stc_ops->set_resolution) {
        return _stc_ops->set_resolution(id, resolution);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_get_resolution(int id, GxSTCProperty_TimeResolution* resolution)
{
    if (resolution == NULL) {
        gx_printf("%s(),stc time resolution error!\n", __func__);
        return -1;
    }

    if (_stc_ops && _stc_ops->get_resolution) {
        return _stc_ops->get_resolution(id, resolution);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}


int gxav_stc_get_base_resolution(int id, GxSTCProperty_TimeResolution* resolution)
{
    if (resolution == NULL) {
        gx_printf("%s(),stc time resolution error!\n", __func__);
        return -1;
    }

    if (_stc_ops && _stc_ops->get_resolution) {
        _stc_ops->get_resolution(id, resolution);
		resolution->freq_HZ = (resolution->freq_HZ==45000 ? 45000 : 2000);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
    return 0;
}

int gxav_stc_set_time(int id, GxSTCProperty_Time* time)
{
    if (time == NULL) {
        gx_printf("%s(),stc time error!\n", __func__);
        return -1;
    }

    if (_stc_ops && _stc_ops->set_time) {
        return _stc_ops->set_time(id, time);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_get_time(int id, GxSTCProperty_Time* time)
{
    if (time == NULL) {
        gx_printf("%s(),stc time error!\n", __func__);
        return -1;
    }

    if (_stc_ops && _stc_ops->get_time) {
        return _stc_ops->get_time(id, time);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_play(int id)
{
    if (_stc_ops && _stc_ops->play) {
        return _stc_ops->play(id);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_stop(int id)
{
    if (_stc_ops && _stc_ops->stop) {
        return _stc_ops->stop(id);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_pause(int id)
{
    if (_stc_ops && _stc_ops->pause) {
        return _stc_ops->pause(id);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_resume(int id)
{
    if (_stc_ops && _stc_ops->resume) {
        return _stc_ops->resume(id);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_read_stc(int id, unsigned int *value)
{
	GxSTCProperty_Time time;

    if (_stc_ops && _stc_ops->get_time) {
        _stc_ops->get_time(id, &time);
		*value = (unsigned int)time.time;
		return 0;
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_read_apts(int id, unsigned int *value)
{
    if (_stc_ops && _stc_ops->read_apts) {
        return _stc_ops->read_apts(id, value);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_read_pcr(int id, unsigned int *value)
{
    if (_stc_ops && _stc_ops->read_pcr) {
        return _stc_ops->read_pcr(id, value);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_write_apts(int id, unsigned int value, int immd)
{
    if (_stc_ops && _stc_ops->write_apts) {
        return _stc_ops->write_apts(id, value, immd);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_write_vpts(int id, unsigned int value, int immd)
{
    if (_stc_ops && _stc_ops->write_vpts) {
        return _stc_ops->write_vpts(id, value, immd);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

int gxav_stc_invaild_apts(unsigned int value)
{
    if (_stc_ops && _stc_ops->invaild_apts) {
        return _stc_ops->invaild_apts(value);
    } else {
        gx_printf("%s(),stc ops NULL\n",__func__);
        return -1;
    }
}

