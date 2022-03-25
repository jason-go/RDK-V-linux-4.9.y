#ifdef ECOS_OS

#include <cyg/kernel/kapi.h>
#include <cyg/kernel/mqueue.hxx>
#include "kernelcalls.h"

extern "C" void * gx_queue_malloc(size_t len)
{
	void *p = NULL;

	p = malloc(len);
	if(p == NULL)
		return NULL;
	return (p);
}

extern "C" void gx_queue_free(void *ptr, size_t len)
{
	free(ptr);
	ptr=NULL;
}

extern "C" int gx_queue_create(unsigned int queue_depth, unsigned int data_size)
{
	Cyg_Mqueue::qerr_t err;

	Cyg_Mqueue *mqueue = new Cyg_Mqueue(queue_depth, data_size, &gx_queue_malloc, &gx_queue_free, &err);

	return (int)mqueue;
}

extern "C" int gx_queue_delete (int queue_id)
{
	Cyg_Mqueue *mqueue = (Cyg_Mqueue*)queue_id;
	//mqueue->~Cyg_Mqueue();
	delete mqueue;

	return 0;
}

extern "C" int gx_queue_get(int queue_id, char *data, unsigned int size, int timeout)
{
	Cyg_Mqueue *mqueue = (Cyg_Mqueue*)queue_id;
	unsigned int prio = 1;
	return mqueue->get(data, (size_t*)&size, &prio, true, timeout + (int)cyg_current_time());
}

extern "C" int gx_queue_put(int queue_id, char *data, unsigned int size)
{
	Cyg_Mqueue *mqueue = (Cyg_Mqueue*)queue_id;

	mqueue->put(data, size, true);

	return 0;
}

#endif
