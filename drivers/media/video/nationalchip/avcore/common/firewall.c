#include "kernelcalls.h"
#include "gxav_common.h"
#include "secure_hal.h"

#define FIREWALL_MAX_BUF_ID (24)

struct gxav_firewall_buf_node {
	int busy;
	GxAvBufferType type;
	unsigned int   phy_addr;
	unsigned int   size;
};

static struct firewall {
	unsigned protect_flag;
	unsigned align_flag;
	struct gxav_firewall_buf_node bufs[FIREWALL_MAX_BUF_ID];
	struct gx_mem_info videomem;
	unsigned videomem_protected;
} fw = {0,  };


static int _firewall_find_buffer(GxAvBufferType type, unsigned int buf_phy_addr, unsigned int buf_size)
{
	int i, ret_idx = -1;

	for (i = 0; i < FIREWALL_MAX_BUF_ID; i++) {
		if (fw.bufs[i].busy == 1 && fw.bufs[i].type == type && fw.bufs[i].phy_addr == buf_phy_addr &&fw.bufs[i].size == buf_size) {
			ret_idx = i;
			break;
		}
	}

	return ret_idx;
}

static int _firewall_note_buffer(GxAvBufferType type, unsigned int buf_phy_addr, unsigned int buf_size)
{
	int i, ret_idx = -1;

	for (i = 0; i < FIREWALL_MAX_BUF_ID; i++) {
		if (fw.bufs[i].busy == 0) {
			fw.bufs[i].busy = 1;
			fw.bufs[i].type = type;
			fw.bufs[i].size = buf_size;
			fw.bufs[i].phy_addr = buf_phy_addr;
			ret_idx = i;
			break;
		}
	}

	return ret_idx;
}

static void _firewall_sirius_patch(void)
{
// NOS videomem 不能用,HwMalloc会转换为系统内存.
// Sirius bug; softmode 整个videomem会被保护起来, 节省内存.
#ifndef NO_OS
	if (CHIP_IS_SIRIUS) {
		if (fw.protect_flag != 0 && (fw.protect_flag & GXAV_BUFFER_VIDEO_FRAME) == 0) { //soft mode
			int ret = gx_mem_info_get("videomem", &fw.videomem);
			if(ret == 0 && fw.videomem.start) {
				fw.videomem_protected = 1;
				 gxav_secure_config_filter((int)fw.videomem.start, (int)fw.videomem.size, 0xFFFFFFFF, 0xFFFFFFFF);
			}
		}
	}
#endif
}

static int _firewall_sirius_patch_check(GxAvBufferType type, unsigned int buf_phy_addr, unsigned int buf_size)
{
// NOS videomem 不能用,HwMalloc会转换为系统内存.
// Sirius bug; softmode 整个videomem已经被保护起来.
#ifndef NO_OS
	if (fw.videomem_protected) {
		if ((buf_phy_addr >= fw.videomem.start && buf_phy_addr < fw.videomem.start + fw.videomem.size) ||
				(buf_phy_addr+buf_size >= fw.videomem.start && buf_phy_addr+buf_size < fw.videomem.start + fw.videomem.size)){
			//gx_printf("[warning]:%s: buffer:%d, addr = 0x%x !####\n", __func__, type, buf_phy_addr);
			return -1;
		}
	}
#endif
	return 0;
}

int gxav_firewall_init(void)
{
	memset(&fw.bufs, 0, sizeof(fw.bufs));
	memset(&fw.videomem, 0, sizeof(fw.videomem));
	fw.protect_flag = gxav_secure_get_protect_buffer();
	fw.align_flag = (gxav_secure_query_access_align() == 4) ? 1 : 0;
	fw.videomem_protected = 0;

	gx_printf("%s. [protect_flag]:0x%x, [align_flag]:0x%x\n", __func__, fw.protect_flag, fw.align_flag);
	_firewall_sirius_patch();
	return 0;
}

int gxav_firewall_register_buffer(GxAvBufferType type, unsigned int buf_phy_addr, unsigned int buf_size)
{
	int master_read = 0, master_write = 0;

	if (fw.protect_flag == 0)
		return 0;

	if ((type & fw.protect_flag) == 0) {
		if (!CHIP_IS_SIRIUS)
			return 0;
		// Sirius 默认权限 bug;
		master_read  = 0x7FFFFFFF;
		master_write = 0x7FFFFFFF;
	}

	if (_firewall_sirius_patch_check(type, buf_phy_addr, buf_size) != 0)
		return -1;

	if (_firewall_find_buffer(type, buf_phy_addr, buf_size) != -1)
		return 0;

	switch (type) {
	case GXAV_BUFFER_DEMUX_ES:
		master_write |= MASTER_DEMUX_ES | MASTER_M2M_PVR_DECRYPT;
		master_read  |= MASTER_VIDEO_FW | MASTER_AUDIODEC_CM | MASTER_M2M_PVR_ENCRYPT;
		break;
	case GXAV_BUFFER_DEMUX_TSW:
	case GXAV_BUFFER_DEMUX_TSR:
		master_write |= MASTER_DEMUX_TS_WRITE | MASTER_M2M_PVR_DECRYPT;
		master_read  |= MASTER_DEMUX_TS_READ | MASTER_M2M_PVR_ENCRYPT;
		break;
	case GXAV_BUFFER_GP_ES:
		master_write |= MASTER_GP_WRITE | MASTER_M2M_PVR_DECRYPT;
		master_read  |= MASTER_GP_READ | MASTER_VIDEO_FW | MASTER_AUDIODEC_CM | MASTER_M2M_PVR_ENCRYPT;
		break;
	case GXAV_BUFFER_AUDIO_FIRMWARE:
		master_write |= MASTER_M2M_FW_DECRYPT;
		master_read  |= MASTER_AUDIODEC_FW | MASTER_RCC | MASTER_HASH;
		break;
	case GXAV_BUFFER_AUDIO_FRAME:
		master_write |= MASTER_AUDIODEC_CM;
		master_read  |= MASTER_AUDIODEC_CM | MASTER_AUDIOPLAY;
		break;
	case GXAV_BUFFER_VIDEO_FIRMWARE:
		master_write |= MASTER_M2M_FW_DECRYPT;
		master_read  |= MASTER_VIDEO_FW | MASTER_RCC | MASTER_HASH;
		break;
		/*
		 * sirius hardware bug : video frame buffer must add MASTER_VIDEO_FW (only in soft_mode)
		 */
	case GXAV_BUFFER_VIDEO_FRAME:
		master_write |= MASTER_VIDEO_FRAME | MASTER_PP | MASTER_VPU | MASTER_VIDEO_FW;
		master_read  |= MASTER_VIDEO_FRAME | MASTER_PP | MASTER_VPU | MASTER_VIDEO_FW;
#ifdef NO_OS
		/*
		 * 按照上面的逻辑, NOS会打开帧存保护,CPU无法访问帧存.会导致静帧拷贝失效, SLT测试失败.
		 * 所以, 此处打开NOS下CPU对帧存的读写权限.
		 */
		master_write |= MASTER_A7;
		master_read  |= MASTER_A7;
#endif
		break;
	case GXAV_BUFFER_VPU_CAP:
	case GXAV_BUFFER_VPU_SVPU:
		master_write |= MASTER_VPU | MASTER_PP;
		master_read  |= MASTER_VPU | MASTER_PP;
		break;

	default:
		return -1;
	}

	if (_firewall_note_buffer(type, buf_phy_addr, buf_size) != -1) {
		if (master_read || master_write) {
			gxav_secure_config_filter((int)buf_phy_addr, (int)buf_size, master_read, master_write);
			//gx_printf("%s[%x]: addr:0x%x, size=0x%x\n", __func__, type, buf_phy_addr, buf_size);
		}
	}

	return 0;
}

int gxav_firewall_buffer_protected(GxAvBufferType type)
{
	return ((fw.protect_flag & type) == type);
}

int gxav_firewall_access_align(void)
{
	return fw.align_flag;
}

int gxav_firewall_protect_align(void)
{
	return gxav_secure_query_protect_align();
}

EXPORT_SYMBOL(gxav_firewall_access_align);

