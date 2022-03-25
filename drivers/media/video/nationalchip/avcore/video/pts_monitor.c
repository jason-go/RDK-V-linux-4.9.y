#include "pts_monitor.h"
#include "kernelcalls.h"
#include "stc_hal.h"

#define NEQ_PERCENT (5)
#define ALMOST_EQ(a, b) (abs(a-b) <= (a)*(NEQ_PERCENT)/100)

static void cn_init(CntNode *node, int window)
{
	if(node) {
		node->id = 0;
		node->window = window;
	}
}

int pm_init(PtsMonitor *thiz, int fps)
{
	GxSTCProperty_TimeResolution resolution = {0};

	if (thiz && fps) {
		memset(thiz, 0, sizeof(PtsMonitor));
		gxav_stc_get_base_resolution(0, &resolution);
		thiz->fps  = fps;
		thiz->freq = resolution.freq_HZ*1000;
		thiz->step = resolution.freq_HZ*1000/fps;
		cn_init(&thiz->linear,   5);
		cn_init(&thiz->step_eq,  5);
		cn_init(&thiz->half,     30);
		cn_init(&thiz->one_half, 5);
		thiz->inited = 1;
		//gx_printf("fps = %d, freq = %d\n", fps, resolution.freq_HZ);
	}

	return 0;
}

int pm_uninit(PtsMonitor *thiz)
{
	if (thiz)
		memset(thiz, 0, sizeof(PtsMonitor));

	return 0;
}

static void flag_update(CntNode *node, int val)
{
#define SET_BIT(flag, bit) ((flag) |=  (1<<(bit)))
#define CLR_BIT(flag, bit) ((flag) &= ~(1<<(bit)))
	if (val == 0)
		SET_BIT(node->flag, node->id);
	else
		CLR_BIT(node->flag, node->id);

	node->id = (node->id+1)%node->window;
}

static int flag_peek(CntNode *node)
{
	int i, cnt;
	int ret = 0;

	if (node && node->window) {
		cnt = 0;
		for (i = 0; i < node->window; i++)
			if ((node->flag>>i)&1)
				cnt++;

		ret = 100 - cnt*100/node->window;
	}

	return ret;
}

int pm_is_dts(PtsMonitor *thiz)
{
	int linear = (flag_peek(&thiz->linear) > 95);

	return !linear;
}

int pm_is_mov(PtsMonitor *thiz)
{
	int linear   = (flag_peek(&thiz->linear)   >  95);
	int step_eq  = (flag_peek(&thiz->step_eq)  >= 80);
	int one_half = (flag_peek(&thiz->one_half) >  0);
	int half     = (flag_peek(&thiz->half)     >  0);

	return linear && !step_eq && one_half && !half;
	//return linear && !step_eq && one_half;
}

int pm_is_nor(PtsMonitor *thiz)
{
	int linear  = (flag_peek(&thiz->linear) > 95);
	int step_eq = (flag_peek(&thiz->step_eq) >= 80);

	return linear && step_eq;
}

int pm_is_wave(PtsMonitor *thiz)
{
	int linear   = (flag_peek(&thiz->linear)   > 95);
	int step_eq  = (flag_peek(&thiz->step_eq)  > 50);
	int one_half = (flag_peek(&thiz->one_half) >  0);
	int half     = (flag_peek(&thiz->half)     >  0);

	return linear && !step_eq && !one_half && !half;
}

int pm_run(PtsMonitor *thiz, int pts, int fps)
{
	int step;

	if (!thiz->inited || fps != thiz->fps)
		pm_init(thiz, fps);

	if (thiz->inited && pts && thiz->last_pts != 0) {
		step = pts - thiz->last_pts;
		flag_update(&thiz->linear, step > 0);
		flag_update(&thiz->step_eq,  (ALMOST_EQ(thiz->step, step)));
		flag_update(&thiz->one_half, (ALMOST_EQ(thiz->step*3/2, step)));
		flag_update(&thiz->half,     (ALMOST_EQ(thiz->step*1/2, step)));
	}
#if 0
	int linear   = (flag_peek(&thiz->linear));
	int step_eq  = (flag_peek(&thiz->step_eq));
	int one_half = (flag_peek(&thiz->one_half));
	int     half = (flag_peek(&thiz->half));
	gx_printf("step = %d, l = %d, eq = %d, one_hf = %d, half = %d\n", step, linear, step_eq, one_half, half);
#endif

	thiz->last_pts = pts;
	return 0;
}

