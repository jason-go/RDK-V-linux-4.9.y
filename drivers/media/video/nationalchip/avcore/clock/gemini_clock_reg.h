#ifndef __GEMINI_CLOCK_REG_H__
#define __GEMINI_CLOCK_REG_H__

#include "gxav_bitops.h"

#define CLOCK_BASE_ADDR (0x0030A000)
#define DTO_BASE_ADDR   (0x00601000)

#define BIT_DTO_PLL_F  0
#define BIT_DTO_PLL_R  12
#define BIT_DTO_PLL_OD 8

#define DTO_PLL_F_MASK  ( 0x3F << BIT_DTO_PLL_F)
#define DTO_PLL_R_MASK  ( 0x0F << BIT_DTO_PLL_R)
#define DTO_PLL_OD_MASK ( 0x3F << BIT_DTO_PLL_OD)

#define DTO_PLL_F	(REG_GET_FIELD(&(gemini_clock_reg->cfg_pll4), DTO_PLL_F_MASK, BIT_DTO_PLL_F))
#define DTO_PLL_R	(REG_GET_FIELD(&(gemini_clock_reg->cfg_pll4), DTO_PLL_R_MASK, BIT_DTO_PLL_R))
#define DTO_PLL_OD	(REG_GET_FIELD(&(gemini_clock_reg->cfg_pll4), DTO_PLL_OD_MASK, BIT_DTO_PLL_OD))

struct dto_regs {
	unsigned int   cfg_dto[16];
};

struct clock_regs {
	unsigned int   cfg_mpeg_cold_reset;
	unsigned int   cfg_mpeg_cold_reset_set;
	unsigned int   cfg_mpeg_cold_reset_clr;
	unsigned int   cfg_mpeg_hot_reset;           /* 0x0c */
	unsigned int   cfg_mpeg_hot_reset_set;
	unsigned int   cfg_mpeg_hot_reset_clr;
	unsigned int   cfg_mepg_clk_inhibit;
	unsigned int   cfg_mepg_clk_inhibit_set;
	unsigned int   cfg_mepg_clk_inhibit_clr;     /* 0x20 */
	unsigned int   cfg_clk;
	unsigned int   cfg_resv0[16];
	unsigned int   cfg_mpeg_clk_inhibit2_norm;   /* 0x68 */
	unsigned int   cfg_mepg_clk_inhibit2_set;
	unsigned int   cfg_mepg_clk_inhibit2_clr;    /* 0x70 */
	unsigned int   cfg_resv1[19];
	unsigned int   cfg_pll[4];                   /* 0xc0 */
	unsigned int   cfg_resv2[6];
	unsigned int   cfg_pll_in;                   /* 0xe8 */
	unsigned int   cfg_resv3[5];
	unsigned int   cfg_usb_config[4];            /* 0x100 */
	unsigned int   dvb_config;                   /* 0x110 */
	unsigned int   ephy_config;
	unsigned int   cfg_resv4[2];
	unsigned int   dram_config[4];               /* 0x120 */
	unsigned int   dram_status[3];               /* 0x130 */
	unsigned int   pin_func_sel[6];
	unsigned int   low_power;                    /* 0x154 */
	unsigned int   ddr_status;                   /* 0x158 */
	unsigned int   cfg_resv5[5];
	unsigned int   cfg_source_sel;               /* 0x170 */
	unsigned int   cfg_source_sel2;              /* 0x174 */
	unsigned int   clock_div_config2;
	unsigned int   clock_div_config3;
	unsigned int   cfg_chip_info;                /* 0x180 */
	unsigned int   cfg_chip_id;                  /* 0x184 */
	unsigned int   cfg_resv6[2];
	unsigned int   cfg_chip_name[4];             /* 0x190 */
	unsigned int   cfg_audio_codec_data;         /* 0x1a0 */
	unsigned int   cfg_audio_codec_ctrl;
	unsigned int   cfg_audio_lcodec_config1;
	unsigned int   cfg_audio_lcodec_config2;
	unsigned int   cfg_eth_config;               /* 0x1b0 */
	unsigned int   cfg_resv7[3];
	unsigned int   cfg_mpeg_cold_reset2;         /* 0x1c0 */
	unsigned int   cfg_mpeg_cold_reset2_set;
	unsigned int   cfg_mpeg_cold_reset2_clr;
	unsigned int   cfg_resv8[1];
	unsigned int   cfg_mpeg_hot_reset2;         /* 0x1d0 */
	unsigned int   cfg_mpeg_hot_reset2_set;
	unsigned int   cfg_mpeg_hot_reset2_clr;
	unsigned int   cfg_resv9[1];
	unsigned int   cfg_hdmi_reg_data;           /* 0x1e0 */
	unsigned int   cfg_hdmi_reg_ctrl;           /* 0x1e4 */
};

/* rCFG_dto[0] and rCFG_dto[1] register bit definition */
#define vCFG_DTO_STEP                  (0x3FFFFFFF)
#define vCFG_DTO_LOAD                  (1 << 30)
#define vCFG_DTO_WSEL                  (1 << 31)

#define vCFG_MPEG_HOT_SDC              (1 << 0)
#define vCFG_MPEG_HOT_AUDIO_AD_SDC     (1 << 1)
#define vCFG_MPEG_HOT_AUDIO_AD         (1 << 2)
#define vCFG_MPEG_HOT_AUDIO_I2S_SDC    (1 << 4)
#define vCFG_MPEG_HOT_AUDIO_SPDIF_SDC  (1 << 5)
#define vCFG_MPEG_HOT_AUDIO_I2S        (1 << 6)
#define vCFG_MPEG_HOT_AUDIO_SPDIF      (1 << 7)
#define vCFG_MPEG_HOT_AUDIO_SPDIF_CH   (1 << 8)
#define vCFG_MPEG_HOT_TV_INPUT         (1 << 9)
#define vCFG_H264_HOT_VIDEO_HIGHT      (1 << 11)
#define vCFG_H264_HOT_VIDEO_LOW        (1 << 13)
#define vCFG_MPEG_HOT_VIDEO_MPEG2      (1 << 14)
#define vCFG_MPEG_HOT_GA               (1 << 15)
#define vCFG_MPEG_HOT_OSD              (1 << 19)
#define vCFG_MPEG_HOT_PP               (1 << 20)
#define vCFG_MPEG_HOT_PIC              (1 << 21)
#define vCFG_MPEG_HOT_DVE              (1 << 22)
#define vCFG_MPEG_HOT_DEMUX_SDC        (1 << 28)
#define vCFG_MPEG_HOT_DEMUX_SYS        (1 << 29)
#define vCFG_H264_HOT_DISP             (1 << 30)
#define vCFG_MPEG_HOT_DISP             (1 << 31)

#define vCFG_MPEG_COLD_SDC             (1 << 0)
#define vCFG_MPEG_COLD_AUDIO_AD_SDC    (1 << 1)
#define vCFG_MPEG_COLD_AUDIO_AD        (1 << 2)
#define vCFG_MPEG_COLD_AUDIO_OR        (1 << 3)
#define vCFG_MPEG_COLD_AUDIO_I2S_SDC   (1 << 4)
#define vCFG_MPEG_COLD_AUDIO_I2S       (1 << 6)
#define vCFG_MPEG_COLD_AUDIO_SPDIF     (1 << 7)
#define vCFG_MPEG_COLD_TV_INPUT        (1 << 9)
#define vCFG_H264_COLD_VIDEO_HIGHT     (1 << 11)
#define vCFG_H264_COLD_VIDEO_H264      (1 << 12)
#define vCFG_H264_COLD_VIDEO_LOW       (1 << 13)
#define vCFG_MPEG_COLD_VIDEO_MPEG2     (1 << 14)
#define vCFG_MPEG_COLD_GA              (1 << 15)
#define vCFG_MPEG_COLD_VPU             (1 << 18)
#define vCFG_MPEG_COLD_DVE             (1 << 22)
#define vCFG_MPEG_COLD_DEMUX_SDC       (1 << 28)
#define vCFG_MPEG_COLD_DEMUX_SYS       (1 << 29)
#define vCFG_H264_COLD_DISP            (1 << 30)
#define vCFG_MPEG_COLD_DISP            (1 << 31)

#define bCFG_CLOCK_SOURCE_AUDIO_I2S          (0)
#define bCFG_CLOCK_SOURCE_AUDIO_SPDIF        (17)
#define bCFG_CLOCK_SOURCE_AUDIO_PLAY         (30)
#define bCFG_CLOCK_SOURCE_DE_INTERLACE       (3)

#define bCFG_CLOCK_SOURCE_DEMUX_SYS          (14)
#define bCFG_CLOCK_SOURCE_DEMUX_OUT          (7)
#define bCFG_CLOCK_SOURCE_DEMUX_STC          (15)
#define bCFG_CLOCK_SOURCE_AUDIO_CODEC        (9)
#define bCFG_CLOCK_SOURCE_VOUT0              (12)
#define bCFG_CLOCK_SOURCE_VOUT1              (24)

#define bCFG_AUDIO_I2S_SELECT_DTO            (27)
#define bCFG_AUDIO_SPD_SELECT_DTO            (13)
#define bCFG_AUDIO_AD_SELECT_DTO             (12)
#define bCFG_VIDEO_OUT_CLOCK_DIV             (7)


/* Exported Variables ----------------------------------------------------- */
#define CFG_DEV_HOT_GET(base)                      \
    REG_GET_VAL(&(base->cfg_mpeg_hot_reset))

#define CFG_DEV_HOT_SET(base,val)                       \
    REG_SET_VAL(&(base->cfg_mpeg_hot_reset_set),val)

#define CFG_DEV_HOT_CLEAR(base,val)                     \
    REG_SET_VAL(&(base->cfg_mpeg_hot_reset_clr),val)

#define CFG_DEV_COLD_GET(base)                          \
    REG_GET_VAL(&(base->cfg_mpeg_cold_reset))

#define CFG_DEV_COLD_SET(base,val)                      \
    REG_SET_VAL(&(base->cfg_mpeg_cold_reset_set),val)

#define CFG_DEV_COLD_CLEAR(base,val)                    \
    REG_SET_VAL(&(base->cfg_mpeg_cold_reset_clr),val)

#define CFG_DEV_INHIBIT_SET(base,val)                   \
    REG_SET_VAL(&(base->cfg_mepg_clk_inhibit_set),val)

#define CFG_DEV_INHIBIT_CLEAR(base,val)                 \
    REG_SET_VAL(&(base->cfg_mepg_clk_inhibit_clr),val)

#define CFG_DEV_CLOCK_SOURCE_ENABLE(base,bit)           \
    REG_SET_BIT(&(base->cfg_source_sel),bit)

#define CFG_DEV_CLOCK_SOURCE_DISABLE(base,bit)          \
    REG_CLR_BIT(&(base->cfg_source_sel),bit)

#define CFG_AUDIO_I2S_PLL7_SET(base,val)                \
    REG_SET_VAL(&(base->cfg_pll7),val)

#define CFG_AUDIO_I2S_CLOCK_SET(base,val)               \
    REG_SET_FIELD( &(base->cfg_clk), (0x1f<<22), val, 22)

#define CFG_AUDIO_I2S_SEL_DTO(base) \
    REG_SET_BIT(&(base->cfg_clk),bCFG_AUDIO_I2S_SELECT_DTO)

#define CFG_AUDIO_SPD_SEL_DTO(base) \
    REG_SET_BIT(&(base->cfg_clk),bCFG_AUDIO_SPD_SELECT_DTO)
#define CFG_AUDIO_SPD_SEL_PLL(base) \
    REG_CLR_BIT(&(base->cfg_clk),bCFG_AUDIO_SPD_SELECT_DTO)

#define CFG_AUDIO_AD_SEL_DTO(base) \
    REG_SET_BIT(&(base->cfg_clk),bCFG_AUDIO_AD_SELECT_DTO)

#define CFG_VIDEO_OUT_CLOCK_DIV_EN(base) \
    REG_SET_BIT(&(base->cfg_clk),bCFG_VIDEO_OUT_CLOCK_DIV)

#define CFG_VIDEO_OUT_CLOCK_DIV_DIS(base) \
    REG_CLR_BIT(&(base->cfg_clk),bCFG_VIDEO_OUT_CLOCK_DIV)

#define CFG_AUDIO_AD_SET_DTO(base,val)                      \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto[0]),0);               \
        REG_SET_VAL(&(base->cfg_dto[0]), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff)));                 \
        REG_SET_VAL(&(base->cfg_dto[0]), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto[0]), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff)));                 \
    } while(0)

#define CFG_AUDIO_LODEC_SET_DTO(base,val)                      \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto[0]), ((1<<31) | (val & 0x3fffffff)));                 \
        REG_SET_VAL(&(base->cfg_dto[0]), ((1<<31) | (1<<30) | (val & 0x3fffffff)));       \
    } while(0)

#define CFG_AUDIO_I2S_SET_DTO(base,val)                     \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto[0]),0);                   \
        REG_SET_VAL(&(base->cfg_dto[0]), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | ((val) & 0x3fffffff))); \
    } while(0)

#define CFG_AUDIO_SPDIF_SET_DTO(base,val)                   \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto[1]),0);                   \
        REG_SET_VAL(&(base->cfg_dto[1] ), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto[1] ), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto[1] ), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff))); \
    } while(0)

#define CFG_DE_INTERLACE_SET_DTO(base,val)                  \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto[3]),0);                   \
        REG_SET_VAL(&(base->cfg_dto[3] ), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | (FREQ_TO_DTO_STEP(val) & 0x3fffffff))); \
    } while(0)

#define CFG_AUDIO_OR_SET_DTO(base,val)                   \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto[5]),0);                   \
        REG_SET_VAL(&(base->cfg_dto[5] ), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto[5] ), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto[5] ), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff))); \
    } while(0)


#define CFG_VIDEO_DEC_SET_DTO(base,val) do {                \
        REG_SET_BIT(&(base->cfg_source_sel),3);             \
        REG_CLR_BIT(&(base->cfg_source_sel),3);             \
        REG_SET_VAL(&(base->cfg_dto3),1<<31);               \
        REG_SET_VAL(&(base->cfg_dto3),0);                   \
        REG_SET_VAL(&(base->cfg_dto3),1<<31);               \
        REG_SET_VAL(&(base->cfg_dto3),(1<<31)|val);         \
        REG_SET_VAL(&(base->cfg_dto3),(1<<31)|(1<<30)|val); \
        REG_SET_VAL(&(base->cfg_dto3),(1<<31)|(1<<30)|val); \
        REG_SET_VAL(&(base->cfg_dto3),(1<<31)|val);         \
    } while(0)


#define CFG_DEMUX_SYS_SET_DTO(base,val) do {                \
        REG_SET_VAL(&(base->cfg_dto7), 0);                  \
        REG_SET_VAL(&(base->cfg_dto7), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | (FREQ_TO_DTO_STEP(val) & 0x3fffffff))); \
    } while(0)

#define CFG_DEMUX_STC_SET_DTO(base,val) do {                \
        REG_SET_VAL(&(base->cfg_dto8), 0);                  \
        REG_SET_VAL(&(base->cfg_dto8), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | (FREQ_TO_DTO_STEP(val) & 0x3fffffff))); \
    } while(0)


#define CFG_DEMUX_OUT_SET_DTO(base,val) do {                \
        REG_SET_VAL(&(base->cfg_dto9), 0);                  \
        REG_SET_VAL(&(base->cfg_dto9), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | (FREQ_TO_DTO_STEP(val) & 0x3fffffff))); \
    } while(0)

#define CFG_AUDIO_LODAC_CLOCK_ENABLE(base) \
	REG_CLR_BIT(&(base->cfg_mpeg_clk_inhibit2_norm), 20)

#define CFG_AUDIO_LODAC_CLOCK_DISABLE(base) \
	REG_SET_BIT(&(base->cfg_mpeg_clk_inhibit2_norm), 20)

#define CFG_VIDEO_OUT_PLL(base,val)  \
    REG_SET_VAL(&(base->cfg_pll3),val)

#define  CFG_AUDIO_DAC_CLKTOGGLE(base) \
    do {  \
        REG_SET_BITS(&(base->cfg_audio_codec_ctrl),0x40001);\
        REG_CLR_BITS(&(base->cfg_audio_codec_ctrl),1);      \
    } while(0)

#define CFG_AUDIO_DAC_WRITE(base,regaddr,val) \
    REG_SET_VAL(&(base->cfg_audio_codec_ctrl), ( (1 << 18) |(regaddr << 9) | (val << 1) | (1 << 16)))

#define CFG_AUDIO_DAC_READ_CTRL(base,regaddr) \
    REG_SET_VAL(&(base->cfg_audio_codec_ctrl), ((1 << 18)|(regaddr << 9) | (1<< 17)))

#define CFG_AUDIO_DAC_READ(base)  \
    REG_GET_VAL(&(base->cfg_audio_codec_data))

#define CFG_AUDIO_RESET_SET(base) \
    REG_SET_VAL(&(base->cfg_mpeg_hot_reset_set), 0x1<<20)

#define CFG_AUDIO_RESET_CLR(base) \
    REG_SET_VAL(&(base->cfg_mpeg_hot_reset_clr), 0x1<<20)

#define CFG_VIDEOOUT0_HOT_SET(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset), 28)

#define CFG_VIDEOOUT0_HOT_CLR(base) \
    REG_CLR_BIT(&(base->cfg_mpeg_hot_reset), 28)

#define CFG_VIDEOOUT1_HOT_SET(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset2_set), 4)

#define CFG_VIDEOOUT1_HOT_CLR(base) \
    REG_CLR_BIT(&(base->cfg_mpeg_hot_reset2_set), 4)

#define CFG_SVPU_HOT_SET(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset2_set), 8)

#define CFG_JPEG_HOT_SET(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset), 2)

#define CFG_SVPU_HOT_CLR(base) \
    REG_CLR_BIT(&(base->cfg_mpeg_hot_reset2_set), 8)

#define CFG_JPEG_HOT_CLR(base) \
    REG_CLR_BIT(&(base->cfg_mpeg_hot_reset), 2)

#define CFG_VPU_HOT_SET(base)							\
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset), 28)

#define CFG_VPU_HOT_CLR(base)							\
    REG_CLR_BIT(&(base->cfg_mpeg_hot_reset), 28)

#define CFG_VIDEO0_COLD_SET(base) \
    REG_SET_BIT(&(gemini_clock_reg->cfg_mpeg_cold_reset), 22)

#define CFG_VIDEO0_COLD_CLR(base) \
    REG_CLR_BIT(&(gemini_clock_reg->cfg_mpeg_cold_reset), 22)

#define CFG_VIDEO1_COLD_SET(base) \
    REG_SET_BIT(&(gemini_clock_reg->cfg_mpeg_hot_reset2_set), 4)

#define CFG_VIDEO1_COLD_CLR(base) \
    REG_CLR_BIT(&(gemini_clock_reg->cfg_mpeg_hot_reset2_set), 4)

#define CFG_PINC_I2S_AUDIO_SEL(base) \
    REG_CLR_BITS(&(base->cfg_resv5), (3<<26))

#define CFG_PINC_TS3_GPIO_SEL(base) \
    do { \
        REG_CLR_BITS(&(base->cfg_resv5), (0x03<<4)); \
        REG_SET_BITS(&(base->cfg_resv5), (0x00<<4)); \
    } while(0)

#define CFG_PINB_TS3_GPIO_SEL(base) \
    do { \
        REG_CLR_BITS(&(base->cfg_resv6), (0x07<<14)); \
        REG_SET_BITS(&(base->cfg_resv6), (0x01<<14)); \
    } while(0)

#define CFG_PINA_TS3_GPIO_SEL(base) \
    do { \
        REG_CLR_BITS(&(base->cfg_resv7), (0x07<<14)); \
        REG_SET_BITS(&(base->cfg_resv7), (0x02<<14)); \
    } while(0)

#define CFG_PINA_VIDEOOUT_YUV_SEL(base) \
    do { \
        REG_SET_FIELD(&(base->cfg_resv7), (0x7<<5),  4, 5) ; \
        REG_SET_FIELD(&(base->cfg_resv7), (0x7<<8),  2, 8) ; \
        REG_SET_FIELD(&(base->cfg_resv7), (0x7<<11), 1, 11); \
        REG_SET_FIELD(&(base->cfg_resv7), (0x7<<0),  4, 0);  \
        REG_SET_FIELD(&(base->cfg_resv6), (0x7<<26), 4, 26); \
        REG_SET_FIELD(&(base->cfg_resv6), (0x7<<29), 4, 29); \
    } while(0)

#define CFG_PINA_VIDEOOUT_SCART_SEL(base) \
    REG_SET_FIELD(&(base->cfg_resv7), (0x3<<0), 0,0)

#define CFG_PINA_VIDEOOUT_CVBS_SEL(base) \
    do {\
        REG_SET_FIELD(&(base->cfg_resv6), (0x3f<<26), ((4<<26)|(5<<29)), 0);\
    } while (0)

#endif
