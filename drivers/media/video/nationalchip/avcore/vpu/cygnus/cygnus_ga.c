#include "vpu_color.h"
#include "cygnus_ga.h"
#include "cygnus_ga_internel.h"
#include "kernelcalls.h"
#include "profile.h"
#include "firewall.h"
#include "clock_hal.h"

#ifdef LINUX_OS
#include <linux/slab.h>
#endif

static unsigned int GA_REG_BASE_ADDR = 0x04700000;

static unsigned int ga_zt_coefficient[] = {
	0x10909010, 0x00ff0100, 0x00ff0201, 0x00ff0302,
	0x00ff0403, 0x00ff0504, 0x00fe0705, 0x00fe0806,
	0x00fd0a07, 0x00fc0b07, 0x00fc0c08, 0x00fb0e09,
	0x01fa1009, 0x01f9120a, 0x01f8140b, 0x01f7150b,
	0x01f6170c, 0x01f5180c, 0x02f41b0d, 0x02f31c0d,
	0x02f11f0e, 0x02f0200e, 0x03ef230f, 0x03ed250f,
	0x03ec2710, 0x03ea2910, 0x04e92b10, 0x04e72d10,
	0x04e53011, 0x05e33311, 0x05e13511, 0x05df3711,
	0x06de3a12, 0x06dc3c12, 0x06da3e12, 0x06d74112,
	0x07d54412, 0x07d34612, 0x07d14812, 0x08cf4b12,
	0x08cd4d12, 0x08ca5012, 0x09c85312, 0x09c65512,
	0x09c35812, 0x0ac15b12, 0x0abf5d12, 0x0abc6012,
	0x0bba6312, 0x0bb76612, 0x0bb56812, 0x0bb26b12,
	0x0cb06e12, 0x0cad7112, 0x0daa7512, 0x0da87611,
	0x0da57911, 0x0ea27d11, 0x0ea07f11, 0x0e9d8211,
	0x0e9a8410, 0x0f988710, 0x0f958a10, 0x0f928d10,
};

static int ga_data_type[] = {
	0, //GX_COLOR_FMT_CLUT1
	1, //GX_COLOR_FMT_CLUT2
	2, //GX_COLOR_FMT_CLUT4
	3, //GX_COLOR_FMT_CLUT8
	5, //GX_COLOR_FMT_RGBA4444
	6, //GX_COLOR_FMT_RGBA5551
	4, //GX_COLOR_FMT_RGB565
	7, //GX_COLOR_FMT_RGBA8888
	7, //GX_COLOR_FMT_ABGR8888
	7, //GX_COLOR_FMT_RGB888
	7, //GX_COLOR_FMT_BGR888
	17, //GX_COLOR_FMT_ARGB4444
	19, //GX_COLOR_FMT_ARGB1555
	7,  //GX_COLOR_FMT_ARGB8888

	9,  //GX_COLOR_FMT_YCBCR422
	11, //GX_COLOR_FMT_YCBCRA6442
	-1, //GX_COLOR_FMT_YCBCR420  /*same as GX_COLOR_FMT_YCBCR420_Y_UV*/

	22, //GX_COLOR_FMT_YCBCR420_Y_UV
	10, //GX_COLOR_FMT_YCBCR420_Y_U_V
	21, //GX_COLOR_FMT_YCBCR420_Y
	21, //GX_COLOR_FMT_YCBCR420_U
	21, //GX_COLOR_FMT_YCBCR420_V
	-1, //GX_COLOR_FMT_YCBCR420_UV

	-1, //GX_COLOR_FMT_YCBCR422_Y_UV
	-1, //GX_COLOR_FMT_YCBCR422_Y_U_V
	-1, //GX_COLOR_FMT_YCBCR422_Y
	-1, //GX_COLOR_FMT_YCBCR422_U
	-1, //GX_COLOR_FMT_YCBCR422_V
	-1, //GX_COLOR_FMT_YCBCR422_UV

	14, //GX_COLOR_FMT_YCBCR444
	-1, //GX_COLOR_FMT_YCBCR444_Y_UV
	-1, //GX_COLOR_FMT_YCBCR444_Y_U_V
	-1, //GX_COLOR_FMT_YCBCR444_Y
	-1, //GX_COLOR_FMT_YCBCR444_U
	-1, //GX_COLOR_FMT_YCBCR444_V
	-1, //GX_COLOR_FMT_YCBCR444_UV

	15, //GX_COLOR_FMT_YCBCR400
	16, //GX_COLOR_FMT_A8
	18, //GX_COLOR_FMT_ABGR4444
	20, //GX_COLOR_FMT_ABGR1555
	21, //GX_COLOR_FMT_Y8
	22, //GX_COLOR_FMT_UV16
	12, //GX_COLOR_FMT_YCBCR422v
	13, //GX_COLOR_FMT_YCBCR422h
	 8, //GX_COLOR_FMT_YUVA8888
};

/*
   struct {
   GxAluMode   mode;
   BlendFun    src_func;
   BlendFun    dst_func;
   }mix_matrix[] = {
   {GX_ALU_MIX_CLEAR   ,   GABF_ZERO           ,   GABF_ZERO           },
   {GX_ALU_MIX_A       ,   GABF_ONE            ,   GABF_ZERO           },
   {GX_ALU_MIX_B       ,   GABF_ZERO           ,   GABF_ONE            },
   {GX_ALU_MIX_A_OVER_B,   GABF_ONE            ,   GABF_INVSRCALPHA    },
   {GX_ALU_MIX_B_OVER_A,   GABF_INVDESTALPHA   ,   GABF_ONE            },
   {GX_ALU_MIX_A_IN_B  ,   GABF_DESTALPHA      ,   GABF_ZERO           },
   {GX_ALU_MIX_B_IN_A  ,   GABF_ZERO           ,   GABF_SRCALPHA       },
   {GX_ALU_MIX_A_OUT_B ,   GABF_INVDESTALPHA   ,   GABF_ZERO           },
   {GX_ALU_MIX_B_OUT_A ,   GABF_ZERO           ,   GABF_INVSRCALPHA    },
   {GX_ALU_MIX_A_TOP_B ,   GABF_DESTALPHA      ,   GABF_INVSRCALPHA    },
   {GX_ALU_MIX_B_TOP_A ,   GABF_INVDESTALPHA   ,   GABF_SRCALPHA       },
   {GX_ALU_MIX_A_XOR_B ,   GABF_INVDESTALPHA   ,   GABF_INVSRCALPHA    },
   {GX_ALU_MIX_A_TO_B  ,   GABF_INVSRCALPHA    ,   GABF_SRCALPHA       },
   {GX_ALU_MIX_B_TO_A  ,   GABF_DESTALPHA      ,   GABF_INVDESTALPHA   },
   {GX_ALU_MIX_NONE    ,   GABF_SRCALPHA       ,   GABF_INVSRCALPHA    },
   {GX_ALU_MIX_ADD     ,   GABF_ONE            ,   GABF_ONE            },
   };
   */

static GA  *cygnusga = NULL;
static volatile CygnusGaReg *cygnusga_reg = NULL;

static GaJob* ga_job_create(unsigned int max_cmd_num);
static GaRet  ga_job_addcmd(GaJob *job, GaCmd *cmd);
static GaRet  ga_job_submit(GaJob *job);
static GaRet  ga_job_destroy(GaJob *job);

static GaRet  ga_joblist_init(void);
static GaRet  ga_joblist_addjob(GaJob *job);
static GaJob* ga_joblist_getcurrent(int create_if_null);
static GaRet  ga_joblist_removejob(GaJob **job);
static GaRet  ga_joblist_destroy(void);

static GaRet ga_cmd_blit(GaBlitParam *blit_param);
static GaRet ga_cmd_fillrect(GaFillRectParam *fillrect);
static GaRet ga_cmd_zoom(GaZoomParam *zoom);
static GaRet ga_cmd_turn(GaTurnParam *turn);
static GaRet ga_cmd_complet(GaCompletParam *complet);
static GaRet ga_submit_cmd(GaCmd *cmd);

static void gx_blitobj_getcct(GxBlitObject * obj, GxClutConvertTable* cct);
static void gx_ga_end(void);
static int ga_clk_enable;
static int ga_level;

static GaJob* ga_job_create(unsigned int max_cmd_num)
{
	char *base;
	GaJob *job = NULL;
	unsigned jobmem_size, cmds_size, ccts_size, job_block_size;

	cmds_size = max_cmd_num*GA_CMD_SIZE;
	ccts_size = max_cmd_num*GA_CCT_SIZE;
	// buffer地址要8对齐，文档上没看到，差点进坑
	// GaJob结构体不要与给设备使用的Buffer共享Cache Line，还是进坑了。:D
	job_block_size = ((sizeof(GaJob) / 64) + 1) * 64;

	jobmem_size = job_block_size + cmds_size + ccts_size;
	base = (char*)gx_dma_malloc(jobmem_size);
	if(base != NULL) {
		memset(base, 0, jobmem_size);
		job = (GaJob*)base;
		job->jobmem_size = jobmem_size;
		job->max_cmd_num = max_cmd_num;
		job->cmd_start_addr = (void*)(base + job_block_size);
		job->cct_start_addr = (void*)(base + job_block_size + cmds_size);
	}
	ga_level++;

	return job;
}

static GaRet ga_job_addcmd(GaJob *job, GaCmd *cmd)
{
	unsigned int addr, offset;
	if(!ga_clk_enable) {
		gxav_clock_module_enable(MODULE_TYPE_GA, 1);
		ga_clk_enable = 1;
	}

	IF_TRUE_RETURN_WITH_VAL(job==NULL, GA_RET_FAIL);

	if(job->cur_cmd_num+1 > job->max_cmd_num)
		return GA_RET_FAIL;

	offset = job->cur_cmd_num*GA_CMD_SIZE;
	addr   = (unsigned int)job->cmd_start_addr + offset;
	GA_COMPRESS_CMD(addr, cmd);
	job->cur_cmd_num ++;

	return GA_RET_OK;
}

static GaRet ga_job_submit(GaJob *job)
{
	IF_TRUE_RETURN_WITH_VAL(job==NULL, GA_RET_FAIL);

	if(cygnusga->mode == GX_GAMODE_BATCH) {
		if(job->cur_cmd_num > 0) {
			gx_dcache_clean_range(0,0);

			job->fin_cmd_num = 0;
			SUBMIT_JOB_TO_GA(cygnusga_reg, job);
			gx_sem_wait(&(cygnusga->job_fin_sem));
			GA_SET_INTERRUPT_CLR(cygnusga_reg->rGA_INTERRUPT);

			gx_dcache_inv_range(0,0);
		}
	} else {
		unsigned int addr, offset;

		gx_dcache_clean_range(0,0);

		GA_SET_CMD_NUM ( cygnusga_reg->rCMD_GAIN, 1);

		gx_interrupt_mask(GA_INT_SRC);
		GA_SEND_CMD_NUM_SIGNAL( cygnusga_reg->rCMD_GAIN);
		REG_SET_BIT(&(cygnusga_reg->rGA_INTERRUPT), bGA_INTERRUPT_CLR);
		REG_CLR_BIT(&(cygnusga_reg->rGA_INTERRUPT), bGA_INTERRUPT_CLR);
		job->ready = 1;
		gx_interrupt_unmask(GA_INT_SRC);

		if (job->cur_cmd_num == job->max_cmd_num) job->cur_cmd_num = 0;

		offset = job->cur_cmd_num * GA_CMD_SIZE;
		addr = gx_virt_to_phys((unsigned int)(job)->cmd_start_addr + offset);

		/* If write cmd too fast , here to wait */
		while (addr == REG_GET_VAL(&cygnusga_reg->rCMD_READ_ADDR));
	}
	ga_level--;
	if(ga_clk_enable && (!ga_level)) {
		gxav_clock_module_enable(MODULE_TYPE_GA, 0);
		ga_clk_enable = 0;
	}

	return GA_RET_OK;
}

static GaRet ga_job_destroy(GaJob *job)
{
	IF_TRUE_RETURN_WITH_VAL(job == NULL, GA_RET_FAIL);

	if(cygnusga->mode == GX_GAMODE_FLOW) {
		unsigned int addr_cur, addr_start, size;

		addr_cur = REG_GET_VAL(&cygnusga_reg->rCMD_READ_ADDR);
		addr_start = gx_virt_to_phys((unsigned int)job->cmd_start_addr);
		size = addr_cur - addr_start;

		/*
		 *  GA硬件上需要在DISABLE GA 时当前执行的任务地址为cmd_buffer_length的最后一块地址,
		 *  故在这里判断并添加任务确保这一点
		 */
		if (size != (job->max_cmd_num - 1) * GA_CMD_SIZE) {
			GaCmd dummy;
			void *dummy_data = gx_malloc(32);
			if (NULL == dummy_data) {
				gx_printf("GA error!:%s\n", __func__);
				while(1);
			}

			GA_SET_CMD_BUF_LENGTH( cygnusga_reg->rCMD_BUFFER_LENGTH, size + 2*GA_CMD_SIZE);

			memset(&dummy, 0, sizeof(dummy));
			dummy.SrcA.Addr = gx_virt_to_phys((unsigned int)dummy_data);
			dummy.SrcA.Base_Width = 32;
			dummy.SrcA.Width = 32;
			dummy.SrcA.Height = 1;
			dummy.SrcA.Data_Type = 3;

			dummy.Dst = dummy.SrcB = dummy.SrcA;

			dummy.GaMode.Inter_Mode = 1;
			dummy.GaMode.Ga_Op_Mode = 1;
			dummy.Endianness.Endianness_Srca_Read_Mode = 0xfac688;
			dummy.Endianness.Endianness_Srcb_Read_Mode = 0xfac688;
			dummy.Endianness.Endianness_Write_Mode = 0xfac688;

			ga_job_addcmd(job, &dummy);
			ga_job_submit(job);
			gx_ga_end();
			gx_free(dummy_data);
		}
	}

	gx_dma_free(job, job->jobmem_size);

	return GA_RET_OK;
}

static GaRet ga_joblist_init(void)
{
	if(cygnusga) {
		cygnusga->joblist.zt_coefficient = (unsigned int)gx_malloc(sizeof(ga_zt_coefficient));
		if(cygnusga->joblist.zt_coefficient)
			gx_memcpy((void*)cygnusga->joblist.zt_coefficient, (void*)ga_zt_coefficient, sizeof(ga_zt_coefficient));
		else
			gx_printf("malloc zt_coefficient failed!\n");

		cygnusga->joblist.job_num = 0;
		gx_memset(cygnusga->joblist.jobs, 0, sizeof(GaJob*)*GA_MAX_JOB_NUM);
	}

	return GA_RET_OK;
}

static GaRet ga_joblist_addjob(GaJob *job)
{
	unsigned int job_num = cygnusga->joblist.job_num;

	if((job == NULL) || (cygnusga->joblist.job_num + 1 > GA_MAX_JOB_NUM))
		return GA_RET_FAIL;

	cygnusga->joblist.jobs[job_num] = job;
	cygnusga->joblist.job_num ++;

	return GA_RET_OK;
}

static GaJob* ga_joblist_getcurrent(int create_if_null)
{
	unsigned int cur_job, job_num = cygnusga->joblist.job_num;

	if(job_num > 0) {
		cur_job = job_num - 1;
		return cygnusga->joblist.jobs[cur_job];
	}
	else if(create_if_null) {
		GaJob *job;
		job = ga_job_create(1);
		ga_joblist_addjob(job);
		return job;
	}
	else
		return NULL;
}

static GaRet ga_joblist_removejob(GaJob **job)
{
	unsigned int cur_job, job_num = cygnusga->joblist.job_num;

	if(job_num == 0)
		return GA_RET_FAIL;

	cur_job = job_num - 1;
	if(job != NULL)
		*job =  cygnusga->joblist.jobs[cur_job];

	cygnusga->joblist.jobs[cur_job] = NULL;
	cygnusga->joblist.job_num --;

	return GA_RET_OK;
}

static GaRet ga_joblist_destroy(void)
{
	unsigned int i, job_num = cygnusga->joblist.job_num;

	for(i = 0; i < job_num; i++)
	{
		ga_job_destroy(cygnusga->joblist.jobs[i]);
	}
	cygnusga->joblist.job_num = 0;
	if(cygnusga->joblist.zt_coefficient)
		gx_free((void*)cygnusga->joblist.zt_coefficient);

	return GA_RET_OK;
}

#define IS_JOB_READY(job) (job)&&((job)->cur_cmd_num==(job)->max_cmd_num)
static GaRet ga_submit_cmd_nolock(GaCmd *cmd)
{
	GaJob *job;

	job = ga_joblist_getcurrent(1);
	ga_job_addcmd(job, cmd);

	if(cygnusga->mode == GX_GAMODE_BATCH) {
		if(IS_JOB_READY(job)) {
			ga_job_submit(job);
			ga_joblist_removejob(NULL);
			ga_job_destroy(job);
		}
	}else
		ga_job_submit(job);

	return GA_RET_OK;
}

static GaRet ga_submit_cmd(GaCmd *cmd)
{
	int ret = 0;

	gx_sem_wait(&cygnusga->job_lock);

	ret = ga_submit_cmd_nolock(cmd);

	gx_sem_post(&cygnusga->job_lock);

	return (ret);
}

static GaRet ga_cmd_blit_modulator_config(GaCmd* cmd, GaBlitParam *blit)
{
	if(blit->src_a.modulator.premultiply_en) {
		cmd->Modulator.Premultiply_En = 1;
		cmd->Modulator.Modulator_Src_Color = blit->src_a.modulator.reg_color;
		if(blit->src_a.modulator.step1_en)
			cmd->Modulator.Modulator_Mode_SrcA = (1<<2)| blit->src_a.modulator.step1_mode;
		if(blit->src_a.modulator.step2_en)
			cmd->Modulator.Modulator_Mode_SrcB = (1<<2)| blit->src_a.modulator.step2_mode;
		if(blit->src_a.modulator.step3_en)
			cmd->Modulator.Modulator_Mode_SrcC = (1<<2)| blit->src_a.modulator.step3_mode;
	}
	if(blit->src_b.modulator.premultiply_en) {
		cmd->Modulator.Premultiply_En = 1;
		cmd->Modulator.Modulator_Dst_Color = blit->src_b.modulator.reg_color;
		if(blit->src_b.modulator.step1_en)
			cmd->Modulator.Modulator_Mode_DstA = (1<<2)| blit->src_b.modulator.step1_mode;
		if(blit->src_b.modulator.step2_en)
			cmd->Modulator.Modulator_Mode_DstB = (1<<2)| blit->src_b.modulator.step2_mode;
		if(blit->src_b.modulator.step3_en)
			cmd->Modulator.Modulator_Mode_DstC = (1<<2)| blit->src_b.modulator.step3_mode;
	}

	return GA_RET_OK;
}

static GaRet ga_cmd_blit_rop_config(GaCmd* cmd, GaBlitParam *blit)
{
	cmd->Rop.Rop_En  = 1;
	cmd->Rop.Rop_Code= blit->mode;
	GA_CMD_SET_COLORKEY_INFO(cmd->ColorKey, blit->colorkey_info);

	return GA_RET_OK;
}

static GaRet ga_blend_func_config(GaCmd* cmd, GxAluMode mode)
{
	cmd->Blender.Blend_En = 1;
	switch(mode)
	{
		case GX_ALU_MIX_CLEAR:
			cmd->Blender.Blend_Src_Fun = GABF_ZERO;
			cmd->Blender.Blend_Dst_Fun = GABF_ZERO;
			break;
		case GX_ALU_MIX_A:
			cmd->Blender.Blend_Src_Fun = GABF_ONE;
			cmd->Blender.Blend_Dst_Fun = GABF_ZERO;
			break;
		case GX_ALU_MIX_B:
			cmd->Blender.Blend_Src_Fun = GABF_ZERO;
			cmd->Blender.Blend_Dst_Fun = GABF_ONE;
			break;
		case GX_ALU_MIX_A_OVER_B:
			cmd->Blender.Blend_Src_Fun = GABF_ONE;
			cmd->Blender.Blend_Dst_Fun = GABF_INVSRCALPHA;
			break;
		case GX_ALU_MIX_B_OVER_A:
			cmd->Blender.Blend_Src_Fun = GABF_INVDESTALPHA;
			cmd->Blender.Blend_Dst_Fun = GABF_ONE;
			break;
		case GX_ALU_MIX_A_IN_B:
			cmd->Blender.Blend_Src_Fun = GABF_DESTALPHA;
			cmd->Blender.Blend_Dst_Fun = GABF_ZERO;
			break;
		case GX_ALU_MIX_B_IN_A:
			cmd->Blender.Blend_Src_Fun = GABF_ZERO;
			cmd->Blender.Blend_Dst_Fun = GABF_SRCALPHA;
			break;
		case GX_ALU_MIX_A_OUT_B:
			cmd->Blender.Blend_Src_Fun = GABF_INVDESTALPHA;
			cmd->Blender.Blend_Dst_Fun = GABF_ZERO;
			break;
		case GX_ALU_MIX_B_OUT_A:
			cmd->Blender.Blend_Src_Fun = GABF_ZERO;
			cmd->Blender.Blend_Dst_Fun = GABF_INVSRCALPHA;
			break;
		case GX_ALU_MIX_A_TOP_B:
			cmd->Blender.Blend_Src_Fun = GABF_DESTALPHA;
			cmd->Blender.Blend_Dst_Fun = GABF_INVSRCALPHA;
			break;
		case GX_ALU_MIX_B_TOP_A:
			cmd->Blender.Blend_Src_Fun = GABF_INVDESTALPHA;
			cmd->Blender.Blend_Dst_Fun = GABF_SRCALPHA;
			break;
		case GX_ALU_MIX_A_XOR_B:
			cmd->Blender.Blend_Src_Fun = GABF_INVDESTALPHA;
			cmd->Blender.Blend_Dst_Fun = GABF_INVSRCALPHA;
			break;
		case GX_ALU_MIX_NONE:
			cmd->Blender.Blend_Src_Fun = GABF_SRCALPHA;
			cmd->Blender.Blend_Dst_Fun = GABF_INVSRCALPHA;
			break;
		case GX_ALU_MIX_ADD:
			cmd->Blender.Blend_Src_Fun = GABF_ONE;
			cmd->Blender.Blend_Dst_Fun = GABF_ONE;
			break;
		case GX_ALU_MIX_TRUE:
			cmd->Blender.Blend_Src_Fun = GABF_TRUE;
			cmd->Blender.Blend_Dst_Fun = GABF_TRUE;
			break;
		case GX_ALU_PAVE:
			cmd->Blender.Blend_Src_Fun = GABF_TRUE;
			cmd->Blender.Blend_Dst_Fun = GABF_TRUE;
			break;
		default:
			break;
	}
	return GA_RET_OK;
}


static GaRet ga_cmd_blit_mix_config(GaCmd* cmd, GaBlitParam *blit)
{
	if(blit->src_a.global_alpha_en) {
		cmd->Blender.Src_Blend_Mode	= 1; /* global alpha */
		cmd->Blender.Blend_Src_Color= blit->src_a.global_alpha&0xff;
	}
	if(blit->dst.global_alpha_en) {
		cmd->Blender.Dst_Blend_Mode	= 1; /* global alpha */
		cmd->Blender.Blend_Dst_Color= blit->dst.global_alpha&0xff;
	}
	ga_blend_func_config(cmd, blit->mode);
	GA_CMD_SET_COLORKEY_INFO(cmd->ColorKey, blit->colorkey_info);

	return GA_RET_OK;
}

static GaRet ga_cmd_blit_pave_config(GaCmd* cmd, GaBlitParam *blit)
{
	if(blit->src_a.global_alpha_en) {
		cmd->Blender.Src_Blend_Mode	= 1; /* global alpha */
	}
	if(blit->dst.global_alpha_en) {
		cmd->Blender.Dst_Blend_Mode	= 1; /* global alpha */
	}
	ga_blend_func_config(cmd, blit->mode);
	cmd->Blender.Blend_En = 0;
	cmd->Pave_En = 1;
	cmd->Blender.Blend_En = 0;

	return GA_RET_OK;
}

#define GA_GET_CCT_COLOR_VAL(color_val, dst_bpp)\
	do {\
		typeof(dst_bpp)     _dst_bpp    = (dst_bpp);\
		typeof(color_val)*  _color_val  =&(color_val);\
		*_color_val = ((color_val)>>(32 - _dst_bpp));\
	}while(0)
static GaRet ga_cmd_blit_vfcopy_config(GaCmd* cmd, GaBlitParam *blit)
{
	unsigned int color;
	GxClutConvertTable *cct;

	cct = &blit->src_a.clut_convert_table;
	color = cct->table_addr[1];/*[0] background, [1]foreground*/
	GA_GET_CCT_COLOR_VAL(color, blit->dst.bpp);
	if( blit->dst.bpp == 32  && blit->dst.argb_flag) {
		color =  ((color&0xffffff) << 8) | (color>>24);
	}
	cmd->Clut_A.Clut_En = 0;
	cmd->SrcA.Data_Type = ga_data_type[GX_COLOR_FMT_A8];

	cmd->Blender.Blend_En= 1;
	cmd->Modulator.Modulator_Src_Color = color;
	cmd->Clut_A.Clut_Type = blit->dst.data_type;
	cmd->Blender.Blend_Src_Fun = GABF_SRCALPHA;
	cmd->Blender.Blend_Dst_Fun = GABF_INVSRCALPHA;

	return GA_RET_OK;
}

#define GET_RD_SEQENCE(flag, bpp, mode)\
	do{\
		if(bpp == 32)\
			mode = (flag) ? 0xbbc298 : 0x977053;\
		else if(bpp == 24) \
			mode = 0xfac688; \
		else if(bpp == 16)\
			mode = 0xde54c1;\
		else\
			mode = 0xfac688;\
	}while(0)

#define GET_WR_SEQENCE(flag, bpp, mode)\
	do{\
		if(bpp == 32)\
			mode = (flag) ? 0xf2e60a : 0x977053;\
		else if(bpp == 24) \
			mode = 0xfac688; \
		else if(bpp == 16)\
			mode = 0xde54c1;\
		else\
			mode = 0xfac688;\
	}while(0)

static GaRet ga_cmd_blit(GaBlitParam *blit)
{
	GaCmd cmd;
	unsigned int wmode, srca_rmode, srcb_rmode, uv_addr;
	gx_memset(&cmd, 0, sizeof(GaCmd));

	cmd.GaMode.Ga_Op_Mode = 1;
	cmd.GaMode.Inter_Mode = 1;
	if((blit->src_a.data_type != blit->dst.data_type) &&
	   (blit->src_a.data_type == ga_data_type[GX_COLOR_FMT_YCBCR422]) &&
	   (blit->dst.data_type == ga_data_type[GX_COLOR_FMT_RGB565]))
		cmd.GaMode.Dither_Mode = 1;
	GA_CMD_SET_CLUT_CONVERT_INFO(cmd.Clut_A, blit->src_a);
	GA_CMD_SET_CLUT_CONVERT_INFO(cmd.Clut_B, blit->src_b);
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcA, blit->src_a);
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcB, blit->src_b);
	GA_CMD_SET_CHANNEL_INFO(cmd.Dst , blit->dst);

	if(blit->src_b.clut_convert_table.table_addr) {
		cmd.Clut_B.Clut_Addr = gx_virt_to_phys((unsigned int)(blit->src_b.clut_convert_table.table_addr));
	}

	if(blit->time_wait_en){
		cmd.GaMode.Ga_Cmd_Mode = 1;
		if(blit->dst.bpp == 8){
			cmd.GaMode.Ga_Cmd_Point = 35;
		}else if(blit->dst.bpp == 16){
			cmd.GaMode.Ga_Cmd_Point = 33;
		}else if(blit->dst.bpp == 32){
			cmd.GaMode.Ga_Cmd_Point = 31;
		}
		if(cmd.GaMode.Ga_Cmd_Point >=36){
			cmd.GaMode.Ga_Cmd_Point = 35;
		}
	}else{
		cmd.GaMode.Ga_Cmd_Mode = 0;
	}

	ga_cmd_blit_modulator_config(&cmd, blit);

	if(blit->mode <= GX_ALU_ROP_EXTEND) {
		ga_cmd_blit_rop_config(&cmd, blit);
		/*Use normal copy instead of rop copy in same color format*/
		if((blit->mode == GX_ALU_ROP_COPY) &&
		   (blit->src_a.color_format == blit->src_b.color_format) &&
		   (blit->src_b.color_format == blit->dst.color_format) &&
		   (32 < (blit->src_a.width * blit->src_a.bpp / 8))) {
			cmd.Rop.Rop_En  = 0;
			cmd.GaMode.Inter_Mode = 0;
		}
	}
	else if((blit->mode>=GX_ALU_MIX_CLEAR) && (blit->mode<=GX_ALU_MIX_TRUE))
		ga_cmd_blit_mix_config(&cmd, blit);
	else if(blit->mode == GX_ALU_VECTOR_FONT_COPY)
		ga_cmd_blit_vfcopy_config(&cmd, blit);
	else if(blit->mode == GX_ALU_PAVE)
		ga_cmd_blit_pave_config(&cmd, blit);

	if(blit->src_a.bpp == 24)
		cmd.RGB888_EN = 1;
	else
		cmd.RGB888_EN = 0;

	GET_RD_SEQENCE(blit->src_a.argb_flag, blit->src_a.bpp, srca_rmode);
	/*patch for spp */
	if(blit->src_a.data_type>=9 && blit->src_a.data_type<=11)
		srca_rmode = 0xfac688;

	GET_RD_SEQENCE(blit->src_b.argb_flag, blit->src_b.bpp, srcb_rmode);
	/*patch for spp */
	if(blit->src_b.data_type>=9 && blit->src_b.data_type<=11)
		srcb_rmode = 0xfac688;

	GET_WR_SEQENCE(blit->dst.argb_flag,  blit->dst.bpp, wmode);
	/*patch for spp */
	if(blit->dst.data_type>=9 && blit->dst.data_type<=11)
		wmode = 0xfac688;

	GA_CMD_SET_DATA_ENDIANNESS(cmd.Endianness, wmode, srca_rmode, srcb_rmode);

	/*patch for alpha out*/
	cmd.Blender.Alpha_Out_En = blit->alpha_out_info.alpha_out_en;
	cmd.Blender.Blend_Alpha_Out = blit->alpha_out_info.alpha_out;

	return ga_submit_cmd_nolock(&cmd);
}
static GaRet ga_cmd_fillrect(GaFillRectParam *fillrect)
{
	GaCmd cmd;
	unsigned int wmode, srca_rmode, srcb_rmode;
	gx_memset(&cmd, 0, sizeof(GaCmd));

	cmd.GaMode.Ga_Op_Mode = 0; /* fill mode */

	cmd.GaMode.Inter_Mode = 1;
	cmd.ColorKey.ColorKey_Src_Reg = fillrect->color;


	GA_CMD_SET_COLORKEY_INFO(cmd.ColorKey, fillrect->colorkey_info);
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcA, fillrect->dst);
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcB, fillrect->dst);
	GA_CMD_SET_CHANNEL_INFO(cmd.Dst , fillrect->dst);
	GET_WR_SEQENCE(fillrect->dst.argb_flag, fillrect->dst.bpp, wmode);
	GET_RD_SEQENCE(fillrect->dst.argb_flag, fillrect->dst.bpp, srca_rmode);
	srcb_rmode = srca_rmode;
	if(fillrect->time_wait_en){
		cmd.GaMode.Ga_Cmd_Mode = 1;
		if(fillrect->dst.bpp == 8){
			cmd.GaMode.Ga_Cmd_Point = 35;
		}else if(fillrect->dst.bpp == 16){
			cmd.GaMode.Ga_Cmd_Point = 33;
		}else if(fillrect->dst.bpp == 32){
			cmd.GaMode.Ga_Cmd_Point = 31;
		}
		if(cmd.GaMode.Ga_Cmd_Point >=36){
			cmd.GaMode.Ga_Cmd_Point = 35;
		}
	}else{
		cmd.GaMode.Ga_Cmd_Mode = 0;
	}

	/*patch for spp */
	if(fillrect->dst.data_type>=9 && fillrect->dst.data_type<=11)
		wmode = 0xfac688;
	GA_CMD_SET_DATA_ENDIANNESS(cmd.Endianness, wmode, srca_rmode, srcb_rmode);
	if(fillrect->blend_en) {
		ga_blend_func_config(&cmd, fillrect->blend_mode);
	}

	cmd.Xor_En = fillrect->draw_XOR_en;

	return ga_submit_cmd(&cmd);
}

static GaRet ga_cmd_zoom(GaZoomParam *zoom)
{
	GaCmd cmd;
	unsigned int wmode, srca_rmode, srcb_rmode;
	gx_memset(&cmd, 0, sizeof(GaCmd));

	cmd.Scaler.Scaler_En    = 1;
	cmd.GaMode.Ga_Op_Mode   = 1;
	cmd.GaMode.Inter_Mode   = 1;
	cmd.Scaler.Scaler_Mode  = zoom->stretch_en;
	cmd.Rotater.Rotater_Flip_Mode       = 0;
	cmd.Scaler.Scaler_Filter_Sign       = 9;
	cmd.Scaler.Scaler_Coefficent_Addr   = gx_virt_to_phys(cygnusga->joblist.zt_coefficient);
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcA, zoom->src);
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcB, zoom->dst);
	GA_CMD_SET_CHANNEL_INFO(cmd.Dst , zoom->dst);
	GET_WR_SEQENCE(zoom->dst.argb_flag, zoom->dst.bpp, wmode);
	GET_RD_SEQENCE(zoom->src.argb_flag, zoom->src.bpp, srca_rmode);
	srcb_rmode = srca_rmode;
	GA_CMD_SET_DATA_ENDIANNESS(cmd.Endianness, wmode, srca_rmode, srcb_rmode);

	return ga_submit_cmd(&cmd);
}

static GaRet ga_cmd_turn(GaTurnParam *turn)
{
	GaCmd cmd;
	unsigned int wmode, srca_rmode, srcb_rmode;
	gx_memset(&cmd, 0, sizeof(GaCmd));

	cmd.Scaler.Scaler_En    = 1;
	cmd.Rotater.Rotater_En  = 1;
	cmd.GaMode.Ga_Op_Mode   = 1;
	cmd.GaMode.Inter_Mode   = 1;
	cmd.Rotater.Rotater_Flip_Mode		= turn->flip_mode;
	cmd.Rotater.Rotater_Rotate_Mode 	= turn->rotate_mode;
	cmd.Scaler.Scaler_Filter_Sign       = 9;
	cmd.Scaler.Scaler_Coefficent_Addr   = gx_virt_to_phys(cygnusga->joblist.zt_coefficient);
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcA, turn->src);
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcB, turn->dst);
	GA_CMD_SET_CHANNEL_INFO(cmd.Dst , turn->dst);
	GET_WR_SEQENCE(turn->dst.argb_flag, turn->dst.bpp, wmode);
	GET_RD_SEQENCE(turn->src.argb_flag, turn->src.bpp, srca_rmode);
	srcb_rmode = srca_rmode;
	GA_CMD_SET_DATA_ENDIANNESS(cmd.Endianness, wmode, srca_rmode, srcb_rmode);

	return ga_submit_cmd(&cmd);
}

static GaRet ga_cmd_complet(GaCompletParam *complet)
{
	GaCmd cmd;
	unsigned int wmode, srca_rmode, srcb_rmode;
	gx_memset(&cmd, 0, sizeof(GaCmd));

	cmd.Rop.Rop_En  = 1;
	cmd.Rop.Rop_Code= GARC_COPY;
	cmd.GaMode.Ga_Op_Mode = 1;
	cmd.GaMode.Inter_Mode = 1;
	GA_CMD_SET_CHANNEL_INFO(cmd.SrcA, complet->src);
	cmd.Clut_A.Clut_Addr = gx_virt_to_phys((unsigned int)complet->src_buf_U);
	cmd.Clut_B.Clut_Addr = gx_virt_to_phys((unsigned int)complet->src_buf_V);
	GA_CMD_SET_CHANNEL_INFO(cmd.Dst , complet->dst);

	GET_WR_SEQENCE(complet->dst.argb_flag, complet->dst.bpp, wmode);
	/*patch for spp */
	if(complet->dst.data_type>=9 && complet->dst.data_type<=11)
		wmode = 0xfac688;
	srcb_rmode = srca_rmode = 0xfac688; /* 16bpp */
	GA_CMD_SET_DATA_ENDIANNESS(cmd.Endianness, wmode, srca_rmode, srcb_rmode);

	return ga_submit_cmd(&cmd);
}

GaRet cygnusga_init()
{
	if(!gx_request_mem_region(GA_REG_BASE_ADDR, sizeof(CygnusGaReg)))
		GA_PRINTF("request_mem_region failed");

	cygnusga_reg = (CygnusGaReg*)gx_ioremap(GA_REG_BASE_ADDR, sizeof(CygnusGaReg));

	gxav_clock_module_enable(MODULE_TYPE_GA, 1);
	if(gxav_firewall_access_align())
		GA_SET_AS_ACCESS_ALIGN(cygnusga_reg->rNDSW_CHECK_EN);
	else
		GA_CLR_AS_ACCESS_ALIGN(cygnusga_reg->rNDSW_CHECK_EN);
	GA_SET_CMD_LITTLE_ENDIAN(cygnusga_reg->rCMD_ENDIANNESS);
	gxav_clock_module_enable(MODULE_TYPE_GA, 0);
	ga_clk_enable = 0;

	return GA_RET_OK;
}

GaRet cygnusga_cleanup()
{
	if(cygnusga_reg) {
		gx_iounmap(cygnusga_reg);
		gx_release_mem_region(GA_REG_BASE_ADDR, sizeof(CygnusGaReg));
		cygnusga_reg = NULL;
	}
	cygnusga_close();

	return GA_RET_OK;
}

GaRet cygnusga_open(void)
{
	cygnusga = (GA*)gx_mallocz(sizeof(GA));
	IF_TRUE_RETURN_WITH_VAL(cygnusga==NULL, GA_RET_FAIL);

	/* init ga */
	ga_joblist_init();
	gx_sem_create(&cygnusga->job_lock, 1);
	gx_sem_create(&cygnusga->job_fin_sem, 0);

	cygnusga->mode = GX_GAMODE_BATCH;
	if(cygnusga->mode == GX_GAMODE_FLOW) {
		GaJob *job = ga_job_create(3000);
		ga_joblist_addjob(job);
		START_GA(cygnusga_reg, job);
	}

	return  GA_RET_OK;
}

GaRet cygnusga_close(void)
{
	if(cygnusga != NULL) {
		ga_joblist_destroy();
		gx_sem_delete(&cygnusga->job_lock);
		gx_sem_delete(&cygnusga->job_fin_sem);
		if(cygnusga_reg) {
			gx_iounmap(GA_REG_BASE_ADDR);
			gx_release_mem_region(GA_REG_BASE_ADDR, sizeof(CygnusGaReg));
			cygnusga_reg = NULL;
		}
		gx_free(cygnusga);
		cygnusga = NULL;
	}

	return  GA_RET_OK;
}

static void gx_blitobj_getcct(GxBlitObject * obj, GxClutConvertTable* cct)
{
	unsigned int offset, is_kpalette;
	GaJob *job = NULL;
	GxPalette *src_palette;
	GxColorFormat dst_format;

	job = ga_joblist_getcurrent(1);
	offset  = job->cur_cmd_num*GA_CCT_SIZE;
	cct->table_addr = (unsigned int*)((unsigned int)job->cct_start_addr + offset);

	src_palette = (GxPalette*)obj->cct;
	is_kpalette = obj->is_kcct;
	dst_format  = obj->dst_format;

	gx_color_convert(src_palette, is_kpalette, dst_format, cct);
}

static GaRet ga_dfb_blit(GxVpuProperty_DfbBlit *property, int num, GxAvRect *srcs, GxAvRect *dsts)
{
	int i;
	GaCmd cmd;
	GaDfbBlitParam dfbBlit;
	CygnusVpuSurface *sura, *surb, *surd;
	unsigned int wmode, srca_rmode, srcb_rmode;

	unsigned int flip_mode[]   = {0,1,2};   /*no , v, h*/
	unsigned int rotate_mode[] = {0,1,2,3}; /* 0, 90, 270, 180 逆时针*/

	if (!(property->valid & DESTINATION)){
		property->basic.srcb = property->basic.dst = property->basic.srca;
	}

	sura = property->basic.srca.surface;
	surb = property->basic.srcb.surface;
	surd  = property->basic.dst.surface;

	memset(&dfbBlit,0,sizeof(dfbBlit));
	/*colorkey info*/
	memcpy(&dfbBlit.basic.colorkey_info, &property->basic.colorkey_info, sizeof(GxBlitColorKeyInfo));
	/* ARGB8888 patch */
	if ((property->blittingflags & GXBLIT_SRC_COLORKEY) && (property->valid & SRC_COLORKEY)) {
		dfbBlit.basic.colorkey_info.src_colorkey_en = 1;
	}
	if ((property->blittingflags & GXBLIT_DST_COLORKEY) && (property->valid & DST_COLORKEY)) {
		dfbBlit.basic.colorkey_info.dst_colorkey_en = 1;

	}
	if (sura->color_format==GX_COLOR_FMT_ARGB8888 && dfbBlit.basic.colorkey_info.src_colorkey_en) {
		dfbBlit.basic.colorkey_info.src_colorkey =
			((dfbBlit.basic.colorkey_info.src_colorkey<< 8) | (dfbBlit.basic.colorkey_info.src_colorkey>>24));
	}
	if (surd->color_format==GX_COLOR_FMT_ARGB8888 && dfbBlit.basic.colorkey_info.dst_colorkey_en) {
		dfbBlit.basic.colorkey_info.dst_colorkey =
			((dfbBlit.basic.colorkey_info.dst_colorkey<< 8) | (dfbBlit.basic.colorkey_info.dst_colorkey>>24));
	}

	/* A8 is special */
	if (property->basic.mode != GX_ALU_VECTOR_FONT_COPY){
		if ((property->valid & BLEND_MODE) && ((property->blittingflags & GXBLIT_BLEND_ALPHACHANNEL) || (property->blittingflags & GXBLIT_BLEND_COLORALPHA))) {
			dfbBlit.basic.mode = property->basic.mode;
		} else if ((property->valid & ROP_MODE) && property->blittingflags & GXBLIT_ROP) {
			dfbBlit.basic.mode = property->rop_mode;
		} else {
			dfbBlit.basic.mode = GX_ALU_ROP_COPY;
		}
	} else {
		dfbBlit.basic.mode = property->basic.mode;
	}

	if((property->blittingflags & GXBLIT_BLEND_ALPHACHANNEL) || !(property->valid & FILLCOLOR)) {
		dfbBlit.basic.src_a.global_alpha_en	= 0;
		dfbBlit.basic.src_b.global_alpha_en	= 0;
		dfbBlit.basic.dst.global_alpha_en	= 0;
	} else if (property->blittingflags & GXBLIT_BLEND_COLORALPHA) {
		dfbBlit.basic.src_a.global_alpha_en	= 1;
		dfbBlit.basic.src_a.global_alpha = property->color.a;

		dfbBlit.basic.src_b.global_alpha_en	= 1;
		dfbBlit.basic.src_b.global_alpha = property->color.a;

		dfbBlit.basic.dst.global_alpha_en	= 1;
		dfbBlit.basic.dst.global_alpha = property->color.a;
	}
	gx_blitobj_getcct(&property->basic.srca, &dfbBlit.basic.src_a.clut_convert_table);

	/* edit command */
	gx_memset(&cmd, 0, sizeof(GaCmd));

	/*option mode   */
	cmd.GaMode.Ga_Op_Mode = 1;
	cmd.GaMode.Inter_Mode = 1;

	if (property->blittingflags & GXBLIT_ROTATE90
			|| property->blittingflags & GXBLIT_ROTATE180
			|| property->blittingflags & GXBLIT_ROTATE270
			|| property->blittingflags & GXBLIT_FLIP_HORIZONTAL
			|| property->blittingflags & GXBLIT_FLIP_VERTICAL) {
		cmd.Scaler.Scaler_En    = 1;
		cmd.Rotater.Rotater_En  = 1;
		cmd.Scaler.Scaler_Filter_Sign       = 9;
		cmd.Scaler.Scaler_Coefficent_Addr   = gx_virt_to_phys(cygnusga->joblist.zt_coefficient);

		if (property->blittingflags & GXBLIT_ROTATE90)
			cmd.Rotater.Rotater_Rotate_Mode = rotate_mode[SCREW_90];
		else if (property->blittingflags & GXBLIT_ROTATE180)
			cmd.Rotater.Rotater_Rotate_Mode = rotate_mode[SCREW_180];
		else if (property->blittingflags & GXBLIT_ROTATE270)
			cmd.Rotater.Rotater_Rotate_Mode = rotate_mode[SCREW_270];

		if (property->blittingflags & GXBLIT_FLIP_HORIZONTAL)
			cmd.Rotater.Rotater_Flip_Mode   = flip_mode[REVERSE_HORIZONTAL];
		else if (property->blittingflags & GXBLIT_FLIP_VERTICAL)
			cmd.Rotater.Rotater_Flip_Mode   = flip_mode[REVERSE_VERTICAL];
	}

	/* scale */
	if (property->ifscale) {
		cmd.Scaler.Scaler_En    = 1;
		cmd.Scaler.Scaler_Mode  = 0;
		cmd.Scaler.Scaler_Filter_Sign       = 9;
		cmd.Scaler.Scaler_Coefficent_Addr   = gx_virt_to_phys(cygnusga->joblist.zt_coefficient);
	}

	if (property->blittingflags & GXBLIT_XOR){
		cmd.Xor_En = 1;
	}

	/* modulation */
	if (property->blittingflags & GXBLIT_SRC_PREMULTCOLOR){
		GxColorFormat color_format;
		cmd.Modulator.Premultiply_En = 1;
		cmd.Modulator.Modulator_Mode_SrcA = 0x4;
		if (property->valid & FILLCOLOR){
			if (sura->color_format == GX_COLOR_FMT_ARGB8888){
				color_format = GX_COLOR_FMT_RGBA8888;
			}
			cmd.Modulator.Modulator_Src_Color = gx_color_get_value(color_format, &property->color);

		} else {
			cmd.Modulator.Modulator_Src_Color =0xffffffff;
		}
	}
	if (property->blittingflags & GXBLIT_COLORIZE) {
		GxColorFormat color_format;
		cmd.Modulator.Premultiply_En = 1;
		cmd.Modulator.Modulator_Mode_SrcB = 0x5;
		if (property->valid & FILLCOLOR){
			if (sura->color_format == GX_COLOR_FMT_ARGB8888){
				color_format = GX_COLOR_FMT_RGBA8888;
			}
			cmd.Modulator.Modulator_Src_Color = gx_color_get_value(color_format, &property->color);

		} else {
			cmd.Modulator.Modulator_Src_Color =0xffffffff;
		}
	}
	if (property->blittingflags & GXBLIT_SRC_PREMULTIPLY) {
		cmd.Modulator.Premultiply_En = 1;
		cmd.Modulator.Modulator_Mode_SrcB = 0x7;
	}

	/* demodulation */
	if (property->blittingflags & GXBLIT_DEMULTIPLY){
		cmd.Modulator.Demultiply_En = 1;
		cmd.Modulator.Demultiply_Mode = 1;
	}

	GA_CMD_SET_CLUT_CONVERT_INFO(cmd.Clut_A, dfbBlit.basic.src_a);
	GA_CMD_SET_CLUT_CONVERT_INFO(cmd.Clut_B, dfbBlit.basic.src_b);

	/* color_key */
	GA_CMD_SET_COLORKEY_INFO(cmd.ColorKey, dfbBlit.basic.colorkey_info);

	for(i = 0; i < num; i++) {
		GA_PARAM_SET_CHANNEL_INFO(dfbBlit.basic.src_a, sura, srcs[i]);
		GA_PARAM_SET_CHANNEL_INFO(dfbBlit.basic.src_b, surb, dsts[i]);
		GA_PARAM_SET_CHANNEL_INFO(dfbBlit.basic.dst  , surd, dsts[i]);

		GA_CMD_SET_CHANNEL_INFO(cmd.SrcA, dfbBlit.basic.src_a);
		GA_CMD_SET_CHANNEL_INFO(cmd.SrcB, dfbBlit.basic.src_b);
		GA_CMD_SET_CHANNEL_INFO(cmd.Dst , dfbBlit.basic.dst);

		if(dfbBlit.basic.mode <= GX_ALU_ROP_EXTEND)
			ga_cmd_blit_rop_config(&cmd, &dfbBlit.basic);
		else if((dfbBlit.basic.mode>=GX_ALU_MIX_CLEAR) && (dfbBlit.basic.mode<=GX_ALU_MIX_ADD))
			ga_cmd_blit_mix_config(&cmd, &dfbBlit.basic);
		else if(dfbBlit.basic.mode == GX_ALU_VECTOR_FONT_COPY)
			ga_cmd_blit_vfcopy_config(&cmd, &dfbBlit.basic);

		GET_RD_SEQENCE(dfbBlit.basic.src_a.argb_flag, dfbBlit.basic.src_a.bpp, srca_rmode);
		GET_RD_SEQENCE(dfbBlit.basic.src_b.argb_flag, dfbBlit.basic.src_b.bpp, srcb_rmode);
		GET_WR_SEQENCE(dfbBlit.basic.dst.argb_flag,  dfbBlit.basic.dst.bpp, wmode);
		GA_CMD_SET_DATA_ENDIANNESS(cmd.Endianness, wmode, srca_rmode, srcb_rmode);

		ga_submit_cmd(&cmd);
	}

	return GA_RET_OK;
}

/* such big function */
GaRet cygnusga_dfb_blit(GxVpuProperty_DfbBlit *property)
{
	return ga_dfb_blit(property, 1, &property->basic.srca.rect, &property->basic.dst.rect);
}

GaRet cygnusga_batch_dfb_blit(GxVpuProperty_BatchDfbBlit *property)
{
	return ga_dfb_blit((GxVpuProperty_DfbBlit *)property, property->num, property->srcs, property->dsts);
}


GaRet cygnusga_blit(GxVpuProperty_Blit *property)
{
	GaBlitParam param;
	GxBlitColorKeyInfo *sck, *tck;
	CygnusVpuSurface *sura, *surb, *dst;
	int ret = 0;

	gx_memset(&param,0,sizeof(param));
	sura = property->srca.surface;
	surb = property->srcb.surface;
	dst  = property->dst.surface;

	/*colorkey info*/
	sck = &property->colorkey_info;
	tck = &param.colorkey_info;
	memcpy(tck, sck, sizeof(GxBlitColorKeyInfo));
	/* ARGB8888 patch */
	if(sura->color_format==GX_COLOR_FMT_ARGB8888 && tck->src_colorkey_en) {
		tck->src_colorkey = ((tck->src_colorkey<< 8) | (tck->src_colorkey>>24));
	}
	if(dst->color_format==GX_COLOR_FMT_ARGB8888 && tck->dst_colorkey_en) {
		tck->dst_colorkey = ((tck->dst_colorkey<< 8) | (tck->dst_colorkey>>24));
	}

	param.mode = property->mode;
	GA_PARAM_SET_CHANNEL_INFO(param.src_a, sura, property->srca.rect);
	if(sura->color_format == GX_COLOR_FMT_YCBCR420_Y_UV) {
		param.src_a.addr = (unsigned int)sura->buffer + property->srca.rect.y * sura->width + property->srca.rect.x;
	}
	if(property->srca.alpha_en) {
		param.src_a.global_alpha	= property->srca.alpha;
		param.src_a.global_alpha_en	= 1;
	}
	if(property->srca.modulator.premultiply_en) {
		param.src_a.modulator = property->srca.modulator;
	}

	GA_PARAM_SET_CHANNEL_INFO(param.src_b, surb, property->srcb.rect);
	if(property->srcb.alpha_en) {
		param.src_b.global_alpha	= property->srcb.alpha;
		param.src_b.global_alpha_en	= 1;
	}
	if(property->srcb.modulator.premultiply_en) {
		param.src_b.modulator = property->srcb.modulator;
	}

	GA_PARAM_SET_CHANNEL_INFO(param.dst, dst, property->dst.rect);
	if(property->dst.alpha_en) {
		param.dst.global_alpha		= property->dst.alpha;
		param.dst.global_alpha_en	= 1;
	}

	gx_sem_wait(&cygnusga->job_lock);
	gx_blitobj_getcct(&property->srca, &param.src_a.clut_convert_table);
	param.time_wait_en = property->time_wait_en;

	/*alpha out*/
	param.alpha_out_info = property->alpha_out_info;

	param.src_a.color_format = sura->color_format;
	param.src_b.color_format = surb->color_format;
	param.dst.color_format = dst->color_format;

	if(sura->color_format == GX_COLOR_FMT_YCBCR420_Y_UV) {
		param.src_b.clut_convert_table.table_addr = (unsigned int *)(sura->buffer + sura->width * sura->height + property->srca.rect.y * sura->width / 2 + property->srca.rect.x );
	}

	ret = ga_cmd_blit(&param);
	gx_sem_post(&cygnusga->job_lock);

	return (ret);
}

GaRet cygnusga_fillrect(GxVpuProperty_FillRect *property)
{
	GaFillRectParam param;
	GxColorFormat color_format;
	CygnusVpuSurface *dst_surface = (CygnusVpuSurface*)property->surface;
	IF_TRUE_RETURN_WITH_VAL(dst_surface ==NULL, GA_RET_FAIL);

	gx_memset(&param,0,sizeof(param));

	color_format = dst_surface->color_format;
	if(dst_surface->color_format == GX_COLOR_FMT_ARGB8888) {
		color_format = GX_COLOR_FMT_RGBA8888;
	}

	param.color = gx_color_get_value(color_format, &property->color);
	GA_PARAM_SET_CHANNEL_INFO(param.dst, dst_surface, property->rect);
	param.colorkey_info = property->colorkey_info;
	param.blend_en = property->blend_en;
	if(param.blend_en)
		param.blend_mode = property->blend_mode;
	param.draw_XOR_en = property->draw_XOR_en;
	param.time_wait_en = property->time_wait_en;

	return ga_cmd_fillrect(&param);
}

GaRet cygnusga_zoom(GxVpuProperty_ZoomSurface *property)
{
	GaZoomParam param;
	GxAvRect src_rect, dst_rect;
	CygnusVpuSurface *src = (CygnusVpuSurface*)property->src_surface;
	CygnusVpuSurface *dst = (CygnusVpuSurface*)property->dst_surface;
	IF_TRUE_RETURN_WITH_VAL((src==NULL||dst==NULL), GA_RET_FAIL);

	gx_memset(&param,0,sizeof(param));
	src_rect = property->src_rect;
	dst_rect = property->dst_rect;

	GA_PARAM_SET_CHANNEL_INFO(param.src, src, src_rect);
	GA_PARAM_SET_CHANNEL_INFO(param.dst, dst, dst_rect);

	param.stretch_en = property->stretch_en;

	return ga_cmd_zoom(&param);
}

GaRet cygnusga_turn(GxVpuProperty_TurnSurface *property)
{
	GaTurnParam param;
	unsigned int flip_mode[]   = {0,1,2};   /*no , v, h*/
	unsigned int rotate_mode[] = {0,1,2,3}; /* 0, 90, 270, 180*/
	GxAvRect src_rect, dst_rect;
	CygnusVpuSurface *src = (CygnusVpuSurface*)property->src_surface;
	CygnusVpuSurface *dst = (CygnusVpuSurface*)property->dst_surface;
	IF_TRUE_RETURN_WITH_VAL((src==NULL||dst==NULL), GA_RET_FAIL);

	gx_memset(&param,0,sizeof(param));
	src_rect = property->src_rect;
	dst_rect = property->dst_rect;

	param.flip_mode   = flip_mode[property->reverse];
	param.rotate_mode = rotate_mode[property->screw];
	GA_PARAM_SET_CHANNEL_INFO(param.src, src, src_rect);
	GA_PARAM_SET_CHANNEL_INFO(param.dst, dst, dst_rect);

	return ga_cmd_turn(&param);
}

GaRet cygnusga_complet(GxVpuProperty_Complet *property)
{
	GaCompletParam param;
	GxAvRect src_rect, *dst_rect = &(property->dst_rect);
	CygnusVpuSurface src, *dst = (CygnusVpuSurface*)property->dst_surface;
	IF_TRUE_RETURN_WITH_VAL((dst==NULL), GA_RET_FAIL);

	gx_memset(&param,0,sizeof(param));
	/* info of src surface */
	src.width   = property->src_base_width;
	src.buffer  = property->src_buf_Y;
	src.color_format  = property->src_color;

	/* info of src_rect */
	src_rect.x = src_rect.y = 0;
	src_rect.width = property->src_width;
	src_rect.height= property->src_height;

	/* complet params */
	param.src_buf_U = property->src_buf_U;
	param.src_buf_V = property->src_buf_V;
	GA_PARAM_SET_CHANNEL_INFO(param.src, &src,  src_rect);
	GA_PARAM_SET_CHANNEL_INFO(param.dst,  dst, *dst_rect);

	return ga_cmd_complet(&param);
}

static void gx_ga_end(void)
{
	GaJob *job = ga_joblist_getcurrent(0);

	if(job) {
		if(cygnusga->mode == GX_GAMODE_BATCH) {
			ga_job_submit(job);
			ga_joblist_removejob(NULL);
			ga_job_destroy(job);
		}else {
			if(job->ready) {
				job->ready = 0;
				while((REG_GET_VAL(&cygnusga_reg->rGA_INTERRUPT) & 0x10) != 0x10);
			}
		}
	}
}

GaRet cygnusga_setmode(GxVpuProperty_SetGAMode * property)
{
	GaJob *job = NULL;

	if(property->mode != GX_GAMODE_BATCH &&
	   property->mode != GX_GAMODE_FLOW)
		return GA_RET_FAIL;

	if(cygnusga->mode == property->mode)
		return GA_RET_OK;

	gx_sem_wait(&cygnusga->job_lock);
	switch (cygnusga->mode) {
		case GX_GAMODE_BATCH:
			while(cygnusga->joblist.job_num)
				gx_ga_end();

			job = ga_job_create(3000);
			ga_joblist_addjob(job);
			START_GA(cygnusga_reg, job);
			break;
		case GX_GAMODE_FLOW:
			gx_ga_end();

			job = ga_joblist_getcurrent(0);
			ga_job_destroy(job);
			ga_joblist_removejob(NULL);
			break;
		default:
			break;
	}

	cygnusga->mode = property->mode;
	gx_sem_post(&cygnusga->job_lock);

	return GA_RET_OK;
}

GaRet cygnusga_wait(GxVpuProperty_WaitUpdate * property)
{
	if(cygnusga->mode == GX_GAMODE_BATCH)
		return GA_RET_OK;

	gx_sem_wait(&cygnusga->job_lock);
	gx_ga_end();
	gx_sem_post(&cygnusga->job_lock);

	gx_dcache_inv_range(0,0);
	return GA_RET_OK;
}

GaRet cygnusga_begin(GxVpuProperty_BeginUpdate * property)
{
	GaRet ret;
	GaJob *job = NULL;

	if(cygnusga->mode == GX_GAMODE_FLOW)
		return GA_RET_OK;

	gx_sem_wait(&cygnusga->job_lock);
	job = ga_job_create(property->max_job_num);
	ret = ga_joblist_addjob(job);
	gx_sem_post(&cygnusga->job_lock);

	return ret;
}

GaRet cygnusga_end(GxVpuProperty_EndUpdate * property)
{
	gx_sem_wait(&cygnusga->job_lock);
	gx_ga_end();
	gx_sem_post(&cygnusga->job_lock);

	return GA_RET_OK;
}

GaRet cygnusga_interrupt(int irq)
{
	GaJob *job = NULL;

	GA_CLEAN_INTERRUPT(cygnusga_reg->rGA_INTERRUPT);

	job = ga_joblist_getcurrent(0);
	if(!job) {
		gx_printf("GA error!\n");
		return GA_RET_FAIL;
	}

	job->fin_cmd_num = job->cur_cmd_num;
	gx_sem_post(&(cygnusga->job_fin_sem));

	return GA_RET_OK;
}


