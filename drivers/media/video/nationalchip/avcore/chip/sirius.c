#include "gxav.h"
#include "avcore.h"
#include "profile.h"
#include "firewall.h"
#include "clock_hal.h"
#include "hdmi_hal.h"
#include "hdmi_api_tx/phy/hdmi_phy_hal.h"
#include "kernelcalls.h"

struct gxav_sirius_device {
	int active;
};

static struct gxav_device_ops sirius_device_interface;
static struct gxav_sirius_device sirius;

extern struct gxav_module_ops gx3201_sdc_module;
extern struct gxav_module_ops gx3201_audiodec_module;
extern struct gxav_module_ops sirius_audioout_module;
extern struct gxav_module_ops sirius_vpu_module;
extern struct gxav_module_ops gx3211_videodec_module;
extern struct gxav_module_ops gx3201_jpeg_module;
extern struct gxav_module_ops sirius_demux_module;
extern struct gxav_module_ops sirius_dvr_module;
extern struct gxav_module_ops gx3211_videoout_module;
extern struct gxav_module_ops gx3211_hdmi_module;
extern struct gxav_module_ops gx3211_stc_module;
extern struct gxav_clock_module sirius_clock_module;
extern struct gxav_hdmi_phy_ops gx3211_hdmi_phy_ops;
extern struct gxav_module_ops sirius_gp_module;

static struct gxav_device_resource sirius_resource = {
	.module[GXAV_MOD_JPEG_DECODER] = {
		.regs = {0x8A600000, },
		.irqs = {4, }
	},
	.module[GXAV_MOD_VPU] = { //vpu, svpu, ga
		.regs = {0x8A800000, 0x8a900000, 0x8a700000},
		.irqs = {12, 14}
	},
	.module[GXAV_MOD_VIDEO_OUT] = { //vout0, vout1
		.regs = {0x8A804000, 0x8A904000, 0x8AE00000},
	},
	.module[GXAV_MOD_HDMI] = {
		.regs = {0x8AE00000},
		.irqs = {21, }
	},
	.module[GXAV_MOD_AUDIO_DECODE] = {
		.regs = {0x8A500000, },
		.irqs = {5, }
	},
	.module[GXAV_MOD_VIDEO_DECODE] = {
		.irqs = {15, 14, 11, },
	},
	.module[GXAV_MOD_STC] = {
		.regs = {0x8A203560, 0x8A213560, },
	},
	.module[GXAV_MOD_AUDIO_OUT] = {
		.regs = {
			0x8AD00000, //I2S_BASE_ADDR_0
			0x8AD00040, //I2S_BASE_ADDR_1
			0x8AC00000, //SDC_BASE_ADDR_0
			0x8AC80000, //SDC_BASE_ADDR_1
			0x8AB00000, //SPD_BASE_ADDR_0
			0x8AB00070, //SPD_BASE_ADDR_1
		},
		.irqs = {
			6, //INTC_I2S
			7, //INTC_SPDIF0
			8, //INTC_SPDIF1
		},
		.clks = {1188000000},
	},
};

static int sirius_init(struct gxav_device *dev __maybe_unused)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	gxav_firewall_init();

#ifdef CONFIG_AV_MODULE_CLOCK
	gxav_clock_init(&sirius_clock_module);
#endif

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
	gxav_module_register(dev, &sirius_demux_module);
#ifdef CONFIG_AV_MODULE_DVR
	gxav_module_register(dev, &sirius_dvr_module);
#endif
#endif

#ifdef CONFIG_AV_MODULE_VPU
	gxav_module_register(dev, &sirius_vpu_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_DEC
	gxav_module_register(dev, &gx3211_videodec_module);
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

#ifdef CONFIG_AV_MODULE_GP
	gxav_module_register(dev, &sirius_gp_module);
#endif

	gxav_device_setup(dev, &sirius_resource);
	return 0;
}

static int sirius_open(struct gxav_device *dev)
{
	struct gxav_sirius_device * priv= (struct gxav_sirius_device *)dev->priv;

	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	priv->active = 1;

	dev->priv = (void *)priv;

	return 0;
}

static int sirius_close(struct gxav_device *dev)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	dev->active = 0;

	return 0;
}

static int sirius_cleanup(struct gxav_device *dev __maybe_unused)
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
	gxav_module_unregister(dev, &sirius_demux_module);
#ifdef CONFIG_AV_MODULE_DVR
	gxav_module_unregister(dev, &sirius_dvr_module);
#endif
#endif

#ifdef CONFIG_AV_MODULE_VPU
	gxav_module_unregister(dev, &sirius_vpu_module);
#endif

#ifdef CONFIG_AV_MODULE_VIDEO_DEC
	gxav_module_unregister(dev, &gx3211_videodec_module);
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

#ifdef CONFIG_AV_MODULE_GP
	gxav_module_unregister(dev, &sirius_gp_module);
#endif

#ifdef CONFIG_AV_MODULE_HDMI
	gxav_hdmi_phy_unregister();
	gxav_module_unregister(dev, &gx3211_hdmi_module);
#endif

#ifdef CONFIG_AV_MODULE_CLOCK
	gxav_clock_uninit();
#endif

	sirius_close(dev);
	return 0;
}

static int sirius_set_property(struct gxav_device *dev __maybe_unused, int module_id, int property_id, void *property __maybe_unused, int size)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	switch (property_id) {
		case GxAVGenericPropertyID_ClearEventMask:
			{
				//int *value = property;
				//int value_size = size;
				//gx_printf("%s(),sirius_set_property: GxAVGenericPropertyID_ClearEventMask\n", __func__);
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

static int sirius_get_property(struct gxav_device *dev __maybe_unused, int module_id, int property_id, void *property, int size)
{
	GXAV_DBGEV("%s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	switch (property_id) {
		case GxAVGenericPropertyID_ClearEventMask:
			{
				int value = 10;
				//int value_size = size;
				//gx_printf("%s(),sirius_get_property: GxAVGenericPropertyID_ClearEventMask\n", __func__);
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

static struct gxav_device_ops sirius_device_interface = {
	.init = sirius_init,
	.cleanup = sirius_cleanup,
	.open = sirius_open,
	.close = sirius_close,
	.set_property = sirius_set_property,
	.get_property = sirius_get_property
};

static struct gxav_device chip_sirius = {
	.id = GXAV_ID_SIRIUS,
	.profile = NULL,
	.interface = &sirius_device_interface,
	.priv = &sirius
};

int gxav_chip_register_sirius(void)
{
	return chip_register(&chip_sirius);
}

