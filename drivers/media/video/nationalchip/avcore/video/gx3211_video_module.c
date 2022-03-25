#include "gxav.h"
#include "avcore.h"
#include "video_hal.h"

extern struct pp_ops gx3211pp_ops;
struct gxav_module_ops gx3211_videodec_module;

static int video_hal_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	gx3211_videodec_module.irqs[0] = VIDEO_BODA_INIT_SRC = res->irqs[0];
	gx3211_videodec_module.irqs[1] = VIDEO_VPU_INC_SRC   = res->irqs[1];
	gx3211_videodec_module.irqs[2] = VIDEO_PP_INC_SRC    = res->irqs[2];
	gx3211_videodec_module.interrupts[res->irqs[0]] = video_hal_interrupt;
	gx3211_videodec_module.interrupts[res->irqs[1]] = video_hal_interrupt;
	gx3211_videodec_module.interrupts[res->irqs[2]] = video_hal_interrupt;

	return 0;
}

struct gxav_module_ops gx3211_videodec_module = {
	.priv                           = &gx3211pp_ops,
	.module_type                    = GXAV_MOD_VIDEO_DECODE,
	.count                          = 1,
	.irqs                           = {61, 46, 43, -1},
	.irq_names                      = {"vdec", "vpu", "pp"},
	.irq_flags                      = {-1,GXAV_IRQ_FAST,-1}, //GXAV_IRQ_NORMAL
	.init                           = video_hal_init,
	.cleanup                        = video_hal_cleanup,
	.open                           = video_hal_open,
	.close                          = video_hal_close,
	.setup                          = video_hal_setup,
	.set_property                   = video_hal_set_property,
	.get_property                   = video_hal_get_property,
	.interrupts[61] = video_hal_interrupt,
	.interrupts[46] = video_hal_interrupt,
	.interrupts[43] = video_hal_interrupt,
	.event_mask = (EVENT_VIDEO_0_FRAMEINFO | EVENT_VIDEO_1_FRAMEINFO \
			| EVENT_VIDEO_0_DECOVER | EVENT_VIDEO_1_DECOVER \
			| EVENT_VIDEO_ONE_FRAME_OVER | EVENT_VPU_INTERRUPT | EVENT_VIDEO_SEQ_CHANGED),
};

