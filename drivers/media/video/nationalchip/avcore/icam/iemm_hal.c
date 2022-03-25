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

#include "iemm_hal.h"

static struct iemm_ops* iemm_hal_ops = NULL ;

int iemm_hal_init(struct gxav_module_ops *ops)
{
	iemm_hal_ops = ops->priv;

	if (iemm_hal_ops && iemm_hal_ops->init)
	{
		return iemm_hal_ops->init();
	}

	return -1 ;
}

int iemm_hal_cleanup(void)
{
	if (iemm_hal_ops && iemm_hal_ops->cleanup)
	{
		return iemm_hal_ops->cleanup();
	}
	return -1 ;
}

int iemm_hal_open(int id)
{
	if (iemm_hal_ops && iemm_hal_ops->open)
	{
		return iemm_hal_ops->open(id);
	}
	return -1 ;
}

int iemm_hal_close(int id)
{
	if (iemm_hal_ops && iemm_hal_ops->close)
	{
		return iemm_hal_ops->close(id);
	}
	return -1 ;
}

int iemm_hal_request(int id,GxIemmProperty_Request *p_Request)
{
	if (iemm_hal_ops && iemm_hal_ops->request)
	{
		return iemm_hal_ops->request(id,p_Request);
	}
	return -1 ;
}

int iemm_hal_stop(int id,GxIemmProperty_Stop *p_Stop)
{
	if (iemm_hal_ops && iemm_hal_ops->stop)
	{
		return iemm_hal_ops->stop(id,p_Stop);
	}
	return -1 ;
}

int iemm_hal_run(int id,GxIemmProperty_Run *p_Run)
{
	if (iemm_hal_ops && iemm_hal_ops->run)
	{
		return iemm_hal_ops->run(id,p_Run);
	}
	return -1 ;
}

int iemm_hal_update_read_index(int id,GxIemmProperty_UpdateReadIndex *p_UpdateReadIndex)
{
	if (iemm_hal_ops && iemm_hal_ops->update_read_index)
	{
		return iemm_hal_ops->update_read_index(id,p_UpdateReadIndex);
	}
	return -1 ;
}

int iemm_hal_get_event(int id ,GxIemmProperty_InterruptEvent *p_InterruptEvent)
{
	if (iemm_hal_ops && iemm_hal_ops->get_event)
	{
		return iemm_hal_ops->get_event(id,p_InterruptEvent);
	}
	return -1 ;
}

int iemm_hal_read_data_buff(int id, GxIemmProperty_ReadDataBuff *p_ReadDataBuff)
{
	if (iemm_hal_ops && iemm_hal_ops->read_data_buff)
	{
		return iemm_hal_ops->read_data_buff(id,p_ReadDataBuff);
	}
	return -1 ;

}

int iemm_hal_read_data_list(int id, GxIemmProperty_ReadDataList *p_ReadDataList)
{
	if (iemm_hal_ops && iemm_hal_ops->read_data_list)
	{
		return iemm_hal_ops->read_data_list(id,p_ReadDataList);
	}
	return -1 ;

}

int iemm_hal_interrupt(int id,int (*callback)(unsigned int event,void *priv),void *priv)
{
	if (iemm_hal_ops && iemm_hal_ops->interrupt)
	{
		return iemm_hal_ops->interrupt(id,callback,priv);
	}
	return -1 ;
}
