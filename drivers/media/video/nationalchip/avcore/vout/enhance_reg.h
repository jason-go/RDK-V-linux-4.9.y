#ifndef    __VPU_ENHANCE_H__
#define    __VPU_ENHANCE_H__

struct cygnus_vpu_enhance_reg {
	unsigned rPRM_LTI_CFG_0; //0x0
	unsigned rPRM_LTI_CFG_1; //0x4
	unsigned rPRM_PEAKING_CFG_0; //0x8
	unsigned rPRM_PEAKING_CFG_1; //0xc
	unsigned rPRM_CTI_CFG_0; //0x10
	unsigned rPRM_CTI_CFG_1; //0x14
	unsigned rPRM_BLEC_BRT_CFG; //0x18
	unsigned rPRM_HUE_SAT_CON_CFG; //0x1c
	unsigned rPRM_HEC_CFG_0; //0x20
	unsigned rPRM_HEC_CFG_1; //0x24
	unsigned rPRM_HEC_CFG_2; //0x28
	unsigned rPRM_HEC_CFG_3; //0x2c
	unsigned rPRM_HEC_CFG_4; //0x30
};

#if 0
typedef struct _enhance_reg {
	unsigned int rGAMMA_COEF00; //0x4110
	unsigned int rGAMMA_COEF01;
	unsigned int rGAMMA_COEF02;
	unsigned int rGAMMA_COEF03;
	unsigned int rGAMMA_COEF04;

	unsigned int rGAMMA_RESERVED0[3];

	unsigned int rGAMMA_COEF10; //0x4130
	unsigned int rGAMMA_COEF11;
	unsigned int rGAMMA_COEF12;
	unsigned int rGAMMA_COEF13;
	unsigned int rGAMMA_COEF14;

	unsigned int rGAMMA_RESERVED1[3];

	unsigned int rGAMMA_COEF20; //0x4150
	unsigned int rGAMMA_COEF21;
	unsigned int rGAMMA_COEF22;
	unsigned int rGAMMA_COEF23;
	unsigned int rGAMMA_COEF24; //0x4160

	unsigned int rG_ENHANCEMENT_CTL; //0x4164
	unsigned int rB_STRETCH_CTL;
	unsigned int rENH_SRC_SEL;
	unsigned int rPRM_LTI_CFG_0; //0x4170
	unsigned int rPRM_LTI_CFG_1;

	unsigned int rPRM_PEAKING_CFG_0;
	unsigned int rPRM_PEAKING_CFG_1;
	unsigned int rPRM_CTI_CFG_0; //0x4180
	unsigned int rPRM_CTI_CFG_1;
	unsigned int rPRM_BLEC_BRT_CFG;
	unsigned int rPRM_HUE_STA_CON_CFG;
	unsigned int rPRM_HEC_CFG_0; //0x4190
	unsigned int rPRM_HEC_CFG_1;
	unsigned int rPRM_HEC_CFG_2;
	unsigned int rPRM_HEC_CFG_3;
	unsigned int rPRM_HEC_CFG_4; //0x41A0

	unsigned int rSCN_BLEC_BRT_HUE_CFG;
	unsigned int rSCN_STA_CON_CFG;
	unsigned int rMIX_Y_FIR_PARA0;
	unsigned int rMIX_Y_FIR_PARA1; //0x41B0
	unsigned int rMIX_C_FIR_PARA0;
	unsigned int rMIX_C_FIR_PARA1;
}enhance_reg;
#endif

/*  PRM_LTI_CFG_0  */
	 /* name: f_LTI_EN */
	 /* desc: 主画面LTI功能使能 (0：不做LTI  1：做LTI) */
	 /* range: [24:24] */
	 #define    b_LTI_EN    ( 24 )
	 #define    m_LTI_EN    ( 0x1 << b_LTI_EN)


	 /* name: f_LTI_TOT_LIMT */
	 /* desc: 主画面整体限幅门限 */
	 /* range: [23:16] */
	 #define    b_LTI_TOT_LIMT    ( 16 )
	 #define    m_LTI_TOT_LIMT    ( 0xff << b_LTI_TOT_LIMT)


	 /* name: f_LTI_TOT_CORE */
	 /* desc: 主画面整体效果降噪门限 */
	 /* range: [15:8] */
	 #define    b_LTI_TOT_CORE    ( 8 )
	 #define    m_LTI_TOT_CORE    ( 0xff << b_LTI_TOT_CORE)


	 /* name: f_LTI_DIF_CORE */
	 /* desc: 主画面一阶差分降噪门限 */
	 /* range: [7:0] */
	 #define    b_LTI_DIF_CORE    ( 0 )
	 #define    m_LTI_DIF_CORE    ( 0xff << b_LTI_DIF_CORE)


/*  PRM_LTI_CFG_1  */
	 /* name: f_LTI_GAIN_DLY */
	 /* desc: 主画面LTI勾边延时调整 */
	 /* range: [26:24] */
	 #define    b_LTI_GAIN_DLY    ( 24 )
	 #define    m_LTI_GAIN_DLY    ( 0x7 << b_LTI_GAIN_DLY)


	 /* name: f_LTI_TOT_GAIN */
	 /* desc: 主画面整体增益 */
	 /* range: [21:16] */
	 #define    b_LTI_TOT_GAIN    ( 16 )
	 #define    m_LTI_TOT_GAIN    ( 0x3f << b_LTI_TOT_GAIN)


	 /* name: f_LTI_LVL_GAIN */
	 /* desc: 主画面灰度拉伸增益 */
	 /* range: [13:8] */
	 #define    b_LTI_LVL_GAIN    ( 8 )
	 #define    m_LTI_LVL_GAIN    ( 0x3f << b_LTI_LVL_GAIN)


	 /* name: f_LTI_POS_GAIN */
	 /* desc: 主画面窗口位置增益 */
	 /* range: [5:0] */
	 #define    b_LTI_POS_GAIN    ( 0 )
	 #define    m_LTI_POS_GAIN    ( 0x3f << b_LTI_POS_GAIN)


/*  PRM_PEAKING_CFG_0  */
	 /* name: f_PEK_EN */
	 /* desc: 主画面Peaking功能使能 (0：不做Peaking   1：做Peaking) */
	 /* range: [24] */
	 #define    b_PEK_EN    ( 24 )
	 #define    m_PEK_EN    ( 0x1 << b_PEK_EN)


	 /* name: f_PEK_FRQ_CORE */
	 /* desc: 主画面频率判断降噪门限（<LTI_TOT_LIMT） */
	 /* range: [23:16] */
	 #define    b_PEK_FRQ_CORE    ( 16 )
	 #define    m_PEK_FRQ_CORE    ( 0xff << b_PEK_FRQ_CORE)


	 /* name: f_PEK_FIL_BK */
	 /* desc: 主画面可配置二阶滤波器BK参数 */
	 /* range: [12:8] */
	 #define    b_PEK_FIL_BK    ( 8 )
	 #define    m_PEK_FIL_BK    ( 0x1f << b_PEK_FIL_BK)


	 /* name: f_PEK_FIL_HK */
	 /* desc: 主画面可配置二阶滤波器HK参数 */
	 /* range: [4:0] */
	 #define    b_PEK_FIL_HK    ( 0 )
	 #define    m_PEK_FIL_HK    ( 0x1f << b_PEK_FIL_HK)


/*  PRM_PEAKING_CFG_1  */
	 /* name: f_PEK_FRQ_TH */
	 /* desc: 主画面高频判断门限 */
	 /* range: [11:8] */
	 #define    b_PEK_FRQ_TH    ( 8 )
	 #define    m_PEK_FRQ_TH    ( 0xf << b_PEK_FRQ_TH)


	 /* name: f_PEK_WIN_TH */
	 /* desc: 主画面窗口保护限幅门限 */
	 /* range: [7:0] */
	 #define    b_PEK_WIN_TH    ( 0 )
	 #define    m_PEK_WIN_TH    ( 0xff << b_PEK_WIN_TH)


/*  PRM_CTI_CFG_0  */
	 /* name: f_CTI_EN */
	 /* desc: 主画面CTI使能  0：不做CTI  1：做CTI */
	 /* range: [24] */
	 #define    b_CTI_EN    ( 24 )
	 #define    m_CTI_EN    ( 0x1 << b_CTI_EN)


	 /* name: f_CTI_STEP */
	 /* desc: 主画面限幅窗口选择 */
	 /* range: [21:20] */
	 #define    b_CTI_STEP    ( 20 )
	 #define    m_CTI_STEP    ( 0x3 << b_CTI_STEP)


	 /* name: f_CTI_FILTER_SEL */
	 /* desc: 主画面滤波器选择 */
	 /* range: [17:16] */
	 #define    b_CTI_FILTER_SEL    ( 16 )
	 #define    m_CTI_FILTER_SEL    ( 0x3 << b_CTI_FILTER_SEL)


	 /* name: f_CTI_WINDELTA */
	 /* desc: 主画面过冲调节门限 */
	 /* range: [15:8] */
	 #define    b_CTI_WINDELTA    ( 8 )
	 #define    m_CTI_WINDELTA    ( 0xff << b_CTI_WINDELTA)


	 /* name: f_CTI_CORE */
	 /* desc: 主画面降噪门限 */
	 /* range: [7:0] */
	 #define    b_CTI_CORE    ( 0 )
	 #define    m_CTI_CORE    ( 0xff << b_CTI_CORE)


/*  PRM_CTI_CFG_1  */
	 /* name: f_CTI_GAIN_DLY */
	 /* desc: 主画面CTI勾边延时调整 */
	 /* range: [18:16] */
	 #define    b_CTI_GAIN_DLY    ( 16 )
	 #define    m_CTI_GAIN_DLY    ( 0x7 << b_CTI_GAIN_DLY)


	 /* name: f_CTI_LVL_GAIN */
	 /* desc: 主画面色度拉伸增益调节 */
	 /* range: [12:8] */
	 #define    b_CTI_LVL_GAIN    ( 8 )
	 #define    m_CTI_LVL_GAIN    ( 0x1f << b_CTI_LVL_GAIN)


	 /* name: f_CTI_DIF_GAIN */
	 /* desc: 主画面二阶差分增益调节 */
	 /* range: [4:0] */
	 #define    b_CTI_DIF_GAIN    ( 0 )
	 #define    m_CTI_DIF_GAIN    ( 0x1f << b_CTI_DIF_GAIN)


/*  PRM_BLEC_BRT_CFG  */
	 /* name: f_PRM_BRT_STEP */
	 /* desc: 主画面亮度调节参数 */
	 /* range: [23:16] */
	 #define    b_PRM_BRT_STEP    ( 16 )
	 #define    m_PRM_BRT_STEP    ( 0xff << b_PRM_BRT_STEP)


	 /* name: f_PRM_BLEC_GAIN */
	 /* desc: 主画面黑电平扩展增益，为0时，即关闭该功 */
	 /* range: [12:8] */
	 #define    b_PRM_BLEC_GAIN    ( 8 )
	 #define    m_PRM_BLEC_GAIN    ( 0x1f << b_PRM_BLEC_GAIN)


	 /* name: f_PRM_BLEC_TILT */
	 /* desc: 主画面黑电平扩展门限 */
	 /* range: [7:0] */
	 #define    b_PRM_BLEC_TILT    ( 0 )
	 #define    m_PRM_BLEC_TILT    ( 0xff << b_PRM_BLEC_TILT)


/*  PRM_HUE_SAT_CON_CFG  */
	 /* name: f_PRM_CON_STEP */
	 /* desc: 主画面对比度调节参数，0~63 */
	 /* range: [21:16] */
	 #define    b_PRM_CON_STEP    ( 16 )
	 #define    m_PRM_CON_STEP    ( 0x3f << b_PRM_CON_STEP)


	 /* name: f_PRM_SAT_STEP */
	 /* desc: 主画面色饱和度调节参数，0~63 */
	 /* range: [13:8] */
	 #define    b_PRM_SAT_STEP    ( 8 )
	 #define    m_PRM_SAT_STEP    ( 0x3f << b_PRM_SAT_STEP)


	 /* name: f_PRM_HUE_ANGLE */
	 /* desc: 主画面色调调节参数，bit 6为符号位1为负0为正，-45~45 */
	 /* range: [6:0] */
	 #define    b_PRM_HUE_ANGLE    ( 0 )
	 #define    m_PRM_HUE_ANGLE    ( 0x7f << b_PRM_HUE_ANGLE)


/*  PRM_HEC_CFG_0  */
	 /* name: f_HEC_EN */
	 /* desc: 六基色使能 */
	 /* range: [4] */
	 #define    b_HEC_EN    ( 4 )
	 #define    m_HEC_EN    ( 0x1 << b_HEC_EN)


	 /* name: f_HEC_DIFF */
	 /* desc: 六基色调整差值控制 */
	 /* range: [3:0] */
	 #define    b_HEC_DIFF    ( 0 )
	 #define    m_HEC_DIFF    ( 0xf << b_HEC_DIFF)


/*  PRM_HEC_CFG_1  */
	 /* name: f_GRN_SAT_STEP */
	 /* desc: 六基色调整中绿色饱和度调节参数 */
	 /* range: [21:16] */
	 #define    b_GRN_SAT_STEP    ( 16 )
	 #define    m_GRN_SAT_STEP    ( 0x3f << b_GRN_SAT_STEP)


	 /* name: f_CYN_SAT_STEP */
	 /* desc: 六基色调整中青色饱和度调节参数 */
	 /* range: [13:8] */
	 #define    b_CYN_SAT_STEP    ( 8 )
	 #define    m_CYN_SAT_STEP    ( 0x3f << b_CYN_SAT_STEP)


	 /* name: f_YLW_SAT_STEP */
	 /* desc: 六基色调整中黄色饱和度调节参数 */
	 /* range: [5:0] */
	 #define    b_YLW_SAT_STEP    ( 0 )
	 #define    m_YLW_SAT_STEP    ( 0x3f << b_YLW_SAT_STEP)


/*  PRM_HEC_CFG_2  */
	 /* name: f_BLU_SAT_STEP */
	 /* desc: 六基色调整中蓝色饱和度调节参数 */
	 /* range: [21:16] */
	 #define    b_BLU_SAT_STEP    ( 16 )
	 #define    m_BLU_SAT_STEP    ( 0x3f << b_BLU_SAT_STEP)


	 /* name: f_RED_SAT_STEP */
	 /* desc: 六基色调整中红色饱和度调节参数 */
	 /* range: [13:8] */
	 #define    b_RED_SAT_STEP    ( 8 )
	 #define    m_RED_SAT_STEP    ( 0x3f << b_RED_SAT_STEP)


	 /* name: f_MGN_SAT_STEP */
	 /* desc: 六基色调整中紫色饱和度调节参数 */
	 /* range: [5:0] */
	 #define    b_MGN_SAT_STEP    ( 0 )
	 #define    m_MGN_SAT_STEP    ( 0x3f << b_MGN_SAT_STEP)


/*  PRM_HEC_CFG_3  */
	 /* name: f_GRN_HUE_ANGLE */
	 /* desc: 六基色调整中绿色色调调节角度控制 */
	 /* range: [23:16] */
	 #define    b_GRN_HUE_ANGLE    ( 16 )
	 #define    m_GRN_HUE_ANGLE    ( 0xff << b_GRN_HUE_ANGLE)


	 /* name: f_CYN_HUE_ANGLE */
	 /* desc: 六基色调整中青色色调调节角度控制 */
	 /* range: [14:8] */
	 #define    b_CYN_HUE_ANGLE    ( 8 )
	 #define    m_CYN_HUE_ANGLE    ( 0x7f << b_CYN_HUE_ANGLE)


	 /* name: f_YLW_HUE_ANGLE */
	 /* desc: 六基色调整中黄色色调调节角度控制 */
	 /* range: [6:0] */
	 #define    b_YLW_HUE_ANGLE    ( 0 )
	 #define    m_YLW_HUE_ANGLE    ( 0x7f << b_YLW_HUE_ANGLE)


/*  PRM_HEC_CFG_4  */
	 /* name: f_BLU_HUE_ANGLE */
	 /* desc: 六基色调整中蓝色色调调节角度控制 */
	 /* range: [23:16] */
	 #define    b_BLU_HUE_ANGLE    ( 16 )
	 #define    m_BLU_HUE_ANGLE    ( 0xff << b_BLU_HUE_ANGLE)


	 /* name: f_RED_HUE_ANGLE */
	 /* desc: 六基色调整中红色色调调节角度控制 */
	 /* range: [14:8] */
	 #define    b_RED_HUE_ANGLE    ( 8 )
	 #define    m_RED_HUE_ANGLE    ( 0x7f << b_RED_HUE_ANGLE)


	 /* name: f_MGN_HUE_ANGLE */
	 /* desc: 六基色调整中紫色色调调节角度控制 */
	 /* range: [6:0] */
	 #define    b_MGN_HUE_ANGLE    ( 0 )
	 #define    m_MGN_HUE_ANGLE    ( 0x7f << b_MGN_HUE_ANGLE)


#endif

