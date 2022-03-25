#include "ptsfixer.h"
#include "kernelcalls.h"
#include "stc_hal.h"

#define IS_VALID_PTS(to_check, should_be, tolerance)\
	(abs((to_check)-(should_be)) < (tolerance))

#define IS_MAIN_SLOT(slot)\
	((slot)->fixer->main_slot == (slot)->id)

static int keypts_is_ok(FixerInfo *fixer, int pts)
{
	int ok = 0;
	int to_last_cnt;
	unsigned remainder, sum_remainder, fps, stc_freq;
	unsigned should_be, tolerance;

	if (fixer->inited) {
		fps       = fixer->pts_slot[0].fps;
		stc_freq  = fixer->pts_slot[0].stc_freq;
		remainder = fixer->pts_slot[0].remainder;
		tolerance = fixer->pts_slot[0].tolerance;

		if (fixer->last_keypts == 0) {
			fixer->last_keypts     = pts;
			fixer->last_keypts_cnt = fixer->cnt;
			ok = 1;
		} else {
			to_last_cnt = fixer->cnt - fixer->last_keypts_cnt;

			sum_remainder = to_last_cnt * remainder;
			should_be = fixer->last_keypts + stc_freq/fps*to_last_cnt;
			if (sum_remainder / fps) {
				should_be += sum_remainder/fps;
			}

			fixer->last_keypts     = pts;
			fixer->last_keypts_cnt = fixer->cnt;

			if (IS_VALID_PTS(pts, should_be, tolerance)) {
				ok = 1;
			}
		}
	}

	//gx_printf("[%d] pts = %d, keypts %s!\n", fixer->cnt, pts, ok == 1 ? "OK" : "BAD");
	return ok;
}

static int fix_pts(PtsSlot *slot, unsigned int pts, int key_pts)
{
	int ret = 0;
	unsigned int should_be;

	if (!slot->actived && pts) {
		slot->actived       = 1;
		slot->last_ok_pts   = pts;
		slot->to_lastok_cnt = 0;
		slot->synced_cnt    = 0;
		slot->fixed_pts     = pts;
		slot->sum_remainder = 0;
	} else {
		slot->to_lastok_cnt++;
		slot->sum_remainder = slot->to_lastok_cnt * slot->remainder;
		should_be = slot->last_ok_pts + slot->stc_freq/slot->fps*slot->to_lastok_cnt;
		if (slot->sum_remainder / slot->fps) {
			should_be += slot->sum_remainder / slot->fps;
		}
		if (key_pts && pts) {
			should_be = pts;
		}
		if (IS_VALID_PTS(pts, should_be, slot->tolerance)) {
			slot->last_ok_pts   = pts;
			slot->to_lastok_cnt = 0;
			slot->fixed_pts     = pts;
			slot->synced_cnt++;
		} else {
			slot->fixed_pts = should_be;
			if ((!IS_MAIN_SLOT(slot)) && pts) {
				slot->last_ok_pts   = pts;
				slot->to_lastok_cnt = 0;
				slot->synced_cnt    = 0;
			}
			ret = -1;
		}
	}

	return ret;
}

int ptsfixer_init(FixerInfo *fixer, int synced_gate, int frame_rate)
{
	GxSTCProperty_TimeResolution  resolution = {0};

	if(!frame_rate) return -1;

	memset(fixer, 0, sizeof(FixerInfo));
	fixer->frame_rate  = frame_rate;
	fixer->synced_gate = synced_gate;
	gxav_stc_get_base_resolution(0, &resolution);
	fixer->pts_slot[0].id           = 0;
	fixer->pts_slot[0].fixer        = fixer;
	fixer->pts_slot[0].stc_freq     = resolution.freq_HZ*1000;
	fixer->pts_slot[0].fps          = frame_rate;
	fixer->pts_slot[0].pts_distance = resolution.freq_HZ*1000/frame_rate;
	fixer->pts_slot[0].tolerance    = fixer->pts_slot[0].pts_distance/10;
	fixer->pts_slot[0].remainder    = resolution.freq_HZ*1000%frame_rate;
	fixer->pts_slot[1].id           = 1;
	fixer->pts_slot[1].fixer        = fixer;
	fixer->pts_slot[1].stc_freq     = resolution.freq_HZ*1000;
	fixer->pts_slot[1].fps          = frame_rate;
	fixer->pts_slot[1].pts_distance = resolution.freq_HZ*1000/frame_rate;
	fixer->pts_slot[1].tolerance    = fixer->pts_slot[0].pts_distance/10;
	fixer->pts_slot[1].remainder    = resolution.freq_HZ*1000%frame_rate;

	fixer->main_slot = 0;
	fixer->mode      = MODE_NORMAL;
	fixer->inited    = 1;

	return 0;
}

int ptsfixer_uninit(FixerInfo *fixer)
{
	memset(fixer, 0, sizeof(FixerInfo));
	return 0;
}

unsigned int ptsfixer_run(FixerInfo *fixer, unsigned int pts, int frame_rate, int key_pts)
{
	int ret;
	unsigned int ret_pts = pts;
	int assist_slot, main_slot = fixer->main_slot;

	fixer->cnt++;
	if (frame_rate && !fixer->inited && pts) {
		ptsfixer_init(fixer, SYNCED_GATE, frame_rate);
	}

	if (fixer->inited) {
		if (key_pts) {
			key_pts = keypts_is_ok(fixer, pts);
		}

		if(frame_rate && fixer->frame_rate!=frame_rate) {
			fixer->mode = MODE_BYPASS;
		}

		if (fixer->mode == MODE_BYPASS) {
			ret_pts = pts;
		} else if (fixer->mode == MODE_NORMAL) {
			assist_slot = (main_slot+1)%2;
			ret = fix_pts(&fixer->pts_slot[main_slot], pts, key_pts);
			if (ret) {
				fix_pts(&fixer->pts_slot[assist_slot], pts, key_pts);
				if (fixer->pts_slot[assist_slot].synced_cnt >= fixer->synced_gate) {
					fixer->pts_slot[main_slot].actived = 0;
					fixer->main_slot = assist_slot;
				}
			}
			ret_pts = fixer->pts_slot[fixer->main_slot].fixed_pts;
		}
	}

	return ret_pts;
}

