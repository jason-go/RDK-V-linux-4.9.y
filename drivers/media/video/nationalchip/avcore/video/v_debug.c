#include "v_debug.h"
#include "kernelcalls.h"

#ifdef V_DEBUG

static void print_frame_info(VPrintNode *thiz, void *arg)
{
	char *pic_type = "IPB";
	static VideoFrameInfo last_pts_frame, *last;
	VideoFrameInfo *cur = ((FrameInfoArg*)arg)->frame;

	last = &last_pts_frame;
	if(thiz && thiz->enable) {
		int step = last->pts && cur->pts && (cur->disp_num-last->disp_num) ? (cur->pts-last->pts)/(cur->disp_num-last->disp_num) : 0;
		if(step) {
			gx_printf("\n[%d] %c, tf=%d, re=%d, err=%d, pts=%u, step=%d\n", cur->disp_num, pic_type[cur->pic_type], cur->top_first, cur->repeat_first_field, cur->num_of_err_mb, cur->pts, step);
		} else {
			gx_printf("\n[%d] %c, tf=%d, re=%d, err=%d, pts=%u\n", cur->disp_num, pic_type[cur->pic_type], cur->top_first, cur->repeat_first_field, cur->num_of_err_mb, cur->pts);
		}
		if(cur->pts) {
			last_pts_frame = *cur;
		}
	}
}

static void print_frame_flow(VPrintNode *thiz, void *arg)
{
	char *fmt = "\n%s, [%d] %s:%d, %s:%d\n";
	char *owner[] = {"pp", "dec"};
	char *desc = ((FrameFlowArg*)arg)->desc;
	VideoFrameInfo *frame = ((FrameFlowArg*)arg)->frame;

	if(thiz && thiz->enable) {
		gx_printf(fmt, desc, (frame)->disp_num, owner[(frame)->top.is_ref], (frame)->top.fb_id, owner[(frame)->bottom.is_ref], (frame)->bottom.fb_id);
	}
}

static void print_field_info(VPrintNode *thiz, void *arg)
{
	char *field_type = "BT";
	char *start_end  = "es";
	unsigned char is_start     = ((FieldInfoArg*)arg)->is_start;
	unsigned char is_top_field = ((FieldInfoArg*)arg)->is_top_field;

	if(thiz && thiz->enable) {
		gx_printf("\n%c-- %c ---\n",field_type[is_top_field], start_end[is_start]);
	}
}

static void print_sync_info(VPrintNode *thiz, void *arg)
{
	char *fmt = "%sstc=%08u, pts=%08u, pts-stc=%d\n";
	char *desc = ((SyncInfoArg*)arg)->desc;
	unsigned stc = ((SyncInfoArg*)arg)->stc;
	unsigned pts = ((SyncInfoArg*)arg)->pts;

	if(thiz && thiz->enable) {
		gx_printf(fmt, desc, stc, pts, (int)(pts-stc));
	}
}

static void print_ptsfixer_info(VPrintNode *thiz, void *arg)
{
	char *fmt = "pts = %u, f_pts = %u, f_pts-pts = %d\n";
	unsigned pts   = ((PtsFixerInfoArg*)arg)->pts;
	unsigned f_pts = ((PtsFixerInfoArg*)arg)->fixed_pts;

	if(thiz && thiz->enable) {
		if(pts && (f_pts != pts)) {
			gx_printf(fmt, pts, f_pts, (int)f_pts-(int)pts);
		}
	}
}

static void print_dec_info(VPrintNode *thiz, void *arg)
{
	char     *desc   = ((DecInfoArg*)arg)->desc;
	int      dec_id  = ((DecInfoArg*)arg)->dec_id;
	int      disp_id = ((DecInfoArg*)arg)->disp_id;
	unsigned fb_mask = ((DecInfoArg*)arg)->mask;

	if(thiz && thiz->enable) {
		if(strcasecmp(desc, "dec one") == 0) {
			gx_printf("%s, mask = 0x%x\n", desc, fb_mask);
		} else if (strcasecmp(desc, "dec isr") == 0) {
			gx_printf("%s, dec: %d, disp: %d\n", desc, dec_id, disp_id);
		} else if (strcasecmp(desc, "dec clr") == 0) {
			gx_printf("%s: %d\n", desc, dec_id);
		}
	}
}

static void print_apts_vpts_pcr(VPrintNode *thiz, void *arg)
{
	char *desc = ((PtsViewer*)arg)->desc;
	unsigned apts = ((PtsViewer*)arg)->apts;
	unsigned vpts = ((PtsViewer*)arg)->vpts;
	unsigned pcr  = ((PtsViewer*)arg)->pcr;

	if(thiz && thiz->enable) {
		gx_printf("%s%u\t%u\t%u\n", desc, apts, vpts, pcr);
	}
}

static void print_esv_info(VPrintNode *thiz, void *arg)
{
	EsvInfo *esv = (EsvInfo*)arg;

	if (thiz && thiz->enable) {
		gx_printf("esv_cnt = %u\n", esv->cnt);
	}
}


VDebuger vdebuger = {
	.frame_info = {
		.enable = 0,
		.desc   = "print dec frame info : [id] [pic type] [err] [pts] [pts step]",
		.print  = print_frame_info,
	},
	.frame_flow = {
		.enable = 0,
		.desc   = "print frame flow: [dec|pp|disp] [put|get|disp|clr] [fb id]",
		.print  = print_frame_flow,
	},
	.field_info = {
		.enable = 0,
		.desc   = "print field info: [T|B] [start|end]",
		.print  = print_field_info,
	},
	.sync_info = {
		.enable = 0,
		.desc   = "print sync info: [cm|re|sk] [stc] [pts] [pts-stc]",
		.print  = print_sync_info,
	},
	.ptsfixer_info = {
		.enable = 0,
		.desc   = "[pts] [fixed_pts], [fixed_pts-pts]",
		.print  = print_ptsfixer_info,
	},
	.dec_info = {
		.enable = 0,
		.desc   = "[dec-dec|isr|clr], [dec] [disp] [mask]",
		.print  = print_dec_info,
	},
	.pts_viewer = {
		.enable = 0,
		.desc   = "apts vpts pcr",
		.print  = print_apts_vpts_pcr,
	},
	.esv_info = {
		.enable = 0,
		.desc   = "print esv cnt in each vpu interrupt",
		.print  = print_esv_info,
	},
	.force_sync = {
		.enable = 0,
		.desc   = "force sync node: 0-freerun, 1-normal, 2-slowly, 3-later",
		/**
		  * SYNC_MODE_FREERUN     = 0
		  * SYNC_MODE_NORMAL      = 1
		  * SYNC_MODE_SYNC_SLOWLY = 2
	      * SYNC_MODE_SYNC_LATER  = 3
		 **/
		.force_val = 0,
	},
	.force_pp = {
		.enable    = 0,
		.desc      = "force pp: 0-disable, 1-enable",
		.force_val = 0,
	},
};

#endif
