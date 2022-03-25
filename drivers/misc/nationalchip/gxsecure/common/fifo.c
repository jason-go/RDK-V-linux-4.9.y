#include "kernelcalls.h"
#include "fifo.h"

int gxse_common_fifo_init(GxSeFifo *fifo, void *buffer, unsigned int size)
{
	if (fifo->buffer)
		return 0;

#ifdef CPU_ACPU
	fifo->flags = 0;
	if (NULL == buffer) {
		fifo->buffer = gx_malloc(size);
		fifo->flags |= FIFO_FLAG_MALLOC;
	} else
		fifo->buffer = buffer;
#else
	fifo->buffer = buffer;
#endif

	if (NULL == fifo->buffer) {
		gxlog_e(GXSE_LOG_MOD_HAL, "gxfifo buffer is NULL\n");
		return -1;
	}

	fifo->len = size;
	fifo->in = fifo->out = 0;
	return 0;
}

void gxse_common_fifo_free(GxSeFifo *fifo)
{
#ifdef CPU_ACPU
	if (fifo->flags & FIFO_FLAG_MALLOC)
		gx_free(fifo->buffer);
#endif
	memset(fifo, 0, sizeof(GxSeFifo));
}

unsigned int gxse_common_fifo_len(GxSeFifo *fifo)
{
	if (fifo->buffer)
		return (fifo->len + fifo->in - fifo->out) % fifo->len;
	return 0;
}

unsigned int gxse_common_fifo_freelen(GxSeFifo *fifo)
{
	if (fifo->buffer)
		return fifo->len - gxse_common_fifo_len(fifo) - 1;
	return 0;
}

unsigned int gxse_common_fifo_put(GxSeFifo *fifo, unsigned char *buf, unsigned int len)
{
	unsigned int l;

	if (len > gxse_common_fifo_freelen(fifo))
		return 0;

	len = min(len, fifo->len - gxse_common_fifo_len(fifo));
	l = min(len, fifo->len - fifo->in);
	memcpy(fifo->buffer + fifo->in, buf, l);
	memcpy(fifo->buffer, buf + l, len - l);
	fifo->in = (fifo->in + len) % fifo->len;

	return len;
}

unsigned int gxse_common_fifo_get(GxSeFifo *fifo, unsigned char *buf, unsigned int len)
{
	unsigned int l;

	len = min(len, gxse_common_fifo_len(fifo));
	l = min(len, fifo->len - fifo->out);
	memcpy(buf, fifo->buffer + fifo->out, l);
	memcpy(buf + l, fifo->buffer, len - l);
	fifo->out = (fifo->out + len) % fifo->len;

	return len;
}

#if 0
unsigned int gxse_common_fifo_peek(GxSeFifo *pfifo, void *buf, unsigned int len, unsigned int off)
{
	unsigned int todo, size;
	unsigned int split = 0;
	unsigned int pread;
	unsigned char *pointer = (unsigned char *)buf;

	if (pfifo->data) {
		size = gxse_common_fifo_len(pfifo);
		size = size > off ? size - off : 0;
		todo = len > size ? size : len;

		pread = (pfifo->pread + off) % pfifo->size;
		split = (pread + todo > pfifo->size) ? pfifo->size - pread : 0;
		if (split > 0) {
			if(pfifo->disable_memcpy != 1)
				memcpy(pointer, pfifo->data+pread, split);
			pointer += split;
			todo -= split;
			pread = 0;
		}
		if(pfifo->disable_memcpy != 1)
			memcpy(pointer, pfifo->data+pread, todo);
	} else {
		return -1;
	}

	return (todo + split);
}

void gxse_common_fifo_reset(GxSeFifo *pfifo)
{
	if(pfifo->data) {
		pfifo->pread = pfifo->pwrite = 0;
	}
}

unsigned int gxse_common_fifo_is_empty(GxSeFifo *pfifo)
{
	if(pfifo->data) {
		return gxse_common_fifo_len(pfifo) == 0 ? 1 :0;
	}
	return 1;
}

unsigned int gxse_common_fifo_is_full(GxSeFifo *pfifo)
{
	if(pfifo->data) {
		return gxse_common_fifo_len(pfifo) == (pfifo->size-1) ? 1 : 0;
	}
	return 1;
}

unsigned int gxse_common_fifo_produce(GxSeFifo *pfifo, unsigned int len)
{
	if(pfifo->data) {
		pfifo->pwrite = (pfifo->pwrite + len) % pfifo->size;
		return len;
	}

	return -1;
}

unsigned int gxse_common_fifo_consume(GxSeFifo *pfifo, unsigned int len)
{
	if(pfifo->data) {
		pfifo->pread = (pfifo->pread + len) % pfifo->size;
		return len;
	}

	return -1;
}

void gxse_common_fifo_wptr_set(GxSeFifo *pfifo, unsigned int wptr)
{
	if(pfifo->data) {
		pfifo->pwrite = wptr;
	}
}

void gxse_common_fifo_rptr_set(GxSeFifo *pfifo, unsigned int rptr)
{
	if(pfifo->data) {
		pfifo->pread = rptr;
	}
}

unsigned int gxse_common_fifo_rptr(GxSeFifo *pfifo)
{
	if(pfifo->data) {
		return pfifo->pread;
	}

	return 0;
}

unsigned int gxse_common_fifo_wptr(GxSeFifo *pfifo)
{
	if(pfifo->data) {
		return pfifo->pwrite;
	}

	return 0;
}
#endif
