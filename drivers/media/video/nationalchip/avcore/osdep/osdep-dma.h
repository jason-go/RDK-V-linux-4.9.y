#ifndef __OSDEP_DMA_H__
#define __OSDEP_DMA_H__

struct gxav_dmainfo {
	unsigned int phys;
	unsigned int virt;
	unsigned int size;
};

extern void gxav_dmainfo_init(void);
extern int gxav_dmainfo_add(unsigned int virt, unsigned int phys, unsigned int size);
extern int gxav_dmainfo_remove(unsigned int virt);
extern struct gxav_dmainfo *gxav_dmainfo_find(unsigned int virt);

#endif
