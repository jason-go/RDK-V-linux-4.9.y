#ifndef GXAV_PLIST_H
#define GXAV_PLIST_H

#include "kernelcalls.h"
#include "pfifo.h"

#define KPAGE_SIZE  (4096)

struct kpage {
	unsigned char *buffer;
	unsigned int  pos_r;
	unsigned int  pos_w;
	unsigned int  used;
};

struct plist {
	struct pfifo used_fifo;
	int max_used_num;

	unsigned int in;
	unsigned int out;
	unsigned int idx;
	unsigned int cap;
	unsigned int page_size;

	struct kpage*  cur_page_w;
	struct kpage** page_alloced;
	struct kpage** page_buffered;
	gx_spin_lock_t spin_lock;
};

unsigned int plist_init    (struct plist *plist, unsigned int size);
unsigned int plist_put     (struct plist *plist, void *buf, unsigned int len, void *(*copy)(void *dest, const void *src, int n));
unsigned int plist_get     (struct plist *plist, void *buf, unsigned int len, void *(*copy)(void *dest, const void *src, int n));
unsigned int plist_peek    (struct plist *plist, void *buf, unsigned int len, unsigned int off);
unsigned int plist_is_full (struct plist *plist);
unsigned int plist_is_empty(struct plist *plist);
unsigned int plist_len     (struct plist *plist);
unsigned int plist_freelen (struct plist *plist);
unsigned int plist_wptr    (struct plist *plist);
unsigned int plist_rptr    (struct plist *plist);
void plist_rptr_set(struct plist *plist, unsigned int rptr);
void plist_wptr_set(struct plist *plist, unsigned int wptr);
void plist_reset (struct plist *plist);
void plist_free (struct plist *plist);

#endif

