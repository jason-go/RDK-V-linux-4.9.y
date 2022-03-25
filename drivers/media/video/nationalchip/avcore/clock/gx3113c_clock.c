#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "clock_hal.h"
#include "gx3113c_clock_reg.h"

extern int gx3113c_vpu_SetBufferStateDelay(int v);
extern int gx3113c_vpu_GetScanInfo(int *scan_line, int *top);

static volatile struct clock_regs *gx3113c_clock_reg = NULL;
static unsigned int gx3113c_module_clk[1] = {0};

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

int gx3113c_clock_init(void)
{
	if(gx3113c_clock_reg == NULL){
		gx3113c_clock_reg = gx_ioremap(CLOCK_BASE_ADDR, sizeof(struct clock_regs));
		if (gx3113c_clock_reg == NULL) {
			gx_printf("%s,Ioremap failed.\n",__func__);
			return -1;
		}

		CLOCK_DBG("[gx3113c_clock_reg=%x]\n",(int)gx3113c_clock_reg);
		memset(gx3113c_module_clk, 0, sizeof(gx3113c_module_clk));
	}

	return 0;
}

int gx3113c_clock_uninit(void)
{
	if(gx3113c_clock_reg != NULL){
		gx_iounmap(gx3113c_clock_reg);
		gx3113c_clock_reg = NULL;
	}

	return 0;
}

static void _gx3113c_clock_videoout_ClockSourceSel(unsigned int select, unsigned int set)
{
	if(set)
		REG_SET_BIT(&(gx3113c_clock_reg->cfg_source_sel), select);
	else
		REG_CLR_BIT(&(gx3113c_clock_reg->cfg_source_sel), select);
}

static void _gx3113c_clock_videoout_ClockDIVConfig(unsigned int clock, unsigned int div)
{
	if(div){
		REG_SET_BIT(&(gx3113c_clock_reg->cfg_clk), clock);
	}else{
		REG_CLR_BIT(&(gx3113c_clock_reg->cfg_clk), clock);
	}
}

static void _gx3113c_video_out_clockset(unsigned int src, unsigned int mode)
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
			gx3113c_vpu_SetBufferStateDelay(0x50);

			_gx3113c_clock_videoout_ClockDIVConfig(8,0);  //rst -> 0
			_gx3113c_clock_videoout_ClockDIVConfig(8,1);  //rst -> 1
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) &= ~(0x3f);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) |= 0x1e;
			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0
			_gx3113c_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1

			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) &= ~(3<<9);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) |= (1<<9);

			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) &= ~(1<<11);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) |= (1<<11);

			_gx3113c_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M

			break;

		case GXAV_VOUT_480P:

			gx3113c_vpu_SetBufferStateDelay(0x60);

			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_mepg_clk_inhibit))	|=	(1<<30); //EN SVPU CORRECT
			_gx3113c_clock_videoout_ClockSourceSel(4,0); //sel pll video 297M

			_gx3113c_clock_videoout_ClockDIVConfig(8,0);  //rst -> 0
			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0

			_gx3113c_clock_videoout_ClockDIVConfig(8,1);  //rst -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(6,0);  //bypass -> 0
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) &= ~(0x3f);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) |= 20;//ratio;

			_gx3113c_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0


			_gx3113c_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M


			_gx3113c_clock_videoout_ClockDIVConfig(9,1);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(10,0);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(11,0);  //148.5M div rst
			_gx3113c_clock_videoout_ClockDIVConfig(11,1);  //148.5M div rst

			break;
		case GXAV_VOUT_576P:
			gx3113c_vpu_SetBufferStateDelay(0x60);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_mepg_clk_inhibit))	|=	(1<<30); //DIS SVPU CORRECT

			_gx3113c_clock_videoout_ClockSourceSel(4,0); //sel pll video 297M

			_gx3113c_clock_videoout_ClockDIVConfig(8,0);  //rst -> 0
			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0

			_gx3113c_clock_videoout_ClockDIVConfig(8,1);  //rst -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(6,0);  //bypass -> 0
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) &= ~(0x3f);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) |= 20;//ratio;

			_gx3113c_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0


			_gx3113c_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M


			_gx3113c_clock_videoout_ClockDIVConfig(9,1);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(10,0);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(11,0);  //148.5M div rst
			_gx3113c_clock_videoout_ClockDIVConfig(11,1);  //148.5M div rst

			break;

		case GXAV_VOUT_720P_50HZ:
		case GXAV_VOUT_1080I_50HZ:
			gx3113c_vpu_SetBufferStateDelay(0x30);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_mepg_clk_inhibit))	|=	(1<<30); //DIS SVPU CORRECT

			_gx3113c_clock_videoout_ClockSourceSel(4,0); //sel pll video 297M

			_gx3113c_clock_videoout_ClockDIVConfig(8,0);  //rst -> 0
			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0

			_gx3113c_clock_videoout_ClockDIVConfig(8,1);  //rst -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(6,0);  //bypass -> 0
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) &= ~(0x3f);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) |= 6;//ratio;
			//*(volatile unsigned int *)(0xa030a000+0x24) |= 14;//ratio;huanglei tmp 2012-10-11
			_gx3113c_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0


			_gx3113c_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M

			//_gx3113c_clock_videoout_ClockDIVConfig(9,0);  //148.5M div config huanglei tmp 2012-10-11
			_gx3113c_clock_videoout_ClockDIVConfig(9,1);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(10,0);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(11,0);  //148.5M div rst
			_gx3113c_clock_videoout_ClockDIVConfig(11,1);  //148.5M div rst

			break;

		case GXAV_VOUT_720P_60HZ:
		case GXAV_VOUT_1080I_60HZ:
			gx3113c_vpu_SetBufferStateDelay(0x30);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_mepg_clk_inhibit))	|=	(1<<30); //DIS SVPU CORRECT

			_gx3113c_clock_videoout_ClockSourceSel(4,1); //sel pll video 297M

			_gx3113c_clock_videoout_ClockDIVConfig(8,0);  //rst -> 0
			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0

			_gx3113c_clock_videoout_ClockDIVConfig(8,1);  //rst -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(6,0);  //bypass -> 0
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) &= ~(0x3f);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) |= 6;//ratio;

			_gx3113c_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0


			_gx3113c_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M

			_gx3113c_clock_videoout_ClockDIVConfig(11,0);  //148.5M div rst
			_gx3113c_clock_videoout_ClockDIVConfig(11,1);  //148.5M div rst
			_gx3113c_clock_videoout_ClockDIVConfig(9,1);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(10,0);  //148.5M div config

			break;

		case GXAV_VOUT_1080P_50HZ:
			gx3113c_vpu_SetBufferStateDelay(0x18);
			//VPU_SET_BUFF_STATE_DELAY(gx3201vpu_reg->rBUFF_CTRL2, 0x40);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_mepg_clk_inhibit))	|=	(1<<30); //DIS SVPU CORRECT

			_gx3113c_clock_videoout_ClockSourceSel(4,0);

			_gx3113c_clock_videoout_ClockDIVConfig(8,0);  //rst -> 0
			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0

			_gx3113c_clock_videoout_ClockDIVConfig(8,1);  //rst -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(6,0);  //bypass -> 0
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) &= ~(0x3f);
			*(volatile unsigned int *)(&(gx3113c_clock_reg->cfg_clk)) |= 2;//ratio;

			_gx3113c_clock_videoout_ClockDIVConfig(7,1);  //load_en -> 1

			_gx3113c_clock_videoout_ClockDIVConfig(7,0);  //load_en -> 0


			_gx3113c_clock_videoout_ClockSourceSel(5,1); //sel pll video 148.5M


			_gx3113c_clock_videoout_ClockDIVConfig(9,1);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(10,1);  //148.5M div config
			_gx3113c_clock_videoout_ClockDIVConfig(11,0);  //148.5M div rst
			_gx3113c_clock_videoout_ClockDIVConfig(11,1);  //148.5M div rst

			break;
		default:
			gx_printf("un-supported.\n");
			return;
		}
	}
}

static int gx3113c_clock_device_cold_rst(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_VOUT:
			CFG_VIDEO0_COLD_SET(gx3113c_clock_reg);
			gx_mdelay(1);
			CFG_VIDEO0_COLD_CLR(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_VIDEO_DEC:
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset_set), 1);
			gx_mdelay(1);
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset_clr), 1);
			break;
		case MODULE_TYPE_JPEG:
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset_set), 2);
			gx_mdelay(1);
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset_clr), 2);
			break;
		case MODULE_TYPE_PP:
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset_set), 3);
			gx_mdelay(1);
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset_clr), 3);
			break;
		case MODULE_TYPE_HDMI:
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_rst_2_1set), 13);
			gx_mdelay(1);
			REG_CLR_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_rst_2_1set), 13);
			break;
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset_set), 20);
			gx_mdelay(1);
			REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset_clr), 20);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3113c_clock_device_hot_rst_set(unsigned int module)
{
	switch (module)
	{
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_RESET_SET(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_VOUT:
			CFG_VIDEOOUT0_HOT_SET(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_VPU:
			CFG_VPU_HOT_SET(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_SVPU:
			CFG_SVPU_HOT_SET(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_JPEG:
			CFG_JPEG_HOT_SET(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_HDMI:
			gx3113c_clock_reg->cfg_mpeg_cold_reset |= (1<<7);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3113c_clock_device_hot_rst_clear(unsigned int module)
{
	switch (module)
	{
		case MODULE_TYPE_AUDIO_I2S:
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_RESET_CLR(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_VOUT:
			CFG_VIDEOOUT0_HOT_CLR(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_VPU:
			CFG_VPU_HOT_CLR(gx3113c_clock_reg);
		case MODULE_TYPE_SVPU:
			CFG_SVPU_HOT_CLR(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_JPEG:
			CFG_JPEG_HOT_CLR(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_HDMI:
			gx3113c_clock_reg->cfg_mpeg_cold_reset &= ~(1<<7);
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

static void _gx3113c_audioout_pllclockset(unsigned int index, unsigned int speed)
{
	unsigned int bwadj,clkf,clkr,clkod,od;

	bwadj  = gxav_audioout_pllclk[index][0]*speed/100;
	clkr  = gxav_audioout_pllclk[index][1];
	clkod  = gxav_audioout_pllclk[index][2];
	clkf  = gxav_audioout_pllclk[index][3]*speed/100;
	od = (gxav_audioout_pllclk[index][4]>>1)-1;

	gx3113c_clock_reg->cfg_source_sel &=~(1<<1); //select clock start

	gx3113c_clock_reg->cfg_pll5 = (clkf-1)|((clkr-1)<<16)|((clkod-1)<<24);
	gx3113c_clock_reg->cfg_pll6 = 1;
	gx3113c_clock_reg->cfg_pll6 = (1<<7)|((bwadj-1)<<16);

	gx3113c_clock_reg->cfg_clk &=~(0x1f<<24);
	gx3113c_clock_reg->cfg_clk &=~(1<<31);
	gx3113c_clock_reg->cfg_clk |=(1<<31)|(od<<25);
	gx3113c_clock_reg->cfg_clk |=(1<<31)|(od<<25)|(1<<30);
	gx3113c_clock_reg->cfg_clk |=(1<<31)|(od<<25);

	gx3113c_clock_reg->cfg_source_sel &= ~(1);//select PLL
	gx3113c_clock_reg->cfg_source_sel |= (1<<1);//select i2s clock

}

static int gx3113c_clock_device_setclock(unsigned int module, clock_params* arg)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_OR:
			CFG_AUDIO_OR_SET_DTO(gx3113c_clock_reg, arg->audio_or.value);
			break;
		case MODULE_TYPE_AUDIO_I2S:
			if(arg->audio_i2s.clock_source == I2S_DTO) {
				CFG_AUDIO_AD_SET_DTO(gx3113c_clock_reg, arg->audio_i2s.value);
				CFG_AUDIO_AD_SEL_DTO(gx3113c_clock_reg);
			}else{
				_gx3113c_audioout_pllclockset(arg->audio_i2s.sample_index, arg->audio_i2s.clock_speed);
			}
			break;
		case MODULE_TYPE_AUDIO_LODEC:
			if(arg->audio_lodec.clock_source == I2S_DTO) {
				CFG_AUDIO_LODEC_SET_DTO(gx3113c_clock_reg, arg->audio_lodec.value);
			}
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_AUDIO_SPDIF_SET_DTO(gx3113c_clock_reg, arg->audio_spdif.value);
			if(arg->audio_spdif.clock_source == SPDIF_FROM_I2S)
				CFG_AUDIO_SPD_SEL_PLL(gx3113c_clock_reg);
			else
				CFG_AUDIO_SPD_SEL_DTO(gx3113c_clock_reg);
			break;
		case MODULE_TYPE_VPU:
			if(arg->vpu.div)
				REG_SET_BIT(&(gx3113c_clock_reg->cfg_clk), arg->vpu.clock);
			else
				REG_CLR_BIT(&(gx3113c_clock_reg->cfg_clk), arg->vpu.clock);
			break;
		case MODULE_TYPE_VOUT:
			_gx3113c_video_out_clockset(arg->vout.src, arg->vout.mode);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3113c_clock_clksource_enable(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_OR:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3113c_clock_reg, 15);
			break;
		case MODULE_TYPE_AUDIO_I2S:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3113c_clock_reg, 1);
			break;
		case MODULE_TYPE_AUDIO_LODEC:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3113c_clock_reg, 0);
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3113c_clock_reg, 2);
			break;
		case MODULE_TYPE_DEMUX_SYS:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_DEMUX_SYS);
			break;
		case MODULE_TYPE_DEMUX_OUT:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_DEMUX_OUT);
			break;
		case MODULE_TYPE_DEMUX_STC:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_DEMUX_STC);
			break;
		case MODULE_TYPE_DE_INTERLACE:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_DE_INTERLACE);
			break;
		case MODULE_TYPE_VOUT:
			CFG_DEV_CLOCK_SOURCE_ENABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_VOUT0);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3113c_clock_clocksource_disable(unsigned int module)
{
	switch(module)
	{
		case MODULE_TYPE_AUDIO_I2S:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_AUDIO_I2S);
			break;
		case MODULE_TYPE_AUDIO_SPDIF:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3113c_clock_reg, 2);
			break;
		case MODULE_TYPE_DEMUX_SYS:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_DEMUX_SYS);
			break;
		case MODULE_TYPE_DEMUX_STC:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_DEMUX_STC);
			break;
		case MODULE_TYPE_DEMUX_OUT:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_DEMUX_OUT);
			break;
		case MODULE_TYPE_VOUT:
			CFG_DEV_CLOCK_SOURCE_DISABLE(gx3113c_clock_reg, bCFG_CLOCK_SOURCE_VOUT0);
			break;
		default:
			break;
	}
	return 0;
}

static int gx3113c_clock_audioout_dacinside(unsigned int mclock)
{
	int value = 0;

	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 0);

	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	switch (mclock) {
		case 256:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa8<<4)|(1<<2)|(1<<0)));
			break;
		case 384:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa0<<4)|(1<<2)|(1<<0)));
			break;
		case 512:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x98<<4)|(1<<2)|(1<<0)));
			break;
		case 768:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x90<<4)|(1<<2)|(1<<0)));
			break;
		case 1024:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x88<<4)|(1<<2)|(1<<0)));
			break;
		case 1536:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0x80<<4)|(1<<2)|(1<<0)));
			break;
		case 128:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xb8<<4)|(1<<2)|(1<<0)));
			break;
		case 192:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xb0<<4)|(1<<2)|(1<<0)));
			break;
		default:
			REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl),
					((0<<12)|(0xa8<<4)|(1<<2)|(1<<0)));
			break;
	}

	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|(1<<3)|(1<<0)));

	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);

	value = REG_GET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl)) & 0xfffffffe;

	REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|((0x00|value)<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), ((3<<12)|(0x7f<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), ((0<<12)|((0x01|value)<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), ((3<<12)|(0x7f<<4)|(1<<2)|(1<<0)));
	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);

	REG_SET_VAL(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), ((2<<12)|(0x38<<4)|(1<<2)|(1<<0)));

	REG_SET_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);
	REG_CLR_BIT(&(gx3113c_clock_reg->cfg_audio_codec_ctrl), 1);

	return 0;
}

static int gx3113c_clock_multiplex_pinsel(unsigned int type)
{
	switch(type)
	{
		case PIN_TYPE_AUDIOOUT_I2S:
			CFG_PINC_I2S_AUDIO_SEL(gx3113c_clock_reg);
			break;
		case PIN_TYPE_DEMUX_TS3:
			CFG_PINC_TS3_GPIO_SEL(gx3113c_clock_reg);
			break;
		case PIN_TYPE_VIDEOOUT_YUV:
			CFG_PINA_VIDEOOUT_YUV_SEL(gx3113c_clock_reg);
			break;
		case PIN_TYPE_VIDEOOUT_SCART:
			CFG_PINA_VIDEOOUT_SCART_SEL(gx3113c_clock_reg);
			break;
		default:
			break;
	}
	return 0;
}

struct gxav_clock_module gx3113c_clock_module = {

	.init = gx3113c_clock_init,
	.uninit = gx3113c_clock_uninit,
	.cold_rst = gx3113c_clock_device_cold_rst,
	.hot_rst_set = gx3113c_clock_device_hot_rst_set,
	.hot_rst_clr = gx3113c_clock_device_hot_rst_clear,
	.setclock = gx3113c_clock_device_setclock,
	.source_enable = gx3113c_clock_clksource_enable,
	.source_disable = gx3113c_clock_clocksource_disable,
	.audioout_dacinside = gx3113c_clock_audioout_dacinside,
	.multiplex_pinsel = gx3113c_clock_multiplex_pinsel,

};
