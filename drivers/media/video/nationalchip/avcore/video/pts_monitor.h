#ifndef  __PTS_MONITOR_H__
#define  __PTS_MONITOR_H__

typedef struct {
	int id;
	int window;
	unsigned char flag;
} CntNode;

typedef struct {
	int inited;
	int fps;
	int freq;
	int pts;
	int last_pts;
	int step;

	CntNode linear, step_eq, half, one_half;
} PtsMonitor;

int pm_uninit (PtsMonitor *thiz);
int pm_init   (PtsMonitor *thiz, int fps);
int pm_is_dts (PtsMonitor *thiz);
int pm_is_mov (PtsMonitor *thiz);
int pm_is_nor (PtsMonitor *thiz);
int pm_run    (PtsMonitor *thiz, int pts, int fps);
int pm_is_wave(PtsMonitor *thiz);

#endif
