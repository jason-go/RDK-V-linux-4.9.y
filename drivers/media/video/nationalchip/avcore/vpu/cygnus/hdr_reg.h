#ifndef    __VPU_HDR_H__
#define    __VPU_HDR_H__

struct cygnus_vpu_hdr_reg {
	unsigned rHDR_VERSION; //0x0
	unsigned rHDR_CTRL; //0x4
	unsigned rHDR_INT; //0x8
	unsigned rRESV0[ 1 ];
	unsigned rYUV2RGB_COEF0; //0x10
	unsigned rYUV2RGB_COEF1; //0x14
	unsigned rYUV2RGB_COEF2; //0x18
	unsigned rYUV2RGB_COEF3; //0x1c
	unsigned rYUV2RGB_COEF4; //0x20
	unsigned rYUV2RGB_COEF5; //0x24
	unsigned rRESV1[ 2 ];
	unsigned rRGB2YUV_COEF0; //0x30
	unsigned rRGB2YUV_COEF1; //0x34
	unsigned rRGB2YUV_COEF2; //0x38
	unsigned rRGB2YUV_COEF3; //0x3c
	unsigned rRGB2YUV_COEF4; //0x40
	unsigned rRGB2YUV_COEF5; //0x44
	unsigned rRESV2[ 2 ];
	unsigned rHDR_R_MAP_VAL; //0x50
	unsigned rHDR_G_MAP_VAL; //0x54
	unsigned rHDR_B_MAP_VAL; //0x58
	unsigned rRESV3[ 9 ];
	unsigned rHDR_R_CENTER_VAL0; //0x80
	unsigned rHDR_R_CENTER_VAL1; //0x84
	unsigned rHDR_R_CENTER_VAL2; //0x88
	unsigned rHDR_R_CENTER_VAL3; //0x8c
	unsigned rHDR_R_CENTER_VAL4; //0x90
	unsigned rHDR_R_CENTER_VAL5; //0x94
	unsigned rRESV4[ 22 ];
	unsigned rHDR_G_CENTER_VAL0; //0xf0
	unsigned rHDR_G_CENTER_VAL1; //0xf4
	unsigned rHDR_G_CENTER_VAL2; //0xf8
	unsigned rHDR_G_CENTER_VAL3; //0xfc
	unsigned rHDR_G_CENTER_VAL4; //0x100
	unsigned rHDR_G_CENTER_VAL5; //0x104
	unsigned rRESV5[ 22 ];
	unsigned rHDR_B_CENTER_VAL0; //0x160
	unsigned rHDR_B_CENTER_VAL1; //0x164
	unsigned rHDR_B_CENTER_VAL2; //0x168
	unsigned rHDR_B_CENTER_VAL3; //0x16c
	unsigned rHDR_B_CENTER_VAL4; //0x170
	unsigned rHDR_B_CENTER_VAL5; //0x174
	unsigned rRESV6[ 34 ];
	unsigned rHDR_R_CURVE_VAL[448]; //0x200
	unsigned rHDR_G_CURVE_VAL[448]; //0x900
	unsigned rHDR_B_CURVE_VAL[448]; //0x1000
};

/*  hdr_version  */
	 /* name: f_hdr_version */
	 /* desc: ;HDR版本号 */
	 /* range: [31:0] */
	 #define    b_HDR_VERSION    ( 0 )
	 #define    m_HDR_VERSION    ( 0xffffffff << b_HDR_VERSION)


/*  HDR_CTRL  */
	 /* name: f_HDR_R_SEARCH_MODE */
	 /* desc: R分量分段中心点搜索模式;2'd0/2'd3：以R分量进行搜索;2'd1：以G分量进行搜索;2'd2：以B分量进行搜索 */
	 /* range: [21:20] */
	 #define    b_HDR_R_SEARCH_MODE    ( 20 )
	 #define    m_HDR_R_SEARCH_MODE    ( 0x3 << b_HDR_R_SEARCH_MODE)


	 /* name: f_HDR_G_SEARCH_MODE */
	 /* desc: G分量分段中心点搜索模式;2'd0/2'd3：以R分量进行搜索;2'd1：以G分量进行搜索;2'd2：以B分量进行搜索 */
	 /* range: [19:18] */
	 #define    b_HDR_G_SEARCH_MODE    ( 18 )
	 #define    m_HDR_G_SEARCH_MODE    ( 0x3 << b_HDR_G_SEARCH_MODE)


	 /* name: f_HDR_B_SEARCH_MODE */
	 /* desc: B分量分段中心点搜索模式;2'd0/2'd3：以R分量进行搜索;2'd1：以G分量进行搜索;2'd2：以B分量进行搜索 */
	 /* range: [17:16] */
	 #define    b_HDR_B_SEARCH_MODE    ( 16 )
	 #define    m_HDR_B_SEARCH_MODE    ( 0x3 << b_HDR_B_SEARCH_MODE)


	 /* name: f_HDR_READ_INDEX */
	 /* desc: 曲线系数SRAM读取起始地址偏移;对于16分段可初始化多份，用于切换系数 */
	 /* range: [12:8] */
	 #define    b_HDR_READ_INDEX    ( 8 )
	 #define    m_HDR_READ_INDEX    ( 0x1f << b_HDR_READ_INDEX)


	 /* name: f_HDR_R_REG_RST */
	 /* desc: R分量Curve系数SRAM读写指针复位 */
	 /* range: [6:6] */
	 #define    b_HDR_R_REG_RST    ( 6 )
	 #define    m_HDR_R_REG_RST    ( 0x1 << b_HDR_R_REG_RST)


	 /* name: f_HDR_G_REG_RST */
	 /* desc: G分量Curve系数SRAM读写指针复位 */
	 /* range: [5:5] */
	 #define    b_HDR_G_REG_RST    ( 5 )
	 #define    m_HDR_G_REG_RST    ( 0x1 << b_HDR_G_REG_RST)


	 /* name: f_HDR_B_REG_RST */
	 /* desc: B分量Curve系数SRAM读写指针复位 */
	 /* range: [4:4] */
	 #define    b_HDR_B_REG_RST    ( 4 )
	 #define    m_HDR_B_REG_RST    ( 0x1 << b_HDR_B_REG_RST)


	 /* name: f_HDR_SPLIT_MODE */
	 /* desc: 曲线系数分段模式：;0/3：16分段;1：32分段;2：64分段 */
	 /* range: [3:2] */
	 #define    b_HDR_SPLIT_MODE    ( 2 )
	 #define    m_HDR_SPLIT_MODE    ( 0x3 << b_HDR_SPLIT_MODE)


	 /* name: f_HDR_CLK_EN */
	 /* desc: HDR模块clk_gate使能控制信号 */
	 /* range: [1:1] */
	 #define    b_HDR_CLK_EN    ( 1 )
	 #define    m_HDR_CLK_EN    ( 0x1 << b_HDR_CLK_EN)


	 /* name: f_HDR_ENABLE */
	 /* desc: HDR模块使能信号，当HDR模块不使能时数据会Bypass处理。 */
	 /* range: [0:0] */
	 #define    b_HDR_ENABLE    ( 0 )
	 #define    m_HDR_ENABLE    ( 0x1 << b_HDR_ENABLE)


/*  HDR_INT  */
	 /* name: f_HDR_BYPASS_INT_EN */
	 /* desc: HDR模块帧头Bypass中断使能，开启时在帧头遇到Bypass状态会上报一次中断 */
	 /* range: [1:1] */
	 #define    b_HDR_BYPASS_INT_EN    ( 1 )
	 #define    m_HDR_BYPASS_INT_EN    ( 0x1 << b_HDR_BYPASS_INT_EN)


	 /* name: f_HDR_BYPASS_INT_STATUS */
	 /* desc: HDR模块帧头Bypass中断状态，帧头检测到处于Bypass状态，硬件置1，软件写1清零 */
	 /* range: [0:0] */
	 #define    b_HDR_BYPASS_INT_STATUS    ( 0 )
	 #define    m_HDR_BYPASS_INT_STATUS    ( 0x1 << b_HDR_BYPASS_INT_STATUS)


/*  YUV2RGB_COEF0  */
	 /* name: f_YUV2RGB_COEF00 */
	 /* desc: YUV转RGB系数coef00 */
	 /* range: [27:16] */
	 #define    b_YUV2RGB_COEF00    ( 16 )
	 #define    m_YUV2RGB_COEF00    ( 0xfff << b_YUV2RGB_COEF00)


	 /* name: f_YUV2RGB_COEF01 */
	 /* desc: YUV转RGB系数coef01 */
	 /* range: [11:0] */
	 #define    b_YUV2RGB_COEF01    ( 0 )
	 #define    m_YUV2RGB_COEF01    ( 0xfff << b_YUV2RGB_COEF01)


/*  YUV2RGB_COEF1  */
	 /* name: f_YUV2RGB_COEF02 */
	 /* desc: YUV转RGB系数coef02 */
	 /* range: [27:16] */
	 #define    b_YUV2RGB_COEF02    ( 16 )
	 #define    m_YUV2RGB_COEF02    ( 0xfff << b_YUV2RGB_COEF02)


	 /* name: f_YUV2RGB_COEF03 */
	 /* desc: YUV转RGB系数coef03 */
	 /* range: [11:0] */
	 #define    b_YUV2RGB_COEF03    ( 0 )
	 #define    m_YUV2RGB_COEF03    ( 0xfff << b_YUV2RGB_COEF03)


/*  YUV2RGB_COEF2  */
	 /* name: f_YUV2RGB_COEF10 */
	 /* desc: YUV转RGB系数coef10 */
	 /* range: [27:16] */
	 #define    b_YUV2RGB_COEF10    ( 16 )
	 #define    m_YUV2RGB_COEF10    ( 0xfff << b_YUV2RGB_COEF10)


	 /* name: f_YUV2RGB_COEF11 */
	 /* desc: YUV转RGB系数coef11 */
	 /* range: [11:0] */
	 #define    b_YUV2RGB_COEF11    ( 0 )
	 #define    m_YUV2RGB_COEF11    ( 0xfff << b_YUV2RGB_COEF11)


/*  YUV2RGB_COEF3  */
	 /* name: f_YUV2RGB_COEF12 */
	 /* desc: YUV转RGB系数coef12 */
	 /* range: [27:16] */
	 #define    b_YUV2RGB_COEF12    ( 16 )
	 #define    m_YUV2RGB_COEF12    ( 0xfff << b_YUV2RGB_COEF12)


	 /* name: f_YUV2RGB_COEF13 */
	 /* desc: YUV转RGB系数coef13 */
	 /* range: [11:0] */
	 #define    b_YUV2RGB_COEF13    ( 0 )
	 #define    m_YUV2RGB_COEF13    ( 0xfff << b_YUV2RGB_COEF13)


/*  YUV2RGB_COEF4  */
	 /* name: f_YUV2RGB_COEF20 */
	 /* desc: YUV转RGB系数coef20 */
	 /* range: [27:16] */
	 #define    b_YUV2RGB_COEF20    ( 16 )
	 #define    m_YUV2RGB_COEF20    ( 0xfff << b_YUV2RGB_COEF20)


	 /* name: f_YUV2RGB_COEF21 */
	 /* desc: YUV转RGB系数coef21 */
	 /* range: [11:0] */
	 #define    b_YUV2RGB_COEF21    ( 0 )
	 #define    m_YUV2RGB_COEF21    ( 0xfff << b_YUV2RGB_COEF21)


/*  YUV2RGB_COEF5  */
	 /* name: f_YUV2RGB_COEF22 */
	 /* desc: YUV转RGB系数coef22 */
	 /* range: [27:16] */
	 #define    b_YUV2RGB_COEF22    ( 16 )
	 #define    m_YUV2RGB_COEF22    ( 0xfff << b_YUV2RGB_COEF22)


	 /* name: f_YUV2RGB_COEF23 */
	 /* desc: YUV转RGB系数coef23 */
	 /* range: [11:0] */
	 #define    b_YUV2RGB_COEF23    ( 0 )
	 #define    m_YUV2RGB_COEF23    ( 0xfff << b_YUV2RGB_COEF23)


/*  RGB2YUV_COEF0  */
	 /* name: f_RGB2YUV_COEF00 */
	 /* desc: RGB转YUV系数coef00 */
	 /* range: [27:16] */
	 #define    b_RGB2YUV_COEF00    ( 16 )
	 #define    m_RGB2YUV_COEF00    ( 0xfff << b_RGB2YUV_COEF00)


	 /* name: f_RGB2YUV_COEF01 */
	 /* desc: RGB转YUV系数coef01 */
	 /* range: [11:0] */
	 #define    b_RGB2YUV_COEF01    ( 0 )
	 #define    m_RGB2YUV_COEF01    ( 0xfff << b_RGB2YUV_COEF01)


/*  RGB2YUV_COEF1  */
	 /* name: f_RGB2YUV_COEF02 */
	 /* desc: RGB转YUV系数coef02 */
	 /* range: [27:16] */
	 #define    b_RGB2YUV_COEF02    ( 16 )
	 #define    m_RGB2YUV_COEF02    ( 0xfff << b_RGB2YUV_COEF02)


	 /* name: f_RGB2YUV_COEF03 */
	 /* desc: RGB转YUV系数coef03 */
	 /* range: [11:0] */
	 #define    b_RGB2YUV_COEF03    ( 0 )
	 #define    m_RGB2YUV_COEF03    ( 0xfff << b_RGB2YUV_COEF03)


/*  RGB2YUV_COEF2  */
	 /* name: f_RGB2YUV_COEF10 */
	 /* desc: RGB转YUV系数coef10 */
	 /* range: [27:16] */
	 #define    b_RGB2YUV_COEF10    ( 16 )
	 #define    m_RGB2YUV_COEF10    ( 0xfff << b_RGB2YUV_COEF10)


	 /* name: f_RGB2YUV_COEF11 */
	 /* desc: RGB转YUV系数coef11 */
	 /* range: [11:0] */
	 #define    b_RGB2YUV_COEF11    ( 0 )
	 #define    m_RGB2YUV_COEF11    ( 0xfff << b_RGB2YUV_COEF11)


/*  RGB2YUV_COEF3  */
	 /* name: f_RGB2YUV_COEF12 */
	 /* desc: RGB转YUV系数coef12 */
	 /* range: [27:16] */
	 #define    b_RGB2YUV_COEF12    ( 16 )
	 #define    m_RGB2YUV_COEF12    ( 0xfff << b_RGB2YUV_COEF12)


	 /* name: f_RGB2YUV_COEF13 */
	 /* desc: RGB转YUV系数coef13 */
	 /* range: [11:0] */
	 #define    b_RGB2YUV_COEF13    ( 0 )
	 #define    m_RGB2YUV_COEF13    ( 0xfff << b_RGB2YUV_COEF13)


/*  RGB2YUV_COEF4  */
	 /* name: f_RGB2YUV_COEF20 */
	 /* desc: RGB转YUV系数coef20 */
	 /* range: [27:16] */
	 #define    b_RGB2YUV_COEF20    ( 16 )
	 #define    m_RGB2YUV_COEF20    ( 0xfff << b_RGB2YUV_COEF20)


	 /* name: f_RGB2YUV_COEF21 */
	 /* desc: RGB转YUV系数coef21 */
	 /* range: [11:0] */
	 #define    b_RGB2YUV_COEF21    ( 0 )
	 #define    m_RGB2YUV_COEF21    ( 0xfff << b_RGB2YUV_COEF21)


/*  RGB2YUV_COEF5  */
	 /* name: f_RGB2YUV_COEF22 */
	 /* desc: RGB转YUV系数coef22 */
	 /* range: [27:16] */
	 #define    b_RGB2YUV_COEF22    ( 16 )
	 #define    m_RGB2YUV_COEF22    ( 0xfff << b_RGB2YUV_COEF22)


	 /* name: f_RGB2YUV_COEF23 */
	 /* desc: RGB转YUV系数coef23 */
	 /* range: [11:0] */
	 #define    b_RGB2YUV_COEF23    ( 0 )
	 #define    m_RGB2YUV_COEF23    ( 0xfff << b_RGB2YUV_COEF23)


/*  HDR_R_MAP_VAL  */
	 /* name: f_HDR_R_MAP_MIN */
	 /* desc: R分量最小值 */
	 /* range: [26:16] */
	 #define    b_HDR_R_MAP_MIN    ( 16 )
	 #define    m_HDR_R_MAP_MIN    ( 0x7ff << b_HDR_R_MAP_MIN)


	 /* name: f_HDR_R_MAP_RECI */
	 /* desc: R分量映射倒数 */
	 /* range: [10:0] */
	 #define    b_HDR_R_MAP_RECI    ( 0 )
	 #define    m_HDR_R_MAP_RECI    ( 0x7ff << b_HDR_R_MAP_RECI)


/*  HDR_G_MAP_VAL  */
	 /* name: f_HDR_G_MAP_MIN */
	 /* desc: G分量最小值 */
	 /* range: [26:16] */
	 #define    b_HDR_G_MAP_MIN    ( 16 )
	 #define    m_HDR_G_MAP_MIN    ( 0x7ff << b_HDR_G_MAP_MIN)


	 /* name: f_HDR_G_MAP_RECI */
	 /* desc: G分量映射倒数 */
	 /* range: [10:0] */
	 #define    b_HDR_G_MAP_RECI    ( 0 )
	 #define    m_HDR_G_MAP_RECI    ( 0x7ff << b_HDR_G_MAP_RECI)


/*  HDR_B_MAP_VAL  */
	 /* name: f_HDR_B_MAP_MIN */
	 /* desc: B分量最小值 */
	 /* range: [26:16] */
	 #define    b_HDR_B_MAP_MIN    ( 16 )
	 #define    m_HDR_B_MAP_MIN    ( 0x7ff << b_HDR_B_MAP_MIN)


	 /* name: f_HDR_B_MAP_RECI */
	 /* desc: B分量映射倒数 */
	 /* range: [10:0] */
	 #define    b_HDR_B_MAP_RECI    ( 0 )
	 #define    m_HDR_B_MAP_RECI    ( 0x7ff << b_HDR_B_MAP_RECI)


/*  HDR_R_CENTER_VAL0  */
	 /* name: f_HDR_R_CENTER_0 */
	 /* desc: R分量曲线分段0中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_R_CENTER_0    ( 20 )
	 #define    m_HDR_R_CENTER_0    ( 0x3ff << b_HDR_R_CENTER_0)


	 /* name: f_HDR_R_CENTER_1 */
	 /* desc: R分量曲线分段1中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_R_CENTER_1    ( 10 )
	 #define    m_HDR_R_CENTER_1    ( 0x3ff << b_HDR_R_CENTER_1)


	 /* name: f_HDR_R_CENTER_2 */
	 /* desc: R分量曲线分段2中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_R_CENTER_2    ( 0 )
	 #define    m_HDR_R_CENTER_2    ( 0x3ff << b_HDR_R_CENTER_2)


/*  HDR_R_CENTER_VAL1  */
	 /* name: f_HDR_R_CENTER_3 */
	 /* desc: R分量曲线分段3中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_R_CENTER_3    ( 20 )
	 #define    m_HDR_R_CENTER_3    ( 0x3ff << b_HDR_R_CENTER_3)


	 /* name: f_HDR_R_CENTER_4 */
	 /* desc: R分量曲线分段4中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_R_CENTER_4    ( 10 )
	 #define    m_HDR_R_CENTER_4    ( 0x3ff << b_HDR_R_CENTER_4)


	 /* name: f_HDR_R_CENTER_5 */
	 /* desc: R分量曲线分段5中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_R_CENTER_5    ( 0 )
	 #define    m_HDR_R_CENTER_5    ( 0x3ff << b_HDR_R_CENTER_5)


/*  HDR_R_CENTER_VAL2  */
	 /* name: f_HDR_R_CENTER_6 */
	 /* desc: R分量曲线分段6中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_R_CENTER_6    ( 20 )
	 #define    m_HDR_R_CENTER_6    ( 0x3ff << b_HDR_R_CENTER_6)


	 /* name: f_HDR_R_CENTER_7 */
	 /* desc: R分量曲线分段7中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_R_CENTER_7    ( 10 )
	 #define    m_HDR_R_CENTER_7    ( 0x3ff << b_HDR_R_CENTER_7)


	 /* name: f_HDR_R_CENTER_8 */
	 /* desc: R分量曲线分段8中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_R_CENTER_8    ( 0 )
	 #define    m_HDR_R_CENTER_8    ( 0x3ff << b_HDR_R_CENTER_8)


/*  HDR_R_CENTER_VAL3  */
	 /* name: f_HDR_R_CENTER_9 */
	 /* desc: R分量曲线分段9中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_R_CENTER_9    ( 20 )
	 #define    m_HDR_R_CENTER_9    ( 0x3ff << b_HDR_R_CENTER_9)


	 /* name: f_HDR_R_CENTER_10 */
	 /* desc: R分量曲线分段10中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_R_CENTER_10    ( 10 )
	 #define    m_HDR_R_CENTER_10    ( 0x3ff << b_HDR_R_CENTER_10)


	 /* name: f_HDR_R_CENTER_11 */
	 /* desc: R分量曲线分段11中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_R_CENTER_11    ( 0 )
	 #define    m_HDR_R_CENTER_11    ( 0x3ff << b_HDR_R_CENTER_11)


/*  HDR_R_CENTER_VAL4  */
	 /* name: f_HDR_R_CENTER_12 */
	 /* desc: R分量曲线分段12中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_R_CENTER_12    ( 20 )
	 #define    m_HDR_R_CENTER_12    ( 0x3ff << b_HDR_R_CENTER_12)


	 /* name: f_HDR_R_CENTER_13 */
	 /* desc: R分量曲线分段13中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_R_CENTER_13    ( 10 )
	 #define    m_HDR_R_CENTER_13    ( 0x3ff << b_HDR_R_CENTER_13)


	 /* name: f_HDR_R_CENTER_14 */
	 /* desc: R分量曲线分段14中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_R_CENTER_14    ( 0 )
	 #define    m_HDR_R_CENTER_14    ( 0x3ff << b_HDR_R_CENTER_14)


/*  HDR_G_CENTER_VAL0  */
	 /* name: f_HDR_G_CENTER_0 */
	 /* desc: G分量曲线分段0中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_G_CENTER_0    ( 20 )
	 #define    m_HDR_G_CENTER_0    ( 0x3ff << b_HDR_G_CENTER_0)


	 /* name: f_HDR_G_CENTER_1 */
	 /* desc: G分量曲线分段1中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_G_CENTER_1    ( 10 )
	 #define    m_HDR_G_CENTER_1    ( 0x3ff << b_HDR_G_CENTER_1)


	 /* name: f_HDR_G_CENTER_2 */
	 /* desc: G分量曲线分段2中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_G_CENTER_2    ( 0 )
	 #define    m_HDR_G_CENTER_2    ( 0x3ff << b_HDR_G_CENTER_2)


/*  HDR_G_CENTER_VAL1  */
	 /* name: f_HDR_G_CENTER_3 */
	 /* desc: G分量曲线分段3中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_G_CENTER_3    ( 20 )
	 #define    m_HDR_G_CENTER_3    ( 0x3ff << b_HDR_G_CENTER_3)


	 /* name: f_HDR_G_CENTER_4 */
	 /* desc: G分量曲线分段4中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_G_CENTER_4    ( 10 )
	 #define    m_HDR_G_CENTER_4    ( 0x3ff << b_HDR_G_CENTER_4)


	 /* name: f_HDR_G_CENTER_5 */
	 /* desc: G分量曲线分段5中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_G_CENTER_5    ( 0 )
	 #define    m_HDR_G_CENTER_5    ( 0x3ff << b_HDR_G_CENTER_5)


/*  HDR_G_CENTER_VAL2  */
	 /* name: f_HDR_G_CENTER_6 */
	 /* desc: G分量曲线分段6中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_G_CENTER_6    ( 20 )
	 #define    m_HDR_G_CENTER_6    ( 0x3ff << b_HDR_G_CENTER_6)


	 /* name: f_HDR_G_CENTER_7 */
	 /* desc: G分量曲线分段7中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_G_CENTER_7    ( 10 )
	 #define    m_HDR_G_CENTER_7    ( 0x3ff << b_HDR_G_CENTER_7)


	 /* name: f_HDR_G_CENTER_8 */
	 /* desc: G分量曲线分段8中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_G_CENTER_8    ( 0 )
	 #define    m_HDR_G_CENTER_8    ( 0x3ff << b_HDR_G_CENTER_8)


/*  HDR_G_CENTER_VAL3  */
	 /* name: f_HDR_G_CENTER_9 */
	 /* desc: G分量曲线分段9中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_G_CENTER_9    ( 20 )
	 #define    m_HDR_G_CENTER_9    ( 0x3ff << b_HDR_G_CENTER_9)


	 /* name: f_HDR_G_CENTER_10 */
	 /* desc: G分量曲线分段10中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_G_CENTER_10    ( 10 )
	 #define    m_HDR_G_CENTER_10    ( 0x3ff << b_HDR_G_CENTER_10)


	 /* name: f_HDR_G_CENTER_11 */
	 /* desc: G分量曲线分段11中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_G_CENTER_11    ( 0 )
	 #define    m_HDR_G_CENTER_11    ( 0x3ff << b_HDR_G_CENTER_11)


/*  HDR_G_CENTER_VAL4  */
	 /* name: f_HDR_G_CENTER_12 */
	 /* desc: G分量曲线分段12中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_G_CENTER_12    ( 20 )
	 #define    m_HDR_G_CENTER_12    ( 0x3ff << b_HDR_G_CENTER_12)


	 /* name: f_HDR_G_CENTER_13 */
	 /* desc: G分量曲线分段13中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_G_CENTER_13    ( 10 )
	 #define    m_HDR_G_CENTER_13    ( 0x3ff << b_HDR_G_CENTER_13)


	 /* name: f_HDR_G_CENTER_14 */
	 /* desc: G分量曲线分段14中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_G_CENTER_14    ( 0 )
	 #define    m_HDR_G_CENTER_14    ( 0x3ff << b_HDR_G_CENTER_14)


/*  HDR_B_CENTER_VAL0  */
	 /* name: f_HDR_B_CENTER_0 */
	 /* desc: B分量曲线分段0中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_B_CENTER_0    ( 20 )
	 #define    m_HDR_B_CENTER_0    ( 0x3ff << b_HDR_B_CENTER_0)


	 /* name: f_HDR_B_CENTER_1 */
	 /* desc: B分量曲线分段1中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_B_CENTER_1    ( 10 )
	 #define    m_HDR_B_CENTER_1    ( 0x3ff << b_HDR_B_CENTER_1)


	 /* name: f_HDR_B_CENTER_2 */
	 /* desc: B分量曲线分段2中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_B_CENTER_2    ( 0 )
	 #define    m_HDR_B_CENTER_2    ( 0x3ff << b_HDR_B_CENTER_2)


/*  HDR_B_CENTER_VAL1  */
	 /* name: f_HDR_B_CENTER_3 */
	 /* desc: B分量曲线分段3中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_B_CENTER_3    ( 20 )
	 #define    m_HDR_B_CENTER_3    ( 0x3ff << b_HDR_B_CENTER_3)


	 /* name: f_HDR_B_CENTER_4 */
	 /* desc: B分量曲线分段4中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_B_CENTER_4    ( 10 )
	 #define    m_HDR_B_CENTER_4    ( 0x3ff << b_HDR_B_CENTER_4)


	 /* name: f_HDR_B_CENTER_5 */
	 /* desc: B分量曲线分段5中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_B_CENTER_5    ( 0 )
	 #define    m_HDR_B_CENTER_5    ( 0x3ff << b_HDR_B_CENTER_5)


/*  HDR_B_CENTER_VAL2  */
	 /* name: f_HDR_B_CENTER_6 */
	 /* desc: B分量曲线分段6中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_B_CENTER_6    ( 20 )
	 #define    m_HDR_B_CENTER_6    ( 0x3ff << b_HDR_B_CENTER_6)


	 /* name: f_HDR_B_CENTER_7 */
	 /* desc: B分量曲线分段7中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_B_CENTER_7    ( 10 )
	 #define    m_HDR_B_CENTER_7    ( 0x3ff << b_HDR_B_CENTER_7)


	 /* name: f_HDR_B_CENTER_8 */
	 /* desc: B分量曲线分段8中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_B_CENTER_8    ( 0 )
	 #define    m_HDR_B_CENTER_8    ( 0x3ff << b_HDR_B_CENTER_8)


/*  HDR_B_CENTER_VAL3  */
	 /* name: f_HDR_B_CENTER_9 */
	 /* desc: B分量曲线分段9中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_B_CENTER_9    ( 20 )
	 #define    m_HDR_B_CENTER_9    ( 0x3ff << b_HDR_B_CENTER_9)


	 /* name: f_HDR_B_CENTER_10 */
	 /* desc: B分量曲线分段10中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_B_CENTER_10    ( 10 )
	 #define    m_HDR_B_CENTER_10    ( 0x3ff << b_HDR_B_CENTER_10)


	 /* name: f_HDR_B_CENTER_11 */
	 /* desc: B分量曲线分段11中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_B_CENTER_11    ( 0 )
	 #define    m_HDR_B_CENTER_11    ( 0x3ff << b_HDR_B_CENTER_11)


/*  HDR_B_CENTER_VAL4  */
	 /* name: f_HDR_B_CENTER_12 */
	 /* desc: B分量曲线分段12中心位置数值 */
	 /* range: [29:20] */
	 #define    b_HDR_B_CENTER_12    ( 20 )
	 #define    m_HDR_B_CENTER_12    ( 0x3ff << b_HDR_B_CENTER_12)


	 /* name: f_HDR_B_CENTER_13 */
	 /* desc: B分量曲线分段13中心位置数值 */
	 /* range: [19:10] */
	 #define    b_HDR_B_CENTER_13    ( 10 )
	 #define    m_HDR_B_CENTER_13    ( 0x3ff << b_HDR_B_CENTER_13)


	 /* name: f_HDR_B_CENTER_14 */
	 /* desc: B分量曲线分段14中心位置数值 */
	 /* range: [9:0] */
	 #define    b_HDR_B_CENTER_14    ( 0 )
	 #define    m_HDR_B_CENTER_14    ( 0x3ff << b_HDR_B_CENTER_14)


/*  HDR_R_CURVE_VAL  */
	 /* name: f_HDR_R_CURVE_VALxx */
	 /* desc: R分量曲线系数xx */
	 /* range: [31:0] */
	 #define    b_HDR_R_CURVE_VALXX    ( 0 )
	 #define    m_HDR_R_CURVE_VALXX    ( 0xffffffff << b_HDR_R_CURVE_VALXX)


/*  HDR_G_CURVE_VAL  */
	 /* name: f_HDR_G_CURVE_VALxx */
	 /* desc: G分量曲线系数xx */
	 /* range: [31:0] */
	 #define    b_HDR_G_CURVE_VALXX    ( 0 )
	 #define    m_HDR_G_CURVE_VALXX    ( 0xffffffff << b_HDR_G_CURVE_VALXX)


/*  HDR_B_CURVE_VAL  */
	 /* name: f_HDR_B_CURVE_VALxx */
	 /* desc: B分量曲线系数xx */
	 /* range: [31:0] */
	 #define    b_HDR_B_CURVE_VALXX    ( 0 )
	 #define    m_HDR_B_CURVE_VALXX    ( 0xffffffff << b_HDR_B_CURVE_VALXX)


#endif

