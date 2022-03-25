#include "clock_hal.h"

#if defined CONFIG_AV_TEE_MODULE_CLOCK && defined CONFIG_OPTEE
#include "hal/property.h"
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>

int gxav_clock_cold_rst(unsigned int module)
{
	struct avhal_clock_property_cold_rst data = { .module = module };

	gxav_tee_ioctl(AVHAL_CLOCK_COLD_RST, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_hot_rst_set(unsigned int module)
{
	struct avhal_clock_property_hot_rst_set data = { .module = module };

	gxav_tee_ioctl(AVHAL_CLOCK_HOT_RST_SET, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_hot_rst_clr(unsigned int module)
{
	struct avhal_clock_property_hot_rst_clr data = { .module = module };

	gxav_tee_ioctl(AVHAL_CLOCK_HOT_RST_CLR, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_setclock(unsigned int module, clock_params* arg)
{
	struct avhal_clock_property_setclock data = { .module = module };

	memcpy(&data.arg, arg, sizeof(data.arg));
	gxav_tee_ioctl(AVHAL_CLOCK_SETCLOCK, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_getconfig(unsigned int module, clock_configs* arg)
{
	struct avhal_clock_property_getconfig data = { .module = module };

	memcpy(&data.arg, arg, sizeof(data.arg));
	gxav_tee_ioctl(AVHAL_CLOCK_GETCONFIG, &data, sizeof(data));
	memcpy(arg, &data.arg, sizeof(data.arg));

	return data.ret;
}

int gxav_clock_source_enable(unsigned int module)
{
	struct avhal_clock_property_source_enable data = { .module = module };

	gxav_tee_ioctl(AVHAL_CLOCK_SOURCE_ENABLE, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_audioout_dacinside(unsigned int mclock)
{
	struct avhal_clock_property_audioout_dacinside data = { .mclock = mclock};

	gxav_tee_ioctl(AVHAL_CLOCK_AUDIOOUT_DACINSIDE, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_audioout_dacinside_mute(unsigned int enable)
{
	struct avhal_clock_property_audioout_dacinside_mute data = { .enable = enable};

	gxav_tee_ioctl(AVHAL_CLOCK_AUDIOOUT_DACINSIDE_MUTE, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_audioout_dacinside_slow_enable(unsigned int power, unsigned int enable)
{
	struct avhal_clock_property_audioout_dacinside_slow_enable data = { .power = power, .enable = enable};

	gxav_tee_ioctl(AVHAL_CLOCK_AUDIOOUT_DACINSIDE_SLOW_ENABLE, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_audioout_dacinside_slow_config(unsigned int step_num, unsigned int skip_num)
{
	struct avhal_clock_property_audioout_dacinside_slow_config data = { .step_num = step_num, .skip_num = skip_num };

	gxav_tee_ioctl(AVHAL_CLOCK_AUDIOOUT_DACINSIDE_SLOW_CONFIG, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_multiplex_pinsel(unsigned int type)
{
	struct avhal_clock_property_multiplex_pinsel data = { .type = type};

	gxav_tee_ioctl(AVHAL_CLOCK_MULTIPLEX_PINSEL, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_set_video_dac_clock_source(unsigned int dacid, enum video_dac_clk_source src)
{
	struct avhal_clock_property_set_video_dac_clock_source data = { .dacid = dacid, .src = src };

	gxav_tee_ioctl(AVHAL_CLOCK_SET_VIDEO_DAC_CLOCK_SOURCE, &data, sizeof(data));

	return data.ret;
}

int gxav_clock_module_enable(int module, int flag)
{
#ifdef CONFIG_AV_ENABLE_DYNAMIC_CLOCK
	struct avhal_clock_property_module_enable data = { .module = module, .flag = flag};

	gxav_tee_ioctl(AVHAL_CLOCK_SET_VIDEO_DAC_CLOCK_SOURCE, &data, sizeof(data));

	return data.ret;
#else
	return 0;
#endif
}

#else

static struct gxav_clock_module* _clock_ops = NULL;

int gxav_clock_init(struct gxav_clock_module* module)
{
	_clock_ops = module;

	if (_clock_ops && _clock_ops->init) {
		return _clock_ops->init();
	}

	return 0;
}

int gxav_clock_uninit(void)
{
	if (_clock_ops && _clock_ops->uninit) {
		return _clock_ops->uninit();
	}

	_clock_ops = NULL;
	return 0;
}

int gxav_clock_cold_rst(unsigned int module)
{
	if (_clock_ops && _clock_ops->cold_rst) {
		return _clock_ops->cold_rst(module);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_hot_rst_set(unsigned int module)
{
	if (_clock_ops && _clock_ops->hot_rst_set) {
		return _clock_ops->hot_rst_set(module);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_hot_rst_clr(unsigned int module)
{
	if (_clock_ops && _clock_ops->hot_rst_clr) {
		return _clock_ops->hot_rst_clr(module);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_setclock(unsigned int module, clock_params* arg)
{
	if (_clock_ops && _clock_ops->setclock) {
		return _clock_ops->setclock(module, arg);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_getconfig(unsigned int module, clock_configs* arg)
{
	if (_clock_ops && _clock_ops->getconfig) {
		return _clock_ops->getconfig(module, arg);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_source_enable(unsigned int module)
{
	if (_clock_ops && _clock_ops->source_enable) {
		return _clock_ops->source_enable(module);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_source_disable(unsigned int module)
{
	if (_clock_ops && _clock_ops->source_disable) {
		return _clock_ops->source_disable(module);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_audioout_dacinside(unsigned int mclock)
{
	if (_clock_ops && _clock_ops->audioout_dacinside) {
		return _clock_ops->audioout_dacinside(mclock);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_audioout_dacinside_mute(unsigned int enable)
{
	if (_clock_ops && _clock_ops->audioout_dacinside_mute) {
		return _clock_ops->audioout_dacinside_mute(enable);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_audioout_dacinside_clock_enable(unsigned int enable)
{
	if (_clock_ops && _clock_ops->audioout_dacinside_clock_enable) {
		return _clock_ops->audioout_dacinside_clock_enable(enable);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_audioout_dacinside_clock_select(unsigned int div)
{
	if (_clock_ops && _clock_ops->audioout_dacinside_clock_select) {
		return _clock_ops->audioout_dacinside_clock_select(div);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_audioout_dacinside_slow_enable(unsigned int power, unsigned int enable)
{
	if (_clock_ops && _clock_ops->audioout_dacinside_slow_enable) {
		return _clock_ops->audioout_dacinside_slow_enable(power, enable);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_audioout_dacinside_slow_config(unsigned int step_num, unsigned int skip_num)
{
	if (_clock_ops && _clock_ops->audioout_dacinside_slow_config) {
		return _clock_ops->audioout_dacinside_slow_config(step_num, skip_num);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_multiplex_pinsel(unsigned int type)
{
	if (_clock_ops && _clock_ops->multiplex_pinsel) {
		return _clock_ops->multiplex_pinsel(type);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_set_video_dac_clock_source(unsigned int dacid, enum video_dac_clk_source src)
{
	if (_clock_ops && _clock_ops->vdac_clock_source) {
		return _clock_ops->vdac_clock_source(dacid, src);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
}

int gxav_clock_module_enable(int module, int flag)
{
#ifdef CONFIG_AV_ENABLE_DYNAMIC_CLOCK
	if(_clock_ops && _clock_ops->module_clock_enable) {
		return _clock_ops->module_clock_enable(module, flag);
	} else {
		gx_printf("%s(),clock ops NULL\n",__func__);
		return -1;
	}
#else
	return 0;
#endif
}

#endif
EXPORT_SYMBOL(gxav_clock_setclock);
EXPORT_SYMBOL(gxav_clock_getconfig);

unsigned int gxav_pll_fre_table[][4] = {
	/*	{BWADJ,CLKR,CLKOD,CLKF } */
	{0x00,0x00,0x00,0x00},	///< 18
	{0x0F,0x00,0x0F,0x0F},	///< 27
	{0x1B,0x00,0x0D,0x1B},	///< 54
	{0x00,0x00,0x00,0x00},	///< 108
	{0x00,0x00,0x00,0x00},	///< 120
	{0x15,0x00,0x03,0x15},	///< 148.5
	{0x24,0x01,0x02,0x24},	///< 166
	{0x1B,0x00,0x03,0x1B},	///< 189
	{0x17,0x00,0x02,0x17},	///< 216
	{0x10,0x00,0x01,0x10},	///< 229.5
	{0x1A,0x00,0x02,0x1A},	///< 243
	{0x13,0x00,0x01,0x13},	///< 270
	{0x15,0x00,0x01,0x15},	///< 297
	{0x17,0x00,0x01,0x17},	///< 324
	{0x24,0x02,0x00,0x24},	///< 333
	{0x25,0x02,0x00,0x25},	///< 342
	{0x19,0x00,0x01,0x19},	///< 351
	{0x27,0x02,0x00,0x27}	///< 360
};

unsigned int gxav_dto_fre_table[] = {
	18*1000000,	    ///< 18
	27*1000000,	    ///< 27
	54*1000000,	    ///< 54
	108*1000000,	///< 108
	120*1000000,	///< 120
	1485*100000,	///< 148.5
	166*1000000,	///< 166
	189*1000000,	///< 189
	216*1000000,	///< 216
	2295*100000,	///< 229.5
	243*1000000,	///< 243
	270*1000000,	///< 270
	297*1000000,	///< 297
	324*1000000,	///< 324
	333*1000000,	///< 333
	342*1000000,	///< 342
	351*1000000,	///< 351
	360*1000000	    ///< 360
};

unsigned int gxav_audioout_pllclk[][5] = {
	/*	{BWADJ,CLKR,CLKOD,CLKF,P } */
	{2569, 64, 12, 2569, 2 },	///44.1khz
	{1835, 63,  8, 1835, 2 },	///48khz
	{2369, 61, 16, 2369, 2 },	///32khz
	{2569, 64, 12, 2569, 4 },	///22.05khz
	{1835, 56,  6, 1835, 6 },	///24khz
	{2369, 61, 16, 2369, 4 },	///16khz
	{1835, 63,  4, 1835, 2 },	///96khz
	{2569, 64,  6, 2569, 2 },	///88.2khz
	{2369, 61,  4, 2369, 2},	///128khz
	{2569, 64, 12, 2569, 2 },	///176.4khz*256
	{1835, 63,  8, 1835, 2 },	///192khz*256
	{2369, 61,  8, 2369, 2 },	///64khz
	{1835, 48, 14, 1835, 6 },	///12khz
	{2569, 64, 16, 2569, 6 },	///11.025khz
	{},	///9.6khz
	{2369, 61, 16, 2369, 8 },	///8khz
};

