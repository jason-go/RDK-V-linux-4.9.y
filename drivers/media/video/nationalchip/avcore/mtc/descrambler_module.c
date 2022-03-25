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
#include "gxav_descrambler_propertytypes.h"
#include "descrambler_hal.h"


int gxav_descrambler_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return descrambler_hal_init(inode->interface);
}

int gxav_descrambler_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	//gx_printf(" gx3201_icam_cleanup 0x%x , 0x%x \n",dev,inode);
	return descrambler_hal_cleanup(); ;
}

int gxav_descrambler_open(struct gxav_module *module)
{
	//gx_printf(" gx3201_icam_open 0x%x \n",module);
	return descrambler_hal_open(module->sub) ; ;
}

int gxav_descrambler_close(struct gxav_module *module)
{
	//gx_printf(" gx3201_icam_close 0x%x \n",module);
	return descrambler_hal_close(module->sub) ;
}

int gxav_descrambler_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0 ;
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
		case GxDescramblerPropertyID_Free:
			{
				GxDescrambler_Desc *p_config = (GxDescrambler_Desc *) property;
				if ( size != sizeof(GxDescrambler_Desc))
				{
					ret = -1 ;
				}
				else
				{
					ret = descrambler_hal_free(module->sub,p_config->m_desc_id);
				}
			}
			break;

		case GxDescramblerPropertyID_Link:
			{
				GxDescrambler_Link *p_config = (GxDescrambler_Link *) property;
				if ( size != sizeof(GxDescrambler_Link))
				{
					ret = -1 ;
				}
				else
				{
					ret = decrambler_hal_link(module->sub,p_config->m_slot_id,p_config->m_desc_id,p_config->m_is_link);
				}
			}
			break;

		default:
			ret = -2  ;
			break;
	}

	return ret ;
}

int gxav_descrambler_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	int ret = 0 ;
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
		case GxDescramblerPropertyID_Alloc:
			{
				GxDescrambler_Desc *p_config = (GxDescrambler_Desc *) property;
				if ( size != sizeof(GxDescrambler_Desc))
				{
					ret = -1 ;
				}
				else
				{
					if ( DESC_ALLOC_BY_SPECAIL == p_config->m_flag)
						ret = descrambler_hal_user_alloc(module->sub,p_config->m_desc_id);
					else
						ret = descrambler_hal_default_alloc(module->sub,&(p_config->m_desc_id));
				}
			}
			break;

		case GxDescramblerPropertyID_Status:
			{
				GxDescrambler_Status *p_config = (GxDescrambler_Status *) property;
				if ( size != sizeof(GxDescrambler_Status))
				{
					ret = -1 ;
				}
				else
				{
					descrambler_hal_get_status(module->sub,&(p_config->m_desc_count),&(p_config->m_desc_status));
				}
			}
			break;

		default:
			ret = -2 ;
			break ;
	}

	return ret ;

}








