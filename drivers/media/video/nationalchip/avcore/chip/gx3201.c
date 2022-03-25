#include "gxav.h"
#include "avcore.h"
#include "clock_hal.h"
#include "hdmi_hal.h"
#include "kernelcalls.h"

struct gxav_gx3201_device {
	int active;
};

static struct gxav_device_ops gx3201_device_interface;
static struct gxav_gx3201_device gx3201;

extern struct gxav_module_ops gx3201_sdc_module;
extern struct gxav_module_ops gx3201_demux_module;
extern struct gxav_module_ops gx3201_vpu_module;
extern struct gxav_module_ops gx3201_videodec_module;
extern struct gxav_module_ops gx3201_jpeg_module;
extern struct gxav_module_ops gx3201_videoout_module;
extern struct gxav_module_ops gx3201_hdmi_module;
extern struct gxav_module_ops gx3201_stc_module;
extern struct gxav_module_ops gx3201_audiodec_module;
extern struct gxav_module_ops gx3201_audioout_module;
extern struct gxav_clock_module gx3201_clock_module;

static int gx3201_init(struct gxav_device *dev  __maybe_unused)
{

	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	gxav_clock_init(&gx3201_clock_module);

#ifdef CONFIG_AV_MODULE_HDMI
	gxav_module_register(dev, &gx3201_hdmi_module);
#endif

#ifdef CONFIG_AV_MODULE_SDC
	gxav_module_register(dev, &gx3201_sdc_module);
#endif

#ifdef CONFIG_AV_MODULE_AUDIO_DEC
	gxav_module_register(dev, &gx3201_audiodec_module);
#endif

#ifdef CONFIG_AV_MODULE_AUDIO_OUT
	gxav_module_register(dev, &gx3201_audioout_module);
#endif

#ifdef CONFIG_AV_MODULE_DEMUX
	gxav_module_register(dev, &gx3201_demux_module);
#endif

#ifdef CONFIG_AV_MODULE_VPU
	gxav_module_register(dev, &gx3201_vpu_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_DEC
	gxav_module_register(dev, &gx3201_videodec_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_OUT
	gxav_module_register(dev, &gx3201_videoout_module);
#endif

#ifdef CONFIG_AV_MODULE_STC
	gxav_module_register(dev, &gx3201_stc_module);
#endif

#ifdef CONFIG_AV_MODULE_JPEG
	gxav_module_register(dev, &gx3201_jpeg_module);
#endif

	return 0;
}

static int gx3201_open(struct gxav_device *dev)
{
	struct gxav_gx3201_device *priv = (struct gxav_gx3201_device *)dev->priv;

	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	priv->active = 1;

	dev->priv = (void *)priv;

	return 0;
}

static int gx3201_close(struct gxav_device *dev)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	dev->active = 0;

	return 0;
}

static int gx3201_cleanup(struct gxav_device *dev)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

#ifdef CONFIG_AV_MODULE_SDC
	gxav_module_unregister(dev, &gx3201_sdc_module);
#endif

#ifdef CONFIG_AV_MODULE_AUDIO_DEC
	gxav_module_unregister(dev, &gx3201_audiodec_module);
#endif

#ifdef CONFIG_AV_MODULE_AUDIO_OUT
	gxav_module_unregister(dev, &gx3201_audioout_module);
#endif

#ifdef CONFIG_AV_MODULE_DEMUX
	gxav_module_unregister(dev, &gx3201_demux_module);
#endif

#ifdef CONFIG_AV_MODULE_VPU
	gxav_module_unregister(dev, &gx3201_vpu_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_DEC
	gxav_module_unregister(dev, &gx3201_videodec_module);
#endif
#ifdef CONFIG_AV_MODULE_VIDEO_OUT
	gxav_module_unregister(dev, &gx3201_videoout_module);
#endif

#ifdef CONFIG_AV_MODULE_STC
	gxav_module_unregister(dev, &gx3201_stc_module);
#endif

#ifdef CONFIG_AV_MODULE_JPEG
	gxav_module_unregister(dev, &gx3201_jpeg_module);
#endif

#ifdef CONFIG_AV_MODULE_HDMI
	gxav_module_unregister(dev, &gx3201_hdmi_module);
#endif
	gxav_clock_uninit();

	gx3201_close(dev);
	return 0;
}

static int gx3201_set_property(struct gxav_device *dev __maybe_unused, int module_id, int property_id, void *property __maybe_unused, int size)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	switch (property_id) {
		case GxAVGenericPropertyID_ClearEventMask:
			{
				//int *value = property;
				//int value_size = size;
				//gx_printf("%s(),gx3201_set_property: GxAVGenericPropertyID_ClearEventMask\n", __func__);
				//gx_printf("%s(),value = %d\n", __func__, *value);
				//gx_printf("%s(),value_size = %d\n", __func__, value_size);
			}
			break;

		default:
			gx_printf("%s(),the parameter property_id is wrong! ID=%d,MOD=%d,size=%d\n", __func__,property_id,dev->modules_handle[module_id]->module->module_type,size);
			return -2;
	}

	return 0;
}

static int gx3201_get_property(struct gxav_device *dev __maybe_unused, int module_id, int property_id, void *property, int size)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	switch (property_id) {
		case GxAVGenericPropertyID_ClearEventMask:
			{
				int value = 10;
				//int value_size = size;
				//gx_printf("%s(),gx3201_get_property: GxAVGenericPropertyID_ClearEventMask\n", __func__);
				gx_memcpy(property, &value, sizeof(int));
				//gx_printf("%s(),value_size = %d\n", __func__, value_size);
			}
			break;

		default:
			gx_printf("%s(),the parameter property_id is wrong! ID=%d,MOD=%d,size=%d\n", __func__,property_id,dev->modules_handle[module_id]->module->module_type,size);
			return -2;
	}

	return 0;
}

static struct gxav_device_ops gx3201_device_interface = {
	.init = gx3201_init,
	.cleanup = gx3201_cleanup,
	.open = gx3201_open,
	.close = gx3201_close,
	.set_property = gx3201_set_property,
	.get_property = gx3201_get_property
};

static struct gxav_device chip_gx3201 = {
	.id = GXAV_ID_GX3201,
	.profile = NULL,
	.interface = &gx3201_device_interface,
	.priv = &gx3201
};

int gxav_chip_register_gx3201(void)
{
	return chip_register(&chip_gx3201);
}

