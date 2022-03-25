#include "video.h"
#include "video_decode.h"
#include "pp_core.h"
#include "video_display.h"
#include "porting.h"
#include "stc_hal.h"
#include "vout_hal.h"
#include "vpu_hal.h"
#include "sdc_hal.h"
#include "firewall.h"

static struct gxav_video_disp video_disp[MAX_DISP] = {{0}};

#ifdef CONFIG_AV_ENABLE_BFRAME_FREEZE
volatile int bbt_freezed = 0;
static struct freeze_node {
	int fz_pic_type;
	int fz_pic_id;
	int fz_pic_cnt;
}bbt_freeze;

static void bbtfz_init(struct freeze_node *thiz)
{
	if(thiz) {
		thiz->fz_pic_type = B_FRAME;
		thiz->fz_pic_id   = 1;
		thiz->fz_pic_cnt  = 0;
		bbt_freezed = 0;
	}
}

static int bbtfz_check_freezed(struct freeze_node *thiz, VideoFrameInfo *frame)
{
	int freezed = 0;

	if(thiz && frame) {
		if(thiz->fz_pic_cnt < thiz->fz_pic_id) {
			thiz->fz_pic_cnt += (frame->pic_type == thiz->fz_pic_type);
			freezed = 0;
		}
		else {
			freezed = 1;
			bbt_freezed = 1;
		}
	}

	return freezed;
}
#endif


static int videodisp_display(struct gxav_video_module *module,
		VideoFrameInfo  *to_disp,
		DispMode disp_mode, int freeze_frame)
{
	int ret = -1;
	int dec_mode;
	unsigned int same_flag, stream_id;
	struct frame frame = {0};
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	if (to_disp->type<VFRAME_DECODE || to_disp->type>VFRAME_ZOOM)
		return -1;

#ifdef DBG_SWITCH_PROGRAM
	if(disp->play_frame_cnt == 0) {
		display_first = gx_current_tick();
		gx_printf("\n-----------------\n");
		gx_printf("head   : %lld\n", head_end      - head_start);
		gx_printf("decode : %lld\n", decode_first  - head_end);
		gx_printf("sync   : %lld\n", display_first - decode_first);
		gx_printf("totle  : %lld\n", display_first - head_start);
		gx_printf("-----------------\n");
	}
#endif

	frame.w = to_disp->clip_width;
	frame.h = to_disp->clip_height;
	frame.baseline          = to_disp->stride;
	frame.topfirst          = to_disp->top_first;
	frame.bpp               = to_disp->bpp;
	frame.is_image          = 0;
	frame.is_freeze         = freeze_frame;
	frame.stream_id         = to_disp->stream_id;
	frame.colorimetry       = to_disp->colorimetry;
	frame.top.addry         = to_disp->top.phys.addry;
	frame.top.addrcbcr      = to_disp->top.phys.addrcb;
	frame.top.addry_2bit    = to_disp->top.phys.addry_2bit;
	frame.top.addrcbcr_2bit = to_disp->top.phys.addrcb_2bit;
	frame.bot.addry         = to_disp->bottom.phys.addry;
	frame.bot.addrcbcr      = to_disp->bottom.phys.addrcb;
	frame.bot.addry_2bit    = to_disp->bottom.phys.addry_2bit;
	frame.bot.addrcbcr_2bit = to_disp->bottom.phys.addrcb_2bit;
	frame.disp_num = disp->play_frame_cnt;
#if 0
	if (to_disp->type==VFRAME_DEINTERLACE_YUV || to_disp->type==VFRAME_DEINTERLACE_Y) {
		frame.top.addry    = to_disp->top.phys.addry     + 2*frame.baseline;
		frame.top.addrcbcr = to_disp->top.phys.addrcb    + 1*frame.baseline;
		frame.bot.addry    = to_disp->bottom.phys.addry  + 2*frame.baseline;
		frame.bot.addrcbcr = to_disp->bottom.phys.addrcb + 1*frame.baseline;
		frame.h -= 2;
	}
#endif
	videodec_get_decmode(((struct gxav_video*)module->pre)->dec, &dec_mode);
	if (to_disp->type==VFRAME_DEINTERLACE_Y)
		frame.playmode = PLAY_MODE_Y_FRAME_UV_FIELD;
	else if (to_disp->type==VFRAME_DEINTERLACE_YUV)
		frame.playmode = PLAY_MODE_FRAME;
	else {
		if (disp_mode == DISP_MODE_AUTO && to_disp->interlace == SCAN_INTERLACE && disp->fps <= FRAME_RATE_30) {
			frame.playmode = PLAY_MODE_FIELD;
			if (dec_mode == DECODE_ONE_STOP || dec_mode == DECODE_FAST) {
				frame.top = frame.bot;
				frame.playmode = PLAY_MODE_FRAME;
			}
		} else {
			frame.playmode = PLAY_MODE_FRAME;
		}
	}
	ret = gxav_vpu_vpp_play_frame(&frame);
	if (ret == 0)
		disp->play_frame_cnt++;

	videodec_get_stream_id(((struct gxav_video*)module->pre)->dec, &stream_id);
	same_flag = (stream_id == to_disp->stream_id);
	if (freeze_frame == 0 && (same_flag == 1 && disp->last_same_flag == 0))
		disp->freeze_state = FZ_UNFREEZE_REQUIRED;

	disp->last_frame = *to_disp;
	VDBG_PRINT_FRAME_FLOW("disp disp", to_disp);
	return ret;
}

static struct gxav_video_disp *videodisp_creat_instance( int sub )
{
	int i = 0;
	struct gxav_video_disp *disp = video_disp;
	static  unsigned int instance_init = 0;

	if(disp == NULL) {
		return NULL;
	}

	if( !instance_init ) {
		instance_init = ~instance_init;
		for( i=0, disp=video_disp; i<MAX_DISP; i++ ) {
			disp->id   = -1;
			disp->used = 0;
			disp++;
		}
	}

	///< find ever been used
	for( i=0,disp=video_disp; i<MAX_DISP; i++ ) {
		if(disp->used && disp->id==sub){
			disp->used++;
			return disp;
		}
		disp++;
	}

	for( i=0,disp=video_disp; i<MAX_DISP; i++ ) {
		if( 0==disp->used ) {
			disp->id = sub;
			disp->used = 1;
			break;
		}
		disp++;
	}
	return ( MAX_DISP==i ) ? NULL : disp;
}

static int videodisp_delete_instance( struct gxav_video_disp *disp )
{
	if(disp->used && --disp->used){
		return 0;
	}
	gx_memset( disp, 0, sizeof(struct gxav_video_disp) );
	disp->id   = -1;
	disp->used = 0;
	return 0;
}

static void* videodisp_open(int sub)
{
	unsigned int cc_fifo_size = 0;
	struct gxav_video_disp *video_disp;

	video_disp = videodisp_creat_instance( sub );

	if(video_disp != NULL) {
		video_disp->hw_vpu_show_enable = 0;
		video_disp->need_drop_firstint_after_reset = 1;
		gxav_vpu_disp_set_rst();
		{
			int sl = 100;
			do{
				sl = gxav_vpu_disp_get_view_active_cnt();
				if(sl < 50){
					gx_mdelay(10);
				}
			}while(sl < 50);
		}
		gxav_vpu_disp_clr_rst();
	}

	video_disp->state           = DISP_STOPED;
	video_disp->state_shadow    = DISP_STOPED;
	//init cc_fifo
	cc_fifo_size = CC_FIFO_LENGTH*(sizeof(CcPerFrame));
	gxfifo_init(&video_disp->cc_fifo, NULL, cc_fifo_size);

	video_disp->freeze_en    =  0;
	video_disp->freeze_state = FZ_NO_REQUIRE;

	return video_disp;
}

static int videodisp_start(struct gxav_video_module *module)
{
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	if (disp == NULL) {
		VIDEODEC_PRINTF (" pointer disp is null\n");
		return -1;
	}

	disp->hd_init            = 0;
	disp->pp_enable          = 0;
	disp->hw_vpu_show_enable = 0;

	disp->cc_enable = 0;
	gxfifo_reset(&disp->cc_fifo);

	disp->freeze_state = FZ_FREEZED;
	disp->state        = DISP_READY;
	disp->state_shadow = DISP_READY;

	disp->bypass             = 0;
	disp->disp_num           = 0;
	disp->update_timeout     = 0;
	disp->sync_disp.pts      = 0;
	disp->sync_disp.last_pts = 0;
	disp->play_frame_cnt     = 0;

	gx_memset(&disp->sync_disp,  0, sizeof(disp->sync_disp));
	gx_memset(&disp->last_frame, 0, sizeof(disp->last_frame));

	gxav_vpu_disp_clr_field_start_int();
	gxav_vpu_disp_field_start_int_en();
	return 0;
}

static void frame_copy(VideoFrameInfo *dst, VideoFrameInfo *src)
{
	unsigned int i = 0;
	unsigned int dst_stride = 0;
	struct field_addr top = {0}, bot = {0};

	if (src && dst) {
		top = dst->top;
		bot = dst->bottom;
		dst_stride = dst->stride;
		gx_memcpy(dst, src, sizeof(VideoFrameInfo));
		dst->stride = dst_stride;
		if (src->store_mode != STORE_FRAME && src->top.fb_id == 0 && src->bottom.fb_id == 1)
			return;
		dst->top = top, dst->bottom = bot;

		for (i = 0; i < src->height; i += 2) {
			memmove((void*)(dst->top.virt.addry+i*dst->stride),
					(void*)(src->top.virt.addry+i*src->stride),
					src->width);
#ifdef LINUX_OS
			if (i%64 == 0)
				cond_resched();
#endif
		}
		for (i = 0; i < src->height; i += 2) {
			memmove((void*)(dst->bottom.virt.addry+i*dst->stride),
					(void*)(src->bottom.virt.addry+i*src->stride),
					src->width);
#ifdef LINUX_OS
			if (i%64 == 0)
				cond_resched();
#endif
		}
		for (i = 0; i < src->height/2; i += 2) {
			memmove((void*)(dst->top.virt.addrcb+i*dst->stride),
					(void*)(src->top.virt.addrcb+i*src->stride),
					src->width);
#ifdef LINUX_OS
			if (i%64 == 0)
				cond_resched();
#endif
		}
		for (i = 0; i < src->height/2; i += 2) {
			memmove((void*)(dst->bottom.virt.addrcb+i*dst->stride),
					(void*)(src->bottom.virt.addrcb+i*src->stride),
					src->width);
#ifdef LINUX_OS
			if (i%64 == 0)
				cond_resched();
#endif
		}
		gx_dcache_clean_range(0, 0);
	}
}

static int vfreeze_alloc_framebuf(struct gxav_video_module *module, struct frame_info *frame)
{
	int ret = -1;
	unsigned width = 0, height = 0;
	struct fb_desc desc = {0};
	VideoFrameInfo *showing = NULL;
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	showing = &disp->sync_disp.showing;

	if (showing->type != VFRAME_NULL) {
		width  = showing->stride;
		height = showing->height;
	}

	if (width && height) {
		struct frame_buffer *fb = &disp->freeze_fb;
		desc.type   = FB_FREEZE;
		desc.mode   = FB_FRAME;
		desc.num    = 1;
		desc.align  = 64;
		desc.fbs    = fb;
		desc.bpp    = showing->bpp;
		desc.width  = showing->stride;
		desc.height = SIZE_ALIGN(height, 64);
		if (fb_alloc(&desc) == desc.num) {
			gx_memcpy(frame, showing, sizeof(VideoFrameInfo));
			frame->top.phys.addry     = fb->phys.bufY;
			frame->top.phys.addrcb    = fb->phys.bufCb;
			frame->bottom.phys.addry  = fb->phys.bufY  + width;
			frame->bottom.phys.addrcb = fb->phys.bufCb + width;
			frame->top.virt.addry     = fb->virt.bufY;
			frame->top.virt.addrcb    = fb->virt.bufCb;
			frame->bottom.virt.addry  = fb->virt.bufY  + width;
			frame->bottom.virt.addrcb = fb->virt.bufCb + width;
			frame->top.fb_id = frame->bottom.fb_id = FREEZE_FB_ID;
			disp->freeze_w = showing->clip_width;
			disp->freeze_h = showing->clip_height;
			ret = 0;
		}
	}

	return ret;
}

static void vfreeze_mask_framebuf(struct gxav_video_module *module)
{
	videodec_fb_mask_require(((struct gxav_video*)module->pre)->dec, FREEZE_FB_ID);
}

static void frame_simp_deinterlace(struct frame_info *frame)
{
	if (CHIP_IS_GX3113C == 0 && (frame->type != VFRAME_DEINTERLACE_Y && frame->type != VFRAME_DEINTERLACE_YUV) && frame->interlace == SCAN_INTERLACE) {
		frame->top.phys.addry  = frame->bottom.phys.addry;
		frame->top.phys.addrcb = frame->bottom.phys.addrcb;
	}

}

static void vfreeze_copy_frame(struct frame_info *dst, struct frame_info *src)
{
	if (src && dst) {
		if (src->type==VFRAME_DECODE && src->top.fb_id==FREEZE_FB_ID && src->bottom.fb_id==FREEZE_FB_ID) {
			*dst = *src;
			return;
		}
		frame_copy(dst, src);
		frame_simp_deinterlace(dst);
	}
}

static void vfreeze_fill_frame(struct frame_info *frame, char color_y, char color_cbcr)
{
	if (frame) {
		gx_memset((void*)frame->top.virt.addry,  color_y,    frame->width*frame->height);
#ifdef LINUX_OS
		cond_resched();
#endif
		gx_memset((void*)frame->top.virt.addrcb, color_cbcr, frame->width*frame->height/2);
		gx_dcache_clean_range(0,0);
	}
}

static void vfreeze_play(struct gxav_video_module *module, struct frame_info *frame)
{
	DispMode disp_mode;
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;
	struct frame_info *showing = &disp->sync_disp.showing;

	//display freeze
	if(!(showing->type==VFRAME_DECODE && showing->top.fb_id==FREEZE_FB_ID && showing->bottom.fb_id==FREEZE_FB_ID)) {
		disp_mode = (CHIP_IS_GX3113C == 1) ? DISP_MODE_FRAME : DISP_MODE_AUTO;
		gx_printf("\nfreeze\n");
		videodisp_display(module, frame, disp_mode, 1);
		gx_mdelay(20);
	}
	disp->freeze_state = FZ_FREEZED;
}

static int videodisp_stop(struct gxav_video_module *module)
{
	VideoFrameInfo *freeze = NULL;
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	freeze = &disp->freeze_frame;
	if(disp==NULL || disp->state==DISP_STOPED) {
		VIDEODEC_PRINTF (  " pointer disp is null\n " );
		return -1;
	}

	if (CHIP_IS_GX3113C == 1) {
		if(disp->freeze_en) {
			disp->freeze_state = FZ_REQUIRED;
			while(disp->freeze_state == FZ_REQUIRED);
		}
	}
	disp->cc_enable = 0;
	gxfifo_reset(&disp->cc_fifo);

	gxav_vpu_disp_field_start_int_dis();
	gxav_vpu_disp_clr_field_start_int();

	disp->update       = 0;
	disp->state        = DISP_STOPED;
	disp->state_shadow = DISP_STOPED;

	if (gxav_firewall_buffer_protected(GXAV_BUFFER_VIDEO_FRAME) == 0) {
		if (0 == vfreeze_alloc_framebuf(module, freeze)) {
			vfreeze_mask_framebuf(module);
			if (disp->freeze_en && disp->bypass == 0) {
				vfreeze_copy_frame(freeze, &disp->sync_disp.showing);
			} else {
				char color_y    = 0x10;
				char color_cbcr = 0x80;
				vfreeze_fill_frame(freeze, color_y, color_cbcr);
			}
			vfreeze_play(module, freeze);
		}
	}

	//clear all state
	disp->last_same_flag     = 0;
	disp->play_frame_cnt     = 0;
	disp->hw_vpu_show_enable = 0;
	gx_memset(&disp->sync_disp,  0, sizeof(disp->sync_disp));
	gx_memset(&disp->last_frame, 0, sizeof(disp->last_frame));
	gxav_vpu_disp_clr_field_start_int();
	gxav_vpu_disp_field_start_int_en();

	return 0;
}

static int videodisp_close(struct gxav_video_module *module)
{
	int ret = 0;
	struct gxav_video_disp *video_disp = (struct gxav_video_disp *)module->priv;

	videodisp_stop(module);
	gxfifo_free(&video_disp->cc_fifo);
	video_disp->state        = DISP_STOPED;
	video_disp->state_shadow = DISP_STOPED;
	ret = videodisp_delete_instance( video_disp );
	GX_VIDEO_CHECK_RET(ret);
	return 0;
}

static void videodisp_set_state(struct gxav_video_disp *video_disp, int state )
{
	video_disp->state         = state;
	video_disp->state_shadow  = state;
}

static int videodisp_get_state(struct gxav_video_module *module, int *state)
{
	struct gxav_video_disp *video_disp = (struct gxav_video_disp *)module->priv;

	if( video_disp==NULL ) {
		VIDEODEC_PRINTF (  " pointer disp is null\n " );
		return -1;
	}
	*state = video_disp->state;
	return 0;
}

static int videodisp_pause (struct gxav_video_module *module)
{
	struct gxav_video_disp *video_disp = (struct gxav_video_disp *)module->priv;

	if(video_disp == NULL) {
		VIDEODEC_PRINTF (  " pointer disp is null\n " );
		return -1;
	}
	if(video_disp->state== DISP_RUNNING) {
		video_disp->state_shadow = video_disp->state;
		video_disp->state        = DISP_PAUSED;
	}
	return 0;
}

static int videodisp_resume (struct gxav_video_module *module)
{
	struct gxav_video_disp *video_disp = (struct gxav_video_disp *)module->priv;

	if(video_disp == NULL) {
		VIDEODEC_PRINTF (  " pointer disp is null\n " );
		return -1;
	}
	if(video_disp->state == DISP_PAUSED) {
		video_disp->state = video_disp->state_shadow;
	}
	return 0;
}



static int videodisp_config(struct gxav_video_module *module, unsigned int cmd, void *config, unsigned int size)
{
	struct gxav_video_disp *video_disp = (struct gxav_video_disp *)module->priv;

	if(video_disp==NULL ){
		VIDEODEC_PRINTF (  " pointer disp is null\n " );
		return -1;
	}

	switch(cmd)
	{
	case DISP_CONFIG_UPDATE:
		if(*(unsigned int*)config)
			video_disp->update = 1;
		else
			return -1;
		break;
	case DISP_CONFIG_FREEZE_FRAME:
		video_disp->freeze_en = *(unsigned int*)config;
		break;
	default:
		break;
	}
	return 0;
}

static int videodisp_pp_up(struct gxav_video_module *module, unsigned int *up)
{
	struct gxav_video_disp *video_disp = (struct gxav_video_disp *)module->priv;

	if(video_disp==NULL) {
		VIDEODEC_PRINTF (  " pointer disp is null\n " );
		return -1;
	}
	*up = (video_disp->hw_vpu_show_enable>0)?1:0;
	return 0;
}

static void videodisp_clr_frame(struct gxav_video_module *module, VideoFrameInfo *frame)
{
	int same_flag = 0;
	unsigned int stream_id;
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	if (frame->type != VFRAME_NULL) {
		videodec_get_stream_id(((struct gxav_video*)module->pre)->dec, &stream_id);
		same_flag = (stream_id == frame->stream_id);
#if 0
		if (same_flag == 1 && disp->last_same_flag == 0) {
			videodec_fb_unmask_require(((struct gxav_video*)module->pre)->dec, FREEZE_FB_ID);
			disp->freeze_state = FZ_NO_REQUIRE;
		}
#endif
		if (same_flag == 1) {
			VDBG_PRINT_FRAME_FLOW("disp clr", frame);
			videopp_clr_frame(((struct gxav_video*)module->pre)->pp, frame);
			//int is_top_field = gxav_vpu_disp_get_view_field();
			//gx_printf("tf = %d, clr at %c\n", frame->top_first, is_top_field ? 'T' : 'B');
		}
		frame->type = VFRAME_NULL;
		disp->last_same_flag = same_flag;
	}
}

static int vpu_get_frame(struct gxav_video_module *module, VideoFrameInfo *new_get)
{
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	disp->disp_num++;
	new_get->type = VFRAME_NULL;
	new_get->disp_num = disp->disp_num;

	videopp_get_frame(((struct gxav_video*)module->pre)->pp, new_get);
	if(new_get->type != VFRAME_NULL) {
		VDBG_PRINT_FRAME_FLOW("disp get", new_get);
#ifdef CONFIG_AV_ENABLE_BFRAME_FREEZE
		if(bbtfz_check_freezed(&bbt_freeze, new_get) == 1)
			videodisp_clr_frame(module, new_get);
#endif
		return 0;
	} else if(disp->update) {
		if(++(disp->update_timeout) >= 30) {
			struct gxav_video_module *dec = ((struct gxav_video *)module->pre)->dec;
			DecState state = VIDEODEC_OVER;
			disp->update = 0;
			videodec_set_state(dec, &state);
		}
	}

	return -1;
}

static int videodisp_play_cc(struct gxav_video_module *module, unsigned int is_top_field)
{
	short i = 0, cc_type;
	unsigned int fifo_len = 0, len = 0;
	static CcPerFrame frame_cc = {0};
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;
	struct vout_ccinfo ccinfo;

	if(frame_cc.top.data_num==0 && frame_cc.bottom.data_num==0) {
		fifo_len = gxfifo_len(&disp->cc_fifo)/sizeof(CcPerFrame);
		if(fifo_len) {
			len = gxfifo_get(&disp->cc_fifo, &frame_cc, sizeof(CcPerFrame));
			if(len != sizeof(CcPerFrame)) {
				gx_printf("get cc data from fifo error!\n");
				return -1;
			}
		}
		else
			return -1;
	}

	if(is_top_field && frame_cc.top.data_num) {
		cc_type = 0x00;//top field
		for(i = 0; i < frame_cc.top.data_num; i++) {
			ccinfo.cc_type = cc_type;
			ccinfo.cc_data = frame_cc.top.data[i];
			gxav_videoout_play_cc(0, &ccinfo);
		}
		frame_cc.top.data_num = 0;
	}
	if((!is_top_field) && frame_cc.bottom.data_num) {
		cc_type = 0x01;//bottom field
		for(i = 0; i < frame_cc.bottom.data_num; i++) {
			ccinfo.cc_type = cc_type;
			ccinfo.cc_data = frame_cc.bottom.data[i];
			gxav_videoout_play_cc(0, &ccinfo);
		}
		frame_cc.bottom.data_num = 0;
	}

	return 0;
}

int videodisp_vpu_isr(struct gxav_video_module *module, void (*callback)(int id))
{
	int disp_left_num;
	VideoFrameInfo *to_disp;
	unsigned int is_top_field = 0;

	static int last_is_top_field = 0;
	static unsigned int pts = 0;
	static unsigned int interlace = 0;
	static VideoFrameInfo new_get;
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	VDBG_PRINT_ESV();

	gxav_vpu_disp_clr_field_error_int();
	gxav_vpu_disp_clr_field_start_int();

	is_top_field = gxav_vpu_disp_get_view_field();

	VDBG_PRINT_FIELD_INFO(is_top_field, 1);

	if(disp->state == DISP_STOPED) {
		callback(((struct gxav_video*)(module->pre))->id);
		goto EXIT;
	}

	if(disp->hd_init == 0) {
		disp->hd_init = 1;
		last_is_top_field = -1;
		pts = 0;
#ifdef CONFIG_AV_ENABLE_BFRAME_FREEZE
		bbtfz_init(&bbt_freeze);
#endif
		gx_memset(&new_get, 0, sizeof(VideoFrameInfo));
		disp->sync_disp.require_clr.type = VFRAME_NULL;
	}

	disp_left_num = gxav_vpu_disp_get_buff_cnt();
	if(disp_left_num == 8) {
		//gx_printf(" vpu disp fifo is full fatal error \n");
		goto EXIT;
	}
	if(disp->cc_enable)
		videodisp_play_cc(module, is_top_field);

	if(last_is_top_field == is_top_field && last_is_top_field!=-1) {
		//gx_printf("vpu interrupt lost %x %x\n",last_is_top_field ,is_top_field);
	}
	last_is_top_field = is_top_field;
	to_disp = &(disp->sync_disp.to_disp);

	if (disp->freeze_state == FZ_UNFREEZE_REQUIRED) {
		videodec_fb_unmask_require(((struct gxav_video*)module->pre)->dec, FREEZE_FB_ID);
		video_disp->freeze_state = FZ_NO_REQUIRE;
	}

	//GET_FRAME:
	videodisp_clr_frame(module, &disp->sync_disp.require_clr);

	callback(((struct gxav_video*)(module->pre))->id);
	if(disp->pp_enable || is_top_field == 0 || disp->fps >= FRAME_RATE_50) {
		if((disp->state==DISP_READY || disp->state==DISP_RUNNING) && new_get.type==VFRAME_NULL) {
			vpu_get_frame(module, &new_get);
		}
		if(new_get.type != VFRAME_NULL) {
			disp->fps = new_get.fps;
			interlace = new_get.interlace;
			disp->top_first = new_get.top_first;
			pts = new_get.pts;
			disp->pp_enable = (new_get.type==VFRAME_DEINTERLACE_Y || new_get.type==VFRAME_DEINTERLACE_YUV);
		}
	}

	//FIELD_CHECK:
	if(disp->pp_enable == 0) {
		if(disp->fps >= FRAME_RATE_50) {
			goto TO_DISP;
		}
		if(is_top_field) {
			if(interlace==SCAN_INTERLACE && disp->top_first==0) {
				goto TO_DISP;
			}
		} else {
			if((interlace==SCAN_PROGRESSIVE) || (interlace==SCAN_INTERLACE&&disp->top_first==1)) {
				goto TO_DISP;
			}
		}
		goto EXIT;
	} else if(disp->pp_enable == 1) {
		goto TO_DISP;
	}

TO_DISP:
	if (new_get.type == VFRAME_NULL) {
		if (disp->sync_disp.showing.type != VFRAME_NULL && disp->bypass == 0) {
			struct frame_info tmp = disp->sync_disp.showing;
			if (disp->state == DISP_PAUSED)
				frame_simp_deinterlace(&tmp);
			videodisp_display(module, &tmp, DISP_MODE_AUTO, 0);
			gx_video_ppopen_enable(0);
		}
		goto EXIT;
	} else {
		*to_disp = new_get;
		if (disp->bypass) {
			new_get.type = VFRAME_NULL;
			disp->sync_disp.require_clr = disp->sync_disp.showing;
			disp->sync_disp.showing     = *to_disp;
		} else if (videodisp_display(module, to_disp, DISP_MODE_AUTO, 0) == 0) {
			new_get.type = VFRAME_NULL;
			disp->sync_disp.require_clr = disp->sync_disp.showing;
			disp->sync_disp.showing     = *to_disp;
		}
	}
#if 0
	if (disp->play_frame_cnt>1 && disp->freeze_state == FZ_FREEZED) {
		videodec_fb_unmask_require(((struct gxav_video*)module->pre)->dec, FREEZE_FB_ID);
		video_disp->freeze_state = FZ_NO_REQUIRE;
	}
#endif

	gx_video_ppopen_enable(0);
	disp->hw_vpu_show_enable++;
	if (disp->hw_vpu_show_enable) {
		videodisp_set_state(disp, DISP_RUNNING);
	}

EXIT:
	VDBG_PRINT_FIELD_INFO(is_top_field, 0);
	return 0;
}

int videodisp_get_showing_frame(struct gxav_video_module *module, struct cap_frame *frame)
{
	int ret = -1;
	VideoFrameInfo *src_frame = NULL;
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	if (frame != NULL) {
		if (disp->sync_disp.showing.type != VFRAME_NULL)
			src_frame = &(disp->sync_disp.showing);
		else if (disp->freeze_state == FZ_FREEZED)
			src_frame = &(disp->freeze_frame);

		if (src_frame != NULL) {
			struct fb_desc desc = {0};
			struct frame_buffer tmp_fb = {{0}};
			desc.type   = FB_NORMAL;
			desc.num    = 1;
			desc.fbs    = &tmp_fb;
			desc.bpp    = 8;
			desc.width  = src_frame->width;
			desc.height = src_frame->height;
			desc.buf_addr = frame->buf_addr;
			desc.buf_size = frame->buf_size;
			if (fb_alloc(&desc) == 1) {
				VideoFrameInfo tmp_frame = {0};
				tmp_frame.top.virt.addry     = tmp_fb.virt.bufY;
				tmp_frame.top.virt.addrcb    = tmp_fb.virt.bufCb;
				tmp_frame.bottom.virt.addry  = tmp_fb.virt.bufY  + src_frame->width;
				tmp_frame.bottom.virt.addrcb = tmp_fb.virt.bufCb + src_frame->width;
				tmp_frame.stride = src_frame->width;
				frame_copy(&tmp_frame, src_frame);
				frame->width  = src_frame->width;
				frame->height = src_frame->height;
			}
			ret = 0;
		}
	}

	return ret;
}

int videodisp_push_cc(int id, CcPerFrame *frame_cc)
{
	struct gxav_video_disp*	disp = videodisp_creat_instance(id);

	if(frame_cc) {
		gxfifo_put(&disp->cc_fifo, frame_cc, sizeof(CcPerFrame));
		if(!disp->cc_enable)
			disp->cc_enable = 1;
	}
	return 0;
}

int  videodisp_bypass(int id, int bypass)
{
	struct gxav_video_disp*	disp = videodisp_creat_instance(id);

	disp->bypass = bypass;

	return 0;
}

int videodisp_get_frame_cnt(struct gxav_video_module *module, unsigned long long *frame_cnt)
{
	struct gxav_video_disp *disp = (struct gxav_video_disp *)module->priv;

	if(disp && frame_cnt) {
		*frame_cnt = disp->play_frame_cnt;
	}

	return 0;
}

struct disp_ops video_disp_ops = {
	.open           = videodisp_open,
	.close          = videodisp_close,
	.run            = videodisp_start,
	.stop           = videodisp_stop,
	.pause          = videodisp_pause,
	.resume         = videodisp_resume,
	.config         = videodisp_config,
	.get_pp_up      = videodisp_pp_up,
	.get_state      = videodisp_get_state,
};

