#ifndef _GX_FB_H_
#define _GX_FB_H_

#include <asm/types.h>

/* GXFB debug */
#define GXFB_ANOUCE    (0)
#define GXFB_ERR    (1)
#define GXFB_WARN    (2)
#define GXFB_TACE    (8)
#define GXFB_DBG    (9)

#define GXFB_DBG_LEVEL GXFB_WARN
#ifdef GXFB_DBG_LEVEL
#define GXFB_LOG(l, fmt, args...)                         \
	do {                                     \
		if (l <= GXFB_DBG_LEVEL)                     \
		printk(("[%s] %s():%d: "fmt),                 \
#l, __FUNCTION__, __LINE__, ##args); \
	}while(0)
#else
#define GXFB_LOG(l, fmt, args...)        \
	do {                    \
		if (l <= GXFB_WARN)        \
		printk((fmt), ##args);    \
	}while(0)
#endif
#define GXFB_ENTER(...)        GXFB_LOG(GXFB_DBG, " START \n")
#define GXFB_LEAVE(...)        GXFB_LOG(GXFB_DBG, " END \n")
#define GXFB_CHECK_RET(p)    \
	do {            \
		if (!!(p))    \
		GXFB_LOG(GXFB_ERR, " <<< ERR >>> \n"); \
	} while (0)

//#define GX_FB_GET_PAGES

#define GX_FB_LAYER GX_LAYER_OSD
#define GX_NFB_LAYER (GX_FB_LAYER + 1)

#define GX_FB_MAX_XRES (1280)
#define GX_FB_MAX_YRES (720)

#define GX_FB_MAX_XRES_VIR GX_FB_MAX_XRES
#define GX_FB_MAX_YRES_VIR GX_FB_MAX_YRES

#define GX_FB_MAX_BIT_PER_PIXEL (32)

#define GX_FB_MAX_CMAP_LEN (256)

#define GX_FB_MIN_BUFFER_SIZE (0xF00000)

enum {
	GX_FB_POWERDOWN = 0,
	GX_FB_POWERUP   = 1,
};

struct gxfb_pri {
	/* for av framework */
	void   *avdev;
	int    module_id;
	int    vout_id;
	void   *surface;
	void   *palette;
	int    resource_id;

	int    color_fmt;
	int    pm_mode;
	unsigned long fb_phys_base;
	unsigned long fb_phys_offset;

	void    *pseudo_palette;

	void    *vpu_iom;
	void    *ga_iom;
	unsigned char *vout_tmp_buf;
};

/* GA defined */
enum {
	GX_FB_ACCEL_GX3211 = 0x100,
};
#define GX_FB_ACCEL_NATIONALCHIP GX_FB_ACCEL_GX3211

/* ioctls */
#define GXFB_IOCTL_PROPERTY_SET        _IOW('G', 0x00, struct gxfb_pri)
#define GXFB_IOCTL_PROPERTY_GET        _IOR('G', 0x01, struct gxfb_pri)

#if 0
#define GX_FB_ACCEL_CMD_BUFFER_SIZE (0x200)
#define GX_FB_ACCEL_CMD_ADDR_OFFSET (GX_FB_MAX_BUFFER_SIZE - GX_FB_ACCEL_CMD_BUFFER_SIZE -1)
#endif

/* depend on av.ko module */
#define GX_FB_TO_AVDEV_ID_DEF (0)

#endif

