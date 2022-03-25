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


#ifndef  __GX_ICAM_28_INCLDUE_H_201506111325__
#define  __GX_ICAM_28_INCLDUE_H_201506111325__

//#include "avcore.h"


#ifdef __cplusplus
extern "C" {
#endif


#define MIN_ALIGNMENT_NUM              (256)
#define SYS_MALLOC_ALIGNMENT_LEN       (32)
#define M2M_MALLOC_ALIGNMENT(x)  (x + SYS_MALLOC_ALIGNMENT_LEN - (x % SYS_MALLOC_ALIGNMENT_LEN))


#define MAX_ICAM_COMMUNICATION_BUFF_SIZE (1024)
#define MAX_ICAM_EVENT_BUFF_SIZE         (40)


typedef enum {
	ICAM_INT_TYPE_RECEIVER            = 0,
	ICAM_INT_TYPE_TRANSMIT            = 1,
	ICAM_INT_TYPE_DETECT              = 2,
	ICAM_INT_TYPE_OVERFLOW            = 3,
	ICAM_INT_TYPE_PARITY_ERROR        = 4,
	ICAM_INT_TYPE_VCCON               = 5,
	ICAM_INT_TYPE_VCCOFF              = 6
}GxIcamInt_Type;

typedef enum
{
	HAL_COMMAND_MODE_NORMAL         = 0 ,
	HAL_COMMAND_MODE_RESTART        = 1 ,
	HAL_COMMAND_MODE_LAST_WRITE_CW  = 2 ,
	HAL_COMMAND_MODE_SELF_SETTING   = 3 ,
	HAL_COMMAND_MODE_NONCE          = 4 ,
	HAL_COMMAND_MODE_BY_PASS        = 5
}HAL_COMMAND_MODE_Op_e;

typedef enum
{
	COMMUNICATION_NORMAL_STATUS   = 0,  //not communicating and can do that
	COMMUNICATION_TRANSFER_STATUS = 1,  //transferring data
	COMMUNICATION_RECEIVE_STATUS  = 2   //receiving data
}CommunicationStatus_e;

typedef enum
{
	HAL_EVENT_DETECT_VCC_ON       = 0 ,
	HAL_EVENT_DETECT_VCC_OFF      = 1 ,
	HAL_EVENT_DATA_TRANSFER_OK    = 2 ,
	HAL_EVENT_DATA_RECEIVE_OK     = 3 ,
	HAL_EVENT_DATA_OVER_FLOW      = 4 ,
	HAL_EVENT_DATA_BAD_PARITY     = 5
}HAL_EVENT_FLAG_e;

typedef struct
{
	unsigned char *m_p_user_buff ;
	unsigned char m_buff[MAX_ICAM_COMMUNICATION_BUFF_SIZE];
	unsigned int  m_size   ;
	unsigned int  m_index  ;
}CommunicationData_t  ;

typedef struct
{
	CommunicationData_t   m_unit_send ;
	CommunicationData_t   m_unit_rece ;
	CommunicationStatus_e m_work_status;
	unsigned char         m_UseFlowControl ;
	unsigned char         m_UseNullFilter   ;
	unsigned char         m_buff_event[MAX_ICAM_EVENT_BUFF_SIZE]    ;
	unsigned int          m_read_event      ;
	unsigned int          m_write_event     ;
}CommunicationUnit_t    ;

typedef struct
{
	unsigned int  m_reset_card_time ;
	unsigned int  m_clock ;
	unsigned int  m_icam_frequency  ;
	unsigned int  m_icam_buard_rate_reg_value ;
	unsigned int  m_icam_buard_rate_factory   ;
	unsigned char m_vcc_level ;
	unsigned char m_vcc_switch;
	unsigned char m_vcc_value ;
}HAL_ICAM28_Config_t;


int gx28_icam_init(void *ops);
int gx28_icam_cleanup(void);
int gx28_icam_open(int id);
int gx28_icam_close(int id);
int gx28_icam_set_cardclockdivisor(int id ,unsigned freq);
int gx28_icam_set_vcclevel(int id ,unsigned char vcc_level);
int gx28_icam_set_uartconvdirection(int id,unsigned char convention);
int gx28_icam_set_uartbaudrate(int id,GxIcamProperty_UartBaudRate *p_BaudRate);
int gx28_icam_set_vccswitch(int id,unsigned char vcc_switch);
int gx28_icam_set_uartguardtime(int id,unsigned char guardtime);
int gx28_icam_set_resetcard(int id,unsigned char reset_switch);
int gx28_icam_set_uartcommand(int id ,TYPE_ICAM28_UartCommand_t *pCommand);
int gx28_icam_send_receive(int id ,GxIcamProperty_SendAndReceive *pSendAndRece);
int gx28_icam_receive(int id ,GxIcamProperty_Receive *pRece);
int gx28_icam_card_insert_remove(int id,unsigned char bRemove);
int gx28_icam_set_controlword(int id,GxIcamProperty_ControlWord *p_ControlWord);
int gx28_icam_set_config(int id,GxIcamProperty_ConfigInfo *p_Config);

int gx28_icam_get_uartstatue(int id,TYPE_ICAM28_UartStatus_t  *p_uart_status);
int gx28_icam_get_uartcommond(int id ,TYPE_ICAM28_UartCommand_t *pCommand);
int gx28_icam_get_card_event(int id ,unsigned char *p_event);
int gx28_icam_get_event(int id ,unsigned char *p_event);
int gx28_icam_get_chipproperties(int id ,GxIcamProperty_ChipProperties *p_Properties);
int gx28_icam_get_responsetochallenge(int id ,GxIcamProperty_ResponseToChallenge *p_ResponseToChallenge);
int gx28_icam_get_encryptdata(int id ,GxIcamProperty_EncryptData *p_EncryptData);
int gx28_icam_get_read_register(int id ,unsigned int register_offset , unsigned int *pData);
int gx28_icam_get_write_register(int id ,unsigned int register_offset , unsigned int value);
int gx28_icam_get_config(int id ,GxIcamProperty_ConfigInfo *p_Config);
int gx28_icam_get_read_otp(int id  ,unsigned int addr ,int num , unsigned char *data);
int gx28_icam_get_write_otp(int id ,unsigned int addr ,int num , unsigned char *data);

int gx28_icam_interrupt(int id,int (*callback)(unsigned int event,void *priv),void *priv);

#ifdef __cplusplus
}
#endif


#endif

