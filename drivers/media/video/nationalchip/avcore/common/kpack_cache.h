#ifndef __kpack_CACHE_H__
#define __kpack_CACHE_H__

#include "kernelcalls.h"

#define MAX_PACKS (4096)

struct cache_pack {
	unsigned char*     buffer;
	size_t             size;
	unsigned long long pts;
	unsigned int       state;
};

struct kpack_cache {
	gx_mutex_t    mutex;

	int           pos_v;
	int           pos_r;
	int           pos_w;

	int           cached;

	unsigned long max_tick;
	int           max_packs;

	size_t        pack_size;
	size_t        now_packs;

	unsigned int rst_time;
	int rst_size;

	struct cache_pack packs[MAX_PACKS];
};

int kpack_cache_init(struct kpack_cache *cache, size_t pack_size, int max_packs, unsigned long max_tick);

void kpack_cache_uninit(struct kpack_cache *cache);

struct cache_pack *kpack_cache_alloc_pack(struct kpack_cache *cache);

void kpack_cache_free_pack(struct kpack_cache *cache, struct cache_pack *kpack);

struct cache_pack* kpack_cache_get_pack(struct kpack_cache *cache);

int kpack_cache_put_pack(struct kpack_cache *cache, struct cache_pack* kpack);

int kpack_cache_reset(struct kpack_cache *cache);

int kpack_cache_cache_enable(struct kpack_cache *cache, int enable);

#endif
