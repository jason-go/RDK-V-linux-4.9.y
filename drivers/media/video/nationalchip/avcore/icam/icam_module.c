/*****************************************************************************
*                            CONFIDENTIAL                                
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :    icam_module.c
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
#include "icam_hal.h"
#include "icam_module.h"

int gxav_icam_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return icam_hal_init(inode->interface);
}

int gxav_icam_cleanup(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	//gx_printf(" gx3201_icam_cleanup 0x%x , 0x%x \n",dev,inode);
	return icam_hal_cleanup();
}

int gxav_icam_open(struct gxav_module *module)
{
	//gx_printf(" gx3201_icam_open 0x%x \n",module);
	return icam_hal_open(module->sub) ;
}

int gxav_icam_close(struct gxav_module *module)
{
	//gx_printf(" gx3201_icam_close 0x%x \n",module);
	return icam_hal_close(module->sub) ;
}

int gxav_icam_set_property(struct gxav_module *module, int property_id, void *property, int size)
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
		case GxICAMPropertyID_CardClockDivisor:
			{
				GxIcamProperty_ClockDivisor *p_clockdivisor = (GxIcamProperty_ClockDivisor *) property;

				if ( size != sizeof(GxIcamProperty_ClockDivisor))
					return -1 ;

			//	gx_printf("icam card clock divisor  %d \n",p_clockdivisor->m_clockdivisor);

				return icam_hal_set_cardclockdivisor(module->sub,p_clockdivisor->m_clockdivisor);
			}
			break;

		case GxICAMPropertyID_VccLevel:
			{
				GxIcamProperty_VccLevel *p_vcclevel = (GxIcamProperty_VccLevel *) property;

				if ( size != sizeof(GxIcamProperty_VccLevel))
					return -1 ;

			//	gx_printf("icam card vcc level  %d \n",p_vcclevel->m_vcclevel);

				return icam_hal_set_vcclevel(module->sub,p_vcclevel->m_vcclevel);

			}
			break;

		case GxICAMPropertyID_Convention:
			{
				GxIcamProperty_Convention *p_convention = (GxIcamProperty_Convention *) property;

				if ( size != sizeof(GxIcamProperty_Convention))
					return -1 ;

			//	gx_printf("icam card convention  %d \n",p_convention->m_convention);

				return icam_hal_set_uartconvdirection(module->sub,p_convention->m_convention);
			}
			break;

		case GxICAMPropertyID_UartBaudRate:
			{
				GxIcamProperty_UartBaudRate *p_baudrate = (GxIcamProperty_UartBaudRate *) property;

				if ( size != sizeof(GxIcamProperty_UartBaudRate))
					return -1 ;

			//	gx_printf("icam card uart baud rate  %d \n",p_baudrate->m_uartbaudrate);

				return icam_hal_set_uartbaudrate(module->sub,p_baudrate);
			}
			break;

		case GxICAMPropertyID_VccSwitch:
			{
				GxIcamProperty_VccSwitch *p_vccswitch = (GxIcamProperty_VccSwitch *) property;

				if ( size != sizeof(GxIcamProperty_VccSwitch))
					return -1 ;

			//	gx_printf("icam card vcc switch  %d \n",p_vccswitch->m_vccswitch);

				return icam_hal_set_vccswitch(module->sub,p_vccswitch->m_vccswitch);
			}
			break;

		case GxICAMPropertyID_GuardTime:
			{
				GxIcamProperty_GuardTime *p_guardtime = (GxIcamProperty_GuardTime *) property;

				if ( size != sizeof(GxIcamProperty_GuardTime))
					return -1 ;

			//	gx_printf("icam card guard time  %d \n",p_guardtime->m_guardtime);

				return icam_hal_set_uartguardtime(module->sub,p_guardtime->m_guardtime) ;
			}
			break;

		case GxICAMPropertyID_ResetCard:
			{
				GxIcamProperty_ResetCard *p_reset = (GxIcamProperty_ResetCard *) property;

				if ( size != sizeof(GxIcamProperty_ResetCard))
					return -1 ;

			//	gx_printf("icam card reset card  %d \n",p_reset->m_resetswitch);

				return icam_hal_set_resetcard(module->sub,p_reset->m_resetswitch);
			}
			break;

		case GxICAMPropertyID_UartCommand:
			{
				GxIcamProperty_UartCommand *p_command = (GxIcamProperty_UartCommand *) property;

				if ( size != sizeof(GxIcamProperty_UartCommand))
					return -1 ;

			//	gx_printf("icam card set command pin mask  %d \n",p_command->m_pinmask);

				return icam_hal_set_uartcommand(module->sub,&(p_command->m_pinmask));
			}
			break;

		case GxICAMPropertyID_SendAndReceive:
			{
				GxIcamProperty_SendAndReceive *p_sr = (GxIcamProperty_SendAndReceive *) property;

				if ( size != sizeof(GxIcamProperty_SendAndReceive))
					return -1 ;


			//	gx_printf(" send and rece  flow control %d  \n",p_sr->m_flow_control );

				return icam_hal_send_receive(module->sub,p_sr);
			}
			break;

		case GxICAMPropertyID_Receive:
			{
				GxIcamProperty_Receive *p_rece = (GxIcamProperty_Receive *) property;

				if ( size != sizeof(GxIcamProperty_Receive))
					return -1 ;

			//	gx_printf(" rece  flow control %d  \n",p_rece->m_flow_control );

				return icam_hal_receive(module->sub,p_rece);

			}
			break;

		case GxICAMPropertyID_Abort:
			{
			}
			break;

		case GxICAMPropertyID_InsertRemove:
			{
				GxIcamProperty_InsertRemove *p_InRe = (GxIcamProperty_InsertRemove *) property;

				if ( size != sizeof(GxIcamProperty_InsertRemove))
					return -1 ;

			//	gx_printf(" insert or remove %d  \n",p_InRe->m_insert_or_remove);

				return icam_hal_card_insert_remove(module->sub,p_InRe->m_insert_or_remove);
			}
			break;

		case GxICAMPropertyID_ControlWord:
			{
				GxIcamProperty_ControlWord *p_ControlWord = (GxIcamProperty_ControlWord *) property;

				if ( size != sizeof(GxIcamProperty_ControlWord))
					return -1 ;

			//	gx_printf(" set control word   \n",p_ControlWord->m_command_mode);

				return icam_hal_set_controlword(module->sub,p_ControlWord);
			}
			break;

		case GxICAMPropertyID_ConfigInfo:
			{
				GxIcamProperty_ConfigInfo *p_ConfigInfo = (GxIcamProperty_ConfigInfo *) property;

				if ( size != sizeof(GxIcamProperty_ConfigInfo))
					return -1 ;

				return icam_hal_set_config(module->sub,p_ConfigInfo);
			}
			break;

		default:
			return -2 ;
	}

	return 0 ;
}

int gxav_icam_get_property(struct gxav_module *module, int property_id, void *property, int size)
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
		case GxICAMPropertyID_UartStatus:
			{
				GxIcamProperty_UartStatus *p_status = (GxIcamProperty_UartStatus *) property;

				if ( size != sizeof(GxIcamProperty_UartStatus))
					return -1 ;

				return icam_hal_get_uartstatue(module->sub,&(p_status->m_uartstatus));
			}
			break;

		case GxICAMPropertyID_UartCommand:
			{
				GxIcamProperty_UartCommand *p_command = (GxIcamProperty_UartCommand *) property;

				if ( size != sizeof(GxIcamProperty_UartCommand))
					return -1 ;

		//		gx_printf("icam card get command pin mask  %d \n",p_command->m_pinmask);

				memset(p_command,0,sizeof(GxIcamProperty_UartCommand));

				return icam_hal_get_uartcommond(module->sub,&(p_command->m_pinmask));
			}
			break;

		case GxICAMPropertyID_Abort:
			{
				GxIcamProperty_InterruptEvent *p_event = (GxIcamProperty_InterruptEvent *) property;

				if ( size != sizeof(GxIcamProperty_InterruptEvent))
					return -1 ;

				return icam_hal_get_card_event(module->sub,&(p_event->m_interrupt_event));

			}
			break;

		case GxICAMPropertyID_Event:
			{
				GxIcamProperty_InterruptEvent *p_event = (GxIcamProperty_InterruptEvent *) property;

				if ( size != sizeof(GxIcamProperty_InterruptEvent))
					return -1 ;

				return icam_hal_get_event(module->sub,&(p_event->m_interrupt_event));
			}
			break;

		case GxICAMPropertyID_ChipProperties:
			{
				GxIcamProperty_ChipProperties *p_Properties =(GxIcamProperty_ChipProperties *) property;

				if ( size != sizeof(GxIcamProperty_ChipProperties))
					return -1 ;

				return icam_hal_get_chipproperties(module->sub,p_Properties);
			}
			break;

		case GxICAMPropertyID_ResponseToChallenge:
			{
				GxIcamProperty_ResponseToChallenge *p_ResponseToChallenge =\
					(GxIcamProperty_ResponseToChallenge *) property;

				if ( size != sizeof(GxIcamProperty_ResponseToChallenge))
					return -1 ;

				return icam_hal_get_responsetochallenge(module->sub,p_ResponseToChallenge);
			}
			break;

		case GxICAMPropertyID_EncryptData:
			{
				GxIcamProperty_EncryptData *p_EncryptData = (GxIcamProperty_EncryptData *) property;

				if ( size != sizeof(GxIcamProperty_EncryptData))
					return -1 ;

				return icam_hal_get_encryptdata(module->sub,p_EncryptData);
			}
			break;

		case GxICAMPropertyID_ReadRegister:
			{
				GxIcamProperty_Read_Write_Register *p_RWRegister =(GxIcamProperty_Read_Write_Register *)property;

				if ( size != sizeof(GxIcamProperty_Read_Write_Register))
					return -1 ;

				return icam_hal_get_read_register(module->sub,p_RWRegister->m_offset,
										&(p_RWRegister->m_read_write_data));

			}
			break;

		case GxICAMPropertyID_WriteRegister:
			{
				GxIcamProperty_Read_Write_Register *p_RWRegister =(GxIcamProperty_Read_Write_Register *) property;

				if ( size != sizeof(GxIcamProperty_Read_Write_Register))
					return -1 ;

				return icam_hal_get_write_register(module->sub,p_RWRegister->m_offset,
								p_RWRegister->m_read_write_data);
			}
			break;

		case GxICAMPropertyID_ConfigInfo:
			{
				GxIcamProperty_ConfigInfo *p_ConfigInfo = (GxIcamProperty_ConfigInfo *) property;

				if ( size != sizeof(GxIcamProperty_ConfigInfo))
					return -1 ;

				return icam_hal_get_config(module->sub,p_ConfigInfo);
			}
			break;

		case GxICAMPropertyID_ReadOTP:
			{
				GxIcamProperty_Read_Write_OTP *p_otp = (GxIcamProperty_Read_Write_OTP *) property;

				if ( size != sizeof(GxIcamProperty_Read_Write_OTP))
					return -1 ;

				return icam_hal_get_read_otp(module->sub,p_otp->m_addr,p_otp->m_num,p_otp->m_data);
			}
			break;

		case GxICAMPropertyID_WriteOTP:
			{
				GxIcamProperty_Read_Write_OTP *p_otp = (GxIcamProperty_Read_Write_OTP *) property;

				if ( size != sizeof(GxIcamProperty_Read_Write_OTP))
					return -1 ;

				return icam_hal_get_write_otp(module->sub,p_otp->m_addr,p_otp->m_num,p_otp->m_data);
			}
			break;

		default:
			return -2 ;
	}

	return 0 ;

}

int gxav_icam_callback(unsigned int event, void *priv)
{
	struct gxav_module_inode *inode = (struct gxav_module_inode *)priv;

//	gx_printf(">>drive icam callback set event 0x%x ,0x%x \n",event,(int)priv);

	return gxav_module_inode_set_event(inode, event);
}

struct gxav_module_inode *gxav_icam_interrupt(struct gxav_module_inode *inode, int irq)
{
	icam_hal_interrupt(0,gxav_icam_callback,inode);

	return inode ;
}








