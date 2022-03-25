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

#include "mtc_hal.h"

static struct mtc_ops* mtc_hal_ops = NULL ;

int mtc_hal_init(struct gxav_module_ops *ops)
{
	mtc_hal_ops = ops->priv;

	if (mtc_hal_ops && mtc_hal_ops->init)
	{
		return mtc_hal_ops->init();
	}

	return -1 ;
}

int mtc_hal_cleanup(void)
{
	if (mtc_hal_ops && mtc_hal_ops->cleanup)
	{
		return mtc_hal_ops->cleanup();
	}
	return -1 ;
}

int mtc_hal_open(int id)
{
	if (mtc_hal_ops && mtc_hal_ops->open)
	{
		return mtc_hal_ops->open(id);
	}
	return -1 ;
}

int mtc_hal_close(int id)
{
	if (mtc_hal_ops && mtc_hal_ops->close)
	{
		return mtc_hal_ops->close(id);
	}
	return -1 ;
}

int mtc_hal_updatekey(int id ,GxMTCProperty_M2M_UpdateKey *p_M2M_UpdateKey)
{
	if (mtc_hal_ops && mtc_hal_ops->updatekey)
	{
		return mtc_hal_ops->updatekey(id,p_M2M_UpdateKey);
	}
	return -1 ;
}

int mtc_hal_set_params_execute(int id ,GxMTCProperty_M2M_SetParamsAndExecute *p_M2M_SetParamsAndExecute)
{
	if (mtc_hal_ops && mtc_hal_ops->set_params_execute)
	{
		return mtc_hal_ops->set_params_execute(id,p_M2M_SetParamsAndExecute);
	}
	return -1 ;
}

int mtc_get_event(int id ,GxMTCProperty_M2M_Event *p_M2M_Event)
{
	if (mtc_hal_ops && mtc_hal_ops->get_event)
	{
		return mtc_hal_ops->get_event(id,p_M2M_Event);
	}
	return -1 ;
}

/*
------------------------------------------------------------------------------
*/

int mtc_hal_set_config(int id ,GxMTCProperty_Config *p_config)
{
	if (mtc_hal_ops && mtc_hal_ops->set_config)
	{
		return mtc_hal_ops->set_config(id,p_config);
	}
	return -1 ;
}

int mtc_hal_runing(int id ,GxMTCProperty_Run *p_Run)
{
	if (mtc_hal_ops && mtc_hal_ops->runing)
	{
		return mtc_hal_ops->runing(id,p_Run);
	}
	return -1 ;
}

