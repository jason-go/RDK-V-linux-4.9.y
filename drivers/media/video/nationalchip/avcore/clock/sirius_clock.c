#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "clock_hal.h"
#include "sirius_clock_reg.h"

volatile struct clock_regs *sirius_clock_reg = NULL;
static unsigned int sirius_module_clk[1] = {0};

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

int sirius_clock_init(void)
{
	if(sirius_clock_reg == NULL){
		sirius_clock_reg = gx_ioremap(CLOCK_BASE_ADDR, sizeof(struct clock_regs));
		if (sirius_clock_reg == NULL) {
			gx_printf("%s,Ioremap failed.\n",__func__);
			return -1;
		}

		CLOCK_DBG("[sirius_clock_reg=%x]\n",(int)sirius_clock_reg);
		memset(sirius_module_clk, 0, sizeof(sirius_module_clk));
	}

	return 0;
}

int sirius_clock_uninit(void)
{
	if(sirius_clock_reg != NULL){
		gx_iounmap(sirius_clock_reg);
		sirius_clock_reg = NULL;
	}

	return 0;
}

#if defined CONFIG_AV_MODULE_VIDEO_OUT || defined TEE_OS
static void _sirius_clock_videoout_ClockSourceSel(unsigned int select, unsigned int set)
{
	if(set)
		REG_SET_BIT(&(sirius_clock_reg->cfg_source_sel), select);
	else
		REG_CLR_BIT(&(sirius_clock_reg->cfg_source_sel), select);
}

static void _sirius_clock_videoout_ClockDIVConfig(unsigned int clock, unsigned int div)
{
	if(div){
		REG_SET_BIT(&(sirius_clock_reg->cfg_clk), clock);
	}else{
		REG_CLR_BIT(&(sirius_clock_reg->cfg_clk), clock);
	}
}
#ifdef TEE_OS
int sirius_vpu_SetBufferStateDelay(int v)
{
	//TODO:
	return 0;
}
#else
extern int sirius_vpu_SetBufferStateDelay(int v);
#endif
static void _sirius_video_out_clockset(unsigned int src, unsigned int mode)
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
				sirius_vpu_SetBufferStateDelay(0xc0);
				_sirius_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 43;//ratio;
				_sirius_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_sirius_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
				_sirius_clock_videoout_ClockDIVConfig(9,0);  //div dcs
				_sirius_clock_videoout_ClockDIVConfig(10,1); //div pixel
				_sirius_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_sirius_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;

			case GXAV_VOUT_480P:
				sirius_vpu_SetBufferStateDelay(0x60);
				_sirius_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) &= ~(0x3f);

				if (CHIP_IS_GX6605S) {
					*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 43;//ratio;

					_sirius_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
					_sirius_clock_videoout_ClockDIVConfig(9,0);  //div dcs
					_sirius_clock_videoout_ClockDIVConfig(10,0); //div pixel
				} else {
					*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 21;//ratio;

					_sirius_clock_videoout_ClockDIVConfig(8,1);  //div hdmi
					_sirius_clock_videoout_ClockDIVConfig(9,1);  //div dcs
					_sirius_clock_videoout_ClockDIVConfig(10,1); //div pixel
				}

				_sirius_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_sirius_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_sirius_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;
			case GXAV_VOUT_576P:
				sirius_vpu_SetBufferStateDelay(0x60);
				_sirius_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) &= ~(0x3f);

				if (CHIP_IS_GX6605S) {
					*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 43;//ratio;

					_sirius_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
					_sirius_clock_videoout_ClockDIVConfig(9,0);  //div dcs
					_sirius_clock_videoout_ClockDIVConfig(10,0); //div pixel
				} else {
					*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 21;//ratio;

					_sirius_clock_videoout_ClockDIVConfig(8,1);  //div hdmi
					_sirius_clock_videoout_ClockDIVConfig(9,1);  //div dcs
					_sirius_clock_videoout_ClockDIVConfig(10,1); //div pixel
				}

				_sirius_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_sirius_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_sirius_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;

			case GXAV_VOUT_720P_50HZ:
			case GXAV_VOUT_1080I_50HZ:
				sirius_vpu_SetBufferStateDelay(0x30);
				_sirius_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) &= ~(0x3f);

				if (CHIP_IS_GX6605S) {
					*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 15;//ratio;

					_sirius_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
					_sirius_clock_videoout_ClockDIVConfig(9,0);  //div dcs
					_sirius_clock_videoout_ClockDIVConfig(10,0); //div pixel
				} else {
					*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 7;//ratio;

					_sirius_clock_videoout_ClockDIVConfig(8,1);  //div hdmi
					_sirius_clock_videoout_ClockDIVConfig(9,1);  //div dcs
					_sirius_clock_videoout_ClockDIVConfig(10,1); //div pixel
				}

				_sirius_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_sirius_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_sirius_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;

			case GXAV_VOUT_720P_60HZ:
			case GXAV_VOUT_1080I_60HZ:
				sirius_vpu_SetBufferStateDelay(0x30);
				_sirius_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) &= ~(0x3f);

				if (CHIP_IS_GX6605S) {
					*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 15;//ratio;

					_sirius_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
					_sirius_clock_videoout_ClockDIVConfig(9,0);  //div dcs
					_sirius_clock_videoout_ClockDIVConfig(10,0); //div pixel
				} else {
					*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 7;//ratio;

					_sirius_clock_videoout_ClockDIVConfig(8,1);  //div hdmi
					_sirius_clock_videoout_ClockDIVConfig(9,1);  //div dcs
					_sirius_clock_videoout_ClockDIVConfig(10,1); //div pixel
				}

				_sirius_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_sirius_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_sirius_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;

			case GXAV_VOUT_1080P_50HZ:
				sirius_vpu_SetBufferStateDelay(0x18);
				_sirius_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 7;//ratio;
				_sirius_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_sirius_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
				_sirius_clock_videoout_ClockDIVConfig(9,0);  //div dcs
				_sirius_clock_videoout_ClockDIVConfig(10,0); //div pixel
				_sirius_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_sirius_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;
			case GXAV_VOUT_1080P_60HZ:
				sirius_vpu_SetBufferStateDelay(0x18);
				_sirius_clock_videoout_ClockSourceSel(5,0); //sel pll video 1.188GHz
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockDIVConfig(6,1);  //bypass -> 0
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) &= ~(0x3f);
				*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 7;//ratio;
				_sirius_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1
				_sirius_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
				_sirius_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M
				_sirius_clock_videoout_ClockDIVConfig(8,0);  //div hdmi
				_sirius_clock_videoout_ClockDIVConfig(9,0);  //div dcs
				_sirius_clock_videoout_ClockDIVConfig(10,0); //div pixel
				_sirius_clock_videoout_ClockDIVConfig(11,0); //148.5M div rsts
				_sirius_clock_videoout_ClockDIVConfig(11,1); //148.5M div rst

				break;
			default:
				gx_printf("un-supported.\n");
				return;
		}
	} else {
		_sirius_clock_videoout_ClockSourceSel(6,0);   //1: pll 0: xtal

		_sirius_clock_videoout_ClockDIVConfig(20,0);  //rst -> 0
		_sirius_clock_videoout_ClockDIVConfig(19,0);  //load_en -> 0
		_sirius_clock_videoout_ClockDIVConfig(20,1);  //rst -> 1
		_sirius_clock_videoout_ClockDIVConfig(18,0);  //bypass -> 0
		REG_SET_VAL(&(sirius_clock_reg->cfg_mepg_clk_inhibit_set), 1<<30);
		*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) &= ~(0x3f000);
		*(volatile unsigned int *)(&(sirius_clock_reg->cfg_clk)) |= 43<<12;
		_sirius_clock_videoout_ClockDIVConfig(19,1);  //load_en -> 1
		_sirius_clock_videoout_ClockDIVConfig(19,0);  //load_en -> 0

		_sirius_clock_videoout_ClockSourceSel(6,1);   //1: pll 0: xtal

	}
}
#endif

static int sirius_clock_device_cold_rst(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_VOUT:
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<9);
			gx_mdelay(1);
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<9);
			break;
		case MODULE_TYPE_VIDEO_DEC:
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<1);
			gx_mdelay(1);
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<1);
			break;
		case MODULE_TYPE_JPEG:
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<2);
			gx_mdelay(1);
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<2);
			break;
		case MODULE_TYPE_PP:
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<3);
			gx_mdelay(1);
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<3);
			break;
		case MODULE_TYPE_HDMI:
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<7);
			gx_mdelay(1);
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<7);
			break;
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<20);
			gx_mdelay(1);
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<20);
			break;
		default:
			break;
	}

	gx_mdelay(1);
	return 0;
}

static int sirius_clock_device_hot_rst_set(unsigned int module)
{
	switch (module)
	{
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_RESET_SET(sirius_clock_reg);
			break;
		case MODULE_TYPE_VOUT:
		case MODULE_TYPE_VPU:
			CFG_VPU_HOT_SET(sirius_clock_reg);
			break;
		case MODULE_TYPE_SVPU:
			break;
		case MODULE_TYPE_JPEG:
			CFG_JPEG_HOT_SET(sirius_clock_reg);
			break;
		case MODULE_TYPE_HDMI:
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_set), 0x1<<7);
			break;
		default:
			break;
	}
	return 0;
}

static int sirius_clock_device_hot_rst_clear(unsigned int module)
{
	switch (module)
	{
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_RESET_CLR(sirius_clock_reg);
			break;
		case MODULE_TYPE_VOUT:
		case MODULE_TYPE_VPU:
			CFG_VPU_HOT_CLR(sirius_clock_reg);
			break;
		case MODULE_TYPE_SVPU:
			break;
		case MODULE_TYPE_JPEG:
			CFG_JPEG_HOT_CLR(sirius_clock_reg);
			break;
		case MODULE_TYPE_HDMI:
			REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_cold_reset_clr), 0x1<<7);
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

static int sirius_clock_device_setclock(unsigned int module, clock_params* arg)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_OR:
			CFG_AUDIO_OR_SET_DTO(sirius_clock_reg, arg->audio_or.value);
			break;
		case MODULE_TYPE_AUDIO_I2S:
			if(arg->audio_i2s.clock_source == I2S_DTO) {
				CFG_AUDIO_AD_SET_DTO(sirius_clock_reg, arg->audio_i2s.value);
				CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 1);
			}
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_SPDIF_SET_DTO(sirius_clock_reg, arg->audio_spdif.value);
			if(arg->audio_spdif.clock_source == SPDIF_FROM_I2S)
				CFG_AUDIO_SPD_SEL_PLL(sirius_clock_reg);
			else
				CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 2);
			break;
		case MODULE_TYPE_VPU:
			if(arg->vpu.div)
				REG_SET_BIT(&(sirius_clock_reg->cfg_clk), arg->vpu.clock);
			else
				REG_CLR_BIT(&(sirius_clock_reg->cfg_clk), arg->vpu.clock);
			break;
#if defined CONFIG_AV_MODULE_VIDEO_OUT || defined TEE_OS
		case MODULE_TYPE_VOUT:
			_sirius_video_out_clockset(arg->vout.src, arg->vout.mode);
			break;
#endif
		case MODULE_TYPE_FRONTEND:
			if(arg->frontend.mode == FRONTEND_CLOCK_CONFIG){
				CFG_FRONTEND_DEMOD_CLK(sirius_clock_reg, arg->frontend.demod_clk_div, 1);
				CFG_FRONTEND_FEC_CLK(sirius_clock_reg, arg->frontend.fec_clk_div, 1);
				CFG_FRONTEND_ADC_CLK(sirius_clock_reg, arg->frontend.adc_clk_div, 1);
			}else if(arg->frontend.mode == FRONTEND_ADC_INIT){
				CFG_FRONTEND_ADC_INIT(sirius_clock_reg);
			}else{
				gx_printf("%s, mode error!%d\n", __func__, arg->frontend.mode);
				return -1;
			}
			break;
		default:
			break;
	}
	return 0;
}

static int sirius_clock_device_getconfig(unsigned int module, clock_configs* arg)
{
	switch(module)
	{
		case MODULE_TYPE_FRONTEND:
			arg->frontend.dvb_pll_fbdiv = (REG_GET_VAL(&(sirius_clock_reg->cfg_pll1))&0xFFF);
			break;
		default:
			break;
	}
	return 0;
}

static int sirius_clock_clksource_enable(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_OR:
			CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 15);
			CFG_DEV_INHIBIT_CLEAR(sirius_clock_reg, 0x1<<11);
			break;
		case MODULE_TYPE_AUDIO_I2S:
			CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 1);
			CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 0);
			CFG_DEV_INHIBIT_CLEAR(sirius_clock_reg, 0x1<<0);
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 2);
			CFG_DEV_INHIBIT_CLEAR(sirius_clock_reg, 0x1<<1);
			break;
		case MODULE_TYPE_DEMUX_SYS:
			CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 17);
			CFG_DEV_INHIBIT_CLEAR(sirius_clock_reg, 0x1<<13);
			break;
		case MODULE_TYPE_DEMUX_OUT:
			break;
		case MODULE_TYPE_DEMUX_STC:
			CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 18);
			CFG_DEV_INHIBIT_CLEAR(sirius_clock_reg, 0x1<<14);
			break;
		case MODULE_TYPE_DE_INTERLACE:
			break;
		case MODULE_TYPE_VOUT:
			CFG_DEV_CLOCK_SOURCE_ENABLE(sirius_clock_reg, 5);
			CFG_DEV_INHIBIT_CLEAR(sirius_clock_reg, 0x1<<3);
			break;
		default:
			break;
	}
	return 0;
}

static int sirius_clock_clocksource_disable(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_I2S:
			CFG_DEV_CLOCK_SOURCE_DISABLE(sirius_clock_reg, 1);
			CFG_DEV_INHIBIT_SET(sirius_clock_reg, 0x1<<0);
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_DEV_CLOCK_SOURCE_DISABLE(sirius_clock_reg, 2);
			CFG_DEV_INHIBIT_SET(sirius_clock_reg, 0x1<<1);
			break;
		case MODULE_TYPE_DEMUX_SYS:
			CFG_DEV_CLOCK_SOURCE_DISABLE(sirius_clock_reg, 17);
			CFG_DEV_INHIBIT_SET(sirius_clock_reg, 0x1<<13);
			break;
		case MODULE_TYPE_DEMUX_STC:
			CFG_DEV_CLOCK_SOURCE_DISABLE(sirius_clock_reg, 18);
			CFG_DEV_INHIBIT_SET(sirius_clock_reg, 0x1<<14);
			break;
		case MODULE_TYPE_DEMUX_OUT:
			break;
		case MODULE_TYPE_VOUT:
			CFG_DEV_CLOCK_SOURCE_DISABLE(sirius_clock_reg, 5);
			CFG_DEV_INHIBIT_SET(sirius_clock_reg, 0x1<<3);
			break;
		default:
			break;
	}
	return 0;
}

static int sirius_clock_audioout_dacinside(unsigned int mclock)
{
	int value = 0;

	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 0);

	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	switch (mclock) {
		case 256:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa8<<4)|(1<<2)|(1<<0)));
			break;
		case 384:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa0<<4)|(1<<2)|(1<<0)));
			break;
		case 512:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x98<<4)|(1<<2)|(1<<0)));
			break;
		case 768:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x90<<4)|(1<<2)|(1<<0)));
			break;
		case 1024:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x88<<4)|(1<<2)|(1<<0)));
			break;
		case 1536:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x80<<4)|(1<<2)|(1<<0)));
			break;
		case 128:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xb8<<4)|(1<<2)|(1<<0)));
			break;
		case 192:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xb0<<4)|(1<<2)|(1<<0)));
			break;
		default:
			REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa8<<4)|(1<<2)|(1<<0)));
			break;
	}
	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|(1<<3)|(1<<0)));

	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);

	value = REG_GET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl)) & 0xfffffffe;

	REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|((0x00|value)<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl), ((3<<12)|(0x7f<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|((0x01|value)<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl), ((3<<12)|(0x7f<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl), ((2<<12)|(0x38<<4)|(1<<2)|(1<<0)));

	REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);

	return 0;
}

static int sirius_clock_audioout_dacinside_mute(unsigned int enable)
{
	if (enable) {
		REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl), ((0x3<<12)|(0xff<<4)|(0x1<<2)|(0x1<<0)));
		REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
		REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	} else {
		REG_SET_VAL(&(sirius_clock_reg->cfg_audio_codec_ctrl), ((0x3<<12)|(0x7f<<4)|(0x1<<2)|(0x1<<0)));
		REG_SET_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
		REG_CLR_BIT(&(sirius_clock_reg->cfg_audio_codec_ctrl), 1);
	}

	return 0;
}

static int sirius_clock_audioout_dacinside_clock_enable(unsigned int enable)
{
	if (enable)
		CFG_AUDIO_LODAC_CLOCK_ENABLE(sirius_clock_reg);
	else
		CFG_AUDIO_LODAC_CLOCK_DISABLE(sirius_clock_reg);

	return 0;
}

static int sirius_clock_audioout_dacinside_slow_enable(unsigned int power, unsigned int enable)
{
	if (power) {
		if (enable) {
			REG_SET_FIELD(&(sirius_clock_reg->cfg_audio_lcodec_config1), (0x3<<24), 2, 24);
			REG_SET_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 19);
			REG_CLR_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 27);
		} else {
			REG_SET_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 31);
			REG_SET_FIELD(&(sirius_clock_reg->cfg_audio_lcodec_config1), (0x3<<24), 0, 24);
			REG_SET_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 27);
		}
	} else {
		if (enable) {
			REG_SET_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 29);
			REG_SET_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 28);
			REG_SET_FIELD(&(sirius_clock_reg->cfg_audio_lcodec_config1), (0x3<<24), 3, 24);
			REG_CLR_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 26);
			REG_SET_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 31);
		} else {
			REG_CLR_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 31);
			REG_SET_FIELD(&(sirius_clock_reg->cfg_audio_lcodec_config1), (0x3<<24), 0, 24);
			REG_SET_BIT  (&(sirius_clock_reg->cfg_audio_lcodec_config1), 26);
		}
	}

	return 0;
}

static int sirius_clock_audioout_dacinside_slow_config(unsigned int step_num, unsigned int skip_num)
{
	REG_SET_FIELD(&(sirius_clock_reg->cfg_audio_lcodec_config1), (0xf<<20), step_num, 20);
	REG_SET_VAL  (&(sirius_clock_reg->cfg_audio_lcodec_config2), skip_num);
	return 0;
}

static int sirius_clock_multiplex_pinsel(unsigned int type)
{
	gx_printf("TODO: sirius_clock_multiplex_pinsel\n");
	gx_printf("TODO: sirius_clock_multiplex_pinsel\n");
	gx_printf("TODO: sirius_clock_multiplex_pinsel\n");
	gx_printf("TODO: sirius_clock_multiplex_pinsel\n");
	gx_printf("TODO: sirius_clock_multiplex_pinsel\n");
	gx_printf("TODO: sirius_clock_multiplex_pinsel\n");
	gx_printf("TODO: sirius_clock_multiplex_pinsel\n");
	switch(type)
	{
		case PIN_TYPE_AUDIOOUT_I2S:
			//CFG_PINC_I2S_AUDIO_SEL(sirius_clock_reg);
			break;
		case PIN_TYPE_DEMUX_TS3:
			//CFG_PINC_TS3_GPIO_SEL(sirius_clock_reg);
			break;
		case PIN_TYPE_VIDEOOUT_YUV:
		//	CFG_PINA_VIDEOOUT_YUV_SEL(sirius_clock_reg);
			break;
		case PIN_TYPE_VIDEOOUT_SCART:
		//	CFG_PINA_VIDEOOUT_SCART_SEL(sirius_clock_reg);
			break;
		default:
			break;
	}
	return 0;
}

static int sirius_clock_video_dac_source(unsigned int id, enum video_dac_clk_source src)
{
	int bit = 8 + id;

	if (src == VIDEO_DAC_SRC_VPU)
		REG_CLR_BIT(&(sirius_clock_reg->cfg_source_sel), bit);
	else
		REG_SET_BIT(&(sirius_clock_reg->cfg_source_sel), bit);

	return 0;
}

static int sirius_clock_jpeg_aix_enable(int flag)
{
	if(flag) {
		REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_clk_inhibit2_clr), 1<<0);
	} else {
		REG_SET_VAL(&(sirius_clock_reg->cfg_mpeg_clk_inhibit2_set), 1<<0);
	}
	return (0);
}

static int sirius_clock_jpeg_module_enable(int flag)
{
	if(flag) {
		REG_SET_VAL(&(sirius_clock_reg->cfg_mepg_clk_inhibit_clr), 1<<9);
	} else {
		REG_SET_VAL(&(sirius_clock_reg->cfg_mepg_clk_inhibit_set), 1<<9);
	}
	return (0);
}

static int sirius_clock_ga_module_enable(int flag)
{
	if(flag) {
		REG_SET_VAL(&(sirius_clock_reg->cfg_mepg_clk_inhibit_clr), 1<<12);
	} else {
		REG_SET_VAL(&(sirius_clock_reg->cfg_mepg_clk_inhibit_set), 1<<12);
	}
	return (0);
}

static int sirius_clock_pp_module_enable(int flag)
{
	if(flag) {
		REG_SET_VAL(&(sirius_clock_reg->cfg_mepg_clk_inhibit_clr), 1<<10);
	} else {
		REG_SET_VAL(&(sirius_clock_reg->cfg_mepg_clk_inhibit_set), 1<<10);
	}
	return (0);
}

static int sirius_module_clock_enable(int module, int flag)
{
	switch(module) {
		case MODULE_TYPE_JPEG:
			sirius_clock_jpeg_aix_enable(flag);
			sirius_clock_jpeg_module_enable(flag);
			break;
		case MODULE_TYPE_GA:
			sirius_clock_ga_module_enable(flag);
			break;
		case MODULE_TYPE_PP:
			sirius_clock_pp_module_enable(flag);
			break;
		default:
			break;
	}
	return (0);
}

struct gxav_clock_module sirius_clock_module = {

	.init = sirius_clock_init,
	.uninit = sirius_clock_uninit,
	.cold_rst = sirius_clock_device_cold_rst,
	.hot_rst_set = sirius_clock_device_hot_rst_set,
	.hot_rst_clr = sirius_clock_device_hot_rst_clear,
	.setclock = sirius_clock_device_setclock,
	.getconfig = sirius_clock_device_getconfig,
	.vdac_clock_source = sirius_clock_video_dac_source,
	.source_enable = sirius_clock_clksource_enable,
	.source_disable = sirius_clock_clocksource_disable,
	.audioout_dacinside              = sirius_clock_audioout_dacinside,
	.audioout_dacinside_mute         = sirius_clock_audioout_dacinside_mute,
	.audioout_dacinside_clock_enable = sirius_clock_audioout_dacinside_clock_enable,
	.audioout_dacinside_slow_enable  = sirius_clock_audioout_dacinside_slow_enable,
	.audioout_dacinside_slow_config  = sirius_clock_audioout_dacinside_slow_config,
	.module_clock_enable             = sirius_module_clock_enable,
	.multiplex_pinsel = sirius_clock_multiplex_pinsel,

};
