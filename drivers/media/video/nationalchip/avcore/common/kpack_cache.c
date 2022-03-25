
#include "kpack_cache.h"

#if 0
#define KPACK_CACHE_LOCK(c)        gx_mutex_lock(&c->mutex)
#define KPACK_CACHE_UNLOCK(c)      gx_mutex_unlock(&c->mutex)
#define KPACK_CACHE_LOCK_INIT(c)   gx_mutex_init(&c->mutex)
#define KPACK_CACHE_LOCK_UNINIT(c) gx_mutex_destroy(&c->mutex)
#else
#define KPACK_CACHE_LOCK(c)         (void)0
#define KPACK_CACHE_UNLOCK(c)       (void)0
#define KPACK_CACHE_LOCK_INIT(c)    (void)0
#define KPACK_CACHE_LOCK_UNINIT(c)  (void)0
#endif

int kpack_cache_init(struct kpack_cache *cache, size_t pack_size, int max_packs, unsigned long max_tick)
{
	int i;
	memset(cache, 0, sizeof(struct kpack_cache));

	if (max_packs > MAX_PACKS) {
		gx_printf("max_packs error!");
		goto errout;
	}

	for (i=0; i<max_packs; i++) {
		cache->packs[i].buffer = gx_malloc(pack_size);
		if (cache->packs[i].buffer == NULL) {
			gx_printf("oom error!");
			goto errout;
		}
		cache->packs[i].size = pack_size;
	}

	cache->pack_size = pack_size;
	cache->max_packs = max_packs;
	cache->max_tick  = max_tick;
	cache->cached    = 0;

	KPACK_CACHE_LOCK_INIT(cache);
	return 0;

errout:
	kpack_cache_uninit(cache);
	return -1;
}

void kpack_cache_uninit(struct kpack_cache *cache)
{
	int i;

	KPACK_CACHE_LOCK(cache);
	for (i=0; i<cache->max_packs; i++) {
		if (cache->packs[i].buffer != NULL) {
			gx_free(cache->packs[i].buffer);
			cache->packs[i].buffer = NULL;
		}
	}
	KPACK_CACHE_UNLOCK(cache);

	KPACK_CACHE_LOCK_UNINIT(cache);
}

#if defined ECOS_OS || defined UCOS_OS
#define KPACK_TICK_AFTER(s, e, t)   ((long long)((e)-(s)) > (long long)(t))
#define KPACK_TICK_BEFORE(s, e, t)  ((long long)((e)-(s)) < (long long)(t))
#else
#define KPACK_TICK_AFTER(s, e, t)   ((long)((e)-(s)) > (long)(t))
#define KPACK_TICK_BEFORE(s, e, t)  ((long)((e)-(s)) < (long)(t))
#endif

static void kpack_cache_update(struct kpack_cache *cache)
{
	if (cache->pos_v != cache->pos_r) {
		int curw = cache->pos_w == 0 ? cache->max_packs - 1 :  cache->pos_w - 1;

		if (cache->packs[cache->pos_v].state == 2 && KPACK_TICK_AFTER(cache->packs[cache->pos_v].pts, cache->packs[curw].pts, cache->max_tick)) {
			cache->packs[cache->pos_v].state = 3;
			cache->pos_v = (cache->pos_v + 1) % cache->max_packs;
		}
	}
}

struct cache_pack *kpack_cache_alloc_pack(struct kpack_cache *cache)
{
	struct cache_pack* kpack = NULL;

	KPACK_CACHE_LOCK(cache);
	kpack_cache_update(cache);
	if (cache->packs[cache->pos_w].state == 0 || cache->packs[cache->pos_w].state == 3){
		kpack = &cache->packs[cache->pos_w];
		cache->pos_w = (cache->pos_w + 1) % cache->max_packs;
		kpack->state = 1;
		kpack->size = cache->pack_size;
		//gx_printf("%s, pos_r w v = %d %d %d\n", __func__, cache->pos_r, cache->pos_w, cache->pos_v);
	}
	KPACK_CACHE_UNLOCK(cache);

	return kpack;
}

void kpack_cache_free_pack(struct kpack_cache *cache, struct cache_pack *kpack)
{
	KPACK_CACHE_LOCK(cache);
	kpack_cache_update(cache);
	KPACK_CACHE_UNLOCK(cache);
}

static int kpack_cache_pack_ready(struct kpack_cache *cache)
{
	if (cache->cached == 1) {
		int curw = cache->pos_w == 0 ? cache->max_packs - 1 :  cache->pos_w - 1;

		if (cache->packs[cache->pos_r].state != 2)
			return 0;
		if (KPACK_TICK_BEFORE(cache->packs[cache->pos_r].pts, cache->packs[curw].pts, cache->max_tick))
			return 0;
	}

	return 1;
}

struct cache_pack* kpack_cache_get_pack(struct kpack_cache *cache)
{
	struct cache_pack* kpack = NULL;

	if (kpack_cache_pack_ready(cache) == 0)
		return NULL;

	if (cache->packs[cache->pos_r].state == 2) {
		kpack = &cache->packs[cache->pos_r];
		cache->pos_r = (cache->pos_r + 1) % cache->max_packs;
		cache->now_packs --;
		//gx_printf("%s, pos_r w v = %d %d %d\n", __func__, cache->pos_r, cache->pos_w, cache->pos_v);
	}

	return kpack;
}

int kpack_cache_put_pack(struct kpack_cache *cache, struct cache_pack* kpack)
{
	KPACK_CACHE_LOCK(cache);
	kpack->state = 2;
	cache->now_packs ++;
	kpack_cache_update(cache);
	KPACK_CACHE_UNLOCK(cache);

	return 0;
}

int kpack_cache_reset(struct kpack_cache *cache)
{
	int i;

	KPACK_CACHE_LOCK(cache);
	//gx_printf("##########################################################\n");
	//gx_printf("%s, w:%d, r:%d, n:%d, v=%d", __func__, cache->pos_w, cache->pos_r, cache->now_packs, cache->pos_v);
	cache->pos_r = cache->pos_v;
	cache->rst_size = 0;
	if (cache->pos_w >= cache->pos_r) {
		for (i=cache->pos_r; i<cache->pos_w; i++) {
			//gx_printf("state[%d]=%d, s=%d", i, cache->packs[i].state, cache->packs[i].size);
			if (cache->packs[i].state == 3)
				cache->packs[i].state = 2;
			if (cache->packs[i].state == 2)
				cache->rst_size += cache->packs[i].size;
		}
	}
	else {
		for (i=0; i<cache->pos_w; i++) {
			//gx_printf("state[%d]=%d, s=%d", i, cache->packs[i].state, cache->packs[i].size);
			if (cache->packs[i].state == 3)
				cache->packs[i].state = 2;
			if (cache->packs[i].state == 2)
				cache->rst_size += cache->packs[i].size;
		}
		for (i=cache->pos_r; i<cache->max_packs; i++) {
			//gx_printf("state[%d]=%d, s=%d", i, cache->packs[i].state, cache->packs[i].size);
			if (cache->packs[i].state == 3)
				cache->packs[i].state = 2;
			if (cache->packs[i].state == 2)
				cache->rst_size += cache->packs[i].size;
		}
	}

	cache->now_packs = cache->pos_w > cache->pos_r ? cache->pos_w - cache->pos_r :
		cache->pos_w - cache->pos_r + cache->max_packs;

	//gx_printf("##########################################################\n");
	//gx_printf("%s, pksize:%d, pknb:%d, tick:%d0\n", __func__, cache->pack_size, cache->max_packs, cache->max_tick);
	//gx_printf("%s, w:%d, r:%d, n:%d, s=%d\n", __func__, cache->pos_w, cache->pos_r, cache->now_packs, cache->rst_size);
	//gx_printf("##########################################################\n");
	KPACK_CACHE_UNLOCK(cache);

	return 0;
}

int kpack_cache_cache_enable(struct kpack_cache *cache, int enable)
{
	if (cache)
		cache->cached = enable;
	return 0;
}

