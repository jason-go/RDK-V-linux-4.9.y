#ifndef __GX_VIDEO_H__
#define __GX_VIDEO_H__

/* Includes --------------------------------------------------------------- */
#include "decode_core.h"
#include "video_sync.h"
#include "fifo.h"
#include "vpu_hal.h"
#include "kernelcalls.h"

#include "v_debug.h"

#ifdef GX_DEBUG
//#define GX_VIDEODEC_DEBUG
#define GX_VIDEODEC_PRINTF
#endif

#ifdef GX_VIDEODEC_PRINTF
#define VIDEODEC_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[VIDEO_DEC][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)
#else
#define VIDEODEC_PRINTF(fmt, args...)   ((void)0)
#endif

#ifdef GX_VIDEODEC_DEBUG
#define VIDEODEC_DBG(fmt, args...) \
	do { \
		gx_printf("\n[VIDEO_DEC][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)
#else
#define VIDEODEC_DBG(fmt, args...)   ((void)0)
#endif

#define GX_VIDEO_CHECK_RET(ret) \
	do { \
		if(ret < 0) {\
			gx_printf("%s[%d]:%s,####Error####\n", __FILE__, __LINE__, __FUNCTION__);\
			return ret;    \
		}\
	}while(0)

#define ASSERT_PARAM_SIZE_CHECK(type,size)    \
	do{                    \
		if(sizeof(type) != size) {    \
			VIDEODEC_PRINTF("cmd size check error !\n"); \
			return -1;        \
		}            \
	}while(0);
/* Exported Macros -------------------------------------------------------- */


//#define DBG_SWITCH_PROGRAM (1)
#ifdef DBG_SWITCH_PROGRAM
extern unsigned long long head_start, head_end;
extern unsigned long long decode_first;
extern unsigned long long display_first;
#endif

#define  MAX_VIDEO        2

#define VPU_FRAME_MEM_SIZE  (FRAME_MAX_X*FRAME_MAX_Y*3/2)

#define GET_VIDEO_ID(maj_id,min_id) (min_id+(maj_id<<16))
#define GET_VIDEO_MAJ_ID(id) ((id>>16)&0xffff)

enum  module_support{
	VIDEO_DECODER = (0),
	JPEG_DECODER  = (1)
};

enum sub_module {
	VIDEO_DEC      = (1<<0),
	VIDEO_PP       = (1<<1),
	VIDEO_DISP     = (1<<2)
};

enum {
	VIDEO_CONFIG_PARAM,
	VIDEO_CONFIG_FORMAT,
	VIDEO_CONFIG_SYNC,
	VIDEO_CONFIG_FRAME_IGNORE,
	VIDEO_CONFIG_DEC_MODE,
	VIDEO_CONFIG_UPDATE,
	VIDEO_CONFIG_SHOW_LOGO,
	VIDEO_CONFIG_FPS,
	VIDEO_CONFIG_DEINTERLACE,
	VIDEO_CONFIG_PTS_REORDER,
	VIDEO_CONFIG_USERDATA_PARAM,
	VIDEO_CONFIG_FREEZE_FRAME,
	VIDEO_CONFIG_COLBUF,
	VIDEO_CONFIG_WORKBUF,
	VIDEO_CONFIG_FRAMEBUF,
	VIDEO_CONFIG_DROP_FRAME_GATE,
	VIDEO_CONFIG_MOSAIC_FREEZE_GATE,
	VIDEO_CONFIG_MOSAIC_DROP_GATE,
	VIDEO_CONFIG_DEINTERLACE_EN,
	VIDEO_CONFIG_PLAYBYPASS,
	VIDEO_CONFIG_FPSSOURCE,
};

struct gxav_video_module {
	int    id;
	void  *pre;
	struct gxfifo *in;
	struct gxfifo *out;
	struct gxfifo *back_from;
	struct gxfifo *back_to;
	void  *ops;
	void  *priv;
};

typedef enum {
	VSTATE_STOPPED,
	VSTATE_READY,
	VSTATE_RUNNING,
}VideoState;

struct gxav_video {
	int id;
	int used;
	struct gxav_video_module *dec;
	struct gxav_video_module *pp;
	struct gxav_video_module *disp;

	int                     bs_bufid;
	unsigned int            bitstream_buf_addr;
	unsigned int            bitstream_buf_size;
	int                     pts_bufid;
	unsigned int            pts_buf_addr;
	unsigned int            pts_buf_size;

	void *channel;
	GxLayerID	layer_select;

#define PPOPEN_REQUIRE  (1<<0)
#define PPOPEN_ENBALE   (1<<1)
	int  ppopen;

	struct head_info  h_info;

#define FR_REQUIRE  (1<<0)
#define FR_ENBALE   (1<<1)
	int fr_transform;

	unsigned int frame_convert;
	unsigned int convert_rate;

#define SHOW_ENABLE  (1<<0)
#define SHOW_REQU    (1<<1)
	unsigned int logo;

	unsigned int redecode;

	DecState   state_shadow;
	DecState   state;
	DecErrCode err_code;

	volatile VideoState vstate;

	unsigned int over_flow;
};

int  gx_video_get_chipid      (void);
int  gx_video_init            (void *arg);
int  gx_video_cleanup         (void);
int  gx_video_close           (int id);
int  gx_video_open            (int id);
int  gx_video_fifo_link       (int id, struct h_vd_video_fifo *fifo);
int  gx_video_fifo_unlink     (int id, struct h_vd_video_fifo *fifo);
int  gx_video_config          (int id, unsigned int cmd,void *config, unsigned int size);
int  gx_video_run             (int id);
int  gx_video_probe           (int id);
int  gx_video_stop            (int id);
int  gx_video_pause           (int id);
int  gx_video_resume          (int id);
int  gx_video_mask_interrupt  (int id, unsigned int module_mask);
int  gx_video_unmask_interrupt(int id, unsigned int module_unmask);
int  gx_video_skip            (int id, unsigned int num);
int  gx_video_set_decmode     (int id, int speed);
int  gx_video_set_frame_ignore(int id, unsigned int mode);
int  gx_video_refresh         (int id);
int  gx_video_update          (int id);
int  gx_video_get_frame_info  (int id, struct head_info *info);
int  gx_video_get_frame_stat  (int id, struct gxav_frame_stat *stat);
int  gx_video_get_pts         (int id, unsigned int *pts);
int  gx_video_get_state       (int id, DecState *state, DecErrCode *err_code);
int  gx_video_set_ptsoffset   (int id, int offset_ms);
int  gx_video_set_tolerance   (int id, int value);
int  gx_video_get_pp_up       (int id, unsigned int *up);
int  gx_video_get_cap         (int id, struct h_vd_cap *cap);
int  gx_video_disp_patch      (int id);
int  gx_video_pp_zoom_enable  (int id, unsigned int force, GxAvRect *clip, GxAvRect *view);
int  gx_video_ppopen_enable   (int id);
int  gx_video_dec_interrupt   (int id, int irq,h_vd_int_type *int_type);
int  gx_video_pp_interrupt    (int id, int irq);
int  gx_video_vpu_interrupt   (int id, int irq);
int  gx_video_get_speed       (int id, int *speed);
int  gx_video_ppopen_require  (int id);
int  gx_video_ppclose_require (int id);

int  gx_video_get_cliprect(GxAvRect *rect);
int  gx_video_set_cliprect(GxAvRect *rect);
int  gx_video_get_viewrect(GxAvRect *rect);
int  gx_video_set_stream_ratio(unsigned int ratio);
int  gx_video_zoom_require(int id);
int  gx_video_zoom(int id, unsigned int force);
int  gx_video_frame_rate_transform_enable(int id, int pp_enable);
int  gx_video_frame_rate_transform_require(int id);


struct cap_frame {
	unsigned int width;
	unsigned int height;
	unsigned int buf_addr;
	unsigned int buf_size;
};

int gx_video_disp_patch(int id);
int gx_video_pp_zoom(int id);
unsigned int video_sync_get_frame_dis(struct video_sync *sync);
int gx_video_cap_frame(int id, struct cap_frame *frame);
int videodisp_get_showing_frame(struct gxav_video_module *module, struct cap_frame *frame);

int gx_video_get_userdata(int id, GxVideoDecoderProperty_UserData *user_data);

#endif /*__GX_VIDEO_DEC_H__*/


