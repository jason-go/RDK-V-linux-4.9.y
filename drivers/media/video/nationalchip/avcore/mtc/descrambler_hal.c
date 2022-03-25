/*****************************************************************************
*                            CONFIDENTIAL                                
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :    iemm_hal.c
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

static struct descrambler_ops* descrambler_hal_ops = NULL ;

int descrambler_hal_init(struct gxav_module_ops *ops)
{
	descrambler_hal_ops = ops->priv;

	if (descrambler_hal_ops && descrambler_hal_ops->init)
	{
		return descrambler_hal_ops->init();
	}

	return -1 ;
}

int descrambler_hal_cleanup(void)
{
	if (descrambler_hal_ops && descrambler_hal_ops->cleanup)
	{
		return descrambler_hal_ops->cleanup();
	}
	return -1 ;
}

int descrambler_hal_open(int id)
{
	if (descrambler_hal_ops && descrambler_hal_ops->open)
	{
		return descrambler_hal_ops->open(id);
	}
	return -1 ;
}

int descrambler_hal_close(int id)
{
	if (descrambler_hal_ops && descrambler_hal_ops->close)
	{
		return descrambler_hal_ops->close(id);
	}
	return -1 ;
}

int descrambler_hal_free(int id ,int desc_id)
{
	if (descrambler_hal_ops && descrambler_hal_ops->free)
	{
		return descrambler_hal_ops->free(id,desc_id);
	}
	return -1 ;
}

int decrambler_hal_link(int id ,int slot_id ,int desc_id,int valid_flag)
{
	if (descrambler_hal_ops && descrambler_hal_ops->link)
	{
		return descrambler_hal_ops->link(id,slot_id,desc_id,valid_flag);
	}
	return -1 ;
}

int descrambler_hal_user_alloc(int id ,int desc_id)
{
	if (descrambler_hal_ops && descrambler_hal_ops->user_alloc)
	{
		return descrambler_hal_ops->user_alloc(id,desc_id);
	}
	return -1 ;
}

int descrambler_hal_default_alloc(int id ,int *p_desc_id)
{
	if (descrambler_hal_ops && descrambler_hal_ops->default_alloc)
	{
		return descrambler_hal_ops->default_alloc(id,p_desc_id);
	}
	return -1 ;
}

int descrambler_hal_get_status(int id ,int *p_count,int *p_status)
{
	if (descrambler_hal_ops && descrambler_hal_ops->get_status)
	{
		return descrambler_hal_ops->get_status(id,p_count,p_status);
	}
	return -1 ;
}



