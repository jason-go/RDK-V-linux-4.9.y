#include "kernelcalls.h"
#include "avcore.h"
#include "gxav_bitops.h"
#include "gxdemux.h"
#include "gx3211_regs.h"
#include "sirius_regs.h"
#include "profile.h"

#ifdef  FPGA_TEST_STREAM_PRODUCER_EN
#define GXAV_DTO_BASE 0x0030a054

static volatile unsigned int *s_dto_base_addr = NULL;
extern unsigned char g_dvr_fpga_stream[];
extern unsigned char parallel_sync_txt[];
extern unsigned int  g_dvr_fpga_stream_len;
extern unsigned int  parallel_sync_txt_len;

extern unsigned char parallel_data_204_txt[];
extern unsigned char parallel_sync_204_txt[];
extern unsigned int  parallel_data_204_txt_len;
extern unsigned int  parallel_sync_204_txt_len;

extern unsigned char serial_data_1bit_txt[];
extern unsigned char serial_sync_1bit_txt[];
extern unsigned int  serial_data_1bit_txt_len;
extern unsigned int  serial_sync_1bit_txt_len;

extern unsigned char serial_data_2bit_txt[];
extern unsigned char serial_sync_2bit_txt[];
extern unsigned int  serial_data_2bit_txt_len;
extern unsigned int  serial_sync_2bit_txt_len;

extern unsigned char serial_data_4bit_txt[];
extern unsigned char serial_sync_4bit_txt[];
extern unsigned int  serial_data_4bit_txt_len;
extern unsigned int  serial_sync_4bit_txt_len;

void dmx_fpga_set_stream_addr(int stream_mode)
{
	unsigned int data_beg_addr, data_end_addr, sync_beg_addr, sync_end_addr;
	switch (stream_mode) {
	case DEMUX_PARALLEL:
#if 1
		data_beg_addr  = (unsigned int)gx_virt_to_phys(g_dvr_fpga_stream);
		data_end_addr   = data_beg_addr + g_dvr_fpga_stream_len - 1;
		sync_beg_addr = (unsigned int)gx_virt_to_phys(parallel_sync_txt);
		sync_end_addr  = sync_beg_addr + parallel_sync_txt_len - 1;
#else
// use 204 bytes ts stream
		data_beg_addr  = (unsigned int)gx_virt_to_phys(parallel_data_204_txt);
		data_end_addr   = data_beg_addr + parallel_data_204_txt_len - 1;
		sync_beg_addr = (unsigned int)gx_virt_to_phys(parallel_sync_204_txt);
		sync_end_addr  = sync_beg_addr + parallel_sync_204_txt_len - 1;
#endif
		break;

	case DEMUX_SERIAL:
		data_beg_addr  = (unsigned int)gx_virt_to_phys(serial_data_1bit_txt);
		data_end_addr   = data_beg_addr + serial_data_1bit_txt_len - 1;
		sync_beg_addr = (unsigned int)gx_virt_to_phys(serial_sync_1bit_txt);
		sync_end_addr  = sync_beg_addr + serial_sync_1bit_txt_len - 1;
		break;

	case DEMUX_SERIAL_2BIT:
		data_beg_addr  = (unsigned int)gx_virt_to_phys(serial_data_2bit_txt);
		data_end_addr   = data_beg_addr + serial_data_2bit_txt_len - 1;
		sync_beg_addr = (unsigned int)gx_virt_to_phys(serial_sync_2bit_txt);
		sync_end_addr  = sync_beg_addr + serial_sync_2bit_txt_len - 1;
		break;

	case DEMUX_SERIAL_4BIT:
		data_beg_addr  = (unsigned int)gx_virt_to_phys(serial_data_4bit_txt);
		data_end_addr   = data_beg_addr + serial_data_4bit_txt_len - 1;
		sync_beg_addr = (unsigned int)gx_virt_to_phys(serial_sync_4bit_txt);
		sync_end_addr  = sync_beg_addr + serial_sync_4bit_txt_len - 1;
		break;

	default:
		return;
	}
	gx_dcache_clean_range(0,0);
	REG_SET_VAL((unsigned char *)(s_dto_base_addr) + 0x04, data_beg_addr);
	REG_SET_VAL((unsigned char *)(s_dto_base_addr) + 0x08, data_end_addr);
	REG_SET_VAL((unsigned char *)(s_dto_base_addr) + 0x0c, sync_beg_addr);
	REG_SET_VAL((unsigned char *)(s_dto_base_addr) + 0x10, sync_end_addr);
}

/*********************************
 *
 * gs_clk : a
 * fe_clk : 4 ( < gs_clk / 2)
 *
 *********************************/
void dmx_fpga_hot_rst(int continue_play, int record)
{
	unsigned int value;
	unsigned int fe_clk = 4;
	unsigned int gs_clk = record ? 0x4a : 0xa;

	REG_CLR_BIT((unsigned char *)(s_dto_base_addr), 0);
	REG_CLR_BIT((unsigned char *)(s_dto_base_addr), 4);
	if (continue_play)
		value = fe_clk << 16 | gs_clk << 8 | 0x17;
	else
		value = fe_clk << 16 | gs_clk << 8 | 0x15;
	REG_SET_VAL((unsigned char *)(s_dto_base_addr), value);
}

struct demux_fpga_ops fpga_ops = {
	.set_stream_addr = dmx_fpga_set_stream_addr,
	.hot_rst         = dmx_fpga_hot_rst,
};

int dmx_fpga_init(void)
{
	int i;
	struct dmxdev *dev;

	if (!s_dto_base_addr) {
		gx_request_mem_region(GXAV_DTO_BASE, 0x100);
		s_dto_base_addr = gx_ioremap(GXAV_DTO_BASE, 0x100);
	}

	for (i=0; i<MAX_DEMUX_UNIT; i++) {
		dev = &gxdmxdev[i];
		dev->fpga_ops = &fpga_ops;
	}
	return 0;
}

#endif
