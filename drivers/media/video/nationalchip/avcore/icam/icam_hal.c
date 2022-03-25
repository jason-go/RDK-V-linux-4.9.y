/*****************************************************************************
*                            CONFIDENTIAL                                
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :    icam_hal.c
* Author    :    yuguotao
* Project   :    goxcceed api
******************************************************************************
* Purpose   :    
******************************************************************************
* Release History:
  VERSION       Date                AUTHOR          Description
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include "icam_hal.h"

static struct icam_hal_ops* _icam_hal_ops = NULL ;

int icam_hal_init(struct gxav_module_ops *ops)
{
	_icam_hal_ops = ops->priv;

	if (_icam_hal_ops && _icam_hal_ops->init)
	{
		return _icam_hal_ops->init(_icam_hal_ops->priv);
	}

	return -1 ;
}

int icam_hal_cleanup(void)
{
	if (_icam_hal_ops && _icam_hal_ops->cleanup)
	{
		return _icam_hal_ops->cleanup();
	}
	return -1 ;
}

int icam_hal_open(int id)
{
	if (_icam_hal_ops && _icam_hal_ops->open)
	{
		return _icam_hal_ops->open(id);
	}
	return -1 ;
}

int icam_hal_close(int id)
{
	if (_icam_hal_ops && _icam_hal_ops->close)
	{
		return _icam_hal_ops->close(id);
	}
	return -1 ;
}

int icam_hal_set_cardclockdivisor(int id,unsigned int freq)
{
	if (_icam_hal_ops && _icam_hal_ops->set_cardclockdivisor)
	{
		return _icam_hal_ops->set_cardclockdivisor(id,freq);
	}
	return -1 ;
}

int icam_hal_set_vcclevel(int id,unsigned char  vcc_level)
{
	if (_icam_hal_ops && _icam_hal_ops->set_vcclevel)
	{
		return _icam_hal_ops->set_vcclevel(id,vcc_level);
	}
	return -1 ;
}

int icam_hal_set_uartconvdirection(int id,unsigned char convention)
{
	if (_icam_hal_ops && _icam_hal_ops->set_uartconvdirection)
	{
		return _icam_hal_ops->set_uartconvdirection(id,convention);
	}
	return -1 ;
}

int icam_hal_set_uartbaudrate(int id,GxIcamProperty_UartBaudRate *p_BaudRate)
{
	if (_icam_hal_ops && _icam_hal_ops->set_uartbaudrate)
	{
		return _icam_hal_ops->set_uartbaudrate(id,p_BaudRate);
	}
	return -1 ;
}

int icam_hal_set_vccswitch(int id,unsigned char vcc_switch)
{
	if (_icam_hal_ops && _icam_hal_ops->set_vccswitch)
	{
		return _icam_hal_ops->set_vccswitch(id,vcc_switch);
	}
	return -1 ;
}

int icam_hal_set_uartguardtime(int id,unsigned char guardtime)
{
	if (_icam_hal_ops && _icam_hal_ops->set_uartguardtime)
	{
		return _icam_hal_ops->set_uartguardtime(id,guardtime);
	}
	return -1 ;
}

int icam_hal_set_resetcard(int id,unsigned char reset_switch)
{
	if (_icam_hal_ops && _icam_hal_ops->set_resetcard)
	{
		return _icam_hal_ops->set_resetcard(id,reset_switch);
	}
	return -1 ;
}

int icam_hal_set_uartcommand(int id ,TYPE_ICAM28_UartCommand_t *pCommand)
{
	if (_icam_hal_ops && _icam_hal_ops->set_uartcommand)
	{
		return _icam_hal_ops->set_uartcommand(id,pCommand);
	}
	return -1 ;
}

int icam_hal_send_receive(int id ,GxIcamProperty_SendAndReceive *pSendAndRece)
{
	if (_icam_hal_ops && _icam_hal_ops->send_receive)
	{
		return _icam_hal_ops->send_receive(id,pSendAndRece);
	}
	return -1 ;
}

int icam_hal_receive(int id ,GxIcamProperty_Receive *pRece)
{
	if (_icam_hal_ops && _icam_hal_ops->receive)
	{
		return _icam_hal_ops->receive(id,pRece);
	}
	return -1 ;
}

int icam_hal_card_insert_remove(int id,unsigned char bRemove)
{
	if (_icam_hal_ops && _icam_hal_ops->card_insert_remove)
	{
		return _icam_hal_ops->card_insert_remove(id,bRemove);
	}
	return -1 ;
}

int icam_hal_set_controlword(int id,GxIcamProperty_ControlWord *p_ControlWord)
{
	if (_icam_hal_ops && _icam_hal_ops->set_controlword)
	{
		return _icam_hal_ops->set_controlword(id,p_ControlWord);
	}
	return -1 ;
}

int icam_hal_set_config(int id,GxIcamProperty_ConfigInfo *p_Config)
{
	if (_icam_hal_ops && _icam_hal_ops->set_config)
	{
		return _icam_hal_ops->set_config(id,p_Config);
	}
	return -1 ;
}

int icam_hal_get_uartstatue(int id,TYPE_ICAM28_UartStatus_t  *p_uart_status)
{
	if (_icam_hal_ops && _icam_hal_ops->get_uartstatue)
	{
		return _icam_hal_ops->get_uartstatue(id,p_uart_status);
	}
	return -1 ;
}

int icam_hal_get_uartcommond(int id ,TYPE_ICAM28_UartCommand_t *pCommand)
{
	if (_icam_hal_ops && _icam_hal_ops->get_uartcommond)
	{
		return _icam_hal_ops->get_uartcommond(id,pCommand);
	}
	return -1 ;
}

int icam_hal_get_card_event(int id ,unsigned char *p_event)
{
	if (_icam_hal_ops && _icam_hal_ops->get_card_event)
	{
		return _icam_hal_ops->get_card_event(id,p_event);
	}
	return -1 ;
}

int icam_hal_get_event(int id ,unsigned char *p_event)
{
	if (_icam_hal_ops && _icam_hal_ops->get_event)
	{
		return _icam_hal_ops->get_event(id,p_event);
	}
	return -1 ;
}

int icam_hal_get_chipproperties(int id ,GxIcamProperty_ChipProperties *p_Properties)
{
	if (_icam_hal_ops && _icam_hal_ops->get_chipproperties)
	{
		return _icam_hal_ops->get_chipproperties(id,p_Properties);
	}
	return -1 ;
}

int icam_hal_get_responsetochallenge(int id ,GxIcamProperty_ResponseToChallenge *p_ResponseToChallenge)
{
	if (_icam_hal_ops && _icam_hal_ops->get_responsetochallenge)
	{
		return _icam_hal_ops->get_responsetochallenge(id,p_ResponseToChallenge);
	}
	return -1 ;
}

int icam_hal_get_encryptdata(int id ,GxIcamProperty_EncryptData *p_EncryptData)
{
	if (_icam_hal_ops && _icam_hal_ops->get_encryptdata)
	{
		return _icam_hal_ops->get_encryptdata(id,p_EncryptData);
	}
	return -1 ;
}

int icam_hal_get_read_register(int id ,unsigned int register_offset , unsigned int *pData)
{
	if (_icam_hal_ops && _icam_hal_ops->get_read_register)
	{
		return _icam_hal_ops->get_read_register(id,register_offset,pData);
	}
	return -1 ;
}

int icam_hal_get_write_register(int id ,unsigned int register_offset , unsigned int value)
{
	if (_icam_hal_ops && _icam_hal_ops->get_write_register)
	{
		return _icam_hal_ops->get_write_register(id,register_offset,value);
	}
	return -1 ;
}

int icam_hal_get_config(int id ,GxIcamProperty_ConfigInfo *p_Config)
{
	if (_icam_hal_ops && _icam_hal_ops->get_config)
	{
		return _icam_hal_ops->get_config(id,p_Config);
	}
	return -1 ;
}

int icam_hal_get_read_otp( int id ,unsigned int addr ,int num , unsigned char *data)
{
	if (_icam_hal_ops && _icam_hal_ops->get_read_otp)
	{
		return _icam_hal_ops->get_read_otp(id,addr,num,data);
	}
	return -1 ;
}

int icam_hal_get_write_otp(int id ,unsigned int addr ,int num ,unsigned char *data)
{
	if (_icam_hal_ops && _icam_hal_ops->get_write_otp)
	{
		return _icam_hal_ops->get_write_otp(id,addr,num,data);
	}
	return -1 ;
}

int icam_hal_interrupt(int id,int (*callback)(unsigned int event,void *priv),void *priv )
{
	if (_icam_hal_ops && _icam_hal_ops->interrupt)
	{
		return _icam_hal_ops->interrupt(id,callback,priv);
	}
	return -1 ;
}
