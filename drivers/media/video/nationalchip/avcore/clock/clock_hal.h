#ifndef __GX_CLOCK_HAL_H__
#define __GX_CLOCK_HAL_H__

#include "gxav.h"
#include "kernelcalls.h"
#include "vout_hal.h"

#ifdef GX_DEBUG
#define CLOCK_PRINTF(fmt, args...) do{ \
    gx_printf("\n[COMMON][%s():%d]: ", __func__, __LINE__);\
    gx_printf(fmt, ##args); \
} while(0)
#define CLOCK_DBG(fmt, args...)   do{\
    gx_printf("\n[COMMON][%s():%d]: ", __func__, __LINE__);\
    gx_printf(fmt, ##args);\
}while(0)
#else
#define CLOCK_PRINTF(fmt, args...)   ((void)0)
#define CLOCK_DBG(fmt, args...)   ((void)0)
#endif

enum module_type {
    MODULE_TYPE_SDC         = 0,
    MODULE_TYPE_AUDIO_AD_SDC   ,
    MODULE_TYPE_AUDIO_AD       ,
    MODULE_TYPE_AUDIO_OR       ,
    MODULE_TYPE_AUDIO_I2S_SDC  ,
    MODULE_TYPE_AUDIO_SPDIF_SDC,
    MODULE_TYPE_AUDIO_I2S      ,
    MODULE_TYPE_AUDIO_LODEC    ,
    MODULE_TYPE_AUDIO_SPDIF    ,
    MODULE_TYPE_VIDEO_DEC      ,
    MODULE_TYPE_VIDEO_INPUT    ,
    MODULE_TYPE_DE_INTERLACE   ,
    MODULE_TYPE_GA             ,
    MODULE_TYPE_OSD            ,
    MODULE_TYPE_PP             ,
    MODULE_TYPE_PIC            ,
    MODULE_TYPE_DEMUX_SDC      ,
    MODULE_TYPE_DEMUX_SYS      ,
    MODULE_TYPE_AUDIO_CODEC    ,
    MODULE_TYPE_DEMUX_STC      ,
    MODULE_TYPE_DEMUX_OUT      ,
    MODULE_TYPE_HDMI           ,
    MODULE_TYPE_VOUT           ,
    MODULE_TYPE_VPU            ,
    MODULE_TYPE_SVPU           ,
    MODULE_TYPE_JPEG           ,
    MODULE_TYPE_FRONTEND       ,
    MODULE_TYPE_MAX
};

enum pin_type {
    PIN_TYPE_AUDIOOUT_I2S,
    PIN_TYPE_DEMUX_TS3,
    PIN_TYPE_VIDEOOUT_YUV,
    PIN_TYPE_VIDEOOUT_SCART,
    PIN_TYPE_VIDEOOUT_CVBS,
};

enum pll_freq {
    PLL_CLOCK_018M  = 0,
    PLL_CLOCK_027M,
    PLL_CLOCK_054M,
    PLL_CLOCK_108M,
    PLL_CLOCK_120M,
    PLL_CLOCK_148x5M,
    PLL_CLOCK_166M,
    PLL_CLOCK_189M,
    PLL_CLOCK_216M,
    PLL_CLOCK_229x5M,
    PLL_CLOCK_243M,
    PLL_CLOCK_270M,
    PLL_CLOCK_297M,
    PLL_CLOCK_324M,
    PLL_CLOCK_333M,
    PLL_CLOCK_342M,
    PLL_CLOCK_351M,
    PLL_CLOCK_360M
};

enum audio_clk_source {
	I2S_PLL,
	I2S_DTO,
    SPDIF_FROM_I2S,
    SPDIF_DTO
};

enum video_dac_clk_source {
	VIDEO_DAC_SRC_VPU,
	VIDEO_DAC_SRC_SVPU,
};

enum frontend_config_mode {
	FRONTEND_CLOCK_CONFIG,
	FRONTEND_ADC_INIT,
};

typedef union {
	struct {
		unsigned int clock;
		unsigned int div;
	}vpu;
	struct {
		unsigned int src;
		unsigned int mode;
	}vout;
	struct {
		unsigned int value;
	}audio_or;
	struct {
		unsigned int clock_source;
		unsigned int value;
	}audio_spdif;
	struct {
		unsigned int clock_source;
		unsigned int value;
	}audio_lodec;
	struct {
		unsigned int clock_source;
		unsigned int value;
		unsigned int sample_index;
		unsigned int clock_speed;
	}audio_i2s;
	struct {
		unsigned int mode;
		unsigned int demod_clk_div;
		unsigned int fec_clk_div;
		unsigned int adc_clk_div;
	}frontend;

}clock_params;

typedef union {
	struct {
		unsigned int dvb_pll_fbdiv;
	}frontend;

}clock_configs;

struct gxav_clock_module {
	int (*init)(void);
	int (*uninit)(void);
	int (*cold_rst)(unsigned int module);
	int (*hot_rst_set)(unsigned int module);
	int (*hot_rst_clr)(unsigned int module);
	int (*setclock)(unsigned int module, clock_params* arg);
	int (*getconfig)(unsigned int module, clock_configs* arg);
	int (*vdac_clock_source)(unsigned int dacid, enum video_dac_clk_source src);
	int (*source_enable)(unsigned int module);
	int (*source_disable)(unsigned int module);
	int (*audioout_dacinside)             (unsigned int mclock);
	int (*audioout_dacinside_mute)        (unsigned int enable);
	int (*audioout_dacinside_clock_enable)(unsigned int enable);
	int (*audioout_dacinside_clock_select)(unsigned int div);
	int (*audioout_dacinside_slow_enable) (unsigned int power,    unsigned int enable);
	int (*audioout_dacinside_slow_config) (unsigned int step_num, unsigned int skip_num);
	int (*module_clock_enable)(int module, int flag);
	int (*multiplex_pinsel)(unsigned int type);
};

extern int gxav_clock_init(struct gxav_clock_module* module);
extern int gxav_clock_uninit(void);
extern int gxav_clock_cold_rst(unsigned int module);
extern int gxav_clock_hot_rst_set(unsigned int module);
extern int gxav_clock_hot_rst_clr(unsigned int module);
extern int gxav_clock_setclock(unsigned int module, clock_params* arg);
extern int gxav_clock_getconfig(unsigned int module, clock_configs* arg);
extern int gxav_clock_source_enable(unsigned int module);
extern int gxav_clock_source_disable(unsigned int module);
extern int gxav_clock_audioout_dacinside(unsigned int mclock);
extern int gxav_clock_audioout_dacinside_mute        (unsigned int enable);
extern int gxav_clock_audioout_dacinside_clock_enable(unsigned int enable);
extern int gxav_clock_audioout_dacinside_clock_select(unsigned int enable);
extern int gxav_clock_multiplex_pinsel(unsigned int type);
extern int gxav_clock_set_video_dac_clock_source(unsigned int dacid, enum video_dac_clk_source src);
extern int gxav_clock_audioout_dacinside_slow_enable(unsigned int power,    unsigned int enable  );
extern int gxav_clock_audioout_dacinside_slow_config(unsigned int step_num, unsigned int skip_num);

extern int gxav_clock_module_enable(int module, int flag);

extern unsigned int gxav_pll_fre_table[][4];
extern unsigned int gxav_dto_fre_table[];
extern unsigned int gxav_audioout_pllclk[][5];

#endif
