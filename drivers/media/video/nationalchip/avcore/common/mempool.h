#ifndef __MEMPOOL_H__
#define __MEMPOOL_H__
#include "kernelcalls.h"

typedef int handle_t;

handle_t gxav_mem_pool_init(size_t size, size_t block_size);
void gxav_mem_pool_destroy(handle_t pool);
void *gxav_mem_pool_alloc(handle_t pool);
void *gxav_mem_pool_allocz(handle_t pool);
int gxav_mem_pool_free(handle_t pool, void *p);

#endif /*__MEMPOOL_H__*/

