#ifndef __WH_FILTER_H__
#define __WH_FILTER_H__


#define MIN_WIDTH   (96+16)
#define MIN_HEIGHT  (80)

typedef struct score_record ScoreRecord;
typedef struct score_list   ScoreList;
typedef struct wh_filter    WhFilter;

struct score_record {
	int width, height;
	unsigned score;
};
struct score_list {
	int         len;
	#define MAX_LIST_LEN (80)
	ScoreRecord records[MAX_LIST_LEN];
};
struct wh_filter {
	int key_wh_id;
	int last_wh_id;
	ScoreList score_list;
};

typedef enum {
	RET_SAME_AS_KEY,
	RET_DIFF_TO_KEY,
	RET_KEY_ERR,
}FiltRet;

int     is_valid_wh(int width, int height);
int     whfilter_init(WhFilter *ft, int key_width, int key_height);
FiltRet whfilter_filt(WhFilter *ft, int width, int height);

#endif
