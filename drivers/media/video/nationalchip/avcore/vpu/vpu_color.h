#ifndef __GXAV_COLOR_H__
#define __GXAV_COLOR_H__

#include "gxav_vpu_propertytypes.h"

/* differences between Palette and GxClutConvertTable as follow:
 * 1) palette is used for on-screen show, the content should be RGBA8888,
 * 2) clut convert table is used for calculate between different colors, the
 * content can be any color format.
 * */
typedef struct {
	GxColorFormat   dst_format;
	unsigned        table_len;
	unsigned int    *table_addr;
}GxClutConvertTable;
extern void gx_color_convert(GxPalette *src_palette, unsigned int is_kpalette, GxColorFormat dst_format, GxClutConvertTable* cct);

extern unsigned int gx_color_get_value(GxColorFormat format, GxColor * color);
extern unsigned int gx_color_get_clut_length(int num_entries);
extern int gx_color_get_bpp(GxColorFormat format);

extern void gx_color_pixelcolor_read (void *addr, GxColorFormat dst_format, GxColor *color);
extern void gx_color_pixelcolor_write(void *addr, GxColorFormat dst_format, GxColor *color);
#endif
