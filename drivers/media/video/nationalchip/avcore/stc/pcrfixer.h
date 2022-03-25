#ifndef __PCR_FIXER_H__
#define __PCR_FIXER_H__

struct pcr_fixer {
	unsigned int apts, vpts;
	int enable;
	int tolerance;
	int dst_offset;
	int cnt;
	int cache_size;
	int sum_offset;
	int fix_val, last_fix_val;
	int last_apts, last_vpts;
	int apts_err, vpts_err;
	int apts_err_time, vpts_err_time;
	int sync_low_tolerance, sync_high_tolerance;
	int pts_error_time_gate;
	unsigned long long standard_deviation_gate;
	unsigned long long standard_deviation;
	unsigned int convert_time_factor;
};

extern void pcrfixer_init(struct pcr_fixer *thiz, int freq);
extern unsigned pcrfixer_get(struct pcr_fixer *thiz, int stc, int pcr);

#endif
