#include "cygnus_vpu_internel.h"
#include "vout_hal.h"

extern int vpp2_enable(int en);
extern int vpp2_bind_to_ce(int en);
extern int vpp2_zoom(GxAvRect *clip, GxAvRect *view);

static GxVideoOutProperty_Mode vout1_mode;

int cygnus_svpu_init(void)
{
	ce_init();
	return 0;
}

int cygnus_svpu_config(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo *buf_info)
{
	GxAvRect view = {0};
	CygnusVpuSurface surface = {0};

	ce_set_format(GX_COLOR_FMT_YCBCR422);
	ce_set_mode(CE_MODE_AUTO);
	ce_set_layer((1<<GX_LAYER_VPP)|(1<<GX_LAYER_SPP)|(1<<GX_LAYER_OSD));
	surface.width  = 720;
	surface.height = 576;
	surface.buffer = (void*)buf_info->buf_addrs[0];
	ce_set_buf(&surface);
	ce_bind_to_vpp2(1);

	if (IS_P_MODE(mode_vout1)) {
		view.x = view.y = 0;
		view.width  = 720;
		view.height = 576;
	} else {
		view.x = view.y = 0;
		view.width  = 720;
		view.height = 480;
	}
	vpp2_zoom(NULL, &view);

	return 0;
}

int cygnus_svpu_run(void)
{
	ce_enable(1);
	vpp2_enable(1);
	vpu_dac_set_source(VPU_DAC_3, SD_MIXER);

	return 0;
}

int cygnus_svpu_stop(void)
{
	vpp2_enable(0);
	ce_enable(0);
	gx_mdelay(30);
	return 0;
}

