#ifndef _H_VD_H_
#define _H_VD_H_

#include "pts_fetcher.h"
#include "gxav_common.h"
#include "gxav_video_propertytypes.h"
#include "chip_media/VpuApi.h"
#include "aspect_ratio.h"
#include "frame_buf.h"

#define H_VD_DEBUG_DECODING   (0)
#define H_VD_DEBUG_VPTS_FIND  (0)
#define MAX_DECODER           (2)

#define USE_NEW_PTSFETCHER    (1)

#define H_VD_BUG

#define VIDEODEC_WORK_BUF_SIZE (CM_WORK_BUFFER_SIZE)

extern struct h_vd_ops vd_ops;

#define DECODER_FIRST_FRAME_DISP_INDEX (0xFF)

#define DISP_MIN      (1)
#define MAX_PP_FRAME  (3)

#define SIZE_ALIGN(val, align) ((val+align-1)&~(align-1))

#define SD_MAX_DEC_FRAME         (28)///< AVC REF 16, REC 1 , FULL DUP 4
#define HD_MAX_DEC_FRAME         (8)
#define FHD_MAX_DEC_FRAME        (6)

#define MAX_SHOW_FRAME           (4)
#define MAX_DEC_FRAME            (SD_MAX_DEC_FRAME+MAX_SHOW_FRAME)

#define MAX_FRAME                (MAX_PP_FRAME+MAX_DEC_FRAME)
#define FRAME_MAX_X              1920
#define FRAME_MAX_Y              1088

#define MIN_FRAME                (4)
#define FRAME_MIN_X              (720)
#define FRAME_MIN_Y              (576)

typedef enum {
	CODEC_STD_MPEG2V = 1,
	CODEC_STD_MPEG4V,
	CODEC_STD_H264,
	CODEC_STD_H263,
	CODEC_STD_DIV3,
	CODEC_STD_AVSV,
	CODEC_STD_RV,
	CODEC_STD_RV_BITSTREAM,
	CODEC_STD_VC1,
	CODEC_STD_JMPG,
	CODEC_STD_MAX
}decoder_type;

typedef enum {
	FRAME_RATE_NONE     = 0,
	FRAME_RATE_MIN      = 10000,
	FRAME_RATE_15		= 15000,
	FRAME_RATE_18		= 18000,
	FRAME_RATE_22		= 22000,
	FRAME_RATE_23976    = 23976,
	FRAME_RATE_24       = 24000,
	FRAME_RATE_25       = 25000,
	FRAME_RATE_29397    = 29397,
	FRAME_RATE_2997     = 29970,
	FRAME_RATE_301      = 30100,
	FRAME_RATE_30       = 30000,
	FRAME_RATE_50       = 50000,
	FRAME_RATE_5994     = 59940,
	FRAME_RATE_60       = 60000,
	FRAME_RATE_RESERVE  = 0xFFFF
}frame_rate;

typedef enum {
	SCAN_PROGRESSIVE,
	SCAN_INTERLACE,
	SCAN_NONE
}scan_type;

typedef enum {
	STORE_NONE  = 1<<0,
	STORE_FRAME = 1<<1,
	STORE_SINGLE_FIELD_T = 1<<2,
	STORE_SINGLE_FIELD_B = 1<<3,
	STORE_FIELD = STORE_SINGLE_FIELD_T|STORE_SINGLE_FIELD_B,
}StoreMode;

typedef enum {
	NONE_IGNORE = 1,
	BP_IGNORE      ,
	B_IGNORE
}frame_ignore;

typedef enum {
	DECODE_FAST = 0,
	DECODE_SLOW    ,
	DECODE_ONE_STOP,
	DECODE_NORMAL  ,
}decode_mode;

typedef enum { 
	I_FRAME = 0,
	P_FRAME = 1,
	B_FRAME = 2,
}VPicType;

typedef enum  int_type {
	VD_SEQ_EMPTY    = (1<<0),
	VD_SEQ_OVER     = (1<<1),
	VD_SEQ_OVER_EX  = (1<<2),
	VD_DECODE_EMPTY = (1<<3),
	VD_DECODE_OVER  = (1<<4),
	VD_SEQ_CHANGED  = (1<<5)
}h_vd_int_type;

typedef enum {
	DEC_PROCESS_START         = 0,
	DEC_PROCESS_FINDED_HEADER = 1,
	DEC_PROCESS_SYNC_FRAME    = 2,
	DEC_PROCESS_RUNNING       = 3,
}VHeaderState;

typedef GxVcodecState   DecState;
typedef GxVcodecErrCode DecErrCode;

struct head_info
{
	int          frame_size;
	int          min_framebuf_num;
	int          colbuf_size;
	unsigned     bpp;
	int          width;
	int          height;
	GxAvRect     clip;
	frame_rate   fps;
	int          fpsok;
	scan_type    type;
	DispAspectRatio ratio;
	int          ready;
	int          ready_frame_out;

	StoreMode    store_mode;
	int          movie_mode;
	int          terrible_pts;
	GxAvRational sar, dar;
	int          stride;
	int          base_height;
	int          w_align, h_align;
	int          no_dec_fps;
};

struct h_vd_cap
{
	decoder_type    format;
	unsigned int    min_width;
	unsigned int    max_width;
	unsigned int    min_height;
	unsigned int    max_height;
	frame_rate      frame_rate;
	unsigned int    workbuf_size;
};


struct frame_buffer {
	FrameBuffer virt, phys;
};

struct h_vd_dec_info
{
	StoreMode           store_mode;
	scan_type           type;
	frame_rate	        rate;
	int                 index_frame_disp;
	int                 index_frame_deced;
	VPicType            pic_type;
	int                 top_field_first;
	unsigned int        num_of_err_mb;

	unsigned            bpp;
	int                 width;
	int                 height;
	GxAvRect            clip;
	DispAspectRatio     dar;

	unsigned int        pts;
	struct {
		unsigned int enable;
		unsigned int data;
		unsigned int display;
	}userdata;

	unsigned cost;
	int colorimetry;
	int repeat_first_field;
	struct frame_buffer frame_info;
};

typedef int  (*outcallback)(void *data);
struct h_vd_video_fifo
{
	int             bs_id;
	unsigned int    bitstream_buf_addr;
	unsigned int    bitstream_buf_size;

	int             pts_id;
	unsigned int    pts_buf_addr;
	unsigned int    pts_buf_size;

	outcallback    *call;
	void           **outdata;

	void           *channel;
};

struct h_vd_config_param
{
	struct h_vd_video_fifo *fifo;
};

typedef enum {
	FB_MASK_REQUIRE,
	FB_MASKED,
	FB_UNMASK_REQUIRE,
	FB_UNMASKED,
	FB_NORMAL_USEING,
}FreezeFBState;

struct h_vd_ops
{
	void* (*h_vd_create)(unsigned int id);
	void  (*h_vd_delete)(void *vd);
	int   (*h_vd_start)(void *vd);
	int   (*h_vd_stop)(void *vd, int timeout);
	void  (*h_vd_pause)(void *vd);
	void  (*h_vd_resume)(void *vd );
	int   (*h_vd_config)(void *vd,unsigned int cmd,void *config, unsigned int size);
	void  (*h_vd_set_format)(void *vd, decoder_type format);
	void  (*h_vd_start_find_header)(void *vd);
	int   (*h_vd_get_capability)(void *vd, struct h_vd_cap *cap);
	int   (*h_vd_decode_one_frame)(void *vd);
	int   (*h_vd_clr_frame)(void *vd,int clr_frame);
	int   (*h_vd_config_frame_buf)(void *vd);
	void  (*h_vd_bs_flush)(void *vd);
	int   (*h_vd_update_data)(void *vd);
	int   (*h_vd_down_data)(void *vd);
	int   (*h_vd_get_head_info)(void *vd, struct  head_info *info);
	int   (*h_vd_get_frame_info)(void *vd, struct h_vd_dec_info *dec_info);
	int   (*h_vd_get_int_type)(void *vd);
	int   (*interrupts)(void *vd,h_vd_int_type *int_type,struct h_vd_dec_info *dec_info );
	void  (*h_vd_get_state)(void *vd, DecState *state, DecErrCode *err_code);
	void  (*h_vd_get_afd)(void *vd, void *afd);
	void  (*h_vd_get_seqstate)(void *vd, int *state);
	int   (*h_vd_callback)(void* data);
	void  (*h_vd_int_mask)(void *vd);
	void  (*h_vd_int_unmask)(void *vd );
	void  (*h_vd_fb_mask_require)(void *vd,   int fb_id);
	void  (*h_vd_fb_unmask_require)(void *vd, int fb_id);
	void  (*h_vd_set_fb_mask_state)(void *vd, FreezeFBState state);
	int   (*h_vd_get_framesize)(void *vd, int *frame_size);
	int   (*check_skip_able)(void *vd);
	int   (*h_vd_userdata_read)(void *vd, unsigned char *buf, int count);
	FreezeFBState (*h_vd_get_fb_mask_state)(void *vd);
};

#endif


