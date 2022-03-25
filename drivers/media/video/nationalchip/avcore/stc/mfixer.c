#include "kernelcalls.h"
#include "mfixer.h"
#include "gxav_common.h"

#ifdef NEW_SYNC

#define ERROR_OK_GATE       (3)
#define STC_FIX_MAX         (9000)
#define STC_FIX_MIN         (-9000)
#define AUDIO_SYNC_GATE     (2250)

#define ORIG_PCR 0
#define FIX_PCR  1


GxAvSyncParams av_sync_params;

int gxav_set_sync_params(GxAvSyncParams *sync)
{
#define NZERO_CPY(a, b) if(b != 0) a = b;

	NZERO_CPY(av_sync_params.pcr_err_gate, sync->pcr_err_gate);
	NZERO_CPY(av_sync_params.pcr_err_time, sync->pcr_err_time);
	NZERO_CPY(av_sync_params.apts_err_gate, sync->apts_err_gate);
	NZERO_CPY(av_sync_params.apts_err_time, sync->apts_err_time);
	NZERO_CPY(av_sync_params.audio_high_tolerance, sync->audio_high_tolerance);
	NZERO_CPY(av_sync_params.audio_low_tolerance, sync->audio_low_tolerance);
	NZERO_CPY(av_sync_params.stc_offset, sync->stc_offset);

	return 0;
}

void mv_init(struct magical_verge *mv, int l, int h, int count)
{
	mv->level = -1;
	mv->low = l;
	mv->high = h;
	mv->high_count = 0;
	mv->low_count = count;
	mv->max_count = count;
}

//FIXME: no test.
int mv_compar(struct magical_verge *mv, int v)
{
	int ret = 0;
	if (mv->level == -1) {
		ret = v - mv->high;
		if (v > 0) {
			mv->high_count--;
			if (mv->high_count == 0) {
				mv->level = 1;
				mv->high_count = mv->max_count;
			}
			else
				ret = -1;
		}
	}
	else if (mv->level == 1) {
		ret = v - mv->low;
		if (v < 0) {
			mv->low_count--;
			if (mv->low_count == 0) {
				mv->level = -1;
				mv->low_count = mv->max_count;
			}
			else
				ret = 1;
		}
	}

	return ret;
}

void fixer_init(struct fixer *fix)
{
	gx_memset(fix, 0, sizeof(struct fixer));
	fix->error_gate = ERROR_OK_GATE;
}

static inline int fixer_compar(struct fixer *fix, int a, int b)
{
	return abs(a - b) <= (fix->stc_offset+fix->allow_error) ? 0 : 1;
}

int fixer_get(struct fixer *fix, int pcr, int stc)
{
	if (fix->inited == 0) {
		fix->inited++;
		fix->valid_pcr = pcr;
		fix->stc = stc;
		fix->valid_total++;

		return pcr;
	}

	fix->stc_offset     = stc - fix->stc;
	fix->stc            = stc;

	fix->used           = FIX_PCR;

	fix->priv_valid_pcr = fix->valid_pcr;
	fix->valid_pcr     += fix->stc_offset;
	fix->error_pcr     += fix->stc_offset;

	fix->relock         = 0;

	if (fixer_compar(fix, pcr, fix->valid_pcr) == 0) {
		fix->valid_pcr = pcr;
		fix->error_count = 0;
		fix->used = ORIG_PCR;
		fix->valid_total++;
		fix->locked = 1;
	}
	else {
		if (pcr != fix->base_pcr) {
			if (fix->error_count > 0 && fixer_compar(fix, pcr, fix->error_pcr) != 0)
				fix->error_count = 0;

			fix->error_pcr = pcr;
			fix->error_count++;

			if (fix->error_count >= fix->error_gate) {
				fix->valid_pcr = fix->error_pcr;
				fix->error_count = 0;
				fix->used = ORIG_PCR;
				fix->relock = 1;
				fix->locked = 1;
			}
		}
		fix->error_total++;
	}

	fix->base_pcr = pcr;

	return fix->valid_pcr;
}

void mfixer_init(struct multifix *mf, int freq)
{
#define ALLOW_ERROR_OFFSET (50)
	fixer_init(&mf->pcr_fixer);
	fixer_init(&mf->apts_fixer);
	mv_init(&mf->mv, 3000, 9000, 1);

	mf->pcr_fixer.name = "PCR";
	mf->pcr_fixer.allow_error  = (ALLOW_ERROR_OFFSET*freq)/1000;
	mf->apts_fixer.name = "APTS";
	mf->apts_fixer.allow_error = (ALLOW_ERROR_OFFSET*freq)/1000;
	mf->pcr_error_cnt = 0;
	mf->pts_error_cnt = 0;
	mf->last_apts = 0;
	mf->useapts = 0;
	mf->set_stc = NULL;

	mf->stc_offset = av_sync_params.stc_offset;
	mf->pcr_error_time = av_sync_params.pcr_err_time;
	mf->pcr_error_gate = av_sync_params.pcr_err_gate;
	mf->apts_error_time = av_sync_params.apts_err_time;
	mf->apts_error_gate = av_sync_params.apts_err_gate;
}

void mfixer_fix(struct multifix *mf, int stc, int pcr, int apts)
{
	fixer_get(&mf->apts_fixer, apts, stc);
}

int mfixer_get(struct multifix *mf, int stc, int pcr, int apts)
{
	int offset;
#define PCR_ERROR(mf, stc) (abs(stc - mf->pcr_error_cnt) >= mf->pcr_error_time)
#define PTS_ERROR(mf, stc) (abs(stc - mf->pts_error_cnt) >= mf->apts_error_time)

	if(apts == mf->last_apts){
		if(mf->pts_error_cnt == 0)
			mf->pts_error_cnt = stc;

		if(PTS_ERROR(mf, stc))
			return ((pcr + mf->stc_offset)|(1));
	}
	else{
		 mf->pts_error_cnt = 0;
	}

	mf->last_apts = apts;

	pcr  = fixer_get(&mf->pcr_fixer, pcr, stc);
	apts = mf->apts_fixer.valid_pcr;

	if(mf->apts_fixer.locked == 0) {
		return ((pcr + mf->stc_offset)&(~1));
	}

	offset = apts - (pcr + mf->stc_offset);

	if(abs(offset) >= mf->pcr_error_gate){
		if(mf->pcr_error_cnt == 0)
			mf->pcr_error_cnt = stc;

		if(!PCR_ERROR(mf, stc))
			STC_PRINTF("[%d]stc:%d : apts:%d\n", (stc - mf->pcr_error_cnt)/45000, pcr, apts);
	}
	else{
		mf->pcr_error_cnt = 0;

		if(mf->apts_fixer.relock == 1 || mf->pcr_fixer.relock == 1){
			if(abs(offset) <= AUDIO_SYNC_GATE) {
				mf->stc_offset = STC_FIX_MIN;
			}else {
				mf->stc_offset = STC_FIX_MIN + offset;
			}

			if(mf->stc_offset > STC_FIX_MAX)
				mf->stc_offset = STC_FIX_MAX;
			else if(mf->stc_offset < STC_FIX_MIN)
				mf->stc_offset = STC_FIX_MIN;

			STC_PRINTF(">>>>stc offset:%d\n", mf->stc_offset);
		}
	}

	if(mf->pcr_error_cnt && PCR_ERROR(mf, stc)) {
		pcr = mf->last_apts;
		mf->stc_offset = -AUDIO_SYNC_GATE;
		mf->useapts = 1;
	}
	else {
		mf->useapts = 0;
	}

	return (pcr + mf->stc_offset) | 1;
}

void mfixer_showinfo(struct multifix *mf)
{
	int x;
	char buf[128];

#if 0
	x = abs(mf->pcr_fixer.valid_pcr - mf->pcr_fixer.priv_valid_pcr - mf->pcr_fixer.stc_offset);
	gx_sprintf(buf, " | %d", x);
	gx_printf("%s: %c %10u->%10u [%11u:%11u], %6d%s\n",
			mf->pcr_fixer.name,
			mf->pcr_fixer.used == FIX_PCR ? '*' : ' ',
			mf->pcr_fixer.base_pcr,
			mf->pcr_fixer.valid_pcr,
			abs(mf->pcr_fixer.base_pcr - mf->pcr_fixer.valid_pcr),
			mf->pcr_fixer.valid_pcr - mf->pcr_fixer.priv_valid_pcr,
			mf->pcr_fixer.stc_offset,
			x == 0 ? "" : buf
			);
#endif
#if 1
	x = abs(mf->apts_fixer.valid_pcr - mf->apts_fixer.priv_valid_pcr - mf->apts_fixer.stc_offset);
	gx_sprintf(buf, " | %d", x);
	gx_printf("%s: %c %10u->%10u [%10u:%10u], %6d%s\n",
			mf->apts_fixer.name,
			mf->apts_fixer.used == FIX_PCR ? '*' : ' ',
			mf->apts_fixer.base_pcr,
			mf->apts_fixer.valid_pcr,
			(unsigned int)abs(mf->apts_fixer.base_pcr - mf->apts_fixer.valid_pcr),
			(unsigned int)(mf->apts_fixer.valid_pcr - mf->apts_fixer.priv_valid_pcr),
			mf->apts_fixer.stc_offset,
			x == 0 ? "" : buf
			);
#endif
}

int mfixer_getpcr(struct multifix *mf)
{
	return mf->pcr_fixer.valid_pcr;
}

int mfixer_getapts(struct multifix *mf)
{
	return mf->apts_fixer.valid_pcr|1;
}

int  mfixer_useapts  (struct multifix *mf)
{
	return mf->useapts;;
}
#endif


#if 0
int data[][3] = {
	{-1185982961 ,    -1185968121,     -2136953488},
	{-1185979808 ,    -1185967500,     -2136953112},
	{-1185979808 ,    -1185968669,     -2136952734},
	{-1199308177 ,    -1199301477,     -2136952332},
	{-1199308177 ,    -1199299677,     -2136951338},
	{-1199308177 ,    -1199299675,     -2136950170},
	{-1199305012 ,    -1199297877,     -2136949000},
	{-1199305012 ,    -1199297875,     -2136947826},
	{-1199305012 ,    -1199296077,     -2136946652},
	{-1199301848 ,    -1199296075,     -2136945480},
	{-1199301848 ,    -1199296075,     -2136944324},
	{-1199301848 ,    -1199294277,     -2136944014},
	{-1199301848 ,    -1199294277,     -2136943698},
	{0,0,0},
};

int main(int argc, char **argv)
{
	int i = 0, x;
	struct multifix fix;

	mfixer_init(&fix);

	for (i=0; data[i][0] != 0; i++) {
		int x = mfixer_get(&fix, data[i][2], data[i][0], data[i][1]);
		mfixer_showinfo(&fix);
	}
	gx_printf("%d\n", i);
	gx_printf("%s : %6d %6d\n", fix.pcr_fixer.name, fix.pcr_fixer.valid_total, fix.pcr_fixer.error_total);
	gx_printf("%s: %6d %6d\n", fix.apts_fixer.name, fix.apts_fixer.valid_total, fix.apts_fixer.error_total);

	return 0;
}

#endif
