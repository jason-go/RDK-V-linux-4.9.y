#include "kernelcalls.h"
#include "fifo.h"

int gxfifo_init(struct gxfifo *fifo, void *buffer, unsigned int size)
{
	if (size == 0 || fifo == NULL)
		return -1;

	gx_memset(fifo, 0, sizeof(struct gxfifo));

	if (buffer != NULL || size < 32*1024){
		if (buffer == NULL) {
			buffer = gx_malloc(size);
			if (buffer == NULL)
				return -1;
			fifo->prealloced = 1;
		}
		fifo->data = buffer;
		fifo->size = size;
		gx_memset(fifo->data, 0, fifo->size);
	} else {
		if (plist_init(&fifo->plist, size) != 0)
			return -1;
		fifo->size = size;
	}

	return 0;
}

void gxfifo_free(struct gxfifo *fifo)
{
	if (fifo == NULL) return;

	if (fifo->data) {
		if (fifo->prealloced)
			gx_free(fifo->data);
		gx_memset(fifo, 0, sizeof(struct gxfifo));
	} else {
		plist_free(&fifo->plist);
	}
}

#define _FIFO_COPY(copy, a, b, c) if(copy) copy(a, b, c)
static unsigned int _fifo_put(struct gxfifo *fifo, void *buf, unsigned int len, void *(*copy)(void *, const void *, int))
{
	unsigned int todo, freelen;
	unsigned int split = 0;

	if (fifo->data) {
		freelen = gxfifo_freelen(fifo);
		todo = len > freelen ? freelen : len;

		split = (fifo->pwrite + todo > fifo->size) ? fifo->size - fifo->pwrite : 0;
		if (split > 0) {
			_FIFO_COPY(copy, fifo->data+fifo->pwrite, buf, split);
			buf  += split;
			todo -= split;
			fifo->pwrite = 0;
		}

		_FIFO_COPY(copy, fifo->data + fifo->pwrite, buf, todo);
		fifo->pwrite = (fifo->pwrite + todo) % fifo->size;
	} else {
		todo = plist_put(&fifo->plist, buf, len, copy);
	}

	return (todo + split);
}

static unsigned int _fifo_get(struct gxfifo *fifo, void *buf, unsigned int len, void *(*copy)(void *, const void *, int))
{
	unsigned int todo, size;
	unsigned int split = 0;

	if (fifo->data) {
		size = gxfifo_len(fifo);
		todo = len > size  ? size : len;

		split = (fifo->pread + todo > fifo->size) ? fifo->size - fifo->pread : 0;
		if (split > 0) {
			_FIFO_COPY(copy, buf, fifo->data+fifo->pread, split);
			buf += split;
			todo -= split;
			fifo->pread = 0;
		}

		_FIFO_COPY(copy, buf, fifo->data+fifo->pread, todo);
		fifo->pread = (fifo->pread + todo) % fifo->size;
	} else {
		todo = plist_get(&fifo->plist, buf, len, copy);
	}

	return (todo + split);
}


unsigned int gxfifo_get(struct gxfifo *fifo, void *buf, unsigned int len)
{
	if (fifo == NULL) return 0;

	if (fifo->get_func) {
		return _fifo_get(fifo, buf, len, (void *(*)(void *, const void *, int))(fifo->get_func));
	} else {
		return _fifo_get(fifo, buf, len, (void *(*)(void *, const void *, int))gx_memcpy);
	}
}

unsigned int gxfifo_put(struct gxfifo *fifo, void *buf, unsigned int len)
{
	if (fifo == NULL) return 0;

	if (fifo->put_func) {
		return _fifo_put(fifo, buf, len, (void *(*)(void *, const void *, int))(fifo->put_func));
	} else {
		return _fifo_put(fifo, buf, len, (void *(*)(void *, const void *, int))gx_memcpy);
	}
}

unsigned int gxfifo_user_get(struct gxfifo *fifo, void *buf, unsigned int len)
{
	if (fifo == NULL) return 0;

	return _fifo_get(fifo, buf, len, (void *(*)(void *, const void *, int))gx_copy_to_user);
}

unsigned int gxfifo_user_put(struct gxfifo *fifo, void *buf, unsigned int len)
{
	if (fifo == NULL) return 0;

	return _fifo_put(fifo, buf, len, (void *(*)(void *, const void *, int))gx_copy_from_user);
}

unsigned int gxfifo_shallow_get(struct gxfifo *fifo, void *buf, unsigned int len)
{
	if (fifo == NULL) return 0;

	return _fifo_get(fifo, buf, len, NULL);
}

unsigned int gxfifo_shallow_put(struct gxfifo *fifo, void *buf, unsigned int len)
{
	if (fifo == NULL) return 0;

	return _fifo_put(fifo, buf, len, NULL);
}

unsigned int gxfifo_peek(struct gxfifo *fifo, void *buf, unsigned int len, unsigned int off)
{
	unsigned int todo, size;
	unsigned int split = 0;
	unsigned int pread;

	if (fifo == NULL) return 0;

	if (fifo->data) {
		size = gxfifo_len(fifo);
		size = size > off ? size - off : 0;
		todo = len > size ? size : len;

		pread = (fifo->pread + off) % fifo->size;
		split = (pread + todo > fifo->size) ? fifo->size - pread : 0;
		if (split > 0) {
			gx_memcpy(buf, fifo->data+pread, split);
			buf  += split;
			todo -= split;
			pread = 0;
		}
		gx_memcpy(buf, fifo->data+pread, todo);
	} else {
		todo = plist_peek(&fifo->plist, buf, len, off);
	}

	return (todo + split);
}

void gxfifo_reset(struct gxfifo *fifo)
{
	if (fifo == NULL) return;

	if (fifo->data) {
		fifo->pread = fifo->pwrite = 0;
	} else {
		plist_reset(&fifo->plist);
	}
}

unsigned int gxfifo_len( struct gxfifo *fifo)
{
	if (fifo == NULL) return 0;

	if (fifo->data) {
		return (fifo->pwrite - fifo->pread + fifo->size) % fifo->size;
	} else {
		return plist_len(&fifo->plist);
	}
}

unsigned int gxfifo_is_empty(struct gxfifo *fifo)
{
	if (fifo == NULL) return 0;

	if (fifo->data) {
		return gxfifo_len(fifo) == 0 ? 1 :0;
	} else {
		return plist_is_empty(&fifo->plist);
	}
}

unsigned int gxfifo_is_full(struct gxfifo *fifo)
{
	if (fifo == NULL) return 0;

	if (fifo->data) {
		return gxfifo_len(fifo) == (fifo->size-1) ? 1 : 0;
	} else {
		return plist_is_full(&fifo->plist);
	}
}

unsigned int gxfifo_freelen(struct gxfifo *fifo)
{
	if (fifo == NULL) return 0;

	if (fifo->data) {
		return fifo->size - gxfifo_len(fifo) - 1;
	} else {
		return plist_freelen(&fifo->plist);
	}
}

void gxfifo_wptr_set(struct gxfifo *fifo, unsigned int wptr)
{
	if (fifo == NULL) return;

	if (fifo->data) {
		fifo->pwrite = wptr;
	} else {
		plist_wptr_set(&fifo->plist, wptr);
	}
}

void gxfifo_rptr_set(struct gxfifo *fifo, unsigned int rptr)
{
	if (fifo == NULL) return;

	if (fifo->data) {
		fifo->pread = rptr;
	} else {
		plist_rptr_set(&fifo->plist, rptr);
	}
}

unsigned int gxfifo_rptr(struct gxfifo *fifo)
{
	if (fifo == NULL) return 0;

	if (fifo->data) {
		return fifo->pread;
	} else {
		return plist_rptr(&fifo->plist);
	}

	return 0;
}

unsigned int gxfifo_wptr(struct gxfifo *fifo)
{
	if (fifo == NULL) return 0;

	if (fifo->data) {
		return fifo->pwrite;
	} else {
		return plist_wptr(&fifo->plist);
	}

	return 0;
}


