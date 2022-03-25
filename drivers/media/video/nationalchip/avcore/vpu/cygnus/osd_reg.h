#ifndef    __VPU_OSD_H__
#define    __VPU_OSD_H__

struct cygnus_vpu_osd_reg {
	unsigned rOSD_ENABLE; //0x0
	unsigned rOSD_FIRST_HEAD_PTR; //0x4
	unsigned rOSD_VIEW_SIZE; //0x8
	unsigned rOSD_ZOOM; //0xc
	unsigned rOSD_COLOR_KEY; //0x10
	unsigned rOSD_COLORKEY_CTROL; //0x14
	unsigned rOSD_ALPHA_5551; //0x18
	unsigned rOSD_ZOOM_FILTER_SIGN; //0x1c
	unsigned rOSD_POSITION; //0x20
	unsigned rOSD_FRAME_PARA; //0x24
	unsigned rOSD_ZOOM_PHASE0_H; //0x28
	unsigned rOSD_ZOOM_PHASE0_V; //0x2c
	unsigned rOSD_PHASE_BIAS; //0x30
	unsigned rRESV0[ 1 ];
	unsigned rOSD_CTRL; //0x38
	unsigned rRESV1[ 2 ];
	unsigned rOSD_INT_EMPTY_GATE; //0x44
	unsigned rOSD_BUFFER_INT; //0x48
	unsigned rRESV2[ 2 ];
	unsigned rOSD_HFILTER_PARA1; //0x54
	unsigned rOSD_HFILTER_PARA2; //0x58
	unsigned rOSD_HFILTER_PARA3; //0x5c
	unsigned rOSD_HFILTER_PARA4; //0x60
	unsigned rRESV3[ 39 ];
	unsigned rOSD_ZOOM_PARA[64]; //0x100
};

/*  OSD_ENABLE  */
	 /* name: f_OSD_VISIBLE */
	 /* desc: 图层显示使能信号，高电平有效 */
	 /* range: [0] */
	 #define    b_OSD_VISIBLE    ( 0 )
	 #define    m_OSD_VISIBLE    ( 0x1 << b_OSD_VISIBLE)


/*  OSD_FIRST_HEAD_PTR  */
	 /* name: f_OSD_FIRST_HEAD_PTR */
	 /* desc: 首个REGION起始地址指针 */
	 /* range: [31:0] */
	 #define    b_OSD_FIRST_HEAD_PTR    ( 0 )
	 #define    m_OSD_FIRST_HEAD_PTR    ( 0xffffffff << b_OSD_FIRST_HEAD_PTR)


/*  OSD_VIEW_SIZE  */
	 /* name: f_OSD_HEIGHT */
	 /* desc: 缩放输出数据高度，以行为单位 */
	 /* range: [26:16] */
	 #define    b_OSD_HEIGHT    ( 16 )
	 #define    m_OSD_HEIGHT    ( 0x7ff << b_OSD_HEIGHT)


	 /* name: f_OSD_LENGTH */
	 /* desc: 缩放输出数据宽度，以像素为单位 */
	 /* range: [10:0] */
	 #define    b_OSD_LENGTH    ( 0 )
	 #define    m_OSD_LENGTH    ( 0x7ff << b_OSD_LENGTH)


/*  OSD_ZOOM  */
	 /* name: f_OSD_V_ZOOM */
	 /* desc: 垂直方向缩放系数，计算公式为：源高度*4096/目标高度，取整 */
	 /* range: [29:16] */
	 #define    b_OSD_V_ZOOM    ( 16 )
	 #define    m_OSD_V_ZOOM    ( 0x3fff << b_OSD_V_ZOOM)


	 /* name: f_OSD_H_ZOOM */
	 /* desc: 水平方向缩放系数，计算公式为：源宽度*4096/目标宽度，取整 */
	 /* range: [13:0] */
	 #define    b_OSD_H_ZOOM    ( 0 )
	 #define    m_OSD_H_ZOOM    ( 0x3fff << b_OSD_H_ZOOM)


/*  OSD_COLOR_KEY  */
	 /* name: f_OSD_COLORKEY */
	 /* desc: ColorKey值，其中功能分配分别是高24bit为colorkey色，Bit[31:24]：R（Y）、Bit[23:16]：G（U)、Bit[15:8]：B(V)，低8bit Bit[7:0]是colorkey ALPHA值 */
	 /* range: [31:0] */
	 #define    b_OSD_COLORKEY    ( 0 )
	 #define    m_OSD_COLORKEY    ( 0xffffffff << b_OSD_COLORKEY)


/*  OSD_COLORKEY_CTROL  */
	 /* name: f_OSD_COLORKEY_MODE */
	 /* desc: ColorKey模式控制;0:只比较colorkey色，匹配上时采用寄存器OSD_COLOR_KEY中的ALPHA值；1：比较整个colorkey，全部匹配上时，透明度采用寄存器OSD_COLORKEY_ALPHA */
	 /* range: [8] */
	 #define    b_OSD_COLORKEY_MODE    ( 8 )
	 #define    m_OSD_COLORKEY_MODE    ( 0x1 << b_OSD_COLORKEY_MODE)


	 /* name: f_OSD_COLORKEY_ALPHA */
	 /* desc: ColorKey透明度,仅仅当OSD_COLORKEY_MODE为高电平有效，当OSD_COLORKEY_MODE为低电平时，COLORKEY透明度采用寄存器OSD_COLOR_KEY配置的ALPHA值 */
	 /* range: [7:0] */
	 #define    b_OSD_COLORKEY_ALPHA    ( 0 )
	 #define    m_OSD_COLORKEY_ALPHA    ( 0xff << b_OSD_COLORKEY_ALPHA)


/*  OSD_ALPHA_5551  */
	 /* name: f_OSD_COLORKEY_ALPHA_EN */
	 /* desc:  */
	 /* range: [31] */
	 #define    b_OSD_COLORKEY_ALPHA_EN    ( 31 )
	 #define    m_OSD_COLORKEY_ALPHA_EN    ( 0x1 << b_OSD_COLORKEY_ALPHA_EN)


	 /* name: f_OSD_ALPHA_REG1 */
	 /* desc: RGBA5551格式A等于1时alpha值 */
	 /* range: [15:8] */
	 #define    b_OSD_ALPHA_REG1    ( 8 )
	 #define    m_OSD_ALPHA_REG1    ( 0xff << b_OSD_ALPHA_REG1)


	 /* name: f_OSD_ALPHA_REG0 */
	 /* desc: RGBA5551格式A等于0时alpha值 */
	 /* range: [7:0] */
	 #define    b_OSD_ALPHA_REG0    ( 0 )
	 #define    m_OSD_ALPHA_REG0    ( 0xff << b_OSD_ALPHA_REG0)


/*  OSD_ZOOM_FILTER_SIGN  */
	 /* name: f_OSD_PHASE0_V_PN */
	 /* desc: 垂直滤波初始滤波系数符号，使用CUBIC滤波系数时配置为4’b1001 */
	 /* range: [23:20] */
	 #define    b_OSD_PHASE0_V_PN    ( 20 )
	 #define    m_OSD_PHASE0_V_PN    ( 0xf << b_OSD_PHASE0_V_PN)


	 /* name: f_OSD_PHASE0_H_PN */
	 /* desc: 水平滤波初始滤波系数符号，使用CUBIC滤波系数时配置为4’b1001 */
	 /* range: [19:16] */
	 #define    b_OSD_PHASE0_H_PN    ( 16 )
	 #define    m_OSD_PHASE0_H_PN    ( 0xf << b_OSD_PHASE0_H_PN)


	 /* name: f_OSD_V_PN_CHG_POS */
	 /* desc: 垂直滤波系数负正符号变更位置，使用CUBIC滤波系数时配置为8’d128 */
	 /* range: [15:8] */
	 #define    b_OSD_V_PN_CHG_POS    ( 8 )
	 #define    m_OSD_V_PN_CHG_POS    ( 0xff << b_OSD_V_PN_CHG_POS)


	 /* name: f_OSD_H_PN_CHG_POS */
	 /* desc: 水平滤波系数负正符号变更位置，使用CUBIC滤波系数时配置为8’d128 */
	 /* range: [7:0] */
	 #define    b_OSD_H_PN_CHG_POS    ( 0 )
	 #define    m_OSD_H_PN_CHG_POS    ( 0xff << b_OSD_H_PN_CHG_POS)


/*  OSD_POSITION  */
	 /* name: f_START_Y_OSD */
	 /* desc: 层显示垂直方向起始行 */
	 /* range: [26:16] */
	 #define    b_START_Y_OSD    ( 16 )
	 #define    m_START_Y_OSD    ( 0x7ff << b_START_Y_OSD)


	 /* name: f_START_X_OSD */
	 /* desc: 层显示水平方向起始像素 */
	 /* range: [10:0] */
	 #define    b_START_X_OSD    ( 0 )
	 #define    m_START_X_OSD    ( 0x7ff << b_START_X_OSD)


/*  OSD_FRAME_PARA  */
	 /* name: f_OSD_ENDIAN_MODE */
	 /* desc: 层Region数据排列字节序,例如像素数据从左到右连续8个像素编号分辨为B0、B1、B2、B3、B4、B5、B6、B7；0:排序为B0、B1、B2、B3、B4、B5、B6、B7；1:排序为B7、B6、B5、B4、B3、B2、B1、B0；2:排序为B4、B5、B6、B7、B0、B1、B2、B3；3:排序为B3、B2、B1、B0、B7、B6、B5、B4；4:排序为B1、B0、B3、B2、B5、B4、B7、B6；5:排序为B6、B7、B4、B5、B2、B3、B1、B0；6:排序为B5、B4、B7、B6、B1、B0、B3、B2；7:排序为B3、B2、B0、B1、B5、B4、B7、B6 */
	 /* range: [22:20] */
	 #define    b_OSD_ENDIAN_MODE    ( 20 )
	 #define    m_OSD_ENDIAN_MODE    ( 0x7 << b_OSD_ENDIAN_MODE)


	 /* name: f_OSD_ENDIAN_MODE_HEAD */
	 /* desc: 层Region头数据排列字节序,例如像素数据从左到右连续8个像素编号分辨为B0、B1、B2、B3、B4、B5、B6、B7；0:排序为B0、B1、B2、B3、B4、B5、B6、B7；1:排序为B7、B6、B5、B4、B3、B2、B1、B0；2:排序为B4、B5、B6、B7、B0、B1、B2、B3；3:排序为B3、B2、B1、B0、B7、B6、B5、B4；4:排序为B1、B0、B3、B2、B5、B4、B7、B6；5:排序为B6、B7、B4、B5、B2、B3、B1、B0；6:排序为B5、B4、B7、B6、B1、B0、B3、B2；7:排序为B3、B2、B0、B1、B5、B4、B7、B6 */
	 /* range: [14:12] */
	 #define    b_OSD_ENDIAN_MODE_HEAD    ( 12 )
	 #define    m_OSD_ENDIAN_MODE_HEAD    ( 0x7 << b_OSD_ENDIAN_MODE_HEAD)


	 /* name: f_OSD_BUFF_LEN */
	 /* desc: 层行bufer当次设计数据长度 */
	 /* range: [10:0] */
	 #define    b_OSD_BUFF_LEN    ( 0 )
	 #define    m_OSD_BUFF_LEN    ( 0x7ff << b_OSD_BUFF_LEN)


/*  OSD_ZOOM_PHASE0_H  */
	 /* name: f_OSD_PHASE_0_H */
	 /* desc: 水平缩放0相位值，和OSD_ZOOM_PARA_H参数配套使用 */
	 /* range: [31:0] */
	 #define    b_OSD_PHASE_0_H    ( 0 )
	 #define    m_OSD_PHASE_0_H    ( 0xffffffff << b_OSD_PHASE_0_H)


/*  OSD_ZOOM_PHASE0_V  */
	 /* name: f_OSD_PHASE_0_V */
	 /* desc: 垂直缩放0相位值，和OSD_ZOOM_PARA_V参数配套使用 */
	 /* range: [31:0] */
	 #define    b_OSD_PHASE_0_V    ( 0 )
	 #define    m_OSD_PHASE_0_V    ( 0xffffffff << b_OSD_PHASE_0_V)


/*  OSD_PHASE_BIAS  */
	 /* name: f_OSD_V_PHASE_BIAS */
	 /* desc: 层显示垂直方向起始相位，相对于源数据，4096单位为一行 */
	 /* range: [27:16] */
	 #define    b_OSD_V_PHASE_BIAS    ( 16 )
	 #define    m_OSD_V_PHASE_BIAS    ( 0xfff << b_OSD_V_PHASE_BIAS)


	 /* name: f_OSD_H_PHASE_BIAS */
	 /* desc: 层显示水平方向起始相位，相对于源数据，4096单位为一行 */
	 /* range: [11:0] */
	 #define    b_OSD_H_PHASE_BIAS    ( 0 )
	 #define    m_OSD_H_PHASE_BIAS    ( 0xfff << b_OSD_H_PHASE_BIAS)


/*  OSD_CTRL  */
	 /* name: f_OSD_ZOOM_MODE */
	 /* desc: 1:0相位时，不做运算，直接采用原数据；0:0相位时，采用配置的0相位运算系数进行运算 */
	 /* range: [31] */
	 #define    b_OSD_ZOOM_MODE    ( 31 )
	 #define    m_OSD_ZOOM_MODE    ( 0x1 << b_OSD_ZOOM_MODE)


	 /* name: f_OSD_ALPHA_FLITER_EN */
	 /* desc: alpha数据缩放处理方式控制；0：采用取最近值法。1:采用滤波运算方法 */
	 /* range: [30] */
	 #define    b_OSD_ALPHA_FLITER_EN    ( 30 )
	 #define    m_OSD_ALPHA_FLITER_EN    ( 0x1 << b_OSD_ALPHA_FLITER_EN)


	 /* name: f_OSD_H_DOWNSCALE */
	 /* desc: 层水平降抽样使能控制，高电平有效 */
	 /* range: [28] */
	 #define    b_OSD_H_DOWNSCALE    ( 28 )
	 #define    m_OSD_H_DOWNSCALE    ( 0x1 << b_OSD_H_DOWNSCALE)


	 /* name: f_OSD_ZOOM_MODE_EN_IPS */
	 /* desc: 层缩放模式标识，当OSD_H_DOWNSCALE等1且大于8bpp时，必须为1 */
	 /* range: [25] */
	 #define    b_OSD_ZOOM_MODE_EN_IPS    ( 25 )
	 #define    m_OSD_ZOOM_MODE_EN_IPS    ( 0x1 << b_OSD_ZOOM_MODE_EN_IPS)


	 /* name: f_OSD_FIELD_RST_EN */
	 /* desc: 层场复位控制使能，高电平有效 */
	 /* range: [24] */
	 #define    b_OSD_FIELD_RST_EN    ( 24 )
	 #define    m_OSD_FIELD_RST_EN    ( 0x1 << b_OSD_FIELD_RST_EN)


	 /* name: f_OSD_REGION_MODE */
	 /* desc: 层多region模式使能，高电平有效 */
	 /* range: [23] */
	 #define    b_OSD_REGION_MODE    ( 23 )
	 #define    m_OSD_REGION_MODE    ( 0x1 << b_OSD_REGION_MODE)


	 /* name: f_OSD_V_TOP_PHASE */
	 /* desc: 层垂直起始相位 */
	 /* range: [21:8] */
	 #define    b_OSD_V_TOP_PHASE    ( 8 )
	 #define    m_OSD_V_TOP_PHASE    ( 0x3fff << b_OSD_V_TOP_PHASE)


/*  OSD_INT_EMPTY_GATE  */
	 /* name: f_INT_OSD_EMPTY_GATE */
	 /* desc: 行buffer数据存储门限警报标志，当存储不足配置值时，发起中断,最大值为960 */
	 /* range: [11:0] */
	 #define    b_INT_OSD_EMPTY_GATE    ( 0 )
	 #define    m_INT_OSD_EMPTY_GATE    ( 0xfff << b_INT_OSD_EMPTY_GATE)


/*  OSD_BUFFER_INT  */
	 /* name: f_INT_OSD_IDLE_EN */
	 /* desc: 层行关闭完成中断使能 */
	 /* range: [18] */
	 #define    b_INT_OSD_IDLE_EN    ( 18 )
	 #define    m_INT_OSD_IDLE_EN    ( 0x1 << b_INT_OSD_IDLE_EN)


	 /* name: f_INT_OSD_EMPTY_EN */
	 /* desc: 层行buffer读空中断使能 */
	 /* range: [17] */
	 #define    b_INT_OSD_EMPTY_EN    ( 17 )
	 #define    m_INT_OSD_EMPTY_EN    ( 0x1 << b_INT_OSD_EMPTY_EN)


	 /* name: f_INT_OSD_ALMOST_EN */
	 /* desc: 层行buffer将要读空中断使能 */
	 /* range: [16] */
	 #define    b_INT_OSD_ALMOST_EN    ( 16 )
	 #define    m_INT_OSD_ALMOST_EN    ( 0x1 << b_INT_OSD_ALMOST_EN)


	 /* name: f_INT_OSD_IDLE */
	 /* desc: 层行关闭完成中断使能状态，写1清0 */
	 /* range: [2] */
	 #define    b_INT_OSD_IDLE    ( 2 )
	 #define    m_INT_OSD_IDLE    ( 0x1 << b_INT_OSD_IDLE)


	 /* name: f_INT_OSD_EMPTY */
	 /* desc: 层行buffer读空中断状态，写1清0 */
	 /* range: [1] */
	 #define    b_INT_OSD_EMPTY    ( 1 )
	 #define    m_INT_OSD_EMPTY    ( 0x1 << b_INT_OSD_EMPTY)


	 /* name: f_INT_OSD_ALMOST */
	 /* desc: 层行buffer将要读空中断状态，写1清0 */
	 /* range: [0] */
	 #define    b_INT_OSD_ALMOST    ( 0 )
	 #define    m_INT_OSD_ALMOST    ( 0x1 << b_INT_OSD_ALMOST)


/*  OSD_HFILTER_PARA1  */
	 /* name: f_OSD_FIR3 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数3 */
	 /* range: [31:24] */
	 #define    b_OSD_FIR3    ( 24 )
	 #define    m_OSD_FIR3    ( 0xff << b_OSD_FIR3)


	 /* name: f_OSD_FIR2 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数2 */
	 /* range: [23:16] */
	 #define    b_OSD_FIR2    ( 16 )
	 #define    m_OSD_FIR2    ( 0xff << b_OSD_FIR2)


	 /* name: f_OSD_FIR1 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数1 */
	 /* range: [15:8] */
	 #define    b_OSD_FIR1    ( 8 )
	 #define    m_OSD_FIR1    ( 0xff << b_OSD_FIR1)


	 /* name: f_OSD_FIR0 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数0 */
	 /* range: [7:0] */
	 #define    b_OSD_FIR0    ( 0 )
	 #define    m_OSD_FIR0    ( 0xff << b_OSD_FIR0)


/*  OSD_HFILTER_PARA2  */
	 /* name: f_OSD_FIR7 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数7 */
	 /* range: [31:24] */
	 #define    b_OSD_FIR7    ( 24 )
	 #define    m_OSD_FIR7    ( 0xff << b_OSD_FIR7)


	 /* name: f_OSD_FIR6 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数6 */
	 /* range: [23:16] */
	 #define    b_OSD_FIR6    ( 16 )
	 #define    m_OSD_FIR6    ( 0xff << b_OSD_FIR6)


	 /* name: f_OSD_FIR5 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数5 */
	 /* range: [15:8] */
	 #define    b_OSD_FIR5    ( 8 )
	 #define    m_OSD_FIR5    ( 0xff << b_OSD_FIR5)


	 /* name: f_OSD_FIR4 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数4 */
	 /* range: [7:0] */
	 #define    b_OSD_FIR4    ( 0 )
	 #define    m_OSD_FIR4    ( 0xff << b_OSD_FIR4)


/*  OSD_HFILTER_PARA3  */
	 /* name: f_OSD_FIR9 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数9 */
	 /* range: [16:8] */
	 #define    b_OSD_FIR9    ( 8 )
	 #define    m_OSD_FIR9    ( 0x1ff << b_OSD_FIR9)


	 /* name: f_OSD_FIR8 */
	 /* desc: 缩放处理后，行最大19阶FIR滤波参数8 */
	 /* range: [7:0] */
	 #define    b_OSD_FIR8    ( 0 )
	 #define    m_OSD_FIR8    ( 0xff << b_OSD_FIR8)


/*  OSD_HFILTER_PARA4  */
	 /* name: f_OSD_FIR_BY_PASS */
	 /* desc: FIR滤波bypass控制，高电平有效 */
	 /* range: [31] */
	 #define    b_OSD_FIR_BY_PASS    ( 31 )
	 #define    m_OSD_FIR_BY_PASS    ( 0x1 << b_OSD_FIR_BY_PASS)


	 /* name: f_OSD_FIR_SIGN */
	 /* desc: 滤波系数符号位。bitx代表. OSD_FIRx的符号位 */
	 /* range: [9:0] */
	 #define    b_OSD_FIR_SIGN    ( 0 )
	 #define    m_OSD_FIR_SIGN    ( 0x3ff << b_OSD_FIR_SIGN)


/*  OSD_ZOOM_PARA  */
	 /* name: f_OSD_ZOOM_PARA */
	 /* desc: 缩放相位系数 */
	 /* range: [31:0] */
	 #define    b_OSD_ZOOM_PARA    ( 0 )
	 #define    m_OSD_ZOOM_PARA    ( 0xffffffff << b_OSD_ZOOM_PARA)


#endif
