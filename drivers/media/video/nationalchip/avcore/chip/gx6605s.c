#include "gxav.h"
#include "avcore.h"
#include "profile.h"
#include "firewall.h"
#include "clock_hal.h"
#include "hdmi_hal.h"
#include "hdmi_api_tx/phy/hdmi_phy_hal.h"
#include "kernelcalls.h"

struct gxav_gx6605s_device {
	int active;
};

static struct gxav_device_ops gx6605s_device_interface;
static struct gxav_gx6605s_device gx6605s;

extern struct gxav_module_ops gx3201_sdc_module;
extern struct gxav_module_ops gx3201_audiodec_module;
extern struct gxav_module_ops gx3201_audioout_module;
extern struct gxav_module_ops gx3211_vpu_module;
extern struct gxav_module_ops gx3201_videodec_module;
extern struct gxav_module_ops gx3201_jpeg_module;
extern struct gxav_module_ops gx6605s_demux_module;
extern struct gxav_module_ops pegasus_demux_module;
extern struct gxav_module_ops gx6605s_dvr_module;
extern struct gxav_module_ops pegasus_dvr_module;
extern struct gxav_module_ops gx3211_videoout_module;
extern struct gxav_module_ops gx3211_hdmi_module;
extern struct gxav_module_ops gx3211_stc_module;
extern struct gxav_clock_module gx3211_clock_module;
extern struct gxav_hdmi_phy_ops gx3211_hdmi_phy_ops;
extern struct gxav_module_ops gx3211_icam_module ;
extern struct gxav_module_ops gx3211_iemm_module;
//extern struct gxav_module_ops gx3211_descrambler_module ;
//extern struct gxav_module_ops gx3211_mtc_module;

static int gx6605s_init(struct gxav_device *dev)
{

	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	gxav_clock_init(&gx3211_clock_module);

#ifdef CONFIG_AV_MODULE_HDMI
	gxav_module_register(dev, &gx3211_hdmi_module);
	gxav_hdmi_phy_register(&gx3211_hdmi_phy_ops);
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
	if((gxcore_chip_sub_probe() == 1))
		gxav_module_register(dev, &pegasus_demux_module);
	else
		gxav_module_register(dev, &gx6605s_demux_module);
#ifdef CONFIG_AV_MODULE_DVR
	if((gxcore_chip_sub_probe() == 1))
		gxav_module_register(dev, &pegasus_dvr_module);
	else
		gxav_module_register(dev, &gx6605s_dvr_module);
#endif
#endif

#ifdef CONFIG_AV_MODULE_VPU
	gxav_module_register(dev, &gx3211_vpu_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_DEC
	gxav_module_register(dev, &gx3201_videodec_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_OUT
	gxav_module_register(dev, &gx3211_videoout_module);
#endif

#ifdef CONFIG_AV_MODULE_STC
	gxav_module_register(dev, &gx3211_stc_module);
#endif

#ifdef CONFIG_AV_MODULE_JPEG
	gxav_module_register(dev, &gx3201_jpeg_module);
#endif

#ifdef CONFIG_AV_MODULE_ICAM
	if (gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)) {
		gxav_module_register(dev, &gx3211_icam_module);
		gxav_module_register(dev, &gx3211_iemm_module);
	}
#endif

#ifdef CONFIG_AV_MODULE_MTC
	//gxav_module_register(dev, &gx3211_descrambler_module);
	//gxav_module_register(dev, &gx3211_mtc_module);
#endif

	return 0;
}

static int gx6605s_open(struct gxav_device *dev)
{
	struct gxav_gx6605s_device *gx6605s = (struct gxav_gx6605s_device *)dev->priv;

	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	gx6605s->active = 1;

	dev->priv = (void *)gx6605s;

	return 0;
}

static int gx6605s_close(struct gxav_device *dev)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	dev->active = 0;

	return 0;
}

static int gx6605s_cleanup(struct gxav_device *dev)
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
#ifdef CONFIG_AV_MODULE_DVR
	if((gxcore_chip_sub_probe() == 1))
		gxav_module_unregister(dev, &pegasus_dvr_module);
	else
		gxav_module_unregister(dev, &gx6605s_dvr_module);
#endif
	if((gxcore_chip_sub_probe() == 1))
		gxav_module_unregister(dev, &pegasus_demux_module);
	else
		gxav_module_unregister(dev, &gx6605s_demux_module);
#endif

#ifdef CONFIG_AV_MODULE_VPU
	gxav_module_unregister(dev, &gx3211_vpu_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_DEC
	gxav_module_unregister(dev, &gx3201_videodec_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_OUT
	gxav_module_unregister(dev, &gx3211_videoout_module);
#endif

#ifdef CONFIG_AV_MODULE_JPEG
	gxav_module_unregister(dev, &gx3201_jpeg_module);
#endif

#ifdef CONFIG_AV_MODULE_STC
	gxav_module_unregister(dev, &gx3211_stc_module);
#endif

#ifdef CONFIG_AV_MODULE_ICAM
	gxav_module_unregister(dev, &gx3211_icam_module);
	gxav_module_unregister(dev, &gx3211_iemm_module);
#endif

#ifdef CONFIG_AV_MODULE_MTC
	//gxav_module_unregister(dev, &gx3211_descrambler_module);
	//gxav_module_unregister(dev, &gx3211_mtc_module);
#endif

#ifdef CONFIG_AV_MODULE_HDMI
	gxav_hdmi_phy_unregister();
	gxav_module_unregister(dev, &gx3211_hdmi_module);
#endif
	gxav_clock_uninit();

	gx6605s_close(dev);
	return 0;
}

static int gx6605s_set_property(struct gxav_device *dev, int module_id, int property_id, void *property, int size)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	switch (property_id) {
		case GxAVGenericPropertyID_ClearEventMask:
			{
				//int *value = property;
				//int value_size = size;
				//gx_printf("%s(),gx6605s_set_property: GxAVGenericPropertyID_ClearEventMask\n", __func__);
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

static int gx6605s_get_property(struct gxav_device *dev, int module_id, int property_id, void *property, int size)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	switch (property_id) {
		case GxAVGenericPropertyID_ClearEventMask:
			{
				int value = 10;
				//int value_size = size;
				//gx_printf("%s(),gx6605s_get_property: GxAVGenericPropertyID_ClearEventMask\n", __func__);
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

static struct gxav_device_ops gx6605s_device_interface = {
	.init = gx6605s_init,
	.cleanup = gx6605s_cleanup,
	.open = gx6605s_open,
	.close = gx6605s_close,
	.set_property = gx6605s_set_property,
	.get_property = gx6605s_get_property
};

static struct gxav_device chip_gx6605s = {
	.id = GXAV_ID_GX6605S,
	.profile = NULL,
	.interface = &gx6605s_device_interface,
	.priv = &gx6605s
};

int gxav_chip_register_gx6605s(void)
{
	return chip_register(&chip_gx6605s);
}

