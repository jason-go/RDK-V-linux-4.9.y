#include "gxav.h"
#include "avcore.h"
#include "profile.h"
#include "firewall.h"
#include "clock_hal.h"
#include "hdmi_hal.h"
#include "hdmi_api_tx/phy/hdmi_phy_hal.h"
#include "kernelcalls.h"

struct gxav_gemini_device {
	int active;
};

static struct gxav_device_ops gemini_device_interface;
static struct gxav_gemini_device gemini;

extern struct gxav_module_ops gx3201_sdc_module;
extern struct gxav_module_ops gx3201_audiodec_module;
extern struct gxav_module_ops sirius_audioout_module;
extern struct gxav_module_ops sirius_vpu_module;
extern struct gxav_module_ops gx3201_videodec_module;
extern struct gxav_module_ops gx3201_jpeg_module;
extern struct gxav_module_ops taurus_demux_module;
extern struct gxav_module_ops taurus_dvr_module;
extern struct gxav_module_ops gx3211_videoout_module;
extern struct gxav_module_ops gx3211_hdmi_module;
extern struct gxav_module_ops gx3211_stc_module;
extern struct gxav_clock_module gemini_clock_module;
extern struct gxav_hdmi_phy_ops gx3211_hdmi_phy_ops;

static struct gxav_device_resource gemini_resource = {
	.module[GXAV_MOD_JPEG_DECODER] = {
		.regs = {0x04400000, },
		.irqs = {36, }
	},
	.module[GXAV_MOD_VPU] = { //vpu, svpu, ga
		.regs = {0x04800000, 0x04900000, 0x04700000},
		.irqs = {62, 46}
	},
	.module[GXAV_MOD_VIDEO_OUT] = { //vout0, vout1
		.regs = {0x04804000, 0x04904000},
	},
	.module[GXAV_MOD_HDMI] = { //hdmi
		.regs = {0x04F00000},
		.irqs = {54, }
	},
	.module[GXAV_MOD_AUDIO_DECODE] = {
		.regs = {0x04600000, },
		.irqs = {37, }
	},
	.module[GXAV_MOD_VIDEO_DECODE] = { // cm, vpu, pp
		.irqs = {61, 46, 43, },
	},
	.module[GXAV_MOD_STC] = {
		.regs = {0x04003560, 0x04003560, },
	},
	.module[GXAV_MOD_AUDIO_OUT] = {
		.regs = {
			0x04A00000, //I2S_BASE_ADDR_0
			0x04A00040, //I2S_BASE_ADDR_1
			0x04B00000, //SDC_BASE_ADDR_0
			0x04B80000, //SDC_BASE_ADDR_1
			0x04C00000, //SPD_BASE_ADDR_0
			0x04C00070, //SPD_BASE_ADDR_1
		},
		.irqs = {
			38, //INTC_I2S
			39, //INTC_SPDIF0
			40, //INTC_SPDIF1
		},
		.clks = {1188000000},
	},
};

static int gemini_init(struct gxav_device *dev)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	gxav_firewall_init();
	gxav_clock_init(&gemini_clock_module);

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
	gxav_module_register(dev, &sirius_audioout_module);
#endif

#ifdef CONFIG_AV_MODULE_DEMUX
	gxav_module_register(dev, &taurus_demux_module);
#ifdef CONFIG_AV_MODULE_DVR
	gxav_module_register(dev, &taurus_dvr_module);
#endif
#endif

#ifdef CONFIG_AV_MODULE_VPU
	gxav_module_register(dev, &sirius_vpu_module);
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

	gxav_device_setup(dev, &gemini_resource);
	return 0;
}

static int gemini_open(struct gxav_device *dev)
{
	struct gxav_gemini_device * priv = (struct gxav_gemini_device *)dev->priv;

	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	priv->active = 1;

	dev->priv = (void *)priv;

	return 0;
}

static int gemini_close(struct gxav_device *dev)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	dev->active = 0;

	return 0;
}

static int gemini_cleanup(struct gxav_device *dev)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

#ifdef CONFIG_AV_MODULE_SDC
	gxav_module_unregister(dev, &gx3201_sdc_module);
#endif

#ifdef CONFIG_AV_MODULE_AUDIO_DEC
	gxav_module_unregister(dev, &gx3201_audiodec_module);
#endif

#ifdef CONFIG_AV_MODULE_AUDIO_OUT
	gxav_module_unregister(dev, &sirius_audioout_module);
#endif

#ifdef CONFIG_AV_MODULE_DEMUX
	gxav_module_unregister(dev, &taurus_demux_module);
#ifdef CONFIG_AV_MODULE_DVR
	gxav_module_unregister(dev, &taurus_dvr_module);
#endif
#endif

#ifdef CONFIG_AV_MODULE_VPU
	gxav_module_unregister(dev, &sirius_vpu_module);
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

#ifdef CONFIG_AV_MODULE_HDMI
	gxav_hdmi_phy_unregister();
	gxav_module_unregister(dev, &gx3211_hdmi_module);
#endif
	gxav_clock_uninit();

	gemini_close(dev);
	return 0;
}

static int gemini_set_property(struct gxav_device *dev, int module_id, int property_id, void *property, int size)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	switch (property_id) {
		case GxAVGenericPropertyID_ClearEventMask:
			{
				//int *value = property;
				//int value_size = size;
				//gx_printf("%s(),gemini_set_property: GxAVGenericPropertyID_ClearEventMask\n", __func__);
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

static int gemini_get_property(struct gxav_device *dev, int module_id, int property_id, void *property, int size)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	switch (property_id) {
		case GxAVGenericPropertyID_ClearEventMask:
			{
				int value = 10;
				//int value_size = size;
				//gx_printf("%s(),gemini_get_property: GxAVGenericPropertyID_ClearEventMask\n", __func__);
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

static struct gxav_device_ops gemini_device_interface = {
	.init    = gemini_init,
	.cleanup = gemini_cleanup,
	.open    = gemini_open,
	.close   = gemini_close,
	.set_property = gemini_set_property,
	.get_property = gemini_get_property
};

static struct gxav_device chip_gemini = {
	.id = GXAV_ID_GEMINI,
	.profile = NULL,
	.interface = &gemini_device_interface,
	.priv      = &gemini
};

int gxav_chip_register_gemini(void)
{
	return chip_register(&chip_gemini);
}
