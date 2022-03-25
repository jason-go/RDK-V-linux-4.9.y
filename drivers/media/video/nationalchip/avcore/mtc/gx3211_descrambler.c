/*****************************************************************************
*                            CONFIDENTIAL                                
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :    gx3211_iemm.c
* Author    :    yuguotao
* Project   :    goxcceed api
******************************************************************************
* Purpose   :    
******************************************************************************
* Release History:
  VERSION       Date                AUTHOR          Description
*****************************************************************************/

/* Includes --------------------------------------------------------------- */


#include "descrambler_hal.h"
#include "descrambler_module.h"
#include "gx3211_reg_descrambler.h"

#define  DRV_AS_DESCRAMBLER_NUM     (DESC_NUM_DESCRAMBLER)

extern void gx3211_demux_func_lock(void);
extern void gx3211_demux_func_unlock(void);

static struct DESC_Demux_Regs_s *gx3211_desc_demux_reg = NULL;

static int s_as_descrambler_mask = 0 ;

static int desc_find_first_zero(void)
{
	int temp  = s_as_descrambler_mask;
	int i;

	for (i = 0; i < DRV_AS_DESCRAMBLER_NUM; i++)
	{
		if (((temp >> i)&0x1) == 0)
			return i;
	}
	return -1;
}

static int desc_is_zero_by_desc_id(int desc_id)
{
	int temp  = s_as_descrambler_mask;

	if (((temp >> desc_id)&0x1) == 0)
	{
		return desc_id ;
	}
	else
	{
		return -1 ;
	}
}

static void desc_set_mask_bit(int bit)    { s_as_descrambler_mask |= (0x1ULL << bit) ; }
static void desc_clr_mask_bit(int bit)    { s_as_descrambler_mask &= ~(0x1ULL << bit); }


//////////////////////////////////////////////
/* interface */

static int s_gx3211_descrambler_mem_region = 0 ;

static int gx3211_descrambler_init(void)
{
	if ( gx3211_desc_demux_reg == NULL )
	{
		if(!gx_request_mem_region(DESC_DEMUX_REG_ADDR, sizeof(struct DESC_Demux_Regs_s)))
		{
			s_gx3211_descrambler_mem_region = 0 ;
		}
		else
		{
			s_gx3211_descrambler_mem_region = 1 ;
		}

		gx3211_desc_demux_reg = gx_ioremap(DESC_DEMUX_REG_ADDR, sizeof(struct DESC_Demux_Regs_s));
		if (!gx3211_desc_demux_reg)
		{
			return -1;
		}
	}

	return 0 ;
}

static int gx3211_descrambler_cleanup(void)
{

	gx_iounmap(gx3211_desc_demux_reg);

	if ( s_gx3211_descrambler_mem_region )
	{
		s_gx3211_descrambler_mem_region = 0 ;

		gx_release_mem_region(DESC_DEMUX_REG_ADDR, sizeof(struct DESC_Demux_Regs_s));

	}

	gx3211_desc_demux_reg = NULL;

	s_as_descrambler_mask = 0 ;

	return 0 ;
}

static int gx3211_descrambler_open(int id)
{
	return 0 ;
}

static int gx3211_descrambler_close(int id)
{
	return 0 ;
}

static int gx3211_descrambler_default_alloc(int id ,int *p_desc_id)
{
	int i ;

	if (p_desc_id == NULL )
		return -1 ;

	i = desc_find_first_zero();
	if ( i<0 )
	{
		return -1 ;
	}

	gx3211_demux_func_lock();
	desc_set_mask_bit(i);
	gx3211_demux_func_unlock();

	*p_desc_id = i ;

	return 0 ;
}

static int gx3211_descrambler_user_alloc(int id ,int desc_id)
{
	int i;

	if ( (desc_id >= DRV_AS_DESCRAMBLER_NUM ) || (desc_id<0) )
	{
		return -1 ;
	}

	i = desc_is_zero_by_desc_id(desc_id);
	if (i<0)
	{
		return -1 ;
	}

	gx3211_demux_func_lock();
	desc_set_mask_bit(i);
	gx3211_demux_func_unlock();

	return 0 ;
}

static int gx3211_descrambler_free(int id ,int desc_id)
{
	if ( (desc_id >= DRV_AS_DESCRAMBLER_NUM ) || (desc_id<0) )
	{
		return -1 ;
	}

	gx3211_demux_func_lock();
	desc_clr_mask_bit(desc_id);
	gx3211_demux_func_unlock();

	return 0 ;
}

static int gx3211_decrambler_link(int id ,int slot_id ,int desc_id,int valid_flag)
{
	if ( (desc_id >= DRV_AS_DESCRAMBLER_NUM ) || (desc_id<0) )
	{
		return -1 ;
	}

	if (slot_id >= 64 || slot_id < 0)
	{
		return -1 ;
	}

	gx3211_demux_func_lock();
	REG_SET_BITS(&(gx3211_desc_demux_reg->wen_mask), DESC_DEMUX_PID_CFG_KEY_ID);
	REG_SET_VAL(&(gx3211_desc_demux_reg->pid_cfg_sel[slot_id].pid_cfg), desc_id <<DESC_BIT_DEMUX_PID_CFG_KEY_ID);
	REG_CLR_BITS(&(gx3211_desc_demux_reg->wen_mask), DESC_DEMUX_PID_CFG_KEY_ID);

	if (valid_flag == 1)
	{
		REG_SET_BIT(&(gx3211_desc_demux_reg->wen_mask), DESC_BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		REG_SET_BIT(&(gx3211_desc_demux_reg->pid_cfg_sel[slot_id].pid_cfg), DESC_BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		REG_CLR_BIT(&(gx3211_desc_demux_reg->wen_mask), DESC_BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
	}
	else if (valid_flag == 0)
	{
		REG_SET_BIT(&(gx3211_desc_demux_reg->wen_mask), DESC_BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		REG_CLR_BIT(&(gx3211_desc_demux_reg->pid_cfg_sel[slot_id].pid_cfg), DESC_BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
		REG_CLR_BIT(&(gx3211_desc_demux_reg->wen_mask), DESC_BIT_DEMUX_PID_CFG_ODD_KEY_VALID);
	}
	gx3211_demux_func_unlock();

	return 0 ;
}

static int gx3211_descrambler_get_status(int id ,int *p_count, int *p_status)
{
	if ( p_count == NULL || p_status == NULL )
		return -1 ;

	*p_count  = DRV_AS_DESCRAMBLER_NUM ;

	*p_status = s_as_descrambler_mask ;

	return 0 ;

}

static struct descrambler_ops gx3211_descrambler_ops = {
	.init          = gx3211_descrambler_init,
	.cleanup       = gx3211_descrambler_cleanup,
	.open          = gx3211_descrambler_open,
	.close         = gx3211_descrambler_close,
	.free          = gx3211_descrambler_free,
	.link          = gx3211_decrambler_link,
	.user_alloc    = gx3211_descrambler_user_alloc,
	.default_alloc = gx3211_descrambler_default_alloc,
	.get_status    = gx3211_descrambler_get_status,
};

struct gxav_module_ops gx3211_descrambler_module = {
	.module_type  = GXAV_MOD_DESCRAMBLER,
	.count        = 1,
	.irqs         = {-1},
	.irq_flags    = {-1},
	.event_mask   = 0xffffffff,
	.open         = gxav_descrambler_open,
	.close        = gxav_descrambler_close,
	.init         = gxav_descrambler_init,
	.cleanup      = gxav_descrambler_cleanup,
	.set_property = gxav_descrambler_set_property,
	.get_property = gxav_descrambler_get_property,
	.priv         = &gx3211_descrambler_ops
};
