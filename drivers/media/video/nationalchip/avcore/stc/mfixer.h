#ifndef __MFIXER_H__
#define __MFIXER_H__

#define NEW_SYNC

//#define GX_DEBUG

#ifdef GX_DEBUG
#define STC_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[STC][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)
#else
#define STC_PRINTF(fmt, args...)   ((void)0)
#endif

#ifdef NEW_SYNC

/*
 * 魔术边界
 * 当前位于low 之内，即level = -1 新的数值超过 high 共 count 次时，跳至边界外
 * 当前在边界外时，新的数组小于low 共 count 次时，跳入边界内
 */
struct magical_verge {
	int low, high;
	int level;
	int low_count, high_count;
	int max_count;
};

void mv_init(struct magical_verge *mv, int l, int h, int count);
int mv_compar(struct magical_verge *mv, int v);

struct fixer {
	int inited;
	int stc;
	int used;
	int base_pcr, valid_pcr, error_pcr, priv_valid_pcr;
	int error_count;
	int stc_offset;
	int allow_error;
	int error_gate;
	int valid_total, error_total;
	int locked, relock;
	char *name;
};

void fixer_init(struct fixer *fix);
int fixer_get(struct fixer *fix, int pcr, int stc);

struct multifix {
	struct magical_verge mv;
	struct fixer pcr_fixer, apts_fixer;
	int pcr_error_cnt;
	int pts_error_cnt;
	int pcr_error_gate;
	int pcr_error_time;
	int apts_error_gate;
	int apts_error_time;
	int last_apts;
	int stc_offset;
	int useapts;
	void (*set_stc)(int stc);
};

void mfixer_init     (struct multifix *mf, int freq);
int  mfixer_get      (struct multifix *mf, int stc, int pcr, int apts);
void mfixer_fix      (struct multifix *mf, int stc, int pcr, int apts);
void mfixer_showinfo (struct multifix *mf);
int  mfixer_getpcr   (struct multifix *mf);
int  mfixer_getapts  (struct multifix *mf);
int  mfixer_useapts  (struct multifix *mf);

#else

struct hwfix {
	unsigned int stc_last_valid;
	unsigned int offset_soft;
	unsigned int pcr_loss_cnt;
	unsigned int pcr_lock_cnt;
	unsigned int stc_minus_cnt;
	unsigned int stc_plus_cnt;
	unsigned int stc_sel;
};

#endif

#endif
