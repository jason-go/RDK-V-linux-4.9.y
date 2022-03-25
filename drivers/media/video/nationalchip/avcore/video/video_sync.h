#ifndef _VIDEO_SYNC_H_
#define _VIDEO_SYNC_H_
#include "kernelcalls.h"

typedef enum {
	SYNC_COMMON,
	SYNC_SKIP,
	SYNC_REPEAT,
}VideoSyncRet;

typedef enum {
	SYNC_MODE_FREERUN,
	SYNC_MODE_NORMAL,
	SYNC_MODE_SYNC_SLOWLY,
	SYNC_MODE_SYNC_LATER,
}VideoSyncMode;

struct video_sync {
	VideoSyncMode sync_mode;
	unsigned int  freq;
	unsigned int  low_tolerance;
	unsigned int  high_tolerance;
	unsigned int  high_tolerance_ms;
	unsigned int  low_tolerance_ms;
	unsigned int  frame_dis;
	unsigned int  last_pts;
	int           pts_offset;

	struct {
		unsigned flag;
		unsigned cnt;
		unsigned gate;
	} synced, lose_sync;

	struct {
		unsigned recovered;
		unsigned cnt;
		unsigned gate;
	} stc_check;

	unsigned int  strict;
	struct {
		int repeat_cnt;
		int repeat_gate;
	} slowly;
};

#define MIN_LOW_TOLERANCE_MS  (150)
#define MAX_HIGH_TOLERANCE_MS (4000)

extern struct video_sync av_sync[2];
int  video_pts_sync     (struct video_sync *sync, unsigned int pts, unsigned int fps);
void video_sync_init    (struct video_sync *sync);
void video_set_sync_mode(struct video_sync *sync, VideoSyncMode mode);
void video_get_sync_mode(struct video_sync *sync, int *mode);
void video_get_pts      (struct video_sync *sync, unsigned int *pts);
void video_tolerance_fresh(struct video_sync *sync);
void video_set_band_tolerance_fresh(struct video_sync *sync, unsigned int val);
void video_sync_strict_enable(struct video_sync *sync, unsigned int enable);
unsigned int video_sync_get_frame_dis(struct video_sync *sync);
int  videosync_set_ptsoffset(struct video_sync *sync, int offset_ms);
#endif
