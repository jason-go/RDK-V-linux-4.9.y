#include "cygnus_vpu_reg.h"
#include "cygnus_vpu_reg.h"
#include "kernelcalls.h"

static struct _cygnus_vpu_reg_info {
	unsigned base_addr;
	struct sub_mod {
		char *name;
		unsigned offset;
		unsigned len;
		void *reg;
	} sub[CYGNUS_VPU_SUB_MAX];
} cygnusvpu_reg = {
	.base_addr = 0x04800000,
	.sub = {
		{.name = "sys"  , .offset = 0x0000, .len = sizeof(struct cygnus_vpu_sys_reg)},
		{.name = "pp0"  , .offset = 0x1000, .len = sizeof(struct cygnus_vpu_pp_reg)},
		{.name = "pp1"  , .offset = 0x2000, .len = sizeof(struct cygnus_vpu_pp_reg)},
		{.name = "pic"  , .offset = 0x3000, .len = sizeof(struct cygnus_vpu_pic_reg)},
		{.name = "osd"  , .offset = 0x4000, .len = sizeof(struct cygnus_vpu_osd_reg)},
		{.name = "osd1" , .offset = 0x5000, .len = sizeof(struct cygnus_vpu_osd_reg)},
		{.name = "pp2"  , .offset = 0x6000, .len = sizeof(struct cygnus_vpu_pp2_reg)},
		{.name = "ce"   , .offset = 0x7000, .len = sizeof(struct cygnus_vpu_ce_reg)},
		{.name = "vout0", .offset = 0x8000, .len = sizeof(struct cygnus_vpu_ce_reg)},
		{.name = "vout1", .offset = 0x9000, .len = sizeof(struct cygnus_vpu_ce_reg)},
		{.name = "hdr"  , .offset = 0xa000, .len = sizeof(struct cygnus_vpu_hdr_reg)},
	},
};

int cygnus_vpu_reg_iounmap(void)
{
	int i;
	unsigned base_addr, len;

	for (i = CYGNUS_VPU_SUB_SYS; i < CYGNUS_VPU_SUB_MAX; i++) {
		if (cygnusvpu_reg.sub[i].reg != NULL) {
			base_addr = cygnusvpu_reg.base_addr + cygnusvpu_reg.sub[i].offset;
			len       = cygnusvpu_reg.sub[i].len;
			gx_iounmap(cygnusvpu_reg.sub[i].reg);
			gx_release_mem_region(base_addr, len);
			cygnusvpu_reg.sub[i].reg = NULL;
		}
	}

	return 0;
}

int cygnus_vpu_reg_ioremap(void)
{
	int i;
	unsigned base_addr, len;

	for (i = CYGNUS_VPU_SUB_SYS; i < CYGNUS_VPU_SUB_MAX; i++) {
		base_addr = cygnusvpu_reg.base_addr + cygnusvpu_reg.sub[i].offset;
		len       = cygnusvpu_reg.sub[i].len;
		if (!gx_request_mem_region(base_addr, len)) {
			gx_printf("request_mem_region %s failed", cygnusvpu_reg.sub[i].name);
			goto vpu_iomap_error;
		}
		cygnusvpu_reg.sub[i].reg = gx_ioremap(base_addr, len);
		if (!cygnusvpu_reg.sub[i].reg)
			goto vpu_iomap_error;
	}

	return (0);

vpu_iomap_error:
	cygnus_vpu_reg_iounmap();
	return -1;
}

void* get_reg_by_sub_id(int id)
{
	void *reg = NULL;

	if (id >= CYGNUS_VPU_SUB_SYS && id < CYGNUS_VPU_SUB_MAX)
		reg = cygnusvpu_reg.sub[id].reg;

	return reg;
}

