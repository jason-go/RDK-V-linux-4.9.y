/*****************************************************************************
*                          CONFIDENTIAL                             
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2014, All right reserved
******************************************************************************
******************************************************************************
* Purpose   :   HAL Service Routine of DEMUX module 
* Release History:
  VERSION       Date              AUTHOR         Description
  2.5           20141105          yuguotao        
*****************************************************************************/


#ifndef  __GX_ICAM_HAL_H_201506111025__
#define  __GX_ICAM_HAL_H_201506111025__

#include "avcore.h"
#include "gxav_icam_propertytypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct icam_reg_ops {
	int (*init)(void);
	int (*cleanup)(void);
	int (*open)(int id);
	int (*close)(int id);
	int (*set_cardclockdivisor)(int id,unsigned int freq);
	int (*card_insert_remove)(int id,unsigned char bRemove);
};

struct icam_hal_ops {
	void *priv ;
	int (*init)(void *ops);
	int (*cleanup)(void);
	int (*open)(int id);
	int (*close)(int id);
	int (*set_cardclockdivisor)(int id,unsigned int freq);
	int (*set_vcclevel)(int id,unsigned char  vcc_level);
	int (*set_uartconvdirection)(int id,unsigned char convention);
	int (*set_uartbaudrate)(int id,GxIcamProperty_UartBaudRate *p_BaudRate);
	int (*set_vccswitch)(int id,unsigned char vcc_switch);
	int (*set_uartguardtime)(int id,unsigned char guardtime);
	int (*set_resetcard)(int id,unsigned char reset_switch);
	int (*set_uartcommand)(int id ,TYPE_ICAM28_UartCommand_t *pCommand);
	int (*send_receive)(int id ,GxIcamProperty_SendAndReceive *pSendAndRece);
	int (*receive)(int id ,GxIcamProperty_Receive *pRece);
	int (*card_insert_remove)(int id,unsigned char bRemove);
	int (*set_controlword)(int id,GxIcamProperty_ControlWord *p_ControlWord);
	int (*set_config)(int id,GxIcamProperty_ConfigInfo *p_Config);

	int (*get_uartstatue)(int id,TYPE_ICAM28_UartStatus_t  *p_uart_status);
	int (*get_uartcommond)(int id ,TYPE_ICAM28_UartCommand_t *pCommand);
	int (*get_card_event)(int id ,unsigned char *p_event);
	int (*get_event)(int id ,unsigned char *p_event);
	int (*get_chipproperties)(int id ,GxIcamProperty_ChipProperties *p_Properties);
	int (*get_responsetochallenge)(int id ,GxIcamProperty_ResponseToChallenge *p_ResponseToChallenge);
	int (*get_encryptdata)(int id ,GxIcamProperty_EncryptData *p_EncryptData);
	int (*get_read_register)(int id ,unsigned int register_offset , unsigned int *pData);
	int (*get_write_register)(int id ,unsigned int register_offset , unsigned int value);
	int (*get_config)(int id ,GxIcamProperty_ConfigInfo *p_Config);
	int (*get_read_otp)(int id ,unsigned int addr ,  int num ,unsigned char *data);
	int (*get_write_otp)(int id ,unsigned int addr , int num ,unsigned char *data);

	int (*interrupt)(int id ,int (*callback)(unsigned int event,void *priv),void *priv);
};

int icam_hal_init(struct gxav_module_ops *ops);
int icam_hal_cleanup(void);
int icam_hal_open(int id);
int icam_hal_close(int id);
int icam_hal_set_cardclockdivisor(int id,unsigned int freq);
int icam_hal_set_vcclevel(int id,unsigned char vcc_level);
int icam_hal_set_uartconvdirection(int id,unsigned char convention);
int icam_hal_set_uartbaudrate(int id,GxIcamProperty_UartBaudRate *p_BaudRate);
int icam_hal_set_vccswitch(int id,unsigned char vcc_switch);
int icam_hal_set_uartguardtime(int id,unsigned char guardtime);
int icam_hal_set_resetcard(int id,unsigned char reset_switch);
int icam_hal_set_uartcommand(int id ,TYPE_ICAM28_UartCommand_t *pCommand);
int icam_hal_send_receive(int id ,GxIcamProperty_SendAndReceive *pSendAndRece);
int icam_hal_receive(int id ,GxIcamProperty_Receive *pRece);
int icam_hal_card_insert_remove(int id,unsigned char bRemove);
int icam_hal_set_controlword(int id,GxIcamProperty_ControlWord *p_ControlWord);
int icam_hal_set_config(int id,GxIcamProperty_ConfigInfo *p_Config);

int icam_hal_get_uartstatue(int id,TYPE_ICAM28_UartStatus_t  *p_uart_status);
int icam_hal_get_uartcommond(int id ,TYPE_ICAM28_UartCommand_t *pCommand);
int icam_hal_get_card_event(int id ,unsigned char *p_event);
int icam_hal_get_event(int id ,unsigned char *p_event);
int icam_hal_get_chipproperties(int id ,GxIcamProperty_ChipProperties *p_Properties);
int icam_hal_get_responsetochallenge(int id ,GxIcamProperty_ResponseToChallenge *p_ResponseToChallenge);
int icam_hal_get_encryptdata(int id ,GxIcamProperty_EncryptData *p_EncryptData);
int icam_hal_get_read_register(int id ,unsigned int register_offset , unsigned int *pData);
int icam_hal_get_write_register(int id ,unsigned int register_offset , unsigned int value);
int icam_hal_get_config(int id ,GxIcamProperty_ConfigInfo *p_Config);
int icam_hal_get_read_otp( int id ,unsigned int addr ,int num ,unsigned char *data);
int icam_hal_get_write_otp(int id ,unsigned int addr ,int num ,unsigned char *data);

int icam_hal_interrupt(int id,int (*callback)(unsigned int event,void *priv),void *priv);

#ifdef __cplusplus
}
#endif


#endif

