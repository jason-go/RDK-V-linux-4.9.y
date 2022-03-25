#include "kernelcalls.h"
#include "mpool.h"

#define CHECK_ALIGN(num, times)     num % times ? ((num / times) + 1) * times : num

struct mnode {
	unsigned used;
	unsigned size;
	void    *ptr;
};

struct mpool {
	unsigned     blk_num;
	struct mnode *blks;
};

void* mpool_create(unsigned blk_size, unsigned blk_num)
{
	int i;
	struct mpool *pool = NULL;

	if (blk_size && blk_num) {
		pool = (struct mpool*)gx_malloc(sizeof(struct mpool));
		if (pool) {
			pool->blk_num = blk_num;
			pool->blks    = (struct mnode*)gx_malloc(sizeof(struct mnode)*blk_num);
			if (pool->blks) {
				gx_memset(pool->blks, 0, sizeof(struct mnode)*blk_num);

				blk_size = CHECK_ALIGN(blk_size, 8);
				void *blkhdr = gx_dma_malloc(blk_size * blk_num);
				for (i = 0; i < blk_num; i++) {
					pool->blks[i].ptr = blkhdr + blk_size * i;
					pool->blks[i].size = blk_size;
				}
			}
		}
	}

	return pool;
}

void  mpool_destroy(void *mp)
{
	int i;
	struct mpool *pool = (struct mpool*)mp;

	if (pool) {
		if (pool->blks) {
			if (pool->blks)
				gx_dma_free(pool->blks, pool->blk_num * pool->blks->size);
			gx_free(pool->blks);
		}
		gx_free(pool);
	}
}

void* mpool_alloc(void *mp, unsigned blk_size)
{
	int i;
	void *ptr = NULL;
	struct mpool *pool = (struct mpool*)mp;

	if (pool) {
		if (pool->blks) {
			for(i = 0; i < pool->blk_num; i++) {
				if (pool->blks[i].size >= blk_size && pool->blks[i].used == 0) {
					pool->blks[i].used = 1;
					ptr = pool->blks[i].ptr;
					break;
				}
			}
		}
	}

	return ptr;
}

void  mpool_free(void *mp, void *p)
{
	int i;
	struct mpool *pool = (struct mpool*)mp;

	if (pool) {
		if (pool->blks) {
			for(i = 0; i < pool->blk_num; i++) {
				if (pool->blks[i].ptr == p) {
					pool->blks[i].used = 0;
					break;
				}
			}
		}
	}
}

void mpool_print(void *mp)
{
	int i;
	struct mpool *pool = (struct mpool*)mp;

	if (pool) {
		if (pool->blks) {
			for(i = 0; i < pool->blk_num; i++) {
				struct mnode *node = NULL;
				node = &pool->blks[i];
				gx_printf("\t[%d] used = %d, size = %d, ptr = 0x%x\n", i, node->used, node->size, (unsigned)node->ptr);
			}
		}
	}
}

