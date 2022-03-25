#ifndef __frame_buf_h__
#define __frame_buf_h__

struct frame_buffer;

typedef enum fb_type {
	FB_NORMAL,
	FB_FREEZE,
} FbType;

typedef enum fb_mode {
	FB_FIELD,
	FB_FRAME,
} FbMode;

struct fb_desc {
	FbType type;
	FbMode mode;
	unsigned num;
	struct frame_buffer *fbs;

	unsigned width, height;
	unsigned bpp;
	unsigned align;
	unsigned colbuf_size;
	unsigned userdata_size;

	unsigned buf_addr;
	unsigned buf_size;
};

int fb_alloc(struct fb_desc *desc);
unsigned calc_y_size(struct fb_desc *desc);

#endif

