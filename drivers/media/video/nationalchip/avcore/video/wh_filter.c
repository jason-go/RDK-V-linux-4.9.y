#include "wh_filter.h"
#include "kernelcalls.h"
#include "gxav_common.h"

static ScoreList special_wh_list = {
	.len = 22,
	.records  = {
		{  720,   480,  100, },
		{  720,   576,  100, },
		{ 1280,   720,  100, },
		{ 1920,  1088,  100, },
		{ 1920,  1080,  100, },
		{  704,   576,   80, },
		{  640,   480,   80, },
		{  352,   288,   60, },
		{  352,   240,   60, },
		{  540,   576,   60, },
		{  540,   480,   60, },
		{  352,   576,   60, },
		{  480,   576,   60, },
		{  352,   480,   60, },
		{  480,   480,   40, },
		{  352,   576,   10, },
		{  352,   480,   10, },
		{  480,   576,   10, },
		{  480,   480,   10, },
		{  540,   576,   10, },
		{  540,   480,   10, },
		{    0,     0,    0, },
	},
};

static int scorelist_search(ScoreList *list, int width, int height)
{
	int i, id;

	id = -1;
	for(i = 0; i < list->len; i++) {
		if(list->records[i].width==width && list->records[i].height==height)
			id = i;
	}
	return id;
}

static int scorelist_add(ScoreList *list, int width, int height)
{
	int i;
	int min_score, addto_id;

	min_score = list->records[0].score;
	addto_id  = 0;
	for(i = 0; i < list->len; i++) {
		if(list->records[i].width==width && list->records[i].height==height) {
			list->records[i].score ++;
			return i;
		}
		if(list->records[i].score < min_score) {
			min_score = list->records[i].score;
			addto_id  = i;
		}
	}
	if(i == list->len) {
		int tmp_id;
		list->records[addto_id].width = width;
		list->records[addto_id].height= height;
		tmp_id = scorelist_search(&special_wh_list, width, height);
		if(tmp_id >= 0)
			list->records[addto_id].score = special_wh_list.records[tmp_id].score;
		else
			list->records[addto_id].score = 1;
	}
	return addto_id;
}

int is_valid_wh(int width, int height)
{
	unsigned int max_width, max_height;

	if (CHIP_IS_GX3113C == 1) {
		max_width  = (720);
		max_height = (576);
	}
	else {
		max_width  = (1920);
		max_height = (1920);
	}

	return width>=MIN_WIDTH && width <=max_width && height>=MIN_HEIGHT && height<=max_height &&\
		(width*height <= 1920*1088);
}

int whfilter_init(WhFilter *ft, int key_width, int key_height)
{
	int id;
	gx_memset(ft, 0, sizeof(WhFilter));
	ft->key_wh_id = 0;
	ft->last_wh_id = ft->key_wh_id;
	#define LIST_LEN (8)
	ft->score_list.len = LIST_LEN;
	ft->score_list.records[ft->key_wh_id].width = key_width;
	ft->score_list.records[ft->key_wh_id].height= key_height;
	id = scorelist_search(&special_wh_list, key_width, key_height);
	if(id >= 0)
		ft->score_list.records[ft->key_wh_id].score = special_wh_list.records[id].score;
	else
		ft->score_list.records[ft->key_wh_id].score = 1;
	return 0;
}

#define MAX_SCORE (200)
#define SCORE_INC(ft, id) \
	do {\
		if(id == ft->last_wh_id)\
			ft->score_list.records[id].score += 50;\
		else\
			ft->score_list.records[id].score += 1;\
		if(ft->score_list.records[id].score >= MAX_SCORE)\
			ft->score_list.records[id].score = MAX_SCORE;\
	}while(0)
FiltRet whfilter_filt(WhFilter *ft, int width, int height)
{
	int id;
	int key_id    = ft->key_wh_id;
	int key_score = ft->score_list.records[key_id].score;

	id = scorelist_search(&ft->score_list, width, height);
	if(id >= 0) {
		SCORE_INC(ft, id);
		ft->last_wh_id = id;

		if(id == key_id)
			return RET_SAME_AS_KEY;
		else if(ft->score_list.records[id].score >= key_score)
			return RET_KEY_ERR;
		else
			return RET_DIFF_TO_KEY;
	}
	else {
		ft->last_wh_id = scorelist_add(&ft->score_list, width, height);
		return RET_DIFF_TO_KEY;
	}
}

