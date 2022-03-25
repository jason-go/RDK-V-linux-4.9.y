#ifndef __PTS_FETCHER_H__
#define __PTS_FETCHER_H__

typedef struct {
	unsigned int id;
	unsigned int addr;
	unsigned int size;
}RingBufInfo;

typedef struct {
	unsigned int addr;
	unsigned int pts;
	unsigned int es_loop;
	unsigned int used;
}PtsNode;

typedef struct {
	struct {
		unsigned int id;
		unsigned int addr;
		unsigned int size;
		unsigned int loop;
		unsigned int last_fetch_addr;
		unsigned int loop_loop;
	}esbuf;
	struct {
		unsigned int id;
		unsigned int addr;
		unsigned int size;
		unsigned int es_loop;
		unsigned int end_addr;
		unsigned int got_first_node;
		unsigned int rd_offset;
		PtsNode cur, nxt;
		PtsNode last_node;
		unsigned int loop_loop;
	}ptsbuf;
	unsigned int max_loop;
}PtsFetcher;

int ptsfetcher_init (PtsFetcher *thiz, RingBufInfo *esbuf_info, RingBufInfo *ptsbuf_info);
int ptsfetcher_fetch(PtsFetcher *thiz, unsigned int fetch_addr, unsigned int *ret_pts);
int ptsfetcher_drop (PtsFetcher *thiz, unsigned int drop_addr);

#endif
