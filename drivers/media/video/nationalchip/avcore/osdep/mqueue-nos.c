#ifdef NO_OS

#include "kernelcalls.h"

void * gx_queue_malloc(unsigned int len)
{
	void *p;

	p=malloc(len);
	return (p);
}

void gx_queue_free(void *ptr, unsigned int  len)
{
	free(ptr);
	ptr=NULL;
}

int gx_queue_create(unsigned int queue_depth, unsigned int data_size)
{
	return 0;
}

int gx_queue_delete (int queue_id)
{
	return 0;
}

int gx_queue_get(int queue_id, char *data, unsigned int size, int timeout)
{
	return size;
}

int gx_queue_put(int queue_id, char *data, unsigned int size)
{
	return 0;
}
#endif

