#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/io.h>

#include <linux/err.h>
#include <linux/fb.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>

#include "gxfb.h"
#include "gxfb_reg.h"
#include "gxfb_dep.h"
#include "gxavdev.h"
#include "avcore.h"
#include "vpu_hal.h"
#include "vout_hal.h"
#include "firewall.h"

static unsigned long gxfb_phys_base = 0;
static unsigned long gxfb_phys_size = 0;
static struct gx_mem_info info = {0};

/*Extern parameter*/
static char *gxvout = NULL;
static char *osdlayer = NULL;

#if 0
static GxColor gxfb_av_get_color_val(struct fb_info *info, unsigned int color)
{
#define GXFB_COLOR_VAL_MASK(v, c) \
	(((v) & (((1 << (c).length) - 1) << (c).offset)) >> (c).offset)

	GxColor c;

	GXFB_ENTER();

	gx_memset(&c, 00, sizeof(c));

	if (!info)
		return c;

	c.r = GXFB_COLOR_VAL_MASK(color, info->var.red);
	c.g = GXFB_COLOR_VAL_MASK(color, info->var.green);
	c.b = GXFB_COLOR_VAL_MASK(color, info->var.blue);
	c.a = GXFB_COLOR_VAL_MASK(color, info->var.transp);

	GXFB_LEAVE();

	return c;
}
#endif


static int
gxfb_av_get_color_fmt(struct fb_var_screeninfo *var, int *color_fmt)
{

#define IS_COLOR_RGBA(r,g,b,a) (((r).offset == 24) \
		&& ((g).offset == 16) \
		&& ((b).offset == 8) \
		&& ((a).offset == 0))
#define IS_COLOR_RGBA4444(r,g,b,a) (((r).offset == 12) \
		&& ((g).offset == 8) \
		&& ((b).offset == 4) \
		&& ((a).offset == 0))
#define IS_COLOR_RGBA5551(r,g,b,a) (((r).offset == 11) \
		&& ((g).offset == 6) \
		&& ((b).offset == 1) \
		&& ((a).offset == 0))

#define IS_COLOR_ARGB(r,g,b,a) (((r).offset == 16) \
		&& ((g).offset == 8) \
		&& ((b).offset == 0) \
		&& ((a).offset == 24))
#define IS_COLOR_ARGB4444(r,g,b,a) (((r).offset == 8) \
		&& ((g).offset == 4) \
		&& ((b).offset == 0) \
		&& ((a).offset == 12))
#define IS_COLOR_ARGB1555(r,g,b,a) (((r).offset == 10) \
		&& ((g).offset == 5) \
		&& ((b).offset == 0) \
		&& ((a).offset == 15))

#define IS_COLOR_RGB565(r,g,b) (((r).offset == 11) \
		&& ((g).offset == 5) \
		&& ((b).offset == 0))

#define IS_COLOR_RGB32(r,g,b) (((r).offset == 16) \
		&& ((g).offset == 8) \
		&& ((b).offset == 0))

	GXFB_ENTER();

	if (!var || !color_fmt)
		goto err_color_fmt;

	switch (var->bits_per_pixel)
	{
	case 16:
		if (IS_COLOR_RGBA4444(var->red, var->green, var->blue, var->transp))
			*color_fmt = GX_COLOR_FMT_RGBA4444;
		else if (IS_COLOR_ARGB4444(var->red, var->green, var->blue, var->transp))
			*color_fmt = GX_COLOR_FMT_ARGB4444;
		else if (IS_COLOR_RGBA5551(var->red, var->green, var->blue, var->transp))
			*color_fmt = GX_COLOR_FMT_RGBA5551;
		else if (IS_COLOR_ARGB1555(var->red, var->green, var->blue, var->transp))
			*color_fmt = GX_COLOR_FMT_ARGB1555;
		else if (IS_COLOR_RGB565(var->red, var->green, var->blue))
			*color_fmt = GX_COLOR_FMT_RGB565;
		else
			goto err_color_fmt;
		break;
	case 32:
		if (IS_COLOR_RGBA(var->red, var->green, var->blue, var->transp))
			*color_fmt = GX_COLOR_FMT_RGBA8888;
		else if (IS_COLOR_ARGB(var->red, var->green, var->blue, var->transp))
			*color_fmt = GX_COLOR_FMT_ARGB8888;
		else if (IS_COLOR_RGB32(var->red, var->green, var->blue))
			*color_fmt = GX_COLOR_FMT_ARGB8888;
		else
			goto err_color_fmt;
		break;
	case 1: *color_fmt = GX_COLOR_FMT_CLUT1; break;
	case 2: *color_fmt = GX_COLOR_FMT_CLUT2; break;
	case 4: *color_fmt = GX_COLOR_FMT_CLUT4; break;
	case 8: *color_fmt = GX_COLOR_FMT_CLUT8; break;
	default:
			goto err_color_fmt;
	}

	GXFB_LEAVE();
	return 0;

err_color_fmt:
	GXFB_LOG(GXFB_WARN, "default bits_per_pixel\n");
	return -EINVAL;
}


static int
gxfb_av_module_property_para_set(struct gxfb_pri *pri,
		struct gxav_property *param, void *prop)
{
	GXFB_ENTER();

	if (!pri || !prop || !param)
		return -EFAULT;

	param->module_id = pri->module_id;

	GXFB_LOG(GXFB_TACE, "property id = %d\n", param->prop_id);

	if (GX_FB_LAYER != pri->resource_id)
		return 0;

	switch (param->prop_id) {
	case GxVpuPropertyID_ColorKey: {
		GxVpuProperty_ColorKey *color_key = (GxVpuProperty_ColorKey *)prop;
		color_key->surface = pri->surface;
	}
		break;
	case GxVpuPropertyID_Alpha: {
		GxVpuProperty_Alpha *alpha = (GxVpuProperty_Alpha *)prop;
		alpha->surface = pri->surface;
	}
		break;
	case GxVpuPropertyID_ColorFormat: {
		GxVpuProperty_ColorFormat *fmt = (GxVpuProperty_ColorFormat *)prop;
		fmt->surface = pri->surface;
	}
		break;

	case GxVpuPropertyID_LayerEnable: {
		GxVpuProperty_LayerEnable *layer_en = (GxVpuProperty_LayerEnable *)prop;
		layer_en->layer = pri->resource_id;
	}
		break;
	case GxVpuPropertyID_DestroyPalette:
	case GxVpuPropertyID_DestroySurface:
	case GxVpuPropertyID_CreateSurface:
	case GxVpuPropertyID_FillRect:
	case GxVpuPropertyID_Blit:
	case GxVpuPropertyID_ZoomSurface:
	case GxVpuPropertyID_TurnSurface:
	case GxVpuPropertyID_BeginUpdate:
	case GxVpuPropertyID_EndUpdate:
	case GxVpuPropertyID_Sync:
		break;
	default:
		GXFB_LOG(GXFB_WARN, "default property_para_set, id = %d\n", param->prop_id);
		break;
	}

	GXFB_LEAVE();
	return 0;
}


static void gxfb_print_fix(struct fb_fix_screeninfo *fix)
{
	if (!fix)
		return;

	GXFB_LOG(GXFB_TACE, "\n------------------------------\n\n");

	GXFB_LOG(GXFB_TACE, "fix->type = 0x%x\n", fix->type);
	GXFB_LOG(GXFB_TACE, "fix->type_aux = 0x%x\n", fix->type_aux);
	GXFB_LOG(GXFB_TACE, "fix->xpanstep = 0x%x\n", fix->xpanstep);
	GXFB_LOG(GXFB_TACE, "fix->ypanstep = 0x%x\n", fix->ypanstep);
	GXFB_LOG(GXFB_TACE, "fix->ywrapstep = 0x%x\n", fix->ywrapstep);

	GXFB_LOG(GXFB_TACE, "fix->accel = 0x%x\n", fix->accel);
	GXFB_LOG(GXFB_TACE, "fix->visual = 0x%x\n", fix->visual);
	GXFB_LOG(GXFB_TACE, "fix->smem_len = 0x%x\n", fix->smem_len);
	GXFB_LOG(GXFB_TACE, "fix->line_length = 0x%x\n", fix->line_length);
	GXFB_LOG(GXFB_TACE, "fix->smem_start = 0x%lx\n", fix->smem_start);

	GXFB_LOG(GXFB_TACE, "\n------------------------------\n\n");

	GXFB_LEAVE();

	return;
}

static void gxfb_print_var(struct fb_var_screeninfo *var)
{
	GXFB_ENTER();

	if (!var)
		return;

	GXFB_LOG(GXFB_TACE, "\n------------------------------\n\n");

	GXFB_LOG(GXFB_TACE, "var->nonstd = 0x%x\n", var->nonstd);
	GXFB_LOG(GXFB_TACE, "var->activate = 0x%x\n", var->activate);
	GXFB_LOG(GXFB_TACE, "var->accel_flags = 0x%x\n", var->accel_flags);
	GXFB_LOG(GXFB_TACE, "var->vmode = 0x%x\n", var->vmode);

	GXFB_LOG(GXFB_TACE, "var->xres = 0x%x\n", var->xres);
	GXFB_LOG(GXFB_TACE, "var->yres = 0x%x\n", var->yres);
	GXFB_LOG(GXFB_TACE, "var->xres_virtual = 0x%x\n", var->xres_virtual);
	GXFB_LOG(GXFB_TACE, "var->yres_virtual = 0x%x\n", var->yres_virtual);
	GXFB_LOG(GXFB_TACE, "var->xoffset = 0x%x\n", var->xoffset);
	GXFB_LOG(GXFB_TACE, "var->yoffset = 0x%x\n", var->yoffset);

	GXFB_LOG(GXFB_TACE, "var->bits_per_pixel = 0x%x\n", var->bits_per_pixel);

	GXFB_LOG(GXFB_TACE, "var->red.offset = 0x%x\n", var->red.offset);
	GXFB_LOG(GXFB_TACE, "var->green.offset = 0x%x\n", var->green.offset);
	GXFB_LOG(GXFB_TACE, "var->blue.offset = 0x%x\n", var->blue.offset);
	GXFB_LOG(GXFB_TACE, "var->transp.offset = 0x%x\n", var->transp.offset);

	GXFB_LOG(GXFB_TACE, "var->red.length = 0x%x\n", var->red.length);
	GXFB_LOG(GXFB_TACE, "var->green.length = 0x%x\n", var->green.length);
	GXFB_LOG(GXFB_TACE, "var->blue.length = 0x%x\n", var->blue.length);
	GXFB_LOG(GXFB_TACE, "var->transp.length = 0x%x\n", var->transp.length);

	GXFB_LOG(GXFB_TACE, "\n------------------------------\n\n");

	GXFB_LEAVE();

	return;
}

static int gxfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{

	int color_fmt = 0;

	GXFB_ENTER();

	if (!var || !info)
		return -EINVAL;

	if ((var->xres != GX_FB_MAX_XRES)
			|| (var->yres != GX_FB_MAX_YRES)
			|| (var->xres_virtual != GX_FB_MAX_XRES_VIR)) {
		var->xres = GX_FB_MAX_XRES;
		var->yres = GX_FB_MAX_YRES;
		var->xres_virtual = GX_FB_MAX_XRES_VIR;
		GXFB_LOG(GXFB_WARN, "resolution repair\n");
	}

	if (gxfb_av_get_color_fmt(var, &color_fmt))
		goto err_check_var;

	if ((var->yres_virtual * var->xres_virtual * var->bits_per_pixel >> 3) > GX_FB_MIN_BUFFER_SIZE)
		goto err_check_var;

	var->nonstd	= 0;
	var->xoffset	= 0;
	var->yoffset	= 0;

	GXFB_LEAVE();
	return 0;

err_check_var:
	gxfb_print_var(var);
	return -EINVAL;
}

static int gxfb_alloc_screenbase(void)
{
	int ret = gx_mem_info_get("fbmem", &info);
	if (ret == 0) {
		gxfb_phys_size = info.size;
		gxfb_phys_base = (unsigned long)gx_phys_to_virt(info.start);
	}

	if(0 == gxfb_phys_base || info.size < GX_FB_MIN_BUFFER_SIZE) {
		gxfb_phys_base = (unsigned long)gx_page_malloc(GX_FB_MIN_BUFFER_SIZE);
		if(0 == gxfb_phys_base) {
			printk("framebuffer alloc error\n");
			return -ENODEV;
		}
		gxfb_phys_size = GX_FB_MIN_BUFFER_SIZE;
	}

	return 0;
}

static void gxfb_dealloc_screenbase(void)
{
	if (info.start) {
		gx_page_free((unsigned char *)gxfb_phys_base, gxfb_phys_size);
	}
}

static int gxfb_info_init(struct fb_info *info, struct device *dev)
{
	GXFB_ENTER();

	if (!info)
		return -EINVAL;

	info->var.nonstd	= 0;
	info->var.activate	= FB_ACTIVATE_NOW;
	info->var.accel_flags   = 0;

	info->var.vmode	        = FB_VMODE_NONINTERLACED;

	info->flags		= FBINFO_DEFAULT
		| FBINFO_HWACCEL_COPYAREA
		| FBINFO_HWACCEL_FILLRECT
		| FBINFO_HWACCEL_IMAGEBLIT;
	info->flags &= ~FBINFO_HWACCEL_DISABLED;

	info->var.xres	        = GX_FB_MAX_XRES;
	info->var.yres	        = GX_FB_MAX_YRES;
	info->var.xres_virtual  = GX_FB_MAX_XRES_VIR;
	info->var.yres_virtual  = GX_FB_MAX_YRES_VIR;
	info->var.xoffset	= 0;
	info->var.yoffset	= 0;

	info->var.bits_per_pixel= GX_FB_MAX_BIT_PER_PIXEL;

	info->var.red.offset      = 16;
	info->var.green.offset    = 8;
	info->var.blue.offset     = 0;
	info->var.transp.offset   = 24;

	info->var.red.length      = 8;
	info->var.green.length    = 8;
	info->var.blue.length     = 8;
	info->var.transp.length   = 8;

	if (gxfb_check_var(&info->var, info))
		return -EINVAL;

	info->pseudo_palette    = NULL;

	if (dev && dev->platform_data)
		info->fbops = (struct fb_ops *)dev->platform_data;

	info->fix.type		= FB_TYPE_PACKED_PIXELS;
	info->fix.type_aux	= 0;
	info->fix.xpanstep	= 0;
	info->fix.ypanstep	= info->var.yres;
	info->fix.ywrapstep	= 0;
	info->fix.accel	    = GX_FB_ACCEL_NATIONALCHIP;
	info->fix.visual    = FB_VISUAL_TRUECOLOR;

	info->fix.line_length = info->var.xres_virtual * info->var.bits_per_pixel >> 3;

	info->fix.smem_start = gxfb_phys_base;
	info->fix.smem_len   = gxfb_phys_size;
	info->screen_base    = (char *)(info->fix.smem_start);

	if(fb_alloc_cmap(&info->cmap,GX_FB_MAX_CMAP_LEN,1))
		goto err_cmap;

	info->fix.mmio_len = GX_FB_ACCEL_ADDR_SIZE;
	info->fix.mmio_start = (unsigned long)ioremap((int)gx_page_malloc(info->fix.mmio_len),
			info->fix.mmio_len);
	if (!info->fix.mmio_start)
		goto err_mmio_start;

	GXFB_LEAVE();

	return 0;

err_mmio_start:
	fb_dealloc_cmap(&info->cmap);
err_cmap:
	gxfb_dealloc_screenbase();
	return -ENOMEM;;
}

static void gxfb_info_exit(struct fb_info *info)
{
	if (!info)
		return;
	iounmap((void *)info->fix.mmio_start);
	fb_dealloc_cmap(&info->cmap);
	gxfb_dealloc_screenbase();

	return;
}

static int gxfb_pri_init(struct fb_info *info)
{
	struct gxfb_pri *pri = NULL;

	GXFB_ENTER();

	if (!info || !(info->par))
		return -EINVAL;

	pri = (struct gxfb_pri *)info->par;

	pri->pseudo_palette = kzalloc(256 * 4, GFP_KERNEL);
	if (!pri->pseudo_palette) {
		GXFB_LOG(GXFB_ERR, "pseudo_palette null error\n");
		return -EINVAL;
	}
	info->pseudo_palette = pri->pseudo_palette;
	pri->fb_phys_base = info->fix.smem_start;
	pri->fb_phys_offset = 0;
	pri->module_id = -1;
	pri->resource_id = -1;

	GXFB_LOG(GXFB_TACE, "fb_phys_base = 0x%lx\n", pri->fb_phys_base);

	GXFB_LEAVE();
	return 0;
}

static void gxfb_pri_exit(struct fb_info *info)
{
	struct gxfb_pri *pri = NULL;

	GXFB_ENTER();

	if (!info || !(info->par))
		return;

	pri = (struct gxfb_pri *)info->par;

	kfree(pri->pseudo_palette);
	info->pseudo_palette = pri->pseudo_palette = NULL;

	GXFB_LEAVE();
	return;
}

static int gxfb_enable(struct fb_info *info, int enable)
{
	int ret = 0;
	GxVpuProperty_LayerEnable layer_en = {0};

	GXFB_ENTER();

	layer_en.enable = enable;
	GXFB_CHECK_RET(ret = gxav_vpu_SetLayerEnable(&layer_en));
	if (ret) return -EINVAL;

	GXFB_LEAVE();
	return 0;
}

static int gxfb_close(struct fb_info *info)
{
	GxVpuProperty_DestroySurface dst_surf = {0,};
	GxVpuProperty_DestroyPalette palette = {0};
	struct gxav_priv_dev *dev = NULL;
	struct gxav_module_set mparam = {0};
	struct gxfb_pri *pri = NULL;
	int ret = 0;

	GXFB_ENTER();

	if (!info || !info->par)
		return -EINVAL;

	pri = (struct gxfb_pri *)info->par;
	dev = (struct gxav_priv_dev *)pri->avdev;
	pri->pm_mode = GX_FB_POWERDOWN;

	if (gxfb_enable(info, pri->pm_mode))
		return -EINVAL;

	if (!!(palette.palette = pri->palette)) {
		GXFB_CHECK_RET(ret = gxav_vpu_DestroyPalette(&palette));
		if (ret) return -EINVAL;

		pri->palette = NULL;
	}

	if (!!(dst_surf.surface = pri->surface)) {
		GXFB_CHECK_RET(ret = gxav_vpu_SetDestroySurface(&dst_surf));
		if (ret) return -EINVAL;

		pri->surface = NULL;
	}

	if ((mparam.module_id = pri->module_id) >= 0) {
		GXFB_CHECK_RET(ret = gxav_mod_close(dev, mparam.module_id));
		if (ret) return -EFAULT;
		pri->module_id = -1;
	}

	if ((mparam.module_id = pri->vout_id) >= 0) {
		GXFB_CHECK_RET(ret = gxav_mod_close(dev, mparam.module_id));
		if (ret) return -EFAULT;
		pri->vout_id = -1;
	}

	gxav_dev_close(0);

	if(pri->vout_tmp_buf)
	{
		gx_page_free(pri->vout_tmp_buf, VOUT_BUF_SIZE);
		pri->vout_tmp_buf = NULL;
	}

	GXFB_LEAVE();

	printk("##########gxfb close----end#################\n");
	return 0;
}

static char *_strtok_r(char *s, const char *delim, char **save_ptr)
{
	char *token = NULL;

	if (s == NULL)
		s = *save_ptr;

	s += strspn(s, delim);
	if (*s == '\0')
		return NULL;

	token = s;
	s = strpbrk(token, delim);  //查找字符串中第一个出现的指定字符

	if (s == NULL)
		*save_ptr = strchr(token, '\0');
	else {
		*s = '\0';
		*save_ptr = s + 1;
	}

	return token;
}

static void _set_iface(char *iface, int *outputselect)
{
	if(0 == strcasecmp("rca", iface)) {
		*outputselect |= GXAV_VOUT_RCA;
	} else if(0 == strcasecmp("rca1", iface)) {
		*outputselect |= GXAV_VOUT_RCA1;
	} else if(0 == strcasecmp("yuv", iface)) {
		*outputselect |= GXAV_VOUT_YUV;
	} else if(0 == strcasecmp("scart", iface)) {
		*outputselect |= GXAV_VOUT_SCART;
	} else if(0 == strcasecmp("svideo", iface)) {
		*outputselect |= GXAV_VOUT_SVIDEO;
	} else if(0 == strcasecmp("hdmi", iface)) {
		*outputselect |= GXAV_VOUT_HDMI;
	}
}

static void _set_resolution_mode(const char *vout, int *mode)
{
	if(0 == strcasecmp("pal", vout)) {
		*mode = GXAV_VOUT_PAL;
	} else if(0 == strcasecmp("pal_m", vout)) {
		*mode = GXAV_VOUT_PAL_M;
	} else if(0 == strcasecmp("pal_n", vout)) {
		*mode = GXAV_VOUT_PAL_N;
	} else if(0 == strcasecmp("pal_nc", vout)) {
		*mode = GXAV_VOUT_PAL_NC;
	} else if(0 == strcasecmp("ntsc_m", vout)) {
		*mode = GXAV_VOUT_NTSC_M;
	} else if(0 == strcasecmp("ntsc_443", vout)) {
		*mode = GXAV_VOUT_NTSC_443;
	} else if(0 == strcasecmp("480i", vout)) {
		*mode = GXAV_VOUT_480I;
	} else if(0 == strcasecmp("480p", vout)) {
		*mode = GXAV_VOUT_480P;
	} else if(0 == strcasecmp("576i", vout)) {
		*mode = GXAV_VOUT_576I;
	} else if(0 == strcasecmp("576p", vout)) {
		*mode = GXAV_VOUT_576P;
	} else if(0 == strcasecmp("720p@50hz", vout)) {
		*mode = GXAV_VOUT_720P_50HZ;
	} else if(0 == strcasecmp("720p@60hz", vout)) {
		*mode = GXAV_VOUT_720P_60HZ;
	} else if(0 == strcasecmp("1080i@50hz", vout)) {
		*mode = GXAV_VOUT_1080I_50HZ;
	} else if(0 == strcasecmp("1080p@50hz", vout)) {
		*mode = GXAV_VOUT_1080P_50HZ;
	} else if(0 == strcasecmp("1080i@50hz", vout)) {
		*mode = GXAV_VOUT_1080I_60HZ;
	} else if(0 == strcasecmp("1080p@60hz", vout)) {
		*mode = GXAV_VOUT_1080P_60HZ;
	} else {
		// default
		*mode = GXAV_VOUT_1080I_50HZ;
	}
}

static void _get_vout_param(char *vout, int *iface0, int *vmode0, int *vmode1)
{
	char *iface = NULL, *vmode = NULL;
	char *iface_str[2] = {0}, *vmode_str[2] = {0};

	if(vout) {
		iface = _strtok_r(vout, ",", &vmode);
	}

	if((iface) && (0 == strcasecmp("none", iface))) {
		*iface0 = -1;
	} else if(iface) {
		iface_str[0] = _strtok_r(iface, "|", &(iface_str[1]));
		_set_iface(iface_str[0], iface0);
		_set_iface(iface_str[1], iface0);
	}

	if((vmode) && (0 == strcasecmp("none", vmode))) {
		*vmode0 = -1;
	} else if(vmode) {
		vmode_str[0] = _strtok_r(vmode, "|", &(vmode_str[1]));
		_set_resolution_mode(vmode_str[0], vmode0);
		*vmode1 = -1;
		_set_resolution_mode(vmode_str[1], vmode1);
	}
}

static int _set_vout_config(struct fb_info *info)
{
	GxVideoOutProperty_OutputConfig OutputConfig = {0};
	unsigned char *buf = NULL;
	struct gxfb_pri *gxfb_data = (struct gxfb_pri *)info->par;
	int ret = 0;
	struct gx_mem_info meminfo;

	ret = gx_mem_info_get("svpumem", &meminfo);
	if(ret == 0 && meminfo.start) {
		//printk("\n%s:%d, addr = 0x%x\n", __func__, __LINE__, meminfo.start);
		buf = (unsigned char *)gx_phys_to_virt(meminfo.start);
		//printk("\n%s:%d, addr = 0x%x\n", __func__, __LINE__, buf);
	}
	else {
		buf = gx_page_malloc(VOUT_BUF_SIZE);
	}

	if (buf){
		OutputConfig.svpu_enable = 1;
		OutputConfig.svpu_buf[0] = buf;
		OutputConfig.svpu_buf[1] = buf;
		OutputConfig.svpu_buf[2] = buf;
	}

	OutputConfig.vout1_auto.enable = 1;
	OutputConfig.vout1_auto.pal    = GXAV_VOUT_PAL;
	OutputConfig.vout1_auto.ntsc   = GXAV_VOUT_NTSC_M;

	gxfb_data->vout_tmp_buf = buf;
	GXFB_CHECK_RET(ret = gxav_videoout_config(0, &OutputConfig));
	if (ret) {
		GXFB_LOG(GXFB_ERR, "OutputConfig error\n");
		return (-1);
	}

	return (0);
}

static int _set_video_out(struct gxav_priv_dev *dev, struct fb_info *info)
{
	GxVideoOutProperty_OutputSelect OutputSelect = {GXAV_VOUT_HDMI|GXAV_VOUT_RCA};//|GXAV_VOUT_YUV
	GxVpuProperty_VirtualResolution VResolution = {GX_FB_MAX_XRES_VIR, GX_FB_MAX_YRES_VIR};
	GxVideoOutProperty_Resolution OResolution = {GXAV_VOUT_HDMI, GXAV_VOUT_1080I_50HZ};
	struct gxav_property param = {0};
	struct gxfb_pri *gxfb_data = (struct gxfb_pri *)info->par;
	int ret = 0, vout_id = 0, vmode0 = GXAV_VOUT_1080I_50HZ, vmode1 = GXAV_VOUT_PAL;

	vout_id = gxav_mod_open(dev, GXAV_MOD_VIDEO_OUT, 0);
	if (vout_id <= 0){
		return -EFAULT;
	}

	param.module_id = gxfb_data->module_id;
	gxfb_data->vout_id = param.module_id;

	GXFB_CHECK_RET(ret = gxav_vpu_SetVirtualResolution(&VResolution));
	if (ret) {
		GXFB_LOG(GXFB_ERR, "VirtualResolution error\n");
		return (-1);
	}

	if(gxvout) {
		_get_vout_param(gxvout,
				&(OutputSelect.selection),
				&vmode0,
				&vmode1);
	}

	if(-1 == OutputSelect.selection) {
		goto VOUT;
	}

	param.module_id = vout_id;
	GXFB_CHECK_RET(ret = gxav_videoout_interface(0, &OutputSelect));
	if (ret) {
		GXFB_LOG(GXFB_ERR, "OutputSelect error\n");
		return (-1);
	}

	ret = _set_vout_config(info);
	if(ret) {
		GXFB_LOG(GXFB_ERR, "VoutConfig - 1 error\n");
		return (-1);
	}

VOUT:
	if(-1 == vmode0) {
		return (0);
	}

	GXFB_CHECK_RET(ret = gxav_videoout_resolution(0, &OResolution));
	if (ret) {
		GXFB_LOG(GXFB_ERR, "Resolution - 0 error\n");
		return (-1);
	}

	if(OutputSelect.selection | GXAV_VOUT_RCA) {
		OResolution.iface = GXAV_VOUT_RCA;
		OResolution.mode = vmode1;

		GXFB_CHECK_RET(ret = gxav_videoout_resolution(0, &OResolution));
		if (ret) {
			GXFB_LOG(GXFB_ERR, "Resolution - 1 error\n");
			return (-1);
		}
	}

	return (0);
}

static int gxfb_open(struct fb_info *info)
{
	GxVpuProperty_Alpha Alpha;
	GxVpuProperty_DestroySurface dst_surf = {0,};
	GxVpuProperty_CreateSurface cre_surf = {0};
	GxVpuProperty_LayerMainSurface main_surf = {0};
	GxVpuProperty_Resolution res = {0};
	GxVpuProperty_LayerViewport view = {0};

	GxVpuProperty_CreatePalette palette = {0};
	GxVpuProperty_SurfaceBindPalette bind_palette = {0};
	GxVpuProperty_FillRect fill_rect = {0};

	struct gxav_priv_dev *dev = NULL;
	struct gxav_property param = {0};
	struct gxfb_pri *gxfb_data = (struct gxfb_pri *)info->par;
	int color_fmt = 0;
	int ret = 0;

	GXFB_ENTER();
	if(gxfb_data->surface){
		dst_surf.surface = gxfb_data->surface;
		GXFB_CHECK_RET(ret = gxav_vpu_SetDestroySurface(&dst_surf));
		gxfb_data->surface = NULL;
		gxfb_data->fb_phys_offset = 0;
	}

	dev = gxav_dev_open(0);
	if (!dev || !gxfb_data)
		return -ENODEV;

	if (gxfb_data->surface) {
		if (gxfb_close(info)) {
			GXFB_LOG(GXFB_ERR, "gxfb_close error\n");
			return -EFAULT;
		}
	}

	param.module_id = gxav_mod_open(dev, GXAV_MOD_VPU, 0);
	if (param.module_id <= 0){
		return -EFAULT;
	}
	gxfb_data->module_id = param.module_id;

	if (_set_video_out(dev, info))
		return -EFAULT;

	if (gxfb_av_get_color_fmt(&info->var, &color_fmt))
		goto err_module;

	cre_surf.buffer = (void *)(gxfb_data->fb_phys_base + gxfb_data->fb_phys_offset);
	cre_surf.width  = info->var.xres;
	cre_surf.height = info->var.yres;
	cre_surf.mode   = GX_SURFACE_MODE_IMAGE;
	cre_surf.format = color_fmt;
	GXFB_LOG(GXFB_TACE, "screen_base color type = 0x%x\n", color_fmt);
	GXFB_LOG(GXFB_TACE, "screen_base phys addr = %p\n", cre_surf.buffer);

	GXFB_CHECK_RET(ret = gxav_vpu_GetCreateSurface(&cre_surf));
	if (ret) goto err_module;

	palette.palette_num = 1;
	palette.num_entries = GX_FB_MAX_CMAP_LEN;

	GXFB_CHECK_RET(ret = gxav_vpu_GetCreatePalette(&palette));
	if (ret) goto err_surf;

	bind_palette.palette = palette.palette;
	bind_palette.surface = cre_surf.surface;

	GXFB_CHECK_RET(ret = gxav_vpu_SurfaceBindPalette(&bind_palette));
	if (ret) goto err_surf;

	Alpha.surface = cre_surf.surface;
	Alpha.alpha.type = GX_ALPHA_PIXEL;
	Alpha.alpha.value = 0xFF;
	GXFB_CHECK_RET(ret = gxav_vpu_SetAlpha(&Alpha));
	if (ret) goto err_surf;

	main_surf.layer = GX_FB_LAYER;
	main_surf.surface = cre_surf.surface;

	GXFB_CHECK_RET(ret = gxav_vpu_SetLayerMainSurface(&main_surf));
	if (ret) goto err_surf;

	GXFB_CHECK_RET(ret = gxav_vpu_GetVirtualResolution(&res));
	if (ret) goto err_surf;

	view.layer = main_surf.layer;
	view.rect.x = 0;
	view.rect.y = 0;
	view.rect.width = res.xres;
	view.rect.height= res.yres;

	GXFB_CHECK_RET(ret = gxav_vpu_SetLayerViewport(&view));
	if (ret) goto err_surf;

	fill_rect.surface = cre_surf.surface;
	fill_rect.is_ksurface = 1;
	fill_rect.rect.x = fill_rect.rect.y = 0;
	fill_rect.rect.width = cre_surf.width;
	fill_rect.rect.height = cre_surf.height;
	fill_rect.color.r = fill_rect.color.g = fill_rect.color.b = fill_rect.color.a = 0;
	GXFB_CHECK_RET(ret = gxav_vpu_SetFillRect(&fill_rect));
	if (ret) goto err_surf;

	gxfb_data->avdev = dev;
	gxfb_data->surface = main_surf.surface;
	gxfb_data->palette = palette.palette;
	gxfb_data->color_fmt = color_fmt;
	gxfb_data->pm_mode = GX_FB_POWERUP;
	gxfb_data->resource_id = main_surf.layer;

	if((NULL == osdlayer) || (0 == strcasecmp("enable", osdlayer))) {
		if (gxfb_enable(info, gxfb_data->pm_mode))
			goto err_surf;
	} else if(0 == strcasecmp("disable", osdlayer)) {
		if (gxfb_enable(info, false))
			goto err_surf;
	}

	GXFB_LEAVE();
	return 0;

err_surf:
	dst_surf.surface = cre_surf.surface;

	GXFB_CHECK_RET(ret = gxav_vpu_SetDestroySurface(&dst_surf));
	if (ret) return -EFAULT;
err_module:
	GXFB_CHECK_RET(ret = gxav_mod_close(dev, param.module_id));
	if (ret) return -EFAULT;

	return -EFAULT;
}


extern int gxfb_init_device(struct fb_info *fb_info);
extern void gxfb_cleanup_device(struct fb_info *fb_info);

module_param(gxvout, charp, S_IRUGO);
module_param(osdlayer, charp, S_IRUGO);

static int gxfb_probe(struct platform_device *pdev)
{
	struct fb_info *fb_info = NULL;
	struct gxfb_pri *gxfb_data = NULL;

	printk("gxfb probe\n");
	GXFB_ENTER();

	if (!pdev)
		return -EINVAL;

	fb_info = framebuffer_alloc(sizeof(*gxfb_data),&pdev->dev);
	if (!fb_info)
		return -ENOMEM;

	if (gxfb_info_init(fb_info, &pdev->dev))
		goto err_probe;

	if (gxfb_pri_init(fb_info))
		goto err_probe;

	if (gxfb_open(fb_info))
		goto err_probe;

	dev_info(&pdev->dev, "framebuffer at 0x%lx, 0x%x bytes, mapped to 0x%p\n",
			     fb_info->fix.smem_start, fb_info->fix.smem_len,
			     fb_info->screen_base);
	dev_info(&pdev->dev, "format=a8r8g8b8, mode=%dx%dx%d, linelength=%d\n",
			     fb_info->var.xres, fb_info->var.yres,
			     fb_info->var.bits_per_pixel, fb_info->fix.line_length);

	if (register_framebuffer(fb_info)) {
		dev_err(&pdev->dev, "Unable to register gxfb\n");
		goto err_probe;
	}

	dev_info(&pdev->dev, "fb%d: simplefb registered!\n", fb_info->node);

	if (gxfb_init_device(fb_info))
		goto err_probe;

	gxfb_print_var(&fb_info->var);
	gxfb_print_fix(&fb_info->fix);

	platform_set_drvdata(pdev, fb_info);
	GXFB_LEAVE();

	GXFB_LOG(GXFB_ANOUCE, "framebuffer start ok\n");
	return 0;

err_probe:
	GXFB_LOG(GXFB_ERR, "Gx framebuffer probe error\n");
	framebuffer_release(fb_info);
	return -EINVAL;
}


static int gxfb_remove(struct platform_device *pdev)
{
	struct fb_info *info = NULL;

	GXFB_ENTER();

	info = platform_get_drvdata(pdev);
	if (!info)
		return -EINVAL;

	unregister_framebuffer(info);
	if (gxfb_close(info))
		GXFB_LOG(GXFB_ERR, "gxfb_close error\n");
	gxfb_cleanup_device(info);
	gxfb_pri_exit(info);
	gxfb_info_exit(info);
	framebuffer_release(info);

	platform_set_drvdata(pdev,NULL);

	GXFB_LEAVE();
	return 0;
}

static int
gxfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct gxfb_pri *pri = NULL;
	struct gxfb_pri par;
	struct gxav_priv_dev *dev = NULL;
	void __user *argp = (void __user *)arg;
	int ret = 0;

	GXFB_ENTER();

	if (!info || !(info->par))
		return -EFAULT;

	pri = (struct gxfb_pri *)info->par;
	dev = pri->avdev;
	if (!dev)
		return -EFAULT;

	GXFB_LOG(GXFB_TACE, "ioctl cmd: %x, arg: %lx\n", cmd, arg);

ioctl_fb_layer:
	switch (cmd) {
		// TODO: add csky support
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)
	case FBIO_WAITFORVSYNC:
		gx_dcache_clean_range(0,0);
		break;
#endif
	case GXAV_IOCTL_PROPERTY_SET:
	case GXAV_IOCTL_PROPERTY_GET:
		{
			struct gxav_property param;
			void *prop = NULL;
			void __user *prop_val = NULL;

			gx_memset(&param, 0, sizeof(param));
			if (copy_from_user(&param, (char *)argp, sizeof(param)))
				return -EFAULT;
			/* protect userspace info */
			prop_val = param.prop_val;

			if (param.prop_size > 0) {
				prop = kzalloc(param.prop_size, GFP_KERNEL);
				if (copy_from_user(prop, param.prop_val,
							param.prop_size)) {
					kfree(prop);
					return -EFAULT;
				}
			}

			gxfb_av_module_property_para_set(pri, &param, prop);

			if (cmd == GXAV_IOCTL_PROPERTY_GET) {

				param.prop_val = prop;
				GXFB_CHECK_RET(ret = fb_property_get(dev, &param));
				param.prop_val = prop_val;
				if (copy_to_user(param.prop_val,
							prop,
							param.prop_size)) {
					kfree(prop);
					return -EFAULT;
				}
			} else {
				param.prop_val = prop;
				GXFB_CHECK_RET(ret = fb_property_set(dev, &param));
				param.prop_val = prop_val;
			}

			kfree(prop);

			if (copy_to_user(argp, &param, sizeof(param)))
				return -EFAULT;

			return param.ret;
		}
		break;
		/* support for vpu layers except osd layer */
	case GXFB_IOCTL_PROPERTY_SET:
	case GXFB_IOCTL_PROPERTY_GET:
		memcpy(&par, pri, sizeof(*pri));
		pri = &par;
		pri->resource_id = GX_NFB_LAYER;
		cmd = cmd == GXFB_IOCTL_PROPERTY_GET
			? GXAV_IOCTL_PROPERTY_GET
			: GXAV_IOCTL_PROPERTY_SET;
		goto ioctl_fb_layer;

		break;
	default:
		return -EFAULT;
	}
	GXFB_LEAVE();
	return 0;
}

#if 0
static void gxfb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	GxVpuProperty_FillRect fill = {0};
	struct gxav_priv_dev *dev = NULL;
	struct gxfb_pri *pri = NULL;
	GxColor color;

	GXFB_ENTER();

	if (!info || !(info->par) || !rect) {
		GXFB_LOG(GXFB_ERR, "-EINVAL\n");
		return;
	}
	pri = (struct gxfb_pri *)info->par;
	dev = (struct gxav_priv_dev *)pri->avdev;
	if (!dev)
		return;

	color = gxfb_av_get_color_val(info, rect->color);

	fill.surface = pri->surface;
	fill.rect.x = rect->dx;
	fill.rect.y = rect->dy;
	fill.rect.width = rect->width;
	fill.rect.height = rect->height;
	fill.color = color;
	/* todo
	   fill.rop = rect->rop;
	   */
	if(gxav_module_set_property(dev, pri->module_id,
				GxVpuPropertyID_FillRect, &fill,
				sizeof(fill)))
		return;

	GXFB_LEAVE();
	return;
}


static void
gxfb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	GxVpuProperty_Blit copyarea;
	struct gxav_priv_dev *dev = NULL;
	struct gxfb_pri *pri = NULL;

	GXFB_ENTER();

	if (!info || !(info->par) || !area) {
		GXFB_LOG(GXFB_ERR, "-EINVAL\n");
		return;
	}
	pri = (struct gxfb_pri *)info->par;
	dev = (struct gxav_priv_dev *)pri->avdev;
	if (!dev)
		return;

	gx_memset(&copyarea, 0x00, sizeof(copyarea));

	copyarea.srca.surface = pri->surface;
	copyarea.srca.rect.x = area->sx;
	copyarea.srca.rect.y = area->sy;
	copyarea.srca.rect.width = area->width;
	copyarea.srca.rect.height = area->height;
	copyarea.srca.alpha_en = 0;

	copyarea.dst.surface = pri->surface;
	copyarea.dst.rect.x = area->dx;
	copyarea.dst.rect.y = area->dy;
	copyarea.dst.rect.width = area->width;
	copyarea.dst.rect.height = area->height;
	copyarea.dst.alpha_en = 0;

	copyarea.srcb = copyarea.dst;

	copyarea.mode = GX_ALU_ROP_COPY;
	copyarea.has_clut_conversion = 0;

	if(gxav_module_set_property(dev, pri->module_id,
				GxVpuPropertyID_Blit, &copyarea,
				sizeof(copyarea)))
		return;

	GXFB_LEAVE();
	return;
}

static void
gxfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	GxVpuProperty_CreateSurface cre_surf = {0,};
	GxVpuProperty_DestroySurface dst_surf = {0,};
	GxVpuProperty_ColorFormat color_fmt = {0};
	GxVpuProperty_Blit copyarea;
	struct gxav_priv_dev *dev = NULL;
	struct gxfb_pri *pri = NULL;

	if (!info || !(info->par) || !image || !(image->data)) {
		GXFB_LOG(GXFB_ERR, "-EINVAL\n");
		return;
	}
	gx_memset(&copyarea, 0x00, sizeof(copyarea));

	pri = (struct gxfb_pri *)info->par;
	dev = (struct gxav_priv_dev *)pri->avdev;
	if (!dev)
		return;

	color_fmt.surface = pri->surface;
	if(gxav_module_get_property(dev, pri->module_id,
				GxVpuPropertyID_ColorFormat, &color_fmt,
				sizeof(color_fmt))) {
		GXFB_LOG(GXFB_ERR, "-ENOMEM\n");
		return;
	}
#if 0
	GXFB_LOG(GXFB_TACE, "image->depth = 0x%x, image->width = 0x%x, \
			image->height = 0x%x\n", image->width, image->height,
			image->depth);
#endif

	cre_surf.width  = image->width;
	cre_surf.height = image->height;
	cre_surf.mode   = GX_SURFACE_MODE_IMAGE;
	cre_surf.format = color_fmt.format;
	cre_surf.buffer_from = 0;
	if(gxav_module_get_property(dev, pri->module_id,
				GxVpuPropertyID_CreateSurface, &cre_surf,
				sizeof(cre_surf))) {
		GXFB_LOG(GXFB_ERR, "-ENOMEM\n");
		return;
	}
	memcpy(cre_surf.buffer, image->data, image->width * image->height
			* image->depth / 8);

	copyarea.srca.surface = cre_surf.surface;
	copyarea.srca.rect.x = 0;
	copyarea.srca.rect.y = 0;
	copyarea.srca.rect.width = image->width;
	copyarea.srca.rect.height = image->height;
	copyarea.srca.alpha_en = 0;

	copyarea.dst.surface = pri->surface;
	copyarea.dst.rect.x = image->dx;
	copyarea.dst.rect.y = image->dy;
	copyarea.dst.rect.width = image->width;
	copyarea.dst.rect.height = image->height;
	copyarea.dst.alpha_en = 0;

	copyarea.srcb = copyarea.dst;

	copyarea.mode = GX_ALU_ROP_COPY;
	copyarea.has_clut_conversion = 0;

	if(gxav_module_set_property(dev, pri->module_id,
				GxVpuPropertyID_Blit, &copyarea,
				sizeof(copyarea)))
		GXFB_LOG(GXFB_ERR, "-GxVpuPropertyID_Blit\n");

	dst_surf.surface = cre_surf.surface;
	if(gxav_module_set_property(dev, pri->module_id,
				GxVpuPropertyID_DestroySurface, &dst_surf,
				sizeof(dst_surf))) {
		GXFB_LOG(GXFB_ERR, "-dst_surf\n");
		return;
	}
	return;
}
#endif

int gxfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct gxfb_pri *pri = NULL;

	GXFB_ENTER();

	if (!var || !info || !info->par)
		return -EFAULT;

	pri = (struct gxfb_pri *)info->par;
	pri->fb_phys_offset = (var->yoffset * info->fix.line_length);

	gxav_vpu_PanDisplay(GX_LAYER_OSD, (void *)(info->fix.smem_start + pri->fb_phys_offset));

	GXFB_LEAVE();
	return 0;
}

static int gxfb_set_par(struct fb_info *info)
{
	GxVpuProperty_ColorFormat color_fmt = {0};
	struct gxfb_pri *pri = NULL;
	struct gxav_property param = {0};
	int fmt = 0;
	int ret = 0;

	GXFB_ENTER();

	if (!info || !info->par)
		return -EFAULT;

	pri = (struct gxfb_pri *)info->par;

	gxfb_av_get_color_fmt(&info->var, &fmt);
	GXFB_LOG(GXFB_TACE, "color_fmt = 0x%x\n", fmt);

	if (fmt != pri->color_fmt) {
		pri->color_fmt = fmt;
		info->fix.line_length = info->var.xres_virtual * info->var.bits_per_pixel >> 3;
		color_fmt.format = pri->color_fmt;
		GXAV_PACKET_PARA(&param, GxVpuPropertyID_ColorFormat, &color_fmt, sizeof(color_fmt));
		gxfb_av_module_property_para_set(pri, &param, &color_fmt);
		GXFB_CHECK_RET(ret = gxav_vpu_SetColorFormat(&color_fmt));
		if (ret) return -EFAULT;
	}

	GXFB_LEAVE();

	return 0;
}

static int gxfb_setcolreg(unsigned color_index, unsigned red, unsigned green,
		unsigned blue, unsigned transp, struct fb_info *info)
{
	GxVpuProperty_SurfaceBindPalette bind_palette = {0};
	GxColor *entries = NULL;
	struct gxfb_pri *pri = NULL;
	int ret = 0;

	if (!info || !info->par)
		return -EFAULT;

	pri = (struct gxfb_pri *)info->par;
	if (!pri->palette)
		return -EFAULT;

	entries = (GxColor *)(((GxPalette *)(pri->palette))->entries);
	if (color_index < GX_FB_MAX_CMAP_LEN) {
		entries[color_index].r = red;
		entries[color_index].g = green;
		entries[color_index].b = blue;
		entries[color_index].a = transp;

		if((GX_FB_MAX_CMAP_LEN - 1) == color_index) {
			bind_palette.palette = pri->palette;
			bind_palette.surface = pri->surface;

			GXFB_CHECK_RET(ret = gxav_vpu_SurfaceBindPalette(&bind_palette));
		}
	} else {
		GXFB_LOG(GXFB_WARN, "color index too big error\n");
		return -EFAULT;
	}
	return 0;
}

static int gxfb_blank(int blank_mode, struct fb_info *info)
{
	struct gxfb_pri *pri = NULL;

	GXFB_ENTER();

	pri = (struct gxfb_pri *)info->par;

	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
	case FB_BLANK_NORMAL:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
		pri->pm_mode = GX_FB_POWERUP;
		break;
	case FB_BLANK_POWERDOWN:
		pri->pm_mode = GX_FB_POWERDOWN;
		break;
	default:
		GXFB_LOG(GXFB_WARN, "default blank mode\n");
		return -EFAULT;
	}

	if (gxfb_enable(info, pri->pm_mode)) {
		GXFB_LOG(GXFB_ERR, "POWERDOWN error\n");
		return -EFAULT;
	}

	GXFB_LEAVE();
	return 0;
}

static void gxfb_release(struct device *dev)
{
	if(gxfb_phys_base)
		gx_page_free((void *)gxfb_phys_base, GX_FB_MIN_BUFFER_SIZE);
	gxav_dev_close(0);
}

static int gxfb_mmap(struct fb_info *info, struct vm_area_struct * vma)
{
	if((NULL == info) || (NULL == vma)){
		return (-ENODEV);
	}

#if 0
	if (gxav_firewall_access_align() == 0) {
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); /* disable cache */
	}
#endif

	if(vma->vm_pgoff != 0) {
		unsigned long *buf = (unsigned long *)(vma->vm_pgoff << PAGE_SHIFT);
		if(remap_pfn_range(vma, vma->vm_start, gx_virt_to_phys((unsigned int)buf) >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot))
			return -EAGAIN;
	} else {
		unsigned long *buf = (unsigned long *)(info->fix.smem_start);
		if(remap_pfn_range(vma, vma->vm_start, gx_virt_to_phys((unsigned int)buf) >> PAGE_SHIFT, info->fix.smem_len, vma->vm_page_prot))
			return -EAGAIN;
	}

	return (0);
}

static void gxfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
	u32 bpl = sizeof(u32), bpp = info->var.bits_per_pixel;
	u32 width = image->width;
	u32 dx = image->dx, dy = image->dy;

	printk("gxfb_imageblit :: bpl %d width %d dx %d dy %d\n",bpl,width,dx,dy);
	
	sys_imageblit(info, image);
}

static struct fb_ops gxfb_ops = {
	.owner          = THIS_MODULE,
	.fb_check_var   = gxfb_check_var,  /*Sanity check*/
	.fb_set_par     = gxfb_set_par,    /*Program controller regiseter*/
	.fb_setcolreg   = gxfb_setcolreg,  /*Set color map*/
	.fb_blank       = gxfb_blank,      /*Blank/unblank display*/
	.fb_imageblit   = NULL,//cfb_imageblit,  /*Generic function to draw*/
	.fb_copyarea    = NULL,//cfb_copyarea,
	.fb_fillrect    = NULL,//cfb_fillrect,
	.fb_ioctl       = gxfb_ioctl,      /*Device specific ioctl*/
	.fb_pan_display = gxfb_pan_display,
	.fb_mmap        = gxfb_mmap,
	.fb_imageblit	= gxfb_imageblit,
};

static struct platform_device gxfb_device = {
	.name = "fb0",
	.id   = 0,
	.dev  = {.platform_data = (void *)&gxfb_ops, },
	.dev.release = gxfb_release,
};

static struct platform_driver gxfb_driver =
{
	.probe  = gxfb_probe,
	.remove = gxfb_remove,
	.driver = {.name = "fb0",},
};

static int __init gxfb_init(void)
{
	printk("init gxfb\n");

	if (gxfb_alloc_screenbase() != 0) {
		return -ENODEV;
	}

	platform_device_register(&gxfb_device);
	return platform_driver_register(&gxfb_driver);
}

static void __exit gxfb_exit(void)
{
	platform_driver_unregister(&gxfb_driver);
	platform_device_unregister(&gxfb_device);
}

MODULE_LICENSE("GPL");
late_initcall(gxfb_init);
module_exit(gxfb_exit);

