#include "video.h"
#include "pp_core.h"
#include "frame_convert.h"
#include "video_sync.h"
#include "kernelcalls.h"
#include "gxav_common.h"

static struct pp_ops *pp_ops = NULL;

void videopp_init(struct pp_ops *ops)
{
	pp_ops = ops;
}

static struct pp_ops* videopp_get_ops(void)
{
	return pp_ops;
}

int videopp_ioremap()
{
	struct pp_ops *thiz = videopp_get_ops();

	if(thiz && thiz->ioremap)
		return thiz->ioremap();
	else
		return -1;
}

int videopp_iounmap()
{
	struct pp_ops *thiz = videopp_get_ops();

	if(thiz && thiz->iounmap)
		return thiz->iounmap();
	else
		return -1;
}

int videopp_open(struct gxav_video_module *module, struct video_sync *sync, struct frame_rate_convert *fr_con)
{
	struct pp_ops *thiz = videopp_get_ops();

	if(thiz && thiz->open) {
		module->ops  = thiz;
		module->priv = thiz->open(module->id, sync, fr_con);
		return 0;
	}
	else {
		module->ops = module->priv = NULL;
		return -1;
	}
}

int videopp_close(struct gxav_video_module *module)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->close)
			return thiz->close(module);
	}

	return -1;
}

int videopp_run(struct gxav_video_module *module)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->run)
			return thiz->run(module);
	}

	return -1;
}

int videopp_stop(struct gxav_video_module *module)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->stop)
			return thiz->stop(module);
	}

	return -1;
}

int videopp_pause(struct gxav_video_module *module)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->pause)
			return thiz->pause(module);
	}

	return -1;
}

int videopp_resume(struct gxav_video_module *module)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->resume)
			return thiz->resume(module);
	}

	return -1;
}

int videopp_config(struct gxav_video_module *module, unsigned int cmd, void *arg, unsigned int arg_size)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->config)
			return thiz->config(module, cmd, arg, arg_size);
	}

	return -1;
}

int videopp_get_state(struct gxav_video_module *module, int *state)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->get_state)
			return thiz->get_state(module, state);
	}

	return -1;
}

int videopp_isr(struct gxav_video_module *module)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->isr)
			return thiz->isr(module);
	}

	return -1;
}

void videopp_callback(struct gxav_video_module *module)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->callback)
			thiz->callback(module);
	}
}

void videopp_get_frame(struct gxav_video_module *module, VideoFrameInfo *frame)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->get_frame)
			thiz->get_frame(module, frame);
	}
}

void videopp_clr_frame(struct gxav_video_module *module, VideoFrameInfo *frame)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->clr_frame)
			thiz->clr_frame(module, frame);
	}
}

int videopp_get_sync_stat(struct gxav_video_module *module, unsigned long long *lose_sync_cnt , unsigned int *synced_flag)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->get_sync_stat)
			return thiz->get_sync_stat(module, lose_sync_cnt, synced_flag);
	}

	return -1;
}

void videopp_zoom_param_update_require(struct gxav_video_module *module)
{
	struct pp_ops *thiz = NULL;

	if(module && module->ops) {
		thiz = module->ops;
		if(thiz->zoom_param_update_require)
			thiz->zoom_param_update_require(module);
	}
}
