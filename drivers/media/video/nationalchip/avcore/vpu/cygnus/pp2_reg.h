#ifndef    __VPU_PP2_H__
#define    __VPU_PP2_H__

struct cygnus_vpu_pp2_reg {
	unsigned rPP2_Y_TOP_START_ADDR; //0x0
	unsigned rPP2_Y_BOTTOM_START_ADDR; //0x4
	unsigned rPP2_U_TOP_START_ADDR; //0x8
	unsigned rPP2_U_BOTTOM_START_ADDR; //0xc
	unsigned rPP2_V_TOP_START_ADDR; //0x10
	unsigned rPP2_V_BOTTOM_START_ADDR; //0x14
	unsigned rPP2_MODE_CTRL; //0x18
	unsigned rPP2_FRAME_WIDTH; //0x1c
	unsigned rRESV0[ 2 ];
	unsigned rPP2_PARA_UPDATE; //0x28
	unsigned rPP2_STEP; //0x2c
	unsigned rPP2_CTRL; //0x30
	unsigned rPP2_ALPHA; //0x34
	unsigned rPP2_VIEW_V; //0x38
	unsigned rPP2_VIEW_H; //0x3c
	unsigned rRESV1[ 3 ];
	unsigned rPP2_INT_EMPTY_GATE; //0x4c
	unsigned rPP2_BUFFER_INT; //0x50
};

/*  PP2_Y_TOP_START_ADDR  */
	 /* name: f_PP2_Y_TOP_FIELD_START_ADDR */
	 /* desc: Y数据顶场起始地址 */
	 /* range: [31:0] */
	 #define    b_PP2_Y_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_PP2_Y_TOP_FIELD_START_ADDR    ( 0xffffffff << b_PP2_Y_TOP_FIELD_START_ADDR)


/*  PP2_Y_BOTTOM_START_ADDR  */
	 /* name: f_PP2_Y_BOTTOM_FIELD_START_ADDR */
	 /* desc: Y数据底场起始地址 */
	 /* range: [31:0] */
	 #define    b_PP2_Y_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_PP2_Y_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_PP2_Y_BOTTOM_FIELD_START_ADDR)


/*  PP2_U_TOP_START_ADDR  */
	 /* name: f_PP2_U_TOP_FIELD_START_ADDR */
	 /* desc: Cb数据顶场起始地址 */
	 /* range: [31:0] */
	 #define    b_PP2_U_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_PP2_U_TOP_FIELD_START_ADDR    ( 0xffffffff << b_PP2_U_TOP_FIELD_START_ADDR)


/*  PP2_U_BOTTOM_START_ADDR  */
	 /* name: f_PP2_U_BOTTOM_FIELD_START_ADDR */
	 /* desc: Cb数据底场起始地址 */
	 /* range: [31:0] */
	 #define    b_PP2_U_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_PP2_U_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_PP2_U_BOTTOM_FIELD_START_ADDR)


/*  PP2_V_TOP_START_ADDR  */
	 /* name: f_PP2_V_TOP_FIELD_START_ADDR */
	 /* desc: Cr数据顶场起始地址 */
	 /* range: [31:0] */
	 #define    b_PP2_V_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_PP2_V_TOP_FIELD_START_ADDR    ( 0xffffffff << b_PP2_V_TOP_FIELD_START_ADDR)


/*  PP2_V_BOTTOM_START_ADDR  */
	 /* name: f_PP2_V_BOTTOM_FIELD_START_ADDR */
	 /* desc: Cr数据底场起始地址 */
	 /* range: [31:0] */
	 #define    b_PP2_V_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_PP2_V_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_PP2_V_BOTTOM_FIELD_START_ADDR)


/*  PP2_MODE_CTRL  */
	 /* name: f_PP2_UPSAMPLE_SEL */
	 /* desc: 422转444格式处理方式,1:repeat，0：FIR滤波 */
	 /* range: [4] */
	 #define    b_PP2_UPSAMPLE_SEL    ( 4 )
	 #define    m_PP2_UPSAMPLE_SEL    ( 0x1 << b_PP2_UPSAMPLE_SEL)


	 /* name: f_PP2_DATA_TYPE */
	 /* desc: 数据存储格式格式控制;0：存储格式为UYVY；1：存储格式为YUV422，半交织；2：存储格式为YUV420，半交织;3:存储格式为YUV444，非交织 */
	 /* range: [1:0] */
	 #define    b_PP2_DATA_TYPE    ( 0 )
	 #define    m_PP2_DATA_TYPE    ( 0x3 << b_PP2_DATA_TYPE)


/*  PP2_FRAME_WIDTH  */
	 /* name: f_PP2_FRAME_STRIDE */
	 /* desc: 层数据存储行宽度，单位为像素 */
	 /* range: [10:0] */
	 #define    b_PP2_FRAME_STRIDE    ( 0 )
	 #define    m_PP2_FRAME_STRIDE    ( 0x7ff << b_PP2_FRAME_STRIDE)


/*  PP2_PARA_UPDATE  */
	 /* name: f_PP2_PARA_UPDATE */
	 /* desc: 寄存器更新控制，上升沿有效 */
	 /* range: [0] */
	 #define    b_PP2_PARA_UPDATE    ( 0 )
	 #define    m_PP2_PARA_UPDATE    ( 0x1 << b_PP2_PARA_UPDATE)


/*  PP2_STEP  */
	 /* name: f_PP2_SDRAM_STEP */
	 /* desc: 层数据申请段控制 */
	 /* range: [26:16] */
	 #define    b_PP2_SDRAM_STEP    ( 16 )
	 #define    m_PP2_SDRAM_STEP    ( 0x7ff << b_PP2_SDRAM_STEP)


	 /* name: f_PP2_ENDIAN_MODE */
	 /* desc: 层数据排列字节序,例如像素数据从左到右连续8个像素编号分辨为B0、B1、B2、B3、B4、B5、B6、B7；0:排序为B0、B1、B2、B3、B4、B5、B6、B7；1:排序为B7、B6、B5、B4、B3、B2、B1、B0；2:排序为B4、B5、B6、B7、B0、B1、B2、B3；3:排序为B3、B2、B1、B0、B7、B6、B5、B4；4:排序为B1、B0、B3、B2、B5、B4、B7、B6；5:排序为B6、B7、B4、B5、B2、B3、B1、B0；6:排序为B5、B4、B7、B6、B1、B0、B3、B2；7:排序为B3、B2、B0、B1、B5、B4、B7、B6 */
	 /* range: [10:8] */
	 #define    b_PP2_ENDIAN_MODE    ( 8 )
	 #define    m_PP2_ENDIAN_MODE    ( 0x7 << b_PP2_ENDIAN_MODE)


/*  PP2_CTRL  */
	 /* name: f_PP2_EN */
	 /* desc: 层使能信号，高电平有效 */
	 /* range: [31] */
	 #define    b_PP2_EN    ( 31 )
	 #define    m_PP2_EN    ( 0x1 << b_PP2_EN)


	 /* name: f_PP2_BACK_COLOR */
	 /* desc: 层背景色控制，存储为YUV数据，各8bits */
	 /* range: [23:0] */
	 #define    b_PP2_BACK_COLOR    ( 0 )
	 #define    m_PP2_BACK_COLOR    ( 0xffffff << b_PP2_BACK_COLOR)


/*  PP2_ALPHA  */
	 /* name: f_PP2_ALPHA */
	 /* desc: 层透明度控制，一共256阶，0为全透明，255为不透明 */
	 /* range: [7:0] */
	 #define    b_PP2_ALPHA    ( 0 )
	 #define    m_PP2_ALPHA    ( 0xff << b_PP2_ALPHA)


/*  PP2_VIEW_V  */
	 /* name: f_PP2_VIEW_TOP */
	 /* desc: 层显示首行位置，相对位置为制式有效行 */
	 /* range: [26:16] */
	 #define    b_PP2_VIEW_TOP    ( 16 )
	 #define    m_PP2_VIEW_TOP    ( 0x7ff << b_PP2_VIEW_TOP)


	 /* name: f_PP2_VIEW_BOTTOM */
	 /* desc: 层显示尾行行位置，相对位置为制式有效行 */
	 /* range: [10:0] */
	 #define    b_PP2_VIEW_BOTTOM    ( 0 )
	 #define    m_PP2_VIEW_BOTTOM    ( 0x7ff << b_PP2_VIEW_BOTTOM)


/*  PP2_VIEW_H  */
	 /* name: f_PP2_VIEW_LEFT */
	 /* desc: 层显示行左显示位置，相对位置为制式有效像素 */
	 /* range: [26:16] */
	 #define    b_PP2_VIEW_LEFT    ( 16 )
	 #define    m_PP2_VIEW_LEFT    ( 0x7ff << b_PP2_VIEW_LEFT)


	 /* name: f_PP2_VIEW_RIGHT */
	 /* desc: 层显示行右显示位置，相对位置为制式有效像素 */
	 /* range: [10:0] */
	 #define    b_PP2_VIEW_RIGHT    ( 0 )
	 #define    m_PP2_VIEW_RIGHT    ( 0x7ff << b_PP2_VIEW_RIGHT)


/*  PP2_INT_EMPTY_GATE  */
	 /* name: f_INT_PP2_EMPTY_GATE */
	 /* desc: 层行buffer空预警中断控制门限 */
	 /* range: [10:0] */
	 #define    b_INT_PP2_EMPTY_GATE    ( 0 )
	 #define    m_INT_PP2_EMPTY_GATE    ( 0x7ff << b_INT_PP2_EMPTY_GATE)


/*  PP2_BUFFER_INT  */
	 /* name: f_INT_PP2_IDLE_EN */
	 /* desc: 层行关闭完成中断使能 */
	 /* range: [18] */
	 #define    b_INT_PP2_IDLE_EN    ( 18 )
	 #define    m_INT_PP2_IDLE_EN    ( 0x1 << b_INT_PP2_IDLE_EN)


	 /* name: f_INT_PP2_EMPTY_EN */
	 /* desc: 层行buffer读空中断使能 */
	 /* range: [17] */
	 #define    b_INT_PP2_EMPTY_EN    ( 17 )
	 #define    m_INT_PP2_EMPTY_EN    ( 0x1 << b_INT_PP2_EMPTY_EN)


	 /* name: f_INT_PP2_ALMOST_EN */
	 /* desc: 层行buffer将要读空中断使能 */
	 /* range: [16] */
	 #define    b_INT_PP2_ALMOST_EN    ( 16 )
	 #define    m_INT_PP2_ALMOST_EN    ( 0x1 << b_INT_PP2_ALMOST_EN)


	 /* name: f_INT_PP2_IDLE */
	 /* desc: 层行关闭完成中断使能状态，写1清0 */
	 /* range: [2] */
	 #define    b_INT_PP2_IDLE    ( 2 )
	 #define    m_INT_PP2_IDLE    ( 0x1 << b_INT_PP2_IDLE)


	 /* name: f_INT_PP2_EMPTY */
	 /* desc: 层行buffer读空中断状态，写1清0 */
	 /* range: [1] */
	 #define    b_INT_PP2_EMPTY    ( 1 )
	 #define    m_INT_PP2_EMPTY    ( 0x1 << b_INT_PP2_EMPTY)


	 /* name: f_INT_PP2_ALMOST */
	 /* desc: 层行buffer将要读空中断状态，写1清0 */
	 /* range: [0] */
	 #define    b_INT_PP2_ALMOST    ( 0 )
	 #define    m_INT_PP2_ALMOST    ( 0x1 << b_INT_PP2_ALMOST)

#endif

