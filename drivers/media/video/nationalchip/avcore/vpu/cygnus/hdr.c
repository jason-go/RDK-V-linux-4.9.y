#include "cygnus_vpu_reg.h"
#include "cygnus_vpu_internel.h"

#include "hdr.h"
#include "hdr_param.h"

typedef enum {
	SDR   = (1<<0),
	HDR   = (1<<1),
	HLG   = (1<<2),
} DynamicRangeType;

struct hdr_info {
	DynamicRangeType stream, tv;
} cygnus_hdr_info;

extern struct hdr_param ali_hdr;
extern struct hdr_param ali_hlg;

//使能模块
static void hdr_en(int en)
{
	struct cygnus_vpu_hdr_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_HDR);

	if (en)
		REG_SET_BIT(&(reg->rHDR_CTRL), b_HDR_ENABLE);
	else
		REG_CLR_BIT(&(reg->rHDR_CTRL), b_HDR_ENABLE);
}

static void hdr_load_map_val(DynamicRangeType type)
{
	struct cygnus_vpu_hdr_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_HDR);

	if (type == HDR) {
		REG_SET_VAL((&reg->rHDR_R_MAP_VAL), ali_hdr.r.MAP_VAL);
		REG_SET_VAL((&reg->rHDR_G_MAP_VAL), ali_hdr.g.MAP_VAL);
		REG_SET_VAL((&reg->rHDR_B_MAP_VAL), ali_hdr.b.MAP_VAL);
	}

	if (type == HLG) {
		REG_SET_VAL((&reg->rHDR_R_MAP_VAL), ali_hlg.r.MAP_VAL);
		REG_SET_VAL((&reg->rHDR_G_MAP_VAL), ali_hlg.g.MAP_VAL);
		REG_SET_VAL((&reg->rHDR_B_MAP_VAL), ali_hlg.b.MAP_VAL);
	}
}

static void hdr_load_center_val(DynamicRangeType type)
{
	struct cygnus_vpu_hdr_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_HDR);

	if (type == HDR) {
		//R
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL0), ali_hdr.r.CENTER_VAL0);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL1), ali_hdr.r.CENTER_VAL1);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL2), ali_hdr.r.CENTER_VAL2);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL3), ali_hdr.r.CENTER_VAL3);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL4), ali_hdr.r.CENTER_VAL4);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL5), ali_hdr.r.CENTER_VAL5);
		//G
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL0), ali_hdr.g.CENTER_VAL0);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL1), ali_hdr.g.CENTER_VAL1);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL2), ali_hdr.g.CENTER_VAL2);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL3), ali_hdr.g.CENTER_VAL3);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL4), ali_hdr.g.CENTER_VAL4);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL5), ali_hdr.g.CENTER_VAL5);
		//B
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL0), ali_hdr.b.CENTER_VAL0);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL1), ali_hdr.b.CENTER_VAL1);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL2), ali_hdr.b.CENTER_VAL2);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL3), ali_hdr.b.CENTER_VAL3);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL4), ali_hdr.b.CENTER_VAL4);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL5), ali_hdr.b.CENTER_VAL5);
	}

	if (type == HLG) {
		//R
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL0), ali_hlg.r.CENTER_VAL0);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL1), ali_hlg.r.CENTER_VAL1);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL2), ali_hlg.r.CENTER_VAL2);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL3), ali_hlg.r.CENTER_VAL3);
		REG_SET_VAL((&reg->rHDR_R_CENTER_VAL4), ali_hlg.r.CENTER_VAL4);
		//G
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL0), ali_hlg.g.CENTER_VAL0);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL1), ali_hlg.g.CENTER_VAL1);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL2), ali_hlg.g.CENTER_VAL2);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL3), ali_hlg.g.CENTER_VAL3);
		REG_SET_VAL((&reg->rHDR_G_CENTER_VAL4), ali_hlg.g.CENTER_VAL4);
		//B
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL0), ali_hlg.b.CENTER_VAL0);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL1), ali_hlg.b.CENTER_VAL1);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL2), ali_hlg.b.CENTER_VAL2);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL3), ali_hlg.b.CENTER_VAL3);
		REG_SET_VAL((&reg->rHDR_B_CENTER_VAL4), ali_hlg.b.CENTER_VAL4);
	}
}

static void hdr_load_curve_val(DynamicRangeType type)
{
	int i;
	struct cygnus_vpu_hdr_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_HDR);

	if (type == HDR) {
		for (i = 0; i < 96; i++)
			REG_SET_VAL((&reg->rHDR_R_CURVE_VAL[i]), ali_hdr.r.CURVE_VAL[i]);
		for (i = 0; i < 96; i++)
			REG_SET_VAL((&reg->rHDR_G_CURVE_VAL[i]), ali_hdr.g.CURVE_VAL[i]);
		for (i = 0; i < 96; i++)
			REG_SET_VAL((&reg->rHDR_B_CURVE_VAL[i]), ali_hdr.b.CURVE_VAL[i]);
	}

	if (type == HLG) {
		for (i = 96; i < 192; i++)
			REG_SET_VAL((&reg->rHDR_R_CURVE_VAL[i]), ali_hlg.r.CURVE_VAL[i - 96]);
		for (i = 96; i < 192; i++)
			REG_SET_VAL((&reg->rHDR_G_CURVE_VAL[i]), ali_hlg.g.CURVE_VAL[i - 96]);
		for (i = 96; i < 192; i++)
			REG_SET_VAL((&reg->rHDR_B_CURVE_VAL[i]), ali_hlg.b.CURVE_VAL[i - 96]);
	}
}

//修改曲线数读指针偏移
static void hdr_change_curve_rd_ptr(DynamicRangeType type)
{
	struct cygnus_vpu_hdr_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_HDR);

	if (type == HDR)
		REG_SET_FIELD((&reg->rHDR_CTRL), m_HDR_READ_INDEX, 0, b_HDR_READ_INDEX);
	if (type == HLG)
		REG_SET_FIELD((&reg->rHDR_CTRL), m_HDR_READ_INDEX, 8, b_HDR_READ_INDEX);
}

//切换模块工作模式
void hdr_set_work_mode(HdrWorkMode mode)
{
	if (mode == HDR_TO_SDR) {
		hdr_en(0);
		hdr_load_map_val(HDR);
		hdr_load_center_val(HDR);
		hdr_change_curve_rd_ptr(HDR);
		hdr_en(1);
	}
	if (mode == HLG_TO_SDR) {
		hdr_en(0);
		hdr_load_map_val(HLG);
		hdr_load_center_val(HLG);
		hdr_change_curve_rd_ptr(HLG);
		hdr_en(1);
	}
	if (mode == BYPASS)
		hdr_en(0);
}


void hdr_init(void)
{
	hdr_load_curve_val(HDR);
	hdr_load_curve_val(HLG);
}

////监控stream和tv信息，实时更新参数
//void hdr_update(void)
//{
//	DynamicRangeType tv     = HDR;//gxav_hdmi_get_hdr_info();
//	DynamicRangeType stream = HDR;//gx_video_get_hdr_info();
//
//	if (tv != cygnus_hdr_info.tv || stream != cygnus_hdr_info.tv) {
//		if (stream == HDR && tv == SDR) {
//			hdr_en(0);
//			hdr_set_work_mode(HDR_TO_SDR);
//			hdr_en(1);
//		} else if (stream == HLG && tv == SDR) {
//			hdr_en(0);
//			hdr_set_work_mode(HLG_TO_SDR);
//			hdr_en(1);
//		} else {
//			hdr_set_work_mode(BYPASS);
//		}
//	}
//
//	cygnus_hdr_info.tv     = tv;
//	cygnus_hdr_info.stream = stream;
//}
//
