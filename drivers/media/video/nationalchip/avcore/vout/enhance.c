#include "enhance.h"
#include "enhance_reg.h"
#include "gxav.h"
#include "gxav_bitops.h"
#include "kernelcalls.h"

static struct cygnus_vpu_enhance_reg *regs[2];

#define GET_REG_BY_ID(id) (regs[id])

int enhance_init(void)
{
	unsigned i, base_addr[] = {0x0480c000, 0x0480d000};

	if (CHIP_IS_CYGNUS) {
		for (i = 0; i < 2; i++) {
			if (0 == gx_request_mem_region(base_addr[i], sizeof(struct cygnus_vpu_enhance_reg))) {
				gx_printf("%s:%d, request_mem_region failed", __func__, __LINE__);
				goto err_out;
			}
			regs[i] = gx_ioremap(base_addr[i], sizeof(struct cygnus_vpu_enhance_reg));
			if (!regs[i]) {
				gx_printf("%s:%d, ioremap failed.\n", __func__, __LINE__);
				goto err_out;
			}
		}
	}

err_out:
	return -1;
}

int enhance_set_brightness(EnhanceID id, unsigned brightness)
{
	unsigned int temp, value;
	struct cygnus_vpu_enhance_reg *reg = GET_REG_BY_ID(id);

	temp = REG_GET_VAL(&reg->rPRM_BLEC_BRT_CFG);
	temp = temp & 0xff00ffff;
	value = brightness*2 + 28;
	REG_SET_VAL((&reg->rPRM_BLEC_BRT_CFG), (temp|(value<<16)));

	return 0;
}

int enhance_set_saturation(EnhanceID id, unsigned saturation)
{
	unsigned int temp, value;
	struct cygnus_vpu_enhance_reg *reg = GET_REG_BY_ID(id);

	temp = REG_GET_VAL(&reg->rPRM_HUE_SAT_CON_CFG);
	temp = temp & 0xffffc0ff;
	value = saturation/2 + 7;
	REG_SET_VAL(&reg->rPRM_HUE_SAT_CON_CFG, (temp|(value<<8)));

	return 0;
}

int enhance_set_contrast(EnhanceID id, unsigned contrast)
{
	unsigned int temp, value;
	struct cygnus_vpu_enhance_reg *reg = GET_REG_BY_ID(id);

	temp = REG_GET_VAL(&reg->rPRM_HUE_SAT_CON_CFG);
	temp = temp & 0xffc0ffff;
	value = contrast/2 + 7;
	REG_SET_VAL(&reg->rPRM_HUE_SAT_CON_CFG, (temp|(value<<16)));

	return 0;
}


int enhance_set_sharpness(EnhanceID id, unsigned sharpness)
{
	unsigned dif_core = 5;//0~255
	unsigned tot_core = 5;//0~255
	unsigned tot_limit = 85;//0~255
	unsigned en = 1;
	unsigned pos_gain = 10;//0~63
	unsigned lvl_gain = 10;//0~63
	unsigned tot_gain = 0;//0~63
	unsigned gain_dly = 3;//0~7
	struct cygnus_vpu_enhance_reg *reg = GET_REG_BY_ID(id);

	tot_gain = (sharpness/10)*4;
	REG_SET_VAL(&reg->rPRM_LTI_CFG_1, ((gain_dly<<24)|(tot_gain<<16)|(lvl_gain<<8)|pos_gain));
	REG_SET_VAL(&reg->rPRM_LTI_CFG_0, ((en << 24)|(tot_limit<<16)|(tot_core<<8)|dif_core));

	return 0;
}

int enhance_set_hue(EnhanceID id, unsigned hue)
{
	unsigned temp, value;
	struct cygnus_vpu_enhance_reg *reg = GET_REG_BY_ID(id);

	temp = REG_GET_VAL(&reg->rPRM_HUE_SAT_CON_CFG);
	temp = temp & 0xffffff80;
	value = (hue>>1);
	if (hue < 50)
	  value = 64 + 25 - value;
	else
	  value = value - 25;
	REG_SET_VAL(&reg->rPRM_HUE_SAT_CON_CFG, (temp|value));

	return 0;
}

