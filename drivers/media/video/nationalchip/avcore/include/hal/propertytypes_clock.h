#ifndef __AV_HAL_PROPERTYTYPES_CLOCK_H__
#define __AV_HAL_PROPERTYTYPES_CLOCK_H__

#include "clock_hal.h"


struct avhal_clock_property_cold_rst {
	int res;
	unsigned int module;
	int ret;
};

struct avhal_clock_property_hot_rst_set {
	int res;
	unsigned int module;
	int ret;
};

struct avhal_clock_property_hot_rst_clr {
	int res;
	unsigned int module;
	int ret;
};

struct avhal_clock_property_setclock {
	int res;
	unsigned int module;
	clock_params arg;
	int ret;
};

struct avhal_clock_property_getconfig {
	int res;
	unsigned int module;
	clock_configs arg;
	int ret;
};

struct avhal_clock_property_source_enable {
	int res;
	unsigned int module;
	int ret;
};

struct avhal_clock_property_audioout_dacinside {
	int res;
	unsigned int mclock;
	int ret;
};

struct avhal_clock_property_audioout_dacinside_mute {
	int res;
	unsigned int enable;
	int ret;
};

struct avhal_clock_property_multiplex_pinsel {
	int res;
	unsigned int type;
	int ret;
};

struct avhal_clock_property_set_video_dac_clock_source {
	int res;
	unsigned int dacid;
	enum video_dac_clk_source src;
	int ret;
};

struct avhal_clock_property_module_enable {
	int res;
	int module;
	int flag;
	int ret;
};

struct avhal_clock_property_audioout_dacinside_slow_enable {
	int res;
	unsigned int power;
	unsigned int enable;
	int ret;
};

struct avhal_clock_property_audioout_dacinside_slow_config {
	int res;
	unsigned int step_num;
	unsigned int skip_num;
	int ret;
};

#endif

