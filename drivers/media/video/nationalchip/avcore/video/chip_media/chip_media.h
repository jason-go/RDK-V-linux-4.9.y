#ifndef _CHIP_MEDIA_H_
#define _CHIP_MEDIA_H_

#include "kernelcalls.h"
#include "VpuApi.h"
#include "RegDefine.h"

#include "video.h"
#include "decode_core.h"
#include "wh_filter.h"
#include "aspect_ratio.h"

#define MIXER_LAYER_DISP                1

struct FRAME_BUF{
	int  Format;

	int  Index;
	int  AddrY;
	int  AddrCb;
	int  AddrCr;
	int  StrideY;
	int  StrideC;

	int  DispY;
	int  DispCb;
	int  DispCr;

	int  UserDataBuf;
	int  MvColBuf;
};

struct sync_pts_arch {
	unsigned int loop;
	unsigned int addr;
};


typedef enum {
	CHIP_MEDIA_RUNNING = (1<<0),
	CHIP_MEDIA_IDLE    = (1<<1),
} CmState;

struct vd_chip_media {
	unsigned int   inited;
	unsigned int   id;
	unsigned int   chip_id;
	DecHandle      handle;

	VHeaderState   seqstatus;
	unsigned       dmx_fps;
	struct head_info h_info;

	DecState   state;
	DecState   state_shadow;;
	DecErrCode err_code;

	CmState chip_media_state;

	int video_dec_restart;

#define DEC_STOP_REQUIRE  (1<<0)
#define DEC_STOP_ENABLE   (1<<1)
	volatile int     stop_enable;

	int av_sync;            ///< 是否同步
	int first_pts;          ///< 是否找到第一个包含有合法pts的帧
	int vpts_reorder;

	int             bitstream_buf_id;
	unsigned int    bitstream_buf_addr;
	unsigned int    bitstream_buf_size;
	unsigned int    last_rd_prt;         ///< 解码器读指针
	unsigned int    last_exact_rd_prt;   ///< 解码器内部的精确读指针

	int             pts_buf_id;
	unsigned int    pts_buf_addr;
	unsigned int    pts_buf_size;
	unsigned int    pts_rd_index;

	unsigned int    last_pts;

	DecInitialInfo  initial_info;
	DecOutputInfo   output_info;
	int last_h264PicStruct;
	StoreMode last_store_mode;

	int hevc_10bit_en;

	unsigned int regist_fb_num;
	struct frame_buffer fb[MAX_DEC_FRAME];
	FrameBuffer regist_fb[MAX_DEC_FRAME];

	struct frame_filter_info {
		unsigned int  err_mbs;
		VPicType      pic_type;
		///< hight 16bits is top frame type flag;while low is bottom flag;
		#define TOP_FRAME_TYPE(FLAG)   ((FLAG>>16)&0xFFFF)
		unsigned int  abandon;
		unsigned int  pts;
		unsigned int  cost;

		scan_type     type;
		StoreMode     store_mode;
		int           index_frame_decod;
		int           interlacedFrame;

		int           top_field_first;
		int           colorimetry;
		int           repeat_first_field;
		DispAspectRatio  ratio;
		int           userdata_seg_num;
	}ftype_filter[MAX_DEC_FRAME];
	int           top_field_first;

	DecOpenParam     open_parame;
	DecBufInfo       dec_buf_info;
	PhysicalAddress  vir_workbuf;
	PhysicalAddress  phy_workbuf;

	unsigned int     fb_public_colbuf_size;
	PhysicalAddress  fb_public_colbuf;

	FreezeFBState	 freeze_fb_state;
	int              mask_fb_id;

	int              sec_axi_en;

	int  frame_cnt;
	int  data_request;
	enum {
		NO_REQUIRE = 0,
		REQUIRED,
		FLUSHED,
	} flush_state;

#define CACULATE_FRAMES (40)
	unsigned int   first_decode_gate;

	///< f_ignore暂时无用
	frame_ignore f_ignore;
	decode_mode  dec_mode;
	frame_ignore cur_frame_dec_mode;

	unsigned int stop_header;

	///< 是否继续重新找头
	unsigned int refind_header;

	/*  查找第一个I frame和查找第一个带有pts的帧的门限,
		目前因为pic_type获取仍然有问题，所以在这部分做了特殊处理
	 */
	unsigned int abandon_frame_gate;
#define ABANDON_FRAME_GATE_SEARCH_PTS       (30*1)
#define ABANDON_FRAME_GATE_SEARCH_I_FRAME   (30*0)

	unsigned int fw_start;
	unsigned int reserve_data;

	int fb_is_ready;

	struct {
		unsigned int enable;
		unsigned int data_length;
		unsigned int display;
		unsigned int buf_addr;
		struct gxfifo *fifo;
	}userdata;

	unsigned int hungry_times;
	struct {
		unsigned char enable;
		unsigned char active_format;
	}AFD;
	WhFilter   wh_filter;
	PtsFetcher pts_fetcher;

	int search_first_i_frame;
	volatile unsigned fb_busy_flag;
	gx_spin_lock_t spin_lock;
	int disp_first_iframe;
};


int chip_media_get_dar(int bit_format,int aspect_ratio_info,int w,int h);

void chip_media_int_enable(int type);

void chip_media_int_disable(int type);

void chip_media_clrint(int type);

int chip_media_get_inttype(void );

int chip_media_get_intenable(void);

int chip_media_get_seqinitret( void );

int chip_media_reg_ioremap( void );

int chip_media_reg_iounmap( void );

#endif
