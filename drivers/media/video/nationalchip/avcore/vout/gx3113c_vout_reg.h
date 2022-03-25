#ifndef __GX3113C_VIDEO_OUT_REG_H__
#define __GX3113C_VIDEO_OUT_REG_H__

#include "gxav_bitops.h"

#define TV_VOUT0_BASE_ADDR           (0x04804000)

struct gx3113c_videoout_reg
{
	unsigned int   tv_ctrl        ;//00
	unsigned int   tv_dac         ;
	unsigned int   tv_hsync       ;
	unsigned int   tv_active      ;
	unsigned int   tv_fir_y0_2    ;//10
	unsigned int   tv_fir_y3_5    ;
	unsigned int   tv_fir_y6_8    ;
	unsigned int   tv_fir_y9      ;
	unsigned int   tv_fir_c0_2    ;//20
	unsigned int   tv_fir_c3_5    ;
	unsigned int   yuv_tran_y1    ;       //colr_tran_m0   ;
	unsigned int   yuv_tran_y2    ;       //colr_tran_m1   ;
	unsigned int   tv_range       ;//30
	unsigned int   tv_adjust      ;
	unsigned int   tv_test        ;
	unsigned int   yuv_tran_u1    ;       //colr_tran_m2   ;
	unsigned int   yuv_tran_u2    ;//40   //colr_tran_m3   ;
	unsigned int   yuv_tran_v1    ;       //colr_tran_m4   ;
	unsigned int   yuv_tran_v2    ;       //colr_tran_m5   ;
	unsigned int   colr_tran_c    ;
	unsigned int   pposd_delay    ;//50
	unsigned int   sog_enable     ;
	unsigned int   vsync_width    ;
	unsigned int   vbi_cc_ctrl    ;
	unsigned int   vbi_cc_data    ;//60
	unsigned int   vbi_wss_ctrl   ;
	unsigned int   vbi_vps_ctrl   ;
	unsigned int   vbi_cgms_ctrl  ;
	unsigned int   vbi_wss_data   ;//70
	unsigned int   vbi_cgms_data  ;
	unsigned int   vbi_tlx_ctrl1  ;
	unsigned int   vbi_tlx_ctrl2  ;
	unsigned int   scart_ctrl     ;//80
	unsigned int   dgdp_enable    ;
	unsigned int   dgdp_ctrl_reg1 ;
	unsigned int   dgdp_ctrl_reg2 ;
	unsigned int   digital_ctl    ;//90
	unsigned int   dgdp_ctrl_reg4 ;
	unsigned int   dac_test_x     ;       //3201newly added from this reg.
	unsigned int   lcd_line_ctrl  ;
	unsigned int   pixel_number   ;
	unsigned int   lcd_vsync_width;
	unsigned int   lcd_ctrl       ;
	unsigned int   lcd_pwm_ctl    ;
	unsigned int   lcd_pwmt_ctl   ;
	unsigned int   lcd_topline_ctl ;
	unsigned int   lcd_botline_ctl ;
	unsigned int   lcd_botfield_ctl ;
	unsigned int   lcd_botactive_ctl ;
	unsigned int   dgdp0_enable    ;
	unsigned int   dgdp0_ctrl_reg1 ;
	unsigned int   dgdp0_ctrl_reg2 ;
	unsigned int   dgdp0_ctrl_reg3 ;
	unsigned int   dgdp0_ctrl_reg4 ;
	unsigned int   hdmi_tran_ycbcr;       //colr_tran_m0   ;
};

/* Definition of register rTV_CTRL's bit, mask, value */
#define bDAC_FULLCASE_CHANGE            (25)
#define bTV_CTRL_DAC_OUT_MODE           (17)
#define bDAC_CLK_MODE                   (5)
#define bTV_CLOSE_EN                    (4)
#define bTV_CTRL_TV_FORMAT              (0)

#define mDAC_FULLCASE_CHANGE            (0x1f   << bDAC_FULLCASE_CHANGE     )
#define mTV_CTRL_DAC_OUT_MODE           (0x07   << bTV_CTRL_DAC_OUT_MODE    )
#define mDAC_CLK_MODE                   (0x03   << bDAC_CLK_MODE            )
#define mTV_CTRL_TV_FORMAT              (0x0F   << bTV_CTRL_TV_FORMAT       )

/* Definition of register rTV_HSYNC's bit, mask, value */
#define bTV_HSYNC_BURST_ON              (16)
#define bTV_HSYNC_BURST_OFF             (8)
#define bTV_HSYNC_WIDTH                 (0)

#define mTV_HSYNC_BURST_ON              (0xff   << bTV_HSYNC_BURST_ON       )
#define mTV_HSYNC_BURST_OFF             (0xff   << bTV_HSYNC_BURST_OFF      )
#define mTV_HSYNC_WIDTH                 (0xff   << bTV_HSYNC_WIDTH          )

/* Definition of register rTV_ACTIVE's bit, mask, value */
#define bTV_ACTIVE_ON                   (12)
#define bTV_ACTIVE_OFF                  (0)

#define mTV_ACTIVE_ON                   (0xfff  << bTV_ACTIVE_ON            )
#define mTV_ACTIVE_OFF                  (0xfff  << bTV_ACTIVE_OFF           )

/* Definition of register rTV_FIR_Y0_2's bit, mask, value */
#define bTV_FIR_Y0                      (20)
#define bTV_FIR_Y1                      (10)
#define bTV_FIR_Y2                      (0)

#define mTV_FIR_Y0                      (0x3ff  << bTV_FIR_Y0               )
#define mTV_FIR_Y1                      (0x3ff  << bTV_FIR_Y1               )
#define mTV_FIR_Y2                      (0x3ff  << bTV_FIR_Y2               )

/* Definition of register rTV_FIR_Y3_5's bit, mask, value */
#define bTV_FIR_Y3                      (20)
#define bTV_FIR_Y4                      (10)
#define bTV_FIR_Y5                      (0)

#define mTV_FIR_Y3                      (0x3ff  << bTV_FIR_Y3               )
#define mTV_FIR_Y4                      (0x3ff  << bTV_FIR_Y4               )
#define mTV_FIR_Y5                      (0x3ff  << bTV_FIR_Y5               )

/* Definition of register rTV_FIR_Y6_8's bit, mask, value */
#define bTV_FIR_Y6                      (20)
#define bTV_FIR_Y7                      (10)
#define bTV_FIR_Y8                      (0)

#define mTV_FIR_Y6                      (0x3ff  << bTV_FIR_Y6               )
#define mTV_FIR_Y7                      (0x3ff  << bTV_FIR_Y7               )
#define mTV_FIR_Y8                      (0x3ff  << bTV_FIR_Y8               )

/* Definition of register rTV_FIR_Y9's bit, mask, value */
#define bTV_FIR_Y9                      (20)
#define bTV_FIR_Y9_C6                   (10)
#define bTV_FIR_Y9_C7                   (0)

#define mTV_FIR_Y9                      (0x3ff  << bTV_FIR_Y9               )
#define mTV_FIR_Y9_C6                   (0x3ff  << bTV_FIR_Y9_C6            )
#define mTV_FIR_Y9_C7                   (0x3ff  << bTV_FIR_Y9_C7            )

/* Definition of register rTV_FIR_C0_2's bit, mask, value */
#define bTV_FIR_C0                      (20)
#define bTV_FIR_C1                      (10)
#define bTV_FIR_C2                      (0)

#define mTV_FIR_C0                      (0x3ff  << bTV_FIR_C0               )
#define mTV_FIR_C1                      (0x3ff  << bTV_FIR_C1               )
#define mTV_FIR_C2                      (0x3ff  << bTV_FIR_C2               )

/* Definition of register rTV_FIR_C3_5's bit, mask, value */
#define bTV_FIR_C3                      (20)
#define bTV_FIR_C4                      (10)
#define bTV_FIR_C5                      (0)

#define mTV_FIR_C3                      (0x3ff  << bTV_FIR_C3               )
#define mTV_FIR_C4                      (0x3ff  << bTV_FIR_C4               )
#define mTV_FIR_C5                      (0x3ff  << bTV_FIR_C5               )

/* Definition of register rYUV_TRAN_Y's bit, mask, value */
#define bYUV_TRAN_Y_SCALE0              (20)
#define bYUV_TRAN_Y_SCALE1              (0)
#define bYUV_TRAN_Y_SCALE2              (0)

#define mYUV_TRAN_Y_SCALE0              (0x3ff  << bYUV_TRAN_Y_SCALE0       )
#define mYUV_TRAN_Y_SCALE1              (0x3ff  << bYUV_TRAN_Y_SCALE1       )
#define mYUV_TRAN_Y_SCALE2              (0x3ff  << bYUV_TRAN_Y_SCALE2       )

/* Definition of register rTV_ADJUST's bit, mask, value */
#define bTV_ADJUST_Y_SHIFT              (8)
#define bTV_ADJUST_SUBC_PHASE           (0)

#define mTV_ADJUST_Y_SHIFT              (0xff   << bTV_ADJUST_Y_SHIFT       )
#define mTV_ADJUST_SUBC_PHASE           (0xff   << bTV_ADJUST_SUBC_PHASE    )

/* Definition of register rYUV_TRAN_U's bit, mask, value */
#define bYUV_TRAN_U_SCALE0              (0)
#define bYUV_TRAN_U_SCALE1              (10)
#define bYUV_TRAN_U_SCALE2              (0)

#define mYUV_TRAN_U_SCALE0              (0x3ff  << bYUV_TRAN_U_SCALE0       )
#define mYUV_TRAN_U_SCALE1              (0x3ff  << bYUV_TRAN_U_SCALE1       )
#define mYUV_TRAN_U_SCALE2              (0x3ff  << bYUV_TRAN_U_SCALE2       )

/* Definition of register rYUV_TRAN_V's bit, mask, value */
#define bYUV_TRAN_V_SCALE0              (0)
#define bYUV_TRAN_V_SCALE1              (0)
#define bYUV_TRAN_V_SCALE2              (0)

#define mYUV_TRAN_V_SCALE0              (0x3ff  << bYUV_TRAN_V_SCALE0       )
#define mYUV_TRAN_V_SCALE1              (0x3ff  << bYUV_TRAN_V_SCALE1       )
#define mYUV_TRAN_V_SCALE2              (0x3ff  << bYUV_TRAN_V_SCALE2       )

/* Definition of register rPPOSD_DELAY's bit, mask, value */
#define bTV_INT_DELAY                   (8)
#define bTV_PPOSD_DELAY                 (0)

#define mTV_INT_DELAY                   (0xff   << bTV_INT_DELAY            )
#define mTV_PPOSD_DELAY                 (0xff   << bTV_PPOSD_DELAY          )

/* Definition of register rVSYNC_WIDTH's bit, mask, value */
#define bTV_VGA_VSYNC                   (20)
#define bTV_EQU_WIDTH                   (10)
#define bTV_SERR_WIDTH                  (0)

#define mTV_VGA_VSYNC                   (0xf    << bTV_VGA_VSYNC            )
#define mTV_EQU_WIDTH                   (0x3ff  << bTV_EQU_WIDTH            )
#define mTV_SERR_WIDTH                  (0x3ff  << bTV_SERR_WIDTH           )

/* Definition of register rSCART_CTRL's bit, mask, value */
#define bBT656_EN                       (3)

//3201
/* Definition of register rDAC_TEST_X's bit, mask, value */
#define bACTIVELINE_NUMBER              (21)
#define bHALFLINE_SAMPLES               (10)
#define bDAC_TEST_X                     (0)

#define mACTIVELINE_NUMBER              (0x7ff  << bACTIVELINE_NUMBER       )
#define mHALFLINE_SAMPLES               (0x7ff  << bHALFLINE_SAMPLES        )
#define mDAC_TEST_X                     (0x3ff  << bDAC_TEST_X              )

/* Definition of register rLCD_LINE_CTRL's bit, mask, value */
#define bLINE_ON                        (11)
#define bLINE_OFF                       (0)

#define mLINE_ON                        (0x7ff  << bLINE_ON                 )
#define mLINE_OFF                       (0x7ff  << bLINE_OFF                )

/* Definition of register rPIXEL_NUMBER's bit, mask, value */
#define bHDTV_SYNC_MID_WIDTH            (20)
#define bHDTV_SYNC_WIDTH                (12)
#define bPIXEL_NUMBER                   (0)

#define mHDTV_SYNC_MID_WIDTH            (0xff   << bHDTV_SYNC_MID_WIDTH     )
#define mHDTV_SYNC_WIDTH                (0xff   << bHDTV_SYNC_WIDTH         )
#define mPIXEL_NUMBER                   (0xfff  << bPIXEL_NUMBER            )

/* Definition of register rLCD_VSYNC_WIDTH's bit, mask, value */
#define bVSYNC_WIDTH                    (11)
#define bLINE_NUMBER                    (0)

#define mVSYNC_WIDTH                    (0x7ff  << bVSYNC_WIDTH             )
#define mLINE_NUMBER                    (0x7ff  << bLINE_NUMBER             )

/* Definition of register rLCD_CTRL's bit, mask, value */
#define bRGB_MODE                       (28)
#define bLCD_ODDEVEN_FLAG               (27)
#define bLCD_FULLCASE_CHANGE            (24)
#define bHSYNC_DELAY                    (16)
#define bVSYNC_DELAY                    (8)
#define bCOLORBAR_EN                    (7)
#define bLCD_SYNC_LOW                   (6)
#define bSYNC_EMBEDED                   (5)
#define bMSB_TRAN_LSB                   (4)
#define bLCD_OUT_MODE                   (1)
#define bLCD_CLOSE                      (0)

#define mRGB_MODE                       (0x03   << bRGB_MODE                )
#define mLCD_ODDEVEN_FLAG               (0x01   << bLCD_ODDEVEN_FLAG)
#define mLCD_FULLCASE_CHANGE            (0x07   << bLCD_FULLCASE_CHANGE     )
#define mHSYNC_DELAY                    (0xff   << bHSYNC_DELAY             )
#define mVSYNC_DELAY                    (0xff   << bVSYNC_DELAY             )
#define mLCD_OUT_MODE                   (0x07   << bLCD_OUT_MODE            )

/* Definition of register rLCD_PWM_CTL's bit, mask, value */
#define bLCD_PWM_REG                    (0)
#define mLCD_PWM_REG                    (0xffff << bLCD_PWM_REG             )

/* Definition of register rLCD_PWMT_CTL's bit, mask, value */
#define bLCD_PWMT_REG                   (0)
#define mLCD_PWMT_REG                   (0xffff << bLCD_PWMT_REG            )

#define bHDTV_OUT_MODE                   (23)
#define mHDTV_OUT_MODE                   (0x3 << bHDTV_OUT_MODE            )

/* Exported Macros --------------------------------------------------------- */
//tv_ctrl
#define DVE_SET_DAC_FULLCASE_CHANGE(rp,value)                           \
	REG_SET_FIELD(							\
			&(rp->tv_ctrl),					\
			mDAC_FULLCASE_CHANGE,				\
			value,						\
			bDAC_FULLCASE_CHANGE)

#define DVE_SET_DAC_OUT_MODE(rp,value)                                  \
	REG_SET_FIELD(							\
			&(rp->tv_ctrl),					\
			mTV_CTRL_DAC_OUT_MODE,				\
			value,						\
			bTV_CTRL_DAC_OUT_MODE)

#define DVE_SET_DAC_CLK_MODE(rp,value)                                  \
	REG_SET_FIELD(							\
			&(rp->tv_ctrl),					\
			mDAC_CLK_MODE,					\
			value,						\
			bDAC_CLK_MODE)

#define DVE_SET_TV_OUTPUT_ENABLE(rp)                                    \
	REG_CLR_BIT(                                                    \
			&(rp->tv_ctrl),                                 \
			bTV_CLOSE_EN)

#define DVE_SET_TV_OUTPUT_DISABLE(rp)                                   \
	REG_SET_BIT(                                                    \
			&(rp->tv_ctrl),                                 \
			bTV_CLOSE_EN)

#define DVE_SET_TV_FORMAT(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_ctrl),                                 \
			mTV_CTRL_TV_FORMAT,                             \
			value,						\
			bTV_CTRL_TV_FORMAT)

//tv_hsync
#define DVE_SET_HSYNC_BURST_ON(rp,value)                                \
	REG_SET_FIELD(                                                  \
			&(rp->tv_hsync),                                \
			mTV_HSYNC_BURST_ON,                             \
			value,						\
			bTV_HSYNC_BURST_ON)

#define DVE_SET_HSYNC_BURST_OFF(rp,value)                               \
	REG_SET_FIELD(                                                  \
			&(rp->tv_hsync),                                \
			mTV_HSYNC_BURST_OFF,                            \
			value,						\
			bTV_HSYNC_BURST_OFF)

#define DVE_SET_HSYNC_WIDTH(rp,value)                                   \
	REG_SET_FIELD(                                                  \
			&(rp->tv_hsync),                                \
			mTV_HSYNC_WIDTH,                                \
			value,						\
			bTV_HSYNC_WIDTH)

//tv_active
#define DVE_SET_ACTIVE_ON(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_active),                               \
			mTV_ACTIVE_ON,                                  \
			value,						\
			bTV_ACTIVE_ON)

#define DVE_SET_ACTIVE_OFF(rp,value)                                    \
	REG_SET_FIELD(                                                  \
			&(rp->tv_active),                               \
			mTV_ACTIVE_OFF,                                 \
			value,						\
			bTV_ACTIVE_OFF)

//tv_fir_y0_2
#define DVE_SET_FIR_Y0(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y0_2),                             \
			mTV_FIR_Y0,                                     \
			value,						\
			bTV_FIR_Y0)

#define DVE_SET_FIR_Y1(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y0_2),                             \
			mTV_FIR_Y1,                                     \
			value,						\
			bTV_FIR_Y1)

#define DVE_SET_FIR_Y2(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y0_2),                             \
			mTV_FIR_Y2,                                     \
			value,						\
			bTV_FIR_Y2)

//tv_fir_y3_5
#define DVE_SET_FIR_Y3(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y3_5),                             \
			mTV_FIR_Y3,                                     \
			value,						\
			bTV_FIR_Y3)

#define DVE_SET_FIR_Y4(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y3_5),                             \
			mTV_FIR_Y4,                                     \
			value,						\
			bTV_FIR_Y4)

#define DVE_SET_FIR_Y5(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y3_5),                             \
			mTV_FIR_Y5,                                     \
			value,						\
			bTV_FIR_Y5)

//tv_fir_y6_8
#define DVE_SET_FIR_Y6(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y6_8),                             \
			mTV_FIR_Y6,                                     \
			value,						\
			bTV_FIR_Y6)

#define DVE_SET_FIR_Y7(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y6_8),                             \
			mTV_FIR_Y7,                                     \
			value,						\
			bTV_FIR_Y7)

#define DVE_SET_FIR_Y8(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y6_8),                             \
			mTV_FIR_Y8,                                     \
			value,						\
			bTV_FIR_Y8)

//tv_fir_y9
#define DVE_SET_FIR_Y9(rp,value)                                        \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y9),                               \
			mTV_FIR_Y9,                                     \
			value,						\
			bTV_FIR_Y9)

#define DVE_SET_FIR_Y9_C6(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y9),                               \
			mTV_FIR_Y9_C6,                                  \
			value,						\
			bTV_FIR_Y9_C6)

#define DVE_SET_FIR_Y9_C7(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_y9),                               \
			mTV_FIR_Y9_C7,                                  \
			value,						\
			bTV_FIR_Y9_C7)

//tv_fir_c0_2
#define DVE_SET_TV_FIR_C0(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_c0_2),                             \
			mTV_FIR_C0,                                     \
			value,						\
			bTV_FIR_C0)

#define DVE_SET_TV_FIR_C1(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_c0_2),                             \
			mTV_FIR_C1,                                     \
			value,						\
			bTV_FIR_C1)

#define DVE_SET_TV_FIR_C2(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_c0_2),                             \
			mTV_FIR_C2,                                     \
			value,						\
			bTV_FIR_C2)

//tv_fir_c3_5
#define DVE_SET_TV_FIR_C3(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_c3_5),                             \
			mTV_FIR_C3,                                     \
			value,						\
			bTV_FIR_C3)

#define DVE_SET_TV_FIR_C4(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_c3_5),                             \
			mTV_FIR_C4,                                     \
			value,						\
			bTV_FIR_C4)

#define DVE_SET_TV_FIR_C5(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->tv_fir_c3_5),                             \
			mTV_FIR_C5,                                     \
			value,						\
			bTV_FIR_C5)

//COLR_TRAN_M0
#define DVE_SET_YUV_TRAN_Y_SCALE0(rp,value)                             \
	REG_SET_FIELD(                                                  \
			&(rp->yuv_tran_y1),                             \
			mYUV_TRAN_Y_SCALE0,                             \
			value,						\
			bYUV_TRAN_Y_SCALE0)

//tv_adjust
#define DVE_SET_ADJUST_Y_SHIFT(rp,value)                                \
	REG_SET_FIELD(                                                  \
			&(rp->tv_adjust),                               \
			mTV_ADJUST_Y_SHIFT,                             \
			value,																\
			bTV_ADJUST_Y_SHIFT)

#define DVE_SET_ADJUST_SUBC_PHASE(rp,value)                             \
	REG_SET_FIELD(                                                  \
			&(rp->tv_adjust),                               \
			mTV_ADJUST_SUBC_PHASE,                          \
			value,						\
			bTV_ADJUST_SUBC_PHASE)

//COLR_TRAN_M1(U)

#define DVE_SET_YUV_TRAN_U_SCALE1(rp,value)                             \
	REG_SET_FIELD(                                                  \
			&(rp->yuv_tran_y1),                             \
			mYUV_TRAN_U_SCALE1,                             \
			value,						\
			bYUV_TRAN_U_SCALE1)

//COLR_TRAN_M2(V)

#define DVE_SET_YUV_TRAN_V_SCALE2(rp,value)                             \
	REG_SET_FIELD(                                                  \
			&(rp->yuv_tran_y1),                             \
			mYUV_TRAN_V_SCALE2,                             \
			value,						\
			bYUV_TRAN_V_SCALE2)

//pposd_delay
#define DVE_SET_TV_INT_DELAY(rp,value)                                  \
	REG_SET_FIELD(                                                  \
			&(rp->pposd_delay),                             \
			mTV_INT_DELAY,                                  \
			value,						\
			bTV_INT_DELAY)

#define DVE_SET_PPOSD_DELAY(rp,value)                                   \
	REG_SET_FIELD(                                                  \
			&(rp->pposd_delay),                             \
			mTV_PPOSD_DELAY,                                \
			value,						\
			bTV_PPOSD_DELAY)

//vsync_width
#define DVE_SET_VGA_VSYNC(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->vsync_width),                             \
			mTV_VGA_VSYNC,                                  \
			value,						\
			bTV_VGA_VSYNC)

#define DVE_SET_EQU_WIDTH(rp,value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->vsync_width),                             \
			mTV_EQU_WIDTH,                                  \
			value,						\
			bTV_EQU_WIDTH)

#define DVE_SET_SERR_WIDTH(rp,value)                                    \
	REG_SET_FIELD(                                                  \
			&(rp->vsync_width),                             \
			mTV_SERR_WIDTH,                                 \
			value,						\
			bTV_SERR_WIDTH)

//scart_ctrl
#define DVE_BT656_OUTPUT_ENABLE(rp)                                     \
	REG_SET_BIT(                                                    \
			&(rp->scart_ctrl),                              \
			bBT656_EN)

#define DVE_BT656_OUTPUT_DISABLE(rp)                                    \
	REG_CLR_BIT(                                                    \
			&(rp->scart_ctrl),                              \
			bBT656_EN)

//3201����
//dac_test_x
#define DVE_SET_ACTIVELINE_NUMBER(rp, value)                            \
	REG_SET_FIELD(                                                  \
			&(rp->dac_test_x),                              \
			mACTIVELINE_NUMBER,                             \
			value,						\
			bACTIVELINE_NUMBER)

#define DVE_SET_HALFLINE_SAMPLES(rp, value)                             \
	REG_SET_FIELD(                                                  \
			&(rp->dac_test_x),                              \
			mHALFLINE_SAMPLES,                              \
			value,						\
			bHALFLINE_SAMPLES)

//lcd_line_ctrl
#define DVE_SET_LINE_ON(rp, value)                                      \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_line_ctrl),                           \
			mLINE_ON,                                       \
			value,						\
			bLINE_ON)

#define DVE_SET_LINE_OFF(rp, value)                                     \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_line_ctrl),                           \
			mLINE_OFF,                                      \
			value,						\
			bLINE_OFF)

#define DVE_SET_HDTV_SYNC_WIDTH(rp, value)                              \
	REG_SET_FIELD(                                                  \
			&(rp->pixel_number),                            \
			mHDTV_SYNC_WIDTH,                               \
			value,						\
			bHDTV_SYNC_WIDTH)

#define DVE_SET_PIXEL_NUMBER(rp, value)                                 \
	REG_SET_FIELD(                                                  \
			&(rp->pixel_number),                            \
			mPIXEL_NUMBER,                                  \
			value,						\
			bPIXEL_NUMBER)

//rLCD_VSYNC_WIDTH
#define DVE_SET_VSYNC_WIDTH(rp, value)                                  \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_vsync_width),                         \
			mVSYNC_WIDTH,                                   \
			value,						\
			bVSYNC_WIDTH)

#define DVE_SET_LINE_NUMBER(rp, value)                                  \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_vsync_width),                         \
			mLINE_NUMBER,                                   \
			value,						\
			bLINE_NUMBER)

#define DVE_LCD_SET_ODDEVEN_FLAG(rp, value)                             \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_ctrl),                                \
			mLCD_ODDEVEN_FLAG,                              \
			value,						\
			bLCD_ODDEVEN_FLAG)

#define DVE_SET_LCD_FULLCASE_CHANGE(rp, value)                          \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_ctrl),                                \
			mLCD_FULLCASE_CHANGE,                           \
			value,						\
			bLCD_FULLCASE_CHANGE)

#define DVE_SET_HSYNC_DELAY(rp, value)                                  \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_ctrl),                                \
			mHSYNC_DELAY,                                   \
			value,						\
			bHSYNC_DELAY)

#define DVE_SET_VSYNC_DELAY(rp, value)                                  \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_ctrl),                                \
			mVSYNC_DELAY,                                   \
			value,						\
			bVSYNC_DELAY)

#define DVE_SET_LCD_OUT_MODE(rp, value)                                 \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_ctrl),                                \
			mLCD_OUT_MODE,                                  \
			value,						\
			bLCD_OUT_MODE)

#define DVE_SET_LCD_ON(rp)                                              \
	REG_CLR_BIT(                                                    \
			&(rp->lcd_ctrl),                                \
			bLCD_CLOSE)

#define DVE_SET_LCD_CLOSE(rp)                                           \
	REG_SET_BIT(                                                    \
			&(rp->lcd_ctrl),                                \
			bLCD_CLOSE)

//lcd_pwm_ctl
#define DVE_SET_LCD_PWM_REG(rp, value)                                  \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_pwm_ctl),                             \
			mLCD_PWM_REG,                                   \
			value,						\
			bLCD_PWM_REG)

//lcd_pwmt_ctl
#define DVE_SET_LCD_PWMT_REG(rp, value)                                 \
	REG_SET_FIELD(                                                  \
			&(rp->lcd_pwmt_ctl),                            \
			mLCD_PWMT_REG,                                  \
			value,						\
			bLCD_PWMT_REG)

#define DVE_SET_HDTV_OUT_MODE(rp, value)                                \
	REG_SET_FIELD(                                                  \
			&(rp->sog_enable),                              \
			mHDTV_OUT_MODE,                                 \
			value,                                          \
			bHDTV_OUT_MODE)

#define bHDMI_TRAN_Y_SCALE      	        (20)
#define bHDMI_TRAN_CB_SCALE             	(10)
#define bHDMI_TRAN_CR_SCALE             	(0)

#define mHDMI_TRAN_Y_SCALE  	            	(0x3ff  << bHDMI_TRAN_Y_SCALE        )
#define mHDMI_TRAN_CB_SCALE              	(0x3ff  << bHDMI_TRAN_CB_SCALE       )
#define mHDMI_TRAN_CR_SCALE              	(0x3ff  << bHDMI_TRAN_CR_SCALE       )

#define DVE_SET_HDMI_TRAN_Y_SCALE(rp,value)   \
	REG_SET_FIELD(                        \
			&(rp->hdmi_tran_ycbcr),   \
			mHDMI_TRAN_Y_SCALE,   \
			value,                \
			bHDMI_TRAN_Y_SCALE)
#define DVE_SET_HDMI_TRAN_CB_SCALE(rp,value)   \
	REG_SET_FIELD(                        \
			&(rp->hdmi_tran_ycbcr),   \
			mHDMI_TRAN_CB_SCALE,   \
			value,                \
			bHDMI_TRAN_CB_SCALE)
#define DVE_SET_HDMI_TRAN_CR_SCALE(rp,value)   \
	REG_SET_FIELD(                        \
			&(rp->hdmi_tran_ycbcr),   \
			mHDMI_TRAN_CR_SCALE,   \
			value,                \
			bHDMI_TRAN_CR_SCALE)

#define bCC_TOP_EN							(20)
#define bCC_BOTTOM_EN						(21)
#define DVE_SET_CC_TOP_EN(reg)\
	REG_SET_BIT(&(reg->vbi_cc_ctrl), bCC_TOP_EN)
#define DVE_SET_CC_BOTTOM_EN(reg)\
	REG_SET_BIT(&(reg->vbi_cc_ctrl), bCC_BOTTOM_EN)

#define bCC_LINE_TOP						(0)
#define mCC_LINE_TOP						(0x3ff<<bCC_LINE_TOP)
#define bCC_LINE_BOTTOM						(10)
#define mCC_LINE_BOTTOM						(0x3ff<<bCC_LINE_BOTTOM)
#define DVE_SET_CC_LINE_TOP(reg, value)\
	REG_SET_FIELD(\
			&(reg->vbi_cc_ctrl),\
			mCC_LINE_TOP,\
			value,\
			bCC_LINE_TOP)
#define DVE_SET_CC_LINE_BOTTOM(reg, value)\
	REG_SET_FIELD(\
			&(reg->vbi_cc_ctrl),\
			mCC_LINE_BOTTOM,\
			value,\
			bCC_LINE_BOTTOM)

#define bCC_DATA_TOP						(0)
#define mCC_DATA_TOP						(0xffff<<bCC_DATA_TOP)
#define bCC_DATA_BOTTOM						(1)
#define mCC_DATA_BOTTOM						(0xffff<<bCC_DATA_BOTTOM)
#define DVE_SET_CC_DATA_TOP(reg, value)\
	REG_SET_FIELD(\
			&(reg->vbi_cc_data),\
			mCC_DATA_TOP,\
			value,\
			bCC_DATA_TOP)
#define DVE_SET_CC_DATA_BOTTOM(reg, value)\
	REG_SET_FIELD(\
			&(reg->vbi_cc_data),\
			mCC_DATA_BOTTOM,\
			value,\
			bCC_DATA_BOTTOM)

#endif



