#ifndef __GX3113C_CLOCK_REG_H__
#define __GX3113C_CLOCK_REG_H__

#include "gxav_bitops.h"

#define CLOCK_BASE_ADDR (0x0030A000)

#define BIT_DTO_PLL_F  0
#define BIT_DTO_PLL_R  12
#define BIT_DTO_PLL_OD 8

#define DTO_PLL_F_MASK  ( 0x3F << BIT_DTO_PLL_F)
#define DTO_PLL_R_MASK  ( 0x0F << BIT_DTO_PLL_R)
#define DTO_PLL_OD_MASK ( 0x3F << BIT_DTO_PLL_OD)

#define DTO_PLL_F	(REG_GET_FIELD(&(gx3113c_clock_reg->cfg_pll4), DTO_PLL_F_MASK, BIT_DTO_PLL_F))
#define DTO_PLL_R	(REG_GET_FIELD(&(gx3113c_clock_reg->cfg_pll4), DTO_PLL_R_MASK, BIT_DTO_PLL_R))
#define DTO_PLL_OD	(REG_GET_FIELD(&(gx3113c_clock_reg->cfg_pll4), DTO_PLL_OD_MASK, BIT_DTO_PLL_OD))

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
	unsigned int   cfg_dto1;
	unsigned int   cfg_dto2;
	unsigned int   cfg_dto3;                     /* 0x30 */
	unsigned int   cfg_dto4;
	unsigned int   cfg_dto5;
	unsigned int   cfg_dto6;
	unsigned int   cfg_dto7;
	unsigned int   cfg_dto8;
	unsigned int   cfg_dto9;
	unsigned int   cfg_dto10;                    /* 0x4c */
	unsigned int   cfg_resv0[28];
	unsigned int   cfg_pll1;                     /* 0xc0 */
	unsigned int   cfg_pll2;
	unsigned int   cfg_pll3;
	unsigned int   cfg_pll4;
	unsigned int   cfg_pll5;                     /* 0xD0 */
	unsigned int   cfg_pll6;
	unsigned int   cfg_pll7;
	unsigned int   cfg_pll8;
	unsigned int   cfg_resv1[8];
	unsigned int   cfg_usb_config;               /* 0x100 */
	unsigned int   cfg_usb1_config;
	unsigned int   cfg_resv2[6];
	unsigned int   cfg_emi_config;               /* 0x120 */
	unsigned int   cfg_resv3[3];
	unsigned int   cfg_resv4; //pin_interconnect /* 0x130 */
	unsigned int   cfg_resv5; //pin_interconnect
	unsigned int   cfg_resv6; //pin_interconnect
	unsigned int   cfg_resv7[5];
	unsigned int   cfg_dll_config;               /* 0x150 */
	unsigned int   cfg_resv8[3];
	unsigned int   cfg_pdm_sel_1;                /* 0x160 */
	unsigned int   cfg_pdm_sel_2;
	unsigned int   cfg_pdm_sel_3;
	unsigned int   cfg_resv9;
	unsigned int   cfg_source_sel;               /* 0x170 */
	unsigned int   cfg_resv10[3];
	unsigned int   cfg_chip_info;                /* 0x180 */
	unsigned int   cfg_resv11[3];
	unsigned int   cfg_efuse_data;               /* 0x190 */
	unsigned int   cfg_efuse_ctrl;
	unsigned int   cfg_resv12[2];
	unsigned int   cfg_audio_codec_data;         /* 0x1a0 */
	unsigned int   cfg_audio_codec_ctrl;
	unsigned int   cfg_resv13[2];
	unsigned int   cfg_eth_config;               /* 0x1b0 */
	unsigned int   cfg_resv14[3];
	unsigned int   cfg_mpeg_cold_rst_2_1set;     /* 0x1c0 */
	unsigned int   cfg_resv15[3];
	unsigned int   cfg_mpeg_hot_rst_2_1set;      /* 0x1d0 */
	unsigned int   cfg_resv16[3];
	unsigned int   cfg_hdmi_base;                /* 0x1e0 */
	unsigned int   cfg_hdmi_sel;                 /* 0x1e4 */
};

/* rCFG_DTO1 and rCFG_DTO2 register bit definition */
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

#define bCFG_CLOCK_SOURCE_DEMUX_SYS          (6)
#define bCFG_CLOCK_SOURCE_DEMUX_OUT          (7)
#define bCFG_CLOCK_SOURCE_DEMUX_STC          (8)
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
        REG_SET_VAL(&(base->cfg_dto1),0);               \
        REG_SET_VAL(&(base->cfg_dto1), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff)));                 \
        REG_SET_VAL(&(base->cfg_dto1), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto1), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff)));                 \
    } while(0)

#define CFG_AUDIO_LODEC_SET_DTO(base,val)                      \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto1), ((1<<31) | (val & 0x3fffffff)));                 \
        REG_SET_VAL(&(base->cfg_dto1), ((1<<31) | (1<<30) | (val & 0x3fffffff)));       \
    } while(0)

#define CFG_AUDIO_I2S_SET_DTO(base,val)                     \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto1),0);                   \
        REG_SET_VAL(&(base->cfg_dto1), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | ((val) & 0x3fffffff))); \
    } while(0)

#define CFG_AUDIO_SPDIF_SET_DTO(base,val)                   \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto2),0);                   \
        REG_SET_VAL(&(base->cfg_dto2 ), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto2 ), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto2 ), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff))); \
    } while(0)

#define CFG_DE_INTERLACE_SET_DTO(base,val)                  \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto4),0);                   \
        REG_SET_VAL(&(base->cfg_dto4 ), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | (FREQ_TO_DTO_STEP(val) & 0x3fffffff))); \
    } while(0)

#define CFG_AUDIO_OR_SET_DTO(base,val)                   \
    do {                                                    \
        REG_SET_VAL(&(base->cfg_dto6),0);                   \
        REG_SET_VAL(&(base->cfg_dto6 ), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto6 ), ( vCFG_DTO_WSEL | vCFG_DTO_LOAD | ((val) & 0x3fffffff))); \
        REG_SET_VAL(&(base->cfg_dto6 ), ( vCFG_DTO_WSEL | ((val) & 0x3fffffff))); \
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
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset_set), 20)

#define CFG_AUDIO_RESET_CLR(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset_clr), 20)

#define CFG_VIDEOOUT0_HOT_SET(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset), 22)

#define CFG_VIDEOOUT0_HOT_CLR(base) \
    REG_CLR_BIT(&(base->cfg_mpeg_hot_reset), 22)

#define CFG_VIDEOOUT1_HOT_SET(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_rst_2_1set), 4)

#define CFG_VIDEOOUT1_HOT_CLR(base) \
    REG_CLR_BIT(&(base->cfg_mpeg_hot_rst_2_1set), 4)

#define CFG_SVPU_HOT_SET(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_rst_2_1set), 8)

#define CFG_JPEG_HOT_SET(base) \
    REG_SET_BIT(&(base->cfg_mpeg_hot_rst_2_1set), 2)

#define CFG_SVPU_HOT_CLR(base) \
    REG_CLR_BIT(&(base->cfg_mpeg_hot_rst_2_1set), 8)

#define CFG_JPEG_HOT_CLR(base) \
    REG_CLR_BIT(&(base->cfg_mpeg_hot_rst_2_1set), 2)

#define CFG_VPU_HOT_SET(base)							\
    REG_SET_BIT(&(base->cfg_mpeg_hot_reset), 28)

#define CFG_VPU_HOT_CLR(base)							\
    REG_CLR_BIT(&(base->cfg_mpeg_hot_reset), 28)

#define CFG_VIDEO0_COLD_SET(base) \
    REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset), 22)

#define CFG_VIDEO0_COLD_CLR(base) \
    REG_CLR_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_reset), 22)

#define CFG_VIDEO1_COLD_SET(base) \
    REG_SET_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_rst_2_1set), 4)

#define CFG_VIDEO1_COLD_CLR(base) \
    REG_CLR_BIT(&(gx3113c_clock_reg->cfg_mpeg_cold_rst_2_1set), 4)

#define CFG_PINC_I2S_AUDIO_SEL(base) \
    REG_CLR_BITS(&(base->cfg_resv4), (3<<26))

#define CFG_PINC_TS3_GPIO_SEL(base) \
    do { \
        REG_CLR_BITS(&(base->cfg_resv4), (0x03<<4)); \
        REG_SET_BITS(&(base->cfg_resv4), (0x00<<4)); \
    } while(0)

#define CFG_PINB_TS3_GPIO_SEL(base) \
    do { \
        REG_CLR_BITS(&(base->cfg_resv5), (0x07<<14)); \
        REG_SET_BITS(&(base->cfg_resv5), (0x01<<14)); \
    } while(0)

#define CFG_PINA_TS3_GPIO_SEL(base) \
    do { \
        REG_CLR_BITS(&(base->cfg_resv6), (0x07<<14)); \
        REG_SET_BITS(&(base->cfg_resv6), (0x02<<14)); \
    } while(0)

#define CFG_PINA_VIDEOOUT_YUV_SEL(base) \
    do { \
        REG_SET_FIELD(&(base->cfg_resv6), (0x7<<5),  4, 5) ; \
        REG_SET_FIELD(&(base->cfg_resv6), (0x7<<8),  2, 8) ; \
        REG_SET_FIELD(&(base->cfg_resv6), (0x7<<11), 1, 11); \
        REG_SET_FIELD(&(base->cfg_resv6), (0x7<<0),  4, 0);  \
        REG_SET_FIELD(&(base->cfg_resv5), (0x7<<26), 4, 26); \
        REG_SET_FIELD(&(base->cfg_resv5), (0x7<<29), 4, 29); \
    } while(0)

#define CFG_PINA_VIDEOOUT_SCART_SEL(base) \
    REG_SET_FIELD(&(base->cfg_resv6), (0x3<<0), 0,0)

#define CFG_PINA_VIDEOOUT_CVBS_SEL(base) \
    do {\
        REG_SET_FIELD(&(base->cfg_resv5), (0x3f<<26), ((4<<26)|(5<<29)), 0);\
    } while (0)

#endif
