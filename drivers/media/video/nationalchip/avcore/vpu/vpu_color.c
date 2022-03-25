#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "vpu_color.h"
#include "gxav_vpu_propertytypes.h"

typedef enum {
	GA_CLUT_LEN_CLUT1= 0,
	GA_CLUT_LEN_CLUT2,
	GA_CLUT_LEN_CLUT4,
	GA_CLUT_LEN_CLUT8,
}GaClutType;

static int gxcolor_bpp[] = {
	1,  /* GX_COLOR_FMT_CLUT1           */
	2,  /* GX_COLOR_FMT_CLUT2           */
	4,  /* GX_COLOR_FMT_CLUT4           */
	8,  /* GX_COLOR_FMT_CLUT8           */
	16, /* GX_COLOR_FMT_RGBA4444        */
	16, /* GX_COLOR_FMT_RGBA5551        */
	16, /* GX_COLOR_FMT_RGB565          */
	32, /* GX_COLOR_FMT_RGBA8888        */
	32, /* GX_COLOR_FMT_ABGR8888        */
	24, /* GX_COLOR_FMT_RGB888          */
	24, /* GX_COLOR_FMT_BGR888          */

	16, /* GX_COLOR_FMT_ARGB4444        */
	16, /* GX_COLOR_FMT_ARGB1555        */
	32, /* GX_COLOR_FMT_ARGB8888        */

	16, /* GX_COLOR_FMT_YCBCR422        */
	16, /* GX_COLOR_FMT_YCBCRA6442      */
	12, /* GX_COLOR_FMT_YCBCR420        */

	12, /* GX_COLOR_FMT_YCBCR420_Y_UV   */
	-1, /* GX_COLOR_FMT_YCBCR420_Y_U_V  */
	-1, /* GX_COLOR_FMT_YCBCR420_Y,     */
	-1, /* GX_COLOR_FMT_YCBCR420_U,     */
	-1, /* GX_COLOR_FMT_YCBCR420_V,     */
	-1, /* GX_COLOR_FMT_YCBCR420_UV,    */

	16, /* GX_COLOR_FMT_YCBCR422_Y_UV,  */
	-1, /* GX_COLOR_FMT_YCBCR422_Y_U_V, */
	-1, /* GX_COLOR_FMT_YCBCR422_Y,     */
	-1, /* GX_COLOR_FMT_YCBCR422_U,     */
	-1, /* GX_COLOR_FMT_YCBCR422_V,     */
	-1, /* GX_COLOR_FMT_YCBCR422_UV,    */

	-1, /* GX_COLOR_FMT_YCBCR444,       */
	-1, /* GX_COLOR_FMT_YCBCR444_Y_UV,  */
	24, /* GX_COLOR_FMT_YCBCR444_Y_U_V, */
	-1, /* GX_COLOR_FMT_YCBCR444_Y,     */
	-1, /* GX_COLOR_FMT_YCBCR444_U,     */
	-1, /* GX_COLOR_FMT_YCBCR444_V,     */
	-1, /* GX_COLOR_FMT_YCBCR444_UV,    */

	-1, /* GX_COLOR_FMT_YCBCR400,       */
	8,  /* GX_COLOR_FMT_A8,             */
	16, /* GX_COLOR_FMT_ABGR4444,       */
	16, /* GX_COLOR_FMT_ABGR1555,       */
	8,  /* GX_COLOR_FMT_Y8,             */
	16, /* GX_COLOR_FMT_UV16,           */
	16, /* GX_COLOR_FMT_YCBCR422v,      */
	16, /* GX_COLOR_FMT_YCBCR422h,      */
	32, /* GX_COLOR_FMT_YUVA8888,       */
	32, /*GX_COLOR_FMT_AYUV8888,      */
	32, /*GX_COLOR_FMT_AVUY8888,      */
	16, /*GX_COLOR_FMT_AYCBCR2644*/
	16, /*GX_COLOR_FMT_ACRCBY2446*/
};

unsigned int gx_color_get_value(GxColorFormat format, GxColor *color)
{
	unsigned int val;

	switch(format) {
		case GX_COLOR_FMT_RGBA8888:
			val = ((color->r) << 24) | ((color->g) << 16) | ((color->b) << 8) | ((color->a));
			break;
		case GX_COLOR_FMT_ARGB8888:
			val = ((color->a) << 24) | ((color->r) << 16) | ((color->g) << 8) | ((color->b));
			break;
		case GX_COLOR_FMT_ABGR8888:
			val = ((color->a) << 24) | ((color->b) << 16) | ((color->g) << 8) | ((color->r));
			break;
		case GX_COLOR_FMT_CLUT8:
		case GX_COLOR_FMT_CLUT4:
		case GX_COLOR_FMT_CLUT2:
		case GX_COLOR_FMT_CLUT1:
			val = color->entry;
			break;
		case GX_COLOR_FMT_RGB565:
			val = ((color->r & 0xf8) << 8) | ((color->g & 0xfc) << 3) | ((color->b & 0xf8) >> 3);
			break;
		case GX_COLOR_FMT_RGBA5551:
			val = ((color->r & 0xf8) << 8) | ((color->g & 0xf8) << 3) | ((color->b & 0xf8) >> 2) | ((color->a & 0x80) >> 7);
			break;
		case GX_COLOR_FMT_ARGB1555:
			val = ((color->a & 0x80) << 8) | ((color->r & 0xf8) << 7) | ((color->g & 0xf8) << 2) | ((color->b & 0xf8) >> 3);
			break;
		case GX_COLOR_FMT_ABGR1555:
			val = ((color->a & 0x80) << 8) | ((color->b & 0xf8) << 7) | ((color->g & 0xf8) << 2) | ((color->r & 0xf8) >> 3);
			break;
		case GX_COLOR_FMT_RGBA4444:
			val = ((color->r & 0xf0) << 8) | ((color->g & 0xf0) << 4) | (color->b & 0xf0) | ((color->a & 0xf0) >> 4);
			break;
		case GX_COLOR_FMT_ARGB4444:
			val = ((color->a & 0xf0) << 8) | ((color->r & 0xf0) << 4) | (color->g & 0xf0) | ((color->b & 0xf0) >> 4);
			break;
		case GX_COLOR_FMT_ABGR4444:
			val = ((color->a & 0xf0) << 8) | ((color->b & 0xf0) << 4) | (color->g & 0xf0) | ((color->r & 0xf0) >> 4);
			break;
		case GX_COLOR_FMT_YCBCR422:
			val = (color->cb << 24) | (color->y << 16) | (color->cr << 8) | (color->y);
			break;
		case GX_COLOR_FMT_YCBCRA6442:
			val = (((color->y >> 2) << 10 ) | ((color->cb >> 4) << 6) | ((color->cr >> 4) << 2) | ((color->a & 0xc0) >> 6));
			break;
		case GX_COLOR_FMT_AYCBCR2644:
			val = (((color->a & 0xc0) << 8) | ((color->y >> 2) << 8) | (((color->cb) >> 4) << 4) | ((color->cr) >> 4));
			break;
		case GX_COLOR_FMT_ACRCBY2446:
			val = (((color->a & 0xc0) << 8) | ((color->cr >> 4) << 10) | ((color->cb >> 4) << 6) | (color->y >> 2));
			break;
		case GX_COLOR_FMT_YUVA8888:
			val = ((color->y) << 24) | ((color->cb) << 16) | ((color->cr) << 8) | ((color->a));
			break;
		case GX_COLOR_FMT_AYUV8888:
			val = ((color->a) << 24) | ((color->y) << 16) | ((color->cb) << 8) | ((color->cr));
			break;
		case GX_COLOR_FMT_AVUY8888:
			val = ((color->a) << 24) | ((color->cr) << 16) | ((color->cb) << 8) | ((color->y));
			break;
		default:
			val = -1;
			break;
	}

	return val;
}

unsigned int gx_color_get_clut_length(int num_entries)
{
	switch (num_entries)
	{
		case 2:
			return GA_CLUT_LEN_CLUT1;
		case 4:
			return GA_CLUT_LEN_CLUT2;
		case 16:
			return GA_CLUT_LEN_CLUT4;
		case 256:
			return GA_CLUT_LEN_CLUT8;
		default:
			return GA_CLUT_LEN_CLUT1;
	}
}

int gx_color_get_bpp(GxColorFormat format)
{
	if(format < sizeof(gxcolor_bpp)/sizeof(int))
		return gxcolor_bpp[format];
	else
		return -1;
}

void gx_color_convert(GxPalette *src_palette, unsigned int is_kpalette, GxColorFormat dst_format, GxClutConvertTable* cct)
{
    GxPalette tp;
    unsigned int i, value, *cursor;
    static GxColor entries[256];

    if(src_palette) {
		typedef int ga_copy(void *to, const void *from, unsigned int n);
		ga_copy* copy = NULL;

        if(is_kpalette) {
            copy = (ga_copy*)gx_memcpy;
        }
        else {
            copy = (ga_copy*)gx_copy_from_user;
        }

        copy((void*)&tp, src_palette, sizeof(GxPalette));
        if(tp.num_entries > 2/*256*/) {
		gx_printf("Not support more than 2 colors\n");
		return;
	}
        copy(entries, tp.entries, tp.num_entries * sizeof(GxColor));

        cct->dst_format= dst_format;
        cct->table_len = tp.num_entries;
        cursor = cct->table_addr;
        for(i = 0; i < tp.num_entries; i ++) {
            value = gx_color_get_value(dst_format, &entries[i]);
            value = value << (32 - gx_color_get_bpp(dst_format));
            *cursor= value;
            cursor ++;
        }
    }
}

void gx_color_pixelcolor_read (void *addr, GxColorFormat dst_format, GxColor *color)
{
    return;
}

void gx_color_pixelcolor_write(void *addr, GxColorFormat dst_format, GxColor *color)
{
	int bpp;
	unsigned int color_val;

	GXAV_ASSERT(addr != NULL);
	bpp       = gx_color_get_bpp(dst_format);
	color_val = gx_color_get_value(dst_format, color);

	switch((bpp))
	{
		case 8:
			*(unsigned char*)addr = color_val;
			break;
		case 16:
			if(dst_format==GX_COLOR_FMT_YCBCR422 || dst_format==GX_COLOR_FMT_YCBCRA6442)
				*(unsigned short*)addr = GET_ENDIAN_16(color_val);
			else
				*(unsigned short*)addr = color_val;
			break;
		case 24:
			*((unsigned char*)addr+0) = color->b;
			*((unsigned char*)addr+1) = color->g;
			*((unsigned char*)addr+2) = color->r;
			break;
		case 32:
			*(unsigned int*)addr = color_val;
			break;
		default:
			break;
	}
	/*
	switch(endian)
	{
		case RW_BIG_ENDIAN:
			switch((bpp))
			{
				case 8:
					*(unsigned char*)addr = color_val;
					break;
				case 16:
					*(unsigned short*)addr = GET_ENDIAN_16(color_val);
					break;
				case 24:
					*((unsigned char*)addr+0) = color->r;
					*((unsigned char*)addr+1) = color->g;
					*((unsigned char*)addr+2) = color->b;
					break;
				case 32:
					GX_CMD_SET_VAL_E(addr, color_val);
					break;
				default:
					break;
			}
			break;
		case RW_LITTLE_ENDIAN:
			switch((bpp))
			{
				case 8:
					*(unsigned char*)addr = color_val;
					break;
				case 16:
					*(unsigned short*)addr = color_val;
					break;
				case 24:
					*((unsigned char*)addr+0) = color->b;
					*((unsigned char*)addr+1) = color->g;
					*((unsigned char*)addr+2) = color->r;
					break;
				case 32:
					*(unsigned int*)addr = color_val;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	*/
}

