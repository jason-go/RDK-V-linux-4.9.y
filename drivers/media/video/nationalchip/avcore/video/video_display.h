#ifndef __GX_VIDEO_DISP_H__
#define __GX_VIDEO_DISP_H__

/* Includes --------------------------------------------------------------- */
#include "video_userdata.h"
#include "mb_filter.h"
#include "video.h"
#include "frame.h"

/* Exported Macros -------------------------------------------------------- */

extern struct disp_ops video_disp_ops;

#define MAX_DISP 2


typedef enum {
	DISP_RUNNING = 0,
	DISP_STOPED,
	DISP_PAUSED,
	DISP_READY,
}DispState;

typedef struct {
	volatile unsigned int word0;
	volatile unsigned int word1;
	volatile unsigned int word2;
	volatile unsigned int word3;
	volatile unsigned int word4;
	volatile unsigned int word5;
	volatile unsigned int word6;
	volatile unsigned int word7;
}DispBuf;

#if 0
typedef struct disp_ctrl {
#define MAX_DISPBUF_NUM (8)//limited by VPU module
	int          buf_to_use;
	int          last_playmode;
	DispBuf     *dispbuf[MAX_DISPBUF_NUM];
} DispControler;
#endif

typedef enum disp_mode {
	DISP_MODE_FIELD            = (0<<1),
	DISP_MODE_FRAME            = (1<<1),
	DISP_MODE_Y_FRAME_UV_FIELD = (1<<1)|(1<<2),
	DISP_MODE_AUTO,
} DispMode;

struct gxav_video_disp {
	int                 id;
	int used;

	unsigned int        need_drop_firstint_after_reset;
	int                 hw_vpu_show_enable;
	int                 hd_init;
	struct {
		unsigned long long     pts;
		unsigned long long     last_pts;
		VideoFrameInfo require_clr;
		VideoFrameInfo showing;
		VideoFrameInfo to_disp;
	}sync_disp;

	DispState         state, state_shadow;
	int               no_frame_cnt;

	unsigned int      update;
	unsigned int      update_timeout;

	int freeze_en;
	unsigned freeze_w, freeze_h;
	volatile enum {
		FZ_NO_REQUIRE = 0,
		FZ_REQUIRED,
		FZ_READY,
		FZ_FREEZED,
		FZ_UNFREEZE_REQUIRED,
	}freeze_state;
	struct frame_buffer freeze_fb;
	VideoFrameInfo freeze_frame;

	struct mb_filter  msc_filter;

	unsigned int    zoom_param_update_require;
	int             pp_enable;
	int             top_first;
	unsigned int    fps;

	int             cc_enable;
	struct gxfifo   cc_fifo;

	unsigned long long play_frame_cnt;

	int             bypass;
	int             disp_num;
	volatile int    last_dispbuf_id;
	VideoFrameInfo  last_frame;
	int last_same_flag;
};

enum {
	DISP_CONFIG_UPDATE,
	DISP_CONFIG_FREEZE_FRAME,
};

struct disp_ops
{
	void *(*open)( int sub);
	int  (*close)( struct gxav_video_module *module );
	int  (*run)( struct gxav_video_module *module );
	int  (*stop)( struct gxav_video_module *module );
	int  (*pause)(struct  gxav_video_module *module );
	int  (*resume)( struct gxav_video_module *module );
	int  (*config) ( struct gxav_video_module *module, unsigned int cmd, void *config, unsigned int size);
	int  (*get_pp_up)( struct gxav_video_module *module, unsigned int *up);
	int  (*get_state)( struct gxav_video_module *module, int *state);
	int  (*mask)( struct gxav_video_module *module );
	int  (*unmask)(struct  gxav_video_module *module );
	int  (*reset_hw)(struct  gxav_video_module *module );
	int  (*get_frame_stat)(struct  gxav_video_module *module, struct gxav_frame_stat* stat);
};

int  videodisp_vpu_isr(struct gxav_video_module *module, void (*callback)(int id));
int  videodisp_get_showing_frame(struct gxav_video_module *module, struct cap_frame *frame);
int  videodisp_get_frame_cnt(struct gxav_video_module *module, unsigned long long *frame_cnt);
int  videodisp_push_cc(int id, CcPerFrame *frame_cc);
int  videodisp_bypass(int id, int bypass);
void videodisp_zoom_param_update_require(struct gxav_video_module *module);
#endif
