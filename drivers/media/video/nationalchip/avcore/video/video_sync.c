#include "video_sync.h"
#include "stc_hal.h"
#include "porting.h"
#include "decode_core.h"

#include "v_debug.h"

#define DEFAULT_LOW_TOLERANCE_MS  (200)
#define DEFAULT_HIGH_TOLERANCE_MS (4000)

#define DEFAULT_SYNCED_GATE       (3)
#define DEFAULT_LOSE_SYNC_GATE    (3)

#define SYNC_SLOWLY_REPEAT_GATE   (3)

struct video_sync av_sync[2];

void video_sync_init(struct video_sync *sync)
{
	gx_memset(sync, 0 ,sizeof(struct video_sync));
	sync->high_tolerance_ms  = DEFAULT_HIGH_TOLERANCE_MS;
	sync->low_tolerance_ms   = DEFAULT_LOW_TOLERANCE_MS;
	sync->synced.gate        = DEFAULT_SYNCED_GATE;
	sync->lose_sync.gate     = DEFAULT_LOSE_SYNC_GATE;
	sync->slowly.repeat_cnt  = 0;
	sync->slowly.repeat_gate = SYNC_SLOWLY_REPEAT_GATE;
	sync->stc_check.gate     = 100;
	sync->sync_mode = SYNC_MODE_NORMAL;
	video_tolerance_fresh(sync);
}

void video_set_sync_mode(struct video_sync *sync, VideoSyncMode mode)
{
#ifdef V_DEBUG
	char *mode_desc[] = {
		"SYNC_MODE_FREERUN",
		"SYNC_MODE_NORMAL",
		"SYNC_MODE_SYNC_SLOWLY",
		"SYNC_MODE_SYNC_LATER",
	};
#endif

	sync->sync_mode = mode;
	VDBG_FORCE_SYNC(sync);
	VDBG_PRINT("\n[AV-SYNC] : %s\n", mode_desc[mode]);
}

unsigned int video_sync_get_frame_dis(struct video_sync *sync)
{
	return sync->frame_dis;
}

void video_get_sync_mode(struct video_sync *sync, int *mode)
{
	*mode = sync->sync_mode;
}

void video_get_pts(struct video_sync *sync, unsigned int *pts)
{
	*pts = sync->last_pts;
}

void video_tolerance_fresh(struct video_sync *sync)
{
	GxSTCProperty_TimeResolution  resolution = {0};

	gxav_stc_get_base_resolution(0,&resolution);
	if(resolution.freq_HZ==0) {
		resolution.freq_HZ = 45000;
	}
	sync->freq           = resolution.freq_HZ;
	sync->low_tolerance  = sync->low_tolerance_ms  * (resolution.freq_HZ/1000);
	sync->high_tolerance = sync->high_tolerance_ms * (resolution.freq_HZ/1000);
}

void video_sync_strict_enable(struct video_sync *sync, unsigned int enable)
{
	if(sync) {
		if(enable) {
			sync->strict = 1;
			sync->low_tolerance     = sync->frame_dis/2;
			sync->high_tolerance_ms = DEFAULT_HIGH_TOLERANCE_MS;
			sync->synced.gate       = 1;
			sync->lose_sync.gate    = 1;
		} else {
			sync->strict = 0;
			sync->low_tolerance_ms  = DEFAULT_LOW_TOLERANCE_MS;
			sync->synced.gate       = DEFAULT_SYNCED_GATE;
			sync->lose_sync.gate    = DEFAULT_LOSE_SYNC_GATE;
		}

		video_tolerance_fresh(sync);
	}
}

static void video_sync_update_low_tolerance(struct video_sync *sync)
{
	if(sync->strict) {
		sync->low_tolerance   = sync->frame_dis/2;
		sync->high_tolerance *= 2;
	} else {
		sync->low_tolerance_ms  = DEFAULT_LOW_TOLERANCE_MS;
		video_tolerance_fresh(sync);
	}
}

int videosync_set_ptsoffset(struct video_sync *sync, int offset_ms)
{
	GxSTCProperty_TimeResolution  resolution = {0};

	if(sync && offset_ms) {
		gxav_stc_get_base_resolution(0, &resolution);
		if(resolution.freq_HZ == 0) {
			resolution.freq_HZ = 45000;
		}
		sync->pts_offset = offset_ms * (resolution.freq_HZ/1000);
	}

	return 0;
}

void video_set_band_tolerance_fresh(struct video_sync *sync, unsigned int value)
{
	sync->high_tolerance_ms = value;
	video_tolerance_fresh(sync);
}

int video_pts_sync_common(struct video_sync *sync, unsigned int pts, unsigned int fps)
{
	unsigned long long stc = 0;
	unsigned int stc_tmp = 0, disp_pts = pts;
	unsigned int high_tolerance = 0, low_tolerance = 0;

	static unsigned int first_pts_timeout = 0;

	if(0 == sync->frame_dis && fps) {
		if(fps) {
			sync->frame_dis = sync->freq*1000/fps;
			first_pts_timeout = 0;
			video_sync_update_low_tolerance(sync);
		} else {
			return SYNC_COMMON;
		}
	}

	if(sync->sync_mode == SYNC_MODE_FREERUN) {
		if(disp_pts==0 && sync->last_pts!=0) {
			disp_pts = sync->last_pts + sync->frame_dis;
		}
		sync->last_pts = disp_pts;
		return SYNC_COMMON;
	}

#if 0
	if(sync->frame_dis) {
		sync->low_tolerance = sync->frame_dis*3;
	}
	low_tolerance  = sync->synced.flag ? sync->low_tolerance : sync->low_tolerance/2;
#else
	low_tolerance  = sync->low_tolerance;
#endif
	high_tolerance = sync->high_tolerance;

	if(disp_pts != 0) {
		disp_pts = disp_pts + high_tolerance;
		disp_pts = (int)disp_pts + sync->pts_offset;
	} else {
		sync->lose_sync.cnt++;
		if(first_pts_timeout++ < fps/1000) {
			return SYNC_SKIP;
		} else {
			gx_printf("vpts is zero, please check \n");
			return SYNC_COMMON;
		}
	}

	gxav_stc_read_stc(0, &stc_tmp);
	gxav_stc_write_vpts(0, pts+sync->pts_offset, 0);

	stc = stc_tmp + high_tolerance;
	if((stc&0x1) == 0) {
		sync->lose_sync.cnt++;
		sync->last_pts = disp_pts - high_tolerance;
		if(sync->stc_check.recovered == 0) {
			VDBG_PRINT_SYNC_INFO("re2 ", stc-high_tolerance, disp_pts-high_tolerance);
			if(sync->stc_check.cnt++ > sync->stc_check.gate) {
				sync->stc_check.recovered = 1;
			}
			return SYNC_REPEAT;
		}
		else {
			VDBG_PRINT_SYNC_INFO("cm2 ", stc-high_tolerance, disp_pts-high_tolerance);
			return SYNC_COMMON;
		}
	}
	sync->stc_check.recovered = 1;

	if(disp_pts>(stc+low_tolerance) && disp_pts<(stc+high_tolerance)) {
		sync->lose_sync.cnt++;
		if(sync->synced.flag && sync->lose_sync.cnt < sync->lose_sync.gate) {
			VDBG_PRINT_SYNC_INFO("re0 ", stc-high_tolerance, disp_pts-high_tolerance);
			goto COMMON;
		}
		sync->synced.cnt  = 0;
		sync->synced.flag = 0;
		sync->lose_sync.flag = 1;
		VDBG_PRINT_SYNC_INFO("re1 ", stc-high_tolerance, disp_pts-high_tolerance);
		goto REPEAT;
	} else if((disp_pts>(stc-high_tolerance)) && (disp_pts<(stc-low_tolerance))) {
		sync->lose_sync.cnt++;
		if(sync->synced.flag && sync->lose_sync.cnt < sync->lose_sync.gate) {
			VDBG_PRINT_SYNC_INFO("sk0 ", stc-high_tolerance, disp_pts-high_tolerance);
			goto COMMON;
		}
		sync->synced.cnt  = 0;
		sync->synced.flag = 0;
		sync->lose_sync.flag = 1;
		VDBG_PRINT_SYNC_INFO("sk1 ", stc-high_tolerance, disp_pts-high_tolerance);
		goto SKIP;
	} else {
		if(abs((stc-disp_pts)) > low_tolerance) {
			//free run
			if(++sync->lose_sync.cnt >= sync->lose_sync.gate) {
				sync->lose_sync.flag = 1;
			}
			VDBG_PRINT_SYNC_INFO("cm0 ", stc-high_tolerance, disp_pts-high_tolerance);
		} else {
			if(++sync->synced.cnt >= sync->synced.gate) {
				sync->synced.flag = 1;
				sync->lose_sync.cnt  = 0;
				sync->lose_sync.flag = 0;
				if(sync->sync_mode == SYNC_MODE_SYNC_SLOWLY)
					sync->sync_mode = SYNC_MODE_NORMAL;
			}
			VDBG_PRINT_SYNC_INFO("cm1 ", stc-high_tolerance, disp_pts-high_tolerance);
		}
		goto COMMON;
	}

COMMON:
	sync->last_pts = disp_pts - high_tolerance;
	return SYNC_COMMON;
SKIP:
	sync->last_pts = disp_pts - high_tolerance;
	return SYNC_SKIP;
REPEAT:
	return SYNC_REPEAT;
}

int video_pts_sync(struct video_sync *sync, unsigned int vpts, unsigned int fps)
{
	VideoSyncRet ret;
	unsigned int apts, pcr;
	gxav_stc_read_apts(0, &apts);
	gxav_stc_read_pcr (0, &pcr);
	VDBG_PRINT_APTS_VPTS_PCR("VDBG", apts, vpts, pcr);

	ret = video_pts_sync_common(sync, vpts, fps);

	if(sync && sync->sync_mode == SYNC_MODE_SYNC_LATER) {
		return SYNC_COMMON;
	} else if(sync->sync_mode == SYNC_MODE_SYNC_SLOWLY && ret == SYNC_REPEAT) {
		if((sync->slowly.repeat_cnt++ % sync->slowly.repeat_gate) == 0)
			return SYNC_COMMON;
	}

	return ret;
}

