#ifndef __PTS_FIXER_H__
#define __PTS_FIXER_H__

typedef struct pts_slot   PtsSlot;
typedef struct fixer_info FixerInfo;

#define SYNCED_GATE (2)

struct pts_slot {
	int id;
	int actived;
	int to_lastok_cnt;
	int synced_cnt;
	int tolerance;
	int pts_distance;
	unsigned int last_ok_pts;
	unsigned int fixed_pts;

	int remainder, sum_remainder;
	int stc_freq;
	int fps;
	FixerInfo *fixer;
};

typedef enum {
	MODE_BYPASS,
	MODE_NORMAL,
}FixerMode;

struct fixer_info {
	int       cnt;
	int       inited;
	FixerMode mode;
	int       frame_rate;
	int       synced_gate;
	int       main_slot;
	PtsSlot   pts_slot[2];
	int       last_keypts;
	int       last_keypts_cnt;
};

         int ptsfixer_init  (FixerInfo *thiz, int synced_gate, int frame_rate);
         int ptsfixer_uninit(FixerInfo *thiz);
unsigned int ptsfixer_run   (FixerInfo *thiz, unsigned int pts, int frame_rate, int key_pts);
#endif

