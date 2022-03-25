#include "stc_hal.h"

int gx_stc_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_stc_init(inode->interface);
}

int gx_stc_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_stc_cleanup();
}

int gx_stc_open(struct gxav_module *module)
{
	return gxav_stc_open(module->sub);
}

int gx_stc_close(struct gxav_module *module)
{
	return gxav_stc_close(module->sub);
}

int gx_stc_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	GXAV_DBG("gx3201_stc_set_property :");

	switch (property_id) {
	case GxSTCPropertyID_Config:
		{
			GxSTCProperty_Config *config = (GxSTCProperty_Config *) property;

			GXAV_DBG("[property_id] : 0x%x (STC Config)\n", property_id);

			if (config == NULL || size != sizeof(GxSTCProperty_Config)) {
				gx_printf("%s(),stc config error!\n", __func__);
				return -1;
			}

			return gxav_stc_set_source(module->sub, config);
		}
	case GxSTCPropertyID_TimeResolution:
		{
			GxSTCProperty_TimeResolution *resolution = (GxSTCProperty_TimeResolution *) property;

			GXAV_DBG("[property_id] : 0x%x (STC TimeResolution)\n", property_id);

			if (resolution == NULL || size != sizeof(GxSTCProperty_TimeResolution)) {
				gx_printf("%s(),stc time resolution error!\n", __func__);
				return -1;
			}

			return gxav_stc_set_resolution(module->sub,resolution);
		}
	case GxSTCPropertyID_Time:
		{
			GxSTCProperty_Time *time = (GxSTCProperty_Time *) property;

			GXAV_DBG("[property_id] : 0x%x (STC Time)\n", property_id);

			if (time == NULL || size != sizeof(GxSTCProperty_Time)) {
				gx_printf("%s(),stc time error!\n", __func__);
				return -1;
			}

			return gxav_stc_set_time(module->sub,time);
		}
	case GxSTCPropertyID_Play:
		{
			GXAV_DBG("[property_id] : 0x%x (STC Play)\n", property_id);

			return gxav_stc_play(module->sub);
		}
	case GxSTCPropertyID_Stop:
		{
			GXAV_DBG("[property_id] : 0x%x (STC Stop)\n", property_id);

			return gxav_stc_stop(module->sub);
		}
	case GxSTCPropertyID_Pause:
		{
			GXAV_DBG("[property_id] : 0x%x (STC Pause)\n", property_id);

			return gxav_stc_pause(module->sub);
		}
	case GxSTCPropertyID_Resume:
		{
			GXAV_DBG("[property_id] : 0x%x (STC Resume)\n", property_id);

			return gxav_stc_resume(module->sub);
		}
	default:
		gx_printf("the parameter which stc's property_id (%d) is wrong\n", property_id);
		return -2;

	}

	return 0;
}

int gx_stc_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	GXAV_DBG("gx3110_stc_get_property :");

	switch (property_id) {
	case GxSTCPropertyID_TimeResolution:
		{
			GxSTCProperty_TimeResolution *resolution = (GxSTCProperty_TimeResolution *) property;

			GXAV_DBG("[property_id] : 0x%x (STC TimeResolution)\n", property_id);

			if (resolution == NULL || size != sizeof(GxSTCProperty_TimeResolution)) {
				gx_printf("%s(),stc time resolution error!\n", __func__);
				return -1;
			}

			return gxav_stc_get_resolution(module->sub, resolution);
		}
	case GxSTCPropertyID_Time:
		{
			GxSTCProperty_Time *time = (GxSTCProperty_Time *) property;

			GXAV_DBG("[property_id] : 0x%x (STC Time)\n", property_id);

			if (time == NULL || size != sizeof(GxSTCProperty_Time)) {
				gx_printf("%s(),stc time error!\n", __func__);
				return -1;
			}

			return gxav_stc_get_time(module->sub, time);
		}
	default:
		gx_printf("the parameter which stc's property_id (%d) is wrong\n", property_id);
		return -2;
	}

	return 0;
}


