#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "clock_hal.h"
#include "gx3211_clock_reg.h"

volatile struct clock_regs *gx3211_clock_reg = NULL;
static unsigned int gx3211_module_clk[1] = {0};

static inline unsigned int DTO_PLL_FREQ(void) {
        unsigned long long result = DTO_PLL_F + 1;
        do_div(result,(DTO_PLL_R  + 1)*(CRYSTAL_FREQ));
        do_div(result,(DTO_PLL_OD + 1));
        return result > 0 ? (unsigned int)result : (unsigned int)result;
}

static inline unsigned int FREQ_TO_DTO_STEP(unsigned int x) {
        unsigned long long step = (1ULL << 30) * x;
        do_div(step, DTO_PLL_FREQ());
        return step > 0 ? (unsigned int)step : (unsigned int)step;
}

int gx3211_clock_init(void)
{
	if(gx3211_clock_reg == NULL){
		gx3211_clock_reg = gx_ioremap(CLOCK_BASE_ADDR, sizeof(struct clock_regs));
		if (gx3211_clock_reg == NULL) {
			gx_printf("%s,Ioremap failed.\n",__func__);
			return -1;
		}

		CLOCK_DBG("[gx3211_clock_reg=%x]\n",(int)gx3211_clock_reg);
		memset(gx3211_module_clk, 0, sizeof(gx3211_module_clk));
	}

	return 0;
}

int gx3211_clock_uninit(void)
{
	if(gx3211_clock_reg != NULL){
		gx_iounmap(gx3211_clock_reg);
		gx3211_clock_reg = NULL;
	}

	return 0;
}

#ifdef CONFIG_AV_MODULE_VIDEO_OUT
static void _gx3211_clock_videoout_ClockSourceSel(unsigned int select, unsigned int set)
{
	if(set)
		REG_SET_BIT(&(gx3211_clock_reg->cfg_source_sel), select);
	else
		REG_CLR_BIT(&(gx3211_clock_reg->cfg_source_sel), select);
}

static void _gx3211_clock_videoout_ClockSourceSel2(unsigned int select, unsigned int set)
{
	if(set)
		REG_SET_BIT(&(gx3211_clock_reg->cfg_source_sel2), select);
	else
		REG_CLR_BIT(&(gx3211_clock_reg->cfg_source_sel2), select);
}

static void _gx3211_clock_videoout_ClockDIVConfig(unsigned int clock, unsigned int div)
{
	if(div){
		REG_SET_BIT(&(gx3211_clock_reg->cfg_clk), clock);
	}else{
		REG_CLR_BIT(&(gx3211_clock_reg->cfg_clk), clock);
	}
}

extern int gx3211_vpu_SetBufferStateDelay(int v);
static void _gx3211_video_out_clockset(unsigned int src, unsigned int mode)
{
	if (src == ID_VPU) {
		switch(mode)
		{
			case GXAV_VOUT_PAL:
			case GXAV_VOUT_PAL_M:
			case GXAV_VOUT_PAL_N:
			case GXAV_VOUT_PAL_NC:
			case GXAV_VOUT_NTSC_M:
			case GXAV_VOUT_NTSC_443:
			case GXAV_VOUT_480I:
			case GXAV_VOUT_576I:
				gx3211_vpu_SetBufferStateDelay(0xc0);
				_gx3211_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 43;//ratio;
				_gx3211_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_gx3211_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
				_gx3211_clock_videoout_ClockDIVConfig(9,0);  //div dcs
				_gx3211_clock_videoout_ClockDIVConfig(10,1); //div pixel
				_gx3211_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_gx3211_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;

			case GXAV_VOUT_480P:
				gx3211_vpu_SetBufferStateDelay(0x60);
				_gx3211_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) &= ~(0x3f);

				if (CHIP_IS_GX6605S) {
					*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 43;//ratio;

					_gx3211_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
					_gx3211_clock_videoout_ClockDIVConfig(9,0);  //div dcs
					_gx3211_clock_videoout_ClockDIVConfig(10,0); //div pixel
				} else {
					*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 21;//ratio;

					_gx3211_clock_videoout_ClockDIVConfig(8,1);  //div hdmi
					_gx3211_clock_videoout_ClockDIVConfig(9,1);  //div dcs
					_gx3211_clock_videoout_ClockDIVConfig(10,1); //div pixel
				}

				_gx3211_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_gx3211_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_gx3211_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;
			case GXAV_VOUT_576P:
				gx3211_vpu_SetBufferStateDelay(0x60);
				_gx3211_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) &= ~(0x3f);

				if (CHIP_IS_GX6605S) {
					*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 43;//ratio;

					_gx3211_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
					_gx3211_clock_videoout_ClockDIVConfig(9,0);  //div dcs
					_gx3211_clock_videoout_ClockDIVConfig(10,0); //div pixel
				} else {
					*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 21;//ratio;

					_gx3211_clock_videoout_ClockDIVConfig(8,1);  //div hdmi
					_gx3211_clock_videoout_ClockDIVConfig(9,1);  //div dcs
					_gx3211_clock_videoout_ClockDIVConfig(10,1); //div pixel
				}

				_gx3211_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_gx3211_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_gx3211_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;

			case GXAV_VOUT_720P_50HZ:
			case GXAV_VOUT_1080I_50HZ:
				gx3211_vpu_SetBufferStateDelay(0x30);
				_gx3211_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) &= ~(0x3f);

				if (CHIP_IS_GX6605S) {
					*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 15;//ratio;

					_gx3211_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
					_gx3211_clock_videoout_ClockDIVConfig(9,0);  //div dcs
					_gx3211_clock_videoout_ClockDIVConfig(10,0); //div pixel
				} else {
					*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 7;//ratio;

					_gx3211_clock_videoout_ClockDIVConfig(8,1);  //div hdmi
					_gx3211_clock_videoout_ClockDIVConfig(9,1);  //div dcs
					_gx3211_clock_videoout_ClockDIVConfig(10,1); //div pixel
				}

				_gx3211_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_gx3211_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_gx3211_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;

			case GXAV_VOUT_720P_60HZ:
			case GXAV_VOUT_1080I_60HZ:
				gx3211_vpu_SetBufferStateDelay(0x30);
				_gx3211_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) &= ~(0x3f);

				if (CHIP_IS_GX6605S) {
					*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 15;//ratio;

					_gx3211_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
					_gx3211_clock_videoout_ClockDIVConfig(9,0);  //div dcs
					_gx3211_clock_videoout_ClockDIVConfig(10,0); //div pixel
				} else {
					*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 7;//ratio;

					_gx3211_clock_videoout_ClockDIVConfig(8,1);  //div hdmi
					_gx3211_clock_videoout_ClockDIVConfig(9,1);  //div dcs
					_gx3211_clock_videoout_ClockDIVConfig(10,1); //div pixel
				}

				_gx3211_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_gx3211_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_gx3211_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;

			case GXAV_VOUT_1080P_50HZ:
				gx3211_vpu_SetBufferStateDelay(0x18);
				_gx3211_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 7;//ratio;
				_gx3211_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_gx3211_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
				_gx3211_clock_videoout_ClockDIVConfig(9,0);  //div dcs
				_gx3211_clock_videoout_ClockDIVConfig(10,0); //div pixel
				_gx3211_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_gx3211_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;
			case GXAV_VOUT_1080P_60HZ:
				gx3211_vpu_SetBufferStateDelay(0x18);
				_gx3211_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 7;//ratio;
				_gx3211_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_gx3211_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_gx3211_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_gx3211_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
				_gx3211_clock_videoout_ClockDIVConfig(9,0);  //div dcs
				_gx3211_clock_videoout_ClockDIVConfig(10,0); //div pixel
				_gx3211_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_gx3211_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;
			default:
				gx_printf("un-supported.\n");
				return;
		}
	} else {
		_gx3211_clock_videoout_ClockSourceSel(6,0);   //1: pll 0: xtal

		_gx3211_clock_videoout_ClockDIVConfig(20,0);  //rst -> 0
		_gx3211_clock_videoout_ClockDIVConfig(19,0);  //load_en -> 0
		_gx3211_clock_videoout_ClockDIVConfig(20,1);  //rst -> 1
		_gx3211_clock_videoout_ClockDIVConfig(18,0);  //bypass -> 0
		*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_mepg_clk_inhibit)) |= 1<<30;
		*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) &= ~(0x3f000);
		*(volatile unsigned int *)(&(gx3211_clock_reg->cfg_clk)) |= 43<<12;
		_gx3211_clock_videoout_ClockDIVConfig(19,1);  //load_en -> 1
		_gx3211_clock_videoout_ClockDIVConfig(19,0);  //load_en -> 0

		_gx3211_clock_videoout_ClockSourceSel(6,1);   //1: pll 0: xtal

		_gx3211_clock_videoout_ClockSourceSel2(25,0);   //
		_gx3211_clock_videoout_ClockSourceSel2(26,1);   //pixel div
		_gx3211_clock_videoout_ClockSourceSel2(27,0);   //
		_gx3211_clock_videoout_ClockSourceSel2(25,1);   //
	}

}
#endif

static int gx3211_clock_device_cold_rst(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_VOUT:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<28);
			gx_mdelay(1);
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<28);
			break;
		case MODULE_TYPE_VIDEO_DEC:
			CFG_DEV_INHIBIT_SET(gx3211_clock_reg, 0x1<<3);
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<1);
			gx_mdelay(1);
			CFG_DEV_INHIBIT_CLEAR(gx3211_clock_reg, 0x1<<3);
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<1);
			break;
		case MODULE_TYPE_JPEG:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<2);
			gx_mdelay(1);
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<2);
			break;
		case MODULE_TYPE_PP:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<3);
			gx_mdelay(1);
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<3);
			break;
		case MODULE_TYPE_HDMI:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<7);
			gx_mdelay(1);
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<7);
			break;
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<20);
			gx_mdelay(1);
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<20);
			break;
		default:
			break;
	}

	gx_mdelay(1);
	return 0;
}

static int gx3211_clock_device_hot_rst_set(unsigned int module)
{
	switch (module)
	{
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_RESET_SET(gx3211_clock_reg);
			break;
		case MODULE_TYPE_VOUT:
		case MODULE_TYPE_VPU:
			CFG_VPU_HOT_SET(gx3211_clock_reg);
			break;
		case MODULE_TYPE_SVPU:
			break;
		case MODULE_TYPE_JPEG:
			CFG_JPEG_HOT_SET(gx3211_clock_reg);
			break;
		case MODULE_TYPE_HDMI:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<7);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3211_clock_device_hot_rst_clear(unsigned int module)
{
	switch (module)
	{
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_RESET_CLR(gx3211_clock_reg);
			break;
		case MODULE_TYPE_VOUT:
		case MODULE_TYPE_VPU:
			CFG_VPU_HOT_CLR(gx3211_clock_reg);
			break;
		case MODULE_TYPE_SVPU:
			break;
		case MODULE_TYPE_JPEG:
			CFG_JPEG_HOT_CLR(gx3211_clock_reg);
			break;
		case MODULE_TYPE_HDMI:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<7);
			break;
		default:
			break;
	}
	return 0;
}

#define CFG_VIDEO_DEC_SET_DTO_VIDEDO(base,val)                   \
	do {\
		REG_SET_BIT(&(base->cfg_source_sel),3);\
		REG_SET_VAL(&(base->cfg_dto3),1<<31);               \
		REG_SET_VAL(&(base->cfg_dto3),0);                   \
		REG_SET_VAL(&(base->cfg_dto3),1<<31);               \
		REG_SET_VAL(&(base->cfg_dto3),(1<<31)|val);           \
		REG_SET_VAL(&(base->cfg_dto3),(1<<31)|(1<<30)|val);                   \
		REG_SET_VAL(&(base->cfg_dto3),(1<<31)|(1<<30)|val);                   \
		REG_SET_VAL(&(base->cfg_dto3),(1<<31)|val);                   \
	} while(0)

static int gx3211_clock_device_setclock(unsigned int module, clock_params* arg)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_OR:
			CFG_AUDIO_OR_SET_DTO(gx3211_clock_reg, arg->audio_or.value);
			break;
		case MODULE_TYPE_AUDIO_I2S:
			if(arg->audio_i2s.clock_source == I2S_DTO) {
				CFG_AUDIO_AD_SET_DTO(gx3211_clock_reg, arg->audio_i2s.value);
				CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 1);
			}
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_SPDIF_SET_DTO(gx3211_clock_reg, arg->audio_spdif.value);
			if(arg->audio_spdif.clock_source == SPDIF_FROM_I2S)
				CFG_AUDIO_SPD_SEL_PLL(gx3211_clock_reg);
			else
				CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 2);
			break;
		case MODULE_TYPE_VPU:
			if(arg->vpu.div)
				REG_SET_BIT(&(gx3211_clock_reg->cfg_clk), arg->vpu.clock);
			else
				REG_CLR_BIT(&(gx3211_clock_reg->cfg_clk), arg->vpu.clock);
			break;
#ifdef CONFIG_AV_MODULE_VIDEO_OUT
		case MODULE_TYPE_VOUT:
			_gx3211_video_out_clockset(arg->vout.src, arg->vout.mode);
			break;
#endif
		default:
			break;
	}
	return 0;
}

static int gx3211_clock_clksource_enable(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_OR:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 15);
			CFG_DEV_INHIBIT_CLEAR(gx3211_clock_reg, 0x1<<11);
			break;
		case MODULE_TYPE_AUDIO_I2S:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 1);
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 0);
			CFG_DEV_INHIBIT_CLEAR(gx3211_clock_reg, 0x1<<0);
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 2);
			CFG_DEV_INHIBIT_CLEAR(gx3211_clock_reg, 0x1<<1);
			break;
		case MODULE_TYPE_DEMUX_SYS:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 17);
			CFG_DEV_INHIBIT_CLEAR(gx3211_clock_reg, 0x1<<13);
			break;
		case MODULE_TYPE_DEMUX_OUT:
			break;
		case MODULE_TYPE_DEMUX_STC:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 18);
			CFG_DEV_INHIBIT_CLEAR(gx3211_clock_reg, 0x1<<14);
			break;
		case MODULE_TYPE_DE_INTERLACE:
			break;
		case MODULE_TYPE_VOUT:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3211_clock_reg, 5);
			CFG_DEV_INHIBIT_CLEAR(gx3211_clock_reg, 0x1<<3);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3211_clock_clocksource_disable(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_I2S:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3211_clock_reg, 1);
			CFG_DEV_INHIBIT_SET(gx3211_clock_reg, 0x1<<0);
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3211_clock_reg, 2);
			CFG_DEV_INHIBIT_SET(gx3211_clock_reg, 0x1<<1);
			break;
		case MODULE_TYPE_DEMUX_SYS:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3211_clock_reg, 17);
			CFG_DEV_INHIBIT_SET(gx3211_clock_reg, 0x1<<13);
			break;
		case MODULE_TYPE_DEMUX_STC:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3211_clock_reg, 18);
			CFG_DEV_INHIBIT_SET(gx3211_clock_reg, 0x1<<14);
			break;
		case MODULE_TYPE_DEMUX_OUT:
			break;
		case MODULE_TYPE_VOUT:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3211_clock_reg, 5);
			CFG_DEV_INHIBIT_SET(gx3211_clock_reg, 0x1<<3);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3211_clock_audioout_dacinside(unsigned int mclock)
{
	int value = 0;

	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 0);

	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	switch (mclock) {
		case 256:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa8<<4)|(1<<2)|(1<<0)));
			break;
		case 384:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa0<<4)|(1<<2)|(1<<0)));
			break;
		case 512:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x98<<4)|(1<<2)|(1<<0)));
			break;
		case 768:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x90<<4)|(1<<2)|(1<<0)));
			break;
		case 1024:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x88<<4)|(1<<2)|(1<<0)));
			break;
		case 1536:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x80<<4)|(1<<2)|(1<<0)));
			break;
		case 128:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xb8<<4)|(1<<2)|(1<<0)));
			break;
		case 192:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xb0<<4)|(1<<2)|(1<<0)));
			break;
		default:
			REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa8<<4)|(1<<2)|(1<<0)));
			break;
	}

	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|(1<<3)|(1<<0)));

	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);

	value = REG_GET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl)) & 0xfffffffe;

	REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|((0x00|value)<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl), ((3<<12)|(0x7f<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|((0x01|value)<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl), ((3<<12)|(0x7f<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3211_clock_reg->cfg_audio_codec_ctrl), ((2<<12)|(0x38<<4)|(1<<2)|(1<<0)));

	REG_SET_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3211_clock_reg->cfg_audio_codec_ctrl), 1);

	return 0;
}

static int gx3211_clock_audioout_dacinside_clock_enable(unsigned int enable)
{
	if (enable)
		CFG_AUDIO_LODAC_CLOCK_ENABLE(gx3211_clock_reg);
	else
		CFG_AUDIO_LODAC_CLOCK_DISABLE(gx3211_clock_reg);

	return 0;
}

static int gx3211_clock_multiplex_pinsel(unsigned int type)
{
	switch(type)
	{
		case PIN_TYPE_AUDIOOUT_I2S:
			//CFG_PINC_I2S_AUDIO_SEL(gx3211_clock_reg);
			break;
		case PIN_TYPE_DEMUX_TS3:
			//CFG_PINC_TS3_GPIO_SEL(gx3211_clock_reg);
			break;
		case PIN_TYPE_VIDEOOUT_YUV:
			//CFG_PINA_VIDEOOUT_YUV_SEL(gx3211_clock_reg);
			break;
		case PIN_TYPE_VIDEOOUT_SCART:
			//CFG_PINA_VIDEOOUT_SCART_SEL(gx3211_clock_reg);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3211_clock_video_dac_source(unsigned int id, enum video_dac_clk_source src)
{
	int bit = 8 + id;

	if (src == VIDEO_DAC_SRC_VPU)
		REG_CLR_BIT(&(gx3211_clock_reg->cfg_source_sel), bit);
	else
		REG_SET_BIT(&(gx3211_clock_reg->cfg_source_sel), bit);

	return 0;
}

static int gx3211_clock_jpeg_module_enable(int flag)
{
	if(flag) {
		REG_CLR_BIT(&(gx3211_clock_reg->cfg_mepg_clk_inhibit), 9);
	} else {
		REG_SET_BIT(&(gx3211_clock_reg->cfg_mepg_clk_inhibit), 9);
	}
	return (0);
}

static int gx3211_clock_ga_module_enable(int flag)
{
	if(flag) {
		REG_CLR_BIT(&(gx3211_clock_reg->cfg_mepg_clk_inhibit), 12);
	} else {
		REG_SET_BIT(&(gx3211_clock_reg->cfg_mepg_clk_inhibit), 12);
	}
	return (0);
}

static int gx3211_clock_pp_module_enable(int flag)
{
	if(flag) {
		REG_CLR_BIT(&(gx3211_clock_reg->cfg_mepg_clk_inhibit), 10);
	} else {
		REG_SET_BIT(&(gx3211_clock_reg->cfg_mepg_clk_inhibit), 10);
	}
	return (0);
}

static int gx3211_module_clock_enable(int module, int flag)
{
	switch(module) {
		case MODULE_TYPE_JPEG:
			gx3211_clock_jpeg_module_enable(flag);
			break;
		case MODULE_TYPE_GA:
			gx3211_clock_ga_module_enable(flag);
			break;
		case MODULE_TYPE_PP:
			gx3211_clock_pp_module_enable(flag);
			break;
		default:
			break;
	}
	return (0);
}

struct gxav_clock_module gx3211_clock_module = {

	.init = gx3211_clock_init,
	.uninit = gx3211_clock_uninit,
	.cold_rst = gx3211_clock_device_cold_rst,
	.hot_rst_set = gx3211_clock_device_hot_rst_set,
	.hot_rst_clr = gx3211_clock_device_hot_rst_clear,
	.setclock = gx3211_clock_device_setclock,
	.vdac_clock_source = gx3211_clock_video_dac_source,
	.source_enable = gx3211_clock_clksource_enable,
	.source_disable = gx3211_clock_clocksource_disable,
	.audioout_dacinside = gx3211_clock_audioout_dacinside,
	.audioout_dacinside_clock_enable = gx3211_clock_audioout_dacinside_clock_enable,
	.module_clock_enable             = gx3211_module_clock_enable,
	.multiplex_pinsel = gx3211_clock_multiplex_pinsel,

};
