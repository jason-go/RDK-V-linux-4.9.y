#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gx3211_vpu.h"
#include "gx3211_vpu_internel.h"

void gx3211_vpu_VbiEnable(Gx3211VpuVbi *vbi)
{
	if(vbi->enable != 0) {
		VPU_VBI_START(gx3211vpu_reg->rVBI_CTRL);
	}
    else {
		VPU_VBI_STOP(gx3211vpu_reg->rVBI_CTRL);
		gx_free(gx3211vpu_info->vbi.data_address);
	}
}

void gx3211_vpu_VbiGetReadPtr(Gx3211VpuVbi *vbi)
{
	VPU_VBI_GET_READ_ADDR(gx3211vpu_reg->rVBI_ADDR, gx3211vpu_info->vbi.read_address);
}

int gx3211_vpu_VbiCreateBuffer(Gx3211VpuVbi *vbi)
{
	VPU_VBI_SET_DATA_LEN(gx3211vpu_reg->rVBI_CTRL, gx3211vpu_info->vbi.unit_length);
	VPU_VBI_SET_DATA_ADDR(gx3211vpu_reg->rVBI_FIRST_ADDR, (unsigned int)gx3211vpu_info->vbi.data_address);

	return 0;
}

