#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gx3201_vpu.h"
#include "gx3201_vpu_internel.h"

void gx3201_vpu_VbiEnable(Gx3201VpuVbi *vbi)
{
	if(vbi->enable != 0) {
		VPU_VBI_START(gx3201vpu_reg->rVBI_CTRL);
	}
    else {
		VPU_VBI_STOP(gx3201vpu_reg->rVBI_CTRL);
		gx_free(gx3201vpu_info->vbi.data_address);
	}
}

void gx3201_vpu_VbiGetReadPtr(Gx3201VpuVbi *vbi)
{
	VPU_VBI_GET_READ_ADDR(gx3201vpu_reg->rVBI_ADDR, gx3201vpu_info->vbi.read_address);
}

int gx3201_vpu_VbiCreateBuffer(Gx3201VpuVbi *vbi)
{
	VPU_VBI_SET_DATA_LEN(gx3201vpu_reg->rVBI_CTRL, gx3201vpu_info->vbi.unit_length);
	VPU_VBI_SET_DATA_ADDR(gx3201vpu_reg->rVBI_FIRST_ADDR, (unsigned int)gx3201vpu_info->vbi.data_address);

	return 0;
}

