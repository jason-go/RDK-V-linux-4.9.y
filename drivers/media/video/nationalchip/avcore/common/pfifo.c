#include "kernelcalls.h"
#include "pfifo.h"

unsigned int pfifo_init(struct pfifo *fifo, void *buffer, unsigned int size)
{
	if(size== 0 || fifo == NULL)
		return -1;

	gx_memset(fifo, 0, sizeof(struct pfifo));

	fifo->size = size+1;
	if (buffer == NULL) {
		fifo->data = (unsigned char *)gx_malloc(fifo->size);
		fifo->malloced = 1;
	}
	else
		fifo->data = buffer;
	if (NULL == fifo->data) {
		gx_printf("alloc fifo->buffer NULL!\n");
		return -1;
	}

	gx_memset(fifo->data, 0, fifo->size);

	return 0;
}

void pfifo_free(struct pfifo *pfifo)
{
	if (pfifo->malloced == 1)
		gx_free(pfifo->data);

	memset(pfifo, 0, sizeof(struct pfifo));
}

unsigned int pfifo_put(struct pfifo *pfifo, void *buf, unsigned int len)
{
	unsigned int todo, freelen;
	unsigned int split;

	freelen = pfifo_freelen(pfifo);
	todo = len > freelen ? freelen : len;

	split = (pfifo->pwrite + todo > pfifo->size) ? pfifo->size - pfifo->pwrite : 0;

	if(split > 0) {
		gx_memcpy(pfifo->data+pfifo->pwrite, buf, split);
		buf  += split;
		todo -= split;
		pfifo->pwrite = 0;
	}
	gx_memcpy(pfifo->data + pfifo->pwrite, buf, todo);
	pfifo->pwrite = (pfifo->pwrite + todo) % pfifo->size;

	return (todo + split);
}

unsigned int pfifo_get(struct pfifo *pfifo, void *buf, unsigned int len)
{
	unsigned int todo =len;
	unsigned int split;

	split = (pfifo->pread + len > pfifo->size) ? pfifo->size - pfifo->pread : 0;
	if (split > 0) {
		gx_memcpy(buf, pfifo->data+pfifo->pread, split);
		buf  += split;
		todo -= split;
		pfifo->pread = 0;
	}
	gx_memcpy(buf, pfifo->data+pfifo->pread, todo);

	pfifo->pread = (pfifo->pread + todo) % pfifo->size;

	return len;
}

unsigned int pfifo_peek(struct pfifo *pfifo, void *buf, unsigned int len)
{
	unsigned int todo =len;
	unsigned int split;
	unsigned int pread;

	pread = pfifo->pread;
	split = (pread + len > pfifo->size) ? pfifo->size - pread : 0;
	if (split > 0) {
		gx_memcpy(buf, pfifo->data+pread, split);
		buf  += split;
		todo -= split;
		pread = 0;
	}
	gx_memcpy(buf, pfifo->data+pread, todo);
	return len;
}

void pfifo_flush(struct pfifo *pfifo)
{
	pfifo->pread = pfifo->pwrite;
}

void pfifo_reset(struct pfifo *pfifo)
{
	pfifo->pread = pfifo->pwrite = 0;
}

unsigned int pfifo_len( struct pfifo *pfifo)
{
	return (pfifo->pwrite - pfifo->pread + pfifo->size) % pfifo->size;
}

unsigned int pfifo_is_empty(struct pfifo *pfifo)
{
	return pfifo_len(pfifo) == 0 ? 1 :0;
}

unsigned int pfifo_is_full(struct pfifo *pfifo)
{
	return pfifo_len(pfifo) == (pfifo->size-1) ? 1 : 0;
}

unsigned int pfifo_freelen(struct pfifo *pfifo)
{
	return pfifo->size - pfifo_len(pfifo) - 1;
}

