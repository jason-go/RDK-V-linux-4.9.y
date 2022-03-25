#include "decode_core.h"
#include "video_decode.h"
#include "video_display.h"
#include "pp_core.h"
#include "porting.h"

struct gxav_video_dec video_dec[MAX_DECODER] = {{-1}};

#define VIDEO_VIDEO_MODULE video_dec

struct gxav_video_dec *videodec_creat_instance( int id )
{
	int i = 0;
	static  unsigned int instance_init = 0;
	struct gxav_video_dec *videodec = VIDEO_VIDEO_MODULE;

	if(videodec == NULL) {
		return NULL;
	}

	if( !instance_init ) {
		instance_init = !instance_init;
		for( i=0,videodec=VIDEO_VIDEO_MODULE; i<MAX_DECODER; i++ ) {
			videodec->id = -1;
			videodec->used = 0;
			videodec++;
		}
	}

	///< find someone whether ever been used
	for( i=0,videodec = VIDEO_VIDEO_MODULE; i<MAX_DECODER; i++ ) {
		if(videodec->used && videodec->id==id){
			videodec->used++;
			return videodec;
		}
		videodec++;
	}

	///< find a free block
	for( i=0,videodec = VIDEO_VIDEO_MODULE; i<MAX_DECODER; i++ )
	{
		if( 0==videodec->used ){
			videodec->id   = id;
			videodec->used = 1;
			videodec->ops  = &vd_ops;
			break;
		}
		videodec++;
	}
	return ( MAX_DECODER==i ) ? NULL : videodec;
}

int videodec_delete_instance( struct gxav_video_dec *videodec )
{
	if(videodec->used && --videodec->used){
		return 0;
	}

	gx_memset( videodec, 0, sizeof(struct gxav_video_dec) );
	videodec->id   = -1;
	videodec->used = 0;
	return 0;
}

/*
 * 只要clr frame 1帧成功则返回0,否则-1
 */
static int videodec_clr_frame(struct gxav_video_module *module )
{
#define SRETURN(VALUE)  ((VALUE)>0?0:-1)
	int ret = 0;
	unsigned int  clr_cnt   = 0;
	unsigned int  fifo_len  = 0;
	unsigned int  max_try   = 5;  ///< 随意设置
	VideoFrameInfo frame;

	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if(videodec->state!=VIDEODEC_RUNNING && videodec->state!=VIDEODEC_STARTED) {
		return -1;
	}

	if(videodec->last_clr_frame_t !=-1 ) {
		do{
			ret = videodec->ops->h_vd_clr_frame(videodec->priv, videodec->last_clr_frame_t);
			if(0 == ret) {
				videodec->last_clr_frame_t = -1;
				clr_cnt++;
				break;
			}
		} while( --max_try );
		if( max_try==0) {
			return SRETURN(clr_cnt);
		}
	}

	if(videodec->last_clr_frame_b != -1) {
		do{
			ret = videodec->ops->h_vd_clr_frame(videodec->priv, videodec->last_clr_frame_b);
			if(0 == ret) {
				videodec->last_clr_frame_b = -1;
				clr_cnt++;
				break;
			}
		} while( --max_try );
		if( max_try==0) {
			return SRETURN(clr_cnt);
		}
	}

	if(module->back_from) {
		fifo_len = gxfifo_len(module->back_from)/sizeof(VideoFrameInfo);
		if(fifo_len) {
			do{
				gxfifo_get(module->back_from, &frame, sizeof(VideoFrameInfo) );
				if(frame.type != VFRAME_ABANDON) {
					VIDEODEC_PRINTF( "get clr frame index error \n" );
					return SRETURN(clr_cnt);
				}

				if(frame.top.fb_id != -1) {
					ret = videodec->ops->h_vd_clr_frame(videodec->priv, frame.top.fb_id);
					if(0 != ret) {
						videodec->last_clr_frame_t = frame.top.fb_id;
						videodec->last_clr_frame_b = frame.bottom.fb_id;
						//gx_printf("%s:%d, clr frame failed!\n", __func__, __LINE__);
						return SRETURN(clr_cnt);
					}
					else {
						clr_cnt++;
					}
				}

				if(frame.bottom.fb_id != -1 && frame.bottom.fb_id != frame.top.fb_id) {
					ret = videodec->ops->h_vd_clr_frame(videodec->priv, frame.bottom.fb_id);
					if(0 != ret) {
						videodec->last_clr_frame_b = frame.bottom.fb_id;
						//gx_printf("%s:%d, clr frame failed!\n", __func__, __LINE__);
						return SRETURN(clr_cnt);
					}
					else {
						clr_cnt++;
					}
				}
			} while(--fifo_len);
		}
	}

	return SRETURN(clr_cnt);
}

void videodec_callback(struct gxav_video_module *module )
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if(videodec->state!=VIDEODEC_RUNNING && videodec->state!=VIDEODEC_READY &&\
		videodec->state!=VIDEODEC_STARTED && videodec->state!=VIDEODEC_OVER) {
		return;
	}

	if(videodec->ops->h_vd_callback) {
		videodec->ops->h_vd_callback(videodec->priv);
	}
	if(0 == videodec_clr_frame(module)) {
		if(0 != videodec->ops->h_vd_decode_one_frame(videodec->priv)) {
			videodec->restart = 1;
		} else {
			videodec->restart = 0;
		}
		return ;
	}
	else if(videodec->ops->h_vd_get_fb_mask_state(videodec->priv) == FB_UNMASKED) {
		if(0 != videodec->ops->h_vd_decode_one_frame(videodec->priv)) {
			videodec->restart = 1;
		} else {
			videodec->restart = 0;
		}
		videodec->ops->h_vd_set_fb_mask_state(videodec->priv, FB_NORMAL_USEING);
		return ;
	}
	else if(videodec->restart) {
		if(0==videodec->ops->h_vd_decode_one_frame(videodec->priv)) {
			videodec->restart = 0;
		}
	}

	if(videodec->state == VIDEODEC_STARTED && videodec->dec_mode != DECODE_ONE_STOP) {
		unsigned long long disp_frame_cnt = 0;
		struct gxav_video_module *disp = ((struct gxav_video *)module->pre)->disp;
		videodisp_get_frame_cnt(disp, &disp_frame_cnt);
		if(disp_frame_cnt > 0)
			videodec->state = VIDEODEC_RUNNING;
	}
}

static int videodec_stop(struct gxav_video_module *module, int timeout)
{
	int ret = 0;
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if ( videodec->state==VIDEODEC_STOPED ) {
		return 0;
	}

	if( videodec->ops->h_vd_stop != NULL ) {
		ret = videodec->ops->h_vd_stop( videodec->priv, timeout);
		if ( 0!=ret ) {
			VIDEODEC_PRINTF( " h_vd_stop return error " );
			return -1;
		}
	}

	videodec->format = 0;
	videodec->bufid  = 0;
	videodec->bitstream_buf_addr = 0;
	videodec->bitstream_buf_size = 0;

	videodec->ptsbufid = 0;
	videodec->ptsbufaddr = 0;
	videodec->ptsbufsize = 0;

	videodec->play_bypass = 0;
	videodec->fps = 0;
	videodec->pts_insert = 0;
	videodec->dec_mode = DECODE_NORMAL;
	videodec->f_ignore = NONE_IGNORE;

	videodec->last_clr_frame_t = -1;
	videodec->last_clr_frame_b = -1;

	videodec->state = VIDEODEC_STOPED;
	videodec->update = 0;

	pm_uninit(&videodec->pts_monitor);
	ptsfixer_uninit(&videodec->pts_fixer);
	return 0;
}

static int videodec_start(struct gxav_video_module *module)
{
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);

	if(videodec->state != VIDEODEC_READY) {
		VIDEODEC_PRINTF( " must stop decoder first to start the new decoder \n " );
		return -1;
	}

	videodec->restart   = 0;
	videodec->frame_cnt = 0;
	mb_filter_init(&videodec->msc_filter);
	video_frame_convert_init(&fr_con);
	videodec->last_clr_frame_t = -1;
	videodec->last_clr_frame_b = -1;
	videodec->output_frame.type = VFRAME_NULL;
	gx_memset(&videodec->output_frame, 0, sizeof(VideoFrameInfo));

	if ( videodec->ops==NULL || videodec->ops->h_vd_config_frame_buf==NULL) {
		VIDEODEC_PRINTF( " ops or ops->h_vd_config_frame_buf is null " );
		goto err_out;
	}

	if(videodec->ops->h_vd_config_frame_buf(videodec->priv) != 0) {
		VIDEODEC_PRINTF(" ops->h_vd_start error ");
		goto err_out;
	}

	if(videodec->ops==NULL || videodec->ops->h_vd_decode_one_frame==NULL) {
		VIDEODEC_PRINTF( " ops or ops->h_vd_decode_one_frame is null " );
		goto err_out;
	}

	videodec->state = VIDEODEC_STARTED;
	if(0 != videodec->ops->h_vd_decode_one_frame(videodec->priv)) {
		videodec->restart = 1;
	}

	return 0;

err_out:
	videodec_stop(module, -1);
	return -1;
}

static int videodec_probe(struct gxav_video_module *module )
{
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);

	if(videodec->state != VIDEODEC_STOPED) {
		VIDEODEC_PRINTF( " must stop decoder first to start the new decoder \n " );
		return -1;
	}

	videodec->restart = 0;
	videodec->frame_cnt = 0;
	videodec->last_clr_frame_t = -1;
	videodec->last_clr_frame_b = -1;
	gx_memset(&videodec->h_info, 0, sizeof(videodec->h_info));

	if(videodec->ops==NULL || videodec->ops->h_vd_start==NULL) {
		VIDEODEC_PRINTF( " ops or ops->h_vd_start is null " );
		goto err_out;
	}

	if(videodec->ops->h_vd_start(videodec->priv) != 0) {
		VIDEODEC_PRINTF( " ops->h_vd_start error " );
		goto err_out;
	}

	if(videodec->ops==NULL || videodec->ops->h_vd_start_find_header==NULL) {
		VIDEODEC_PRINTF( " ops or ops->h_vd_start_find_header is null " );
		goto err_out;
	}
	videodec->ops->h_vd_start_find_header(videodec->priv);
	videodec->state = VIDEODEC_READY;
	return 0;

err_out:
	videodec_stop(module, -1);
	return -1;
}

void* videodec_open(int sub)
{
	struct gxav_video_dec *videodec = NULL;

	videodec = videodec_creat_instance(sub);
	if(videodec == NULL) {
		VIDEODEC_PRINTF( " videodec is null ,please check " );
		return NULL;
	}
	videodec->update   = 0;
	videodec->state    = VIDEODEC_STOPED;
	videodec->dec_mode = DECODE_NORMAL;
	videodec->f_ignore = NONE_IGNORE;

	if (videodec->ops==NULL || videodec->ops->h_vd_create==NULL) {
		VIDEODEC_PRINTF( " ops or ops->h_vd_creat is null " );
		goto err_out;
	}
	videodec->priv = videodec->ops->h_vd_create(sub);
	if(videodec->priv == NULL) {
		VIDEODEC_PRINTF( " videodec->ops->h_vd_create return error \n" );
		goto err_out;
	}

	return videodec;

err_out:
	videodec_delete_instance(videodec);
	return NULL;
}

int videodec_close(struct gxav_video_module *module )
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	videodec_stop( module, -1);
	if( videodec->ops && videodec->ops->h_vd_delete ) {
		videodec->ops->h_vd_delete( videodec->priv );
		videodec->priv = NULL;
	}
	else {
		VIDEODEC_PRINTF( " videodec->ops or ops->h_vd_delete is null \n" );
		return -1;
	}
	videodec_delete_instance( videodec );

	return 0;
}

static int videodec_pause(struct gxav_video_module *module )
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if( videodec->state==VIDEODEC_STOPED ) {
		VIDEODEC_PRINTF( " video decoder is stopped,please check \n" );
		return -1;
	}

	if(videodec->state == VIDEODEC_PAUSED) {
		return 0;
	}

	videodec->state_shadow = videodec->state;
	videodec->state = VIDEODEC_PAUSED;

	if( videodec->ops && videodec->ops->h_vd_pause ) {
		videodec->ops->h_vd_pause( videodec->priv );
	}
	else {
		VIDEODEC_PRINTF( " videodec->ops or ops->h_vd_pause is null \n" );
		return 0;
	}

	return 0;
}

static int videodec_resume(struct gxav_video_module *module )
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if( videodec->state == VIDEODEC_PAUSED || videodec->state == VIDEODEC_READY || videodec->state == VIDEODEC_STARTED) {
		videodec->state = videodec->state_shadow;
		if( videodec->ops && videodec->ops->h_vd_resume) {
			videodec->ops->h_vd_resume( videodec->priv );
		} else {
			VIDEODEC_PRINTF( " videodec->ops or ops->h_vd_resume is null \n" );
		}
	} else {
		VIDEODEC_PRINTF(" resume error,because state is not paused \n ");
	}

	return 0;
}

static int videodec_incallback(struct gxav_video_module *module )
{
	int ret = -1;
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);

	if (videodec && videodec->ops && videodec->ops->h_vd_get_seqstate && videodec->priv) {
		int state;
		videodec->ops->h_vd_get_seqstate(videodec->priv, &state);
		if (state<DEC_PROCESS_FINDED_HEADER && videodec->ops->h_vd_callback) {
			ret = videodec->ops->h_vd_callback(videodec->priv);
		} else {
			VIDEODEC_PRINTF( " videodec->ops or ops->h_vd_int_mask is null \n" );
		}
	}

	return ret;
}

static int videodec_get_frame_num(struct gxav_video_module *module, unsigned *num)
{
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);
	*num = videodec->frame_cnt;
	return 0;
}

int videodec_get_stream_id(struct gxav_video_module *module, unsigned *stream_id)
{
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);
	if (stream_id)
		*stream_id = videodec->stream_id;
	return 0;
}

static int videodec_get_frameinfo(struct gxav_video_module *module, struct head_info *head_info )
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	videodec->ops->h_vd_get_head_info(videodec->priv, head_info);
	head_info->width  = videodec->h_info.width;
	head_info->height = videodec->h_info.height;
	return 0;
}

int videodec_set_state(struct gxav_video_module *module, DecState *state)
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	videodec->state = *state;
	return 0;
}

static int videodec_get_state(struct gxav_video_module *module, DecState *state, DecErrCode *err_code)
{
	DecState vd_state = 0;
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if(videodec->ops->h_vd_get_state) {
		videodec->ops->h_vd_get_state(videodec->priv, &vd_state, err_code);
	}
	if(vd_state==VIDEODEC_OVER || vd_state==VIDEODEC_ERROR) {
		videodec->state = vd_state;
	}
	if(state)
		*state = videodec->state;

	return 0;
}

static int videodec_get_cap(struct gxav_video_module *module, struct h_vd_cap *cap )
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	return videodec->ops->h_vd_get_capability( videodec->priv,cap );
}

int videodec_get_framesize(struct gxav_video_module *module, int *size)
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if(videodec->ops->h_vd_get_framesize) {
		videodec->ops->h_vd_get_framesize(videodec->priv, size);
	}

	return 0;
}

int videodec_get_afd(struct gxav_video_module *module, void *afd)
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if (videodec->ops->h_vd_get_afd) {
		videodec->ops->h_vd_get_afd(videodec->priv, afd);
	}

	return 0;
}

int videodec_get_frame_cnt(struct gxav_video_module *module, unsigned long long *decode_frame_cnt, unsigned long long *abandon_frame_cnt, unsigned long long *error_frame_cnt)
{
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);

	if (decode_frame_cnt)
		*decode_frame_cnt = videodec->frame_cnt;
	mb_filter_get_process_info(&videodec->msc_filter, abandon_frame_cnt, error_frame_cnt);

	return 0;
}

int videodec_read_userdata(struct gxav_video_module *module, unsigned char *buf, int count)
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);
	int ret = 0;

	if (videodec->ops->h_vd_userdata_read)
		ret = videodec->ops->h_vd_userdata_read(videodec->priv, buf, count);

	return ret;
}

int videodec_fb_mask_require(struct gxav_video_module *module, int fb_id)
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if (videodec->ops->h_vd_fb_mask_require)
		videodec->ops->h_vd_fb_mask_require(videodec->priv, fb_id);

	return 0;
}

int videodec_fb_unmask_require(struct gxav_video_module *module, int fb_id)
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if (videodec->ops->h_vd_fb_unmask_require)
		videodec->ops->h_vd_fb_unmask_require(videodec->priv, fb_id);

	return 0;
}

static int videodec_show_log(struct gxav_video_module *module )
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if (videodec->ops->h_vd_bs_flush)
		videodec->ops->h_vd_bs_flush( videodec->priv );
	else {
		VIDEODEC_PRINTF(" h_vd_bs_flush is null,couldn't flush bs \n ");
		return -1;
	}

	return 0;
}

static int  videodec_update(struct gxav_video_module *module )
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	videodec->update = 1;
	if( videodec->ops->h_vd_bs_flush ) {
		videodec->ops->h_vd_bs_flush( videodec->priv );
	}
	else {
		VIDEODEC_PRINTF(" h_vd_bs_flush is null,couldn't flush bs \n ");
		return -1;
	}

	return 0;
}

static int videodec_config(struct gxav_video_module *module, unsigned int cmd,void *config, unsigned int size)
{
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if(videodec->ops->h_vd_config==NULL || config==NULL) {
		return 0;
	}

	videodec->ops->h_vd_config(videodec->priv, cmd, config, size);
	switch (cmd)
	{
		case VIDEO_CONFIG_PARAM:
			{
				struct h_vd_video_fifo *fifo    = (struct h_vd_video_fifo *)config;
				videodec->bufid                 = fifo->bs_id;
				videodec->bitstream_buf_addr    = fifo->bitstream_buf_addr;
				videodec->bitstream_buf_size    = fifo->bitstream_buf_size;

				videodec->ptsbufid   = fifo->pts_id;
				videodec->ptsbufaddr = fifo->pts_buf_addr;
				videodec->ptsbufsize = fifo->pts_buf_size;
				videodec->ops->h_vd_config(videodec->priv, VIDEO_CONFIG_PARAM, fifo, sizeof(struct h_vd_video_fifo));
				break;
			}
		case VIDEO_CONFIG_FORMAT:
			{
				videodec->format = *(decoder_type*)config;
				break;
			}
		case VIDEO_CONFIG_FRAME_IGNORE:
			{
				videodec->f_ignore = *(frame_ignore*)config;
				break;
			}
		case VIDEO_CONFIG_DEC_MODE:
			{
				videodec->dec_mode = *(decode_mode*)config;
				break;
			}
		case VIDEO_CONFIG_UPDATE:
			{
				if(*(int*)config != 1) {
					return -1;
				}
				videodec_update( module );
				break;
			}
		case VIDEO_CONFIG_SHOW_LOGO:
			{
				if((*(int*)config != 1)) {
					return -1;
				}
				videodec_show_log( module );
				break;
			}
		case VIDEO_CONFIG_MOSAIC_DROP_GATE:
			{
				mb_filter_set_mosaic_drop_gate(&videodec->msc_filter, *(unsigned int*)config);
				break;
			}
		case VIDEO_CONFIG_MOSAIC_FREEZE_GATE:
			{
				mb_filter_set_mosaic_freeze_gate(&videodec->msc_filter, *(unsigned int*)config);
				break;
			}
		case VIDEO_CONFIG_PLAYBYPASS:
			{
				videodec->play_bypass = *(unsigned int*)config;
				break;
			}
		default:
			break;
	}

	return 0;
}

int videodec_get_decmode(struct gxav_video_module *module, int *dec_mode)
{
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);
	*dec_mode = videodec->dec_mode;
	return 0;
}

#define DEC_PUSH_FRAME(desc) \
	do {\
		output_frame->type = VFRAME_DECODE;\
		output_frame->disp_num  = ++videodec->frame_cnt;\
		output_frame->stream_id = videodec->stream_id;\
		VDBG_PRINT_FRAME_INFO(output_frame);\
		if(videodec_frame_filter(module, output_frame) == 0) {\
			gxfifo_put(module->out, output_frame, sizeof(VideoFrameInfo));\
			VDBG_PRINT_FRAME_FLOW("dec put", output_frame);\
		}\
		ret = 0;\
		output_frame->top.ready = output_frame->bottom.ready = 0;\
		output_frame->type = VFRAME_NULL;\
	}while(0)

static int videodec_frame_filter(struct gxav_video_module *module, VideoFrameInfo *frame)
{
	int ret = 0;
	int msc_drop = 0, need_skip = 0, skip_able = 0;
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);

	int is_key_pts = (frame->pic_type==I_FRAME && frame->pts!=0 && frame->num_of_err_mb==0);
	unsigned fix_pts = ptsfixer_run(&videodec->pts_fixer, frame->pts, frame->fps, is_key_pts);

#if 0
	if(videodec->h_info.movie_mode || videodec->h_info.terrible_pts) {
		video_sync_strict_enable(&av_sync[0], 1);
		video_frame_convert_set_mode(&fr_con, FRMODE_BYPASS);
	} else if(frame->fps) {
		//pts fix
		video_sync_strict_enable(&av_sync[0], 0);
		video_frame_convert_set_mode(&fr_con, FRMODE_NORMAL);
		VDBG_PRINT_PTSFIXER_INFO(frame->pts, fix_pts);
		frame->pts = fix_pts;
	}
#else
	if (frame->pts == 0)
		frame->pts = fix_pts;
#endif

	if(frame->num_of_err_mb) {
		// gx_printf("%u msc!\n", videodec->frame_cnt+1);
		VDBG_PRINT_FRAME_INFO(frame);
	}

	//msc drop
	msc_drop = mb_filter_run(&videodec->msc_filter, frame->pic_type, frame->width, frame->height, frame->num_of_err_mb);
	if(msc_drop) {
		// gx_printf("msc drop\n");
	}

	//fr_skip
	need_skip = (video_frame_convert_run(&fr_con, frame->disp_num) == FR_SKIP);
	//check skip able
	skip_able = videodec->ops->check_skip_able(videodec->priv);

	if(msc_drop || (need_skip && !skip_able)) {
		ret = -1;
		videodec->ops->h_vd_clr_frame(videodec->priv, frame->top.fb_id);
		if(frame->top.fb_id != frame->bottom.fb_id) {
			videodec->ops->h_vd_clr_frame(videodec->priv, frame->bottom.fb_id);
		}
	}

	return ret;
}

#if 0
static char *store_str[] = {
	"none(default)",
	"frame",
	"field-t",
	"field-b",
};
#endif
static void abandon_frame(struct gxav_video_module *module, struct h_vd_dec_info  *dec_info)
{
	VideoFrameInfo output_frame = {0};

	output_frame.type = VFRAME_ABANDON;
	output_frame.top.fb_id = dec_info->index_frame_disp;
	output_frame.bottom.fb_id = dec_info->index_frame_disp;
	/* VDBG_PRINT_FRAME_INFO(output_frame);*/
	gxfifo_put(module->back_from, &output_frame, sizeof(VideoFrameInfo));
}

static int push_frame(struct gxav_video_module *module, struct h_vd_dec_info *dec_info)
{
	int ret = -1, stride;
	unsigned base_height = 0;
	VideoFrameInfo *output_frame = NULL;
	struct field_addr     *top    = NULL;
	struct field_addr     *bottom = NULL;
	struct gxav_video_dec *videodec = GET_VIDEO_DEC(module);
	StoreMode cur_store_mode = dec_info->store_mode;

	stride       = videodec->h_info.stride;
	base_height  = videodec->h_info.base_height;
	output_frame = &videodec->output_frame;
	top          = &output_frame->top;
	bottom       = &output_frame->bottom;

	output_frame->store_mode    = dec_info->store_mode;
	output_frame->bpp           = dec_info->bpp;
	output_frame->stride        = stride;
	output_frame->width         = dec_info->width;
	output_frame->height        = dec_info->height;
	output_frame->clip          = dec_info->clip;
	output_frame->pts           = (dec_info->pts&0x1 ? dec_info->pts : 0);
	output_frame->interlace     = dec_info->type;
	output_frame->top_first     = dec_info->top_field_first;
	output_frame->num_of_err_mb = dec_info->num_of_err_mb;
	output_frame->ref_cnt       = 0;
	output_frame->fps           = dec_info->rate;
	output_frame->repeat_first_field = dec_info->repeat_first_field;
	output_frame->colorimetry   = dec_info->colorimetry;

	pm_run(&videodec->pts_monitor, output_frame->pts, output_frame->fps);
	videodec->h_info.movie_mode   = pm_is_mov(&videodec->pts_monitor);
	videodec->h_info.terrible_pts = pm_is_wave(&videodec->pts_monitor);

	if ( !top->ready && !bottom->ready) {
		output_frame->pic_type = dec_info->pic_type;
	}
	top->is_ref = bottom->is_ref = 1;

	if (cur_store_mode == STORE_FRAME) {
		top->phys.addry       = dec_info->frame_info.phys.bufY;
		top->phys.addrcb      = dec_info->frame_info.phys.bufCb;
		top->phys.addry_2bit  = dec_info->frame_info.phys.bufY  + stride*base_height;
		top->phys.addrcb_2bit = dec_info->frame_info.phys.bufCb + stride*(base_height>>1);
		bottom->phys.addry    = dec_info->frame_info.phys.bufY + stride;
		bottom->phys.addrcb   = dec_info->frame_info.phys.bufCb+ stride;
		bottom->phys.addry_2bit  = top->phys.addry_2bit  + stride;
		bottom->phys.addrcb_2bit = top->phys.addrcb_2bit + stride;

		top->virt.addry       = dec_info->frame_info.virt.bufY;
		top->virt.addrcb      = dec_info->frame_info.virt.bufCb;
		top->virt.addry_2bit  = dec_info->frame_info.virt.bufY  + stride*base_height;
		top->virt.addrcb_2bit = dec_info->frame_info.virt.bufCb + stride*(base_height>>1);
		bottom->virt.addry    = dec_info->frame_info.virt.bufY  + stride;
		bottom->virt.addrcb   = dec_info->frame_info.virt.bufCb + stride;
		bottom->virt.addry_2bit  = top->virt.addry_2bit  + stride;
		bottom->virt.addrcb_2bit = top->virt.addrcb_2bit + stride;

		output_frame->clip_width  = dec_info->clip.width;
		output_frame->clip_height = dec_info->clip.height;

		top->fb_id    = dec_info->index_frame_disp;
		bottom->fb_id = dec_info->index_frame_disp;

		if(dec_info->dar != videodec->h_info.ratio) {
			gx_video_set_stream_ratio(dec_info->dar);
		}
		if (dec_info->userdata.enable) {
			output_frame->userdata.data    = dec_info->userdata.data;
			output_frame->userdata.enable  = dec_info->userdata.enable;
			output_frame->userdata.display = dec_info->userdata.display;
		}
		top->ready = bottom->ready = 1;
		DEC_PUSH_FRAME("frame: normal\n");
	} else if(cur_store_mode == STORE_SINGLE_FIELD_T || cur_store_mode == STORE_SINGLE_FIELD_B) {
		struct field_addr *this_field = NULL;
		output_frame->fps    = dec_info->rate/2;
		output_frame->height = dec_info->height;
		output_frame->clip_width  = dec_info->width;
		output_frame->clip_height = dec_info->height;
		if (videodec->h_info.height != dec_info->clip.height*2) {
			videodec->h_info.width  = dec_info->clip.width;
			videodec->h_info.height = dec_info->clip.height*2;
		}

		if (cur_store_mode == STORE_SINGLE_FIELD_T) {
			this_field = top;
		} else {
			this_field = bottom;
		}

		if (this_field->ready) {
			//conflict, means lost some field, force repeat
			output_frame->top    = *this_field;
			output_frame->bottom = *this_field;
			DEC_PUSH_FRAME("field: lost some field\n");
		}
		this_field->fb_id            = dec_info->index_frame_disp;
		this_field->phys.addry       = dec_info->frame_info.phys.bufY;
		this_field->phys.addrcb      = dec_info->frame_info.phys.bufCb;
		this_field->phys.addry_2bit  = dec_info->frame_info.phys.bufY  + stride*base_height;
		this_field->phys.addrcb_2bit = dec_info->frame_info.phys.bufCb + stride*(base_height>>1);
		this_field->virt.addry  = dec_info->frame_info.virt.bufY;
		this_field->virt.addrcb = dec_info->frame_info.virt.bufCb;
		this_field->virt.addry_2bit  = dec_info->frame_info.virt.bufY  + stride*base_height;
		this_field->virt.addrcb_2bit = dec_info->frame_info.virt.bufCb + stride*(base_height>>1);
		this_field->ready = 1;

		if (top->ready && bottom->ready) {
			//normal
			DEC_PUSH_FRAME("field: normal\n");
		} else if (dec_info->top_field_first == 1 && cur_store_mode == STORE_SINGLE_FIELD_B && top->ready == 0) {
			//some special situation
			output_frame->top = *bottom;
			DEC_PUSH_FRAME("field: force rpt bottom\n");
		} else if (dec_info->top_field_first == 0 && cur_store_mode == STORE_SINGLE_FIELD_T && bottom->ready == 0) {
			output_frame->bottom = *top;
			DEC_PUSH_FRAME("field: force rpt top\n");
		}
	} else {
		gx_printf("\n%s:%d, bad store_mode, decoder err!\n", __func__, __LINE__);
		videodec->ops->h_vd_clr_frame(videodec->priv, dec_info->index_frame_disp);
	}

#ifdef DBG_SWITCH_PROGRAM
	if(videodec->frame_cnt == 1)
		decode_first = gx_current_tick();
#endif

	return ret;
}

int videodec_boda_isr(struct gxav_video_module *module, int irq, h_vd_int_type *int_type)
{
	int ret  = 0;
	struct h_vd_dec_info   dec_info;
	struct gxav_video_dec  *videodec = GET_VIDEO_DEC(module);

	if ( videodec->ops==NULL ) {
		VIDEODEC_PRINTF( " videodec->ops or ops->h_vd_get_int_type==NULL \n");
		return -1;
	}

	if (videodec->ops->h_vd_get_int_type!=NULL ) {
		*int_type = videodec->ops->h_vd_get_int_type( videodec->priv );
	}

	if ( videodec->ops->interrupts != NULL ) {
		gx_memset( &dec_info,-1,sizeof(struct h_vd_dec_info) );
		ret = videodec->ops->interrupts( videodec->priv, int_type, &dec_info );
	}

	switch(*int_type) {
		case VD_SEQ_OVER:
		case VD_SEQ_OVER_EX:
		case VD_SEQ_CHANGED:
			if(ret == 0) {
				videodec->state_shadow = videodec->state = VIDEODEC_READY;
				videodec->ops->h_vd_get_head_info(videodec->priv, &(videodec->h_info));
				gx_video_set_stream_ratio(videodec->h_info.ratio);
				videodec_fb_mask_require(module, FREEZE_FB_ID);
				videodec->stream_id++;
			} else if(ret == -1 || ret == -2) {
				videodec->state = VIDEODEC_ERROR;
			}
			break;
		case VD_DECODE_OVER:
			videodec->restart = (ret==0);
			if(dec_info.index_frame_disp >= 0) {
				if (videodec->dec_mode == DECODE_ONE_STOP) {
					//X speed
					if (videodec->state == VIDEODEC_STARTED)
						videodec->state = VIDEODEC_RUNNING;
					if (videodec->update)
						abandon_frame(module, &dec_info);
					else {
						ret = push_frame(module, &dec_info);
						if (ret == 0)
							videodec_pause(module);
					}
				} else {
					//normal speed
					if (videodec->play_bypass)
						abandon_frame(module, &dec_info);
					else
						ret = push_frame(module, &dec_info);
				}
			} else {
				ret = -1;
			}
			videodec_callback(module);
			break;
		default:
			break;
	}

	return ret;
}

struct video_ops video_dec_ops = {
	.open           = videodec_open,
	.stop           = videodec_stop,
	.run            = videodec_start,
	.probe          = videodec_probe,
	.close          = videodec_close,
	.pause          = videodec_pause,
	.resume         = videodec_resume,
	.get_frameinfo  = videodec_get_frameinfo,
	.get_frame_num  = videodec_get_frame_num,
	.get_state      = videodec_get_state,
	.get_cap        = videodec_get_cap,
	.config         = videodec_config,
	.get_decmode    = videodec_get_decmode,
	.in_callback    = videodec_incallback,
};

