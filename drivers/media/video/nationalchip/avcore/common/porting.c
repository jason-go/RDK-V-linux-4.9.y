#include "porting.h"

#ifndef CONFIG_AV_MODULE_VIDEO_DEC
int gx_video_disp_patch(int id)
{
    return 0;
}
int gx_video_close(int id)
{
    return 0;
}
int gx_video_zoom_require(int id)
{
	return 0;
}
int gx_video_pp_zoom(int id)
{
    return 0;
}
int gx_video_ppopen_require( int id )
{
    return 0;
}
int gx_video_frame_rate_transform_require( int id )
{
	return 0;
}
int h_vd_get_disp_zoom_mode(void)
{
	return 0;
}
void gx_video_init_switchxy(int id, unsigned int switchx, unsigned int switchy)
{
}
void video_sync_strict_enable(struct video_sync *sync, unsigned int enable)
{
}
int gx_video_ppclose_require( int id )
{
	return 0;
}
unsigned int video_sync_get_frame_dis(struct video_sync *sync)
{
	return 0;
}
int gx_video_cap_frame(int id, struct cap_frame *frame)
{
	return 0;
}
int videodisp_get_showing_frame(struct gxav_video_module *module, struct cap_frame *frame)
{
	return 0;
}
#endif


