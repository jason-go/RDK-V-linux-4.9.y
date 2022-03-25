#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "sirius_vpu.h"
#include "sirius_vpu_internel.h"

void sirius_vpu_VbiEnable(SiriusVpuVbi *vbi)
{
	if(vbi->enable != 0) {
		VPU_VBI_START(siriusvpu_reg->rVBI_CTRL);
	}
    else {
		VPU_VBI_STOP(siriusvpu_reg->rVBI_CTRL);
		gx_free(siriusvpu_info->vbi.data_address);
	}
}

void sirius_vpu_VbiGetReadPtr(SiriusVpuVbi *vbi)
{
	VPU_VBI_GET_READ_ADDR(siriusvpu_reg->rVBI_ADDR, siriusvpu_info->vbi.read_address);
}

int sirius_vpu_VbiCreateBuffer(SiriusVpuVbi *vbi)
{
	VPU_VBI_SET_DATA_LEN(siriusvpu_reg->rVBI_CTRL, siriusvpu_info->vbi.unit_length);
	VPU_VBI_SET_DATA_ADDR(siriusvpu_reg->rVBI_FIRST_ADDR, (unsigned int)siriusvpu_info->vbi.data_address);

	return 0;
}

