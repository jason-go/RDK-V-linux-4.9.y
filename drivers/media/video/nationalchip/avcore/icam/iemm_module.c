/*****************************************************************************
*                            CONFIDENTIAL                                
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :    iemm_module.c
* Author    :    yuguotao
* Project   :    goxcceed api
******************************************************************************
* Purpose   :    
******************************************************************************
* Release History:
  VERSION       Date                AUTHOR          Description
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

//#include <string.h>

#include "gxav_module_property.h"
#include "gxav_icam_propertytypes.h"
#include "iemm_hal.h"
#include "iemm_module.h"

static struct gxav_module_inode * s_iemm_inode = NULL ; // for interrupt event

int gxav_iemm_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return iemm_hal_init(inode->interface);
}

int gxav_iemm_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	//gx_printf(" gx3201_icam_cleanup 0x%x , 0x%x \n",dev,inode);
	return iemm_hal_cleanup(); ;
}

int gxav_iemm_open(struct gxav_module *module)
{
	int ret = 0;

	ret = iemm_hal_open(module->sub) ;

	s_iemm_inode = module->inode ;
/*
	if (s_iemm_inode != NULL)
		gx_printf(" gx3201_icam_open inode 0x%x \n",(int)s_iemm_inode);

	gx_printf(" gx3201_icam_open 0x%x \n",(int)module);
*/
	return ret ;
}

int gxav_iemm_close(struct gxav_module *module)
{
	//gx_printf(" gx3201_icam_close 0x%x \n",module);
	return iemm_hal_close(module->sub) ; ;
}

int gxav_iemm_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count))
	{
		return -3;
	}

	if (NULL == property)
	{
		return -1;
	}

//	gx_printf(" gx3201_icam_set_property 0x%x, %d ,%d  \n",module,property_id,size);

	switch(property_id)
	{
		case GxIEMMPropertyID_Request:
			{
				GxIemmProperty_Request *p_Request = (GxIemmProperty_Request *) property;

				if ( size != sizeof(GxIemmProperty_Request))
					return -1 ;

			//	gx_printf("iemm  request \n");

				return iemm_hal_request(module->sub,p_Request);
			}
			break;

		case GxIEMMPropertyID_Stop:
			{
				GxIemmProperty_Stop *p_Stop = (GxIemmProperty_Stop *) property;

				if ( size != sizeof(GxIemmProperty_Stop))
					return -1 ;

			//	gx_printf("iemm stop  %d \n",p_Stop->m_reserver);

				return iemm_hal_stop(module->sub,p_Stop);
			}
			break;

		case GxIEMMPropertyID_Run:
			{
				GxIemmProperty_Run *p_Run = (GxIemmProperty_Run *) property;

				if ( size != sizeof(GxIemmProperty_Run))
					return -1 ;

			//	gx_printf("iemm run  %d \n",p_Run->m_run_en);

				return iemm_hal_run(module->sub,p_Run);
			}
			break;

		case GxIEMMPropertyID_UpdateReadIndex:
			{
				GxIemmProperty_UpdateReadIndex *p_UpdateReadIndex = (GxIemmProperty_UpdateReadIndex *)property ;

				if ( size != sizeof(GxIemmProperty_UpdateReadIndex))
					return -1 ;

			//	gx_printf("iemm update \n");

				return iemm_hal_update_read_index(module->sub,p_UpdateReadIndex);
			}
			break;

		default:
			return -2 ;
	}

	return 0 ;
}

int gxav_iemm_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	if ((NULL == module) || (module->sub < 0) || (module->sub > module->inode->count))
	{
		return -3;
	}

	if (NULL == property)
	{
		return -1;
	}

//	gx_printf(" gx3201_icam_get_property  0x%x, %d ,%d  \n",module,property_id,size);

	switch(property_id)
	{
		case GxIEMMPropertyID_InterruptEvent:
			{
				GxIemmProperty_InterruptEvent *p_InterruptEvent = (GxIemmProperty_InterruptEvent *) property;

				if ( size != sizeof(GxIemmProperty_InterruptEvent))
					return -1 ;

			//	gx_printf("get interrupt event  \n");

				return iemm_hal_get_event(module->sub,p_InterruptEvent);
			}
			break;

		case GxIEMMPropertyID_ReadDataBuff:
			{
				GxIemmProperty_ReadDataBuff *p_ReadDataBuff = (GxIemmProperty_ReadDataBuff *)property ;
				if ( size != sizeof(GxIemmProperty_ReadDataBuff))
				{
					gx_printf("read buff para error  \n");
					return -1 ;
				}

				return iemm_hal_read_data_buff(module->sub,p_ReadDataBuff);
			}
			break;

		case GxIEMMPropertyID_ReadDataList:
			{
				GxIemmProperty_ReadDataList *p_ReadDataList = (GxIemmProperty_ReadDataList *)property ;
				{
					int size1 ;
					size1 = sizeof(GxIemmProperty_ReadDataList);
			//		if (  size != sizeof(GxIemmProperty_ReadDataList));
					if ( size != size1)
					{
						gx_printf("Error ---- read list para error %d,%d \n",size,sizeof(GxIemmProperty_ReadDataList));
						return -1 ;
					}
				}

				return iemm_hal_read_data_list(module->sub,p_ReadDataList);
			}
			break;

		default:
			return -2 ;
	}

	return 0 ;

}

int gxav_iemm_callback(unsigned int event, void *priv)
{
	struct gxav_module_inode *inode = (struct gxav_module_inode *)priv;

//	gx_printf(">>drive iemm callback set event 0x%x ,0x%x \n",event,(int)priv);

	return gxav_module_inode_set_event(inode, event);
}

int gxav_iemm_interrupt(void)
{
	return iemm_hal_interrupt(0,gxav_iemm_callback,s_iemm_inode);
}








