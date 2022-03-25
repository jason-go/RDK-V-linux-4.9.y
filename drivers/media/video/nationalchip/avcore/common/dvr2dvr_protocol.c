#include "gxav_dvr2dvr_protocol.h"
#include "gxav_dvr_propertytypes.h"
#include "kpack_cache.h"
#include "kernelcalls.h"

#ifdef CONFIG_AV_MODULE_DVR

#if 1
#define DVR2DVR_PROTOCOL_LOCK(c)        gx_mutex_lock(&c.mutex)
#define DVR2DVR_PROTOCOL_UNLOCK(c)      gx_mutex_unlock(&c.mutex)
#define DVR2DVR_PROTOCOL_LOCK_INIT(c)   gx_mutex_init(&c.mutex)
#define DVR2DVR_PROTOCOL_LOCK_UNINIT(c) gx_mutex_destroy(&c.mutex)
#else
#define DVR2DVR_PROTOCOL_LOCK(c)         (void)0
#define DVR2DVR_PROTOCOL_UNLOCK(c)       (void)0
#define DVR2DVR_PROTOCOL_LOCK_INIT(c)    (void)0
#define DVR2DVR_PROTOCOL_LOCK_UNINIT(c)  (void)0
#endif


static struct {
	struct kpack_cache *cache;

	gx_mutex_t mutex;

	unsigned int running;
	unsigned int shared;
} _dvr2dvr_protocol;

struct _dvr_pack {
	unsigned int offset;
	unsigned int len;
};

static int _dvr_put_pack (unsigned int offset, unsigned int len)
{
	struct _dvr_pack  *dpack;
	struct cache_pack *pack = kpack_cache_alloc_pack(_dvr2dvr_protocol.cache);
	if (pack == NULL) {
		gx_printf("%s. OOM !\n", __func__);
		return -1;
	}

	dpack = (struct _dvr_pack *)pack->buffer;

	dpack->offset = offset;
	dpack->len = len;
	pack->size = sizeof(struct _dvr_pack);
	pack->pts = gx_current_tick();

	kpack_cache_put_pack(_dvr2dvr_protocol.cache, pack);
	return 0;
}

static int _dvr_get_pack (unsigned int *offset, unsigned int *len)
{
	struct _dvr_pack  *dpack;
	struct cache_pack *pack = kpack_cache_get_pack(_dvr2dvr_protocol.cache);
	if (pack == NULL) {
		//gx_printf("%s. Empty !\n", __func__);
		return -1;
	}

	dpack = (struct _dvr_pack *)pack->buffer;
	*offset = dpack->offset;
	*len = dpack->len;

	while (pack != NULL) {
		pack = kpack_cache_get_pack(_dvr2dvr_protocol.cache);
		if (pack) {
			dpack = (struct _dvr_pack *)pack->buffer;
			*len += dpack->len;
		}
	}

	kpack_cache_free_pack(_dvr2dvr_protocol.cache, NULL);
	return 0;
}

static int _dvr2dvr_protocol_callback(struct dvr_phy_buffer *src, struct dvr_phy_buffer *dst)
{
	int ret = 0;
	unsigned int offset, data_len = 0;
	unsigned int src_turnback, dst_turnback;

	DVR2DVR_PROTOCOL_LOCK(_dvr2dvr_protocol);

	_dvr_put_pack(src->offset, src->data_len);

	if (_dvr2dvr_protocol.running != 1)
		goto end;

	ret = _dvr_get_pack(&offset, &data_len);
	if (ret != 0)
		goto end;

	if (_dvr2dvr_protocol.shared == 1)
		goto end;

	src_turnback = offset + data_len > src->size;
	dst_turnback = dst->offset + data_len > dst->size;

	if (src_turnback == 0 && dst_turnback == 0) {
		memcpy(dst->vaddr + dst->offset, src->vaddr + offset, data_len);
	}
	else if (src_turnback == 0 && dst_turnback != 0) {
		unsigned int len1 = dst->size - dst->offset;
		memcpy(dst->vaddr + dst->offset, src->vaddr + offset, len1);
		memcpy(dst->vaddr, src->vaddr + offset + len1, data_len - len1);
	}
	else if (src_turnback != 0 && dst_turnback == 0) {
		unsigned int len1 = src->size - offset;
		memcpy(dst->vaddr + dst->offset, src->vaddr + offset, len1);
		memcpy(dst->vaddr + dst->offset + len1, src->vaddr, data_len - len1);
	}
	else {
		unsigned int slen1 = src->size - offset;
		unsigned int dlen1 = dst->size - dst->offset;

		if (slen1 == dlen1) {
			memcpy(dst->vaddr + dst->offset, src->vaddr + offset, slen1);
			memcpy(dst->vaddr, src->vaddr, data_len - slen1);
		}
		else if (slen1 > dlen1) {
			unsigned int len1, len2, len3;
			len1 = dlen1;
			len2 = slen1 - len1;
			len3 = data_len - len1 - len2;
			memcpy(dst->vaddr + dst->offset, src->vaddr + offset, len1);
			memcpy(dst->vaddr, src->vaddr + offset + len1, len2);
			memcpy(dst->vaddr + len2, src->vaddr, len3);
		}
		else {
			unsigned int len1, len2, len3;
			len1 = slen1;
			len2 = dlen1 - len1;
			len3 = data_len - len1 - len2;
			memcpy(dst->vaddr + dst->offset, src->vaddr + offset, len1);
			memcpy(dst->vaddr + dst->offset + len1, src->vaddr, len2);
			memcpy(dst->vaddr, src->vaddr + len2, len3);
		}
	}

	gx_dcache_flush_range(0, 0);
end:
	//gxlogi("src vaddr = 0x%p, size = %d, offset = %d, data_len = %d\n", src->vaddr, src->size, src->offset, src->data_len);
	//gxlogi("dst vaddr = 0x%p, size = %d, offset = %d, data_len = %d\n", dst->vaddr, dst->size, dst->offset, dst->data_len);
	//gxlogi("totle_len = %d\n", data_len);
	DVR2DVR_PROTOCOL_UNLOCK(_dvr2dvr_protocol);
	return data_len;
}

static int _dvr2dvr_protocol_open(int dvrid, int cachems, int shared)
{
	_dvr2dvr_protocol.shared = shared;
	_dvr2dvr_protocol.running = 0;

	DVR2DVR_PROTOCOL_LOCK_INIT(_dvr2dvr_protocol);

	_dvr2dvr_protocol.cache = gx_mallocz(sizeof(struct kpack_cache));
	if (_dvr2dvr_protocol.cache)
		return kpack_cache_init(_dvr2dvr_protocol.cache, sizeof(struct _dvr_pack), 2048, cachems/10);
	else {
		gx_printf("%s. %d, kpack_cahce init error\n", __func__, __LINE__);
		return -1;
	}
}

static int _dvr2dvr_protocol_close(int dvrid)
{
	DVR2DVR_PROTOCOL_LOCK_UNINIT(_dvr2dvr_protocol);

	if (_dvr2dvr_protocol.cache) {
		kpack_cache_uninit(_dvr2dvr_protocol.cache);
		gx_free(_dvr2dvr_protocol.cache);
		_dvr2dvr_protocol.cache = NULL;
	}

	return 0;
}

static int _dvr2dvr_protocol_config(int dvrid, int handle)
{
	return gxav_dvr_set_protocol_callback(dvrid, handle, _dvr2dvr_protocol_callback);
}

static int _dvr2dvr_protocol_start(int dvrid)
{
	DVR2DVR_PROTOCOL_LOCK(_dvr2dvr_protocol);
	if (_dvr2dvr_protocol.shared == 0)
		kpack_cache_reset(_dvr2dvr_protocol.cache);
	_dvr2dvr_protocol.running = 1;
	DVR2DVR_PROTOCOL_UNLOCK(_dvr2dvr_protocol);
	return 0;
}

static int _dvr2dvr_protocol_stop(int dvrid)
{
	DVR2DVR_PROTOCOL_LOCK(_dvr2dvr_protocol);
	_dvr2dvr_protocol.running = 0;
	kpack_cache_cache_enable(_dvr2dvr_protocol.cache, 0);
	DVR2DVR_PROTOCOL_UNLOCK(_dvr2dvr_protocol);
	return 0;
}

static int _dvr2dvr_protocol_start_cache(int dvrid)
{
	DVR2DVR_PROTOCOL_LOCK(_dvr2dvr_protocol);
	kpack_cache_cache_enable(_dvr2dvr_protocol.cache, 1);
	DVR2DVR_PROTOCOL_UNLOCK(_dvr2dvr_protocol);
	return 0;
}

int gxav_dvr2dvr_protocol_control(GxAvDVR2DVRProtocol *params)
{
	int ret = -1;

	if (params == NULL)
		return -1;

	switch (params->cmd) {
	case GXAV_DVR2DVR_PROTOCOL_OPEN:
		return _dvr2dvr_protocol_open(params->dvrid, params->params.open.cachems, params->params.open.shared);
	case GXAV_DVR2DVR_PROTOCOL_CLOSE:
		return _dvr2dvr_protocol_close(params->dvrid);
	case GXAV_DVR2DVR_PROTOCOL_CONFIG:
		return _dvr2dvr_protocol_config(params->dvrid, params->params.config.handle);
	case GXAV_DVR2DVR_PROTOCOL_START:
		return _dvr2dvr_protocol_start(params->dvrid);
	case GXAV_DVR2DVR_PROTOCOL_STOP:
		return _dvr2dvr_protocol_stop(params->dvrid);
	case GXAV_DVR2DVR_PROTOCOL_START_CACHE:
		return _dvr2dvr_protocol_start_cache(params->dvrid);
	default:
		break;
	};

	return ret;
}

#endif
