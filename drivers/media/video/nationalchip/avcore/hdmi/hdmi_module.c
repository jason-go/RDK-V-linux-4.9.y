#include "gxav.h"
#include "hdmi_hal.h"
#include "vout_hal.h"
#include "kernelcalls.h"

#ifdef CONFIG_AV_MODULE_HDMI

static struct gxav_module_inode *_hdmi_inode;

int gx_hdmi_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	_hdmi_inode = inode;
	return gxav_hdmi_init(inode->interface);
}

int gx_hdmi_uninit(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_hdmi_uninit();
}

int gx_hdmi_open(struct gxav_module *module)
{
	return 0;
}

int gx_hdmi_close(int sub)
{
	return 0;
}

int gx_hdmi_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	switch (property_id) {
	default:
		VIDEOOUT_PRINTF("there is something wrong with the parameter property_id,"
				"please set the right parameter %d\n", property_id);
		return -2;
	}

	return 0;
}

int gx_hdmi_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	switch (property_id) {
	default:
		VIDEOOUT_PRINTF("there is something wrong with the parameter property_id,"
				"please set the right parameter %d\n", property_id);
		return -2;
	}

	return 0;
}

struct gxav_module_inode *gx_hdmi_interrupt(struct gxav_module_inode *inode, int irq)
{
	int event = gxav_hdmi_interrupt();
	if (event) {
		VIDEOOUT_DBG("hdmi event %d\n", event);
		gxav_module_inode_set_event(inode, event);
	}

	return inode;
}

int gxav_hdmi_wake_event(unsigned int event)
{
	return gxav_module_inode_set_event(_hdmi_inode, event);
}

#endif

