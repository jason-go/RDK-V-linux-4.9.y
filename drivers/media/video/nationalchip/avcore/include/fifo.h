#ifndef GXAV_FIFO_H
#define GXAV_FIFO_H

#include "plist.h"

struct gxfifo {
	unsigned char *data;
	unsigned int  size;
	unsigned int  pread;
	unsigned int  pwrite;
	unsigned int  disable_memcpy;
	unsigned int  prealloced;
	int (*put_func)(void *dest, const void *src, int n);
	int (*get_func)(void *dest, const void *src, int n);

	gx_spin_lock_t spin_lock;
	struct plist plist;
};

int gxfifo_init (struct gxfifo *fifo, void *buffer, unsigned int size);
unsigned int gxfifo_put     (struct gxfifo *fifo, void *buf, unsigned int len);
unsigned int gxfifo_get     (struct gxfifo *fifo, void *buf, unsigned int len);
unsigned int gxfifo_user_get(struct gxfifo *fifo, void *buf, unsigned int len);
unsigned int gxfifo_user_put(struct gxfifo *fifo, void *buf, unsigned int len);
unsigned int gxfifo_shallow_get(struct gxfifo *fifo, void *buf, unsigned int len);
unsigned int gxfifo_shallow_put(struct gxfifo *fifo, void *buf, unsigned int len);
unsigned int gxfifo_peek    (struct gxfifo *fifo, void *buf, unsigned int len, unsigned int off);
unsigned int gxfifo_len     (struct gxfifo *fifo);
unsigned int gxfifo_is_full (struct gxfifo *fifo);
unsigned int gxfifo_is_empty(struct gxfifo *fifo);
unsigned int gxfifo_freelen (struct gxfifo *fifo);
void gxfifo_free  (struct gxfifo *fifo);
void gxfifo_reset (struct gxfifo *fifo);
void gxfifo_flush (struct gxfifo *fifo);
unsigned int gxfifo_rptr(struct gxfifo *pfifo);
unsigned int gxfifo_wptr(struct gxfifo *pfifo);
void gxfifo_wptr_set(struct gxfifo *pfifo, unsigned int wptr);
void gxfifo_rptr_set(struct gxfifo *pfifo, unsigned int rptr);


static inline void gxfifo_set_put_func(struct gxfifo *fifo, int (*func)(void *dest, const void *src, int n)) {
	if (fifo) {
		fifo->put_func = func;
	}
}

static inline void gxfifo_set_get_func(struct gxfifo *fifo, int (*func)(void *dest, const void *src, int n)) {
	if (fifo) {
		fifo->get_func = func;
	}
}

#endif
