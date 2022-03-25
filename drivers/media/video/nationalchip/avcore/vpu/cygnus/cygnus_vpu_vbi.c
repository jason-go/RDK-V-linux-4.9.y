#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "cygnus_vpu.h"
#include "cygnus_vpu_internel.h"

void cygnus_vpu_VbiEnable(CygnusVpuVbi *vbi)
{
#if 0
	if(vbi->enable != 0) {
		VPU_VBI_START(cygnusvpu_reg->rVBI_CTRL);
	}
    else {
		VPU_VBI_STOP(cygnusvpu_reg->rVBI_CTRL);
		gx_free(cygnusvpu_info->vbi.data_address);
	}
#endif
}

void cygnus_vpu_VbiGetReadPtr(CygnusVpuVbi *vbi)
{
#if 0
	VPU_VBI_GET_READ_ADDR(cygnusvpu_reg->rVBI_ADDR, cygnusvpu_info->vbi.read_address);
#endif
}

int cygnus_vpu_VbiCreateBuffer(CygnusVpuVbi *vbi)
{
#if 0
	VPU_VBI_SET_DATA_LEN(cygnusvpu_reg->rVBI_CTRL, cygnusvpu_info->vbi.unit_length);
	VPU_VBI_SET_DATA_ADDR(cygnusvpu_reg->rVBI_FIRST_ADDR, (unsigned int)cygnusvpu_info->vbi.data_address);
#endif
	return 0;
}

