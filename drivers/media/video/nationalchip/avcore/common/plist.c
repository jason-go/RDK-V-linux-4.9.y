#include "gxav_common.h"
#include "plist.h"

#if 0
#define PLIST_DBG(fmt, args...)   do{\
	gx_printf("\n[PLIST][%s():%d]: ", __func__, __LINE__);\
	gx_printf(fmt, ##args);\
}while(0)
#else
#define PLIST_DBG(fmt, args...)   ((void)0)
#endif

#ifdef CONFIG_SMP
#define PLIST_LOCK_INIT(fifo)   gx_spin_lock_init(&fifo->spin_lock);
#define PLIST_LOCK(fifo)        gx_spin_lock_irqsave(&fifo->spin_lock, flags);
#define PLIST_UNLOCK(fifo)      gx_spin_unlock_irqrestore(&fifo->spin_lock, flags);
#define PLIST_LOCK_UNINIT(fifo) (void)0
#else
#define PLIST_LOCK_INIT(fifo)   (void)0
#define PLIST_LOCK(fifo)        (void)flags
#define PLIST_UNLOCK(fifo)      (void)flags
#define PLIST_LOCK_UNINIT(fifo) (void)0
#endif

static struct kpage* plist_alloc_page(struct plist *plist)
{
	struct kpage* page = plist->page_alloced[plist->idx];
	if(page){
		page->buffer = (unsigned char*)page + sizeof(struct kpage);
		if(page->used == 1) {
			PLIST_DBG("try alloc a used page\n");
			return NULL;
		}
		plist->idx++;
		plist->idx %= plist->max_used_num;
		page->pos_w = 0;
		page->used = 1;
	}

	return page;
}

static void plist_free_page(struct plist *plist, struct kpage *page)
{
	page->pos_r = 0;
	page->used = 0;
}

unsigned int plist_init(struct plist *plist, unsigned int size)
{
	int i;
	int align = 188*4;

	memset(plist, 0, sizeof(struct plist));

	plist->cap = size;
	plist->page_size = (KPAGE_SIZE - sizeof(struct kpage))/align*align;
	plist->max_used_num = size/plist->page_size + 2;
	plist->page_buffered = gx_malloc(sizeof(int*) * plist->max_used_num);
	plist->page_alloced = gx_malloc(sizeof(int*) * plist->max_used_num);

	pfifo_init(&plist->used_fifo, NULL, sizeof(int*) * plist->max_used_num);

	for(i=0;i<plist->max_used_num;i++) {
		plist->page_alloced[i] = (struct kpage*)gx_mallocz(KPAGE_SIZE);
	}

	plist->cur_page_w = NULL;

	PLIST_LOCK_INIT(plist);
	return 0;
}

void plist_free(struct plist *plist)
{
	int i;
	unsigned long flags;

	PLIST_LOCK(plist);
	if(plist->cap == 0) {
		gx_printf("%s: [error] plist has been free !!!\n", __func__);
		PLIST_UNLOCK(plist);
		return;
	}

	for(i=0;i<plist->max_used_num;i++) {
		gx_free((void*)plist->page_alloced[i]);
	}

	if(plist->page_buffered)
		gx_free(plist->page_buffered);
	if(plist->page_alloced)
		gx_free(plist->page_alloced);

	pfifo_free(&plist->used_fifo);

	plist->max_used_num = 0;
	plist->cap = 0;
	PLIST_UNLOCK(plist);
}

unsigned int plist_put(struct plist *plist, void *buf, unsigned int len, void *(*copy)(void *dest, const void *src, int n))
{
	int copied;
	int data_pos = 0;
	unsigned int left_size;
	struct kpage *page = plist->cur_page_w;
	unsigned long flags;

	PLIST_LOCK(plist);
	if(plist->cap == 0) {
		gx_printf("%s: [error] plist has been free !!!\n", __func__);
		goto end;
	}

	while (len > 0) {
		if (page == NULL) {
			plist->cur_page_w = page = plist_alloc_page(plist);
			if(page == NULL) {
				PLIST_DBG("plist Full !!!\n");
				goto end;
			}
			pfifo_put(&plist->used_fifo, &page, sizeof(page));
		}

		left_size = plist->page_size - page->pos_w;
		copied = GX_MIN(len, left_size);
		if(copied <= 0){
			PLIST_DBG("page full !!!\n");
			goto end;
		}

		if (copy) {
			PLIST_UNLOCK(plist);
			copy(page->buffer + page->pos_w, buf + data_pos, copied);
			PLIST_LOCK(plist);
		}

		page->pos_w += copied;
		data_pos += copied;
		len -= copied;

		if(page->pos_w == plist->page_size){
			plist->cur_page_w = page = NULL;
		}
	}

end:
	plist->in += data_pos;
	PLIST_UNLOCK(plist);
	return data_pos;
}

unsigned int plist_peek(struct plist *plist, void *buf, unsigned int len, unsigned int off)
{
	int copied, pages, rptr = 0;
	int i = 0, data_pos = 0;
	unsigned int left_size;
	struct kpage *page;
	unsigned long flags;

	PLIST_LOCK(plist);

	if(plist->cap == 0) {
		gx_printf("%s: [error] plist has been free !!!\n", __func__);
		goto end;
	}

	pages = pfifo_peek(&plist->used_fifo, plist->page_buffered, pfifo_len(&plist->used_fifo)) /sizeof(page);
	if (pages <= 0){
		PLIST_DBG("plist empty !!!\n");
		goto end;
	}

	for (i=0; i<pages; i++){
		page = plist->page_buffered[i];
		left_size = page->pos_w - page->pos_r;
		if (off >= left_size) {
			off -= left_size;
			continue;
		}
		else {
			rptr = off;
			left_size -= off;
			off = 0;
		}
		copied = GX_MIN(len, left_size);
		if(copied <= 0){
			PLIST_DBG("page empty!!!\n");
			goto end;
		}

		gx_memcpy(buf + data_pos, page->buffer + page->pos_r + rptr, copied);
		data_pos += copied;
		len -= copied;

		if(len == 0){
			goto end;
		}
	}

end:
	PLIST_UNLOCK(plist);
	return data_pos;
}

unsigned int plist_get(struct plist *plist, void *buf, unsigned int len, void *(*copy)(void *dest, const void *src, int n))
{
	int copied, pages;
	int i = 0, data_pos = 0;
	unsigned int left_size;
	struct kpage *page;
	unsigned long flags;

	PLIST_LOCK(plist);

	if(plist->cap == 0) {
		gx_printf("%s: [error] plist has been free !!!\n", __func__);
		goto end;
	}

	pages = pfifo_peek(&plist->used_fifo, plist->page_buffered, pfifo_len(&plist->used_fifo)) /sizeof(page);
	if(pages <= 0){
		PLIST_DBG("plist empty !!!\n");
		goto end;
	}

	for(i=0; i<pages; i++){
		page = plist->page_buffered[i];
		left_size = page->pos_w - page->pos_r;
		copied = GX_MIN(len, left_size);
		if(copied <= 0){
			PLIST_DBG("page empty!!!\n");
			goto end;
		}

		if (copy) {
			PLIST_UNLOCK(plist);
			copy(buf + data_pos, page->buffer + page->pos_r, copied);
			PLIST_LOCK(plist);
		}

		data_pos += copied;
		len -= copied;

		page->pos_r += copied;
		if(page->pos_r == plist->page_size){
			pfifo_get(&plist->used_fifo, &page, sizeof(page));
			plist_free_page(plist, page);
		}

		if(len == 0){
			goto end;
		}
	}

end:
	plist->out += data_pos;
	PLIST_UNLOCK(plist);
	return data_pos;
}

unsigned int plist_len(struct plist *plist)
{
	return plist->in - plist->out;
}

void plist_reset(struct plist *plist)
{
	int i;
	unsigned long flags;

	PLIST_LOCK(plist);
	pfifo_reset(&plist->used_fifo);

	plist->in  = 0;
	plist->out = 0;
	plist->idx = 0;
	plist->cur_page_w = NULL;

	for(i=0;i<plist->max_used_num;i++) {
		plist->page_alloced[i]->used = 0;
		plist->page_alloced[i]->pos_r = 0;
		plist->page_alloced[i]->pos_w = 0;
	}
	PLIST_UNLOCK(plist);
}

unsigned int plist_is_full(struct plist *plist)
{
	return (plist_freelen(plist) == 0 ?  1 : 0);
}

unsigned int plist_is_empty(struct plist *plist)
{
	return (plist_len(plist) == 0 ?  1 : 0);
}

unsigned int plist_freelen(struct plist *plist)
{
	return plist->cap - plist_len(plist);
}

unsigned int plist_wptr(struct plist *plist)
{
	return plist->in;
}

unsigned int plist_rptr(struct plist *plist)
{
	return plist->out;
}

void plist_rptr_set(struct plist *plist, unsigned int rptr)
{
	plist->out = rptr;
}

void plist_wptr_set(struct plist *plist, unsigned int wptr)
{
	plist->in = wptr;
}
