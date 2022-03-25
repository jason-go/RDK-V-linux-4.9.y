#include <linux/fb.h>
#include <linux/version.h>
#include "gxfb.h"
#include "gxfb_reg.h"
#include "gxav_ioctl.h"
#include "vpu_hal.h"
#include "vout_hal.h"
#include "gxavdev.h"

#define to_dev(obj)	container_of(obj, struct device, kobj)

#define GXFB_ATTR(_name) \
	__ATTR(_name, S_IRUGO|S_IWUSR, gxfb_##_name##_show, gxfb_##_name##_store)

#define GXFB_DEV_BIN_ATTR(_name, _size) \
	static struct bin_attribute device_bin_attr_##_name = {	\
		.attr = {						\
			.name = #_name"_iom",				\
			.mode = S_IRUSR | S_IWUSR,			\
		},							\
		.size = _size,						\
		.read = gxfb_iom_read_##_name,				\
		.write = gxfb_iom_write_##_name,			\
	}

#define GXFB_DEV_BIN_ATTR_NAME(name) (device_bin_attr_##name)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
static ssize_t gxfb_iom_read_vpu(struct file *f, struct kobject *kobj, struct bin_attribute *bin_attr,
		char *buf, loff_t off, size_t count)
#else
static ssize_t gxfb_iom_read_vpu(struct kobject *kobj, struct bin_attribute *bin_attr,
		char *buf, loff_t off, size_t count)
#endif
{
	struct fb_info *fb_info = NULL;
	struct gxfb_pri *pri = NULL;
	struct device *device = NULL;

	GXFB_ENTER();

	device = to_dev(kobj);
	fb_info = dev_get_drvdata(device);

	if (!fb_info || !fb_info->par)
		return 0;

	pri = (struct gxfb_pri *)fb_info->par;

	GXFB_LEAVE();
	return memory_read_from_buffer(buf, count, &off,
			pri->vpu_iom, bin_attr->size);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
static ssize_t gxfb_iom_write_vpu(struct file *f, struct kobject *kobj, struct bin_attribute *bin_attr,
		char *buf, loff_t off, size_t count)
#else
static ssize_t gxfb_iom_write_vpu(struct kobject *kobj, struct bin_attribute *bin_attr,
		char *buf, loff_t off, size_t count)
#endif
{
	struct fb_info *fb_info = NULL;
	struct gxfb_pri *pri = NULL;
	struct device *device = NULL;
	ssize_t retval = 0;

	GXFB_ENTER();

	device = to_dev(kobj);
	fb_info = dev_get_drvdata(device);

	if (!fb_info || !fb_info->par)
		return 0;

	pri = (struct gxfb_pri *)fb_info->par;

	memcpy((u8 *)pri->vpu_iom + off, buf, count);
	retval = count;

	GXFB_LEAVE();
	return retval;
}

GXFB_DEV_BIN_ATTR(vpu, GX_FB_VPU_ADDR_SIZE);

int gxfb_init_device(struct fb_info *fb_info)
{
	int ret = 0;

	GXFB_ENTER();

	if (!fb_info || !fb_info->dev)
		return -EINVAL;

	GXFB_CHECK_RET(ret = device_create_bin_file(fb_info->dev, &GXFB_DEV_BIN_ATTR_NAME(vpu)));
	if (ret) return ret;

	GXFB_LEAVE();
	return ret;
}

void gxfb_cleanup_device(struct fb_info *fb_info)
{
	GXFB_ENTER();

	if (!fb_info)
		return;

	device_remove_bin_file(fb_info->dev, &GXFB_DEV_BIN_ATTR_NAME(vpu));

	GXFB_LEAVE();
	return;
}

int fb_property_set(struct gxav_priv_dev *dev, void *arg){
	int ret = 0;
	unsigned long flag = 0;
	struct gxav_property *param = NULL;
	struct gxav_module *module = NULL;

	if((NULL == dev) || (NULL == arg)){
		return (-1);
	}

	param = (struct gxav_property *)arg;
	module = dev->av_dev->modules_handle[param->module_id]->module;
	switch (param->prop_id) {
	case GxVpuPropertyID_RWPalette:
		ret = gxav_vpu_WPalette((GxVpuProperty_RWPalette *) (param->prop_val));
		break;
	case GxVpuPropertyID_DestroyPalette:
		ret = gxav_vpu_DestroyPalette((GxVpuProperty_DestroyPalette *) (param->prop_val));
		break;
	case GxVpuPropertyID_SurfaceBindPalette:
		ret = gxav_vpu_SurfaceBindPalette((GxVpuProperty_SurfaceBindPalette *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerViewport:
		ret = gxav_vpu_SetLayerViewport((GxVpuProperty_LayerViewport *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerMainSurface:
		ret = gxav_vpu_SetLayerMainSurface((GxVpuProperty_LayerMainSurface *) (param->prop_val));
		break;
	case GxVpuPropertyID_UnsetLayerMainSurface:
		ret = gxav_vpu_UnsetLayerMainSurface((GxVpuProperty_LayerMainSurface *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerEnable:
		flag = gxav_device_spin_lock_irqsave(module->inode->dev);
		ret = gxav_vpu_SetLayerEnable((GxVpuProperty_LayerEnable *) (param->prop_val));
		gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);
		break;
	case GxVpuPropertyID_LayerAntiFlicker:
		ret = gxav_vpu_SetLayerAntiFlicker((GxVpuProperty_LayerAntiFlicker *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerOnTop:
		ret = gxav_vpu_SetLayerOnTop((GxVpuProperty_LayerOnTop *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerVideoMode:
		ret = gxav_vpu_SetLayerVideoMode((GxVpuProperty_LayerVideoMode *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerMixConfig:
		ret = gxav_vpu_SetLayerMixConfig((GxVpuProperty_LayerMixConfig*) (param->prop_val));
		break;
	case GxVpuPropertyID_DestroySurface:
		ret = gxav_vpu_SetDestroySurface((GxVpuProperty_DestroySurface *) (param->prop_val));
		break;
	case GxVpuPropertyID_Palette:
		ret = gxav_vpu_SetPalette((GxVpuProperty_Palette *) (param->prop_val));
		break;
	case GxVpuPropertyID_Alpha:
		ret = gxav_vpu_SetAlpha((GxVpuProperty_Alpha *) (param->prop_val));
		break;
	case GxVpuPropertyID_ColorFormat:
		ret = gxav_vpu_SetColorFormat((GxVpuProperty_ColorFormat *) (param->prop_val));
		break;
	case GxVpuPropertyID_ColorKey:
		ret = gxav_vpu_SetColorKey((GxVpuProperty_ColorKey *) (param->prop_val));
		break;
	case GxVpuPropertyID_BackColor:
		ret = gxav_vpu_SetBackColor((GxVpuProperty_BackColor *) (param->prop_val));
		break;
	case GxVpuPropertyID_Point:
		ret = gxav_vpu_SetPoint((GxVpuProperty_Point *) (param->prop_val));
		break;
	case GxVpuPropertyID_DrawLine:
		ret = gxav_vpu_SetDrawLine((GxVpuProperty_DrawLine *) (param->prop_val));
		break;
	case GxVpuPropertyID_Blit:
		ret = gxav_vpu_SetBlit((GxVpuProperty_Blit *) (param->prop_val));
		break;
	case GxVpuPropertyID_DfbBlit:
		ret = gxav_vpu_SetDfbBlit((GxVpuProperty_DfbBlit *) (param->prop_val));
		break;
	case GxVpuPropertyID_BatchDfbBlit:
		ret = gxav_vpu_SetBatchDfbBlit((GxVpuProperty_BatchDfbBlit *) (param->prop_val));
		break;
	case GxVpuPropertyID_FillRect:
		ret = gxav_vpu_SetFillRect((GxVpuProperty_FillRect *) (param->prop_val));
		break;
	case GxVpuPropertyID_FillPolygon:
		ret = gxav_vpu_SetFillPolygon((GxVpuProperty_FillPolygon *) (param->prop_val));
		break;
	case GxVpuPropertyID_SetGAMode:
		ret = gxav_vpu_SetGAMode((GxVpuProperty_SetGAMode *) (param->prop_val));
		break;
	case GxVpuPropertyID_WaitUpdate:
		ret = gxav_vpu_SetWaitUpdate((GxVpuProperty_WaitUpdate *) (param->prop_val));
		break;
	case GxVpuPropertyID_BeginUpdate:
		ret = gxav_vpu_SetBeginUpdate((GxVpuProperty_BeginUpdate *) (param->prop_val));
		break;
	case GxVpuPropertyID_EndUpdate:
		ret = gxav_vpu_SetEndUpdate((GxVpuProperty_EndUpdate *) (param->prop_val));
		break;
	case GxVpuPropertyID_ConvertColor:
		ret = gxav_vpu_SetConvertColor((GxVpuProperty_ConvertColor *) (param->prop_val));
		break;
	case GxVpuPropertyID_VirtualResolution:
		ret = gxav_vpu_SetVirtualResolution((GxVpuProperty_Resolution *) (param->prop_val));
		break;
	case GxVpuPropertyID_VbiEnable:
		ret = gxav_vpu_SetVbiEnable((GxVpuProperty_VbiEnable *) (param->prop_val));
		break;
	case GxVpuPropertyID_ZoomSurface:
		ret = gxav_vpu_ZoomSurface((GxVpuProperty_ZoomSurface*) (param->prop_val));
		break;
	case GxVpuPropertyID_Complet:
		ret = gxav_vpu_Complet((GxVpuProperty_Complet*) (param->prop_val));
		break;
	case GxVpuPropertyID_TurnSurface:
		ret = gxav_vpu_TurnSurface((GxVpuProperty_TurnSurface*) (param->prop_val));
		break;
	case GxVpuPropertyID_Sync:
		ret = gxav_vpu_Sync(((GxVpuProperty_Sync *)(param->prop_val)));
		break;
	case GxVpuPropertyID_LayerClipport:
		return gxav_vpu_SetLayerClipport((GxVpuProperty_LayerClipport*) (param->prop_val));
	case GxVpuPropertyID_RegistSurface:
		return gxav_vpu_RegistSurface((GxVpuProperty_RegistSurface*) (param->prop_val));
	case GxVpuPropertyID_UnregistSurface:
		return gxav_vpu_UnregistSurface((GxVpuProperty_UnregistSurface*) (param->prop_val));
	case GxVpuPropertyID_FlipSurface:
		return gxav_vpu_FlipSurface((GxVpuProperty_FlipSurface*) (param->prop_val));
	case GxVpuPropertyID_AddRegion:
		return gxav_vpu_AddRegion((GxVpuProperty_AddRegion*) (param->prop_val));
	case GxVpuPropertyID_RemoveRegion:
		return gxav_vpu_RemoveRegion((GxVpuProperty_RemoveRegion*) (param->prop_val));
	case GxVpuPropertyID_UpdateRegion:
		return gxav_vpu_UpdateRegion((GxVpuProperty_UpdateRegion*) (param->prop_val));
	case GxVideoOutPropertyID_Brightness:
		return gxav_videoout_brightness(0, (GxVideoOutProperty_Brightness *)(param->prop_val));
	case GxVideoOutPropertyID_Contrast:
		return gxav_videoout_contrast(0, (GxVideoOutProperty_Contrast *)(param->prop_val));
	case GxVideoOutPropertyID_Saturation:
		return gxav_videoout_saturation(0, (GxVideoOutProperty_Saturation *)(param->prop_val));
	case GxVideoOutPropertyID_Resolution:
		return gxav_videoout_resolution(0, (GxVideoOutProperty_Resolution *)(param->prop_val));
	case GxVideoOutPropertyID_AspectRatio:
		return gxav_videoout_aspratio(0, (GxVideoOutProperty_AspectRatio *)(param->prop_val));
	case GxVideoOutPropertyID_OutputSelect:
		return gxav_videoout_interface(0, (GxVideoOutProperty_OutputSelect *)(param->prop_val));
	default:
		printk("the parameter which vpu's property_id is wrong or not support! ID = %d\n", param->prop_id);
		return -2;
	}

	return (ret);
}

int fb_property_get(struct gxav_priv_dev *dev, void *arg){
	int ret = 0;
	struct gxav_property *param = NULL;

	if((NULL == dev) || (NULL == arg)){
		return (-1);
	}

	param = (struct gxav_property *)arg;
	switch (param->prop_id) {
	case GxVpuPropertyID_GetEntries:
		break;

	case GxVpuPropertyID_RWPalette:
		ret = gxav_vpu_RPalette((GxVpuProperty_RWPalette *) (param->prop_val));
		break;
	case GxVpuPropertyID_CreatePalette:
		ret = gxav_vpu_GetCreatePalette((GxVpuProperty_CreatePalette *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerViewport:
		ret = gxav_vpu_GetLayerViewport((GxVpuProperty_LayerViewport *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerClipport:
		ret = gxav_vpu_GetLayerClipport((GxVpuProperty_LayerClipport *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerMainSurface:
		ret = gxav_vpu_GetLayerMainSurface((GxVpuProperty_LayerMainSurface *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerEnable:
		ret = gxav_vpu_GetLayerEnable((GxVpuProperty_LayerEnable *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerAntiFlicker:
		ret = gxav_vpu_GetLayerAntiFlicker((GxVpuProperty_LayerAntiFlicker *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerOnTop:
		ret = gxav_vpu_GetLayerOnTop((GxVpuProperty_LayerOnTop *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerVideoMode:
		ret = gxav_vpu_GetLayerVideoMode((GxVpuProperty_LayerVideoMode *) (param->prop_val));
		break;
	case GxVpuPropertyID_LayerCapture:
		ret = gxav_vpu_GetLayerCapture((GxVpuProperty_LayerCapture *) (param->prop_val));
		break;
	case GxVpuPropertyID_CreateSurface:
		ret = gxav_vpu_GetCreateSurface((GxVpuProperty_CreateSurface *) (param->prop_val));
		break;
	case GxVpuPropertyID_ReadSurface:
		ret = gxav_vpu_GetReadSurface((GxVpuProperty_ReadSurface *) (param->prop_val));
		break;
	case GxVpuPropertyID_Palette:
		ret = gxav_vpu_GetPalette((GxVpuProperty_Palette *) (param->prop_val));
		break;
	case GxVpuPropertyID_Alpha:
		ret = gxav_vpu_GetAlpha((GxVpuProperty_Alpha *) (param->prop_val));
		break;
	case GxVpuPropertyID_ColorFormat:
		ret = gxav_vpu_GetColorFormat((GxVpuProperty_ColorFormat *) (param->prop_val));
		break;
	case GxVpuPropertyID_ColorKey:
		ret = gxav_vpu_GetColorKey((GxVpuProperty_ColorKey *) (param->prop_val));
		break;
	case GxVpuPropertyID_BackColor:
		ret = gxav_vpu_GetBackColor((GxVpuProperty_BackColor *) (param->prop_val));
		break;
	case GxVpuPropertyID_Point:
		ret = gxav_vpu_GetPoint((GxVpuProperty_Point *) (param->prop_val));
		break;
	case GxVpuPropertyID_ConvertColor:
		ret = gxav_vpu_GetConvertColor((GxVpuProperty_ConvertColor *) (param->prop_val));
		break;
	case GxVpuPropertyID_VirtualResolution:
		ret = gxav_vpu_GetVirtualResolution((GxVpuProperty_Resolution *) (param->prop_val));
		break;
	case GxVpuPropertyID_VbiEnable:
		ret = gxav_vpu_GetVbiEnable((GxVpuProperty_VbiEnable *) (param->prop_val));
		break;
	case GxVpuPropertyID_VbiCreateBuffer:
		ret = gxav_vpu_GetVbiCreateBuffer((GxVpuProperty_VbiCreateBuffer *) (param->prop_val));
		break;
	case GxVpuPropertyID_VbiReadAddress:
		ret = gxav_vpu_GetVbiReadAddress((GxVpuProperty_VbiReadAddress *) (param->prop_val));
		break;
	case GxVpuPropertyID_GetIdleSurface:
		return gxav_vpu_GetIdleSurface((GxVpuProperty_GetIdleSurface*) (param->prop_val));
	default:
		printk("the parameter which vpu's property_id is wrong or not support! ID =%d\n",param->prop_id);
		return -2;
	}

	return (ret);
}

