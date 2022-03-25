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
#include "gxav_mtc_propertytypes.h"
#include "mtc_hal.h"


int gxav_mtc_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return mtc_hal_init(inode->interface);
}

int gxav_mtc_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	//gx_printf(" gx3201_icam_cleanup 0x%x , 0x%x \n",dev,inode);
	return mtc_hal_cleanup(); ;
}

int gxav_mtc_open(struct gxav_module *module)
{
	//gx_printf(" gx3201_icam_open 0x%x \n",module);
	return mtc_hal_open(module->sub) ;
}

int gxav_mtc_close(struct gxav_module *module)
{
	//gx_printf(" gx3201_icam_close 0x%x \n",module);
	return mtc_hal_close(module->sub) ;
}

int gxav_mtc_set_property(struct gxav_module *module, int property_id, void *property, int size)
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
		case GxMTCPropertyID_M2M_UpdateKey:
			{
				GxMTCProperty_M2M_UpdateKey *p_M2M_UpdateKey =(GxMTCProperty_M2M_UpdateKey *)property;

				if ( size != sizeof(GxMTCProperty_M2M_UpdateKey))
					return -1 ;

				return mtc_hal_updatekey(module->sub,p_M2M_UpdateKey);
			}
			break;

		case GxMTCPropertyID_M2M_SetParamsAndExecute:
			{
				GxMTCProperty_M2M_SetParamsAndExecute *p_M2M_SetParamsAndExecute =\
					(GxMTCProperty_M2M_SetParamsAndExecute *) property;

				if ( size != sizeof(GxMTCProperty_M2M_SetParamsAndExecute))
					return -1 ;

				return mtc_hal_set_params_execute(module->sub,p_M2M_SetParamsAndExecute);
			}
			break;

		/*
			------------------------------------------------------------------
		*/

		case  GxMTCPropertyID_Config:
			{
				GxMTCProperty_Config *p_config = (GxMTCProperty_Config *) property;
				if ( size != sizeof(GxMTCProperty_Config))
					return -1;

				return mtc_hal_set_config(module->sub,p_config);
			}
			break;

		case GxMTCPropertyID_Run:
			{
				GxMTCProperty_Run *p_Run = (GxMTCProperty_Run *) property;
				if ( size != sizeof(GxMTCProperty_Run))
					return -1;

				return mtc_hal_runing(module->sub,p_Run);
			}
			break;

		case GxMTCPropertyID_Reset:
			{
			}
			break;

		default:
			return -2 ;
	}

	return 0 ;
}

int gxav_mtc_get_property(struct gxav_module *module, int property_id, void *property, int size)
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
		case GxMTCPropertyID_M2M_Event:
			{
				GxMTCProperty_M2M_Event *p_M2M_Event = (GxMTCProperty_M2M_Event *) property;

				if ( size != sizeof(GxMTCProperty_M2M_Event))
					return -1 ;

				return mtc_get_event(module->sub,p_M2M_Event);
			}
			break;

		default:
			return -2 ;
	}

	return 0 ;

}








