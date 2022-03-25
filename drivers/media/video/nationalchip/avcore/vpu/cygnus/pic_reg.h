#ifndef    __VPU_PIC_H__
#define    __VPU_PIC_H__

struct cygnus_vpu_pic_reg {
	unsigned rPIC_ENABLE; //0x0
	unsigned rPIC_V_PHASE; //0x4
	unsigned rPIC_POSITION; //0x8
	unsigned rPIC_SOURCE_SIZE; //0xc
	unsigned rPIC_VIEW_SIZE; //0x10
	unsigned rPIC_ZOOM; //0x14
	unsigned rPIC_FRAME_PARA; //0x18
	unsigned rPIC_ZOOM_FILTER_SIGN; //0x1c
	unsigned rPIC_PHASE0_H; //0x20
	unsigned rPIC_PHASE0_V; //0x24
	unsigned rPIC_Y_TOP_ADDR; //0x28
	unsigned rPIC_Y_BOTTOM_ADDR; //0x2c
	unsigned rPIC_UV_TOP_ADDR; //0x30
	unsigned rPIC_UV_BOTTOM_ADDR; //0x34
	unsigned rPIC_BACKCOLOR; //0x38
	unsigned rPIC_CTRL; //0x3c
	unsigned rRESV0[ 2 ];
	unsigned rPIC_INT_EMPTY_GATE; //0x48
	unsigned rPIC_BUFFER_INT; //0x4c
	unsigned rRESV1[ 2 ];
	unsigned rPIC_HFILTER_PARA1; //0x58
	unsigned rPIC_HFILTER_PARA2; //0x5c
	unsigned rPIC_HFILTER_PARA3; //0x60
	unsigned rPIC_HFILTER_PARA4; //0x64
	unsigned rRESV2[ 38 ];
	unsigned rPIC_ZOOM_PARA_H[64]; //0x100
	unsigned rPIC_ZOOM_PARA_V[64]; //0x200
};

/*  PIC_ENABLE  */
	 /* name: f_PIC_EN */
	 /* desc: 图层显示使能信号，高电平有效 */
	 /* range: [0] */
	 #define    b_PIC_EN    ( 0 )
	 #define    m_PIC_EN    ( 0x1 << b_PIC_EN)


/*  PIC_V_PHASE  */
	 /* name: f_PIC_VB_PHASE_BIASE */
	 /* desc: 层显示底场输出起始相位，相对于源数据，4096单位为一行 */
	 /* range: [30:16] */
	 #define    b_PIC_VB_PHASE_BIASE    ( 16 )
	 #define    m_PIC_VB_PHASE_BIASE    ( 0x7fff << b_PIC_VB_PHASE_BIASE)


	 /* name: f_PIC_VT_PHASE_BIASE */
	 /* desc: 层显示顶场输出起始相位，相对于源数据，4096单位为一行 */
	 /* range: [14:0] */
	 #define    b_PIC_VT_PHASE_BIASE    ( 0 )
	 #define    m_PIC_VT_PHASE_BIASE    ( 0x7fff << b_PIC_VT_PHASE_BIASE)


/*  PIC_POSITION  */
	 /* name: f_PIC_START_Y */
	 /* desc: 层显示垂直方向起始行 */
	 /* range: [26:16] */
	 #define    b_PIC_START_Y    ( 16 )
	 #define    m_PIC_START_Y    ( 0x7ff << b_PIC_START_Y)


	 /* name: f_PIC_START_X */
	 /* desc: 层显示水平方向起始像素 */
	 /* range: [10:0] */
	 #define    b_PIC_START_X    ( 0 )
	 #define    m_PIC_START_X    ( 0x7ff << b_PIC_START_X)


/*  PIC_SOURCE_SIZE  */
	 /* name: f_PIC_REQ_HEIGHT */
	 /* desc: 源数据高度，以行为单位 */
	 /* range: [26:16] */
	 #define    b_PIC_REQ_HEIGHT    ( 16 )
	 #define    m_PIC_REQ_HEIGHT    ( 0x7ff << b_PIC_REQ_HEIGHT)


	 /* name: f_PIC_REQ_LENGTH */
	 /* desc: 源数据宽度，以像素为单位 */
	 /* range: [11:0] */
	 #define    b_PIC_REQ_LENGTH    ( 0 )
	 #define    m_PIC_REQ_LENGTH    ( 0xfff << b_PIC_REQ_LENGTH)


/*  PIC_VIEW_SIZE  */
	 /* name: f_PIC_VIEW_HEIGHT */
	 /* desc: 缩放输出数据高度，以行为单位 */
	 /* range: [26:16] */
	 #define    b_PIC_VIEW_HEIGHT    ( 16 )
	 #define    m_PIC_VIEW_HEIGHT    ( 0x7ff << b_PIC_VIEW_HEIGHT)


	 /* name: f_PIC_VIEW_LENGTH */
	 /* desc: 缩放输出数据宽度，以像素为单位 */
	 /* range: [10:0] */
	 #define    b_PIC_VIEW_LENGTH    ( 0 )
	 #define    m_PIC_VIEW_LENGTH    ( 0x7ff << b_PIC_VIEW_LENGTH)


/*  PIC_ZOOM  */
	 /* name: f_PIC_V_ZOOM */
	 /* desc: 垂直方向缩放系数，计算公式为：源高度*4096/目标高度，取整 */
	 /* range: [30:16] */
	 #define    b_PIC_V_ZOOM    ( 16 )
	 #define    m_PIC_V_ZOOM    ( 0x7fff << b_PIC_V_ZOOM)


	 /* name: f_PIC_H_ZOOM */
	 /* desc: 水平方向缩放系数，计算公式为：源宽度*4096/目标宽度，取整 */
	 /* range: [14:0] */
	 #define    b_PIC_H_ZOOM    ( 0 )
	 #define    m_PIC_H_ZOOM    ( 0x7fff << b_PIC_H_ZOOM)


/*  PIC_FRAME_PARA  */
	 /* name: f_PIC_MODE */
	 /* desc: 层数据类型；0: YCbCr4:2:0，半交织；1: YCbCr4:2:2，半交织；2：YCbCr4:2:2，全交织；3:YCbCrA 6442 */
	 /* range: [31:30] */
	 #define    b_PIC_MODE    ( 30 )
	 #define    m_PIC_MODE    ( 0x3 << b_PIC_MODE)


	 /* name: f_FRAME_MODE_PIC */
	 /* desc: 层播放模式；1：按帧播放；0按场播放 */
	 /* range: [29] */
	 #define    b_FRAME_MODE_PIC    ( 29 )
	 #define    m_FRAME_MODE_PIC    ( 0x1 << b_FRAME_MODE_PIC)


	 /* name: f_PIC_MODE_ALPHA_FILKER */
	 /* desc: 层alpha滤波使能，高电平有效 */
	 /* range: [28] */
	 #define    b_PIC_MODE_ALPHA_FILKER    ( 28 )
	 #define    m_PIC_MODE_ALPHA_FILKER    ( 0x1 << b_PIC_MODE_ALPHA_FILKER)


	 /* name: f_PIC_BUFF_LEN */
	 /* desc: 层数据申请单次申请长度，为提供总线效率，为128整数倍 */
	 /* range: [26:16] */
	 #define    b_PIC_BUFF_LEN    ( 16 )
	 #define    m_PIC_BUFF_LEN    ( 0x7ff << b_PIC_BUFF_LEN)


	 /* name: f_PIC_FRAME_STRIDE */
	 /* desc: 层读取完一行数据后，跨多少像素读取下一行。为源数据宽度的整数倍 */
	 /* range: [12:0] */
	 #define    b_PIC_FRAME_STRIDE    ( 0 )
	 #define    m_PIC_FRAME_STRIDE    ( 0x1fff << b_PIC_FRAME_STRIDE)


/*  PIC_ZOOM_FILTER_SIGN  */
	 /* name: f_PIC_ENDIAN_MODE*/
	 /* desc: 层数据排列字节序,例如像素数据从左到右连续8个像素编号分辨为B0、B1、B2、B3、B4、B5、B6、B7；0:排序为B0、B1、B2、B3、B4、B5、B6、B7；1:排序为B7、B6、B5、B4、B3、B2、B1、B0；2:排序为B4、B5、B6、B7、B0、B1、B2、B3；3:排序为B3、B2、B1、B0、B7、B6、B5、B4；4:排序为B1、B0、B3、B2、B5、B4、B7、B6；5:排序为B6、B7、B4、B5、B2、B3、B1、B0；6:排序为B5、B4、B7、B6、B1、B0、B3、B2；7:排序为B3、B2、B0、B1、B5、B4、B7、B6 */
	 /* range: [26:24] */
	 #define    b_PIC_ENDIAN_MODE    ( 24 )
	 #define    m_PIC_ENDIAN_MODE    ( 0x7 << b_PIC_ENDIAN_MODE)


	 /* name: f_PIC_PHASE0_V_PN */
	 /* desc: 垂直滤波初始滤波系数符号，使用CUBIC滤波系数时配置为4’b1001 */
	 /* range: [23:20] */
	 #define    b_PIC_PHASE0_V_PN    ( 20 )
	 #define    m_PIC_PHASE0_V_PN    ( 0xf << b_PIC_PHASE0_V_PN)


	 /* name: f_PIC_PHASE0_H_PN */
	 /* desc: 水平滤波初始滤波系数符号，使用CUBIC滤波系数时配置为4’b1001 */
	 /* range: [19:16] */
	 #define    b_PIC_PHASE0_H_PN    ( 16 )
	 #define    m_PIC_PHASE0_H_PN    ( 0xf << b_PIC_PHASE0_H_PN)


	 /* name: f_PIC_V_PN_CHG_POS */
	 /* desc: 垂直滤波系数负正符号变更位置，使用CUBIC滤波系数时配置为8’d128 */
	 /* range: [15:8] */
	 #define    b_PIC_V_PN_CHG_POS    ( 8 )
	 #define    m_PIC_V_PN_CHG_POS    ( 0xff << b_PIC_V_PN_CHG_POS)


	 /* name: f_PIC_H_PN_CHG_POS */
	 /* desc: 水平滤波系数负正符号变更位置，使用CUBIC滤波系数时配置为8’d128 */
	 /* range: [7:0] */
	 #define    b_PIC_H_PN_CHG_POS    ( 0 )
	 #define    m_PIC_H_PN_CHG_POS    ( 0xff << b_PIC_H_PN_CHG_POS)


/*  PIC_PHASE0_H  */
	 /* name: f_PIC_PHASE_0_H */
	 /* desc: 水平缩放0相位值，和PIC_ZOOM_PARA_H参数配套使用 */
	 /* range: [31:0] */
	 #define    b_PIC_PHASE_0_H    ( 0 )
	 #define    m_PIC_PHASE_0_H    ( 0xffffffff << b_PIC_PHASE_0_H)


/*  PIC_PHASE0_V  */
	 /* name: f_PIC_PHASE_0_V */
	 /* desc: 垂直缩放0相位值，和PIC_ZOOM_PARA_V参数配套使用 */
	 /* range: [31:0] */
	 #define    b_PIC_PHASE_0_V    ( 0 )
	 #define    m_PIC_PHASE_0_V    ( 0xffffffff << b_PIC_PHASE_0_V)


/*  PIC_Y_TOP_ADDR  */
	 /* name: f_PIC_Y_TOP_FIELD_START_ADDR */
	 /* desc: 层顶场亮度数据起始地址指针 */
	 /* range: [31:0] */
	 #define    b_PIC_Y_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_PIC_Y_TOP_FIELD_START_ADDR    ( 0xffffffff << b_PIC_Y_TOP_FIELD_START_ADDR)


/*  PIC_Y_BOTTOM_ADDR  */
	 /* name: f_PIC_Y_BOTTOM_FIELD_START_ADDR */
	 /* desc: 层低场亮度数据起始地址指针 */
	 /* range: [31:0] */
	 #define    b_PIC_Y_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_PIC_Y_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_PIC_Y_BOTTOM_FIELD_START_ADDR)


/*  PIC_UV_TOP_ADDR  */
	 /* name: f_PIC_UV_TOP_FIELD_START_ADDR */
	 /* desc: 层顶场色度数据起始地址指针 */
	 /* range: [31:0] */
	 #define    b_PIC_UV_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_PIC_UV_TOP_FIELD_START_ADDR    ( 0xffffffff << b_PIC_UV_TOP_FIELD_START_ADDR)


/*  PIC_UV_BOTTOM_ADDR  */
	 /* name: f_PIC_UV_BOTTOM_FIELD_START_ADDR */
	 /* desc: 层低场色度数据起始地址指针 */
	 /* range: [31:0] */
	 #define    b_PIC_UV_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_PIC_UV_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_PIC_UV_BOTTOM_FIELD_START_ADDR)


/*  PIC_BACKCOLOR  */
	 /* name: f_PIC_ALPHA */
	 /* desc: 层透明度控制，0为全透明，255为不透明 */
	 /* range: [31:24] */
	 #define    b_PIC_ALPHA    ( 24 )
	 #define    m_PIC_ALPHA    ( 0xff << b_PIC_ALPHA)


	 /* name: f_PIC_BACK_COLOR */
	 /* desc: 层背景色控制；23:16 Y值；15:8 CB值； 7:0 Cr值 */
	 /* range: [23:0] */
	 #define    b_PIC_BACK_COLOR    ( 0 )
	 #define    m_PIC_BACK_COLOR    ( 0xffffff << b_PIC_BACK_COLOR)


/*  PIC_CTRL  */
	 /* name: f_PIC_H_PHASE_BIAS */
	 /* desc: 水平相位计数器起始相位。两个像素间划分了4096个相位 */
	 /* range: [30:16] */
	 #define    b_PIC_H_PHASE_BIAS    ( 16 )
	 #define    m_PIC_H_PHASE_BIAS    ( 0x7fff << b_PIC_H_PHASE_BIAS)


	 /* name: f_PIC_UV_H_FILTER_EN */
	 /* desc: UV水平滤波使能控制，高电平有效 */
	 /* range: [12] */
	 #define    b_PIC_UV_H_FILTER_EN    ( 12 )
	 #define    m_PIC_UV_H_FILTER_EN    ( 0x1 << b_PIC_UV_H_FILTER_EN)


	 /* name: f_PIC_UV_MODE */
	 /* desc: UV升采样滤波计算方式;3:水平升UV为四点插值;2: 均值;1: 重复;0: 线性滤波 */
	 /* range: [7:6] */
	 #define    b_PIC_UV_MODE    ( 6 )
	 #define    m_PIC_UV_MODE    ( 0x3 << b_PIC_UV_MODE)


	 /* name: f_PIC_Y_ZOOM_MODE_V */
	 /* desc: 垂直缩放模式；当VZOOM为4096时，必须为1；当VZOOM非4096时，必须为0 */
	 /* range: [5] */
	 #define    b_PIC_Y_ZOOM_MODE_V    ( 5 )
	 #define    m_PIC_Y_ZOOM_MODE_V    ( 0x1 << b_PIC_Y_ZOOM_MODE_V)


	 /* name: f_PIC_Y_ZOOM_MODE_H */
	 /* desc: 水平缩放模式；当HZOOM为4096时，必须为1；当HZOOM非4096时，必须为0 */
	 /* range: [4] */
	 #define    b_PIC_Y_ZOOM_MODE_H    ( 4 )
	 #define    m_PIC_Y_ZOOM_MODE_H    ( 0x1 << b_PIC_Y_ZOOM_MODE_H)


	 /* name: f_PIC_H_DOWN_SCALE */
	 /* desc: 水平两倍降抽样使能，高电平有效 */
	 /* range: [3] */
	 #define    b_PIC_H_DOWN_SCALE    ( 3 )
	 #define    m_PIC_H_DOWN_SCALE    ( 0x1 << b_PIC_H_DOWN_SCALE)


	 /* name: f_PIC_V_DOWN_SCALE */
	 /* desc: 垂直两倍降抽样使能，高电平有效 */
	 /* range: [2] */
	 #define    b_PIC_V_DOWN_SCALE    ( 2 )
	 #define    m_PIC_V_DOWN_SCALE    ( 0x1 << b_PIC_V_DOWN_SCALE)


/*  PIC_INT_EMPTY_GATE  */
	 /* name: f_INT_PIC_EMPTY_GATE */
	 /* desc: 行buffer数据存储门限警报标志，当存储不足配置值时，发起中断,最大值为240 */
	 /* range: [11:0] */
	 #define    b_INT_PIC_EMPTY_GATE    ( 0 )
	 #define    m_INT_PIC_EMPTY_GATE    ( 0xfff << b_INT_PIC_EMPTY_GATE)


/*  PIC_BUFFER_INT  */
	 /* name: f_INT_PIC_IDLE_EN */
	 /* desc: 层行关闭完成中断使能 */
	 /* range: [18] */
	 #define    b_INT_PIC_IDLE_EN    ( 18 )
	 #define    m_INT_PIC_IDLE_EN    ( 0x1 << b_INT_PIC_IDLE_EN)


	 /* name: f_INT_PIC_EMPTY_EN */
	 /* desc: 层行buffer读空中断使能 */
	 /* range: [17] */
	 #define    b_INT_PIC_EMPTY_EN    ( 17 )
	 #define    m_INT_PIC_EMPTY_EN    ( 0x1 << b_INT_PIC_EMPTY_EN)


	 /* name: f_INT_PIC_ALMOST_EN */
	 /* desc: 层行buffer将要读空中断使能 */
	 /* range: [16] */
	 #define    b_INT_PIC_ALMOST_EN    ( 16 )
	 #define    m_INT_PIC_ALMOST_EN    ( 0x1 << b_INT_PIC_ALMOST_EN)


	 /* name: f_INT_PIC_IDLE */
	 /* desc: 层行关闭完成中断使能状态，写1清0 */
	 /* range: [2] */
	 #define    b_INT_PIC_IDLE    ( 2 )
	 #define    m_INT_PIC_IDLE    ( 0x1 << b_INT_PIC_IDLE)


	 /* name: f_INT_PIC_EMPTY */
	 /* desc: 层行buffer读空中断状态，写1清0 */
	 /* range: [1] */
	 #define    b_INT_PIC_EMPTY    ( 1 )
	 #define    m_INT_PIC_EMPTY    ( 0x1 << b_INT_PIC_EMPTY)


	 /* name: f_INT_PIC_ALMOST */
	 /* desc: 层行buffer将要读空中断状态，写1清0 */
	 /* range: [0] */
	 #define    b_INT_PIC_ALMOST    ( 0 )
	 #define    m_INT_PIC_ALMOST    ( 0x1 << b_INT_PIC_ALMOST)


/*  PIC_HFILTER_PARA1  */
	 /* name: f_PIC_FIR3 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数3 */
	 /* range: [31:24] */
	 #define    b_PIC_FIR3    ( 24 )
	 #define    m_PIC_FIR3    ( 0xff << b_PIC_FIR3)


	 /* name: f_PIC_FIR2 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数2 */
	 /* range: [23:16] */
	 #define    b_PIC_FIR2    ( 16 )
	 #define    m_PIC_FIR2    ( 0xff << b_PIC_FIR2)


	 /* name: f_PIC_FIR1 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数1 */
	 /* range: [15:8] */
	 #define    b_PIC_FIR1    ( 8 )
	 #define    m_PIC_FIR1    ( 0xff << b_PIC_FIR1)


	 /* name: f_PIC_FIR0 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数0 */
	 /* range: [7:0] */
	 #define    b_PIC_FIR0    ( 0 )
	 #define    m_PIC_FIR0    ( 0xff << b_PIC_FIR0)


/*  PIC_HFILTER_PARA2  */
	 /* name: f_PIC_FIR7 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数7 */
	 /* range: [31:24] */
	 #define    b_PIC_FIR7    ( 24 )
	 #define    m_PIC_FIR7    ( 0xff << b_PIC_FIR7)


	 /* name: f_PIC_FIR6 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数6 */
	 /* range: [23:16] */
	 #define    b_PIC_FIR6    ( 16 )
	 #define    m_PIC_FIR6    ( 0xff << b_PIC_FIR6)


	 /* name: f_PIC_FIR5 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数5 */
	 /* range: [15:8] */
	 #define    b_PIC_FIR5    ( 8 )
	 #define    m_PIC_FIR5    ( 0xff << b_PIC_FIR5)


	 /* name: f_PIC_FIR4 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数4 */
	 /* range: [7:0] */
	 #define    b_PIC_FIR4    ( 0 )
	 #define    m_PIC_FIR4    ( 0xff << b_PIC_FIR4)


/*  PIC_HFILTER_PARA3  */
	 /* name: f_PIC_FIR9 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数9 */
	 /* range: [16:8] */
	 #define    b_PIC_FIR9    ( 8 )
	 #define    m_PIC_FIR9    ( 0x1ff << b_PIC_FIR9)


	 /* name: f_PIC_FIR8 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数8 */
	 /* range: [7:0] */
	 #define    b_PIC_FIR8    ( 0 )
	 #define    m_PIC_FIR8    ( 0xff << b_PIC_FIR8)


/*  PIC_HFILTER_PARA4  */
	 /* name: f_PIC_FIR_BY_PASS */
	 /* desc: FIR滤波bypass控制，高电平有效 */
	 /* range: [31] */
	 #define    b_PIC_FIR_BY_PASS    ( 31 )
	 #define    m_PIC_FIR_BY_PASS    ( 0x1 << b_PIC_FIR_BY_PASS)


	 /* name: f_PIC_FIR_SIGN */
	 /* desc: 滤波系数符号位。bitx代表. PP_FIRx的符号位 */
	 /* range: [9:0] */
	 #define    b_PIC_FIR_SIGN    ( 0 )
	 #define    m_PIC_FIR_SIGN    ( 0x3ff << b_PIC_FIR_SIGN)


/*  PIC_ZOOM_PARA_H  */
	 /* name: f_PIC_ZOOM_PARA_H */
	 /* desc: 水平缩放相位系数 */
	 /* range: [31:0] */
	 #define    b_PIC_ZOOM_PARA_H    ( 0 )
	 #define    m_PIC_ZOOM_PARA_H    ( 0xffffffff << b_PIC_ZOOM_PARA_H)


/*  PIC_ZOOM_PARA_V  */
	 /* name: f_PIC_ZOOM_PARA_Vxx */
	 /* desc: 垂直缩放相位xx系数 */
	 /* range: [31:0] */
	 #define    b_PIC_ZOOM_PARA_V    ( 0 )
	 #define    m_PIC_ZOOM_PARA_V    ( 0xffffffff << b_PIC_ZOOM_PARA_V)


#endif

