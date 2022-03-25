#include "frame_buf.h"
#include "mem_manage.h"
#include "kernelcalls.h"

#define SIZE_ALIGN(val, align) ((val+align-1)&~(align-1))

unsigned calc_y_size(struct fb_desc *desc)
{
	unsigned size_8 = 0, size_2 = 0;
	unsigned align_w, align_h;

	align_w = SIZE_ALIGN(desc->width, desc->align);
	if (desc->mode == FB_FIELD)
		align_h = SIZE_ALIGN(desc->height, desc->align)*2;
	else
		align_h = SIZE_ALIGN(desc->height, desc->align);

	size_8 = align_w*align_h;

	if (desc->bpp > 8) {
		if (desc->mode == FB_FIELD)
			size_2 = (align_w*desc->height*2)>>2;
		else
			size_2 = (align_w*desc->height)>>2;
	}

	return size_8 + size_2;
}

static unsigned fb_size(struct fb_desc *desc)
{
	unsigned y = 0, uv = 0;
	unsigned size = 0;

	if (desc) {
		y  = calc_y_size(desc);
		uv = y>>1;
		size += y + uv;
		size += desc->colbuf_size;
		size += desc->userdata_size;
	}

	return size;
}

static int __fb_alloc(struct fb_desc *desc)
{
	int i, alloced_num = 0;
	unsigned y_size, cb_size, cr_size;
	unsigned frame_size = fb_size(desc);

	if (desc) {
		y_size  = calc_y_size(desc);
		cb_size = cr_size = y_size>>2;

		if (desc->num*frame_size <= desc->buf_size) {
			for (i = 0; i < desc->num; i++) {
				/*virt*/
				desc->fbs[i].phys.bufY        = desc->buf_addr;
				desc->fbs[i].phys.bufCb       = desc->fbs[i].phys.bufY  + y_size;
				desc->fbs[i].phys.bufCr       = desc->fbs[i].phys.bufCb + cb_size;
				desc->fbs[i].phys.bufMvCol    = desc->fbs[i].phys.bufCr + cr_size;
				desc->fbs[i].phys.bufUserData = desc->fbs[i].phys.bufMvCol + desc->colbuf_size;
				/*phys*/
				desc->fbs[i].virt.bufY        = (unsigned)gx_phys_to_virt(desc->fbs[i].phys.bufY);
				desc->fbs[i].virt.bufCb       = (unsigned)gx_phys_to_virt(desc->fbs[i].phys.bufCb);
				desc->fbs[i].virt.bufCr       = (unsigned)gx_phys_to_virt(desc->fbs[i].phys.bufCr);
				desc->fbs[i].virt.bufMvCol    = (unsigned)gx_phys_to_virt(desc->fbs[i].phys.bufMvCol);
				desc->fbs[i].virt.bufUserData = (unsigned)gx_phys_to_virt(desc->fbs[i].phys.bufUserData);

				desc->buf_addr += frame_size;
				desc->buf_size -= frame_size;
				alloced_num++;
			}
		}
	}

	return alloced_num;
}

int fb_alloc(struct fb_desc *param)
{
	int ret, cnt;
	int alloced_num = 0;
	BufInfo buf = {0};

	if (param) {
		struct fb_desc desc = *param;
		unsigned frame_size = fb_size(&desc);

		if (desc.buf_addr && desc.buf_size) {
			alloced_num = __fb_alloc(&desc);
		} else {
			unsigned dst_num = desc.num;
			while (alloced_num < dst_num) {
				if (desc.buf_size < frame_size) {
					ret = mm_framebuf_alloc(desc.type, frame_size, 1, &buf);
					if (ret == 0) {
						desc.buf_addr = buf.addr;
						desc.buf_size = buf.size;
						if (buf.id == FREEZE_FB_ID) {
							if (desc.mode == FB_FIELD && buf.size >= frame_size*2)
								desc.buf_size = frame_size*2;
							else
								desc.buf_size = frame_size;
						}
					} else {
						gx_printf("\nalloc fb failed!\n");
						break;
					}
				}

				desc.num = 1;
				desc.fbs = &param->fbs[alloced_num];
				cnt = __fb_alloc(&desc);
				if (cnt > 0)
					alloced_num += cnt;
				else {
					gx_printf("\nalloc fb failed!\n");
					break;
				}
			}
		}
	}

	return alloced_num;
}

