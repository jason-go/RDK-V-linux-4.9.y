#ifndef __GX_VIDEO_PP_H__
#define __GX_VIDEO_PP_H__

/* Includes --------------------------------------------------------------- */
#include "video.h"
#include "ptsfixer.h"
#include "video_sync.h"
#include "frame_convert.h"
#include "frame.h"
#include "mb_filter.h"

/* Exported Macros -------------------------------------------------------- */
#define DE_INTERLACE_Y        (1<<0)
#define DE_INTERLACE_UV       (1<<1)
#define MODULE_DE_INTERLACE   (DE_INTERLACE_Y)

extern struct pp_ops video_pp_ops;

#define MAX_PP  (1)
#define GET_VIDEO_PP(module) ((struct gxav_video_pp*)(module)->priv)

struct gxav_video_pp {
	int id;
	int used;

	struct frame_buffer fb[MAX_PP_FRAME];
	unsigned int fb_num;
	unsigned int fb_mask;
	unsigned int fb_flag;
	unsigned int fb_width, fb_height;

	volatile enum {
		PP_STATUS_BUSY = (1<<0),
		PP_STATUS_IDLE = (1<<1),
	}status;

	volatile enum {
		PP_STATE_STOP    = (1<<0),
		PP_STATE_RUNNING = (1<<1),
		PP_STATE_PAUSE   = (1<<2),
		PP_STATE_READY   = (1<<3),
	}state, state_shadow;

	enum {
		FIELD_TYPE_NONE   =  0,
		FIELD_TYPE_ZOOM_T = (1<<0),
		FIELD_TYPE_ZOOM_B = (1<<1),
		FIELD_TYPE_Y      = (1<<2),
		FIELD_TYPE_UV     = (1<<3),
	}action;

	unsigned int    stride;
	unsigned int    show_logo;

	int cal_fb_id;
	struct frame_info  pre_ref;
	struct frame_info  cur_ref;
	struct frame_info  next_ref;
	struct frame_info  pre_cal;
	struct frame_info  cur_cal;

	VideoFrameInfo input_frame;
	VideoFrameInfo process_frame;
	VideoFrameInfo output_frame;

	struct {
		unsigned int vpu_capability;
		unsigned int h_ratio;
		unsigned int v_ratio;
	}zoom;

	unsigned int deinterlace_en;
	struct {
		unsigned int max_width;
		unsigned int max_height;
		unsigned int denoise_en;

		//for top_first changed
		unsigned int last_top_first;
		unsigned int last_top_fb_id;
		unsigned int last_bottom_fb_id;
	}deinterlace;

	struct {
		unsigned int cnt;
		unsigned int gate;
	}drop_filter;

	GxAvRect        clip_rect;
	GxAvRect        view_rect;
	unsigned int    clip_rect_change;
	unsigned int    zoom_param_update_require;
	unsigned int    no_frame_timeout;

	unsigned int last_stream_id;
	unsigned int last_fps;
	unsigned int last_frame_width;
	unsigned int last_frame_height;
	unsigned int frame_cnt;

	struct video_sync         *sync;
	struct frame_rate_convert *fr_con;
	unsigned int update;
	unsigned int update_timeout;

	int decfb_ref_cnt[MAX_DEC_FRAME];
	int ppfb_ref_cnt[MAX_PP_FRAME];
};

enum {
	PP_ERROR,
	PP_DEINTERLACE,
	PP_ZOOM,
	PP_BYPASS
};

enum  {
	PP_CONFIG_SHOW_LOGO,
	PP_CONFIG_VIEW,
	PP_CONFIG_UPDATE,
	PP_CONFIG_DEINTERLACE,
	PP_CONFIG_DROP_GATE,
	PP_CONFIG_DEINTERLACE_EN,
};

struct pp_ops
{
	int  (*ioremap)       (void);
	int  (*iounmap)       (void);
	void*(*open)          (int sub, struct video_sync *sync, struct frame_rate_convert *fr_con);
	int  (*close)         (struct gxav_video_module *module);
	int  (*run)           (struct gxav_video_module *module);
	int  (*stop)          (struct gxav_video_module *module);
	int  (*pause)         (struct gxav_video_module *module);
	int  (*resume)        (struct gxav_video_module *module);
	int  (*config)        (struct gxav_video_module *module, unsigned int cmd, void *config, unsigned int size );
	int  (*get_state)     (struct gxav_video_module *module, int *state);
    int  (*isr)           (struct gxav_video_module *module);
    void (*callback)      (struct gxav_video_module *module);
    void (*get_frame)     (struct gxav_video_module *module, VideoFrameInfo *frame);
    void (*clr_frame)     (struct gxav_video_module *module, VideoFrameInfo *frame);
    int  (*get_sync_stat) (struct gxav_video_module *module, unsigned long long *lose_sync_cnt, unsigned int *synced_flag);
    void (*zoom_param_update_require)(struct gxav_video_module *module);
};

void  videopp_init(struct pp_ops *ops);
int   videopp_ioremap(void);
int   videopp_iounmap(void);
int   videopp_open     (struct gxav_video_module *module, struct video_sync *sync, struct frame_rate_convert *fr_con);
int   videopp_close    (struct gxav_video_module *module);
int   videopp_run      (struct gxav_video_module *module);
int   videopp_stop     (struct gxav_video_module *module);
int   videopp_pause    (struct gxav_video_module *module);
int   videopp_resume   (struct gxav_video_module *module);
int   videopp_config   (struct gxav_video_module *module, unsigned int cmd, void *config, unsigned int size);
int   videopp_get_state(struct gxav_video_module *module, int *state);
int   videopp_isr      (struct gxav_video_module *module);
void  videopp_callback (struct gxav_video_module *module);
void  videopp_get_frame(struct gxav_video_module *module, VideoFrameInfo *frame);
void  videopp_clr_frame(struct gxav_video_module *module, VideoFrameInfo *frame);
int   videopp_get_sync_stat(struct gxav_video_module *module, unsigned long long *lose_sync_cnt, unsigned int *synced_flag);
void  videopp_zoom_param_update_require(struct gxav_video_module *module);
#endif
