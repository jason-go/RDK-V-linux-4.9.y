#ifndef    __VPU_CE_H__
#define    __VPU_CE_H__

struct cygnus_vpu_ce_addr_reg {
	unsigned rCE_Y_TOP_START_ADDR;
	unsigned rCE_Y_BOTTOM_START_ADDR;
	unsigned rCE_U_TOP_START_ADDR;
	unsigned rCE_U_BOTTOM_START_ADDR;
	unsigned rCE_V_TOP_START_ADDR;
	unsigned rCE_V_BOTTOM_START_ADDR;
};

struct cygnus_vpu_ce_reg {
	struct cygnus_vpu_ce_addr_reg ce_addr[4];
	unsigned rCE_BUFF_START_ADDR; //0x60
	unsigned rCE_BUFF_SIZE; //0x64
	unsigned rCE_CAP_ENABLE; //0x68
	unsigned rCE_CAP_CTROL; //0x6c
	unsigned rCE_CAP_MODE; //0x70
	unsigned rCE_FRAME_WIDTH; //0x74
	unsigned rCE_CAP_H; //0x78
	unsigned rCE_CAP_V; //0x7c
	unsigned rCE_ZOOM; //0x80
	unsigned rCE_DEST_SIZE; //0x84
	unsigned rCE_REQ_LENGTH; //0x88
	unsigned rCE_ZOOM_PHASE; //0x8c
	unsigned rCE_PARA_UPDATE; //0x90
	unsigned rCE_ZOOM_PARA_SIGN; //0x94
	unsigned rRESV0[ 4 ];
	unsigned rCE_INT_FIFO_EMPTY_GATE; //0xa8
	unsigned rCE_BUFFER_INT; //0xac
	unsigned rRESV1[ 20 ];
	unsigned rCE_HZOOM_PARA[64]; //0x100
	unsigned rCE_VZOOM_PARA[64]; //0x200
};

/*  CE_Y_TOP_START_ADDR  */
	 /* name: f_CE_Y_TOP_FIELD_START_ADDR */
	 /* desc: Y数据顶场起始地址 */
	 /* range: [31:0] */
	 #define    b_CE_Y_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_CE_Y_TOP_FIELD_START_ADDR    ( 0xffffffff << b_CE_Y_TOP_FIELD_START_ADDR)


/*  CE_Y_BOTTOM_START_ADDR  */
	 /* name: f_CE_Y_BOTTOM_FIELD_START_ADDR */
	 /* desc: Y数据底场起始地址 */
	 /* range: [31:0] */
	 #define    b_CE_Y_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_CE_Y_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_CE_Y_BOTTOM_FIELD_START_ADDR)


/*  CE_U_TOP_START_ADDR  */
	 /* name: f_CE_U_TOP_FIELD_START_ADDR */
	 /* desc: Cb数据顶场起始地址 */
	 /* range: [31:0] */
	 #define    b_CE_U_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_CE_U_TOP_FIELD_START_ADDR    ( 0xffffffff << b_CE_U_TOP_FIELD_START_ADDR)


/*  CE_U_BOTTOM_START_ADDR  */
	 /* name: f_CE_U_BOTTOM_FIELD_START_ADDR */
	 /* desc: Cb数据底场起始地址 */
	 /* range: [31:0] */
	 #define    b_CE_U_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_CE_U_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_CE_U_BOTTOM_FIELD_START_ADDR)


/*  CE_V_TOP_START_ADDR  */
	 /* name: f_CE_V_TOP_FIELD_START_ADDR */
	 /* desc: Cr数据顶场起始地址 */
	 /* range: [31:0] */
	 #define    b_CE_V_TOP_FIELD_START_ADDR    ( 0 )
	 #define    m_CE_V_TOP_FIELD_START_ADDR    ( 0xffffffff << b_CE_V_TOP_FIELD_START_ADDR)


/*  CE_V_BOTTOM_START_ADDR  */
	 /* name: f_CE_V_BOTTOM_FIELD_START_ADDR */
	 /* desc: Cr数据底场起始地址 */
	 /* range: [31:0] */
	 #define    b_CE_V_BOTTOM_FIELD_START_ADDR    ( 0 )
	 #define    m_CE_V_BOTTOM_FIELD_START_ADDR    ( 0xffffffff << b_CE_V_BOTTOM_FIELD_START_ADDR)


/*  CE_BUFF_START_ADDR  */
	 /* name: f_CE_BUFFER_ADDR */
	 /* desc: SVPU使用buffer起始位置，由svpu_nosec_en和nosec_apb_en控制配置。仅当svpu_nosec_en = 0或者nosec_apb_en = 1，且svpu_otp_buf_protect_en = 0时，可以重复配置，否则上电仅仅可以配置一次 */
	 /* range: [31:0] */
	 #define    b_CE_BUFFER_ADDR    ( 0 )
	 #define    m_CE_BUFFER_ADDR    ( 0xffffffff << b_CE_BUFFER_ADDR)


/*  CE_BUFF_SIZE  */
	 /* name: f_CE_BUFFER_SIZE */
	 /* desc: SVPU使用buffer的大小。由svpu_nosec_en和nosec_apb_en控制配置。仅当svpu_nosec_en = 0或者nosec_apb_en = 1，且svpu_otp_buf_protect_en = 0时，可以重复配置，否则上电仅仅可以配置一次 */
	 /* range: [31:0] */
	 #define    b_CE_BUFFER_SIZE    ( 0 )
	 #define    m_CE_BUFFER_SIZE    ( 0xffffffff << b_CE_BUFFER_SIZE)


/*  CE_CAP_ENABLE  */
	 /* name: f_FRAME_CNT_CLR */
	 /* desc: 当前已经采集完成帧数计数器清零控制，高电平有效，硬件自动清0 */
	 /* range: [8] */
	 #define    b_FRAME_CNT_CLR    ( 8 )
	 #define    m_FRAME_CNT_CLR    ( 0x1 << b_FRAME_CNT_CLR)


	 /* name: f_SINGLE_FRAME_CAP_ENABLE */
	 /* desc: 单帧抓图使能控制，上升沿有效，每次配置高电平后，会自动采集一帧数据到最后一个帧存地址中，采集完成后，硬件自动清0 */
	 /* range: [4] */
	 #define    b_SINGLE_FRAME_CAP_ENABLE    ( 4 )
	 #define    m_SINGLE_FRAME_CAP_ENABLE    ( 0x1 << b_SINGLE_FRAME_CAP_ENABLE)


	 /* name: f_CAP_ENABLE */
	 /* desc: 抓图使能控制，高电平有效 */
	 /* range: [0] */
	 #define    b_CAP_ENABLE    ( 0 )
	 #define    m_CAP_ENABLE    ( 0x1 << b_CAP_ENABLE)


/*  CE_CAP_CTROL  */
	 /* name: f_SCE_TIME_DL */
	 /* desc: 数据存储检测延时控制，用于总线时钟域和像素时钟域同步 */
	 /* range: [31:24] */
	 #define    b_SCE_TIME_DL    ( 24 )
	 #define    m_SCE_TIME_DL    ( 0xff << b_SCE_TIME_DL)


	 /* name: f_CAP_V_DOWNSCALE */
	 /* desc: 缩放垂直降抽样使能，高电平有效 */
	 /* range: [20] */
	 #define    b_CAP_V_DOWNSCALE    ( 20 )
	 #define    m_CAP_V_DOWNSCALE    ( 0x1 << b_CAP_V_DOWNSCALE)


	 /* name: f_CE_FRAME_MODE */
	 /* desc: 播放时采用的帧存数目控制;1:需要使用一帧帧存，为帧存地址的第一个个地址;2:需要使用两帧帧存，为帧存地址的前两个地址；3:需要使用三帧帧存，为帧存地址的前三个地址;其它值：保留，无任何定义 */
	 /* range: [17:16] */
	 #define    b_CE_FRAME_MODE    ( 16 )
	 #define    m_CE_FRAME_MODE    ( 0x3 << b_CE_FRAME_MODE)


	 /* name: f_CAP_DLY */
	 /* desc: 抓图延时控制，用于调整正确的抓图位置 */
	 /* range: [13:8] */
	 #define    b_CAP_DLY    ( 8 )
	 #define    m_CAP_DLY    ( 0x3f << b_CAP_DLY)


	 /* name: f_CAP_SURFACE_EN */
	 /* desc: [6]:OSD0层抓图混合使能控制，高电平有效；[4]:PP0层抓图混合使能控制，高电平有效；[3]:PIC层抓图混合使能控制，高电平有效 */
	 /* range: [7:3] */
	 #define    b_CAP_SURFACE_EN    ( 3 )
	 #define    m_CAP_SURFACE_EN    ( 0x1f << b_CAP_SURFACE_EN)


/*  CE_CAP_MODE  */
	 /* name: f_CAP_FORMAT_MODE */
	 /* desc: 自动模式时，当前缩放模式，用于读取当前缩放比例状态；0:1080p转PAL；1:1080i转PAL；2:720p转PAL；3:576p转PAL；4:576i转PAL；8:1080p转NTSC；9:1080i转NTSC；10:720p转NTSC；11:576p转NTSC；12:576i转NTSC；其他：保留 */
	 /* range: [31:28] */
	 #define    b_CAP_FORMAT_MODE    ( 28 )
	 #define    m_CAP_FORMAT_MODE    ( 0xf << b_CAP_FORMAT_MODE)


	 /* name: f_CAP_FORMAT_AUTO */
	 /* desc: 根据HDP和SDP制式自动配置缩放比例控制；1：硬件根据HDP和SDP Format自动配置缩放参数；0：缩放参数手动配置模式，缩放参数必须根据实际需要配置 */
	 /* range: [24] */
	 #define    b_CAP_FORMAT_AUTO    ( 24 )
	 #define    m_CAP_FORMAT_AUTO    ( 0x1 << b_CAP_FORMAT_AUTO)


	 /* name: f_CAP_NO_ZOOM */
	 /* desc: 不做缩放控制使能。高电平有效 */
	 /* range: [21] */
	 #define    b_CAP_NO_ZOOM    ( 21 )
	 #define    m_CAP_NO_ZOOM    ( 0x1 << b_CAP_NO_ZOOM)


	 /* name: f_CAP_DATA_TYPE */
	 /* desc: 数据存储格式格式控制;0：存储格式为UYVY；1：存储格式为YUV422，半交织；2：存储格式为YUV420，半交织;3:存储格式为YUV444，非交织 */
	 /* range: [1:0] */
	 #define    b_CAP_DATA_TYPE    ( 0 )
	 #define    m_CAP_DATA_TYPE    ( 0x3 << b_CAP_DATA_TYPE)


/*  CE_FRAME_WIDTH  */
	 /* name: f_CE_FRAME_STRIDE */
	 /* desc: 抓图数据存储宽度，8字节对齐 */
	 /* range: [10:0] */
	 #define    b_CE_FRAME_STRIDE    ( 0 )
	 #define    m_CE_FRAME_STRIDE    ( 0x7ff << b_CE_FRAME_STRIDE)


/*  CE_CAP_H  */
	 /* name: f_CAP_RIGHT */
	 /* desc: 源数据抓图右边界，单位为像素 */
	 /* range: [26:16] */
	 #define    b_CAP_RIGHT    ( 16 )
	 #define    m_CAP_RIGHT    ( 0x7ff << b_CAP_RIGHT)


	 /* name: f_CAP_LEFT */
	 /* desc: 源数据抓图左边界，单位为像素 */
	 /* range: [10:0] */
	 #define    b_CAP_LEFT    ( 0 )
	 #define    m_CAP_LEFT    ( 0x7ff << b_CAP_LEFT)


/*  CE_CAP_V  */
	 /* name: f_CAP_BOTTOM */
	 /* desc: 源数据抓图下边界，单位为行 */
	 /* range: [26:16] */
	 #define    b_CAP_BOTTOM    ( 16 )
	 #define    m_CAP_BOTTOM    ( 0x7ff << b_CAP_BOTTOM)


	 /* name: f_CAP_TOP */
	 /* desc: 源数据抓图上边界，单位为行 */
	 /* range: [10:0] */
	 #define    b_CAP_TOP    ( 0 )
	 #define    m_CAP_TOP    ( 0x7ff << b_CAP_TOP)


/*  CE_ZOOM  */
	 /* name: f_CAP_H_ZOOM */
	 /* desc: 水平方向缩放比例控制，值计算方法为”源宽*4096/目标宽” */
	 /* range: [29:16] */
	 #define    b_CAP_H_ZOOM    ( 16 )
	 #define    m_CAP_H_ZOOM    ( 0x3fff << b_CAP_H_ZOOM)


	 /* name: f_CAP_V_ZOOM */
	 /* desc: 垂直方向缩放比例控制，值计算方法为”源高*4096/目标高” */
	 /* range: [13:0] */
	 #define    b_CAP_V_ZOOM    ( 0 )
	 #define    m_CAP_V_ZOOM    ( 0x3fff << b_CAP_V_ZOOM)


/*  CE_DEST_SIZE  */
	 /* name: f_SCE_ZOOM_PROGRESIVE */
	 /* desc: 采样缩放目标为逐行高度标识，高电平有效 */
	 /* range: [31] */
	 #define    b_SCE_ZOOM_PROGRESIVE    ( 31 )
	 #define    m_SCE_ZOOM_PROGRESIVE    ( 0x1 << b_SCE_ZOOM_PROGRESIVE)


	 /* name: f_SCE_ZOOM_HEIGHT */
	 /* desc: 采样缩放目标高度 */
	 /* range: [26:16] */
	 #define    b_SCE_ZOOM_HEIGHT    ( 16 )
	 #define    m_SCE_ZOOM_HEIGHT    ( 0x7ff << b_SCE_ZOOM_HEIGHT)


	 /* name: f_SCE_ZOOM_LENGTH */
	 /* desc: 采样缩放目标宽度 */
	 /* range: [10:0] */
	 #define    b_SCE_ZOOM_LENGTH    ( 0 )
	 #define    m_SCE_ZOOM_LENGTH    ( 0x7ff << b_SCE_ZOOM_LENGTH)


/*  CE_REQ_LENGTH  */
	 /* name: f_CE_ENDIAN_MODE */
	 /* desc: 数据排列字节序,例如像素数据从左到右连续8个像素编号分辨为B0、B1、B2、B3、B4、B5、B6、B7；0:排序为B0、B1、B2、B3、B4、B5、B6、B7；1:排序为B7、B6、B5、B4、B3、B2、B1、B0；2:排序为B4、B5、B6、B7、B0、B1、B2、B3；3:排序为B3、B2、B1、B0、B7、B6、B5、B4；4:排序为B1、B0、B3、B2、B5、B4、B7、B6；5:排序为B6、B7、B4、B5、B2、B3、B1、B0；6:排序为B5、B4、B7、B6、B1、B0、B3、B2；7:排序为B3、B2、B0、B1、B5、B4、B7、B6 */
	 /* range: [14:12] */
	 #define    b_CE_ENDIAN_MODE    ( 12 )
	 #define    m_CE_ENDIAN_MODE    ( 0x7 << b_CE_ENDIAN_MODE)


	 /* name: f_SDRAM_REQ_BUF_LEN */
	 /* desc: 抓图输出存储到内存时，单次存储最小长度 */
	 /* range: [10:0] */
	 #define    b_SDRAM_REQ_BUF_LEN    ( 0 )
	 #define    m_SDRAM_REQ_BUF_LEN    ( 0x7ff << b_SDRAM_REQ_BUF_LEN)


/*  CE_ZOOM_PHASE  */
	 /* name: f_ZOOM_MODE */
	 /* desc: zoom_mode[0]==1 帧按场处理（逐行转隔行），zoom_mode[1]==1场按帧处理（隔行转逐行） */
	 /* range: [31:30] */
	 #define    b_ZOOM_MODE    ( 30 )
	 #define    m_ZOOM_MODE    ( 0x3 << b_ZOOM_MODE)


	 /* name: f_SCE_VT_PHASE_BIAS */
	 /* desc: 缩放时顶场起始相位，每行总相位为4096 */
	 /* range: [29:16] */
	 #define    b_SCE_VT_PHASE_BIAS    ( 16 )
	 #define    m_SCE_VT_PHASE_BIAS    ( 0x3fff << b_SCE_VT_PHASE_BIAS)


	 /* name: f_SCE_VB_PHASE_BIAS */
	 /* desc: 缩放时底场起始相位，每行总相位为4096 */
	 /* range: [13:0] */
	 #define    b_SCE_VB_PHASE_BIAS    ( 0 )
	 #define    m_SCE_VB_PHASE_BIAS    ( 0x3fff << b_SCE_VB_PHASE_BIAS)


/*  CE_PARA_UPDATE  */
	 /* name: f_CE_PARA_UPDATE */
	 /* desc: 寄存器更新使能，上升沿更新寄存器 */
	 /* range: [0] */
	 #define    b_CE_PARA_UPDATE    ( 0 )
	 #define    m_CE_PARA_UPDATE    ( 0x1 << b_CE_PARA_UPDATE)


/*  CE_ZOOM_PARA_SIGN  */
	 /* name: f_CE_PHASE_0_ZMODE */
	 /* desc: CE缩放0相位处理;1:直接采用原数据，不做运算处理;0：采用寄存器配置的0相位运算系数进行运算 */
	 /* range: [31] */
	 #define    b_CE_PHASE_0_ZMODE    ( 31 )
	 #define    m_CE_PHASE_0_ZMODE    ( 0x1 << b_CE_PHASE_0_ZMODE)


/*  CE_INT_FIFO_EMPTY_GATE  */
	 /* name: f_INT_CE_FIFO_FULL_GATE */
	 /* desc: CE层抓图buffer监测中断门限 */
	 /* range: [8:0] */
	 #define    b_INT_CE_FIFO_FULL_GATE    ( 0 )
	 #define    m_INT_CE_FIFO_FULL_GATE    ( 0x1ff << b_INT_CE_FIFO_FULL_GATE)


/*  CE_BUFFER_INT  */
	 /* name: f_INT_CE_SINGLE_FRAME_RDY_EN */
	 /* desc: 单帧抓图完成中断使能，高电平有效 */
	 /* range: [20] */
	 #define    b_INT_CE_SINGLE_FRAME_RDY_EN    ( 20 )
	 #define    m_INT_CE_SINGLE_FRAME_RDY_EN    ( 0x1 << b_INT_CE_SINGLE_FRAME_RDY_EN)


	 /* name: f_INT_CE_FRAME_FULL_EN */
	 /* desc: 帧数据写满中断使能，高电平有效 */
	 /* range: [19] */
	 #define    b_INT_CE_FRAME_FULL_EN    ( 19 )
	 #define    m_INT_CE_FRAME_FULL_EN    ( 0x1 << b_INT_CE_FRAME_FULL_EN)


	 /* name: f_INT_CE_IDLE_EN */
	 /* desc: 层行关闭完成中断使能，高电平有效 */
	 /* range: [18] */
	 #define    b_INT_CE_IDLE_EN    ( 18 )
	 #define    m_INT_CE_IDLE_EN    ( 0x1 << b_INT_CE_IDLE_EN)


	 /* name: f_INT_CE_FIFO_FULL_EN */
	 /* desc: 层行buffer读空中断使能，高电平有效 */
	 /* range: [17] */
	 #define    b_INT_CE_FIFO_FULL_EN    ( 17 )
	 #define    m_INT_CE_FIFO_FULL_EN    ( 0x1 << b_INT_CE_FIFO_FULL_EN)


	 /* name: f_INT_CE_FIFO_ALMOST_FULL_EN */
	 /* desc: 层行buffer将要读空中断使能，高电平有效 */
	 /* range: [16] */
	 #define    b_INT_CE_FIFO_ALMOST_FULL_EN    ( 16 )
	 #define    m_INT_CE_FIFO_ALMOST_FULL_EN    ( 0x1 << b_INT_CE_FIFO_ALMOST_FULL_EN)


	 /* name: f_INT_CE_SINGLE_FRAME_RDY */
	 /* desc: 单帧抓图完成中断状态，高电平有效，写1清0 */
	 /* range: [4] */
	 #define    b_INT_CE_SINGLE_FRAME_RDY    ( 4 )
	 #define    m_INT_CE_SINGLE_FRAME_RDY    ( 0x1 << b_INT_CE_SINGLE_FRAME_RDY)


	 /* name: f_INT_CE_FRAME_FULL */
	 /* desc: 帧数据写满中断状态，高电平有效，写1清0 */
	 /* range: [3] */
	 #define    b_INT_CE_FRAME_FULL    ( 3 )
	 #define    m_INT_CE_FRAME_FULL    ( 0x1 << b_INT_CE_FRAME_FULL)


	 /* name: f_INT_CE_IDLE */
	 /* desc: 层行关闭完成中断状态，高电平有效，写1清0 */
	 /* range: [2] */
	 #define    b_INT_CE_IDLE    ( 2 )
	 #define    m_INT_CE_IDLE    ( 0x1 << b_INT_CE_IDLE)


	 /* name: f_INT_CE_FIFO_FULL */
	 /* desc: 层行buffer读空中断状态，高电平有效，写1清0 */
	 /* range: [1] */
	 #define    b_INT_CE_FIFO_FULL    ( 1 )
	 #define    m_INT_CE_FIFO_FULL    ( 0x1 << b_INT_CE_FIFO_FULL)


	 /* name: f_INT_CE_FIFO_ALMOST_FULL */
	 /* desc: 层行buffer将要读空中断状态，高电平有效，写1清0 */
	 /* range: [0] */
	 #define    b_INT_CE_FIFO_ALMOST_FULL    ( 0 )
	 #define    m_INT_CE_FIFO_ALMOST_FULL    ( 0x1 << b_INT_CE_FIFO_ALMOST_FULL)


/*  CE_HZOOM_PARA_L  */
	 /* name: f_CE_HZOOM_PARA_L */
	 /* desc: 水平缩放相位系数低32bits */
	 /* range: [31:0] */
	 #define    b_CE_HZOOM_PARA_L    ( 0 )
	 #define    m_CE_HZOOM_PARA_L    ( 0xffffffff << b_CE_HZOOM_PARA_L)


/*  CE_HZOOM_PARA_H  */
	 /* name: f_CE_HZOOM_PARA_H */
	 /* desc: 水平缩放相位系数高16bits */
	 /* range: [15:0] */
	 #define    b_CE_HZOOM_PARA_H    ( 0 )
	 #define    m_CE_HZOOM_PARA_H    ( 0xffff << b_CE_HZOOM_PARA_H)


/*  CE_VZOOM_PARA_L  */
	 /* name: f_CE_VZOOM_PARA_L */
	 /* desc: 垂直缩放相位系数低32bits */
	 /* range: [31:0] */
	 #define    b_CE_VZOOM_PARA_L    ( 0 )
	 #define    m_CE_VZOOM_PARA_L    ( 0xffffffff << b_CE_VZOOM_PARA_L)


/*  CE_VZOOM_PARA_H  */
	 /* name: f_CE_VZOOM_PARA_H */
	 /* desc: 垂直缩放相位系数高16bits */
	 /* range: [15:0] */
	 #define    b_CE_VZOOM_PARA_H    ( 0 )
	 #define    m_CE_VZOOM_PARA_H    ( 0xffff << b_CE_VZOOM_PARA_H)

#endif

