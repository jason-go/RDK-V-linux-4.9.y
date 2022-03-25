#ifndef __GX_VIDEO_DEC_H__
#define __GX_VIDEO_DEC_H__

/* Includes --------------------------------------------------------------- */
#include "video.h"
#include "pp_core.h"
#include "frame.h"
#include "decode_core.h"

#include "ptsfixer.h"
#include "pts_monitor.h"
/* Exported Macros -------------------------------------------------------- */

#define VIDEO_EVENT_SYNC_EMPTY_MASK  (1<<0)
#define VIDEO_EVENT_DECOD_EMPTY_MASK (1<<1)

#define MIN_FRAME_NUM_FOR_GET_FRAMERATE	(11)
#define GET_VIDEO_DEC(module) (struct gxav_video_dec *)(module)->priv

struct gxav_video_dec {
	int                     id;
	int                     used;

	struct h_vd_ops        *ops;
	void                   *priv;

	///< 以下参数不允许动态调整,在解码器时能之前，这些参数必须设置
	int                     bufid;
	unsigned int            bitstream_buf_addr;
	unsigned int            bitstream_buf_size;

	int                     ptsbufid;
	unsigned int            ptsbufaddr;
	unsigned int            ptsbufsize;

	decoder_type            format;
	int                     fps;       ///< 暂时没用
	int                     pts_insert;
	unsigned int            restart;
	int                     last_clr_frame_t;
	int                     last_clr_frame_b;

	///< 以下允许动态调整,需要及时传递给硬件层
	frame_ignore            f_ignore;
	decode_mode             dec_mode;

	DecState   state;
	DecState   state_shadow;

	int                     update;
	struct head_info        h_info;

	unsigned int frame_convert;
	unsigned int convert_rate;

	unsigned int play_bypass;
	unsigned int vdec_lp;
	unsigned int frame_cnt;

	FixerInfo  pts_fixer;
	PtsMonitor pts_monitor;

	struct mb_filter msc_filter;
	VideoFrameInfo output_frame;
	unsigned int stream_id;
};

struct video_ops
{
	void* (*open)          ( int sub );
	int   (*close)         ( struct gxav_video_module *module );
	int   (*probe)         ( struct gxav_video_module *module );
	int   (*config)        ( struct gxav_video_module *module, unsigned int cmd, void *config, unsigned int size);
	int   (*run)           ( struct gxav_video_module *module );
	int   (*pause)         ( struct gxav_video_module *module );
	int   (*resume)        ( struct gxav_video_module *module );
	int   (*stop)          ( struct gxav_video_module *module, int timeout);
	int   (*get_state)     ( struct gxav_video_module *module, DecState *state, DecErrCode *err_code);
	int   (*get_cap)       ( struct gxav_video_module *module, struct h_vd_cap  *cap);
	int   (*get_frameinfo) ( struct gxav_video_module *module, struct head_info *frame_info);
	int   (*get_frame_num) ( struct gxav_video_module *module, unsigned *num);
	//extern
	int   (*in_callback)   ( struct gxav_video_module *module);
	int   (*get_decmode)   ( struct gxav_video_module *module, int *dec_mode);
	int   (*mask)          ( struct gxav_video_module *module);
	int   (*unmask)        ( struct gxav_video_module *module);
};

extern struct gxav_video_dec video_dec[MAX_DECODER];
extern struct video_ops      video_dec_ops;

struct gxav_video_dec *videodec_creat_instance( int sub );
int  videodec_delete_instance  (struct gxav_video_dec    *videodec);
int  videodec_close            (struct gxav_video_module *videodec);
int  videodec_boda_isr         (struct gxav_video_module *module, int irq,h_vd_int_type *int_type );
int  videodec_set_state        (struct gxav_video_module *module, DecState *state);
int  videodec_alloc_fb         (struct gxav_video_module *module, void *fb, unsigned int num, unsigned int width, unsigned int height, FbType type, unsigned buf_addr, unsigned buf_size);
int  videodec_get_framesize    (struct gxav_video_module *module, int *size);
int  videodec_fb_unmask_require(struct gxav_video_module *module, int fb_id);
int  videodec_fb_mask_require  (struct gxav_video_module *module, int fb_id);
int  videodec_get_decmode      (struct gxav_video_module *module, int *dec_mode);
int  videodec_get_afd          (struct gxav_video_module *module, void *afd);
int  videodec_read_userdata    (struct gxav_video_module *module, unsigned char *buf, int count);
void videodec_callback         (struct gxav_video_module *module);
int  videodec_get_frame_cnt    (struct gxav_video_module *module, unsigned long long *decode_frame_cnt, unsigned long long *abandon_frame_cnt, unsigned long long *error_frame_cnt);
int  videodec_get_stream_id    (struct gxav_video_module *module, unsigned *stream_id);

#endif /*__GX_VIDEO_DEC_H__*/
