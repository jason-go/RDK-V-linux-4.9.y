#include "decode_core.h"
#include "video_userdata.h"
#include "mem_manage.h"
#include "video.h"
#include "chip_media/chip_media.h"
#include "chip_media/VpuApi.h"
#include "chip_media/VpuApiFunc.h"
#include "sdc_hal.h"
#include "gxav_bitops.h"
#include "kernelcalls.h"
#include "gxav_vout_propertytypes.h"
#include "gxav_stc_propertytypes.h"
#include "clock_hal.h"
#include "stc_hal.h"
#include "chip_media/BitCode.h"
#include "profile.h"
#include "firewall.h"
#include "secure_hal.h"

#define DEFAULT_FPS          (FRAME_RATE_2997)
#define IS_VALID_FPS(fps)    (fps >= FRAME_RATE_MIN && fps <= FRAME_RATE_60)
#define IS_VALID_PTSBUF()    (chip_media->pts_buf_id && chip_media->pts_buf_addr && chip_media->pts_buf_size)

static int coder_type[] = {
	0,
	STD_MPEG2,
	STD_MPEG4,
	STD_AVC,
	STD_MPEG4,
	STD_DIV3,
	STD_AVS,
	STD_RV,
	STD_RV,
	STD_HEVC,
	STD_MJPG
};

static struct colbuf_desc {
	int std;
	int public_flag;
	GxAvRational ratio;
} colbuf_table[] = {
	{.std = STD_RV   , .public_flag = 1, .ratio = {.num = 1, .den =  4}},
	{.std = STD_AVS  , .public_flag = 1, .ratio = {.num = 1, .den =  4}},
	{.std = STD_MPEG4, .public_flag = 1, .ratio = {.num = 1, .den =  4}},
	{.std = STD_DIV3 , .public_flag = 1, .ratio = {.num = 1, .den =  4}},
	{.std = STD_AVC  , .public_flag = 0, .ratio = {.num = 1, .den =  4}},
	{.std = STD_HEVC , .public_flag = 0, .ratio = {.num = 1, .den = 16}},
	{.std = STD_MPEG2, .public_flag = 1, .ratio = {.num = 0, .den =  1}},
};

static struct {
	unsigned key;
	unsigned val;
} fps_dict[] = {
	{ .key = 150, .val = FRAME_RATE_15    },
	{ .key = 239, .val = FRAME_RATE_23976 },
	{ .key = 240, .val = FRAME_RATE_24    },
	{ .key = 250, .val = FRAME_RATE_25    },
	{ .key = 293, .val = FRAME_RATE_29397 },
	{ .key = 299, .val = FRAME_RATE_2997  },
	{ .key = 300, .val = FRAME_RATE_30    },
	{ .key = 301, .val = FRAME_RATE_301   },
	{ .key = 500, .val = FRAME_RATE_50    },
	{ .key = 599, .val = FRAME_RATE_5994  },
	{ .key = 600, .val = FRAME_RATE_60    },
};

static void h_vd_set_head_info_rate(void *vd,DecOutputInfo *output_info);
static int  h_vd_set_top_first(void *vd,DecOutputInfo *output_info);

static struct vd_chip_media g_chip_media[MAX_DECODER];
static int decode_pts[MAX_DEC_FRAME];

#define DECODER_PUT_REQUEST(chip_media) \
	do { \
		chip_media->data_request++;\
		chip_media->hungry_times++;\
	}while(0)
#define DECODER_GET_REQUEST(chip_media)  \
	do { \
		chip_media->data_request--; \
	}while(0)

#define DECODER_CHECK_REQUEST(chip_media)  (chip_media->data_request)

static int chip_support_format(int fmt, int ext_bit_depth)
{
	int ret = 1;

	if (fmt != YUV420 || ext_bit_depth > 2)
		ret = 0;

	return ret;
}

static unsigned get_fps(unsigned int key)
{
	int i;
	unsigned fps = FRAME_RATE_NONE;

	for (i = 0; i < sizeof(fps_dict)/sizeof(fps_dict[0]); i++)
		if (fps_dict[i].key == key) {
			fps = fps_dict[i].val;
			break;
		}

	return fps;
}
static int chip_support_hevc(int ext_bit_depth)
{
	int ret = 0, flag = 0;

	if (ext_bit_depth > 2)
		ret = 0;
	else {
		flag = gxav_secure_get_hevc_status();
		if (flag >= 0)//能读到opt控制信息
			ret = (ext_bit_depth > 0) ? flag&GXAV_HEVC_SUPPORT_10BIT : flag&GXAV_HEVC_SUPPORT_8BIT;
		else if (CHIP_IS_GX3211)
			ret = (ext_bit_depth > 0) ? 0 : 1;
		else
			ret = 0;
	}

	return ret;
}

static int get_scan_type(void *vd, DecOutputInfo *output_info)
{
	struct vd_chip_media *chip_media = vd;

	if(chip_media->h_info.type == SCAN_NONE) {
		chip_media->h_info.ready = 1;

		if(chip_media->open_parame.bitstreamFormat==STD_AVC || chip_media->open_parame.bitstreamFormat==STD_HEVC) {
			/* backup && fix */
			if(chip_media->open_parame.bitstreamFormat == STD_AVC) {
				if(output_info->h264PicStruct != 15)
					chip_media->last_h264PicStruct = output_info->h264PicStruct;
				else
					output_info->h264PicStruct = chip_media->last_h264PicStruct;
			}

			if(output_info->h264PicStruct==0)
				return SCAN_PROGRESSIVE;
			else if(output_info->h264PicStruct==15)
				return output_info->interlacedFrame;
			else
				return SCAN_INTERLACE;
		}
		else {
			return (output_info->progressiveFrame==1) ?SCAN_PROGRESSIVE:SCAN_INTERLACE;
		}
	}
	else {
		return chip_media->h_info.type;
	}
}

static int get_store_mode(void *vd, DecOutputInfo *output_info)
{
	StoreMode mode;
	struct vd_chip_media *chip_media = vd;

	if(chip_media->open_parame.bitstreamFormat != STD_HEVC)
		mode = STORE_FRAME;
	else {
		switch(output_info->h264PicStruct) {
			case 1:
			case 9:
			case 11:
				mode = STORE_SINGLE_FIELD_T;
				break;
			case 2:
			case 10:
			case 12:
				mode = STORE_SINGLE_FIELD_B;
				break;
			case 13:
			case 14:
			case 15:
				if(chip_media->last_store_mode == STORE_NONE || chip_media->last_store_mode == STORE_FRAME)
					mode = STORE_FRAME;
				else if(chip_media->last_store_mode == STORE_SINGLE_FIELD_T)
					mode = STORE_SINGLE_FIELD_B;
				else
					mode = STORE_SINGLE_FIELD_T;
				break;
			default:
				mode = STORE_FRAME;
				break;
		}
	}
	chip_media->last_store_mode = mode;

	return mode;
}

static int get_pic_type(void *vd, DecOutputInfo *output_info)
{
	int type = 0;
	struct vd_chip_media *chip_media = vd;
	unsigned int interlaced = get_scan_type(vd, output_info);
	unsigned int cod_format = chip_media->open_parame.bitstreamFormat;

	if (interlaced == SCAN_PROGRESSIVE)
		type = (output_info->picType&0x7) + (((output_info->picType)&0x7)<<16);
	else {
		if (cod_format == STD_MPEG2) {
			if(output_info->pictureStructure == 3)
				type = (output_info->picType&0x7) + (((output_info->picType)&0x7)<<16);
			else
				type = (output_info->picType&0x7) + (((output_info->picTypeFirst)&0x7)<<16);
		} else {
			if(output_info->interlacedFrame == 1)
				type = (output_info->picType&0x7) + (((output_info->picTypeFirst)&0x7)<<16);
			else
				type = (output_info->picType&0x7) + (((output_info->picType)&0x7)<<16);
		}
	}

	return TOP_FRAME_TYPE(type);
}

static int get_frame_rate(void *vd, DecOutputInfo *output_info)
{
	unsigned int fps = 0, frame_rate = 0;
	struct vd_chip_media *chip_media = vd;
	unsigned int cod_format = chip_media->open_parame.bitstreamFormat;

	if (output_info->picFrateNr != -1 && output_info->picFrateDr!= -1 &&
			output_info->picFrateNr != 0 && output_info->picFrateDr != 0)
		frame_rate = output_info->picFrateNr*10/output_info->picFrateDr;
	if (cod_format == STD_AVC)
		frame_rate = frame_rate>>1;
	fps = get_fps(frame_rate);

	return fps;
}
int h_vd_userdata_read(void *vd, unsigned char *buf, int count)
{
	struct vd_chip_media *chip_media = vd;

	if (chip_media->userdata.fifo == NULL)
		return 0;

	return userdata_proc_read(buf, count, chip_media->userdata.fifo);
}

int h_vd_userdata_alloc_buf(void *vd)
{
	struct vd_chip_media *chip_media = vd;

	if(chip_media->userdata.enable) {
		//fix data_len and addr, 8 Byte align
		chip_media->userdata.data_length = (chip_media->userdata.data_length+7)/8*8;
		chip_media->userdata.buf_addr = (unsigned int)gx_dma_malloc(chip_media->userdata.data_length);
		if(!chip_media->userdata.buf_addr) {
			gx_printf("%s:%d, malloc userdata buf failed!\n", __func__, __LINE__);
			return -1;
		}

		if (chip_media->userdata.fifo == NULL)
			chip_media->userdata.fifo = gx_mallocz(sizeof(struct gxfifo));

		gxfifo_init (chip_media->userdata.fifo, NULL, 16*1024);
		gxfifo_reset(chip_media->userdata.fifo);
	}

	return 0;
}

int h_vd_userdata_free_buf(void *vd)
{
	struct vd_chip_media *chip_media = vd;

	if(chip_media->userdata.enable && chip_media->userdata.buf_addr) {
		gx_dma_free((void*)chip_media->userdata.buf_addr, chip_media->userdata.data_length);
		if (chip_media->userdata.fifo) {
			gxfifo_free(chip_media->userdata.fifo);
			gx_free(chip_media->userdata.fifo);
			chip_media->userdata.fifo = NULL;
		}
	}

	return 0;
}

void* h_vd_create(unsigned int id)
{
	struct vd_chip_media *chip_media = NULL;

	if(id < MAX_DECODER)
		chip_media = &g_chip_media[id];
	else
		return NULL;
	chip_media->id = id;

	if(chip_media->id > MAX_DECODER) {
		VIDEODEC_PRINTF("there is no decoder hardware left ,please check to close \n");
		return NULL;
	}
	gx_memset(chip_media, 0, sizeof(struct vd_chip_media));

#ifdef _linux_
	ret = chip_media_reg_ioremap( );
	if( ret!=0 ){
		VIDEODEC_PRINTF( " error " );
		return NULL;
	}
#endif

	chip_media->state = VIDEODEC_STOPED;

	///< init sps buffer
	chip_media->inited    = 0;
	chip_media->av_sync   = 0;
	chip_media->first_pts = 0;
	chip_media->f_ignore  = NONE_IGNORE;
	chip_media->dec_mode  = DECODE_NORMAL;
	chip_media->state     = VIDEODEC_STOPED;
	gx_memset( &chip_media->open_parame, 0, sizeof(DecOpenParam) );
	chip_media->open_parame.reorderEnable    = 1;
	chip_media->open_parame.outloopDbkEnable = 0;
	chip_media->chip_id = gxcore_chip_probe();
	gx_spin_lock_init(&(chip_media->spin_lock));
	bitcode_peek_all();

	return chip_media;
}

#define FPS_ERR_GATE   (3)
static struct t_fps_prober {
	unsigned int get_ok_fps;
	unsigned int ok_fps;
	unsigned int err_fps;
	unsigned int frame_cnt;
	unsigned int stc_freq;
	unsigned int start_pts;
	unsigned int last_pts;
	unsigned int fps_confict;

	unsigned pts_fps_diff;
	unsigned dec_fps;
	unsigned dmx_fps;
	unsigned pts_fps;
	unsigned dec_fpsok;
	unsigned fps_src;
} fps_prober;

#define DISTANCE(a, b)\
	(a > b ? a-b : b-a)
static unsigned int fps_probe(struct t_fps_prober *thiz, unsigned pts, int repeat_first_field, int no_dec_fps)
{
	int select_pts_fps = no_dec_fps || (thiz && thiz->fps_src == FROM_PTS);
	GxSTCProperty_TimeResolution  resolution = {0};

	if(thiz->get_ok_fps == 0) {
		gxav_stc_get_base_resolution(0, &resolution);
		thiz->stc_freq = resolution.freq_HZ;
		thiz->frame_cnt += 2;
		if (repeat_first_field)
			thiz->frame_cnt += 1;

		thiz->ok_fps = thiz->dec_fps;
		if(pts > 1) {
			if(!thiz->start_pts || (thiz->last_pts && pts <= thiz->last_pts)) {
				thiz->start_pts = pts;
				thiz->last_pts  = pts;
				thiz->frame_cnt = 0;
			}
			thiz->last_pts = pts;

			if(pts > thiz->start_pts && thiz->frame_cnt >= 20) {
				thiz->pts_fps = thiz->frame_cnt*thiz->stc_freq*1000/(pts - thiz->start_pts)/2;
				if(DISTANCE(thiz->pts_fps, thiz->dec_fps) < 500) {
					thiz->ok_fps = thiz->dec_fps;
					thiz->get_ok_fps = 1;
				}
				else if(DISTANCE(thiz->pts_fps, thiz->dmx_fps) < 500) {
					thiz->ok_fps = thiz->dmx_fps;
					thiz->get_ok_fps = 1;
				}
				else if(DISTANCE(thiz->dec_fps, thiz->dmx_fps) < 500) {
					thiz->ok_fps = thiz->dec_fps;
					thiz->get_ok_fps = 1;
				}
				else if(DISTANCE(thiz->pts_fps, thiz->err_fps) < 500) {
					if(select_pts_fps && IS_VALID_FPS(thiz->err_fps))
						thiz->ok_fps = thiz->err_fps;
					else
						thiz->ok_fps = thiz->dec_fps;

					thiz->get_ok_fps  = 1;
					thiz->fps_confict = 1;
				}
				else {
					if(++thiz->pts_fps_diff > 1 && fps_prober.dec_fpsok && IS_VALID_FPS(thiz->dec_fps)) {
						thiz->get_ok_fps = 1;
						thiz->ok_fps = thiz->dec_fps;
						gx_printf("\nCan not get fps by pts!\n");
					} else {
						thiz->err_fps = thiz->pts_fps;
					}
				}

				thiz->start_pts = pts;
				thiz->frame_cnt = 0;
			}
		}
	}

	return thiz->ok_fps;
}

static void fps_prober_init(void)
{
	gx_memset(&fps_prober, 0, sizeof(fps_prober));
	fps_prober.start_pts = 0;
	fps_prober.stc_freq  = 2000;
}

int h_vd_start(void *vd)
{
	int codec_backup;
	unsigned int version = 0;
	RetCode ret = 0;
	DecoderWorkbufInfo workbuf_info;
	RingBufInfo esbuf_info, ptsbuf_info;
	struct vd_chip_media *chip_media = vd;

	if (chip_media->phy_workbuf == 0) {
		ret = mm_workbuf_info_get(&workbuf_info);
		if (ret == 0) {
			chip_media->phy_workbuf = workbuf_info.addr;
			chip_media->vir_workbuf = (PhysicalAddress)gx_phys_to_virt(workbuf_info.addr);
		}
		else {
			gx_printf("\n config worbuf failed!\n");
			return -1;
		}
	}

	codec_backup = chip_media->open_parame.bitstreamFormat;
	if (gxav_firewall_buffer_protected(GXAV_BUFFER_VIDEO_FIRMWARE))
		chip_media->open_parame.bitstreamFormat = STD_ALL;
	ret = VPU_Init(chip_media->phy_workbuf, chip_media->vir_workbuf, &(chip_media->open_parame));
	if(ret != 0) {
		VIDEODEC_PRINTF("chip_media init fail ret=%x \n",ret);
		return -1;
	}
	chip_media->inited = 1;
	chip_media->open_parame.bitstreamFormat = codec_backup;

	ret = VPU_DecOpen((DecHandle *)&(chip_media->handle),&(chip_media->open_parame));
	if(ret != 0) {
		VIDEODEC_PRINTF("chip_media open failed,ret=%d \n",ret);
		return -1;
	}

	VPU_GetVersionInfo(&version);
	VIDEODEC_PRINTF("C&M firware version is %x \n",version);

	chip_media->seqstatus          = DEC_PROCESS_START;
	chip_media->state              = VIDEODEC_RUNNING;
	chip_media->state_shadow       = chip_media->state;
	chip_media->err_code           = VIDEODEC_ERR_NONE;
	chip_media->stop_enable        = 0;
	chip_media->video_dec_restart  = 0;
	chip_media->hungry_times       = 0;

	chip_media->fb_is_ready        = 0;
	chip_media->pts_rd_index       = 0;
	chip_media->first_decode_gate  = 2*1024;
	chip_media->search_first_i_frame = 1;
	memset(decode_pts, 0xFF, sizeof(decode_pts));
	chip_media->chip_media_state   = CHIP_MEDIA_IDLE;
	chip_media->data_request       = 0;
	chip_media->stop_header        = 0;
	chip_media->refind_header      = 0;
	chip_media->abandon_frame_gate = 0;

	chip_media->last_pts           = 0;
	gx_memset(chip_media->fb,    0, MAX_DEC_FRAME*sizeof(struct frame_buffer));

	if(CHIP_IS_GX3113C)
		chip_media->sec_axi_en = 0;
	else
		chip_media->sec_axi_en = 1;

	chip_media->fw_start = 0;
	chip_media->hevc_10bit_en = 1;

	chip_media->fb_public_colbuf = 0;
	chip_media->fb_public_colbuf_size = 0;

	chip_media->top_field_first = -1;

	chip_media->fb_busy_flag = 0;
#ifdef H_VD_BUG
	chip_media->reserve_data = 0;
#endif
	gx_memset(&chip_media->h_info, 0 ,sizeof(chip_media->h_info));

	gx_memset(chip_media->ftype_filter, 0, sizeof(chip_media->ftype_filter));

	gx_memset( &(chip_media->initial_info),-1,sizeof(DecInitialInfo) );
	gx_memset( &(chip_media->output_info),0,sizeof(DecOutputInfo) );
	chip_media->last_h264PicStruct = 15;
	chip_media->last_store_mode = STORE_FRAME;

	//pts fetcher
	esbuf_info.id    = chip_media->bitstream_buf_id;
	esbuf_info.addr  = chip_media->bitstream_buf_addr;
	esbuf_info.size  = chip_media->bitstream_buf_size;
	ptsbuf_info.id   = chip_media->pts_buf_id;
	ptsbuf_info.addr = chip_media->pts_buf_addr;
	ptsbuf_info.size = chip_media->pts_buf_size;
	ptsfetcher_init(&chip_media->pts_fetcher, &esbuf_info, &ptsbuf_info);

	if(chip_media->userdata.enable) {
		h_vd_userdata_alloc_buf(vd);
	}

	chip_media_clrint(1<<SEQ_INIT);
	chip_media_clrint(1<<PIC_RUN);
	chip_media_clrint(1<<ESBUF_EMPTY);
	chip_media_clrint(1<<SEQ_CHANGED);
	chip_media_clrint(1<<DEC_RESYNC_SEQ);
	chip_media_int_enable(((1<<SEQ_INIT)|(1<<PIC_RUN)|(1<<ESBUF_EMPTY)|(1<<SEQ_CHANGED)));
	chip_media_int_enable((1<<DEC_RESYNC_SEQ));

	return 0;
}

void h_vd_pause(void *vd )
{
	struct vd_chip_media *chip_media = vd;

	if(chip_media->state != VIDEODEC_PAUSED) {
		chip_media->state_shadow = chip_media->state;
		chip_media->state = VIDEODEC_PAUSED;
	}
}

void h_vd_resume(void *vd )
{
	struct vd_chip_media *chip_media = vd;
	chip_media->state = chip_media->state_shadow;
}

void h_vd_int_mask(void *vd )
{
	chip_media_int_disable(1<<SEQ_INIT);
	chip_media_int_disable(1<<PIC_RUN);
	chip_media_int_disable(1<<ESBUF_EMPTY);
	chip_media_int_disable(1<<DEC_RESYNC_SEQ);
}

void h_vd_int_unmask(void *vd )
{
	chip_media_int_enable(((1<<SEQ_INIT)|(1<<PIC_RUN)|(1<<14)));
	///< 在找头失败后，重启找头命令后会通过9th bit上报下一个找到头的中断
	chip_media_int_enable((1<<9));
}

int h_vd_config(void *vd,unsigned int cmd,void *config, unsigned int size)
{
	struct vd_chip_media *chip_media = vd;

	switch (cmd) {
		case VIDEO_CONFIG_PARAM:
			{
				struct h_vd_video_fifo *fifo = config;
				ASSERT_PARAM_SIZE_CHECK(struct h_vd_video_fifo, size);
				if(fifo != NULL) {
					chip_media->bitstream_buf_addr = fifo->bitstream_buf_addr;
					chip_media->bitstream_buf_size = fifo->bitstream_buf_size;
					chip_media->bitstream_buf_id   = fifo->bs_id;
					chip_media->last_rd_prt        = chip_media->bitstream_buf_addr;
					chip_media->last_exact_rd_prt  = chip_media->bitstream_buf_addr;

					chip_media->pts_buf_addr       = fifo->pts_buf_addr;
					chip_media->pts_buf_size       = fifo->pts_buf_size;
					chip_media->pts_buf_id         = fifo->pts_id;

					// init open param
					chip_media->open_parame.bitstreamBuffer     = fifo->bitstream_buf_addr;
					chip_media->open_parame.bitstreamBufferSize = fifo->bitstream_buf_size;
					return 0;
				}
				break;
			}
		case VIDEO_CONFIG_FORMAT:
			{
				decoder_type format = *(decoder_type*)config;
				if (format>=CODEC_STD_MPEG2V && format<CODEC_STD_MAX) {
					chip_media->open_parame.bitstreamFormat = coder_type[format];
					if (chip_media->open_parame.bitstreamFormat==STD_MPEG4 || chip_media->open_parame.bitstreamFormat==STD_MPEG2)
						chip_media->open_parame.outloopDbkEnable = 1;
					else
						chip_media->open_parame.outloopDbkEnable = 0;
					return 0;
				}
				break;
			}
		case VIDEO_CONFIG_SYNC:
			{
				chip_media->av_sync = *(int*)config;
				return 0;
			}
		case VIDEO_CONFIG_PTS_REORDER:
			{
				chip_media->vpts_reorder = *(int*)config;
				return 0;
			}
		case VIDEO_CONFIG_FRAME_IGNORE:
			{
				frame_ignore ignore = *(frame_ignore*)config;
				if( ignore>=NONE_IGNORE && ignore<=B_IGNORE) {
					chip_media->f_ignore = ignore;
					return 0;
				}
				break;
			}
		case VIDEO_CONFIG_DEC_MODE:
			{
				decode_mode dec_mode = *(decode_mode*)config;
				if(dec_mode>=DECODE_FAST && dec_mode<=DECODE_NORMAL) {
					chip_media->dec_mode = dec_mode;

					if(chip_media->dec_mode == DECODE_NORMAL) {
						chip_media->fw_start = 0;
					}
					return 0;
				}
				break;
			}
		case VIDEO_CONFIG_FPS:
			{
				unsigned int fps = *(unsigned int*)config * 1000;
				fps_prober_init();
				if(IS_VALID_FPS(fps)) {
					fps_prober.dmx_fps  = fps;
					chip_media->dmx_fps = fps;
				}
				break;
			}
		case VIDEO_CONFIG_USERDATA_PARAM:
			{
				gx_memcpy(&chip_media->userdata, config, size);
				break;
			}
		case VIDEO_CONFIG_FPSSOURCE:
			{
				fps_prober.fps_src = *(FpsSource*)config;
			}
		default:
			break;
	}

	chip_media->open_parame.mp4Class = 0;
	return -1;
}

///< 可以省略该函数
void  h_vd_set_format(void *vd, decoder_type format)
{
	struct vd_chip_media *chip_media = vd;

	chip_media->open_parame.bitstreamFormat  = coder_type[format];
}

void h_vd_delete(void *vd)
{
	struct vd_chip_media *chip_media = vd;

	chip_media->phy_workbuf = 0;
	chip_media->vir_workbuf = 0;
	chip_media->sec_axi_en  = 0;
	gx_memset(&(chip_media->open_parame), 0, sizeof(DecOpenParam));
	gx_memset(&(chip_media->dec_buf_info), 0, sizeof(DecBufInfo));
}

void h_vd_start_find_header(void *vd)
{
	struct vd_chip_media *chip_media = vd;

#ifdef DBG_SWITCH_PROGRAM
	head_start = gx_current_tick();
#endif
	if( !VPU_IsBusy( ) ) {
		VIDEODEC_DBG( " \n start find header ............ \n " );
		VPU_DecSetInitialOpt(chip_media->handle);
	} else {
		VIDEODEC_PRINTF( " [fatal] start finding header cmd error " );
	}
}

static int h_vd_restart_find_header(void *vd)
{
#define STOP_HEADER_TIME_OUT	(0)
	int ret = 0;
	struct vd_chip_media *chip_media = vd;

	if(!chip_media->refind_header) {
		return 0;
	}

#if(1==STOP_HEADER_TIME_OUT)
	if(chip_media->stop_header++ > 50) {
		return -1;
	}
#endif

	if(!(chip_media->stop_enable&DEC_STOP_REQUIRE)) {
		ret = VPU_DecResetRdPtr(chip_media->handle, VpuReadReg(BIT_RD_PTR_0)+512, 0);
		if( ret != RETCODE_SUCCESS )
		{
			VIDEODEC_PRINTF("VPU_DecResetRdPtr failed Error code is 0x%x \n", ret );
			return -1;
		}
		ret = VPU_DecResyncSeq(chip_media->handle);
		if( ret != RETCODE_SUCCESS )
		{
			VIDEODEC_PRINTF("VPU_DecBitBufferFlush failed Error code is 0x%x \n", ret );
			return -1;
		}
		chip_media->refind_header = 0;
	}

	return 0;
}


int h_vd_get_capability(void *vd, struct h_vd_cap *cap)
{
	int ret, codec;
	int fw_protected = 0;
	unsigned int max_width, max_height;
	struct bitcode_desc desc = {0};
	struct gx_mem_info fwmem = {0};
	DecoderWorkbufInfo workbuf_info = {0};

	if (CHIP_IS_GX3113C) {
		max_width  = 720;
		max_height = 576;
	} else {
		max_width  = 1920;
		max_height = 1920;
	}

	cap->min_width  = MIN_WIDTH,  cap->max_width  = max_width;
	cap->min_height = MIN_HEIGHT, cap->max_height = max_height;

	cap->frame_rate = (FRAME_RATE_23976 | FRAME_RATE_24 |\
			FRAME_RATE_25   | FRAME_RATE_29397 |\
			FRAME_RATE_2997 | FRAME_RATE_301   |\
			FRAME_RATE_30   | FRAME_RATE_50    |\
			FRAME_RATE_5994 | FRAME_RATE_60);

	fw_protected = gxav_firewall_buffer_protected(GXAV_BUFFER_VIDEO_FIRMWARE);
	codec = (fw_protected ? STD_ALL : coder_type[cap->format]);
	ret = bitcode_fetch(codec, &desc);
	if (ret == 0) {
		mm_init();
		if (gx_mem_info_get("vfwmem", &fwmem) == 0) {
			//loader already set vfwmem
			cap->workbuf_size = 0;
			workbuf_info.size = fwmem.size;
			workbuf_info.addr = (unsigned)gx_phys_to_virt(fwmem.start);
			mm_workbuf_info_set(&workbuf_info);
		} else if (fw_protected) {
			if (CHIP_IS_TAURUS) {
				gx_printf("\nvideo firmware is been protected, but no firmware mem prepared!\n");
				ret = -1;
			}
		} else {
			//cap->workbuf_size  = CODE_BUF_SIZE;
			cap->workbuf_size  = SINGLE_CODE_BUF_SIZE;
			cap->workbuf_size += WORK_BUF_SIZE + PARA_BUF2_SIZE + PARA_BUF_SIZE;
			if (CHIP_IS_GX3113C)
				cap->workbuf_size += AXI_BUFFER_SIZE;
#if FB_BOUND_CHECK
				cap->workbuf_size += MEM_CHECK_SIZE;
#endif
		}
	} else {
		gx_printf("\n%s:%d, fetch firmware failed\n", __func__, __LINE__);
	}

	return ret;
}

static int h_vd_get_fb_mask(void)
{
	return *(volatile unsigned*)BIT_FRM_DIS_FLG_0;
}

int h_vd_decode_one_frame(void *vd)
{
	int      ret = 0;
	DecParam decrunparam = {0};
	struct vd_chip_media *chip_media = vd;

	if (chip_media->fb_is_ready == 0)
		return -1;

	if( (VIDEODEC_RUNNING==chip_media->state)
			&& ((chip_media->stop_enable&DEC_STOP_REQUIRE) == 0)
			&& (!VPU_IsBusy()) && (chip_media->chip_media_state==CHIP_MEDIA_IDLE) ) {
		if(chip_media->f_ignore==BP_IGNORE || chip_media->search_first_i_frame) {
			chip_media->search_first_i_frame = 0;
			decrunparam.iframeSearchEnable  = 1;
			decrunparam.skipframeMode       = 0;
			decrunparam.skipframeNum        = 0;
		} else if (chip_media->f_ignore == B_IGNORE){
			decrunparam.iframeSearchEnable  = 0;
			decrunparam.skipframeMode       = 2; ///<just skip b frame
			decrunparam.skipframeNum        = 1;
		}
		chip_media->cur_frame_dec_mode  = chip_media->f_ignore;

#if FB_BOUND_CHECK
		if (mm_check_all(__func__, __LINE__) != FB_MEM_NORMAL) {
			chip_media->state    = VIDEODEC_ERROR;
			chip_media->err_code = VIDEODEC_ERR_FB_OVERFLOW;
			return -1;
		}
#endif
		ret = VPU_DecStartOneFrame(chip_media->handle, &decrunparam, chip_media);
		if(ret == RETCODE_SUCCESS) {
			VDBG_PRINT_DEC_INFO("dec one", -1, -1, h_vd_get_fb_mask());
			//chip_media->chip_media_state = CHIP_MEDIA_RUNNING;
#if (1==H_VD_DEBUG_DECODING)
			gx_printf("video start one frame %x %x\n",
					fb_mask,
					chip_media->chip_media_state);
#endif
			chip_media->video_dec_restart = 0;
			return 0;
		}
	}
	chip_media->video_dec_restart = 1;

	return -1;
}

int h_vd_clr_frame(void *vd, int clr_frame)
{
	int ret = 0;
	struct vd_chip_media *chip_media = vd;

	if( (!VPU_IsBusy()) && (chip_media->chip_media_state==CHIP_MEDIA_IDLE) ) {
		ret = VPU_DecClrDispFlag( chip_media->handle, clr_frame );
		if(ret != 0) {
			VIDEODEC_PRINTF("[fatal] clr frame error \n");
			return -1;
		}
		VDBG_PRINT_DEC_INFO("dec clr", clr_frame, -1, 0);
		chip_media->fb_busy_flag &= ~(1<<clr_frame);
	} else {
		return -1;
	}

	return 0;
}

static int is_colbuf_public(int std)
{
	int i;
	int public_flag = 0;

	for (i = 0; i < sizeof(colbuf_table)/sizeof(colbuf_table[0]); i++) {
		if (colbuf_table[i].std == std) {
			public_flag = colbuf_table[i].public_flag;
			break;
		}
	}

	return public_flag;
}

static int is_colbuf_inside(int std)
{
	int inside = 0;
	int public_flag = is_colbuf_public(std);

	if ((gxav_firewall_buffer_protected(GXAV_BUFFER_VIDEO_FRAME) == 0) && public_flag == 0)
		inside = 1;

	return inside;
}

static int h_vd_get_colbuf_size(void *vd, int *ret_each_size, int *ret_total_size)
{
	struct vd_chip_media *chip_media = vd;

	int i;
	int width  = chip_media->h_info.width;
	int height = chip_media->h_info.height;
	int std = chip_media->open_parame.bitstreamFormat;
	int fb_num, total_size = 0, each_size = 0;

	if (chip_media->h_info.store_mode == STORE_FIELD) {
		width  = SIZE_ALIGN(width,  chip_media->h_info.w_align);
		height = SIZE_ALIGN(height/2, chip_media->h_info.h_align)*2;
		fb_num = (chip_media->h_info.min_framebuf_num + DISP_MIN + 1)/2;
	} else {
		width  = SIZE_ALIGN(width,  chip_media->h_info.w_align);
		height = SIZE_ALIGN(height, chip_media->h_info.h_align);
		fb_num = (chip_media->h_info.min_framebuf_num + DISP_MIN + 1);
	}

	for (i = 0; i < sizeof(colbuf_table)/sizeof(colbuf_table[0]); i++) {
		if (colbuf_table[i].std == std) {
			each_size = width*height*colbuf_table[i].ratio.num/colbuf_table[i].ratio.den;
#if FB_BOUND_CHECK
			if (each_size)
				each_size += MEM_CHECK_SIZE;
#endif
			break;
		}
	}

	if (colbuf_table[i].public_flag == 1)
		total_size = each_size;
	else
		total_size = each_size * (fb_num + 1);

	if (is_colbuf_inside(std))
		total_size = 0;

	if (ret_each_size)  *ret_each_size  = each_size;
	if (ret_total_size) *ret_total_size = total_size;

	return 0;
}


static int calc_framesize(void *vd, int std, int width, int height, int userdata_length, int *size)
{
	int colbuf_size = 0;
	unsigned y_size = 0, uv_size = 0;
	struct vd_chip_media *chip_media = vd;

	struct fb_desc desc = {0};

	desc.type   = FB_NORMAL;
	desc.mode   = chip_media->h_info.store_mode == STORE_FIELD ? FB_FIELD : FB_FRAME;
	desc.bpp    = 8 + chip_media->initial_info.bitdepth;
	desc.align  = chip_media->h_info.w_align;
	desc.width  = chip_media->h_info.width;
	if (chip_media->h_info.store_mode == STORE_FIELD) {
		desc.height = chip_media->h_info.height/2;
	} else {
		desc.height = chip_media->h_info.height;
	}
	y_size  = calc_y_size(&desc);
	uv_size = y_size>>1;

	if (is_colbuf_inside(std))
		h_vd_get_colbuf_size(vd, &colbuf_size, NULL);

	*size  = y_size + uv_size;
	*size += SIZE_ALIGN(colbuf_size, 8);
	*size += SIZE_ALIGN(userdata_length, 8);

#if FB_BOUND_CHECK
	*size += MEM_CHECK_SIZE;
#endif
	return 0;
}

static int h_vd_get_frame_size(void *vd, int *size)
{
	int fb_size = 0;
	int width = 0, height = 0;
	int userdata_length = 0;
	struct vd_chip_media *chip_media = vd;
	int std = chip_media->open_parame.bitstreamFormat;

	if (chip_media->h_info.frame_size == 0) {
		//patch for divide one frame to two, for zoom
		if (chip_media->userdata.enable) {
			if (chip_media->h_info.store_mode == STORE_FIELD)
				userdata_length = chip_media->userdata.data_length*2;
			else
				userdata_length = chip_media->userdata.data_length;
		}
		width  = chip_media->h_info.width;
		height = chip_media->h_info.height;
		calc_framesize(vd, std, width, height, userdata_length, &fb_size);
		chip_media->h_info.frame_size = fb_size;
	}

	*size = chip_media->h_info.frame_size;

	return 0;
}

void h_vd_set_fb_mask_state(void *vd, FreezeFBState state)
{
	struct vd_chip_media *chip_media = vd;
	chip_media->freeze_fb_state = state;
}

FreezeFBState h_vd_get_fb_mask_state(void *vd)
{
	struct vd_chip_media *chip_media = vd;
	return chip_media->freeze_fb_state;
}

void h_vd_fz_mask(void *vd)
{
	struct vd_chip_media *chip_media = vd;

	if (chip_media->h_info.store_mode == STORE_FIELD) {
		VPU_DecSetDispFlag(chip_media->handle, 0);
		VPU_DecSetDispFlag(chip_media->handle, 1);
	} else {
		VPU_DecSetDispFlag(chip_media->handle, 0);
	}
	h_vd_set_fb_mask_state(vd, FB_MASKED);
}

void h_vd_fz_unmask(void *vd)
{
	struct vd_chip_media *chip_media = vd;

	if (chip_media->freeze_fb_state == FB_UNMASK_REQUIRE) {
		if (chip_media->h_info.store_mode == STORE_FIELD) {
			VPU_DecClrDispFlag(chip_media->handle, 0);
			VPU_DecClrDispFlag(chip_media->handle, 1);
		} else {
			VPU_DecClrDispFlag(chip_media->handle, 0);
		}
		h_vd_set_fb_mask_state(vd, FB_UNMASKED);
	}
}

void h_vd_fb_mask_require(void *vd, int fb_id)
{
	struct vd_chip_media *chip_media = vd;
	chip_media->freeze_fb_state = FB_MASK_REQUIRE;
}

void h_vd_fb_unmask_require(void *vd, int fb_id)
{
	struct vd_chip_media *chip_media = vd;

	if (chip_media->freeze_fb_state == FB_MASKED)
		h_vd_set_fb_mask_state(vd, FB_UNMASK_REQUIRE);
	if ((!VPU_IsBusy()) && (chip_media->chip_media_state==CHIP_MEDIA_IDLE))
		h_vd_fz_unmask(vd);
}

static int h_vd_wh_align(int codec, int wh)
{
	int block_size, align;

	if(codec == STD_HEVC) {
		block_size = 8;
	} else {
		block_size = 16;
	}

	align = block_size - 1;
	return (wh + align) & (~align);
}

int h_vd_set_head_info(void *vd)
{
	int codec;
	unsigned int scale = 0, base_height = 0;
	unsigned int frame_rate = 0;
	unsigned int disp_w, disp_h;
	struct vd_chip_media *chip_media = vd;

	gx_memset(&chip_media->h_info, 0 ,sizeof(chip_media->h_info));
	codec = chip_media->open_parame.bitstreamFormat;
	chip_media->h_info.ready  = 1;
	chip_media->h_info.bpp    = 8 + chip_media->initial_info.bitdepth;

	chip_media->h_info.width  = h_vd_wh_align(codec, chip_media->initial_info.picWidth);
	chip_media->h_info.height = h_vd_wh_align(codec, chip_media->initial_info.picHeight);
	chip_media->h_info.w_align = chip_media->initial_info.picXAign;
	chip_media->h_info.h_align = chip_media->initial_info.picYAign;
	chip_media->h_info.w_align = 64;
	chip_media->h_info.h_align = 64;

	if (chip_media->initial_info.picCropRect.right!=0 && chip_media->initial_info.picCropRect.bottom!=0) {
		chip_media->h_info.clip.x      = chip_media->initial_info.picCropRect.left;
		chip_media->h_info.clip.y      = chip_media->initial_info.picCropRect.top;
		chip_media->h_info.clip.width  = chip_media->initial_info.picCropRect.right;
		chip_media->h_info.clip.height = chip_media->initial_info.picCropRect.bottom;
	} else {
		chip_media->h_info.clip.x      = 0;
		chip_media->h_info.clip.y      = 0;
		chip_media->h_info.clip.width  = chip_media->initial_info.picWidth;
		chip_media->h_info.clip.height = chip_media->initial_info.picHeight;
	}
	chip_media->h_info.min_framebuf_num = chip_media->initial_info.minFrameBufferCount;
	gx_printf("\n[VTEST] min_fb_num = %d\n", chip_media->initial_info.minFrameBufferCount);
	if (chip_media->initial_info.hevcFieldSeqFlag) {
		chip_media->h_info.store_mode = STORE_FIELD;
		chip_media->h_info.height = (chip_media->h_info.height<<1);
	}

	chip_media->h_info.stride = SIZE_ALIGN(chip_media->initial_info.picWidth, chip_media->h_info.w_align);
	if (chip_media->h_info.store_mode == STORE_FIELD)
		chip_media->h_info.base_height = SIZE_ALIGN(chip_media->initial_info.picHeight, chip_media->h_info.h_align)*2;
	else
		chip_media->h_info.base_height = SIZE_ALIGN(chip_media->initial_info.picHeight, chip_media->h_info.h_align);

	if (is_valid_wh(chip_media->h_info.width, chip_media->h_info.height)) {
		if (chip_media->h_info.store_mode == STORE_FIELD)
			whfilter_init(&chip_media->wh_filter, chip_media->h_info.width, chip_media->h_info.height>>1);
		else
			whfilter_init(&chip_media->wh_filter, chip_media->h_info.width, chip_media->h_info.height);
	} else {
		chip_media->h_info.ready = 0;
		return -2;
	}

	if (codec == STD_AVC)
		scale = 1;
	disp_w = chip_media->initial_info.picCropRect.right != 0 ?\
			 chip_media->initial_info.picCropRect.right - chip_media->initial_info.picCropRect.left :\
			 chip_media->initial_info.picWidth;
	disp_h = chip_media->initial_info.picCropRect.bottom != 0 ?\
			 chip_media->initial_info.picCropRect.bottom - chip_media->initial_info.picCropRect.top:\
			 chip_media->initial_info.picHeight;

	if (chip_media->h_info.store_mode == STORE_FIELD)
		disp_h *= 2;
	chip_media->h_info.ratio = vd_get_dar_enum(codec, chip_media->initial_info.aspectRateInfo, disp_w, disp_h);

	if (codec == STD_MPEG2) {
		chip_media->h_info.type = (chip_media->initial_info.progressive==1) ? SCAN_PROGRESSIVE : SCAN_INTERLACE;
	} else if(codec == STD_MPEG4) {
		chip_media->h_info.type = (chip_media->initial_info.progressive==1)
			?SCAN_PROGRESSIVE:SCAN_INTERLACE;
	} else {
		chip_media->h_info.ready  = 0;
		chip_media->h_info.type = SCAN_NONE;
	}

	if (chip_media->initial_info.fRateNumerator!=-1
			&& chip_media->initial_info.fRateDenominator!=-1
			&& chip_media->initial_info.fRateDenominator!=0) {
		frame_rate = chip_media->initial_info.fRateNumerator*10/chip_media->initial_info.fRateDenominator >> scale;
	} else {
		if(codec == STD_MPEG4 || codec == STD_DIV3) {
			if(IS_VALID_FPS(chip_media->dmx_fps))
				chip_media->h_info.fps = chip_media->dmx_fps;
			else
				chip_media->h_info.ready  = 0;
			return 0;
		}
	}

	chip_media->h_info.fpsok = 1;
	switch (frame_rate)
	{
		case 239:
			chip_media->h_info.fps = FRAME_RATE_23976;
			break;
		case 240:
			chip_media->h_info.fps = FRAME_RATE_24;
			break;
		case 250:
			chip_media->h_info.fps = FRAME_RATE_25;
			break;
		case 293:
			chip_media->h_info.fps = FRAME_RATE_29397;
			break;
		case 299:
			chip_media->h_info.fps = FRAME_RATE_2997;
			break;
		case 301:
			chip_media->h_info.fps = FRAME_RATE_301;
			break;
		case 149:
		case 150:
			chip_media->h_info.fps = FRAME_RATE_15;
			break;
		case 180:
			chip_media->h_info.fps = FRAME_RATE_18;
			break;
		case 220:
			chip_media->h_info.fps = FRAME_RATE_22;
			break;
		case 300:
			chip_media->h_info.fps = FRAME_RATE_30;
			break;
		case 500:
			chip_media->h_info.fps = FRAME_RATE_50;
			break;
		case 599:
			chip_media->h_info.fps = FRAME_RATE_5994;
			break;
		case 600:
			chip_media->h_info.fps = FRAME_RATE_60;
			break;
		default:
			if(codec == STD_MPEG4 || codec == STD_DIV3) {
				if(frame_rate > 0)
					chip_media->h_info.fps = frame_rate*100;
				else {
					chip_media->h_info.ready = 0;
					chip_media->h_info.fpsok = 0;
					chip_media->h_info.no_dec_fps = 1;
				}
			}
			else if(IS_VALID_FPS(chip_media->dmx_fps)) {
				gx_printf("%s:%d, unsupport framerate, select demuxer framerate\n", __func__, __LINE__);
				chip_media->h_info.fps  = chip_media->dmx_fps;
			}
			else if(IS_VALID_FPS(frame_rate*100)) {
				chip_media->h_info.fps = frame_rate*100;
			}
			else {
				gx_printf("%s:%d, default framerate!!!\n", __func__, __LINE__);
				chip_media->h_info.fps   = DEFAULT_FPS;
				chip_media->h_info.fpsok = 0;
				chip_media->h_info.no_dec_fps = 1;
			}
			break;
	}

	return 0;
}

int h_vd_check_skip_able(void *vd)
{
	int min_fb_num = 0;
	struct vd_chip_media *chip_media = vd;

	min_fb_num = chip_media->initial_info.minFrameBufferCount + DISP_MIN;
	return (chip_media->regist_fb_num > min_fb_num);
}

int colbuf_alloc(void *vd, struct frame_buffer *fb)
{
	int i, max;
	int public_flag = 0;
	int alloced_num = 0;
	int each_size = 0, total_size = 0;
	DecoderColbufInfo buf = {0};

	struct vd_chip_media *chip_media = vd;
	int std = chip_media->open_parame.bitstreamFormat;

	public_flag = is_colbuf_public(std);

	/* get mem */
	h_vd_get_colbuf_size(vd, &each_size, &total_size);
	mm_colbuf_info_get(&buf);
	if (buf.size < total_size) {
		gx_printf("colbuf alloc failed\n");
		goto out;
	}

	/* alloc */
	if (public_flag)
		max = chip_media->regist_fb_num;
	else
		max = GX_MIN(buf.size/each_size, chip_media->regist_fb_num);
	for (i = 0; i < max; i++) {
		if (public_flag)
			fb[i].phys.bufMvCol = buf.addr;
		else
			fb[i].phys.bufMvCol = buf.addr + i*each_size;
		alloced_num++;
	}

out:
	return alloced_num;
}

int h_vd_config_frame_buf(void *vd)
{
	int	i, min_fb_num = 0;
	int frame_size = 0, free_num = 0, stride;
	int ret = RETCODE_SUCCESS ;
	int colbuf_size = 0, colbuf_inside = 0;
	int userdata_size = 0, unused_fb_id;

	int ext_fb_num = 0;
	int pp_fb_num = MAX_PP_FRAME;
	int disp_min  = DISP_MIN;

	struct fb_desc desc = {0};
	struct vd_chip_media *chip_media = vd;
	struct frame_buffer fb[MAX_DEC_FRAME];
	int std = chip_media->open_parame.bitstreamFormat;

	h_vd_get_frame_size(vd, &frame_size);
	mm_framebuf_get_freenum(frame_size, &free_num);

	ext_fb_num = pp_fb_num + 1;
	if (chip_media->h_info.store_mode == STORE_FIELD)
		min_fb_num = chip_media->initial_info.minFrameBufferCount/2 + disp_min;
	else
		min_fb_num = chip_media->initial_info.minFrameBufferCount + disp_min;

	if(free_num < min_fb_num) {
		gx_printf("\n%s:%d, framesize = %d, free_num = %d, min_fb_num = %d, fb num is too few!\n", __func__, __LINE__, frame_size, free_num, min_fb_num);
		goto ERROR;
	} else if(free_num - min_fb_num >= ext_fb_num) {
		chip_media->regist_fb_num = free_num - pp_fb_num;
	} else {
		chip_media->regist_fb_num = free_num;
	}

	if ((colbuf_inside = is_colbuf_inside(std)) == 1)
		h_vd_get_colbuf_size(vd, &colbuf_size, NULL);

	if (chip_media->userdata.enable) {
		if (chip_media->h_info.store_mode == STORE_FIELD)
			userdata_size = chip_media->userdata.data_length*2;
		else
			userdata_size = chip_media->userdata.data_length;
	}

	/* alloc Y-CB-CR */
	desc.type   = FB_NORMAL;
	desc.mode   = chip_media->h_info.store_mode == STORE_FIELD ? FB_FIELD : FB_FRAME;
	desc.num    = chip_media->regist_fb_num;
	desc.fbs    = fb;
	desc.bpp    = 8 + chip_media->initial_info.bitdepth;
	desc.align  = chip_media->h_info.w_align;
	desc.colbuf_size   = colbuf_size;
	desc.userdata_size = userdata_size;
	desc.width  = chip_media->h_info.width;
	if (chip_media->h_info.store_mode == STORE_FIELD) {
		desc.height = chip_media->h_info.height/2;
	} else {
		desc.height = chip_media->h_info.height;
	}
	chip_media->regist_fb_num = fb_alloc(&desc);
	if (chip_media->regist_fb_num < min_fb_num) {
		gx_printf("[fatal] call function videodec_frame_buffer_alloc error \n");
		goto ERROR;
	}

	/* alloc COL-MV */
	if (colbuf_inside == 0) {
		chip_media->regist_fb_num = colbuf_alloc(vd, fb);
		if (chip_media->regist_fb_num < min_fb_num) {
			gx_printf("[fatal] call function videodec_frame_buffer_alloc error \n");
			goto ERROR;
		}
	}

	/* prepare for regist fbs */
	for (i = 0; i < chip_media->regist_fb_num; i++) {
		if (chip_media->h_info.store_mode == STORE_FIELD) {
			chip_media->fb[2*i]   = fb[i];
			/*virt*/
			chip_media->fb[2*i+1].virt.bufY        = fb[i].virt.bufY  + chip_media->h_info.stride;
			chip_media->fb[2*i+1].virt.bufCb       = fb[i].virt.bufCb + chip_media->h_info.stride;
			chip_media->fb[2*i+1].virt.bufCr       = fb[i].virt.bufCr + chip_media->h_info.stride;
			chip_media->fb[2*i+1].virt.bufMvCol    = fb[i].virt.bufMvCol + colbuf_size/2;
			chip_media->fb[2*i+1].virt.bufUserData = fb[i].virt.bufUserData + userdata_size;
			/*phys*/
			chip_media->fb[2*i+1].phys.bufY        = fb[i].phys.bufY  + chip_media->h_info.stride;
			chip_media->fb[2*i+1].phys.bufCb       = fb[i].phys.bufCb + chip_media->h_info.stride;
			chip_media->fb[2*i+1].phys.bufCr       = fb[i].phys.bufCr + chip_media->h_info.stride;
			chip_media->fb[2*i+1].phys.bufMvCol    = fb[i].phys.bufMvCol + colbuf_size/2;
			chip_media->fb[2*i+1].phys.bufUserData = fb[i].phys.bufUserData + userdata_size;
		} else {
			chip_media->fb[i] = fb[i];
		}
	}

	unused_fb_id = (chip_media->h_info.store_mode == STORE_FIELD) ? chip_media->regist_fb_num*2 : chip_media->regist_fb_num;
	for (i = 0; i < MAX_DEC_FRAME; i++) {
		if (i < unused_fb_id)
			chip_media->regist_fb[i] = (FrameBuffer)chip_media->fb[i].phys;
		else
			chip_media->regist_fb[i] = (FrameBuffer)chip_media->fb[0].phys;
	}

	stride = chip_media->h_info.stride;
	if (chip_media->h_info.store_mode == STORE_FIELD) {
		chip_media->regist_fb_num *= 2;
		stride *= 2;
	}
	gx_printf("\n[VTEST] regist_fb_num = %d, size = %u\n", chip_media->regist_fb_num, frame_size);
	ret = VPU_DecRegisterFrameBuffer( chip_media->handle,
			chip_media->regist_fb,
			chip_media->regist_fb_num,
			stride,
			&chip_media->dec_buf_info);
	if (ret == RETCODE_SUCCESS) {
		chip_media->fb_is_ready = 1;
		h_vd_fz_mask(vd);
		return 0;
	}

ERROR:
	chip_media->regist_fb_num = 0;
	return -1;
}

int h_vd_bitstream_load(void *vd)
{
#define ALIGN_DATA(cnt) ((cnt)&(~0x3ff))

	int cnt = 0;
	RetCode ret = RETCODE_SUCCESS ;
	struct vd_chip_media *chip_media = vd;

	gxav_sdc_length_get(chip_media->bitstream_buf_id, (unsigned int*)&cnt);
	if(chip_media->flush_state == NO_REQUIRE) {
		if(!cnt)
			return -1;
#ifdef H_VD_BUG
		cnt = ALIGN_DATA(cnt-1) - (int)chip_media->reserve_data;
#else
		cnt = ALIGN_DATA(cnt-1);
#endif
		if(cnt < (int)chip_media->first_decode_gate)
			return -1;
		ret = VPU_DecUpdateBitstreamBuffer( chip_media->handle, cnt);
#ifdef H_VD_BUG
		chip_media->reserve_data += cnt;
#endif
	}
	else if(chip_media->flush_state == REQUIRED) {
		cnt = cnt - (int)chip_media->reserve_data;
		if(cnt >= 0) {
			ret = VPU_DecUpdateBitstreamBuffer(chip_media->handle, cnt);
			ret = VPU_DecUpdateBitstreamBuffer(chip_media->handle, 0);
			chip_media->reserve_data += cnt;
			chip_media->flush_state = FLUSHED;
		}
	}
	else if(chip_media->flush_state == FLUSHED) {
		if(chip_media->seqstatus < DEC_PROCESS_FINDED_HEADER)
			chip_media->state = VIDEODEC_OVER;
		return -1;
	}

	return 0;
}

void h_vd_bs_flush(void *vd)
{
	struct vd_chip_media *chip_media = vd;

	if(chip_media->flush_state == NO_REQUIRE) {
		chip_media->flush_state = REQUIRED;
	}
	if(DECODER_CHECK_REQUEST(chip_media)) {
		h_vd_bitstream_load(chip_media);
	}
}

static void h_vd_get_eprd(void *vd, unsigned int *end_addr, unsigned int *start_addr)
{
	int ret = 0;
	unsigned int prd,pwr,size,exact_eprd,exact_sprd;
	unsigned int dis_tmp1,dis_tmp2;
	unsigned int last_eprd_offset, eprd_offset, prd_offset;

	struct vd_chip_media *chip_media = vd;

	ret = VPU_DecGetBitstreamBuffer(chip_media->handle,&prd,&pwr,&size);
	if( ret!=0 ) {
		VIDEODEC_PRINTF( "get prd error \n" );
	}
	ret = VPU_DecReadExactRdPrt(chip_media->handle,&exact_eprd, &exact_sprd);
	if( ret!=0 ) {
		VIDEODEC_PRINTF( "get eprd error \n" );
	}

	last_eprd_offset = chip_media->last_exact_rd_prt - chip_media->bitstream_buf_addr;
	eprd_offset = exact_eprd - chip_media->bitstream_buf_addr;
	prd_offset = prd - chip_media->bitstream_buf_addr;

	dis_tmp1 = (prd_offset+chip_media->bitstream_buf_size-last_eprd_offset)
		%chip_media->open_parame.bitstreamBufferSize;
	dis_tmp2 = (prd_offset+chip_media->bitstream_buf_size-eprd_offset)
		%chip_media->open_parame.bitstreamBufferSize;
	if(dis_tmp1 < dis_tmp2) {
		//gx_printf("exact read pointer error %x %x %x %x %x %x %x %x\n",
		//		chip_media->last_exact_rd_prt,exact_eprd,prd,
		//		last_eprd_offset,eprd_offset,prd_offset,dis_tmp1,dis_tmp2);
		exact_eprd = chip_media->last_exact_rd_prt;
	}

	if(end_addr)
		*end_addr = exact_eprd;
	if(start_addr)
		*start_addr = exact_sprd;
}

static int h_vd_bitstream_unload(void *vd)
{
	int              ret       = 0;
	unsigned int     cost_cnt  = 0;
	unsigned int     cnt	   = 0;
	unsigned int     size      = 0;
	PhysicalAddress  prd       = 0;
	PhysicalAddress  m_prd     = 0;
	PhysicalAddress  pwr       = 0;
	PhysicalAddress  exact_prd = 0;
	struct vd_chip_media *chip_media = vd;

	ret = VPU_DecGetBitstreamBuffer(chip_media->handle,&prd,&pwr,&size);
	if(chip_media->last_rd_prt==chip_media->bitstream_buf_addr
			&& chip_media->last_exact_rd_prt==chip_media->bitstream_buf_addr
			&& prd==chip_media->bitstream_buf_addr) {
		m_prd = chip_media->bitstream_buf_addr;
	} else {
		m_prd = (prd-chip_media->bitstream_buf_addr+chip_media->bitstream_buf_size-512)
			%chip_media->bitstream_buf_size + chip_media->bitstream_buf_addr;
	}

	if(chip_media->seqstatus >= DEC_PROCESS_FINDED_HEADER) {
		h_vd_get_eprd(chip_media,&exact_prd, NULL);
		chip_media->last_exact_rd_prt = exact_prd;
	}

	cost_cnt = (m_prd + chip_media->bitstream_buf_size - chip_media->last_rd_prt)
		%chip_media->open_parame.bitstreamBufferSize;
	gxav_sdc_length_set(chip_media->bitstream_buf_id, cost_cnt, 0x00000000);
	chip_media->last_rd_prt = m_prd;

#ifdef H_VD_BUG
	chip_media->reserve_data -= cost_cnt;
#endif
	gxav_sdc_length_get(chip_media->bitstream_buf_id,&cnt);
	VIDEODEC_DBG("cost = %d %d %d\n",cnt,cost_cnt,chip_media->reserve_data);

	return ret;
}

static int h_vd_get_pic_size(void *vd)
{
	unsigned int size;
	unsigned int end_prd, start_prd;
	struct vd_chip_media *chip_media = vd;

	h_vd_get_eprd(chip_media, &end_prd, &start_prd);

	size = (end_prd+chip_media->open_parame.bitstreamBufferSize-start_prd)
		% chip_media->open_parame.bitstreamBufferSize;

	return size;
}

void h_vd_frames_update_abandon(void *vd)
{
	struct vd_chip_media *chip_media = vd;

	///< 判断第一帧不是I帧
	if(chip_media->seqstatus == DEC_PROCESS_FINDED_HEADER) {

		if(chip_media->ftype_filter[chip_media->output_info.indexFrameDecoded].pic_type != I_FRAME
				&& (chip_media->abandon_frame_gate++<ABANDON_FRAME_GATE_SEARCH_I_FRAME)) {
			chip_media->ftype_filter[chip_media->output_info.indexFrameDecoded].abandon = 1;
		} else {
			if(chip_media->ftype_filter[chip_media->output_info.indexFrameDecoded].pic_type != I_FRAME)
			{
				VIDEODEC_PRINTF("cost too much time to find first I frame \n");
			}
			VIDEODEC_PRINTF("finded the first I frame \n");
			chip_media->seqstatus = DEC_PROCESS_SYNC_FRAME;
		}
		return ;
	}

	if(chip_media->seqstatus == DEC_PROCESS_SYNC_FRAME) {
		if(chip_media->ftype_filter[chip_media->output_info.indexFrameDecoded].pic_type == B_FRAME) {
			chip_media->ftype_filter[chip_media->output_info.indexFrameDecoded].abandon = 1;
		} else {
			chip_media->seqstatus = DEC_PROCESS_RUNNING;
			chip_media->abandon_frame_gate = 0;
		}
		return ;
	}

	return ;
}

static int h_vd_put_pts(int pts)
{
	int i;
	for(i=0; i<MAX_DEC_FRAME; i++){
		if(decode_pts[i] == -1){
			decode_pts[i] = pts;
			return 0;
		}
	}

	return 0;
}

static int h_vd_get_pts(void)
{
	int i, j = -1;
	int pts = -1;

	for(i=0; i<MAX_DEC_FRAME; i++){
		if(decode_pts[i] != -1){
			if(pts == -1 || decode_pts[i] < pts){
				pts = decode_pts[i];
				j = i;
			}
		}
	}

	if(j != -1){
		decode_pts[j] = -1;
		return pts;
	}

	return -1;
}

int h_vd_get_frame_info(void *vd, struct h_vd_dec_info *dec_info)
{
	int i, codec;
	int ret = RETCODE_SUCCESS ;
	int dec_id, disp_id;
	unsigned int pts = 0, fps = 0;
	unsigned int interlaced = 0, store_mode = 0;
	unsigned int disp_w, disp_h;
	unsigned int size  = 0;
	PhysicalAddress prd = 0;
	PhysicalAddress pwr = 0;

	DecOutputInfo *output_info = NULL;
	struct vd_chip_media *chip_media = vd;

	codec = chip_media->open_parame.bitstreamFormat;
	gx_memset(dec_info, -1, sizeof(struct h_vd_dec_info));

	output_info = &(chip_media->output_info);
	ret = VPU_DecGetOutputInfo(chip_media->handle, output_info);
	if(ret != RETCODE_SUCCESS) {
		VIDEODEC_PRINTF("VPU_DecGetOutInfo failed Error code is 0x%x \n", ret );
		chip_media->chip_media_state = CHIP_MEDIA_IDLE;
		return -1;
	}

	dec_id  = output_info->indexFrameDecoded;
	disp_id = output_info->indexFrameDisplay;
	VDBG_PRINT_DEC_INFO("dec isr", dec_id, disp_id, 0);
	if(dec_id==-2 && disp_id==-1)
		chip_media->state = VIDEODEC_OVER;
	if(!chip_media->h_info.ready)
		h_vd_set_head_info_rate(vd, output_info);

	chip_media->chip_media_state = CHIP_MEDIA_IDLE;

	if(chip_media->stop_enable&DEC_STOP_REQUIRE) {
		chip_media->stop_enable |= DEC_STOP_ENABLE;
		VIDEODEC_DBG(" @@@@@@@@@@@@@@@@@@@@@@@@@ %x \n",chip_media->stop_enable);
		return -1;
	}
	ret = 0;
	if(disp_id==-1 || (disp_id==-3 && dec_id==-1)) {
		ret = -1;
	}
	if (disp_id >= 0) {
		int fb_mask = h_vd_get_fb_mask();
		if (((fb_mask>>disp_id) & 0x1) == 0 || chip_media->fb_busy_flag & (1<<disp_id)) {
			disp_id = -1;
			gx_printf("\ndecoder disp_idx error!\n");
		}
	}

	if(dec_id >= 0) {
		FiltRet ret_code;
		output_info->decPicWidth  = h_vd_wh_align(codec, output_info->decPicWidth);
		output_info->decPicHeight = h_vd_wh_align(codec, output_info->decPicHeight);

		if(chip_media->cur_frame_dec_mode != NONE_IGNORE) {
			ret = VPU_DecGetBitstreamBuffer(chip_media->handle,&prd,&pwr,&size);
			VPU_DecResetRdPtr(chip_media->handle, 0, prd);
			if(chip_media->fw_start)
				disp_id = dec_id;
			chip_media->fw_start = 1;
		}
		h_vd_bitstream_unload(chip_media);
		chip_media->ftype_filter[dec_id].cost = h_vd_get_pic_size(chip_media);

		if(chip_media->cur_frame_dec_mode != NONE_IGNORE) {
			chip_media->reserve_data -= (pwr+chip_media->bitstream_buf_size-prd)%chip_media->bitstream_buf_size;
		}

		if (!IS_VALID_PTSBUF()) {
			pts = 0;
		} else {
			unsigned s_addr, e_addr, offset;
			h_vd_get_eprd(chip_media, &e_addr, &s_addr);
			offset = (s_addr+1-chip_media->bitstream_buf_addr+chip_media->bitstream_buf_size)%chip_media->bitstream_buf_size;
			ptsfetcher_fetch(&chip_media->pts_fetcher, offset, &pts);

			if(chip_media->vpts_reorder)
				h_vd_put_pts(pts);
		}

		ret_code = whfilter_filt(&chip_media->wh_filter, output_info->decPicWidth, output_info->decPicHeight);
		if(ret_code == RET_SAME_AS_KEY)
			chip_media->ftype_filter[dec_id].abandon = 0;
		else if(ret_code == RET_DIFF_TO_KEY)
			chip_media->ftype_filter[dec_id].abandon = 1;
		else if(ret_code == RET_KEY_ERR) {
			chip_media->ftype_filter[dec_id].abandon = 1;
			chip_media->state    = VIDEODEC_ERROR;
			chip_media->err_code = VIDEODEC_ERR_SIZE_CHANGED;
			gx_printf("head: %dx%d, dec: %dx%d\n", chip_media->h_info.width, chip_media->h_info.height, output_info->decPicWidth, output_info->decPicHeight);
			return -1;
		}

		chip_media->ftype_filter[dec_id].pts = pts;
		chip_media->ftype_filter[dec_id].err_mbs = output_info->numOfErrMBs;
		interlaced = get_scan_type(vd, output_info);
		store_mode = get_store_mode(vd, output_info);
		fps = get_frame_rate(vd, output_info);
		if (fps && fps != chip_media->h_info.fps) {
			gx_printf("\nupdate dec fps: %d -> %d\n", chip_media->h_info.fps, fps);
			fps_prober_init();
			fps_prober.dec_fps     = fps;
			chip_media->h_info.fps = fps;
		}

		chip_media->ftype_filter[dec_id].colorimetry = output_info->hdr_param.colour_primaries;
		if (gxav_secure_get_hdr_status() == 0)
			chip_media->ftype_filter[dec_id].colorimetry = 0;
		chip_media->ftype_filter[dec_id].pic_type = get_pic_type(chip_media, output_info);
		chip_media->ftype_filter[dec_id].abandon |= !chip_media->h_info.ready;
		if (codec != STD_HEVC)
			h_vd_frames_update_abandon(vd);
		chip_media->ftype_filter[dec_id].interlacedFrame = interlaced;
		chip_media->ftype_filter[dec_id].store_mode = store_mode;
		chip_media->ftype_filter[dec_id].index_frame_decod = dec_id;
		chip_media->ftype_filter[dec_id].err_mbs = output_info->numOfErrMBs;
		chip_media->ftype_filter[dec_id].top_field_first = h_vd_set_top_first(vd, output_info);
		chip_media->ftype_filter[dec_id].repeat_first_field = output_info->repeatFirstField;

		disp_w = output_info->decPicCrop.right != 0 ? \
						  output_info->decPicCrop.right - output_info->decPicCrop.left:\
						  output_info->decPicWidth;
		disp_h = output_info->decPicCrop.bottom != 0 ? \
						  output_info->decPicCrop.bottom - output_info->decPicCrop.top:\
						  output_info->decPicHeight;
		if (chip_media->h_info.store_mode == STORE_FIELD)
			disp_h *= 2;
		chip_media->ftype_filter[dec_id].ratio = vd_get_dar_enum(codec,
			output_info->picAspectRatio, disp_w, disp_h);
		vd_get_dar_rational(codec, output_info->picAspectRatio,\
				disp_w, disp_h, &chip_media->h_info.dar);
		vd_get_sar_rational(codec, output_info->picAspectRatio,\
				disp_w, disp_h, &chip_media->h_info.sar);

		if(chip_media->userdata.enable) {
			output_info->userdataSegNum = output_info->userdataSegNum > 4 ? 4 : output_info->userdataSegNum;
			chip_media->ftype_filter[dec_id].userdata_seg_num = output_info->userdataSegNum;
			gx_memset((void*)chip_media->fb[dec_id].virt.bufUserData, 0, chip_media->userdata.data_length);
			for (i = 0; i < output_info->userdataSegNum; i++)
				userdata_copy((void*)chip_media->fb[dec_id].virt.bufUserData+i*256, (void*)chip_media->userdata.buf_addr+i*256, 256);
			gx_memset((void*)chip_media->userdata.buf_addr, 0, chip_media->userdata.data_length);
		}
	}

	if (disp_id >= 0) {
		chip_media->fb_busy_flag |= (1<<disp_id);
		dec_info->type = chip_media->ftype_filter[disp_id].interlacedFrame;
		dec_info->store_mode = chip_media->ftype_filter[disp_id].store_mode;
#ifdef CONFIG_AV_ENABLE_BFRAME_FREEZE
	extern volatile int bbt_freezed;
	if(bbt_freezed) {
		dec_info->index_frame_disp = -1;
		h_vd_clr_frame(chip_media, disp_id);
		return 0;
	}
#endif

		dec_info->colorimetry =chip_media->ftype_filter[disp_id].colorimetry;
		dec_info->cost = chip_media->ftype_filter[disp_id].cost;
		dec_info->index_frame_deced = chip_media->ftype_filter[disp_id].index_frame_decod;
		dec_info->num_of_err_mb     = chip_media->ftype_filter[disp_id].err_mbs;
		dec_info->pic_type          = chip_media->ftype_filter[disp_id].pic_type;
		dec_info->top_field_first   = chip_media->ftype_filter[disp_id].top_field_first;
		dec_info->bpp    = chip_media->h_info.bpp;
		dec_info->width  = chip_media->h_info.width;
		dec_info->height = chip_media->h_info.height;
		dec_info->repeat_first_field = chip_media->ftype_filter[disp_id].repeat_first_field;
		dec_info->dar = chip_media->ftype_filter[disp_id].ratio;
		if ( disp_id>=0 ) {
			if(codec != STD_MJPG) {
				if(!chip_media->first_pts
						&& !chip_media->ftype_filter[disp_id].abandon
						&& chip_media->av_sync) {
					if(chip_media->ftype_filter[disp_id].pts==0
							&& chip_media->abandon_frame_gate++<ABANDON_FRAME_GATE_SEARCH_PTS) {
						chip_media->ftype_filter[disp_id].abandon = 1;
					} else {
						chip_media->first_pts = 1;
					}
				}

				if (chip_media->disp_first_iframe == 0) {
					if (chip_media->ftype_filter[disp_id].pic_type == I_FRAME)
						chip_media->disp_first_iframe = 1;
					else
						chip_media->ftype_filter[disp_id].abandon = 1;
				}

				if(chip_media->ftype_filter[disp_id].abandon) {
					dec_info->index_frame_disp = -1;
					h_vd_clr_frame(chip_media, disp_id);
					VIDEODEC_DBG(" abandon frame = %x %x \n",dec_info->pic_type,
							chip_media->ftype_filter[disp_id].pts);
					return 0;
				}
			}
			dec_info->index_frame_disp = disp_id;

			if(chip_media->vpts_reorder)
				dec_info->pts = h_vd_get_pts();
			else
				dec_info->pts = chip_media->ftype_filter[disp_id].pts;

			if(chip_media->f_ignore == NONE_IGNORE && !dec_info->num_of_err_mb) {
				chip_media->h_info.fps = fps_probe(&fps_prober, dec_info->pts, dec_info->repeat_first_field, chip_media->h_info.no_dec_fps);
				if(!chip_media->h_info.fpsok && fps_prober.get_ok_fps) {
					chip_media->h_info.fpsok = 1;
				}
			}
			dec_info->frame_info = *(struct frame_buffer*)&chip_media->fb[disp_id];
			dec_info->rate       = chip_media->h_info.fpsok ? chip_media->h_info.fps : 0 ;

			dec_info->userdata.enable  = chip_media->userdata.enable;
			dec_info->userdata.display = chip_media->userdata.display;
			dec_info->userdata.data   = 0;
			dec_info->clip.x = 0;
			dec_info->clip.y = 0;
			dec_info->clip.width = chip_media->h_info.clip.width;
			dec_info->clip.height= chip_media->h_info.clip.height;
			if (chip_media->userdata.enable) {
				for (i = 0; i < chip_media->ftype_filter[disp_id].userdata_seg_num; i++) {
					unsigned payload_addr = 0, payload_len = 0;
					if (userdata_payload_get(codec,
						          (void*)chip_media->fb[disp_id].virt.bufUserData+i*256,
						          chip_media->userdata.data_length, &payload_addr, &payload_len) == 0) {
						if (userdata_payload_probe(vd, (void*)payload_addr, payload_len) == UDT_CC) {
							if (chip_media->userdata.display)
								userdata_payload_parse(vd, (void*)payload_addr, payload_len, (void*)chip_media->fb[disp_id].virt.bufUserData);
							else
								userdata_proc_write((void*)payload_addr, payload_len, dec_info->pts, chip_media->userdata.fifo);
						}
						chip_media->AFD.enable = 0;
						dec_info->userdata.data = (unsigned int)chip_media->fb[disp_id].virt.bufUserData;
					}
				}
			}
		}
	}
	return ret;
}

static int h_vd_get_head_info(void *vd, struct head_info *info)
{
	struct vd_chip_media *chip_media = vd;

	if( chip_media->initial_info.picWidth==-1 || chip_media->initial_info.picHeight==-1 ) {
		return -1;
	}

	if(chip_media->h_info.width!=0 && chip_media->h_info.height!=0) {
		gx_memcpy(info, &(chip_media->h_info), sizeof(struct head_info));
		info->fps = fps_prober.get_ok_fps ? info->fps : 0;
		if (chip_media->h_info.store_mode == STORE_FIELD)
			info->fps /= 2;
		info->bpp    = 8 + chip_media->initial_info.bitdepth;
		info->width  = chip_media->h_info.width;
		info->height = chip_media->h_info.height;

		if(chip_media->initial_info.picCropRect.right!=0
				&& chip_media->initial_info.picCropRect.bottom!=0) {
			info->clip.x      = chip_media->initial_info.picCropRect.left;
			info->clip.y      = chip_media->initial_info.picCropRect.top;
			info->clip.width  = chip_media->initial_info.picCropRect.right;
			info->clip.height = chip_media->initial_info.picCropRect.bottom;
		} else {
			info->clip.x      = 0;
			info->clip.y      = 0;
			info->clip.width  = chip_media->initial_info.picWidth;
			info->clip.height = chip_media->initial_info.picHeight;
		}
		h_vd_get_frame_size(vd, &info->frame_size);
		h_vd_get_colbuf_size(vd, NULL, &info->colbuf_size);
		if (chip_media->h_info.store_mode == STORE_FIELD)
			info->min_framebuf_num = (chip_media->h_info.min_framebuf_num+1)/2+ DISP_MIN;
		else
			info->min_framebuf_num = chip_media->h_info.min_framebuf_num+ DISP_MIN;

		return 0;
	}

	VIDEODEC_PRINTF(" get header info error \n");
	return -1;
}

static void h_vd_get_state(void *vd, DecState *state, DecErrCode *err_code)
{
	struct vd_chip_media *chip_media = vd;

	if(state)
		*state = chip_media->state;
	if(err_code)
		*err_code = chip_media->err_code;
}

static void h_vd_get_afd(void *vd, void *afd)
{
	struct vd_chip_media *chip_media = vd;

	gx_memcpy(afd, &chip_media->AFD, sizeof(chip_media->AFD));
}

static void h_vd_get_seqstate(void *vd, int *state)
{
	struct vd_chip_media *chip_media = vd;

	*state = chip_media->seqstatus;
}

static void h_vd_set_head_info_rate(void *vd,DecOutputInfo *output_info)
{
	int frame_rate  = -1;
	unsigned int dr = 0;
	struct vd_chip_media *chip_media = vd;

	///< just for mpeg4
	if((chip_media->open_parame.bitstreamFormat == STD_MPEG4) ||
			(chip_media->open_parame.bitstreamFormat == STD_DIV3)) {
		if(chip_media->initial_info.mp4_fixed_vop_rate==0) {
			dr = output_info->picFrateDr-output_info->picFrateDrPrev;
			if (dr != 0) {
				frame_rate = output_info->picFrateNr*10/dr;
			}
		} else {
			if(chip_media->h_info.fps != FRAME_RATE_NONE) {
				chip_media->h_info.ready = 1;
				return;
			} else {
				frame_rate = 250;///< mpeg4 mp4_fixed_vop_rate == 1的情况下，实在无法获取帧率
			}
		}

		chip_media->h_info.ready = 1;
		chip_media->h_info.fpsok = 1;

		switch ( frame_rate )
		{
			case 239:
				chip_media->h_info.fps = FRAME_RATE_23976;
				break;
			case 240:
				chip_media->h_info.fps = FRAME_RATE_24;
				break;
			case 250:
				chip_media->h_info.fps = FRAME_RATE_25;
				break;
			case 293:
				chip_media->h_info.fps = FRAME_RATE_29397;
				break;
			case 149:
			case 299:
				chip_media->h_info.fps = FRAME_RATE_2997;
				break;
			case 301:
				chip_media->h_info.fps = FRAME_RATE_301;
				break;
			case 150:
				chip_media->h_info.fps = FRAME_RATE_15;
				break;
			case 300:
				chip_media->h_info.fps = FRAME_RATE_30;
				break;
			case 500:
				chip_media->h_info.fps = FRAME_RATE_50;
				break;
			case 599:
				chip_media->h_info.fps = FRAME_RATE_5994;
				break;
			case 600:
				chip_media->h_info.fps = FRAME_RATE_60;
				break;
			default:
				///< 这样处理主要是为了处理mpeg4,
				chip_media->h_info.ready = 0;
				chip_media->h_info.fpsok = 0;
				if(++chip_media->h_info.ready_frame_out > 1) {
					if(IS_VALID_FPS(chip_media->dmx_fps)) {
						chip_media->h_info.fps = chip_media->dmx_fps;
					} else {
						chip_media->h_info.fps = DEFAULT_FPS;
						gx_printf("[error] default fps\n");
					}
					chip_media->h_info.ready  = 1;
					VIDEODEC_PRINTF("[error] frame rate error ,get default frame rate\n");
				} else {
					chip_media->h_info.fps = FRAME_RATE_NONE;
					VIDEODEC_PRINTF("[error] frame rate error \n");
				}
				break;
		}
	}
}

static int h_vd_set_top_first(void *vd,DecOutputInfo *output_info)
{
	short t_poc,b_poc ;
	int top_field_first = 0;
	unsigned int interlaced = get_scan_type(vd, output_info);

	struct  vd_chip_media *chip_media = vd;
	unsigned int codec = chip_media->open_parame.bitstreamFormat;

	if(interlaced == SCAN_PROGRESSIVE) {
		if(codec == STD_MPEG2 && chip_media->output_info.pictureStructure == 3) {
			top_field_first = (output_info->topFieldFirst==0)?0:1;
		} else {
			top_field_first = 1;
		}
	}
	else {
		if(codec == STD_MPEG2) {
			if(chip_media->output_info.pictureStructure == 3) {
				top_field_first = (output_info->topFieldFirst==0)?0:1;
			} else {
				top_field_first = 1;
			}
		}
		else if(codec == STD_AVC) {
			switch (chip_media->output_info.h264PicStruct) {
				case 0:
					top_field_first = 1;
					break;
				case 1:
				case 2:
					if(output_info->topFieldFirst) {
						t_poc = output_info->decPicPoc & 0xffff;
						b_poc = (output_info->decPicPoc>>16) & 0xffff;
					}
					else {
						b_poc = output_info->decPicPoc & 0xffff;
						t_poc = (output_info->decPicPoc>>16) & 0xffff;
					}

					if(t_poc == b_poc)
						top_field_first = output_info->topFieldFirst;
					else if(t_poc < b_poc)
						top_field_first = 1;
					else
						top_field_first = 0;
					break;
				case 5: output_info->repeatFirstField = 1;
				case 3: top_field_first = 1;
						break;
				case 6: output_info->repeatFirstField = 1;
				case 4: top_field_first = 0;
						break;
				default:
					top_field_first = 1;
					break;
			}
		}
		else if(codec == STD_HEVC) {
			switch (chip_media->output_info.h264PicStruct) {
				case 4:
				case 6:
				case 9:
				case 12:
					top_field_first = 0;
					break;
				default:
					top_field_first = 1;
					break;
			}
		}
		else {
			top_field_first = 1;
		}
	}

	return top_field_first;
}

int h_vd_get_int_type(void *vd)
{
	int ret    = -1;
	int type   = chip_media_get_inttype();

	if (type & (1<<ESBUF_EMPTY))
		ret = VD_DECODE_EMPTY;
	if (type & (1<<SEQ_INIT))
		ret = VD_SEQ_OVER;
	if (type & (1<<DEC_RESYNC_SEQ))
		ret = VD_SEQ_OVER_EX;
	if (type & (1<<PIC_RUN))
		ret = VD_DECODE_OVER;
	if (type & (1<<SEQ_CHANGED))
		ret = VD_SEQ_CHANGED;

	return ret;
}

int h_vd_stop(void *vd, int timeout)
{
	RetCode                 ret  = RETCODE_SUCCESS;
	DecOutputInfo           output_info;

	struct vd_chip_media *chip_media = vd;

	if (chip_media->state == VIDEODEC_STOPED) {
		gx_printf("[driver][video] chip_media status is error,please check \n ");
		return 0;
	}
	VIDEODEC_DBG("start to stop decoder %x \n ",chip_media->stop_enable);

	h_vd_int_mask(vd);

	chip_media->phy_workbuf = 0;
	chip_media->vir_workbuf = 0;
	chip_media->f_ignore = NONE_IGNORE;
	chip_media->dec_mode = DECODE_NORMAL;

	chip_media->stop_enable |= DEC_STOP_REQUIRE;

	if(chip_media->userdata.enable) {
		h_vd_userdata_free_buf(vd);
	}

	if( (chip_media->seqstatus>=DEC_PROCESS_FINDED_HEADER)
			&&chip_media->stop_enable!=(DEC_STOP_REQUIRE|DEC_STOP_ENABLE)
			&& chip_media->chip_media_state==CHIP_MEDIA_RUNNING ) {

		chip_media_int_enable(1<<PIC_RUN);

		ret = VPU_DecUpdateBitstreamBuffer(chip_media->handle, 0);

		VIDEODEC_PRINTF("start to try stopping decoder %x %x %x %x %x\n ",chip_media->stop_enable,
				chip_media->seqstatus,
				chip_media->chip_media_state,
				chip_media->stop_enable,
				ret);

		if(timeout == -1) {
			unsigned long tick = 10;
			do {
				gx_mdelay(10);
				if(chip_media->stop_enable==(DEC_STOP_ENABLE|DEC_STOP_REQUIRE)) {
					VIDEODEC_PRINTF(" DEC_STOP_ENABLE %x \n ",chip_media->stop_enable);
					ret = VPU_DecClose(chip_media->handle);
					if(ret!=0)
						VIDEODEC_PRINTF("VPU_DecClose ret = %x ",ret);
#ifdef H_VD_BUG
					gxav_clock_cold_rst(MODULE_TYPE_VIDEO_DEC);
#endif
					goto end;
				}
				VIDEODEC_PRINTF("try stopping decoder %x %x\n ",chip_media->stop_enable,chip_media->chip_media_state);

			}while( tick-- );
		}

		VIDEODEC_PRINTF("start to stop decoder violence %x \n ",
				chip_media->stop_enable);

		// 强行关闭解码器
		ret = VPU_DecGetOutputInfo(chip_media->handle, &output_info);
		if(ret!=0)
			VIDEODEC_PRINTF("[violence] VPU_DecGetOutputInfo ret = %x ",ret);

		ret = VPU_DecClose(chip_media->handle);
		if(ret!=0)
			VIDEODEC_PRINTF("[violence] VPU_DecClose ret = %x ",ret);
		gxav_clock_cold_rst(MODULE_TYPE_VIDEO_DEC);
	} else {
		if(chip_media->seqstatus == DEC_PROCESS_START){
			VPU_DecSetEscSeqInit (chip_media->handle, 1);
		}
		VIDEODEC_DBG(" stop decoder directly %x %x %x\n ",chip_media->seqstatus,
				chip_media->stop_enable,
				chip_media->chip_media_state);


		ret = VPU_DecClose(chip_media->handle);
#ifdef H_VD_BUG
		gxav_clock_cold_rst(MODULE_TYPE_VIDEO_DEC);
#endif
		if(0 != ret) {
			VIDEODEC_PRINTF("chip media close error  ret=%x ",ret );
			return -1;
		}
	}

end:
#if FB_BOUND_CHECK
	if (mm_check_all(__func__, __LINE__) != FB_MEM_NORMAL) {
		chip_media->state    = VIDEODEC_ERROR;
		chip_media->err_code = VIDEODEC_ERR_FB_OVERFLOW;
		return -1;
	}
#endif
	chip_media->fb_busy_flag = 0;
	chip_media->dmx_fps = 0;
	chip_media->state  = VIDEODEC_STOPED;
	chip_media->handle = 0;
	chip_media->video_dec_restart = 0;

	chip_media->av_sync     = 0;
	chip_media->first_pts   = 0;
	chip_media->flush_state = NO_REQUIRE;

	gx_memset(&(chip_media->initial_info), 0, sizeof(DecInitialInfo) );
	VIDEODEC_DBG(" stopped decoder suuserdataess stop_enable=%x \n\n\n\n ",
			chip_media->stop_enable);

	return 0;
}

static int h_vd_bufferempty_isr( void *vd )
{
	unsigned int size = 0;
	PhysicalAddress prd, pwr;
	struct vd_chip_media *chip_media = vd;

	h_vd_bitstream_unload(chip_media);
#if 1
	//firmware v1.1.1 eprd is ready onlyfor mpeg2
	if(chip_media->seqstatus < DEC_PROCESS_FINDED_HEADER) {
		VPU_DecGetBitstreamBuffer(chip_media->handle,&prd,&pwr,&size);
		prd -= chip_media->bitstream_buf_addr;
		if(prd) {
			prd = (prd+chip_media->open_parame.bitstreamBufferSize-512)%chip_media->open_parame.bitstreamBufferSize;
			ptsfetcher_drop(&chip_media->pts_fetcher, prd);
		}
	} else
#endif
	{
		if (chip_media->open_parame.bitstreamFormat == STD_MPEG2 ||\
				chip_media->open_parame.bitstreamFormat == STD_AVC ||\
				chip_media->open_parame.bitstreamFormat == STD_HEVC ||\
				chip_media->open_parame.bitstreamFormat == STD_AVS) {
			unsigned s_addr, e_addr, offset;
			h_vd_get_eprd(chip_media, &e_addr, &s_addr);
			offset = (s_addr+1-chip_media->bitstream_buf_addr+chip_media->bitstream_buf_size)%chip_media->bitstream_buf_size;
			ptsfetcher_drop(&chip_media->pts_fetcher, offset);
		}
	}

	if(0 != h_vd_bitstream_load(chip_media)) {
		DECODER_PUT_REQUEST(chip_media);
	}

	return 0;
}

static int h_vd_clr_all(void *vd)
{
	int i, ret = 0;
	unsigned fb_mask = 0;
	struct vd_chip_media *chip_media = vd;

	if((!VPU_IsBusy()) && (chip_media->chip_media_state==CHIP_MEDIA_IDLE) ) {
		chip_media->fb_busy_flag = 0;
		fb_mask = h_vd_get_fb_mask();
		for(i = 0; i < 32; i++) {
			if(fb_mask & (1<<i)) {
				ret = VPU_DecClrDispFlag(chip_media->handle, i);
				if(ret != 0) {
					VIDEODEC_PRINTF("[fatal] clr frame error \n");
					return -1;
				}
			}
		}
	}

	return ret;
}

static int h_vd_seqchanged_isr(void *vd)
{
	RetCode   ret = RETCODE_SUCCESS;
	SecAxiUse secAxiUse = {0};
	struct vd_chip_media *chip_media = vd;
	DecInitialInfo *info = &chip_media->initial_info;
	int std = chip_media->open_parame.bitstreamFormat;

	chip_media->fb_busy_flag = 0;
	chip_media->search_first_i_frame = 1;

	chip_media->chip_media_state = CHIP_MEDIA_IDLE;
	h_vd_bitstream_unload(chip_media);

	h_vd_clr_all(vd);
	ret = VPU_DecClrState(chip_media->handle, info);
	if (0 != ret) {
		VIDEODEC_PRINTF( "[warning] clr initial ret = %x\n",ret);
		chip_media->refind_header = 1;
		return -1;
	}

	ret = VPU_DecGetInitialInfo(chip_media->handle, info);
	if (0 != ret) {
		VIDEODEC_PRINTF( "[warning] get init info error code = %x\n",ret);
		chip_media->refind_header = 1;
		return -1;
	}

	if (info->mp4_gmc_flag > 0 || (std == STD_HEVC && !chip_support_hevc(info->bitdepth)) || !chip_support_format(info->chroma_idc, info->bitdepth)) {
		gx_printf("mp4_gmc:%d, hevc:bitdepth:%d, chroma_idc:%d, not support, refind header\n", info->mp4_gmc_flag, info->bitdepth, info->chroma_idc);
		//chip_media->state    = VIDEODEC_ERROR;
		//chip_media->err_code = VIDEODEC_ERR_CODECTYPE_UNSUPPORT;
		//return -2;
		chip_media->refind_header = 1;
		return -1;
	}

	if (chip_media->sec_axi_en) {
		secAxiUse.useBitEnable      = USE_BIT_INTERNAL_BUF;
		secAxiUse.useIpEnable       = USE_IP_INTERNAL_BUF;
		secAxiUse.useDbkYEnable     = USE_DBKY_INTERNAL_BUF;
		secAxiUse.useDbkCEnable     = USE_DBKC_INTERNAL_BUF;
		secAxiUse.useOvlEnable      = USE_OVL_INTERNAL_BUF;
		secAxiUse.useBtpEnable      = USE_BTP_INTERNAL_BUF;
		secAxiUse.useMeEnable       = USE_ME_INTERNAL_BUF;
		secAxiUse.useHostBitEnable  = USE_HOST_BIT_INTERNAL_BUF;
		secAxiUse.useHostDbkYEnable = USE_HOST_DBKY_INTERNAL_BUF;
		secAxiUse.useHostDbkCEnable = USE_HOST_DBKC_INTERNAL_BUF;
		secAxiUse.useHostIpEnable   = USE_HOST_IP_INTERNAL_BUF;
		secAxiUse.useHostOvlEnable  = USE_HOST_OVL_INTERNAL_BUF;
		secAxiUse.useHostBtpEnable  = USE_HOST_BTP_INTERNAL_BUF;
		secAxiUse.useHostMeEnable   = USE_HOST_ME_INTERNAL_BUF;
	} else {
		secAxiUse.useBitEnable      = 0;
		secAxiUse.useIpEnable       = 0;
		secAxiUse.useDbkYEnable     = 0;
		secAxiUse.useDbkCEnable     = 0;
		secAxiUse.useOvlEnable      = 0;
		secAxiUse.useBtpEnable      = 0;
		secAxiUse.useMeEnable       = 0;
		secAxiUse.useHostBitEnable  = 0;
		secAxiUse.useHostDbkYEnable = 0;
		secAxiUse.useHostDbkCEnable = 0;
		secAxiUse.useHostIpEnable   = 0;
		secAxiUse.useHostOvlEnable  = 0;
		secAxiUse.useHostBtpEnable  = 0;
		secAxiUse.useHostMeEnable   = 0;
	}
	VPU_DecGiveCommand(chip_media->handle, SET_SEC_AXI, &secAxiUse);

	if(chip_media->userdata.enable) {
		unsigned int userdata_buf_addr = (unsigned int)gx_dma_to_phys(chip_media->userdata.buf_addr);
		VPU_DecGiveCommand(chip_media->handle, SET_ADDR_REP_USERDATA, &userdata_buf_addr);
		VPU_DecGiveCommand(chip_media->handle, SET_SIZE_REP_USERDATA, &chip_media->userdata.data_length);
		VPU_DecGiveCommand(chip_media->handle, ENABLE_REP_USERDATA, NULL);
	}

	ret =  h_vd_set_head_info(chip_media);
	if(0 != ret) {
		chip_media->refind_header = 1;
		if(-2 == ret) {
			chip_media->state    = VIDEODEC_ERROR;
			chip_media->err_code = VIDEODEC_ERR_SIZE_UNSUPPORT;
			return -2;
		}
		VIDEODEC_PRINTF( "[fatal] get head info error when prepare to config frame buffer \n");
		return -1;
	}

	fps_prober_init();
	fps_prober.dec_fps    = chip_media->h_info.fps;
	fps_prober.dec_fpsok  = chip_media->h_info.fpsok;
	chip_media->seqstatus = DEC_PROCESS_FINDED_HEADER;
	chip_media->disp_first_iframe = 0;

#ifdef DBG_SWITCH_PROGRAM
	head_end = gx_current_tick();
#endif

	return 0;
}

static int h_vd_seqinit_isr(void *vd)
{
	RetCode ret = RETCODE_SUCCESS;
	SecAxiUse secAxiUse = {0};
	struct vd_chip_media *chip_media = vd;
	DecInitialInfo *info = &chip_media->initial_info;
	int std = chip_media->open_parame.bitstreamFormat;

	h_vd_bitstream_unload(chip_media);
	ret = VPU_DecGetInitialInfo(chip_media->handle, &(chip_media->initial_info));
	if (0 != ret) {
		VIDEODEC_PRINTF( "[warning] get init info error code=%x\n",ret);
		chip_media->refind_header = 1;
		return -1;
	}
	if (info->mp4_gmc_flag > 0 || (std == STD_HEVC && !chip_support_hevc(info->bitdepth)) || !chip_support_format(info->chroma_idc, info->bitdepth)) {
		gx_printf("mp4_gmc:%d, hevc:bitdepth:%d, chroma_idc:%d, not support, reset video\n", info->mp4_gmc_flag, info->bitdepth, info->chroma_idc);
		chip_media->state    = VIDEODEC_ERROR;
		chip_media->err_code = VIDEODEC_ERR_CODECTYPE_UNSUPPORT;
		return -2;
	}

	if (chip_media->sec_axi_en) {
		secAxiUse.useBitEnable      = USE_BIT_INTERNAL_BUF;
		secAxiUse.useIpEnable       = USE_IP_INTERNAL_BUF;
		secAxiUse.useDbkYEnable     = USE_DBKY_INTERNAL_BUF;
		secAxiUse.useDbkCEnable     = USE_DBKC_INTERNAL_BUF;
		secAxiUse.useOvlEnable      = USE_OVL_INTERNAL_BUF;
		secAxiUse.useBtpEnable      = USE_BTP_INTERNAL_BUF;
		secAxiUse.useMeEnable       = USE_ME_INTERNAL_BUF;
		secAxiUse.useHostBitEnable  = USE_HOST_BIT_INTERNAL_BUF;
		secAxiUse.useHostDbkYEnable = USE_HOST_DBKY_INTERNAL_BUF;
		secAxiUse.useHostDbkCEnable = USE_HOST_DBKC_INTERNAL_BUF;
		secAxiUse.useHostIpEnable   = USE_HOST_IP_INTERNAL_BUF;
		secAxiUse.useHostOvlEnable  = USE_HOST_OVL_INTERNAL_BUF;
		secAxiUse.useHostBtpEnable  = USE_HOST_BTP_INTERNAL_BUF;
		secAxiUse.useHostMeEnable   = USE_HOST_ME_INTERNAL_BUF;
	} else {
		secAxiUse.useBitEnable      = 0;
		secAxiUse.useIpEnable       = 0;
		secAxiUse.useDbkYEnable     = 0;
		secAxiUse.useDbkCEnable     = 0;
		secAxiUse.useOvlEnable      = 0;
		secAxiUse.useBtpEnable      = 0;
		secAxiUse.useMeEnable       = 0;
		secAxiUse.useHostBitEnable  = 0;
		secAxiUse.useHostDbkYEnable = 0;
		secAxiUse.useHostDbkCEnable = 0;
		secAxiUse.useHostIpEnable   = 0;
		secAxiUse.useHostOvlEnable  = 0;
		secAxiUse.useHostBtpEnable  = 0;
		secAxiUse.useHostMeEnable   = 0;
	}
	VPU_DecGiveCommand(chip_media->handle, SET_SEC_AXI, &secAxiUse);

	if (chip_media->userdata.enable) {
		unsigned int userdata_buf_addr = (unsigned int)gx_dma_to_phys(chip_media->userdata.buf_addr);
		VPU_DecGiveCommand(chip_media->handle, SET_ADDR_REP_USERDATA, &userdata_buf_addr);
		VPU_DecGiveCommand(chip_media->handle, SET_SIZE_REP_USERDATA, &chip_media->userdata.data_length);
		VPU_DecGiveCommand(chip_media->handle, ENABLE_REP_USERDATA, NULL);
	}

	ret =  h_vd_set_head_info( chip_media );
	if(0 != ret) {
		chip_media->refind_header = 1;
		if(-2 == ret) {
			chip_media->state    = VIDEODEC_ERROR;
			chip_media->err_code = VIDEODEC_ERR_SIZE_UNSUPPORT;
			return -2;
		}
		VIDEODEC_PRINTF( "[fatal] get head info error when prepare to config frame buffer \n");
		return -1;
	}
	fps_prober.dec_fps    = chip_media->h_info.fps;
	fps_prober.dec_fpsok  = chip_media->h_info.fpsok;
	chip_media->seqstatus = DEC_PROCESS_FINDED_HEADER;
	chip_media->disp_first_iframe = 0;

#ifdef DBG_SWITCH_PROGRAM
	head_end = gx_current_tick();
#endif
	return 0;
}

static int h_vd_decodeover_isr( void *vd ,struct h_vd_dec_info *dec_info)
{
	RetCode  ret = RETCODE_SUCCESS;
	struct vd_chip_media *chip_media = vd;

#if FB_BOUND_CHECK
	if (mm_check_all(__func__, __LINE__) != FB_MEM_NORMAL) {
		chip_media->state    = VIDEODEC_ERROR;
		chip_media->err_code = VIDEODEC_ERR_FB_OVERFLOW;
		return -1;
	}
#endif
	ret = h_vd_get_frame_info(chip_media,dec_info);

	if(chip_media->freeze_fb_state == FB_UNMASK_REQUIRE)
		h_vd_fz_unmask(vd);

	return ret;
}

int h_vd_interrupt(void *vd, h_vd_int_type *int_type, struct h_vd_dec_info *dec_info)
{
	*int_type = h_vd_get_int_type(vd);
	struct vd_chip_media *chip_media = ( struct vd_chip_media *)vd;

	switch (*int_type) {
		case VD_SEQ_OVER_EX:
			chip_media_clrint((1<<DEC_RESYNC_SEQ));
		case VD_SEQ_EMPTY:
		case VD_SEQ_OVER:
			chip_media->fb_is_ready = 0;
			chip_media_clrint(1<<SEQ_INIT);
			return h_vd_seqinit_isr(vd);
		case VD_DECODE_EMPTY:
			chip_media_clrint(1<<ESBUF_EMPTY);
			return h_vd_bufferempty_isr(vd);
		case VD_DECODE_OVER:
			chip_media_clrint(1<<PIC_RUN);
			return h_vd_decodeover_isr(vd, dec_info);
		case VD_SEQ_CHANGED:
			chip_media->fb_is_ready = 0;
			chip_media_clrint(1<<SEQ_CHANGED);
			return h_vd_seqchanged_isr(vd);
		default:
			return -1;
	}

	return 0;
}

int h_vd_send_callback(void* p)
{
	int ret = 0;
	unsigned long flag = 0;

	struct vd_chip_media *chip_media = ( struct vd_chip_media *)p;

	gx_spin_lock_irqsave(&chip_media->spin_lock, flag);
	if(chip_media->state == VIDEODEC_RUNNING) {
		if(chip_media->seqstatus < DEC_PROCESS_FINDED_HEADER) {
			h_vd_restart_find_header(chip_media);
		}
		if(DECODER_CHECK_REQUEST(chip_media)) {
			ret = h_vd_bitstream_load( chip_media );
			if( ret==0 ) {
				DECODER_GET_REQUEST(chip_media);
			}
		}
	}
	gx_spin_unlock_irqrestore(&chip_media->spin_lock, flag);

	return 0;
}

struct h_vd_ops vd_ops =
{
	.h_vd_create                    = h_vd_create,
	.h_vd_start                     = h_vd_start,
	.h_vd_stop                      = h_vd_stop,
	.h_vd_pause                     = h_vd_pause,
	.h_vd_resume                    = h_vd_resume,
	.h_vd_config                    = h_vd_config,
	.h_vd_set_format                = h_vd_set_format,
	.h_vd_delete                    = h_vd_delete,
	.h_vd_start_find_header         = h_vd_start_find_header,
	.h_vd_get_capability            = h_vd_get_capability,
	.h_vd_decode_one_frame          = h_vd_decode_one_frame,
	.h_vd_clr_frame                 = h_vd_clr_frame,
	.h_vd_bs_flush                  = h_vd_bs_flush,
	.h_vd_config_frame_buf          = h_vd_config_frame_buf,
	.h_vd_get_head_info             = h_vd_get_head_info,
	.h_vd_get_frame_info            = NULL,
	.h_vd_get_int_type              = NULL,
	.h_vd_callback                  = h_vd_send_callback,
	.h_vd_get_state                 = h_vd_get_state,
	.h_vd_get_afd                   = h_vd_get_afd,
	.h_vd_get_seqstate              = h_vd_get_seqstate,
	.interrupts                     = h_vd_interrupt,
	.h_vd_int_mask                  = h_vd_int_mask,
	.h_vd_int_unmask                = h_vd_int_unmask,
	.h_vd_fb_mask_require           = h_vd_fb_mask_require,
	.h_vd_fb_unmask_require         = h_vd_fb_unmask_require,
	.h_vd_get_fb_mask_state         = h_vd_get_fb_mask_state,
	.h_vd_set_fb_mask_state         = h_vd_set_fb_mask_state,
	.h_vd_get_framesize             = h_vd_get_frame_size,
	.h_vd_userdata_read             = h_vd_userdata_read,
	.check_skip_able                = h_vd_check_skip_able,
};

