#ifndef __V_DEBUG_H__
#define __V_DEBUG_H__

//#define V_DEBUG

#include "frame.h"

typedef struct v_debuger    VDebuger;
typedef struct v_print_node VPrintNode;
typedef struct v_force_node VForceNode;

struct v_print_node {
	char   enable;
	char  *desc;
	void (*print) (VPrintNode *thiz, void *arg);
};

struct v_force_node {
	int   enable;
	char *desc;
	int   force_val;
};

struct v_debuger {
	VPrintNode frame_info;
	VPrintNode frame_flow;
	VPrintNode field_info;
	VPrintNode sync_info;
	VPrintNode ptsfixer_info;
	VPrintNode dec_info;
	VPrintNode pts_viewer;
	VPrintNode esv_info;

	VForceNode force_sync;
	VForceNode force_pp;
};

#ifndef V_DEBUG
	#define VDBG_PRINT_FRAME_INFO(frame)                    {;}
	#define VDBG_PRINT_FRAME_FLOW(desc, frame)              {;}
	#define VDBG_PRINT_FIELD_INFO(is_top_field, is_start)   {;}
	#define VDBG_PRINT_SYNC_INFO(desc, stc, pts)            {;}
	#define VDBG_PRINT_PTSFIXER_INFO(pts, fix_pts)          {;}
	#define VDBG_PRINT_DEC_INFO(desc, dec, disp, mask)      {;}
	#define VDBG_PRINT_APTS_VPTS_PCR(desc, apts, vpts, pcr) {;}
	#define VDBG_PRINT_ESV()                                {;}
	#define VDBG_FORCE_SYNC(sync)                           {;}
	#define VDBG_FORCE_PP()                                 {;}
	#define VDBG_PRINT(fmt, args...)                        {;}
#else
	extern VDebuger vdebuger;

	typedef struct {
		VideoFrameInfo *frame;
	}FrameInfoArg;
	#define VDBG_PRINT_FRAME_INFO(_frame)\
	do {\
		FrameInfoArg arg;\
		arg.frame = (_frame);\
		vdebuger.frame_info.print(&vdebuger.frame_info, &arg);\
	} while(0)

	typedef struct {
		char           *desc;
		VideoFrameInfo *frame;
	}FrameFlowArg;
	#define VDBG_PRINT_FRAME_FLOW(_desc, _frame)\
	do {\
		FrameFlowArg arg;\
		arg.desc  = (_desc);\
		arg.frame = (_frame);\
		vdebuger.frame_flow.print(&vdebuger.frame_flow, &arg);\
	} while(0)

	typedef struct {
		unsigned char is_top_field;
		unsigned char is_start;
	}FieldInfoArg;
	#define VDBG_PRINT_FIELD_INFO(_is_top_field, _is_start)\
	do {\
		FieldInfoArg arg;\
		arg.is_top_field = (_is_top_field);\
		arg.is_start     = (_is_start);\
		vdebuger.field_info.print(&vdebuger.field_info, &arg);\
	} while(0)

	typedef struct {
		char     *desc;
		unsigned  stc, pts;
	}SyncInfoArg;
	#define VDBG_PRINT_SYNC_INFO(_desc, _stc, _pts)\
	do {\
		SyncInfoArg arg;\
		arg.desc = (_desc);\
		arg.stc  = (_stc);\
		arg.pts  = (_pts);\
		vdebuger.sync_info.print(&vdebuger.sync_info, &arg);\
	} while(0)

	typedef struct {
		unsigned pts, fixed_pts;
	}PtsFixerInfoArg;
	#define VDBG_PRINT_PTSFIXER_INFO(_pts, _fixed_pts)\
	do {\
		PtsFixerInfoArg arg;\
		arg.pts       = (_pts);\
		arg.fixed_pts = (_fixed_pts);\
		vdebuger.ptsfixer_info.print(&vdebuger.ptsfixer_info, &arg);\
	} while(0)

	typedef struct {
		char     *desc;
		int      dec_id, disp_id;
		unsigned mask;
	}DecInfoArg;
	#define VDBG_PRINT_DEC_INFO(_desc, _dec, _disp, _mask)\
	do {\
		DecInfoArg arg;\
		arg.desc    = (_desc);\
		arg.dec_id  = (_dec);\
		arg.disp_id = (_disp);\
		arg.mask    = (_mask);\
		vdebuger.dec_info.print(&vdebuger.dec_info, &arg);\
	} while(0)

	typedef struct {
		char *desc;
		unsigned apts, vpts, pcr;
	}PtsViewer;
	#define VDBG_PRINT_APTS_VPTS_PCR(_desc, _apts, _vpts, _pcr)\
	do {\
		PtsViewer arg;\
		arg.desc = (_desc);\
		arg.apts = (_apts);\
		arg.vpts = (_vpts);\
		arg.pcr  = (_pcr);\
		vdebuger.pts_viewer.print(&vdebuger.pts_viewer, &arg);\
	} while(0)

	typedef struct {
		unsigned cnt;
	} EsvInfo;
	#define VDBG_PRINT_ESV()\
	do {\
		EsvInfo arg = {0};\
		gxav_sdc_length_get(0, &arg.cnt);\
		vdebuger.esv_info.print(&vdebuger.esv_info, &arg);\
	} while(0)

	#define VDBG_FORCE_SYNC(sync)\
	do {\
		if (sync && vdebuger.force_sync.enable) {\
			sync->sync_mode = vdebuger.force_sync.force_val;\
			gx_printf("[v-dbg] force sync, mode = %d\n", vdebuger.force_sync.force_val);\
		}\
	} while(0)

	#define VDBG_FORCE_PP()\
	do {\
		if (vdebuger.force_pp.enable) {\
			return  vdebuger.force_pp.force_val;\
		}\
	} while(0)

	#define VDBG_PRINT(fmt, args...)  gx_printf(fmt, ##args)
#endif

#endif

