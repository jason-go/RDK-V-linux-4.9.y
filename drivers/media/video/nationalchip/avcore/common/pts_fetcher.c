#include "sdc_hal.h"
#include "kernelcalls.h"
#include "pts_fetcher.h"

#if defined CONFIG_AV_MODULE_AUDIO_DEC || defined CONFIG_AV_MODULE_VIDEO_DEC

#define FETCHER_DEBUG (0)

#if FETCHER_DEBUG
static int p_fetch = 1;
#   define FDBG_PRINT(fmt, arg...) \
	do {\
		if (p_fetch)\
			gx_printf(fmt, ## arg);\
	} while(0)
#else
#   define FDBG_PRINT(fmt, arg...) {;}
#endif

static unsigned int ringbuf_read_word(unsigned int start_addr, unsigned int *offset, unsigned int size)
{
	unsigned int ret_val = 0;

	if(*offset >= size)
		*offset %= size;
	ret_val = *(unsigned int*)(start_addr + *offset);
	*offset += 4;
	return ret_val;
}

static int ptsfetcher_bufloop_inc(PtsFetcher *thiz, unsigned int buf_id)
{
	if(thiz != NULL) {
		if(buf_id == thiz->esbuf.id) {
			thiz->esbuf.loop += 1;
			if(thiz->esbuf.loop == thiz->max_loop) {
				thiz->ptsbuf.es_loop = 0;
				thiz->ptsbuf.cur.es_loop = 0;
				thiz->ptsbuf.nxt.es_loop = 0;
				thiz->esbuf.loop = 1;
			}
		} else if(buf_id == thiz->ptsbuf.id) {
			thiz->ptsbuf.es_loop += 1;
			if(thiz->ptsbuf.es_loop == thiz->max_loop) {
				thiz->esbuf.loop = 0;
				thiz->ptsbuf.cur.es_loop = 0;
				thiz->ptsbuf.nxt.es_loop = 0;
				thiz->ptsbuf.es_loop = 1;
			}
		}
	} else {
		FDBG_PRINT("%s:%d, loop++ failed!\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int ptsfetcher_ptsbuf_read(PtsFetcher *thiz, PtsNode *ret_node)
{
	unsigned int len = 0;

	if(thiz!=NULL && ret_node!=NULL) {
		gxav_sdc_length_get(thiz->ptsbuf.id, &len);
		if(len < 8)
			return -1;
		else {
			ret_node->used = 0;
			ret_node->addr = ringbuf_read_word(thiz->ptsbuf.addr, &thiz->ptsbuf.rd_offset, thiz->ptsbuf.size);
			ret_node->addr = (ret_node->addr&0x0fffffff) - (thiz->esbuf.addr&0x0fffffff);
			ret_node->pts  = ringbuf_read_word(thiz->ptsbuf.addr, &thiz->ptsbuf.rd_offset, thiz->ptsbuf.size);
			if(thiz->ptsbuf.got_first_node == 0) {
				ret_node->es_loop = 0;
			} else {
				if(ret_node->addr < thiz->ptsbuf.last_node.addr)
					ptsfetcher_bufloop_inc(thiz, thiz->ptsbuf.id);
				ret_node->es_loop = thiz->ptsbuf.es_loop;
			}
			thiz->ptsbuf.last_node = *ret_node;
			gxav_sdc_length_set(thiz->ptsbuf.id, 8, GXAV_SDC_READ);
		}
	} else {
		FDBG_PRINT("%s:%d, set bufinfo failed!\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

int ptsfetcher_init(PtsFetcher *thiz, RingBufInfo *esbuf_info, RingBufInfo *ptsbuf_info)
{
	if(thiz!=NULL && esbuf_info!=NULL && esbuf_info!=NULL) {
		memset(thiz, 0, sizeof(PtsFetcher));
		thiz->max_loop         = 500;
		thiz->esbuf.id         = esbuf_info->id;
		thiz->esbuf.size       = esbuf_info->size;
		thiz->esbuf.addr       = esbuf_info->addr;
		thiz->ptsbuf.id        = ptsbuf_info->id;
		thiz->ptsbuf.size      = ptsbuf_info->size;
		thiz->ptsbuf.addr      = ptsbuf_info->addr;
		thiz->ptsbuf.rd_offset = 0;
		thiz->ptsbuf.got_first_node = 0;
	}
	else {
		FDBG_PRINT("%s:%d, set bufinfo failed!\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

int ptsfetcher_fetch(PtsFetcher *thiz, unsigned int fetch_addr, unsigned int *ret_pts)
{
	int ret;
	PtsNode new, *cur = NULL, *nxt = NULL, *result = NULL;
	unsigned int abs_cur, abs_nxt, abs_fetch;
#define GET_ABS_ADDR(addr, loop, size) ((addr) + (loop)*(size))

	if(thiz == NULL) {
		goto NO_PTS;
	}

	cur = &thiz->ptsbuf.cur;
	nxt = &thiz->ptsbuf.nxt;
	//init
	if(thiz->ptsbuf.got_first_node == 0) {
		ret = ptsfetcher_ptsbuf_read(thiz, cur);
		if(ret != 0) {
			goto NO_PTS;
		} else {
			thiz->ptsbuf.got_first_node = 1;
			*nxt = *cur;
		}
	}

	//update es loop
	if(fetch_addr < thiz->esbuf.last_fetch_addr) {
		ptsfetcher_bufloop_inc(thiz, thiz->esbuf.id);
	}
	thiz->esbuf.last_fetch_addr = fetch_addr;

	do {
		//get absolute-addr
		abs_cur   = GET_ABS_ADDR(cur->addr,  cur->es_loop,     thiz->esbuf.size);
		abs_nxt   = GET_ABS_ADDR(nxt->addr,  nxt->es_loop,     thiz->esbuf.size);
		abs_fetch = GET_ABS_ADDR(fetch_addr, thiz->esbuf.loop, thiz->esbuf.size);
		//fetch
		if(abs_fetch < abs_cur) {
			goto NO_PTS;
		} else if(abs_fetch>=abs_cur && abs_fetch<abs_nxt) {
			if(cur->used == 0) {
				result = cur;
				goto GOT_PTS;
			} else {
				goto NO_PTS;
			}
		} else {
			if(ptsfetcher_ptsbuf_read(thiz, &new) == 0) {
				*cur = *nxt;
				*nxt = new;
			} else if(nxt->used == 0) {
				result = nxt;
				goto GOT_PTS;
			} else {
				goto NO_PTS;
			}
		}
	} while(1);

GOT_PTS:
	if (ret_pts != NULL)
		*ret_pts = result->pts;
	result->used = 1;
	FDBG_PRINT("[F]: cur = [%u]%u, nxt = [%u]%u, offset = [%u]%u, pts = %u\n", cur->es_loop, cur->addr, nxt->es_loop, nxt->addr, thiz->esbuf.loop, fetch_addr, result->pts);
	return 0;

NO_PTS:
	if (ret_pts != NULL)
		*ret_pts = 0;
	FDBG_PRINT("[F]: cur = [%u]%u, nxt = [%u]%u, offset = [%u]%u, pts = %u\n", cur->es_loop, cur->addr, nxt->es_loop, nxt->addr, thiz->esbuf.loop, fetch_addr, 0);
	return -1;
}

int ptsfetcher_drop(PtsFetcher *thiz, unsigned int drop_addr)
{
	int ret;
	PtsNode new, *cur = NULL, *nxt = NULL;
	unsigned int abs_cur, abs_nxt, abs_drop;
#define GET_ABS_ADDR(addr, loop, size) ((addr) + (loop)*(size))

	cur = &thiz->ptsbuf.cur;
	nxt = &thiz->ptsbuf.nxt;
	//init
	if(thiz->ptsbuf.got_first_node == 0) {
		ret = ptsfetcher_ptsbuf_read(thiz, cur);
		if(ret != 0) {
			goto OUT;
		} else {
			thiz->ptsbuf.got_first_node = 1;
			*nxt = *cur;
		}
	}

	//update es loop
	if(drop_addr < thiz->esbuf.last_fetch_addr)
		ptsfetcher_bufloop_inc(thiz, thiz->esbuf.id);
	thiz->esbuf.last_fetch_addr = drop_addr;
	do {
		//get absolute-addr
		abs_cur   = GET_ABS_ADDR(cur->addr,  cur->es_loop,     thiz->esbuf.size);
		abs_nxt   = GET_ABS_ADDR(nxt->addr,  nxt->es_loop,     thiz->esbuf.size);
		abs_drop = GET_ABS_ADDR(drop_addr, thiz->esbuf.loop, thiz->esbuf.size);
		//fetch
		if(abs_drop <= abs_nxt) {
			FDBG_PRINT("[D]need not: cur = [%u]%u, nxt = [%u]%u, offset = [%u]%u\n", cur->es_loop, cur->addr, nxt->es_loop, nxt->addr, thiz->esbuf.loop, drop_addr);
			break;
		}
		if(ptsfetcher_ptsbuf_read(thiz, &new) == 0) {
			FDBG_PRINT("[D]drop cur: cur = [%u]%u, nxt = [%u]%u, offset = [%u]%u\n", cur->es_loop, cur->addr, nxt->es_loop, nxt->addr, thiz->esbuf.loop, drop_addr);
			*cur = *nxt;
			*nxt = new;
		} else {
			FDBG_PRINT("[D]no nxt: offset = [%u]%u\n", thiz->esbuf.loop, drop_addr);
			break;
		}
	} while(1);

OUT:
	return 0;
}

#endif
