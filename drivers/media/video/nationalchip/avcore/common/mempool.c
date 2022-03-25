#include "mempool.h"

struct memnode {
	struct memnode *next;
};

struct mempool {
	size_t            block_size;
	size_t            block_number;
	struct memnode*   chunk;
	int               count;
	unsigned int      time_out;
	gx_mutex_t        mutex;

	char *            begin_address;
	char *            end_address;
};

#define GXAV_POOL_DEBUG            (1)
#if GXAV_POOL_DEBUG
#define GXAV_OOB                   (4)
#else /*GXAV_POOL_DEBUG*/
#define GXAV_OOB                   (0)
#endif /*GXAV_POOL_DEBUG*/

#define GXAV_POOL_LOCK(_pool)          gx_mutex_lock(&(_pool->mutex))
#define GXAV_POOL_UNLOCK(_pool)        gx_mutex_unlock(&(_pool->mutex))

static int _mem_pool_create(struct mempool *pool, void *buffer, size_t block_size, size_t block_number)
{
	int ret  = 0;
	uint8_t *p = NULL;
	int i = 0, y = 0, mem_size = block_size * block_number;
	struct memnode *node;

	gx_mutex_init(&(pool->mutex));

	pool->block_number  = pool->count = block_number;
	pool->block_size    = block_size;
	pool->begin_address = (char *)buffer;
	pool->end_address   = pool->begin_address + mem_size;
	pool->chunk         = (struct memnode*)pool->begin_address;

	node = pool->chunk;

	for (i=0; i<block_number - 1; i++) {
		p = (uint8_t*)node + pool->block_size - GXAV_OOB;
		for (y = 0; y < GXAV_OOB; y++) {
			p[y] = 0xA0;
		}
		node->next = (struct memnode*)((char*)node + block_size);
		node = node->next;
	}
	node->next = NULL;
	p = (uint8_t*)node + pool->block_size - GXAV_OOB;
	for (y = 0; y < GXAV_OOB; y++) {
		p[y] = 0xA0;
	}

	return (ret);
}

handle_t gxav_mem_pool_init(size_t size, size_t block_number)
{
	struct mempool *pool = NULL;

	if((0 == size) || (0 == block_number)) {
		return (-1);
	}

	pool = (struct mempool *)gx_mallocz(sizeof(struct mempool) + (size + GXAV_OOB) * block_number);
	if(NULL == pool) {
		return (-1);
	}

	if(_mem_pool_create(pool, (char *)pool + sizeof(struct mempool), (size + GXAV_OOB), block_number)) {
		gx_free(pool);
		pool = NULL;
	}

	return ((handle_t)(pool));
}

void gxav_mem_pool_destroy(handle_t pool)
{
	struct mempool *_pool = (struct mempool *)pool;

	gx_mutex_destroy(&(_pool->mutex));

	gx_free(_pool);
}

void *gxav_mem_pool_alloc(handle_t pool)
{
	uint8_t *p = NULL;
	struct mempool *_pool = (struct mempool *)pool;

	if(NULL == _pool) {
		return (NULL);
	}

	GXAV_POOL_LOCK(_pool);

	if (_pool->chunk != NULL) {
		struct memnode *node = _pool->chunk;
		int i = 0;

		p = (uint8_t*)node + _pool->block_size - GXAV_OOB;
		for (i = 0; i < GXAV_OOB; i++) {
			if( p[i] != 0xA0) {
				while(1) {
					gx_printf("[AV POOL]~mempoll have alloced~%s %d~\n",__FILE__,__LINE__);
				}
			}
		}

		_pool->chunk = node->next;
		_pool->count--;
		p = (uint8_t*)node + _pool->block_size - GXAV_OOB;
		for (i = 0; i < GXAV_OOB; i++) {
			p[i] = 0xB0;
		}

		if ((void *)node < (void *)_pool->begin_address && (void *)node > (void *)_pool->end_address) {
			gx_printf("[AV POOL] MemPool error: node=0x%p\n", node);
			return (NULL);
		}

		GXAV_POOL_UNLOCK(_pool);

		return (void*)node;
	}

	GXAV_POOL_UNLOCK(_pool);

	return ((void *)p);
}


void *gxav_mem_pool_allocz(handle_t pool)
{
	struct mempool *_pool = (struct mempool *)pool;
	void *p = NULL;

	p = gxav_mem_pool_alloc(pool);
	if(p) {
		memset(p, 0, _pool->block_size - GXAV_OOB);
	}

	return (p);
}

int gxav_mem_pool_free(handle_t pool, void *p)
{
	struct mempool *_pool = (struct mempool *)pool;

	if((NULL == _pool) || (NULL == p)) {
		return (-1);
	}

	if(((char *)p >= _pool->begin_address) &&
	   ((char *)p <= _pool->end_address)) {
		int i = 0;
		uint8_t *ptr =  (uint8_t*)p + _pool->block_size - GXAV_OOB;

		for (i = 0; i < GXAV_OOB; i++) {
			if (ptr[i] != 0xB0) {
				gx_printf("[AV POOL] overflow memory.\n");
				while(1);
			}

			struct memnode *node = (struct memnode*)p;

			GXAV_POOL_LOCK(_pool);
			node->next = _pool->chunk;
			_pool->chunk = node;
			_pool->count++;

			ptr = (uint8_t*)node + _pool->block_size - GXAV_OOB;
			for (i = 0; i < GXAV_OOB; i++) {
				ptr[i] = 0xA0;
			}
			GXAV_POOL_UNLOCK(_pool);
			return (0);
		}
	}

	return (-1);
}

