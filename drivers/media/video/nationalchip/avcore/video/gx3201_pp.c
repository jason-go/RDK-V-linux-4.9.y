#include "frame_convert.h"
#include "video_sync.h"
#include "mb_filter.h"
#include "video_userdata.h"
#include "video.h"
#include "video_decode.h"
#include "pp_core.h"
#include "gx3201_pp.h"
#include "video_display.h"
#include "clock_hal.h"
#include "vout_hal.h"
#include "gxav_bitops.h"

static struct gx3201_pp_reg *gx3201_pp_reg = NULL;
static struct gxav_video_pp  gx3201_pp[MAX_PP] = {{0}};

#define PP_ABANDON_FRAME(thiz, frame)\
	do {\
		(frame)->type = VFRAME_ABANDON;\
		gxfifo_put((thiz)->back_to, (frame), sizeof(VideoFrameInfo));\
		/*gx_printf("%s:%d, abandon frame.\n", __func__, __LINE__);*/\
	}while(0)

static void gx3201pp_hw_init(struct gxav_video_module *module)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	PP_CAL_END_INT_ENABLE(gx3201_pp_reg->rPP_INT_CTRL);
	PP_UV_INTERLACED(gx3201_pp_reg->rPP_INT_CTRL);
	PP_ENDIAN_MODE_SET(gx3201_pp_reg->rPP_INT_CTRL,0);
	PP_WINDOW_MODE_SET(gx3201_pp_reg->rPP_INT_CTRL,0);
	PP_UV_DOMOTION_DISABLE(gx3201_pp_reg->rPP_INT_CTRL);
	if(pp->deinterlace.denoise_en) {
		PP_DEMOTION_ENABLE(gx3201_pp_reg->rPP_INT_CTRL);
		PP_Y_WOR_MODE_SET(gx3201_pp_reg->rPP_INT_CTRL,0x3);
	}
	else {
		PP_DEMOTION_DISABLE(gx3201_pp_reg->rPP_INT_CTRL);
		PP_Y_WOR_MODE_SET(gx3201_pp_reg->rPP_INT_CTRL,0x2);
	}
}

static void gx3201pp_reset(struct gxav_video_module *module)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	pp->action = FIELD_TYPE_NONE;
	pp->stride = 0;
	gx_memset(&pp->zoom,     0, sizeof(pp->zoom));
	gx_memset(&pp->pre_ref,  0, sizeof(pp->pre_ref));
	gx_memset(&pp->cur_ref,  0, sizeof(pp->cur_ref));
	gx_memset(&pp->next_ref, 0, sizeof(pp->next_ref));
	gx_memset(&pp->pre_cal,  0, sizeof(pp->pre_cal));
	gx_memset(&pp->cur_cal,  0, sizeof(pp->cur_cal));

	pp->deinterlace.last_top_first    = -1;
	pp->deinterlace.last_top_fb_id    = -1;
	pp->deinterlace.last_bottom_fb_id = -1;
	gxav_clock_cold_rst(MODULE_TYPE_PP);
	gx3201pp_hw_init(module);
}

static unsigned int gx3201pp_alloc_fb(struct gxav_video_module *module ,unsigned int width, unsigned int height)
{
	int ret = 0;
	struct fb_desc desc = {0};
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if(pp->fb_num == 0) {
		desc.type   = FB_NORMAL;
		desc.mode   = FB_FRAME;
		desc.num    = MAX_PP_FRAME;
		desc.fbs    = pp->fb;
		desc.bpp    = 8;
		desc.align  = 64;
		desc.width  = width;
		desc.height = height;
		if (fb_alloc(&desc) != MAX_PP_FRAME) {
			//gx_printf("[pp] alloc fb failed!\n");
			ret = -1;
			goto out;
		}
		pp->fb_width  = width;
		pp->fb_height = height;
		while(pp->fb_num < MAX_PP_FRAME) {
			pp->fb_mask |= (1<<pp->fb_num);
			pp->fb_num++;
		};
	} else {
		if (width == pp->fb_width && height == pp->fb_height)
			ret = 0;
		else
			ret = -1;
	}


out:
	return ret;
}

static int gx3201pp_get_framebuf(struct gxav_video_pp *pp)
{
	int index = 0;
	if(pp->fb_flag != pp->fb_mask) {
		while(index < pp->fb_num) {
			if(pp->fb_mask & pp->fb_flag & (1<<index)) {
				index++;
				continue;
			}
			pp->fb_flag |= (1<<index);
			return index;
		}
	}

	return -1;
}

static int gx3201pp_clr_framebuf(struct gxav_video_pp *pp, unsigned int index)
{
	if(pp->fb_mask!=0 && pp->fb_flag!=0) {
		if(pp->fb_mask & pp->fb_flag & (1<<index)) {
			pp->fb_flag &= ~(1<<index);
			return 0;
		}
		else {
			return -1;
		}
	}

	return 0;
}

static void gx3201_videopp_clr_frame(struct gxav_video_module *module, VideoFrameInfo *frame)
{
	int pp_fb_t  = -1, pp_fb_b  = -1;
	int dec_fb_t = -1, dec_fb_b = -1;
	VideoFrameInfo tmp_frame;
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if(frame->top.is_ref) {
		dec_fb_t = frame->top.fb_id;
	} else {
		pp_fb_t = frame->top.fb_id;
	}

	if(frame->bottom.is_ref) {
		dec_fb_b = frame->bottom.fb_id;
	} else {
		pp_fb_b = frame->bottom.fb_id;
	}

	if(pp_fb_t !=-1 && --pp->ppfb_ref_cnt[pp_fb_t] == 0) {
		gx3201pp_clr_framebuf(pp, pp_fb_t);
	}
	if(pp_fb_b !=-1 && --pp->ppfb_ref_cnt[pp_fb_b] == 0) {
		gx3201pp_clr_framebuf(pp, pp_fb_b);
	}

	if(dec_fb_t !=-1 && --pp->decfb_ref_cnt[dec_fb_t] == 0) {
		tmp_frame.top.fb_id = dec_fb_t;
		tmp_frame.bottom.fb_id = -1;
		PP_ABANDON_FRAME(module, &tmp_frame);
	}

	if(dec_fb_b !=-1 && --pp->decfb_ref_cnt[dec_fb_b] == 0) {
		tmp_frame.top.fb_id = -1;
		tmp_frame.bottom.fb_id = dec_fb_b;
		PP_ABANDON_FRAME(module, &tmp_frame);
	}
	VDBG_PRINT_FRAME_FLOW("pp clr", frame);
}

static int gx3201pp_push_frame(struct gxav_video_module *module)
{
#define REF_CNT_INCREASE(cnt_array, index) (index >= 0 ? cnt_array[index]++ : 0 )
#define PUSH_FRAME(frame) \
	do {\
		if(video_frame_convert_run(pp->fr_con, (frame)->disp_num) == FR_SKIP)\
			gx3201_videopp_clr_frame(module, frame);\
		else {\
			gxfifo_put(module->out, frame, sizeof(VideoFrameInfo));\
			VDBG_PRINT_FRAME_FLOW("pp put", frame);\
		}\
	}while(0)
	VideoFrameInfo first;
	VideoFrameInfo second;
	struct gxav_video_pp  *pp = GET_VIDEO_PP(module);

	switch(pp->action) {
		case FIELD_TYPE_ZOOM_B: {
			first = pp->cur_cal;
			first.type = VFRAME_ZOOM;
			first.disp_num = pp->cur_ref.disp_num;
			first.clip = pp->process_frame.clip;
			first.view = pp->process_frame.view;
			REF_CNT_INCREASE(pp->ppfb_ref_cnt, first.top.fb_id);
			REF_CNT_INCREASE(pp->ppfb_ref_cnt, first.bottom.fb_id);
			PUSH_FRAME(&first);
			PP_ABANDON_FRAME(module, &pp->cur_ref);
			break;
		}
		case FIELD_TYPE_Y :
		case FIELD_TYPE_UV: {
			if(pp->cur_ref.top_first) {
				first        = pp->cur_ref;
				first.type   = VFRAME_DEINTERLACE_Y;
				first.top    = pp->cur_ref.top, REF_CNT_INCREASE(pp->decfb_ref_cnt, first.top.fb_id);
				first.bottom = pp->cur_cal.bottom;
				if (pp->cur_ref.store_mode != STORE_FRAME) {
					first.bottom.phys.addrcb = pp->cur_ref.top.phys.addrcb;
				}

				second        = pp->cur_ref;
				second.type   = VFRAME_DEINTERLACE_Y;
				second.top    = pp->cur_cal.top;
				second.bottom = pp->cur_ref.bottom, REF_CNT_INCREASE(pp->decfb_ref_cnt, second.bottom.fb_id);
				if (pp->cur_ref.store_mode != STORE_FRAME) {
					second.top.phys.addrcb =  pp->cur_ref.bottom.phys.addrcb;
				}
			} else {
				first        = pp->cur_ref;
				first.type   = VFRAME_DEINTERLACE_Y;
				first.top    = pp->pre_cal.top;
				first.bottom = pp->cur_ref.bottom, REF_CNT_INCREASE(pp->decfb_ref_cnt, first.bottom.fb_id);
				first.top.phys.addrcb = pp->cur_ref.top.phys.addrcb;
				if (pp->cur_ref.store_mode != STORE_FRAME) {
					first.top.phys.addrcb = pp->cur_ref.bottom.phys.addrcb;
				}

				second        = pp->cur_ref;
				second.type   = VFRAME_DEINTERLACE_Y;
				second.top    = pp->cur_ref.top, REF_CNT_INCREASE(pp->decfb_ref_cnt, second.top.fb_id);
				second.bottom = pp->cur_cal.bottom;
				second.bottom.phys.addrcb = pp->cur_ref.bottom.phys.addrcb;
				if (pp->cur_ref.store_mode != STORE_FRAME) {
					second.bottom.phys.addrcb = pp->cur_ref.top.phys.addrcb;
				}
			}

			first.disp_num  = pp->cur_ref.disp_num * 2;
			second.disp_num = first.disp_num + 1;
			first.clip = second.clip = pp->process_frame.clip;
			first.view = second.view = pp->process_frame.view;
			PUSH_FRAME(&first);
			PUSH_FRAME(&second);

			if(pp->deinterlace.last_top_first != -1 && pp->deinterlace.last_top_first != pp->cur_ref.top_first) {
				if(pp->deinterlace.last_top_first == 1) {
					REF_CNT_INCREASE(pp->ppfb_ref_cnt, first.top.fb_id);
				} else {
					pp->ppfb_ref_cnt[pp->deinterlace.last_top_fb_id]--;
					if (pp->ppfb_ref_cnt[pp->deinterlace.last_top_fb_id] == 0) {
						gx3201pp_clr_framebuf(pp, pp->deinterlace.last_top_fb_id);
					}
				}
			}
			pp->deinterlace.last_top_first    = pp->cur_ref.top_first;
			pp->deinterlace.last_top_fb_id    = pp->cur_cal.top.fb_id;
			pp->deinterlace.last_bottom_fb_id = pp->cur_cal.bottom.fb_id;
			break;
		}
		case FIELD_TYPE_NONE: {
			first = pp->process_frame;
			REF_CNT_INCREASE(pp->decfb_ref_cnt, first.top.fb_id);
			REF_CNT_INCREASE(pp->decfb_ref_cnt, first.bottom.fb_id);
			PUSH_FRAME(&first);
			break;
		}
		default:
			return -1;
	}

	pp->cal_fb_id = -1;
	pp->process_frame.type = VFRAME_NULL;
	return 0;
}

static int gx3201pp_bypass_require(struct gxav_video_module *module, unsigned int fps)
{
	int dec_mode = 0;

	videodec_get_decmode(((struct gxav_video*)module->pre)->dec, &dec_mode);
	if(dec_mode != DECODE_NORMAL) {
		return 1;
	}
	if(fps >= FRAME_RATE_50) {
		return 1;
	}
	return 0;
}

static int gx3201pp_bypass_config(struct gxav_video_module *module)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if(pp->next_ref.width!=0 || pp->next_ref.height!=0) {
		PP_ABANDON_FRAME(module, &pp->next_ref);
		gx3201pp_reset(module);
	}
	return 0;
}

static void gx3201pp_bypass(struct gxav_video_module *module)
{
	gx3201pp_push_frame(module);
}

#define GET_VPU_ZOOM_CAPALITY(frame_type, cap) \
	do {\
		struct vout_dvemode dvemode;\
		dvemode.id = ID_VPU;\
		gxav_videoout_get_dvemode(0, &dvemode);\
		if ((frame_type==SCAN_PROGRESSIVE) && IS_INTERLACE_MODE(dvemode.mode)) {\
			if (CHIP_IS_GX6605S == 1)\
				cap = 16;\
			else\
				cap = 4;\
		} else {\
			if (CHIP_IS_GX6605S == 1)\
				cap = 32;\
			else\
				cap = 8;\
		}\
	} while(0)

#define GET_ZOOM_RATIO(ratio, clip, view, vpu_capability) \
do {\
	if(view*vpu_capability*1 > clip)\
		ratio = 0; /* pp need not zoom */\
	else if(view*vpu_capability*2 > clip)\
		ratio = 2; /* pp zoom 1/2 */\
	else if(view*vpu_capability*4 > clip)\
		ratio = 4; /* pp zoom 1/4 */\
	else {\
		gx_printf("The view rect is too small, Hardware can not support!\n");\
		return 0;\
	}\
}while(0)
static int gx3201pp_zoom_require(GxAvRect *clip, GxAvRect *view, unsigned int interlace)
{
	unsigned int vpu_capability;
	unsigned int v_ratio, h_ratio;

	return 0;
	if (CHIP_IS_GX3113C == 1)
		return 0;

	GET_VPU_ZOOM_CAPALITY(interlace, vpu_capability);
	GET_ZOOM_RATIO(h_ratio, clip->width,  view->width,  vpu_capability);
	GET_ZOOM_RATIO(v_ratio, clip->height, view->height, vpu_capability);

	if(h_ratio==0 && v_ratio==0)
		return 0;
	else
		return 1;
}

static int gx3201pp_zoom_config(struct gxav_video_module *module, struct frame_info *frame)
{
	unsigned int h_ratio, v_ratio;
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	//由deinterlace模式切换到zoom模式
	if(pp->next_ref.width!=0 || pp->next_ref.height!=0) {
		PP_ABANDON_FRAME(module, &pp->next_ref);
		gx3201pp_reset(module);
	}
	if(pp->zoom.vpu_capability == 0) {
		GET_VPU_ZOOM_CAPALITY(frame->interlace, pp->zoom.vpu_capability);
		GET_ZOOM_RATIO(h_ratio, frame->width,  pp->view_rect.width,  pp->zoom.vpu_capability);
		GET_ZOOM_RATIO(v_ratio, frame->height, pp->view_rect.height, pp->zoom.vpu_capability);
		/*patch for pp module, width must zoom*/
		if(h_ratio==0)
			h_ratio = 2;
		pp->zoom.v_ratio = v_ratio;
		pp->zoom.h_ratio = h_ratio;
	}

	return 0;
}

static void gx3201pp_zoom(struct gxav_video_pp *pp, struct frame_info *in, struct frame_info *out, unsigned int top)
{
	unsigned int width = in->width;
	unsigned int height= in->height;
	unsigned int zoomout_stride = 0;

	REG_SET_FIELD(&(gx3201_pp_reg->rFRAME_BUF_STRIDE),0xFFFFFFFF,pp->stride<<16 | (pp->stride),0);
	PP_SET_TOP_FIELD_FIRST(gx3201_pp_reg->rPP_ZOOM_PIC_INFO,top);
	PP_SET_FRAME_PIC(gx3201_pp_reg->rPP_ZOOM_PIC_INFO,!(in->interlace));

	height /= (1+(in->interlace));
	PP_SET_ZOOM_FRAME_IN_INFO(gx3201_pp_reg->rPP_ZOOM_PIC_SIZE, height|(width<<16));

	if(pp->zoom.h_ratio)
		zoomout_stride = (pp->stride/pp->zoom.h_ratio)>>3<<3;
	else
		zoomout_stride = pp->stride>>3<<3;
	PP_SET_ZOOM_FRAME_IN_STRIDE(gx3201_pp_reg->rPP_ZOOM_PIC_STRIDE, ((1<<31)|zoomout_stride|(zoomout_stride<<16)));

	if(top) {
		PP_SET_Y_CUR_ADDR(gx3201_pp_reg->rPP_ZOOM_YCUR_ADDR, in->top.phys.addry);
		PP_SET_UV_CUR_ADDR(gx3201_pp_reg->rPP_ZOOM_UVCUR_ADDR, in->top.phys.addrcb);
		PP_SET_Y_CUR_STORE_ADDR(gx3201_pp_reg->rPP_ZOOM_YCUR_STORE_ADDR, out->top.phys.addry);
		PP_SET_UV_CUR_STORE_ADDR(gx3201_pp_reg->rPP_ZOOM_UVCUR_STORE_ADDR, out->top.phys.addrcb);
	}
	else {
		unsigned int uv_offset = 0;
		uv_offset = pp->stride + pp->stride;
		PP_SET_Y_CUR_ADDR(gx3201_pp_reg->rPP_ZOOM_YCUR_ADDR, in->bottom.phys.addry);
		PP_SET_UV_CUR_ADDR(gx3201_pp_reg->rPP_ZOOM_UVCUR_ADDR, in->bottom.phys.addrcb);

		uv_offset = pp->stride + pp->stride;
		PP_SET_Y_CUR_STORE_ADDR(gx3201_pp_reg->rPP_ZOOM_YCUR_STORE_ADDR, out->bottom.phys.addry);
		PP_SET_UV_CUR_STORE_ADDR(gx3201_pp_reg->rPP_ZOOM_UVCUR_STORE_ADDR, out->bottom.phys.addrcb);
	}

	PP_SET_H_MINIFICATION(gx3201_pp_reg->rPP_ZOOM_CFG, pp->zoom.h_ratio);
	PP_SET_V_MINIFICATION(gx3201_pp_reg->rPP_ZOOM_CFG, pp->zoom.v_ratio);
	PP_ENDIAN_MODE_SET(gx3201_pp_reg->rPP_INT_CTRL,0);
	PP_SET_ZOOM_OUT_DISABLE(gx3201_pp_reg->rPP_ZOOM_CFG);
	PP_SET_ZOOM_OUT_ENABLE(gx3201_pp_reg->rPP_ZOOM_CFG);
}

static int gx3201pp_deinterlace_require(struct gxav_video_module *module ,
	unsigned int fps, unsigned int scan_type, GxAvRect *clip, GxAvRect *view)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	VDBG_FORCE_PP();
#ifdef CONFIG_AV_ENABLE_BFRAME_FREEZE
	return 0;
#endif
	if (!pp->deinterlace_en)
		return 0;

	if (CHIP_IS_GX3113C == 1)
	     return 0;
	if (gx3201pp_zoom_require(clip, view, SCAN_PROGRESSIVE))
	     return 0;
	if (fps > FRAME_RATE_30 || scan_type==SCAN_PROGRESSIVE || (clip->width > pp->deinterlace.max_width || clip->height > pp->deinterlace.max_height))
		return 0;

	return 1;
}

static int gx3201pp_deinterlace_config(struct gxav_video_module *module)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if( pp->action==FIELD_TYPE_ZOOM_T ||
		pp->action==FIELD_TYPE_ZOOM_B ||
		pp->action==FIELD_TYPE_NONE) {
		gx3201pp_reset(module);
	}
	return 0;
}

static void gx3201pp_deinterlace(
		unsigned     denoise_en,
		int          stride,
		unsigned int width,
		unsigned int height,
		unsigned int PRE_addr,
		unsigned int CUR_addr,
		unsigned int NEXT_addr,
		unsigned int NEXT2_addr,
		unsigned int PREC_addr,
		unsigned int curc_t_addr,
		unsigned int curc_b_addr)
{
	REG_SET_FIELD(&(gx3201_pp_reg->rPIC_REC),0xFFFFFFFF,width<<16 | height,0);
	REG_SET_FIELD(&(gx3201_pp_reg->rFRAME_BUF_STRIDE),0xFFFFFFFF,stride<<16 | (stride),0);

	PP_ADDR_SET(gx3201_pp_reg->rPP_Y_PRE_ADDR,    PRE_addr);
	PP_ADDR_SET(gx3201_pp_reg->rPP_Y_CUR_ADDR,    CUR_addr);
	PP_ADDR_SET(gx3201_pp_reg->rPP_Y_NEXT_ADDR,   NEXT_addr);
	PP_ADDR_SET(gx3201_pp_reg->rPP_Y_NEXT2_ADDR,  NEXT2_addr);
	PP_ADDR_SET(gx3201_pp_reg->rPP_Y_PRC_ADDR,    PREC_addr);
	PP_ADDR_SET(gx3201_pp_reg->rPP_Y_DNINT_ADDR1, curc_t_addr);
	PP_ADDR_SET(gx3201_pp_reg->rPP_Y_DNINT_ADDR0, curc_b_addr);
	if(denoise_en) {
		PP_ADDR_SET(gx3201_pp_reg->rPP_Y_DN_ADDR0, NEXT_addr);
		PP_ADDR_SET(gx3201_pp_reg->rPP_Y_DN_ADDR1, NEXT2_addr);
	}

	PP_START_NEW_DISABLE(gx3201_pp_reg->rPP_INT_CTRL);
	PP_START_NEW_ENABLE(gx3201_pp_reg->rPP_INT_CTRL);
}

static unsigned int gx3201pp_pull_frame(struct gxav_video_module *module, VideoFrameInfo *frame)
{
	GxAvRect clip;
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if(module->in == NULL)
		return PP_ERROR;
	gx_memset(frame, 0 , sizeof(VideoFrameInfo));

	if(gxfifo_len(module->in) == 0) {
		if(pp->update) {
			if(++pp->update_timeout >= 30) {
				DecState state = VIDEODEC_OVER;
				struct gxav_video_module *dec = ((struct gxav_video *)module->pre)->dec;
				pp->update = 0;
				videodec_set_state(dec, &state);
				pp->state        = PP_STATE_RUNNING;
				pp->state_shadow = PP_STATE_RUNNING;
			}
		}
		return PP_ERROR;
	}
	else {
		pp->update_timeout = 0;
		pp->no_frame_timeout = 0;
		gxfifo_get(module->in, frame, sizeof(VideoFrameInfo));
		VDBG_PRINT_FRAME_FLOW("pp get", frame);

		if (pp->last_stream_id != frame->stream_id) {
			gx3201pp_reset(module);
			gx_memset(pp->decfb_ref_cnt, 0, sizeof(pp->decfb_ref_cnt));
			gx_memset(pp->ppfb_ref_cnt,  0, sizeof(pp->ppfb_ref_cnt));
			pp->last_stream_id = frame->stream_id;
		}

		//push_cc:
		if(frame->userdata.enable && frame->userdata.display) {
			CcPerFrame frame_cc;
			if(cc_parse_per_frame((void*)frame->userdata.data, &frame_cc) == 0)
				videodisp_push_cc(0, &frame_cc);
		}

		clip.x = 0;
		clip.y = 0;
		clip.width  = frame->stride;
		clip.height = frame->height;

		if(pp->zoom_param_update_require > 0) {
			gx_video_get_viewrect(&pp->view_rect);
			gx_video_get_cliprect(&pp->clip_rect);
			pp->zoom.vpu_capability = 0;
			pp->zoom_param_update_require--;
		}
		frame->clip.x     += pp->clip_rect.x;
		frame->clip.y     += pp->clip_rect.y;
		frame->clip.width  = pp->clip_rect.width;
		frame->clip.height = pp->clip_rect.height;
		frame->view = pp->view_rect;

		pp->stride = frame->stride;
		if(gx3201pp_bypass_require(module, frame->fps))
			return PP_BYPASS;
		else if(gx3201pp_zoom_require(&clip, &pp->view_rect, frame->interlace)) {
			if(0 == gx3201pp_alloc_fb(module, frame->stride/2, clip.height))
				return PP_ZOOM;
			else
				return PP_BYPASS;
		}
		else if(gx3201pp_deinterlace_require(
				module,
				frame->fps,
				frame->interlace,
				&clip, &pp->view_rect)) {
			if(0 == gx3201pp_alloc_fb(module, frame->stride, clip.height))
				return PP_DEINTERLACE;
			else
				return PP_BYPASS;
		}
		return PP_BYPASS;
	}
}

static unsigned int gx3201pp_get_process_frame(struct gxav_video_module *module, VideoFrameInfo *frame)
{
	static int action = 0;
	VideoFrameInfo *input_frame = NULL;
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	input_frame = &pp->input_frame;
	while(1) {
		if(input_frame->type == VFRAME_DECODE) {
			switch(video_pts_sync(pp->sync, input_frame->pts, input_frame->fps)) {
				case SYNC_COMMON:
					if(pp->state == PP_STATE_READY) {
						pp->state        = PP_STATE_RUNNING;
						pp->state_shadow = PP_STATE_RUNNING;
					}
					if(action!=PP_DEINTERLACE && video_frame_convert_run(pp->fr_con, input_frame->disp_num)==FR_SKIP) {
						PP_ABANDON_FRAME(module, input_frame);
						input_frame->type = VFRAME_NULL;
						continue;
					}
					else {
						gx_memcpy(frame, input_frame, sizeof(VideoFrameInfo));
						input_frame->type = VFRAME_NULL;
						return action;
					}
				case SYNC_REPEAT:
					//gx_printf("sync repeat\n");
					return PP_ERROR;
				case SYNC_SKIP:
					//gx_printf("sync skip\n");
					PP_ABANDON_FRAME(module, input_frame);
					input_frame->type = VFRAME_NULL;
					continue;
				default:
					return action;
			}
		}
		else {
			action = gx3201pp_pull_frame(module, input_frame);
			if(action != PP_ERROR) {
				//force drop
				if(pp->drop_filter.gate > 0) {
					pp->drop_filter.cnt = (pp->drop_filter.cnt+1) % pp->drop_filter.gate;
					if(pp->drop_filter.cnt == 0) {
						PP_ABANDON_FRAME(module, input_frame);
						input_frame->type = VFRAME_NULL;
					}
				}
				if(input_frame->fps != pp->last_fps) {
					video_frame_convert_config(pp->fr_con, action==PP_DEINTERLACE, input_frame->fps);
					pp->last_fps = input_frame->fps;
				}
			}
			else
				return PP_ERROR;
		}
	}
}

static int gx3201pp_show_logo(struct gxav_video_module *module )
{
	struct gxav_video_pp   *pp = GET_VIDEO_PP(module);

	if(pp->cur_ref.width != 0) {
		pp->cur_ref = pp->cur_ref;
		pp->cur_cal = pp->cur_ref;
		pp->cur_cal.top.fb_id = pp->cur_cal.bottom.fb_id = -1;
		gx3201pp_push_frame(module);
		return 0;
	}
	return -1;
}

static int gx3201pp_main(struct gxav_video_module *module)
{
#define FRAME_SHIFT(frame)\
	do{ \
		pp->pre_ref  = pp->cur_ref; pp->cur_ref = pp->next_ref;\
		pp->next_ref = frame;       pp->pre_cal = pp->cur_cal;\
	}while(0)
	int pp_enable= 0 ;
	unsigned int h_ratio=0, v_ratio=0;
	unsigned int curc_t_addr=0, curc_b_addr=0;
	unsigned int PRE_addr=0, CUR_addr=0, NEXT_addr=0, NEXT2_addr=0, PREC_addr=0;
	struct gxav_video_pp   *pp = GET_VIDEO_PP(module);

	if((pp->state!=PP_STATE_RUNNING && pp->state!=PP_STATE_READY) || pp->status==PP_STATUS_BUSY) {
		goto START_ERROR;
	}

GET_ONE_FRAME:
	if(pp->process_frame.type == VFRAME_NULL) {
		pp_enable = 0;
		switch(gx3201pp_get_process_frame(module, &pp->process_frame)) {
			case PP_DEINTERLACE:
				gx3201pp_deinterlace_config(module);
				pp->action = FIELD_TYPE_Y;
				pp_enable  = 1;
				gx_video_frame_rate_transform_enable(0, pp_enable);
				break;
			case PP_ZOOM:
				gx3201pp_zoom_config(module, &pp->process_frame);
				pp->action = FIELD_TYPE_ZOOM_T;
				gx_video_frame_rate_transform_enable(0, pp_enable);
				break;
			case PP_BYPASS:
				gx3201pp_bypass_config(module);
				pp->action = FIELD_TYPE_NONE;
				gx_video_frame_rate_transform_enable(0, pp_enable);
				break;
			case PP_ERROR:
				if(pp->process_frame.type != VFRAME_NULL)
					gx_video_frame_rate_transform_enable(0, pp_enable);
				goto START_ERROR;
		}
	}

	switch(pp->action)
	{
	case FIELD_TYPE_Y:
		{
			if(pp->cur_ref.width==0 || pp->cur_ref.height==0) {
				pp->pre_ref  = pp->process_frame;
				pp->cur_ref  = pp->process_frame;
				pp->next_ref = pp->process_frame;
				pp->pre_cal  = pp->process_frame;
				pp->pre_cal.top.fb_id = pp->pre_cal.bottom.fb_id = -1;
				pp->cur_cal = pp->process_frame;
				pp->cur_cal.top.fb_id = pp->cur_cal.bottom.fb_id = -1;
				pp->cur_cal.top.is_ref    = 0;
				pp->cur_cal.bottom.is_ref = 0;

				///< put logo
				if(pp->show_logo) {
					if(0 == gx3201pp_show_logo(module)) {
						pp->show_logo = 0;
						pp->process_frame.type = VFRAME_NULL;
						return 0;
					}
				}
				pp->process_frame.type = VFRAME_NULL;
				goto GET_ONE_FRAME;
			}

			///<  get frame buffer to store new cal result
			if(pp->cal_fb_id == -1) {
				pp->cal_fb_id = gx3201pp_get_framebuf(pp);
				if(pp->cal_fb_id == -1) {
					goto START_ERROR;
				} else {
					pp->ppfb_ref_cnt[pp->cal_fb_id] = 2;
				}
			}
			FRAME_SHIFT(pp->process_frame);

			pp->cur_cal                = pp->cur_ref;
			pp->cur_cal.top.fb_id      = pp->cal_fb_id;
			pp->cur_cal.top.phys.addry = pp->fb[pp->cal_fb_id].phys.bufY;
			pp->cur_cal.top.virt.addry = pp->fb[pp->cal_fb_id].virt.bufY;

			pp->cur_cal.bottom.phys.addry = pp->fb[pp->cal_fb_id].phys.bufY+pp->cur_cal.stride;
			pp->cur_cal.bottom.virt.addry = pp->fb[pp->cal_fb_id].virt.bufY+pp->cur_cal.stride;
			pp->cur_cal.bottom.fb_id  = pp->cur_cal.top.fb_id;
			pp->cur_cal.top.is_ref    = 0;
			pp->cur_cal.bottom.is_ref = 0;

			curc_t_addr = pp->cur_cal.top.phys.addry;
			curc_b_addr = pp->cur_cal.bottom.phys.addry;
			if(pp->cur_ref.top_first) {
				CUR_addr   = pp->cur_ref.top.phys.addry;
				PRE_addr   = pp->pre_ref.bottom.phys.addry;
				NEXT_addr  = pp->cur_ref.bottom.phys.addry;
				NEXT2_addr = pp->next_ref.top.phys.addry;
				PREC_addr  = pp->pre_cal.top.phys.addry;
			}
			else {
				CUR_addr   = pp->cur_ref.top.phys.addry;
				PRE_addr   = pp->cur_ref.bottom.phys.addry;
				NEXT_addr  = pp->next_ref.bottom.phys.addry;
				NEXT2_addr = pp->next_ref.top.phys.addry;
				PREC_addr  = pp->pre_cal.top.phys.addry;
			}

			pp->status = PP_STATUS_BUSY;
			gx3201pp_deinterlace(
					pp->deinterlace.denoise_en,
					pp->stride,
					pp->cur_cal.width,
					pp->cur_cal.height,
					PRE_addr, CUR_addr, NEXT_addr, NEXT2_addr, PREC_addr, curc_t_addr, curc_b_addr);
			return 0;
		}
	case FIELD_TYPE_ZOOM_T:
		{
			if(pp->cal_fb_id == -1) {
				pp->cal_fb_id = gx3201pp_get_framebuf(pp);
				if(pp->cal_fb_id == -1) {
					goto START_ERROR;
				}
			}
			pp->cur_ref = pp->process_frame;

			//todo to caculate the output rect parameter
			h_ratio = (pp->zoom.h_ratio==0)?1:pp->zoom.h_ratio;
			v_ratio = (pp->zoom.v_ratio==0)?1:pp->zoom.v_ratio;

			pp->cur_cal.width         = (pp->process_frame.width/h_ratio)>>3<<3;
			pp->cur_cal.height        =  pp->process_frame.height/v_ratio;
			pp->cur_cal.clip_width    = (pp->process_frame.clip_width/h_ratio)>>3<<3;
			pp->cur_cal.clip_height   =  pp->process_frame.clip_height/v_ratio;
			pp->cur_cal.interlace     =  pp->process_frame.interlace;
			pp->cur_cal.top_first     =  pp->process_frame.top_first;
			pp->cur_cal.num_of_err_mb =  pp->process_frame.num_of_err_mb;
			pp->cur_cal.pic_type      =  pp->process_frame.pic_type;
			pp->cur_cal.store_mode    =  pp->process_frame.store_mode;

			pp->cur_cal.top.fb_id          = pp->cal_fb_id;
			pp->cur_cal.top.phys.addry     = pp->fb[pp->cal_fb_id].phys.bufY;
			pp->cur_cal.top.virt.addry     = pp->fb[pp->cal_fb_id].virt.bufY;
			pp->cur_cal.top.phys.addrcb    = pp->fb[pp->cal_fb_id].phys.bufCb;
			pp->cur_cal.top.virt.addrcb    = pp->fb[pp->cal_fb_id].virt.bufCb;
			pp->cur_cal.bottom.phys.addry  = pp->cur_cal.top.phys.addry  + pp->cur_cal.width;
			pp->cur_cal.bottom.virt.addry  = pp->cur_cal.top.virt.addry  + pp->cur_cal.width;
			pp->cur_cal.bottom.phys.addrcb = pp->cur_cal.top.phys.addrcb + pp->cur_cal.width;
			pp->cur_cal.bottom.virt.addrcb = pp->cur_cal.top.virt.addrcb + pp->cur_cal.width;
			pp->cur_cal.bottom.fb_id = pp->cur_cal.top.fb_id;
			pp->cur_cal.top.is_ref    = 0;
			pp->cur_cal.bottom.is_ref = 0;
			pp->status = PP_STATUS_BUSY;
			gx3201pp_zoom(pp, &pp->cur_ref, &pp->cur_cal, 1);
			return 0;
		}
	case FIELD_TYPE_ZOOM_B:
		{
			pp->status = PP_STATUS_BUSY;
			gx3201pp_zoom(pp, &pp->cur_ref, &pp->cur_cal, 0);
			return 0;
		}
	default:
		{
			gx3201pp_bypass(module);
			return 0;
		}
	}

START_ERROR:
	return -1;
}

static struct gxav_video_pp* gx3201pp_creat_instance(int sub)
{
	int i = 0;
	static  unsigned int instance_init = 0;
	struct gxav_video_pp  *pp = gx3201_pp;

	if(pp == NULL) {
		return NULL;
	}
	if(!instance_init) {
		instance_init = ~instance_init;
		for(i = 0, pp = gx3201_pp; i < MAX_PP; i++) {
			pp->id   = -1;
			pp->used = 0;
			pp++;
		}
	}

	for(i = 0, pp = gx3201_pp; i < MAX_PP; i++) {
		if(pp->used && pp->id==sub){
			pp->used++;
			return pp;
		}
		pp++;
	}

	for(i = 0, pp = gx3201_pp; i < MAX_PP; i++) {
		if(0 == pp->used){
			gx_memset(pp, 0, sizeof(struct gxav_video_pp));
			pp->id   = sub;
			pp->used = 1;
			break;
		}
		pp++;
	}

	return (MAX_PP==i) ? NULL : pp;
}

static int gx3201pp_delete_instance(struct gxav_video_pp *pp)
{
	if(pp->used && --pp->used){
		return 0;
	}
	gx_memset(pp, 0, sizeof(struct gxav_video_pp) );
	pp->id   = -1;
	pp->used = 0;
	return 0;
}

#define GX3201_VIDEO_PP_BASE_ADDR	0x04500000
static int gx3201_videopp_iounmap(void)
{
	if(gx3201_pp_reg != NULL) {
		gx_iounmap(gx3201_pp_reg );
		gx_release_mem_region(GX3201_VIDEO_PP_BASE_ADDR, 0x610) ;
		gx3201_pp_reg = NULL;
	}
	return 0;
}

static int gx3201_videopp_ioremap(void)
{
	if(gx3201_pp_reg == NULL) {
		if (!gx_request_mem_region(GX3201_VIDEO_PP_BASE_ADDR, 0x610)) {
			VIDEODEC_PRINTF("%s request_mem_region failed",__func__);
			return -1;
		}
		gx3201_pp_reg = gx_ioremap(GX3201_VIDEO_PP_BASE_ADDR,0x610);
		if (gx3201_pp_reg == NULL) {
			VIDEODEC_PRINTF("%p ioremap failed.\n",gx3201_pp_reg);
			goto PP_IO_MAP_ERR;
		}
		else {
			VIDEODEC_DBG("BODA_BIT_BASE base addr = %p\n",gx3201_pp_reg);
		}
	}
	return 0;

PP_IO_MAP_ERR:
	gx3201_videopp_iounmap( );
	return -1;
}

static void* gx3201_videopp_open(int sub, struct video_sync *sync, struct frame_rate_convert *fr_con)
{
	struct gxav_video_pp *pp = NULL;

	pp = gx3201pp_creat_instance(sub);
	if(NULL == pp) {
		return NULL;
	}
	pp->sync     = sync;
	pp->fr_con   = fr_con;
	pp->state    = PP_STATE_STOP;
	pp->zoom_param_update_require = 0;
	pp->last_stream_id = 0;

	video_sync_init(pp->sync);
	return pp;
}

static int gx3201_videopp_close(struct gxav_video_module *module)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if(pp != NULL) {
		gx3201pp_delete_instance(pp);
	}
	return 0;
}

static int gx3201_videopp_start(struct gxav_video_module *module)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	pp->fb_num   = 0;
	pp->fb_mask  = 0;
	pp->fb_flag  = 0;
	pp->last_fps = 0;
	pp->last_frame_width  = 0;
	pp->last_frame_height = 0;
	gx_memset(pp->fb, 0, sizeof(struct frame_buffer)*MAX_PP_FRAME);
	gx_memset(pp->decfb_ref_cnt, 0, sizeof(pp->decfb_ref_cnt));
	gx_memset(pp->ppfb_ref_cnt,  0, sizeof(pp->ppfb_ref_cnt));

	pp->show_logo      = 0;
	pp->no_frame_timeout = 0;
	pp->status         = PP_STATUS_IDLE;

	gx3201pp_reset(module);
	pp->state              = PP_STATE_READY;
	pp->state_shadow       = PP_STATE_READY;
	pp->frame_cnt          =  0;
	pp->cal_fb_id          = -1;
	pp->input_frame.type   = VFRAME_NULL;
	pp->process_frame.type = VFRAME_NULL;
	pp->output_frame.type  = VFRAME_NULL;

	video_tolerance_fresh(pp->sync);
	video_frame_convert_init(pp->fr_con);
	PP_INT_STATUS_CLR(gx3201_pp_reg->rPP_INT_CTRL);
	return 0;
}

static int gx3201_videopp_stop(struct gxav_video_module *module )
{
	struct gxav_video *video = (struct gxav_video*)module->pre;
	unsigned long   tick = 10;
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if(video && (video->vstate   == VSTATE_STOPPED))
		return 0;

	pp->update = 0;
	pp->state  = PP_STATE_STOP;
	pp->update_timeout   = 0;
	pp->drop_filter.gate = 0;
	do{
		if(pp->status != PP_STATUS_BUSY) {
			goto PP_STOP_END;
		}
		gx_mdelay(10);
	}while(tick--);

PP_STOP_END:
	video_sync_init(pp->sync);
	PP_CAL_END_INT_DISABLE(gx3201_pp_reg->rPP_INT_CTRL);
	pp->cal_fb_id = -1;
	pp->deinterlace.last_top_first    = -1;
	pp->deinterlace.last_top_fb_id    = -1;
	pp->deinterlace.last_bottom_fb_id = -1;
	return 0;
}

static int gx3201_videopp_config(struct gxav_video_module *module,unsigned int cmd,void *config, unsigned int size)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	switch(cmd) {
		case PP_CONFIG_SHOW_LOGO:
			{
				unsigned int show_logo = *(unsigned int*)config;
				if(show_logo == 1) {
					if(0 == gx3201pp_show_logo(module))
						pp->show_logo = 0;
					else
						pp->show_logo = 1;
				}
				break;
			}
		case PP_CONFIG_UPDATE:
				pp->update = 1;
				break;
		case PP_CONFIG_DEINTERLACE:
				memcpy(&pp->deinterlace, config, size);
				break;
		case PP_CONFIG_DROP_GATE:
				pp->drop_filter.gate = *(unsigned int*)config;
				break;
		case PP_CONFIG_DEINTERLACE_EN:
				pp->deinterlace_en  = *(unsigned int*)config;
				break;
		default:
				return -1;
	}

	return 0;
}

static int gx3201_videopp_get_state(struct gxav_video_module *module, int *state )
{
	struct gxav_video_pp  *pp = GET_VIDEO_PP(module);
	*state = pp->state;
	return 0;
}

static int gx3201_videopp_isr(struct gxav_video_module *module)
{
	struct gxav_video_pp  *pp = GET_VIDEO_PP(module);

	if(pp->action==FIELD_TYPE_Y || pp->action==FIELD_TYPE_UV) {
		PP_START_NEW_DISABLE(gx3201_pp_reg->rPP_INT_CTRL);
	}
	if(pp->action==FIELD_TYPE_ZOOM_T || pp->action==FIELD_TYPE_ZOOM_B) {
		PP_SET_ZOOM_OUT_DISABLE(gx3201_pp_reg->rPP_INT_CTRL);
	}
	PP_INT_STATUS_CLR(gx3201_pp_reg->rPP_INT_CTRL);

	if(pp->status == PP_STATUS_BUSY) {
		if(pp->action == FIELD_TYPE_ZOOM_T) {
			pp->action = FIELD_TYPE_ZOOM_B;
			goto START_NEXT;
		}
		else if(pp->action == FIELD_TYPE_ZOOM_B) {
			pp->process_frame.clip.width  = pp->cur_cal.width;
			pp->process_frame.clip.height = pp->cur_cal.height;
			gx3201pp_push_frame(module);
		}
		else if(pp->action == FIELD_TYPE_Y) {
#if (MODULE_DE_INTERLACE==DE_INTERLACE_Y)
			gx3201pp_push_frame(module);
			goto ISR_END;
#endif
			pp->action = FIELD_TYPE_UV;
			goto START_NEXT;
		}
	}

#if (MODULE_DE_INTERLACE==DE_INTERLACE_Y)
ISR_END:
#endif
	pp->status = PP_STATUS_IDLE;
	return 0;

START_NEXT:
	pp->status    = PP_STATUS_IDLE;
	gx3201pp_main(module);

	return 0;
}

static void gx3201_videopp_zoom_param_update_require(struct gxav_video_module *module)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);
	pp->zoom_param_update_require++;
}

static int gx3201_videopp_get_sync_stat(struct gxav_video_module *module, unsigned long long *lose_sync_cnt, unsigned int *synced_flag)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if (lose_sync_cnt)
		*lose_sync_cnt = pp->sync->lose_sync.cnt;
	if (synced_flag)
		*synced_flag = pp->sync->synced.flag;

	return 0;
}

static void gx3201_videopp_get_frame(struct gxav_video_module *module, VideoFrameInfo *frame)
{
	struct gxav_video_pp *pp = GET_VIDEO_PP(module);

	if(!frame)
		return;

	frame->type = VFRAME_NULL;
	while(1) {
		if(pp->output_frame.type != VFRAME_NULL) {
			if(video_frame_convert_run(pp->fr_con, frame->disp_num) == FR_REPEAT)
				return;
			else {
				gx_memcpy(frame, &pp->output_frame, sizeof(VideoFrameInfo));
				pp->output_frame.type = VFRAME_NULL;
				return;
			}
		}
		else {
			if(gxfifo_len(module->out))
				gxfifo_get(module->out, &pp->output_frame, sizeof(VideoFrameInfo));
			else
				return;
		}
	}

#if 0
	pp->output_frame.first_disp_times  = video_frame_convert_run(pp->fr_con, ++pp->frame_cnt);
	pp->output_frame.second_disp_times = video_frame_convert_run(pp->fr_con, ++pp->frame_cnt);
	if(pp->output_frame.repeat_first_field) {
		pp->frame_cnt++;
		pp->output_frame.second_disp_times++;
	}
#endif
}

static void gx3201_videopp_callback(struct gxav_video_module *module)
{
	gx3201pp_main(module);
}

struct pp_ops gx3201pp_ops = {
	.ioremap       = gx3201_videopp_ioremap,
	.iounmap       = gx3201_videopp_iounmap,
	.open          = gx3201_videopp_open,
	.close         = gx3201_videopp_close,
	.run           = gx3201_videopp_start,
	.stop          = gx3201_videopp_stop,
	.config        = gx3201_videopp_config,
	.get_state     = gx3201_videopp_get_state,
    .isr           = gx3201_videopp_isr,
    .callback      = gx3201_videopp_callback,
    .get_frame     = gx3201_videopp_get_frame,
    .clr_frame     = gx3201_videopp_clr_frame,
    .get_sync_stat = gx3201_videopp_get_sync_stat,
    .zoom_param_update_require = gx3201_videopp_zoom_param_update_require,
};

