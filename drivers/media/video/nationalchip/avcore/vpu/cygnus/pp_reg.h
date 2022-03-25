#ifndef    __VPU_PP_H__
#define    __VPU_PP_H__

struct cygnus_vpu_pp_reg {
	unsigned rPP_ENABLE; //0x0
	unsigned rPP_V_PHASE; //0x4
	unsigned rPP_POSITION; //0x8
	unsigned rPP_SOURCE_SIZE; //0xc
	unsigned rPP_VIEW_SIZE; //0x10
	unsigned rPP_ZOOM; //0x14
	unsigned rPP_FRAME_STRIDE; //0x18
	unsigned rPP_ZOOM_FILTER_SIGN; //0x1c
	unsigned rPP_PHASE0_H; //0x20
	unsigned rPP_PHASE0_V; //0x24
	unsigned rPP_DISP_CTL; //0x28
	unsigned rPP_DISP_R_PTR; //0x2c
	unsigned rPP_BACKCOLOR; //0x30
	unsigned rPP_CTRL; //0x34
	unsigned rRESV0[ 2 ];
	unsigned rPP_INT_EMPTY_GATE; //0x40
	unsigned rPP_BUFFER_INT; //0x44
	unsigned rRESV1[ 2 ];
	unsigned rPP_HFILTER_PARA1; //0x50
	unsigned rPP_HFILTER_PARA2; //0x54
	unsigned rPP_HFILTER_PARA3; //0x58
	unsigned rPP_HFILTER_PARA4; //0x5c
	unsigned rRESV2[ 40 ];
	unsigned rPP_ZOOM_PARA_H[64]; //0x100
	unsigned rPP_ZOOM_PARA_V[64]; //0x200
	unsigned rDISP_CTRL[8][12]; //0x300
};

/*  PP_ENABLE  */
	 /* name: f_PP_EN */
	 /* desc: 图层显示使能信号，高电平有效 */
	 /* range: [0] */
	 #define    b_PP_EN    ( 0 )
	 #define    m_PP_EN    ( 0x1 << b_PP_EN)


/*  PP_V_PHASE  */
	 /* name: f_PP_VB_PHASE_BIASE */
	 /* desc: 层显示底场输出起始相位，相对于源数据，4096单位为一行 */
	 /* range: [30:16] */
	 #define    b_PP_VB_PHASE_BIASE    ( 16 )
	 #define    m_PP_VB_PHASE_BIASE    ( 0x7fff << b_PP_VB_PHASE_BIASE)


	 /* name: f_PP_VT_PHASE_BIASE */
	 /* desc: 层显示顶场输出起始相位，相对于源数据，4096单位为一行 */
	 /* range: [14:0] */
	 #define    b_PP_VT_PHASE_BIASE    ( 0 )
	 #define    m_PP_VT_PHASE_BIASE    ( 0x7fff << b_PP_VT_PHASE_BIASE)


/*  PP_POSITION  */
	 /* name: f_PP_V_ZOOM_DOWN_SCALE */
	 /* desc: 垂直方向将采样控制；0：不做将采样处理；1：2像素取降采样处理;2：4像素取降采样处理;3：8像素取降采样处理;4：16像素取降采样处理;5：32像素取降采样处理 */
	 /* range: [30:28] */
	 #define    b_PP_V_ZOOM_DOWN_SCALE    ( 28 )
	 #define    m_PP_V_ZOOM_DOWN_SCALE    ( 0x7 << b_PP_V_ZOOM_DOWN_SCALE)


	 /* name: f_PP_START_Y */
	 /* desc: 在显示制式下垂直方向显示起始行 */
	 /* range: [26:16] */
	 #define    b_PP_START_Y    ( 16 )
	 #define    m_PP_START_Y    ( 0x7ff << b_PP_START_Y)


	 /* name: f_PP_H_ZOOM_DOWN_SCALE */
	 /* desc: 水平方向将采样控制；0：不做将采样处理；1：2行取1行降采样处理;2：4行取1行降采样处理;3：8行取1行降采样处理;4：16行取1行降采样处理;5：32行取1行降采样处理 */
	 /* range: [14:12] */
	 #define    b_PP_H_ZOOM_DOWN_SCALE    ( 12 )
	 #define    m_PP_H_ZOOM_DOWN_SCALE    ( 0x7 << b_PP_H_ZOOM_DOWN_SCALE)


	 /* name: f_PP_START_X */
	 /* desc: 在显示制式下水平直方向显示起始像素 */
	 /* range: [10:0] */
	 #define    b_PP_START_X    ( 0 )
	 #define    m_PP_START_X    ( 0x7ff << b_PP_START_X)


/*  PP_SOURCE_SIZE  */
	 /* name: f_PP_REQ_HEIGHT */
	 /* desc: 源数据高度，以行为单位 */
	 /* range: [26:16] */
	 #define    b_PP_REQ_HEIGHT    ( 16 )
	 #define    m_PP_REQ_HEIGHT    ( 0x7ff << b_PP_REQ_HEIGHT)


	 /* name: f_PP_REQ_LENGTH */
	 /* desc: 源数据宽度，以像素为单位 */
	 /* range: [11:0] */
	 #define    b_PP_REQ_LENGTH    ( 0 )
	 #define    m_PP_REQ_LENGTH    ( 0xfff << b_PP_REQ_LENGTH)


/*  PP_VIEW_SIZE  */
	 /* name: f_PP_VIEW_HEIGHT */
	 /* desc: 缩放输出数据高度，以行为单位 */
	 /* range: [26:16] */
	 #define    b_PP_VIEW_HEIGHT    ( 16 )
	 #define    m_PP_VIEW_HEIGHT    ( 0x7ff << b_PP_VIEW_HEIGHT)


	 /* name: f_PP_VIEW_LENGTH */
	 /* desc: 缩放输出数据宽度，以像素为单位 */
	 /* range: [10:0] */
	 #define    b_PP_VIEW_LENGTH    ( 0 )
	 #define    m_PP_VIEW_LENGTH    ( 0x7ff << b_PP_VIEW_LENGTH)


/*  PP_ZOOM  */
	 /* name: f_PP_V_ZOOM */
	 /* desc: 垂直方向缩放系数，计算公式为：源高度*4096/目标高度，取整 */
	 /* range: [30:16] */
	 #define    b_PP_V_ZOOM    ( 16 )
	 #define    m_PP_V_ZOOM    ( 0x7fff << b_PP_V_ZOOM)


	 /* name: f_PP_H_ZOOM */
	 /* desc: 水平方向缩放系数，计算公式为：源宽度*4096/目标宽度，取整 */
	 /* range: [14:0] */
	 #define    b_PP_H_ZOOM    ( 0 )
	 #define    m_PP_H_ZOOM    ( 0x7fff << b_PP_H_ZOOM)


/*  PP_FRAME_STRIDE  */
	 /* name: f_PP_BUFF_LEN */
	 /* desc: 层数据申请单次申请长度，为提供总线效率，为128整数倍 */
	 /* range: [26:16] */
	 #define    b_PP_BUFF_LEN    ( 16 )
	 #define    m_PP_BUFF_LEN    ( 0x7ff << b_PP_BUFF_LEN)


/*  PP_ZOOM_FILTER_SIGN  */
	 /* name: f_PP_ENDIAN_MODE */
	 /* desc: 层数据排列字节序,例如像素数据从左到右连续8个像素编号分辨为B0、B1、B2、B3、B4、B5、B6、B7；0:排序为B0、B1、B2、B3、B4、B5、B6、B7；1:排序为B7、B6、B5、B4、B3、B2、B1、B0；2:排序为B4、B5、B6、B7、B0、B1、B2、B3；3:排序为B3、B2、B1、B0、B7、B6、B5、B4；4:排序为B1、B0、B3、B2、B5、B4、B7、B6；5:排序为B6、B7、B4、B5、B2、B3、B1、B0；6:排序为B5、B4、B7、B6、B1、B0、B3、B2；7:排序为B3、B2、B0、B1、B5、B4、B7、B6 */
	 /* range: [26:24] */
	 #define    b_PP_ENDIAN_MODE    ( 24 )
	 #define    m_PP_ENDIAN_MODE    ( 0x7 << b_PP_ENDIAN_MODE)


	 /* name: f_PP_PHASE0_V_PN */
	 /* desc: 垂直滤波初始滤波系数符号，使用CUBIC滤波系数时配置为4’b1001 */
	 /* range: [23:20] */
	 #define    b_PP_PHASE0_V_PN    ( 20 )
	 #define    m_PP_PHASE0_V_PN    ( 0xf << b_PP_PHASE0_V_PN)


	 /* name: f_PP_PHASE0_H_PN */
	 /* desc: 水平滤波初始滤波系数符号，使用CUBIC滤波系数时配置为4’b1001 */
	 /* range: [19:16] */
	 #define    b_PP_PHASE0_H_PN    ( 16 )
	 #define    m_PP_PHASE0_H_PN    ( 0xf << b_PP_PHASE0_H_PN)


	 /* name: f_PP_V_PN_CHG_POS */
	 /* desc: 垂直滤波系数负正符号变更位置，使用CUBIC滤波系数时配置为8’d128 */
	 /* range: [15:8] */
	 #define    b_PP_V_PN_CHG_POS    ( 8 )
	 #define    m_PP_V_PN_CHG_POS    ( 0xff << b_PP_V_PN_CHG_POS)


	 /* name: f_PP_H_PN_CHG_POS */
	 /* desc: 水平滤波系数负正符号变更位置，使用CUBIC滤波系数时配置为8’d128 */
	 /* range: [7:0] */
	 #define    b_PP_H_PN_CHG_POS    ( 0 )
	 #define    m_PP_H_PN_CHG_POS    ( 0xff << b_PP_H_PN_CHG_POS)


/*  PP_PHASE0_H  */
	 /* name: f_PP_PHASE_0_H */
	 /* desc: 水平缩放0相位值，和PP_ZOOM_PARA_H参数配套使用 */
	 /* range: [31:0] */
	 #define    b_PP_PHASE_0_H    ( 0 )
	 #define    m_PP_PHASE_0_H    ( 0xffffffff << b_PP_PHASE_0_H)


/*  PP_PHASE0_V  */
	 /* name: f_PP_PHASE_0_V */
	 /* desc: 垂直缩放0相位值，和PP_ZOOM_PARA_V参数配套使用 */
	 /* range: [31:0] */
	 #define    b_PP_PHASE_0_V    ( 0 )
	 #define    m_PP_PHASE_0_V    ( 0xffffffff << b_PP_PHASE_0_V)


/*  PP_DISP_CTL  */
	 /* name: f_DISP_BUFFER_CNT */
	 /* desc: Display Word已经存储帧数统计，写一帧加1，读一帧减1 */
	 /* range: [19:16] */
	 #define    b_DISP_BUFFER_CNT    ( 16 )
	 #define    m_DISP_BUFFER_CNT    ( 0xf << b_DISP_BUFFER_CNT)


	 /* name: f_DISP_RST */
	 /* desc: Display Word已经存储帧数统计计清0控制，高电平有效 */
	 /* range: [12] */
	 #define    b_DISP_RST    ( 12 )
	 #define    m_DISP_RST    ( 0x1 << b_DISP_RST)


/*  PP_DISP_R_PTR  */
	 /* name: f_DISP_R_PTR */
	 /* desc: 播放显示控制。用于指示当前播放哪一帧数据 */
	 /* range: [5:0] */
	 #define    b_DISP_R_PTR    ( 0 )
	 #define    m_DISP_R_PTR    ( 0x7F << b_DISP_R_PTR)


/*  PP_BACKCOLOR  */
	 /* name: f_PP_ALPHA */
	 /* desc: 层透明度控制，0为全透明，255为不透明 */
	 /* range: [31:24] */
	 #define    b_PP_ALPHA    ( 24 )
	 #define    m_PP_ALPHA    ( 0xff << b_PP_ALPHA)


	 /* name: f_PP_BACK_COLOR */
	 /* desc: 层背景色控制；23:16 Y值；15:8 CB值； 7:0 Cr值 */
	 /* range: [23:0] */
	 #define    b_PP_BACK_COLOR    ( 0 )
	 #define    m_PP_BACK_COLOR    ( 0xffffff << b_PP_BACK_COLOR)


/*  PP_CTRL  */
	 /* name: f_PP_REG_LOCK_EN */
	 /* desc: PP相关寄存器更新使能：;1：不允许更新pp相关寄存器。配置的PP相关寄存器将会被暂时寄存，不做更新。;0：PP相关寄存器配置直接生效 */
	 /* range: [31] */
	 #define    b_PP_REG_LOCK_EN    ( 31 )
	 #define    m_PP_REG_LOCK_EN    ( 0x1 << b_PP_REG_LOCK_EN)


	 /* name: f_PP_H_PHASE_BIAS */
	 /* desc: 水平相位计数器起始相位。两个像素间划分了4096个相位 */
	 /* range: [30:16] */
	 #define    b_PP_H_PHASE_BIAS    ( 16 )
	 #define    m_PP_H_PHASE_BIAS    ( 0x7fff << b_PP_H_PHASE_BIAS)


	 /* name: f_PP_REG_UPDATEBYFRAME */
	 /* desc: PP寄存器在场起始信号位置更新使能，高电平有效 */
	 /* range: [15] */
	 #define    b_PP_REG_UPDATEBYFRAME    ( 15 )
	 #define    m_PP_REG_UPDATEBYFRAME    ( 0x1 << b_PP_REG_UPDATEBYFRAME)


	 /* name: f_PP_UV_H_FILTER_EN */
	 /* desc: UV水平滤波使能控制，高电平有效 */
	 /* range: [12] */
	 #define    b_PP_UV_H_FILTER_EN    ( 12 )
	 #define    m_PP_UV_H_FILTER_EN    ( 0x1 << b_PP_UV_H_FILTER_EN)


	 /* name: f_PP_UV_MODE */
	 /* desc: UV升采样滤波计算方式;3:水平升UV为四点插值;2: 均值;1: 重复;0: 线性滤波 */
	 /* range: [7:6] */
	 #define    b_PP_UV_MODE    ( 6 )
	 #define    m_PP_UV_MODE    ( 0x3 << b_PP_UV_MODE)


	 /* name: f_PP_Y_ZOOM_MODE_V */
	 /* desc: 垂直缩放模式；当VZOOM为4096时，必须为1；当VZOOM非4096时，必须为0 */
	 /* range: [5] */
	 #define    b_PP_Y_ZOOM_MODE_V    ( 5 )
	 #define    m_PP_Y_ZOOM_MODE_V    ( 0x1 << b_PP_Y_ZOOM_MODE_V)


	 /* name: f_PP_Y_ZOOM_MODE_H */
	 /* desc: 水平缩放模式；当HZOOM为4096时，必须为1；当HZOOM非4096时，必须为0 */
	 /* range: [4] */
	 #define    b_PP_Y_ZOOM_MODE_H    ( 4 )
	 #define    m_PP_Y_ZOOM_MODE_H    ( 0x1 << b_PP_Y_ZOOM_MODE_H)


/*  PP_INT_EMPTY_GATE  */
	 /* name: f_INT_PP_EMPTY_GATE */
	 /* desc: 行buffer数据存储门限警报标志，当存储不足配置值时，发起中断,最大值为240 */
	 /* range: [11:0] */
	 #define    b_INT_PP_EMPTY_GATE    ( 0 )
	 #define    m_INT_PP_EMPTY_GATE    ( 0xfff << b_INT_PP_EMPTY_GATE)


/*  PP_BUFFER_INT  */
	 /* name: f_INT_PP_IDLE_EN */
	 /* desc: 层行关闭完成中断使能 */
	 /* range: [18] */
	 #define    b_INT_PP_IDLE_EN    ( 18 )
	 #define    m_INT_PP_IDLE_EN    ( 0x1 << b_INT_PP_IDLE_EN)


	 /* name: f_INT_PP_EMPTY_EN */
	 /* desc: 层行buffer读空中断使能 */
	 /* range: [17] */
	 #define    b_INT_PP_EMPTY_EN    ( 17 )
	 #define    m_INT_PP_EMPTY_EN    ( 0x1 << b_INT_PP_EMPTY_EN)


	 /* name: f_INT_PP_ALMOST_EN */
	 /* desc: 层行buffer将要读空中断使能 */
	 /* range: [16] */
	 #define    b_INT_PP_ALMOST_EN    ( 16 )
	 #define    m_INT_PP_ALMOST_EN    ( 0x1 << b_INT_PP_ALMOST_EN)


	 /* name: f_INT_PP_IDLE */
	 /* desc: 层行关闭完成中断使能状态，写1清0 */
	 /* range: [2] */
	 #define    b_INT_PP_IDLE    ( 2 )
	 #define    m_INT_PP_IDLE    ( 0x1 << b_INT_PP_IDLE)


	 /* name: f_INT_PP_EMPTY */
	 /* desc: 层行buffer读空中断状态，写1清0 */
	 /* range: [1] */
	 #define    b_INT_PP_EMPTY    ( 1 )
	 #define    m_INT_PP_EMPTY    ( 0x1 << b_INT_PP_EMPTY)


	 /* name: f_INT_PP_ALMOST */
	 /* desc: 层行buffer将要读空中断状态，写1清0 */
	 /* range: [0] */
	 #define    b_INT_PP_ALMOST    ( 0 )
	 #define    m_INT_PP_ALMOST    ( 0x1 << b_INT_PP_ALMOST)


/*  PP_HFILTER_PARA1  */
	 /* name: f_PP_FIR3 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数3 */
	 /* range: [31:24] */
	 #define    b_PP_FIR3    ( 24 )
	 #define    m_PP_FIR3    ( 0xff << b_PP_FIR3)


	 /* name: f_PP_FIR2 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数2 */
	 /* range: [23:16] */
	 #define    b_PP_FIR2    ( 16 )
	 #define    m_PP_FIR2    ( 0xff << b_PP_FIR2)


	 /* name: f_PP_FIR1 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数1 */
	 /* range: [15:8] */
	 #define    b_PP_FIR1    ( 8 )
	 #define    m_PP_FIR1    ( 0xff << b_PP_FIR1)


	 /* name: f_PP_FIR0 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数0 */
	 /* range: [7:0] */
	 #define    b_PP_FIR0    ( 0 )
	 #define    m_PP_FIR0    ( 0xff << b_PP_FIR0)


/*  PP_HFILTER_PARA2  */
	 /* name: f_PP_FIR7 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数7 */
	 /* range: [31:24] */
	 #define    b_PP_FIR7    ( 24 )
	 #define    m_PP_FIR7    ( 0xff << b_PP_FIR7)


	 /* name: f_PP_FIR6 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数6 */
	 /* range: [23:16] */
	 #define    b_PP_FIR6    ( 16 )
	 #define    m_PP_FIR6    ( 0xff << b_PP_FIR6)


	 /* name: f_PP_FIR5 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数5 */
	 /* range: [15:8] */
	 #define    b_PP_FIR5    ( 8 )
	 #define    m_PP_FIR5    ( 0xff << b_PP_FIR5)


	 /* name: f_PP_FIR4 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数4 */
	 /* range: [7:0] */
	 #define    b_PP_FIR4    ( 0 )
	 #define    m_PP_FIR4    ( 0xff << b_PP_FIR4)


/*  PP_HFILTER_PARA3  */
	 /* name: f_PP_FIR9 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数9 */
	 /* range: [16:8] */
	 #define    b_PP_FIR9    ( 8 )
	 #define    m_PP_FIR9    ( 0x1ff << b_PP_FIR9)


	 /* name: f_PP_FIR8 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数8 */
	 /* range: [7:0] */
	 #define    b_PP_FIR8    ( 0 )
	 #define    m_PP_FIR8    ( 0xff << b_PP_FIR8)


/*  PP_HFILTER_PARA4  */
	 /* name: f_PP_FIR_BY_PASS */
	 /* desc: FIR滤波bypass控制，高电平有效 */
	 /* range: [31] */
	 #define    b_PP_FIR_BY_PASS    ( 31 )
	 #define    m_PP_FIR_BY_PASS    ( 0x1 << b_PP_FIR_BY_PASS)


	 /* name: f_PP_FIR_SIGN */
	 /* desc: 滤波系数符号位。bitx代表. PP_FIRx的符号位 */
	 /* range: [9:0] */
	 #define    b_PP_FIR_SIGN    ( 0 )
	 #define    m_PP_FIR_SIGN    ( 0x3ff << b_PP_FIR_SIGN)


/*  PP_ZOOM_PARA_H  */
	 /* name: f_PP_ZOOM_PARA_Hxx */
	 /* desc: 水平缩放相位xx系数 */
	 /* range: [31:0] */
	 #define    b_PP_ZOOM_PARA_HXX    ( 0 )
	 #define    m_PP_ZOOM_PARA_HXX    ( 0xffffffff << b_PP_ZOOM_PARA_HXX)


/*  PP_ZOOM_PARA_V  */
	 /* name: f_PP_ZOOM_PARA_Vxx */
	 /* desc: 垂直缩放相位xx系数 */
	 /* range: [31:0] */
	 #define    b_PP_ZOOM_PARA_VXX    ( 0 )
	 #define    m_PP_ZOOM_PARA_VXX    ( 0xffffffff << b_PP_ZOOM_PARA_VXX)


/*  DISP_CTRL_WORD0  */
	 /* name: f_Y_TOP_FIELD_START_ADDR */
	 /* desc: 显示控制顶场亮度起始地址指针 */
	 /* range: [31:0] */
	 #define    b_Y_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_Y_TOP_FIELD_START_ADDR    ( 0xffffffff << b_Y_TOP_FIELD_START_ADDR)


/*  DISP_CTRL_WORD1  */
	 /* name: f_Y_BOTTOM_FIELD_START_ADDR */
	 /* desc: 显示控制底场亮度起始地址指针 */
	 /* range: [31:0] */
	 #define    b_Y_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_Y_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_Y_BOTTOM_FIELD_START_ADDR)


/*  DISP_CTRL_WORD2  */
	 /* name: f_UV_TOP_FIELD_START_ADDR */
	 /* desc: 显示控制顶场色度起始地址指针 */
	 /* range: [31:0] */
	 #define    b_UV_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_UV_TOP_FIELD_START_ADDR    ( 0xffffffff << b_UV_TOP_FIELD_START_ADDR)


/*  DISP_CTRL_WORD3  */
	 /* name: f_UV_BOTTOM_FIELD_START_ADDR */
	 /* desc: 显示控制底场色度起始地址指针 */
	 /* range: [31:0] */
	 #define    b_UV_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_UV_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_UV_BOTTOM_FIELD_START_ADDR)


/*  DISP_CTRL_WORD4  */
	 /* name: f_Y_LOW2BIT_TOP_FIELD_START_ADDR */
	 /* desc: 显示控制顶场亮度起始地址指针 */
	 /* range: [31:0] */
	 #define    b_Y_LOW2BIT_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_Y_LOW2BIT_TOP_FIELD_START_ADDR    ( 0xffffffff << b_Y_LOW2BIT_TOP_FIELD_START_ADDR)


/*  DISP_CTRL_WORD5  */
	 /* name: f_Y_LOW2BIT_BOTTOM_FIELD_START_ADDR */
	 /* desc: 显示控制底场亮度起始地址指针 */
	 /* range: [31:0] */
	 #define    b_Y_LOW2BIT_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_Y_LOW2BIT_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_Y_LOW2BIT_BOTTOM_FIELD_START_ADDR)


/*  DISP_CTRL_WORD6  */
	 /* name: f_UV_LOW2BIT_TOP_FIELD_START_ADDR */
	 /* desc: 显示控制顶场色度起始地址指针 */
	 /* range: [31:0] */
	 #define    b_UV_LOW2BIT_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_UV_LOW2BIT_TOP_FIELD_START_ADDR    ( 0xffffffff << b_UV_LOW2BIT_TOP_FIELD_START_ADDR)


/*  DISP_CTRL_WORD7  */
	 /* name: f_UV_LOW2BIT_BOTTOM_FIELD_START_ADDR */
	 /* desc: 显示控制底场色度起始地址指针 */
	 /* range: [31:0] */
	 #define    b_UV_LOW2BIT_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_UV_LOW2BIT_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_UV_LOW2BIT_BOTTOM_FIELD_START_ADDR)


/*  DISP_CTRL_WORD8  */
	 /* name: f_DISP_VB_PHASE_BIAS */
	 /* desc: 显示控制底场亮度偏移相位 */
	 /* range: [30:16] */
	 #define    b_DISP_VB_PHASE_BIAS    ( 16 )
	 #define    m_DISP_VB_PHASE_BIAS    ( 0x7fff << b_DISP_VB_PHASE_BIAS)


	 /* name: f_DISP_VT_PHASE_BIAS */
	 /* desc: 显示控制顶场亮度偏移相位 */
	 /* range: [14:0] */
	 #define    b_DISP_VT_PHASE_BIAS    ( 0 )
	 #define    m_DISP_VT_PHASE_BIAS    ( 0x7fff << b_DISP_VT_PHASE_BIAS)


/*  DISP_CTRL_WORD9  */
	 /* name: f_DISP_SOURCE_STRIDE */
	 /* desc: 源数据实际占有空间宽度，需64对齐 */
	 /* range: [13:0] */
	 #define    b_DISP_SOURCE_STRIDE    ( 0 )
	 #define    m_DISP_SOURCE_STRIDE    ( 0x3fff << b_DISP_SOURCE_STRIDE)


/*  DISP_CTRL_WORD11  */
	 /* name: f_YCBCR_SEL_RIGHT */
	 /* desc: 3D输出时，YUV 右屏数据选择 */
	 /* range: [30:28] */
	 #define    b_YCBCR_SEL_RIGHT    ( 28 )
	 #define    m_YCBCR_SEL_RIGHT    ( 0x7 << b_YCBCR_SEL_RIGHT)


	 /* name: f_YCBCR_SEL_LEFT */
	 /* desc: 3D输出时，YUV 左屏数据选择 */
	 /* range: [26:24] */
	 #define    b_YCBCR_SEL_LEFT    ( 24 )
	 #define    m_YCBCR_SEL_LEFT    ( 0x7 << b_YCBCR_SEL_LEFT)


	 /* name: f_RGB_SEL_RIGH */
	 /* desc: 3D输出时，RGB 右屏数据选择 */
	 /* range: [22:20] */
	 #define    b_RGB_SEL_RIGH    ( 20 )
	 #define    m_RGB_SEL_RIGH    ( 0x7 << b_RGB_SEL_RIGH)


	 /* name: f_RGB_SEL_LEFT */
	 /* desc: 3D输出时，RGB 左屏数据选择 */
	 /* range: [18:16] */
	 #define    b_RGB_SEL_LEFT    ( 16 )
	 #define    m_RGB_SEL_LEFT    ( 0x7 << b_RGB_SEL_LEFT)


	 /* name: f_HALF_RIGHT */
	 /* desc: 3D输出时，右屏数据标识 */
	 /* range: [15] */
	 #define    b_HALF_RIGHT    ( 15 )
	 #define    m_HALF_RIGHT    ( 0x1 << b_HALF_RIGHT)


	 /* name: f_HALF_LEFT */
	 /* desc: 3D输出时，左屏数据标识 */
	 /* range: [14] */
	 #define    b_HALF_LEFT    ( 14 )
	 #define    m_HALF_LEFT    ( 0x1 << b_HALF_LEFT)


	 /* name: f_DISP_BIT_MODE */
	 /* desc: 数据源格式控制：0：8bit数据模式；1:9bit输出模式；2:10bit输出模式 */
	 /* range: [9:8] */
	 #define    b_DISP_BIT_MODE    ( 8 )
	 #define    m_DISP_BIT_MODE    ( 0x3 << b_DISP_BIT_MODE)


	 /* name: f_PHASE_SET_BY_DISP */
	 /* desc: 垂直起始相位选择控制；0:使用寄存器配置的垂直起始相位；1使用DISP控制字垂直起始相位 */
	 /* range: [4] */
	 #define    b_PHASE_SET_BY_DISP    ( 4 )
	 #define    m_PHASE_SET_BY_DISP    ( 0x1 << b_PHASE_SET_BY_DISP)


	 /* name: f_DISP_FRAME_MODE2 */
	 /* desc: 亮度低2bit播放方式控制;1:色度按场播放；0：色度按帧播放；当DISP_FRAME_MODE为0时，必须为0;此部分设计出于两部分考虑，如果是帧流的话， */
	 /* range: [3] */
	 #define    b_DISP_FRAME_MODE2    ( 3 )
	 #define    m_DISP_FRAME_MODE2    ( 0x1 << b_DISP_FRAME_MODE2)


	 /* name: f_DISP_FRAME_MODE1 */
	 /* desc: 色度播放方式控制;1:色度按场播放；0：色度按帧播放；当DISP_FRAME_MODE为0时，必须为0;此部分设计出于两部分考虑，如果是帧流的话， */
	 /* range: [2] */
	 #define    b_DISP_FRAME_MODE1    ( 2 )
	 #define    m_DISP_FRAME_MODE1    ( 0x1 << b_DISP_FRAME_MODE1)


	 /* name: f_DISP_FRAME_MODE */
	 /* desc: 亮度播放方式控制;0:亮度按场播放；1：亮度按帧播放 */
	 /* range: [1] */
	 #define    b_DISP_FRAME_MODE    ( 1 )
	 #define    m_DISP_FRAME_MODE    ( 0x1 << b_DISP_FRAME_MODE)


	 /* name: f_DISP_FIELD */
	 /* desc: 顶场数据标志；0：当前源数据为底场数据；1：当前源数据为顶场数据 */
	 /* range: [0] */
	 #define    b_DISP_FIELD    ( 0 )
	 #define    m_DISP_FIELD    ( 0x1 << b_DISP_FIELD)

#endif

