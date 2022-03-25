#include "avcore.h"
#include "pcrfixer.h"
#include "gxav_common.h"

enum {
	DEBUG_PCRFIX_NO        = 0,
	DEBUG_PCRFIX_FIX       = 1,
	DEBUG_PCRFIX_DEVIATION = 2
};

#define MAX_DEBUG_PCRFIX_COUNT (20)
static char debug_pcrfix = DEBUG_PCRFIX_NO;
static char debug_pcrfix_count = 0;

#define SYNC_LOW_TOLERANCE      (1/50)
#define SYNC_HIGH_TOLERANCE     (2)
#define PTS_ERROR_TIME_GATE     (2)
#define STANDARD_DEVIATION_GATE (2000*2000)

void pcrfixer_init(struct pcr_fixer *thiz, int freq)
{
	if (thiz) {
		thiz->enable = (freq == 45000 ? 1: 0);
		thiz->cnt        = 0;
		thiz->sum_offset = 0;
		thiz->cache_size = 20;
		thiz->apts       = thiz->vpts = 0;
		thiz->fix_val    = thiz->last_fix_val = -1;
		thiz->last_apts  = thiz->last_vpts = 0;
		thiz->apts_err   = thiz->vpts_err = 0;
		thiz->apts_err_time = thiz->vpts_err_time = 0;
		thiz->dst_offset = freq*SYNC_LOW_TOLERANCE;
		thiz->tolerance  = freq*SYNC_LOW_TOLERANCE;
		thiz->sync_low_tolerance  = freq*SYNC_LOW_TOLERANCE;
		thiz->sync_high_tolerance = freq*SYNC_HIGH_TOLERANCE;
		thiz->pts_error_time_gate = freq*PTS_ERROR_TIME_GATE;
		thiz->standard_deviation_gate = STANDARD_DEVIATION_GATE;
		thiz->standard_deviation      = 0;
		thiz->convert_time_factor     = (freq==0)?45:freq/1000;
	}
}

void pcrfixer_check(struct pcr_fixer *thiz, int stc)
{
	if (thiz->last_apts != 0) {
		if (thiz->last_apts == thiz->apts) {
			if (thiz->apts_err_time == 0) {
				thiz->apts_err_time = stc;
			}
			else {
				if (stc - thiz->apts_err_time >= thiz->pts_error_time_gate) {
					thiz->apts_err = 1;
				}
			}
		}
		else {
			thiz->apts_err = 0;
			thiz->apts_err_time = 0;
		}
	}

	if (thiz->last_vpts != 0) {
		if (thiz->last_vpts == thiz->vpts) {
			if (thiz->vpts_err_time == 0) {
				thiz->vpts_err_time = stc;
			}
			else {
				if (stc - thiz->vpts_err_time >= thiz->pts_error_time_gate) {
					thiz->vpts_err = 1;
				}
			}
		}
		else {
			thiz->vpts_err = 0;
			thiz->vpts_err_time = 0;
		}
	}
}

unsigned pcrfixer_check_gate(struct pcr_fixer *thiz, unsigned int pcr)
{
	unsigned int apts_ms = thiz->apts/thiz->convert_time_factor;
	unsigned int vpts_ms = thiz->vpts/thiz->convert_time_factor;
	unsigned int pcr_ms  = pcr/thiz->convert_time_factor;
	unsigned int average = (apts_ms + vpts_ms + pcr_ms)/3;
	unsigned long long standard_deviation = 0;

	standard_deviation  = (apts_ms - average)*(apts_ms - average);
	standard_deviation += (vpts_ms - average)*(vpts_ms - average);
	standard_deviation += (pcr_ms  - average)*(pcr_ms  - average);
	do_div(standard_deviation, 3);

	if (debug_pcrfix == DEBUG_PCRFIX_DEVIATION) {
		debug_pcrfix_count++;
		if (standard_deviation > thiz->standard_deviation) {
			gx_printf("~~~~~> %lld gate: %lld\n", standard_deviation, thiz->standard_deviation_gate);
			thiz->standard_deviation = standard_deviation;
		} else if (debug_pcrfix_count > MAX_DEBUG_PCRFIX_COUNT) {
			gx_printf("=====> %lld, apts-pcr: %d, vpts-pcr: %d, vpts-apts: %d\n",
					standard_deviation, apts_ms - pcr_ms, vpts_ms - pcr_ms, vpts_ms - apts_ms);
			debug_pcrfix_count = 0;
		} else if (((standard_deviation > thiz->standard_deviation_gate) &&
					(thiz->standard_deviation < thiz->standard_deviation_gate)) ||
				((standard_deviation < thiz->standard_deviation_gate) &&
				 (thiz->standard_deviation > thiz->standard_deviation_gate))) {
			thiz->standard_deviation = 0;
		}
	}

	return ((standard_deviation > thiz->standard_deviation_gate) ? 1 : 0);
}

unsigned pcrfixer_get(struct pcr_fixer *thiz, int stc, int pcr)
{
	int min_pts = 0, offset = 0, new_fix_val = 0;
	int no_fix_pcr = 0, fix_cnt = 0;
	unsigned char vaild = pcr&1;
	pcrfixer_check(thiz, stc);

	if (!thiz || !thiz->enable)
		return pcr;

	no_fix_pcr = pcr;
	if (!thiz->apts || !thiz->vpts || thiz->apts_err || thiz->vpts_err)
		goto EXIT;

	if (pcrfixer_check_gate(thiz, (unsigned int)pcr)) {
		pcr = GX_MIN(thiz->apts, thiz->vpts);
		fix_cnt = 1;
		goto EXIT;
	}

	min_pts = GX_MIN(thiz->apts, thiz->vpts);
	offset  = min_pts - thiz->dst_offset - pcr;
	thiz->sum_offset += offset;
	thiz->cnt++;
	if (thiz->cnt == thiz->cache_size) {
		thiz->sum_offset /= 2;
		thiz->cnt        /= 2;
	}
	new_fix_val = thiz->sum_offset/thiz->cnt;
	if (thiz->last_fix_val == -1)
		thiz->last_fix_val = new_fix_val;
	else if (abs(new_fix_val - thiz->last_fix_val) > thiz->tolerance) {
		thiz->last_fix_val = new_fix_val;
	}
	fix_cnt = 2;
	pcr = pcr + thiz->last_fix_val;

EXIT:
	if (thiz->apts_err) {
		fix_cnt = 3;
		pcr = thiz->vpts;
	}
	else if (thiz->vpts_err) {
		fix_cnt = 4;
		pcr = thiz->apts;
	}
	else if (thiz->apts != 0 && thiz->vpts != 0) {
		if (thiz->apts > pcr && thiz->vpts > pcr) {
			fix_cnt = 5;
			pcr = GX_MIN(thiz->apts, thiz->vpts) - thiz->sync_low_tolerance;
		}
		else if (thiz->apts < pcr && thiz->vpts < pcr) {
			fix_cnt = 6;
			pcr = GX_MAX(thiz->apts, thiz->vpts) + thiz->sync_low_tolerance;
		}
	}

	pcr &= ~1;
	pcr |= vaild;

	if (debug_pcrfix == DEBUG_PCRFIX_FIX) {
		debug_pcrfix_count++;
		if (debug_pcrfix_count > MAX_DEBUG_PCRFIX_COUNT) {
			gx_printf("fix: %d (err: %d, %d), (av: %u %u offset: %d), (pcr: %u %u %d)\n", \
					fix_cnt, thiz->apts_err, thiz->vpts_err, \
					(unsigned)thiz->apts, (unsigned)thiz->vpts, thiz->vpts-thiz->apts,\
					(unsigned)pcr, (unsigned)no_fix_pcr, pcr - no_fix_pcr);
			debug_pcrfix_count = 0;
		}
	}

	thiz->last_apts = thiz->apts;
	thiz->last_vpts = thiz->vpts;

	return (unsigned)pcr;
}
