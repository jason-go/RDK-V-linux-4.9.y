/*****************************************************************************
 *CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2006, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	gx_video_dec.c
 * Author    :   liql
 * Project   :	GX-RDC-MPEG2
 * Type      :	Driver
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 0.0		2011.04.07	      liql             Creation
 *****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "gxav.h"
#include "avcore.h"
#include "kernelcalls.h"
#include "gxav_module_property.h"
#include "gxav_video_propertytypes.h"
#include "video.h"
#include "gx3201_sdc_reg.h"
#include "video_sync.h"

/* Private types/constants ------------------------------------------------ */

static unsigned int decover[2] = {0, 0};
static unsigned int show_logo[2] = {0, 0};
static unsigned int one_frame_over[2] = {0,0};

/* Private variables ------------------------------------------------------ */

/* Private macros --------------------------------------------------------- */
#define GET_MODULE(inode,sub)   (inode->module[sub])
#define FIND_MODULE_SUBID(  )   (0) ///< 如果是多解码需要修改此宏,手段根据实际情况而定

#define CHECK_PROPERTY_PARAM(para,type,size) \
	do { \
		if (para == NULL || size != sizeof(type)) { \
			VIDEODEC_PRINTF("get or set property error! \n"); \
			return -1; \
		}\
	}while(0)

unsigned int VIDEO_PP_INC_SRC    = 43;
unsigned int VIDEO_VPU_INC_SRC   = 46;
unsigned int VIDEO_BODA_INIT_SRC = 61;

static int is_valid_codec(GxVideoDecoderProperty_Type codec)
{
	unsigned i, codec_full, mask, codecs;

	if (codec < CODEC_MPEG2V || codec >= CODEC_MAX) {
		return 0;
	}

	codecs = codec_full = 0;
	for (i = 1; i < CODEC_MAX; i++) {
		codec_full |= (1<<i);
	}

	if (CHIP_IS_GX3201) {
		mask   = (1<<CODEC_HEVC)|(1<<CODEC_VC1)|(1<<CODEC_YUV)|(1<<CODEC_MJPEG);
		codecs = codec_full ^ mask;
	} else if (CHIP_IS_GX3113C) {
		mask   = (1<<CODEC_HEVC)|(1<<CODEC_VC1)|(1<<CODEC_YUV)|(1<<CODEC_MJPEG);
		codecs = codec_full ^ mask;
	} else if (CHIP_IS_GX3211 || CHIP_IS_TAURUS) {
		mask   = (1<<CODEC_VC1)|(1<<CODEC_YUV)|(1<<CODEC_MJPEG);
		codecs = codec_full ^ mask;
	} else if (CHIP_IS_CYGNUS) {
		mask   = (1<<CODEC_AVSV)|(1<<CODEC_VC1)|(1<<CODEC_YUV)|(1<<CODEC_MJPEG);
		codecs = codec_full ^ mask;
	} else if (CHIP_IS_GX6605S) {
		mask   = (1<<CODEC_AVSV)|(1<<CODEC_HEVC)|(1<<CODEC_VC1)|(1<<CODEC_RV)|(1<<CODEC_RV_BITSTREAM)|(1<<CODEC_YUV)|(1<<CODEC_MJPEG);
		codecs = codec_full ^ mask;
	} else if (CHIP_IS_SIRIUS || CHIP_IS_GEMINI) {
		mask   = (1<<CODEC_VC1)|(1<<CODEC_YUV)|(1<<CODEC_MJPEG);
		codecs = codec_full ^ mask;
	}

	return codecs & (1<<codec);
}

int video_hal_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	if ( dev == NULL || inode == NULL){
		VIDEODEC_PRINTF("param error!\n");
		return -1;
	}

	return gx_video_init(((struct gxav_module_ops*)(inode->interface))->priv);
}

int video_hal_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	if ( dev == NULL || inode == NULL ){
		VIDEODEC_PRINTF(" param error!\n");
	}

	return gx_video_cleanup( );
}

int video_hal_open(struct gxav_module *module)
{
	return gx_video_open(module->sub);
}

int video_hal_close(struct gxav_module *module)
{
	return gx_video_close(module->sub);
}

int video_hal_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	VIDEODEC_DBG("gxav_videodec_set_property :[property_id] : %d \n", property_id);

	switch (property_id) {
		case GxAVGenericPropertyID_ModuleLinkChannel:
			{
				unsigned char buf_id;
				unsigned int start_addr,end_addr;
				struct fifo_info *fifo = (struct fifo_info *)property;
				struct h_vd_video_fifo video_fifo;
				struct gxav_channel* channel;

				CHECK_PROPERTY_PARAM(fifo,struct fifo_info,size);

				if (GXAV_PIN_INPUT == fifo->dir) {
					channel = (struct gxav_channel*)(fifo->channel);
					video_fifo.bs_id  = gxav_channel_id_get(channel);
					video_fifo.pts_id = gxav_channel_pts_id_get(channel);
					gxav_channel_get_phys(channel, &start_addr,&end_addr, &buf_id);
					video_fifo.bitstream_buf_addr = start_addr;
					video_fifo.bitstream_buf_size = end_addr-start_addr+1;
					gxav_channel_get_ptsbuffer(channel, &start_addr, &end_addr, &buf_id);
					video_fifo.pts_buf_addr = (unsigned int)channel->pts_buffer;
					video_fifo.pts_buf_size= end_addr-start_addr+1;
					video_fifo.channel = channel;
				}
				else {
					VIDEODEC_PRINTF("%s(),link fifo dir=%d,error.\n",__func__,fifo->dir);
					return -1;
				}

				return gx_video_fifo_link( module->sub,&video_fifo);
			}
		case GxAVGenericPropertyID_ModuleUnLinkChannel:
			{
				struct fifo_info *fifo = (struct fifo_info *)property;
				struct h_vd_video_fifo video_fifo;

				CHECK_PROPERTY_PARAM(fifo,struct fifo_info,size);

				video_fifo.channel = fifo->channel;

				return gx_video_fifo_unlink( module->sub, &video_fifo);
			}
		case GxVideoDecoderPropertyID_Config:
			{
				unsigned int fps = 0;
				unsigned int ret = 0;
				GxVideoDecoderProperty_Config *config = (GxVideoDecoderProperty_Config *) property;
				CHECK_PROPERTY_PARAM(config,GxVideoDecoderProperty_Config,size);

				if(is_valid_codec(config->type))
					gx_video_config(module->sub, VIDEO_CONFIG_FORMAT, &config->type, 0);
				else {
					VIDEODEC_PRINTF("%s(),codec type [%d] unsupport .\n",__func__,config->type);
					return -1;
				}

				ret = gx_video_config(module->sub, VIDEO_CONFIG_SYNC, &config->pts_sync, 0);
				if(ret != 0) return ret;

				ret = gx_video_config(module->sub, VIDEO_CONFIG_PTS_REORDER, &config->pts_reorder, 0);
				if(ret != 0) return ret;

				ret = gx_video_config(module->sub, VIDEO_CONFIG_DEINTERLACE, &config->deinterlace, sizeof(config->deinterlace));
				if(ret != 0) return ret;

				ret = gx_video_config(module->sub, VIDEO_CONFIG_MOSAIC_FREEZE_GATE, &config->mosaic_freez, 0);
				if(ret != 0) return ret;

				ret = gx_video_config(module->sub, VIDEO_CONFIG_MOSAIC_DROP_GATE, &config->err_gate, 0);
				if(ret != 0) return ret;

				ret = gx_video_config(module->sub, VIDEO_CONFIG_USERDATA_PARAM, &config->userdata, sizeof(config->userdata));
				if(ret != 0) return ret;

				if (config->workbuf.size > 0) {
					ret = gx_video_config(module->sub, VIDEO_CONFIG_WORKBUF, &config->workbuf, sizeof(config->workbuf));
					if(ret != 0) return ret;
				}

				if (config->fps>=0 && config->fps<=60)
					fps = config->fps;
				ret = gx_video_config(module->sub, VIDEO_CONFIG_FPS, &fps, 0);
				if(ret != 0) return ret;

				if (config->fps_source == FROM_DECODER || config->fps_source == FROM_PTS) {
					ret = gx_video_config(module->sub, VIDEO_CONFIG_FPSSOURCE, &config->fps_source, 0);
					if(ret != 0) return ret;
				}

				return 0;
			}
		case GxVideoDecoderPropertyID_Run:
			{
				int ret;
				GxVideoDecoderProperty_Run *run = (GxVideoDecoderProperty_Run*) property;

				ret = gx_video_config(module->sub, VIDEO_CONFIG_COLBUF, &run->colbuf, sizeof(run->colbuf));
				if(ret != 0) return ret;
				ret = gx_video_config(module->sub, VIDEO_CONFIG_FRAMEBUF, &run->framebuf, sizeof(run->framebuf));
				if(ret != 0) return ret;
				ret = gx_video_config(module->sub,
						VIDEO_CONFIG_DEINTERLACE_EN, &run->deinterlace_en, sizeof(run->deinterlace_en));
				if(ret != 0) return ret;

				one_frame_over[module->sub] = 0;
				ret = gx_video_run(module->sub);
				return ret;
			}
		case GxVideoDecoderPropertyID_DeinterlaceEnable:
			{
				GxVideoDecoderProperty_DeinterlaceEnable *param = (GxVideoDecoderProperty_DeinterlaceEnable*)property;
				return gx_video_config(module->sub,
						VIDEO_CONFIG_DEINTERLACE_EN, &param->enable, sizeof(param->enable));
			}
		case GxVideoDecoderPropertyID_Probe:
			{
				int ret;
				ret = gx_video_probe(module->sub);
				return ret;
			}
		case GxVideoDecoderPropertyID_Stop:
			{
				int ret;
				GxVideoDecoderProperty_Stop *stop= (GxVideoDecoderProperty_Stop*) property;

				ret = gx_video_config(module->sub, VIDEO_CONFIG_FREEZE_FRAME, &stop->freeze, sizeof(stop->freeze));
				show_logo[module->sub] = 0;
				one_frame_over[module->sub] = 0;
				ret = gx_video_stop(module->sub);

				return ret;
			}
		case GxVideoDecoderPropertyID_Pause:
			{
				int ret;
				unsigned long flag=0;

				flag = gxav_device_spin_lock_irqsave(module->inode->dev);
				ret = gx_video_pause(module->sub);
				gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);

				return ret;
			}
		case GxVideoDecoderPropertyID_Resume:
			{
				return gx_video_resume(module->sub);
			}
		case GxVideoDecoderPropertyID_Skip:
			{
				return -1;
			}
		case GxVideoDecoderPropertyID_Speed:
			{
				int ret;
				unsigned long flag=0;

				GxVideoDecoderProperty_Speed *speed = (GxVideoDecoderProperty_Speed *) property;
				CHECK_PROPERTY_PARAM(speed,GxVideoDecoderProperty_Speed,size);

				if(speed->mode<FAST_SPEED || speed->mode>NORMAL_SPEED) {
					VIDEODEC_PRINTF("%s(),speed type [%d] unsupport .\n",__func__,speed->mode);
					return -1;
				}

				flag = gxav_device_spin_lock_irqsave(module->inode->dev);
				ret = gx_video_set_decmode(module->sub, speed->mode);
				gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);

				return ret;
			}
		case GxVideoDecoderPropertyID_Refresh:
			{
				int ret;
				unsigned long flag=0;

				flag = gxav_device_spin_lock_irqsave(module->inode->dev);
				show_logo[module->sub] = 1;
				ret = gx_video_refresh(module->sub);
				gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);

				return ret;
			}
		case GxVideoDecoderPropertyID_Update:
			{
				int ret;
				unsigned long flag=0;

				flag = gxav_device_spin_lock_irqsave(module->inode->dev);
				ret = gx_video_update(module->sub);
				gxav_device_spin_unlock_irqrestore(module->inode->dev, flag);

				return ret;
			}
		case GxVideoDecoderPropertyID_DecMode:
			{
				GxVideoDecoderProperty_DecMode *mode = (GxVideoDecoderProperty_DecMode *) property;
				CHECK_PROPERTY_PARAM(mode,GxVideoDecoderProperty_DecMode,size);

				if(mode->dec_mode<VIDEODEC_MODE_NORMAL || mode->dec_mode>VIDEODEC_MODE_IPONLY) {
					VIDEODEC_PRINTF("%s(),dec mode [%d] unsupport .\n",__func__,mode->dec_mode);
					return -1;
				}

				return gx_video_set_frame_ignore(module->sub, mode->dec_mode);
			}
		case GxVideoDecoderPropertyID_PtsOffset:
			{
				GxVideoDecoderProperty_PtsOffset *arg = (GxVideoDecoderProperty_PtsOffset*)property;
				CHECK_PROPERTY_PARAM(arg, GxVideoDecoderProperty_PtsOffset, size);
				return gx_video_set_ptsoffset(module->sub, arg->offset_ms);
			}
		case GxVideoDecoderPropertyID_Tolerance:
			{
				GxVideoDecoderProperty_Tolerance *tolerance= (GxVideoDecoderProperty_Tolerance *) property;

				CHECK_PROPERTY_PARAM(tolerance,GxVideoDecoderProperty_Tolerance,size);

				return gx_video_set_tolerance(module->sub,tolerance->value);
			}
		case GxVideoDecoderPropertyID_SyncMode:
			{
				GxVideoDecoderProperty_SyncMode *mode = (GxVideoDecoderProperty_SyncMode*)property;
				CHECK_PROPERTY_PARAM(mode, GxVideoDecoderProperty_SyncMode, size);
				return gx_video_config(module->sub, VIDEO_CONFIG_SYNC, mode, 0);
			}
		case GxVideoDecoderPropertyID_PlayBypass:
			{
				GxVideoDecoderProperty_PlayBypass *bypass = (GxVideoDecoderProperty_PlayBypass*)property;
				CHECK_PROPERTY_PARAM(bypass, GxVideoDecoderProperty_PlayBypass, size);
				return gx_video_config(module->sub, VIDEO_CONFIG_PLAYBYPASS, &bypass->enable, 0);
			}
		default:
			VIDEODEC_PRINTF("the parameter which decoder's property_id is wrong,"
					"please set the right parameter\n");
			return -2;

	}

	return 0;
}

int video_hal_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	VIDEODEC_DBG("gxav_videodec_get_property :[property_id] : %d\n", property_id);

	switch (property_id) {
		case GxVideoDecoderPropertyID_FrameInfo:
			{
				GxVideoDecoderProperty_FrameInfo *frame_info = (GxVideoDecoderProperty_FrameInfo *) property;
				struct head_info info = {0};
				CHECK_PROPERTY_PARAM(frame_info, GxVideoDecoderProperty_FrameInfo, size);

				if(gx_video_get_frame_info(module->sub, &info) == 0) {
					gx_video_get_frame_stat(module->sub, &frame_info->stat);
					frame_info->width       = info.width;
					frame_info->height      = info.height;
					frame_info->fb_num      = info.min_framebuf_num;
					frame_info->fb_size     = info.frame_size;
					frame_info->colbuf_size = info.colbuf_size;
					frame_info->ratio       = info.ratio;
					frame_info->sar         = info.sar;
					frame_info->dar         = info.dar;
					frame_info->bpp         = info.bpp;
#if 0
					char *str_ratio[] = {
						"ASPECT_RATIO_NONE",
						"ASPECT_RATIO_1BY1",
						"ASPECT_RATIO_4BY3",
						"ASPECT_RATIO_14BY9",
						"ASPECT_RATIO_16BY9",
						"ASPECT_RATIO_221BY1",
						"ASPECT_RATIO_RESERVE",
					};
					gx_printf("\nDar = %s, sar[%d:%d], dar[%d:%d]\n", str_ratio[info.ratio],
					info.sar.num, info.sar.den, info.dar.num, info.dar.den);
#endif

					frame_info->type   = info.type;
					if(info.fpsok)
						frame_info->rate = info.fps;
					else
						frame_info->rate = FRAMERATE_NONE;
				} else {
					return -1;
				}
			}
			break;

		case GxVideoDecoderPropertyID_Pts:
			{
				GxVideoDecoderProperty_PTS *pts = (GxVideoDecoderProperty_PTS *) property;

				CHECK_PROPERTY_PARAM(pts,GxVideoDecoderProperty_PTS,size);

				gx_video_get_pts(module->sub,&(pts->pts));
			}
			break;

		case GxVideoDecoderPropertyID_State:
			{
				GxVideoDecoderProperty_State *state = (GxVideoDecoderProperty_State *) property;

				CHECK_PROPERTY_PARAM(state,GxVideoDecoderProperty_State,size);
				gx_video_get_state(module->sub, &state->state, &state->err_code);
			}
			break;

		case GxVideoDecoderPropertyID_Capability:
			{
				GxVideoDecoderProperty_Capability *cap = (GxVideoDecoderProperty_Capability *) property;

				struct h_vd_cap vd_cap;

				CHECK_PROPERTY_PARAM(cap,GxVideoDecoderProperty_Capability,size);

				if(is_valid_codec(cap->type) == 0)
					return -1;

				vd_cap.format = cap->type;
				if (gx_video_get_cap(module->sub,&vd_cap) == 0)
					///< 由于目前2个结构体一致，移植的时候需要注意
					gx_memcpy(cap, &vd_cap, sizeof(GxVideoDecoderProperty_Capability));
				else
					return -1;
			}
			break;

		case GxVideoDecoderPropertyID_DecMode:
			{
				VIDEODEC_PRINTF("%s TODO: ID = %d\n",__func__,property_id);
			}
			break;
		case GxVideoDecoderPropertyID_OneFrameOver:
			{
				GxVideoDecoderProperty_OneFrameOver *over = (GxVideoDecoderProperty_OneFrameOver *)property;
				CHECK_PROPERTY_PARAM(over,GxVideoDecoderProperty_OneFrameOver,size);

				if(one_frame_over[module->sub]) {
					over->over = one_frame_over[module->sub];
					one_frame_over[module->sub] = 0;
				} else {
					over->over = 0;
				}
			}
			break;
		case GxVideoDecoderPropertyID_UserData:
			{
				GxVideoDecoderProperty_UserData *user_data = (GxVideoDecoderProperty_UserData*)property;
				CHECK_PROPERTY_PARAM(user_data, GxVideoDecoderProperty_UserData, size);
				return gx_video_get_userdata(module->sub, user_data);
			}
		default:
			VIDEODEC_PRINTF("%s(),the parameter which decoder's property_id is wrong,"
					"please set the right parameter\n", __func__);
			return -2;

	}
	return 0;
}

extern int gxav_vpu_VpuIsrCallback(void *arg);
static struct gxav_module_inode *video_hal_vpu_interrupt(struct gxav_module_inode *inode, int irq)
{
	int ret = 0;
	unsigned int pp_up = 0, event = 0;
	struct gxav_module *module = NULL;

	if(inode==NULL && irq!=VIDEO_VPU_INC_SRC) {
		VIDEODEC_PRINTF("video vpu interrup inode error! inode=%p \t irq_num=%x\n",inode,irq);
		return NULL;
	}
	module = GET_MODULE( inode,FIND_MODULE_SUBID( ) );
	if(module == NULL) {
		VIDEODEC_PRINTF("gxav find module fail %p \n", module);
		return NULL;
	}

	if (gxav_vpu_VpuIsrCallback(NULL) == 1)
		return NULL;

	gxav_module_inode_set_event(inode, EVENT_VPU_INTERRUPT);

	ret = gx_video_vpu_interrupt(module->sub,irq);
	if(ret != 0) {
		VIDEODEC_PRINTF( " vpu interrupt error " );
	}
	else if(decover[module->sub] == 1) {
		gx_video_get_pp_up(module->sub,&pp_up);
		if(pp_up || show_logo[module->sub]) {
			event = (module->sub==0) ? EVENT_VIDEO_0_DECOVER : EVENT_VIDEO_1_DECOVER;
			gxav_module_inode_set_event(inode, event);
			decover[module->sub]   = 0;
			show_logo[module->sub] = 0;
		}
	}

	return inode;
}

static struct gxav_module_inode *video_hal_boda_interrupt(struct gxav_module_inode *inode, int irq)
{
	int ret = 0;
	int speed = -1;
	unsigned int event = 0;
	h_vd_int_type int_type;
	struct gxav_module   *module	= NULL;

	if(inode==NULL && irq!=VIDEO_BODA_INIT_SRC) {
		VIDEODEC_PRINTF("video dec interrup inode error! inode=%p \t irq_num=%x\n",inode,irq);
		return NULL;
	}

	module = GET_MODULE(inode, FIND_MODULE_SUBID());
	if(module == NULL) {
		VIDEODEC_PRINTF( "gxav find module fail %p \n",module );
		goto RET;
	}
	ret = gx_video_dec_interrupt(module->sub, irq, &int_type);

	switch (int_type) {
		case VD_SEQ_OVER:
		case VD_SEQ_OVER_EX:
		case VD_SEQ_CHANGED:
			if(ret == 0) {
				event = (int_type == VD_SEQ_CHANGED ? EVENT_VIDEO_SEQ_CHANGED : EVENT_VIDEO_0_FRAMEINFO);
				gxav_module_inode_set_event(inode, event);
				decover[module->sub] = 1;
			}
			break;
		case VD_DECODE_OVER:
			if(ret ==0) {
				gx_video_get_speed(module->sub, &speed);
				if(STEP_SPEED == speed) {
					event = (module->sub==0)?EVENT_VIDEO_ONE_FRAME_OVER:EVENT_VIDEO_ONE_FRAME_OVER;
					one_frame_over[module->sub] = 1;
					gxav_module_inode_set_event(inode, event);
				}
			}
			break;
		default:
			break;
	}

RET:
	return inode;
}

static struct gxav_module_inode *video_hal_pp_interrupt(struct gxav_module_inode *inode, int irq)
{
	struct gxav_module   *module	= NULL;

	if ( inode == NULL && irq != VIDEO_PP_INC_SRC )
	{
		VIDEODEC_PRINTF("video pp interrup inode error! inode=%p \t irq_num=%x\n",inode,irq);
		return NULL;
	}

	module = GET_MODULE( inode,FIND_MODULE_SUBID( ) );
	if( module==NULL ) {
		VIDEODEC_PRINTF( "gxav find module fail %p \n",module );
		return NULL;
	}

	gx_video_pp_interrupt(module->sub, irq);

	return inode;
}

#define INT_CHECK_MASK(int_src, flag)\
	do {\
		flag = 0;\
		if(gx_interrupt_masked(int_src) == 0) {\
			gx_interrupt_mask(int_src);\
			flag = 1;\
		}\
	}while(0)

#define INT_CHECK_UNMASK(int_src, flag)\
	do {\
		if(flag)\
			gx_interrupt_unmask(int_src);\
	}while(0)

struct gxav_module_inode *video_hal_interrupt(struct gxav_module_inode *inode, int irq)
{
#ifdef ECOS_OS
	int flag_boda = 0, flag_pp = 0, flag_vpu = 0;
	gx_interrupt_disable();
	if (irq == VIDEO_VPU_INC_SRC) {
		INT_CHECK_MASK(VIDEO_BODA_INIT_SRC, flag_boda);
		INT_CHECK_MASK(VIDEO_PP_INC_SRC,    flag_pp);
	}
	else if(irq == VIDEO_BODA_INIT_SRC) {
		INT_CHECK_MASK(VIDEO_VPU_INC_SRC, flag_vpu);
		INT_CHECK_MASK(VIDEO_PP_INC_SRC,  flag_pp);
	}
	else if(irq == VIDEO_PP_INC_SRC) {
		INT_CHECK_MASK(VIDEO_VPU_INC_SRC,   flag_vpu);
		INT_CHECK_MASK(VIDEO_BODA_INIT_SRC, flag_boda);
	}
	gx_interrupt_enable();
#endif

	if(irq == VIDEO_BODA_INIT_SRC) {
		video_hal_boda_interrupt(inode, irq);
	} else if(irq == VIDEO_VPU_INC_SRC) {
		video_hal_vpu_interrupt(inode, irq);
	} else if(irq == VIDEO_PP_INC_SRC){
		video_hal_pp_interrupt(inode, irq);
	}

#ifdef ECOS_OS
	gx_interrupt_disable();
	if (irq == VIDEO_VPU_INC_SRC) {
		INT_CHECK_UNMASK(VIDEO_BODA_INIT_SRC, flag_boda);
		INT_CHECK_UNMASK(VIDEO_PP_INC_SRC,    flag_pp);
	}
	else if(irq == VIDEO_BODA_INIT_SRC) {
		INT_CHECK_UNMASK(VIDEO_VPU_INC_SRC, flag_vpu);
		INT_CHECK_UNMASK(VIDEO_PP_INC_SRC,  flag_pp);
	}
	else if(irq == VIDEO_PP_INC_SRC) {
		INT_CHECK_UNMASK(VIDEO_VPU_INC_SRC,   flag_vpu);
		INT_CHECK_UNMASK(VIDEO_BODA_INIT_SRC, flag_boda);
	}
	gx_interrupt_enable();
#endif

	return inode ;
}


