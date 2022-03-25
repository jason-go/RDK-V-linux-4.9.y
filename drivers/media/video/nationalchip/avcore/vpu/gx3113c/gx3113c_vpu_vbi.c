#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gx3113c_vpu.h"
#include "gx3113c_vpu_internel.h"

void gx3113c_vpu_VbiEnable(Gx3113cVpuVbi *vbi)
{
	if(vbi->enable != 0) {
		VPU_VBI_START(gx3113cvpu_reg->rVBI_CTRL);
	}
    else {
		VPU_VBI_STOP(gx3113cvpu_reg->rVBI_CTRL);
		gx_free(gx3113cvpu_info->vbi.data_address);
	}
}

void gx3113c_vpu_VbiGetReadPtr(Gx3113cVpuVbi *vbi)
{
	VPU_VBI_GET_READ_ADDR(gx3113cvpu_reg->rVBI_ADDR, gx3113cvpu_info->vbi.read_address);
}

int gx3113c_vpu_VbiCreateBuffer(Gx3113cVpuVbi *vbi)
{
	VPU_VBI_SET_DATA_LEN(gx3113cvpu_reg->rVBI_CTRL, gx3113cvpu_info->vbi.unit_length);
	VPU_VBI_SET_DATA_ADDR(gx3113cvpu_reg->rVBI_FIRST_ADDR, (unsigned int)gx3113cvpu_info->vbi.data_address);

	return 0;
}

