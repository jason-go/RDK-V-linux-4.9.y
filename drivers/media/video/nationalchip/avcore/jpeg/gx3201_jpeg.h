/*****************************************************************************
 * 						   CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2006, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	gx3201_jpeg.h
 * Author    : 	lvjh
 * Project   :	GoXceed
 * Type      :	Driver
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 0.0  	2011.12.28	      lvjh           creation
 *****************************************************************************/
#ifndef __GX3201_JPEG_H__
#define __GX3201_JPEG_H__

/* Includes --------------------------------------------------------------- */
#include "gxav_common.h"
#include "kernelcalls.h"
#include "avcore.h"
#include "gxav_module_propertytypes.h"
#include "gxav_jpeg_propertytypes.h"

#ifdef GX_JPEG_DEBUG
#define JPEG_DBG(fmt, args...)   do{\
	gx_printf("\n[JPEG][%s():%d]: ", __func__, __LINE__);\
	gx_printf(fmt, ##args);\
}while(0)
#else
#define JPEG_DBG(fmt, args...)   ((void)0)
#endif
#define JPEG_DBG1(fmt, args...)   do{\
	gx_printf("\n[JPEG][%s():%d]: ", __func__, __LINE__);\
	gx_printf(fmt, ##args);\
}while(0)


#define JPEG_FB_DISP_BUF_COUNT_MAX (8)
struct jpeg_head_info{
	GxColorFormat       src_color;
	unsigned int        src_width;
	unsigned int        src_height;
	unsigned int        src_base_width;
	GxAvRect            dst_rect;
};

struct jpeg_frame_info{
	GxColorFormat       color;
	unsigned int        buf_addr;
	int                 id;
	int             pts;
	unsigned int        width;
	unsigned int        height;
};

struct fb_addr{
	unsigned int fb_y_addr;
	unsigned int fb_cb_addr;
	unsigned int fb_cr_addr;
};

struct disp_addr{
	unsigned int addr;
	unsigned int size;
};

struct jpeg_fb_addr{
	struct fb_addr fb;
	int id;
};

struct jpeg_disp_addr{
	struct disp_addr disp;
	int id;
};

typedef struct
{
	int front, rear, max_size, count;
	int elements[JPEG_FB_DISP_BUF_COUNT_MAX];//记录frame/disp seq no.
}SeqQueue;

struct jpeg_ops {
	int (*run)(void);
	int (*stop)(void);
	int (*destroy)(void);
	int (*pause)(void);
	int (*resume)(void);
	int (*update)(void);
	int (*end)(void);
	int (*link)(struct gxav_module *module, struct fifo_info *fifo);
	int (*unlink)(struct gxav_module *module, struct fifo_info *fifo);
	int (*config)(struct gxav_module *module, GxJpegProperty_Config * property);
	int (*irq)(struct gxav_module_inode *inode);
	void *priv;
};

typedef enum
{
	INT_DEC_PIC_FINISH          = (1<<0),
	INT_DEC_SLICE_FINISH        = (1<<1),
	INT_DEC_SOF_FINISH          = (1<<2),
	INT_FRAME_BUF_ERROR         = (1<<3),
	INT_CLIP_ERROR              = (1<<4),
	INT_BS_BUF_ERROR            = (1<<5),
	INT_DEC_ERROR               = (1<<6),
	INT_LINE_CNT_GATE           = (1<<7),
	INT_BS_BUF_ALMOST_EMPTY     = (1<<8),
	INT_BS_BUF_EMPTY            = (1<<9),
}JpegIntType;

struct gxav_jpegdec {
	void         *in_surface;
	void         *out_surface;

	unsigned int frame_buffer_y_addr;
	unsigned int frame_buffer_cb_addr;
	unsigned int frame_buffer_cr_addr;
	unsigned int frame_buffer_444_cb_addr;
	unsigned int frame_buffer_444_cr_addr;
	unsigned int frame_buffer_display_addr;
	unsigned int frame_buffer_size;

	unsigned int jpeg_progressive;
	GxColorFormat src_color;
	GxColorFormat dst_color;
	unsigned int jpeg_slice_idct;
	unsigned int jpeg_origion_width;
	unsigned int jpeg_origion_height;
	unsigned int jpeg_wb_width;
	unsigned int jpeg_wb_height;
	unsigned int jpeg_disp_width;
	unsigned int jpeg_disp_height;
	unsigned int jpeg_disp_coordinate_x;
	unsigned int jpeg_disp_coordinate_y;
	unsigned int jpeg_h_zoom;
	unsigned int jpeg_v_zoom;
	unsigned int jpeg_frame_buffer_stride;
	unsigned int jpeg_frame_buffer_max_width;
	unsigned int jpeg_frame_buffer_max_height;

	signed char  channel_id;
	signed char  pts_channel_id;
	unsigned int empty_gate;
	unsigned int full_gate;

	unsigned int zoom_enable;
	unsigned int zoom_inquire;

	unsigned int dec_state;
	unsigned int dec_status;

	unsigned int mode;//0,normal mode; 1,clip mode;
	unsigned int sof_pause;//0,no pause; 1,pause on sof;
	unsigned int slice_pause;//0,no pause; 1,pause on slice;
	unsigned int resample_mode;//0, ; 1, ; 2, ; 3, ;
	unsigned int resample_manual_h;//0,off; 1,1/2 down sampling; 2,1/4 down sampling; 3,1/8 down sampling;
	unsigned int resample_manual_v;//0,off; 1,1/2 down sampling; 2,1/4 down sampling; 3,1/8 down sampling;
	unsigned int interlace;
	unsigned int surface_width;
	unsigned int surface_height;
	unsigned int jpeg_empty;
	gx_spin_lock_t                   jpeg_spin_lock;

	struct jpeg_fb_addr              fb_addr_array[JPEG_FB_DISP_BUF_COUNT_MAX];
	struct jpeg_disp_addr            disp_addr_array[JPEG_FB_DISP_BUF_COUNT_MAX];
	struct jpeg_frame_info           frame_info_array[JPEG_FB_DISP_BUF_COUNT_MAX];
	SeqQueue fb_queue;
	SeqQueue disp_queue;
	SeqQueue frame_queue;
	gx_spin_lock_t                   fb_spin_lock;
	gx_spin_lock_t                   disp_spin_lock;
	gx_sem_id                        disp_sem;
	gx_spin_lock_t                   frame_spin_lock;
	GxJpegProperty_Config            config;
	struct gxav_module               *module;
	struct gxav_channel              channel;
	unsigned int                     stop_flag;
	gx_mutex_t                       jpeg_mutex;
	struct{
		int cnt_pic;
		int cnt_sof;
		int err_frame;
		int err_clip;
		int err_bs;
		int err_decode;
	}stat;

	gx_thread_id thread_id;
	gx_thread_info thread_info;
	gx_sem_id sem_id;
	struct jpeg_ops *ops;
};

/* Functions ---------------------------------------------------------- */

int jpeg_reg_disp_buf(struct gxav_jpegdec *jpegdec, struct disp_addr addrs[], unsigned int len);
int jpeg_get_frame_info(struct gxav_jpegdec *jpegdec, struct jpeg_frame_info *info);
int jpeg_get_state(struct gxav_jpegdec *jpegdec, int *state);
struct gxav_jpegdec *jpeg_get_cur_dec(void);
int jpeg_get_head_info(struct gxav_jpegdec *jpegdec, struct  jpeg_head_info *info);
int jpeg_disp_queue_in(struct gxav_jpegdec *jpegdec, int id);// disp clear
int jpeg_get_max_decsize(struct gxav_jpegdec *jpegdec, int *width, int *height);


#endif /*__GX3201_JPEG_H__*/


