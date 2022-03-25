#ifndef    __VPU_SYS_H__
#define    __VPU_SYS_H__

struct cygnus_vpu_sys_reg {
	unsigned rVERSION; //0x0
	unsigned rRESV0[ 3 ];
	unsigned rSYS_CTROL; //0x10
	unsigned rSYS_STATUS; //0x14
	unsigned rSYS_PARA; //0x18
	unsigned rRESV1[ 1 ];
	unsigned rINT_MASK; //0x20
	unsigned rINT_CLEAR; //0x24
	unsigned rINT_STATUS; //0x28
	unsigned rINT_LINE_CTROL; //0x2c
	unsigned rBBT_TEST; //0x30
	unsigned rBBT_CRC0; //0x34
	unsigned rBBT_CRC1; //0x38
	unsigned rRESV2[ 1 ];
	unsigned rDAC_GAIN; //0x40
	unsigned rDAC_POWER; //0x44
	unsigned rDAC_SOURCE_SEL; //0x48
	unsigned rRESV3[ 1 ];
	unsigned rHD_MIX_LAYER_SEL; //0x50
	unsigned rHD_MIX_BACK_COLOR; //0x54
	unsigned rRESV4[ 2 ];
	unsigned rSD_MIX_LAYER_SEL; //0x60
	unsigned rSD_MIX_BACK_COLOR; //0x64
	unsigned rRESV5[ 2 ];
	unsigned rVBI_CTRL; //0x70
	unsigned rVBI_FIRST_ADDR; //0x74
	unsigned rVBI_READ_PTR; //0x78
};


/*  VERSION  */
	 /* name: f_VERSION */
	 /* desc:  */
	 /* range: [23:0] */
	 #define    b_VERSION    ( 0 )
	 #define    m_VERSION    ( 0xffffff << b_VERSION)


/*  SYS_CTROL  */
	 /* name: f_TIME_DLY */
	 /* desc: 内部状态机延时控制 */
	 /* range: [15:8] */
	 #define    b_TIME_DLY    ( 8 )
	 #define    m_TIME_DLY    ( 0xff << b_TIME_DLY)


	 /* name: f_CE_BOUND_TO_PP2_EN */
	 /* desc: CE和PP2绑定使能，高电平有效。当此寄存器为高电平时，PP2将直接采用CE抓下来的数据，两者协同工作 */
	 /* range: [4] */
	 #define    b_CE_BOUND_TO_PP2_EN    ( 4 )
	 #define    m_CE_BOUND_TO_PP2_EN    ( 0x1 << b_CE_BOUND_TO_PP2_EN)


	 /* name: f_DISPLAY_OFF */
	 /* desc: VPU 所有层关使能;1：强制关闭所有层显示;0：恢复由原层使能控制;生效时间为一个场周期，可以通过判断VPU_IDLE来确定是否生效 */
	 /* range: [0] */
	 #define    b_DISPLAY_OFF    ( 0 )
	 #define    m_DISPLAY_OFF    ( 0x1 << b_DISPLAY_OFF)


/*  SYS_STATUS  */
	 /* name: f_TOP_FIELD */
	 /* desc: 场状态标识；0表示当前硬件逻辑正在处理低场；1表示当前硬件逻辑正在处理顶场 */
	 /* range: [31] */
	 #define    b_TOP_FIELD    ( 31 )
	 #define    m_TOP_FIELD    ( 0x1 << b_TOP_FIELD)


	 /* name: f_ACTIVE_LINE_COUNTER */
	 /* desc: 当前播放行号 */
	 /* range: [26:16] */
	 #define    b_ACTIVE_LINE_COUNTER    ( 16 )
	 #define    m_ACTIVE_LINE_COUNTER    ( 0x7ff << b_ACTIVE_LINE_COUNTER)


	 /* name: f_APS_OTP_EN */
	 /* desc: 模拟输出拷贝保护OTP授权使能，高电平有效。模拟输出保护包括Macrovision和DCS */
	 /* range: [12] */
	 #define    b_APS_OTP_EN    ( 12 )
	 #define    m_APS_OTP_EN    ( 0x1 << b_APS_OTP_EN)


	 /* name: f_SDP_TVFORMAT */
	 /* desc: 当前标清通路输出分辨率 */
	 /* range: [11:8] */
	 #define    b_SDP_TVFORMAT    ( 8 )
	 #define    m_SDP_TVFORMAT    ( 0xf << b_SDP_TVFORMAT)


	 /* name: f_HDP_FORMATFREQ */
	 /* desc: 当前高清通路输出帧率 */
	 /* range: [6:4] */
	 #define    b_HDP_FORMATFREQ    ( 4 )
	 #define    m_HDP_FORMATFREQ    ( 0x7 << b_HDP_FORMATFREQ)


	 /* name: f_HDP_TVFORMAT */
	 /* desc: 当前高清通路输出分辨率 */
	 /* range: [3:0] */
	 #define    b_HDP_TVFORMAT    ( 0 )
	 #define    m_HDP_TVFORMAT    ( 0xf << b_HDP_TVFORMAT)


/*  SYS_PARA  */
	 /* name: f_SRAM_EN */
	 /* desc: 内部RAM工作模式控制；1:工作状态受使能为控制；0：一直处于工作状态 */
	 /* range: [31] */
	 #define    b_SRAM_EN    ( 31 )
	 #define    m_SRAM_EN    ( 0x1 << b_SRAM_EN)


	 /* name: f_LOCK_NORMAL_W */
	 /* desc: SDRAM总线写锁定控制，剩余长度不大于128像素的总线锁定 */
	 /* range: [13:12] */
	 #define    b_LOCK_NORMAL_W    ( 12 )
	 #define    m_LOCK_NORMAL_W    ( 0x3 << b_LOCK_NORMAL_W)


	 /* name: f_LOCK_REG_W */
	 /* desc: SDRAM总线写锁定控制，剩余长度大于128像素的总线锁定 */
	 /* range: [9:8] */
	 #define    b_LOCK_REG_W    ( 8 )
	 #define    m_LOCK_REG_W    ( 0x3 << b_LOCK_REG_W)


	 /* name: f_LOCK_NORMAL */
	 /* desc: SDRAM总线读锁定控制，剩余长度不大于128像素的总线锁定 */
	 /* range: [5:4] */
	 #define    b_LOCK_NORMAL    ( 4 )
	 #define    m_LOCK_NORMAL    ( 0x3 << b_LOCK_NORMAL)


	 /* name: f_LOCK_REG */
	 /* desc: SDRAM总线读锁定控制，剩余长度大于128像素的总线锁定 */
	 /* range: [1:0] */
	 #define    b_LOCK_REG    ( 0 )
	 #define    m_LOCK_REG    ( 0x3 << b_LOCK_REG)


/*  INT_MASK  */
	 /* name: f_INTERRPUTMASK_HDR */
	 /* desc: HDR bypass 中断MASK控制，高电平有效 */
	 /* range: [15] */
	 #define    b_INTERRPUTMASK_HDR    ( 15 )
	 #define    m_INTERRPUTMASK_HDR    ( 0x1 << b_INTERRPUTMASK_HDR)


	 /* name: f_INTERRPUTMASK_CE */
	 /* desc: CE层中断MASK控制，高电平有效 */
	 /* range: [14] */
	 #define    b_INTERRPUTMASK_CE    ( 14 )
	 #define    m_INTERRPUTMASK_CE    ( 0x1 << b_INTERRPUTMASK_CE)


	 /* name: f_INTERRPUTMASK_PP2 */
	 /* desc: PP2层中断MASK控制，高电平有效 */
	 /* range: [13] */
	 #define    b_INTERRPUTMASK_PP2    ( 13 )
	 #define    m_INTERRPUTMASK_PP2    ( 0x1 << b_INTERRPUTMASK_PP2)


	 /* name: f_INTERRPUTMASK_OSD1 */
	 /* desc: OSD1层中断MASK控制，高电平有效 */
	 /* range: [12] */
	 #define    b_INTERRPUTMASK_OSD1    ( 12 )
	 #define    m_INTERRPUTMASK_OSD1    ( 0x1 << b_INTERRPUTMASK_OSD1)


	 /* name: f_INTERRPUTMASK_OSD0 */
	 /* desc: OSD0层中断MASK控制，高电平有效 */
	 /* range: [11] */
	 #define    b_INTERRPUTMASK_OSD0    ( 11 )
	 #define    m_INTERRPUTMASK_OSD0    ( 0x1 << b_INTERRPUTMASK_OSD0)


	 /* name: f_INTERRPUTMASK_PIC */
	 /* desc: PIC层中断MASK控制，高电平有效 */
	 /* range: [10] */
	 #define    b_INTERRPUTMASK_PIC    ( 10 )
	 #define    m_INTERRPUTMASK_PIC    ( 0x1 << b_INTERRPUTMASK_PIC)


	 /* name: f_INTERRPUTMASK_PP1 */
	 /* desc: PP1层中断MASK控制，高电平有效 */
	 /* range: [9] */
	 #define    b_INTERRPUTMASK_PP1    ( 9 )
	 #define    m_INTERRPUTMASK_PP1    ( 0x1 << b_INTERRPUTMASK_PP1)


	 /* name: f_INTERRPUTMASK_PP0 */
	 /* desc: PP0层中断MASK控制，高电平有效 */
	 /* range: [8] */
	 #define    b_INTERRPUTMASK_PP0    ( 8 )
	 #define    m_INTERRPUTMASK_PP0    ( 0x1 << b_INTERRPUTMASK_PP0)


	 /* name: f_INTERRPUTMASK_FRAMEEND */
	 /* desc: 帧结束中断MASK控制，高电平有效 */
	 /* range: [5] */
	 #define    b_INTERRPUTMASK_FRAMEEND    ( 5 )
	 #define    m_INTERRPUTMASK_FRAMEEND    ( 0x1 << b_INTERRPUTMASK_FRAMEEND)


	 /* name: f_INTERRPUTMASK_FIELDEND */
	 /* desc: 场结束中断MASK控制，高电平有效 */
	 /* range: [4] */
	 #define    b_INTERRPUTMASK_FIELDEND    ( 4 )
	 #define    m_INTERRPUTMASK_FIELDEND    ( 0x1 << b_INTERRPUTMASK_FIELDEND)


	 /* name: f_INTERRPUTMASK_FRAMESTART */
	 /* desc: 帧起始中断MASK控制，高电平有效 */
	 /* range: [3] */
	 #define    b_INTERRPUTMASK_FRAMESTART    ( 3 )
	 #define    m_INTERRPUTMASK_FRAMESTART    ( 0x1 << b_INTERRPUTMASK_FRAMESTART)


	 /* name: f_INTERRPUTMASK_FIELDSTART */
	 /* desc: 场起始中断MASK控制，高电平有效 */
	 /* range: [2] */
	 #define    b_INTERRPUTMASK_FIELDSTART    ( 2 )
	 #define    m_INTERRPUTMASK_FIELDSTART    ( 0x1 << b_INTERRPUTMASK_FIELDSTART)


	 /* name: f_INTERRPUTMASK_LINE */
	 /* desc: 特定行中断MASK控制，高电平有效 */
	 /* range: [1] */
	 #define    b_INTERRPUTMASK_LINE    ( 1 )
	 #define    m_INTERRPUTMASK_LINE    ( 0x1 << b_INTERRPUTMASK_LINE)


	 /* name: f_INTERRPUTMASK_IDLE */
	 /* desc: 模块所有层空闲中断MASK控制，高电平有效 */
	 /* range: [0] */
	 #define    b_INTERRPUTMASK_IDLE    ( 0 )
	 #define    m_INTERRPUTMASK_IDLE    ( 0x1 << b_INTERRPUTMASK_IDLE)


/*  INT_CLEAR  */
	 /* name: f_INTERRPUTCLEAR_FRAMEEND */
	 /* desc: 帧起始中断CLEAR控制，高电平有效 */
	 /* range: [5] */
	 #define    b_INTERRPUTCLEAR_FRAMEEND    ( 5 )
	 #define    m_INTERRPUTCLEAR_FRAMEEND    ( 0x1 << b_INTERRPUTCLEAR_FRAMEEND)


	 /* name: f_INTERRPUTCLEAR_FIELDEND */
	 /* desc: 场起始中断CLEAR控制，高电平有效 */
	 /* range: [4] */
	 #define    b_INTERRPUTCLEAR_FIELDEND    ( 4 )
	 #define    m_INTERRPUTCLEAR_FIELDEND    ( 0x1 << b_INTERRPUTCLEAR_FIELDEND)


	 /* name: f_INTERRPUTCLEAR_FRAMESTART */
	 /* desc: 帧起始中断CLEAR控制，高电平有效 */
	 /* range: [3] */
	 #define    b_INTERRPUTCLEAR_FRAMESTART    ( 3 )
	 #define    m_INTERRPUTCLEAR_FRAMESTART    ( 0x1 << b_INTERRPUTCLEAR_FRAMESTART)


	 /* name: f_INTERRPUTCLEAR_FIELDSTART */
	 /* desc: 场起始中断CLEAR控制，高电平有效 */
	 /* range: [2] */
	 #define    b_INTERRPUTCLEAR_FIELDSTART    ( 2 )
	 #define    m_INTERRPUTCLEAR_FIELDSTART    ( 0x1 << b_INTERRPUTCLEAR_FIELDSTART)


	 /* name: f_INTERRPUTCLEAR_LINE */
	 /* desc: 特定行中断CLEAR控制，高电平有效 */
	 /* range: [1] */
	 #define    b_INTERRPUTCLEAR_LINE    ( 1 )
	 #define    m_INTERRPUTCLEAR_LINE    ( 0x1 << b_INTERRPUTCLEAR_LINE)


	 /* name: f_INTERRPUTCLEAR_IDLE */
	 /* desc: 模块所有层空闲中断清除控制，高电平有效 */
	 /* range: [0] */
	 #define    b_INTERRPUTCLEAR_IDLE    ( 0 )
	 #define    m_INTERRPUTCLEAR_IDLE    ( 0x1 << b_INTERRPUTCLEAR_IDLE)


/*  INT_STATUS  */
	 /* name: f_INTERRPUTSTATUS_FRAMEEND */
	 /* desc: 帧起始中断STATUS，高电平有效 */
	 /* range: [5] */
	 #define    b_INTERRPUTSTATUS_FRAMEEND    ( 5 )
	 #define    m_INTERRPUTSTATUS_FRAMEEND    ( 0x1 << b_INTERRPUTSTATUS_FRAMEEND)


	 /* name: f_INTERRPUTSTATUS_FIELDEND */
	 /* desc: 场起始中断STATUS，高电平有效 */
	 /* range: [4] */
	 #define    b_INTERRPUTSTATUS_FIELDEND    ( 4 )
	 #define    m_INTERRPUTSTATUS_FIELDEND    ( 0x1 << b_INTERRPUTSTATUS_FIELDEND)


	 /* name: f_INTERRPUTSTATUS_FRAMESTART */
	 /* desc: 帧起始中断STATUS，高电平有效 */
	 /* range: [3] */
	 #define    b_INTERRPUTSTATUS_FRAMESTART    ( 3 )
	 #define    m_INTERRPUTSTATUS_FRAMESTART    ( 0x1 << b_INTERRPUTSTATUS_FRAMESTART)


	 /* name: f_INTERRPUTSTATUS_FIELDSTART */
	 /* desc: 场起始中断STATUS，高电平有效 */
	 /* range: [2] */
	 #define    b_INTERRPUTSTATUS_FIELDSTART    ( 2 )
	 #define    m_INTERRPUTSTATUS_FIELDSTART    ( 0x1 << b_INTERRPUTSTATUS_FIELDSTART)


	 /* name: f_INTERRPUTSTATUS_LINE */
	 /* desc: 特定行中断STATUS，高电平有效 */
	 /* range: [1] */
	 #define    b_INTERRPUTSTATUS_LINE    ( 1 )
	 #define    m_INTERRPUTSTATUS_LINE    ( 0x1 << b_INTERRPUTSTATUS_LINE)


	 /* name: f_INTERRPUTSTATUS_IDLE */
	 /* desc: 模块所有层空闲中断状态，高电平有效 */
	 /* range: [0] */
	 #define    b_INTERRPUTSTATUS_IDLE    ( 0 )
	 #define    m_INTERRPUTSTATUS_IDLE    ( 0x1 << b_INTERRPUTSTATUS_IDLE)


/*  INT_LINE_CTROL  */
	 /* name: f_INTLINE */
	 /* desc: 特定中断行设置,用于设计特定行中断位置 */
	 /* range: [10:0] */
	 #define    b_INTLINE    ( 0 )
	 #define    m_INTLINE    ( 0x7ff << b_INTLINE)


/*  BBT_TEST  */
	 /* name: f_CRC_EN */
	 /* desc: CRC校验位置使能，高电平有效 */
	 /* range: [12] */
	 #define    b_CRC_EN    ( 12 )
	 #define    m_CRC_EN    ( 0x1 << b_CRC_EN)


	 /* name: f_CRC_MODE */
	 /* desc: CRC校验位置选择：1、校验VPU混合后数据；0、校验VOUT输出后数据 */
	 /* range: [8] */
	 #define    b_CRC_MODE    ( 8 )
	 #define    m_CRC_MODE    ( 0x1 << b_CRC_MODE)


	 /* name: f_CHIPTEST_REG */
	 /* desc:  0: VPU输出和PIC数据相同；其他VPU显示数据和PIC层不同 */
	 /* range: [7:0] */
	 #define    b_CHIPTEST_REG    ( 0 )
	 #define    m_CHIPTEST_REG    ( 0xff << b_CHIPTEST_REG)


/*  BBT_CRC0  */
	 /* name: f_CRC_OUT1 */
	 /* desc: vout第1路的输出持续一场时间通过CRC校验器的值 */
	 /* range: [31:16] */
	 #define    b_CRC_OUT1    ( 16 )
	 #define    m_CRC_OUT1    ( 0xffff << b_CRC_OUT1)


	 /* name: f_CRC_OUT0 */
	 /* desc: vout第0路的输出持续一场时间通过CRC校验器的值 */
	 /* range: [15:0] */
	 #define    b_CRC_OUT0    ( 0 )
	 #define    m_CRC_OUT0    ( 0xffff << b_CRC_OUT0)


/*  BBT_CRC1  */
	 /* name: f_CRC_OUT2 */
	 /* desc: vout第2路的输出持续一场时间通过CRC校验器的值 */
	 /* range: [15:0] */
	 #define    b_CRC_OUT2    ( 0 )
	 #define    m_CRC_OUT2    ( 0xffff << b_CRC_OUT2)


/*  DAC_GAIN  */
	 /* name: f_GDAC3_IPS */
	 /* desc: 第3路视频DAC增益控制 */
	 /* range: [29:24] */
	 #define    b_GDAC3_IPS    ( 24 )
	 #define    m_GDAC3_IPS    ( 0x3f << b_GDAC3_IPS)


	 /* name: f_GDAC2_IPS */
	 /* desc: 第2路视频DAC增益控制 */
	 /* range: [21:16] */
	 #define    b_GDAC2_IPS    ( 16 )
	 #define    m_GDAC2_IPS    ( 0x3f << b_GDAC2_IPS)


	 /* name: f_GDAC1_IPS */
	 /* desc: 第1路视频DAC增益控制 */
	 /* range: [13:8] */
	 #define    b_GDAC1_IPS    ( 8 )
	 #define    m_GDAC1_IPS    ( 0x3f << b_GDAC1_IPS)


	 /* name: f_GDAC0_IPS */
	 /* desc: 第0路视频DAC增益控制 */
	 /* range: [5:0] */
	 #define    b_GDAC0_IPS    ( 0 )
	 #define    m_GDAC0_IPS    ( 0x3f << b_GDAC0_IPS)


/*  DAC_POWER  */
	 /* name: f_ALL_POWERDOWN */
	 /* desc: 关闭所有的视频DAC使能，高电平有效 */
	 /* range: [4] */
	 #define    b_ALL_POWERDOWN    ( 4 )
	 #define    m_ALL_POWERDOWN    ( 0x1 << b_ALL_POWERDOWN)


	 /* name: f_DAC3_POWERDOWN */
	 /* desc: 第3路视频DAC 关闭控制，高电平有效 */
	 /* range: [3] */
	 #define    b_DAC3_POWERDOWN    ( 3 )
	 #define    m_DAC3_POWERDOWN    ( 0x1 << b_DAC3_POWERDOWN)


	 /* name: f_DAC2_POWERDOWN */
	 /* desc: 第2路视频DAC 关闭控制，高电平有效 */
	 /* range: [2] */
	 #define    b_DAC2_POWERDOWN    ( 2 )
	 #define    m_DAC2_POWERDOWN    ( 0x1 << b_DAC2_POWERDOWN)


	 /* name: f_DAC1_POWERDOWN */
	 /* desc: 第1路视频DAC 关闭控制，高电平有效 */
	 /* range: [1] */
	 #define    b_DAC1_POWERDOWN    ( 1 )
	 #define    m_DAC1_POWERDOWN    ( 0x1 << b_DAC1_POWERDOWN)


	 /* name: f_DAC0_POWERDOWN */
	 /* desc: 第0路视频DAC 关闭控制，高电平有效 */
	 /* range: [0] */
	 #define    b_DAC0_POWERDOWN    ( 0 )
	 #define    m_DAC0_POWERDOWN    ( 0x1 << b_DAC0_POWERDOWN)


/*  DAC_SOURCE_SEL  */
	 /* name: f_DAC_POWERDOWN_BYSELF */
	 /* desc: 硬件自动检测复制关视频dac通路控制使能，高电平有效 */
	 /* range: [31] */
	 #define    b_DAC_POWERDOWN_BYSELF    ( 31 )
	 #define    m_DAC_POWERDOWN_BYSELF    ( 0x1 << b_DAC_POWERDOWN_BYSELF)


	 /* name: f_S3 */
	 /* desc: 第3路负载检测使能，高电平有效 */
	 /* range: [27] */
	 #define    b_S3    ( 27 )
	 #define    m_S3    ( 0x1 << b_S3)


	 /* name: f_S2 */
	 /* desc: 第2路负载检测使能，高电平有效 */
	 /* range: [26] */
	 #define    b_S2    ( 26 )
	 #define    m_S2    ( 0x1 << b_S2)


	 /* name: f_S1 */
	 /* desc: 第1路负载检测使能，高电平有效 */
	 /* range: [25] */
	 #define    b_S1    ( 25 )
	 #define    m_S1    ( 0x1 << b_S1)


	 /* name: f_S0 */
	 /* desc: 第0路负载检测使能，高电平有效 */
	 /* range: [24] */
	 #define    b_S0    ( 24 )
	 #define    m_S0    ( 0x1 << b_S0)


	 /* name: f_DAC_3_FLAG */
	 /* desc: 第3路视频DAC数据源选择；0: 来自数据通路0；1: 来自数据通路1；2: 来自数据通路2；3: 来自数据通路3； */
	 /* range: [23:22] */
	 #define    b_DAC_3_FLAG    ( 22 )
	 #define    m_DAC_3_FLAG    ( 0x3 << b_DAC_3_FLAG)


	 /* name: f_DAC_2_FLAG */
	 /* desc: 第2路视频DAC数据源选择；0: 来自数据通路0；1: 来自数据通路1；2: 来自数据通路2；3: 来自数据通路3； */
	 /* range: [21:20] */
	 #define    b_DAC_2_FLAG    ( 20 )
	 #define    m_DAC_2_FLAG    ( 0x3 << b_DAC_2_FLAG)


	 /* name: f_DAC_1_FLAG */
	 /* desc: 第1路视频DAC数据源选择；0: 来自数据通路0；1: 来自数据通路1；2: 来自数据通路2；3: 来自数据通路3； */
	 /* range: [19:18] */
	 #define    b_DAC_1_FLAG    ( 18 )
	 #define    m_DAC_1_FLAG    ( 0x3 << b_DAC_1_FLAG)


	 /* name: f_DAC_0_FLAG */
	 /* desc: 第0路视频DAC数据源选择；0: 来自数据通路0；1: 来自数据通路1；2: 来自数据通路2；3: 来自数据通路3； */
	 /* range: [17:16] */
	 #define    b_DAC_0_FLAG    ( 16 )
	 #define    m_DAC_0_FLAG    ( 0x3 << b_DAC_0_FLAG)


	 /* name: f_DAC3_SOURCE_SEL */
	 /* desc: 数据源选择控制；0：第3路数据来自高清通路；1：第3路数据来自标清通路； */
	 /* range: [3] */
	 #define    b_DAC3_SOURCE_SEL    ( 3 )
	 #define    m_DAC3_SOURCE_SEL    ( 0x1 << b_DAC3_SOURCE_SEL)


	 /* name: f_DAC2_SOURCE_SEL */
	 /* desc: 数据源选择控制；0：第2路数据来自高清通路；1：第2路数据来自标清通路； */
	 /* range: [2] */
	 #define    b_DAC2_SOURCE_SEL    ( 2 )
	 #define    m_DAC2_SOURCE_SEL    ( 0x1 << b_DAC2_SOURCE_SEL)


	 /* name: f_DAC1_SOURCE_SEL */
	 /* desc: 数据源选择控制；0：第1路数据来自高清通路；1：第1路数据来自标清通路； */
	 /* range: [1] */
	 #define    b_DAC1_SOURCE_SEL    ( 1 )
	 #define    m_DAC1_SOURCE_SEL    ( 0x1 << b_DAC1_SOURCE_SEL)


	 /* name: f_DAC0_SOURCE_SEL */
	 /* desc: 数据源选择控制；0：第0路数据来自高清通路；1：第0路数据来自标清通路； */
	 /* range: [0] */
	 #define    b_DAC0_SOURCE_SEL    ( 0 )
	 #define    m_DAC0_SOURCE_SEL    ( 0x1 << b_DAC0_SOURCE_SEL)


/*  HD_MIX_LAYER_SEL  */
	 /* name: f_HD_LAYER4_SEL */
	 /* desc: HD通路 Layer4输出图像层选择；0：无图层输出；1：输出PIC层图像；2：输出PP0层图像；3：输出PP1层图像；4：输出OSD层图像；5：输出OSD1层图像；其他：保留 */
	 /* range: [19:16] */
	 #define    b_HD_LAYER4_SEL    ( 16 )
	 #define    m_HD_LAYER4_SEL    ( 0xf << b_HD_LAYER4_SEL)


	 /* name: f_HD_LAYER3_SEL */
	 /* desc: HD通路 Layer3输出图像层选择；0：无图层输出；1：输出PIC层图像；2：输出PP0层图像；3：输出PP1层图像；4：输出OSD层图像；5：输出OSD1层图像；其他：保留 */
	 /* range: [15:12] */
	 #define    b_HD_LAYER3_SEL    ( 12 )
	 #define    m_HD_LAYER3_SEL    ( 0xf << b_HD_LAYER3_SEL)


	 /* name: f_HD_LAYER2_SEL */
	 /* desc: HD通路 Layer2输出图像层选择；0：无图层输出；1：输出PIC层图像；2：输出PP0层图像；3：输出PP1层图像；4：输出OSD层图像；5：输出OSD1层图像；其他：保留 */
	 /* range: [11:8] */
	 #define    b_HD_LAYER2_SEL    ( 8 )
	 #define    m_HD_LAYER2_SEL    ( 0xf << b_HD_LAYER2_SEL)


	 /* name: f_HD_LAYER1_SEL */
	 /* desc: HD通路 Layer1输出图像层选择；0：无图层输出；1：输出PIC层图像；2：输出PP0层图像；3：输出PP1层图像；4：输出OSD层图像；5：输出OSD1层图像；其他：保留 */
	 /* range: [7:4] */
	 #define    b_HD_LAYER1_SEL    ( 4 )
	 #define    m_HD_LAYER1_SEL    ( 0xf << b_HD_LAYER1_SEL)


	 /* name: f_HD_LAYER0_SEL */
	 /* desc: HD通路 Layer0输出图像层选择；0：无图层输出；1：输出PIC层图像；2：输出PP0层图像；3：输出PP1层图像；4：输出OSD层图像；5：输出OSD1层图像；其他：保留 */
	 /* range: [3:0] */
	 #define    b_HD_LAYER0_SEL    ( 0 )
	 #define    m_HD_LAYER0_SEL    ( 0xf << b_HD_LAYER0_SEL)


/*  HD_MIX_BACK_COLOR  */
	 /* name: f_HDP_BACKCOLOR */
	 /* desc: 高清通路背景色 */
	 /* range: [23:0] */
	 #define    b_HDP_BACKCOLOR    ( 0 )
	 #define    m_HDP_BACKCOLOR    ( 0xffffff << b_HDP_BACKCOLOR)


/*  SD_MIX_LAYER_SEL  */
	 /* name: f_SD_LAYER2_SEL */
	 /* desc: SD通路 Layer2输出图像层选择；0：无图层输出；1：输出SPP层图像；2：输出PP1层图像；3：输出OSD1层图像；其他：保留 */
	 /* range: [11:8] */
	 #define    b_SD_LAYER2_SEL    ( 8 )
	 #define    m_SD_LAYER2_SEL    ( 0xf << b_SD_LAYER2_SEL)


	 /* name: f_SD_LAYER1_SEL */
	 /* desc: SD通路 Layer2输出图像层选择；0：无图层输出；1：输出SPP层图像；2：输出PP1层图像；3：输出OSD1层图像；其他：保留 */
	 /* range: [7:4] */
	 #define    b_SD_LAYER1_SEL    ( 4 )
	 #define    m_SD_LAYER1_SEL    ( 0xf << b_SD_LAYER1_SEL)


	 /* name: f_SD_LAYER0_SEL */
	 /* desc: SD通路 Layer2输出图像层选择；0：无图层输出；1：输出SPP层图像；2：输出PP1层图像；3：输出OSD1层图像；其他：保留 */
	 /* range: [3:0] */
	 #define    b_SD_LAYER0_SEL    ( 0 )
	 #define    m_SD_LAYER0_SEL    ( 0xf << b_SD_LAYER0_SEL)


/*  SD_MIX_BACK_COLOR  */
	 /* name: f_SDP_BACKCOLOR */
	 /* desc: 标清通路背景色值 */
	 /* range: [23:0] */
	 #define    b_SDP_BACKCOLOR    ( 0 )
	 #define    m_SDP_BACKCOLOR    ( 0xffffff << b_SDP_BACKCOLOR)


/*  VBI_CTRL  */
	 /* name: f_VBI_ENDPOINT */
	 /* desc: VPS、TELETEXT数据申请字节序控制 */
	 /* range: [18:16] */
	 #define    b_VBI_ENDPOINT    ( 16 )
	 #define    m_VBI_ENDPOINT    ( 0x7 << b_VBI_ENDPOINT)


	 /* name: f_VBI_LEN */
	 /* desc: VPS、TELETEXT VBI数据长度 */
	 /* range: [5:1] */
	 #define    b_VBI_LEN    ( 1 )
	 #define    m_VBI_LEN    ( 0x1f << b_VBI_LEN)


	 /* name: f_VBI_EN */
	 /* desc: VPS、TELETEXT 数据使能，高电平有效 */
	 /* range: [0] */
	 #define    b_VBI_EN    ( 0 )
	 #define    m_VBI_EN    ( 0x1 << b_VBI_EN)


/*  VBI_FIRST_ADDR  */
	 /* name: f_VBI_FIRST_ADDR */
	 /* desc: VPS、TELETEXT VBI数据起始指针 */
	 /* range: [31:0] */
	 #define    b_VBI_FIRST_ADDR    ( 0 )
	 #define    m_VBI_FIRST_ADDR    ( 0xffffffff << b_VBI_FIRST_ADDR)


/*  VBI_READ_PTR  */
	 /* name: f_VBI_READ_PTR */
	 /* desc: 当时VBI数据读取指针位置 */
	 /* range: [31:0] */
	 #define    b_VBI_READ_PTR    ( 0 )
	 #define    m_VBI_READ_PTR    ( 0xffffffff << b_VBI_READ_PTR)

#endif

