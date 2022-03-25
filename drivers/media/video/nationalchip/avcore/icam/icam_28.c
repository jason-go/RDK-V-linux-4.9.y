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

#include "icam_hal.h"
#include "icam_28_include.h"
#include "icam_28_reg.h"
#include "gx3211_reg.h"
#include "gxav_icam_propertytypes.h"
#include "gxav_event_type.h"

volatile unsigned int UartCtrlStat = 0 ;
Reg_NdsFunction_t *gx28_icam_reg = NULL;
static struct icam_reg_ops* _icam_reg_ops = NULL ;

static HAL_ICAM28_Config_t ICAM28_Config_Info={0} ;
static CommunicationUnit_t Comm_Unit  ;

#ifdef ICAM_ENABLE_INTTERUPT_SEEK_INTERRUPT

#define ICAM_DEFAULT_INTTERUPT_SEEK_INTERRUPT_TIME (8000)

#define ICAM_MAX_INTTERUPT_SEEK_INTERRUPT_TIME (8000*100)

static unsigned int s_icam_interrupt_seek_interrupt_time = ICAM_DEFAULT_INTTERUPT_SEEK_INTERRUPT_TIME  ;
static unsigned int __hal_icam28_get_interrupt_seek_interrupt_time(void)
{
	return s_icam_interrupt_seek_interrupt_time ;
}
static void __hal_icam28_set_interrupt_seek_interrupt_time(unsigned int time)
{
	s_icam_interrupt_seek_interrupt_time = time ;
}
#endif

#define  OS_INTERRUPT_ENABLE

void __hal_icam28_interrupt_disable(void)
{
#ifdef OS_INTERRUPT_ENABLE	
	__asm__ __volatile__(" psrclr fe,ie;");
#endif
}

void __hal_icam28_interrupt_enable(void)
{
#ifdef OS_INTERRUPT_ENABLE	
	__asm__ __volatile__( " psrset fe,ie;");
#endif
}

int __hal_icam28_get_smartcard_statue(unsigned char *pInsert)
{
	unsigned char ctrl_statue ;

	ctrl_statue = ICAM_GET_UART_ALL_STATUE(gx28_icam_reg);

	if ( (ctrl_statue >> bICAM_UART_DETECT ) & 0x1)
	{
		*pInsert = 1;
	}
	else
	{
		*pInsert = 0 ;
	}

	return 0 ;
}

int __hal_icam28_flowcontrol(unsigned char IsFlowControl)
{
	__hal_icam28_interrupt_disable();

	ICAM_SET_FLOWCONTROL_NOT_ACTIVE(gx28_icam_reg, UartCtrlStat);

	if ( IsFlowControl )
	{
		ICAM_SET_FLOWCONTROL_ACTIVE(gx28_icam_reg, UartCtrlStat);
	}

	__hal_icam28_interrupt_enable();

	return 0 ;
}

int HAL_ICAM28_SetFunctionalData(unsigned char   *pValue)
{
	unsigned int Word;
	int i;

	for (i=0; i<4; i++)
	{
		Word = (pValue[i*4]<<24) | (pValue[i*4+1]<<16) | (pValue[i*4+2]<<8) | (pValue[i*4+3]);

	//	gx_printf("set cw 0x%x \n",Word);

		ICAM_SET_DATA_WORD(gx28_icam_reg, i, Word);
	}

	return 0 ;
}

int HAL_ICAM28_SetFunctionalCWCommand(unsigned int op_mode,unsigned int op_value)
{
	unsigned int value ;

	switch(op_mode)
	{
		case HAL_COMMAND_MODE_NORMAL:
			{
				value = (CW_OPCODE<<bICAM_COMMOND_OPCODE) ;
			}
			break;

		case HAL_COMMAND_MODE_RESTART:
			{
				value = (0x1<<31) | (CW_OPCODE<<bICAM_COMMOND_OPCODE) ;
			}
			break;

		case HAL_COMMAND_MODE_LAST_WRITE_CW:
			{
				value = ( ((0x1<<24)|op_value) & 0xfffffffe ) ;
			}
			break;

		case HAL_COMMAND_MODE_SELF_SETTING:
			{
				value = (op_value  & 0xfffffffe );
			}
			break;

		case HAL_COMMAND_MODE_NONCE:
			{
				 value = (NONCE_OPCODE<<bICAM_COMMOND_OPCODE);
			}
			break;

		case HAL_COMMAND_MODE_BY_PASS:
			{
				value = op_value ;
			}
			break;

		default:
			return -1 ;
	}


//	gx_printf("drv>  set cw Command  0x%x \n",value) ;
	ICAM_SET_COMMAND_CW_WORD_3(gx28_icam_reg, value);

	return 0 ;
}

int HAL_ICAM28_WaitICAMAcceptAcpuCommandStatue(unsigned int waittime)
{
	unsigned int status ;
	int delay ;

	delay = 100000;

	do
	{
		status = ICAM_GET_ICAM_STATUS(gx28_icam_reg);
		if (delay <= 0 )
		{
			return -1 ;
		}
		delay-- ;

	}while ( (status & 0x1) );

	return 0 ;
}

int  HAL_ICAM28_CheckCommandOK_ByIsr(void)
{
	unsigned int           IntByte;

	unsigned char    is_ok  = 1 ;

	unsigned int wait_time = 10000 ;

	do
	{
		wait_time--;

		IntByte    = ICAM_GET_INT_STATUS(gx28_icam_reg);

		if (IntByte & (1<<bICAM_INTC_UART)){}
		else  if (IntByte & (1<<bICAM_INTC_ILLEGAL_SEQ))
		{
			ICAM_CLR_ILLEGAL_INT(gx28_icam_reg, bICAM_INTC_ILLEGAL_SEQ);

			is_ok  = 0 ;
		}
		else if (IntByte & (1<<bICAM_INTC_ILLEGAL_DATA))
		{
			ICAM_CLR_ILLEGAL_INT(gx28_icam_reg, bICAM_INTC_ILLEGAL_DATA);

			is_ok  = 0 ;
		}
		else if (IntByte & (1<<bICAM_INTC_ILLEGAL_OTP))
		{
			ICAM_CLR_ILLEGAL_INT(gx28_icam_reg, bICAM_INTC_ILLEGAL_OTP);
		}
		else if (IntByte & (1<<bICAM_INTC_ILLEGAL_KTE))
		{
			ICAM_CLR_ILLEGAL_INT(gx28_icam_reg, bICAM_INTC_ILLEGAL_KTE);
		}

		if ( (IntByte & 0x1f) == 0 )
		{
			break;
		}
		if ( wait_time <= 0 )
		{
			break;
		}
	}while(1);

	if ( is_ok == 0 )
	{
		return -1 ;
	}
	else
	{
		return 0 ;
	}
}

int HAL_ICAM28_SetCommand_WaitOK(unsigned int op_mode,unsigned int op_value)
{
	int ret = 0 ;

	ret = HAL_ICAM28_SetFunctionalCWCommand(op_mode,op_value) ;

	ret |= HAL_ICAM28_WaitICAMAcceptAcpuCommandStatue(0);

	ret |= HAL_ICAM28_CheckCommandOK_ByIsr();

	return ret ;
}

int HAL_ICAM28_GetUartStatue(TYPE_ICAM28_UartStatus_t  *pUartStauts)
{
	unsigned char ctrl_statue,int_statue ;

	ctrl_statue = ICAM_GET_UART_ALL_STATUE(gx28_icam_reg);
	int_statue = ICAM_GET_INT_UART_STATUS(gx28_icam_reg);

	if ( (ctrl_statue >> bICAM_UART_TX_READY ) & 0x1)
	{
		 pUartStauts->m_TxReady = 1;
	}
	if ( (ctrl_statue >> bICAM_UART_RX_READY ) & 0x1)
	{
		 pUartStauts->m_RxReady = 1;
	}
	if ( (ctrl_statue >> bICAM_UART_INTC_PENDING ) & 0x1)
	{
		 pUartStauts->m_InterruptPending = 1;
	}
	if ( (ctrl_statue >> bICAM_UART_DIRECTION ) & 0x1)
	{
		 pUartStauts->m_ConventionDirect = 1;
	}
	if ( (ctrl_statue >> bICAM_UART_DETECT ) & 0x1)
	{
		 pUartStauts->m_CardInsert = 1;
	}

	if ( (int_statue >> bICAM_UART_INT_OVERFLOW ) & 0x1)
	{
		 pUartStauts->m_OverFlow = 1;
	}
	if ( (int_statue >> bICAM_UART_TRANSMIT_ERROR ) & 0x1)
	{
		 pUartStauts->m_TxError = 1;
	}
	if ( (int_statue >> bICAM_UART_RECEIVE_ERROR ) & 0x1)
	{
		 pUartStauts->m_RxError = 1;
	}

	return 0;
}

int HAL_ICAM28_SetUartIntMask(TYPE_ICAM28_UartStatus_t status)
{
	unsigned char      Value = 0;

	if (status.m_TxReady == 0)
	{
		Value |= (1<<bICAM_UART_INT_TRANSMIT);
	}
	if (status.m_RxReady == 0)
	{
		Value |= (1<<bICAM_UART_INT_RECEIVE);
	}
	if (status.m_TxError == 0)
	{
		Value |= (1<<bICAM_UART_TRANSMIT_ERROR);
	}
	if (status.m_RxError == 0)
	{
		Value |= (1<<bICAM_UART_RECEIVE_ERROR);
	}
	if (status.m_OverFlow == 0)
	{
		Value |= (1<<bICAM_UART_INT_OVERFLOW);
	}
	if (status.m_CardInsert == 0)
	{
		Value |= (1<<bICAM_UART_INT_DETECT);
	}

        // gx_printf("the value 0x%x \n",Value);

	ICAM_SET_UART_INT_MASK(gx28_icam_reg, Value);

	return 0 ;
}


static unsigned char DefaultValue[16] = {0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87, 0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f};

static void __HAL_SetCW(int times)
{
	int i = 0, j =0 ;

	volatile unsigned int data ;

	for (i=0; i<times; i++)
	{
		for(j=0;j<4;j++)
		{
			data = ( (DefaultValue[4*j+0]<<24) | (DefaultValue[4*j+1]<<16) |\
					(DefaultValue[4*j+2]<<8) | DefaultValue[4*j+3] ) ;

			ICAM_SET_DATA_WORD(gx28_icam_reg, j, data);
		}

		if (i == 0)
		{
			ICAM_SET_COMMAND_CW_WORD_3(gx28_icam_reg, 0x80000000) ;
		}
		else
		{
			ICAM_SET_COMMAND_CW_WORD_3(gx28_icam_reg, 0x0) ;
		}

		HAL_ICAM28_WaitICAMAcceptAcpuCommandStatue(0);
	}
}

static void __HAL_SetNonec(unsigned char *pData)
{
	int i,j;
	unsigned int Word;

	volatile unsigned int data ;

	for(j=0;j<4;j++)
	{
		data = ( (DefaultValue[4*j+0]<<24) | (DefaultValue[4*j+1]<<16) | \
				(DefaultValue[4*j+2]<<8) | DefaultValue[4*j+3] ) ;

		ICAM_SET_DATA_WORD(gx28_icam_reg, j, data);
	}

	ICAM_SET_COMMAND_CW_WORD_3(gx28_icam_reg, 0x1) ;

	HAL_ICAM28_WaitICAMAcceptAcpuCommandStatue(0);

	for (i=0; i<4; i++)
	{
		Word = ICAM_GET_DATA_WORD(gx28_icam_reg, i);
		//     test_printf("word : 0x%x\n", Word);
		pData[i*4]   = (unsigned char)((Word>>24)&0xff);
		pData[i*4+1] = (unsigned char)((Word>>16)&0xff);
		pData[i*4+2] = (unsigned char)((Word>>8)&0xff);
		pData[i*4+3] = (unsigned char)(Word&0xff);
	}
	//  test_printf("\n\n\n", Word);
}

int HAL_ICAM28_GetHashKey(unsigned char *pHashKey)
{
	__HAL_SetCW(3);

	__HAL_SetNonec(pHashKey);

	return 0 ;
}

int HAL_ICAM28_SetDefaultData(void)
{
	unsigned int Word;
	int i;
	for (i=0; i<4; i++)
	{
		Word = (DefaultValue[i*4]<<24) | (DefaultValue[i*4+1]<<16) | \
		       (DefaultValue[i*4+2]<<8) | (DefaultValue[i*4+3]);
	//	printf("set cw 0x%x \n",Word);
		ICAM_SET_DATA_WORD(gx28_icam_reg, i, Word);
	}

	return 0 ;
}

int HAL_ICAM28_GetFunctionalData(unsigned char   *pValue)
{
	unsigned int Word;
	int i;

	for (i=0; i<4; i++)
	{
		Word = ICAM_GET_DATA_WORD(gx28_icam_reg, i);
		pValue[i*4]   = (unsigned char)(Word>>24);
		pValue[i*4+1] = (unsigned char)((Word>>16)&0xff);
		pValue[i*4+2] = (unsigned char)((Word>>8)&0xff);
		pValue[i*4+3] = (unsigned char)(Word&0xff);
	}

	return 0 ;
}

int HAL_ICAM28_SetCommand_GetData_WaitOK(unsigned int op_mode,unsigned int op_value,unsigned char *pData)
{
	int ret = 0 ;

	ret = HAL_ICAM28_SetFunctionalCWCommand(op_mode,op_value) ;

	ret |= HAL_ICAM28_WaitICAMAcceptAcpuCommandStatue(0);

	ret |= HAL_ICAM28_GetFunctionalData(pData);

	ret |= HAL_ICAM28_CheckCommandOK_ByIsr();

	return ret ;
}

int HAL_ICAM28_GetSmartCardInterrupt(unsigned int *pInterrupt,unsigned char *pOhter)
{
	unsigned int   IntByte = 0 ;

	IntByte    = ICAM_GET_INT_UART_STATUS(gx28_icam_reg);

	if ( IntByte & (1<<bICAM_UART_RECEIVE_ERROR))
	{
		ICAM_CLR_UART_INT(gx28_icam_reg, bICAM_UART_RECEIVE_ERROR);
	}

	if ( IntByte & (1<<bICAM_UART_INT_RECEIVE))   //recv intc
	{
		*pOhter = ICAM_GET_UART_DATA(gx28_icam_reg);  //*(volatile U32 *)0xd0000100;

		ICAM_CLR_UART_INT(gx28_icam_reg, bICAM_UART_INT_RECEIVE);

		*pInterrupt = ICAM_INT_TYPE_RECEIVER ;
	}
	else if (IntByte & (1<<bICAM_UART_INT_TRANSMIT))
	{
		ICAM_CLR_UART_INT(gx28_icam_reg, bICAM_UART_INT_TRANSMIT);
		*pInterrupt = ICAM_INT_TYPE_TRANSMIT ;
	}
	else if (IntByte & (1<<bICAM_UART_INT_DETECT))
	{
		ICAM_CLR_UART_INT(gx28_icam_reg, bICAM_UART_INT_DETECT);

		*pInterrupt = ICAM_INT_TYPE_DETECT ;
	}
	else if (IntByte & (1<<bICAM_UART_INT_OVERFLOW))
	{
		ICAM_CLR_UART_INT(gx28_icam_reg, bICAM_UART_INT_OVERFLOW);

		*pInterrupt  = ICAM_INT_TYPE_OVERFLOW ;

	//	test_printf("***********isr************************ overflow 0\n");
	}
	else
	{
	//	CAASSERT(0);
	//	error_printf("~~~~~~~~~~~~~~ isr other \n");

		return -1 ;
	}

	return 0 ;
}


//////////////////////////////////////////////////////////////////

/* interface */

int gx28_icam_init(void *ops)
{
	int ret = -1 ;

	_icam_reg_ops = (struct icam_reg_ops*)ops;

	if (_icam_reg_ops && _icam_reg_ops->init)
	{
		ret = _icam_reg_ops->init();

		ICAM_SET_UART_INT_MASK(gx28_icam_reg, 0x3f);

		ICAM_SET_INTTERRUPT_MASK(gx28_icam_reg, 0x24);

		ICAM_SET_UART_COM_VALUE(gx28_icam_reg, 0x60); // add by 20150708 
		// IO_C7_ON | C4_ON | C8_ON  if not set ,the default maybe is not IO_C7_ON | C4_ON | C8_ON

		memset(&ICAM28_Config_Info,0,sizeof(HAL_ICAM28_Config_t));

		Comm_Unit.m_read_event   = 0 ;
		Comm_Unit.m_write_event  = 0 ;
	}

	return ret ;
}

int gx28_icam_cleanup(void)
{
	int ret = -1 ;

	if (_icam_reg_ops && _icam_reg_ops->cleanup)
	{
		ret = _icam_reg_ops->cleanup();
	}

	return ret ;
}

int gx28_icam_open(int id)
{
	int ret = -1 ;

	if (_icam_reg_ops && _icam_reg_ops->open)
	{
		ret = _icam_reg_ops->open(id);
	}

	return ret ;
}

int gx28_icam_close(int id)
{
	int ret = -1 ;

	if (_icam_reg_ops && _icam_reg_ops->close)
	{
		ret = _icam_reg_ops->close(id);
	}

	return ret ;
}

int gx28_icam_set_cardclockdivisor(int id ,unsigned freq)
{
	int ret = -1 ;

	if (_icam_reg_ops && _icam_reg_ops->set_cardclockdivisor)
	{
		ret = _icam_reg_ops->set_cardclockdivisor(id,freq);

		ICAM28_Config_Info.m_clock = freq ;
	}

	return ret ;
}

int gx28_icam_set_vcclevel(int id ,unsigned char vcc_level)
{
	unsigned char card_status = 0;

	__hal_icam28_get_smartcard_statue(&card_status);

	if ( card_status == 0 )
	{
		return -1 ;
	}

	ICAM28_Config_Info.m_vcc_level = vcc_level ;

	return 0 ;
}

int gx28_icam_set_uartconvdirection(int id,unsigned char convention)
{
	__hal_icam28_interrupt_disable();

	if (convention)
	{
		ICAM_SET_UART_CONVENTION_LSB_FIRST(gx28_icam_reg, UartCtrlStat);
	}
	else
	{
		ICAM_SET_UART_CONVENTION_MSB_FIRST(gx28_icam_reg, UartCtrlStat);
	}

	__hal_icam28_interrupt_enable();

	return 0 ;
}

int gx28_icam_set_uartbaudrate(int id,GxIcamProperty_UartBaudRate *p_BaudRate )
{
	unsigned int Factor = 0;
	unsigned int Value =  0;
	unsigned char card_status = 0;
	unsigned char  ValueH, ValueL;
	unsigned char baudrate ;
	unsigned int  icam_frequency ;
	unsigned int offset = 0 ;

	__hal_icam28_get_smartcard_statue(&card_status);

	if ( card_status == 0 )
	{
		return -1 ;
	}

	baudrate = p_BaudRate->m_uartbaudrate ;
	icam_frequency = p_BaudRate->m_icam_frequence ;

	switch (baudrate)
	{
		case TYPE_ICAM28_BAUD_RATE_9600:
			Factor =  372 * 100 ;
			break;
		case TYPE_ICAM28_BAUD_RATE_19200:
			Factor =  372 * 100 / 2;
			break;
		case TYPE_ICAM28_BAUD_RATE_38400:
			Factor =  372 * 100 / 4;
			break;
		case TYPE_ICAM28_BAUD_RATE_76800:
			Factor =  372 * 100 / 8;
			break;
		case TYPE_ICAM28_BAUD_RATE_153600:
			Factor =  372 * 100 / 16;
			break;
		case TYPE_ICAM28_BAUD_RATE_178560:
			Factor =  20  * 100 ;
			break;
		case TYPE_ICAM28_BAUD_RATE_223200:
			Factor =  16 * 100 ;
			break;
		default:
			Factor = 372 * 100 ;
			break;
	}

//	gx_printf("ICAM_FREQUENCY %d,%d,%d \n",icam_frequency,ICAM28_Config_Info.m_clock,Factor);

	offset = 1 ;

	ICAM28_Config_Info.m_icam_frequency = icam_frequency ;
	ICAM28_Config_Info.m_icam_buard_rate_factory = ((baudrate<<24)|(offset<<16)|Factor) ;

	Value = ( icam_frequency ) / ( (ICAM28_Config_Info.m_clock * 100 )/ Factor) - 2 ;
   // 防止FACTOR 由于372除以某个数据，造成数据只能十进制，所以减少，最后实际数据变大，数据偏差.1000是由于超过U32

//  gx_printf("icam baund real vale 0x%x \n",Value);

	Value = Value - offset ;
   	/*
   	* patch 20190427
	*/
	if ( baudrate == TYPE_ICAM28_BAUD_RATE_223200  && ICAM28_Config_Info.m_clock == 13500000)
	{
		gx_printf("ICAM_FREQUENCY 0x%x -> 0xc2 \n",Value);
		Value = 0xc2 ;
	}

	ValueH = (Value>>8)&0xff ;
	ValueL =  Value & 0xff   ;

	ICAM28_Config_Info.m_icam_buard_rate_reg_value = Value ;

 //   gx_printf("uart baund Rate 0x%x, reg h: 0x%x , l : 0x%x \n",baudrate,ValueH,ValueL);

	ICAM_SET_UART_BAUD_RATE_H(gx28_icam_reg, ValueH);
	ICAM_SET_UART_BAUD_RATE_L(gx28_icam_reg, ValueL);

	return 0 ;
}

int gx28_icam_set_vccswitch(int id,unsigned char vcc_switch)
{
	int Delay = 10000 ;
	unsigned char card_status = 0 ;

	__hal_icam28_get_smartcard_statue(&card_status);

	if ( card_status == 0 )
	{
		return -1;
	}

	if (vcc_switch == TYPE_ICAM28_VCC_ON)
	{
		if (ICAM28_Config_Info.m_vcc_level == 3)
		{
			__hal_icam28_interrupt_disable();

			ICAM_SET_VCC_3V(gx28_icam_reg, UartCtrlStat);

			__hal_icam28_interrupt_enable();


			ICAM28_Config_Info.m_vcc_value = 3;
		}
		else
		{
			__hal_icam28_interrupt_disable();

			ICAM_SET_VCC_5V(gx28_icam_reg, UartCtrlStat);

			__hal_icam28_interrupt_enable();

			ICAM28_Config_Info.m_vcc_value = 5;
		}

		while (Delay--);

		__hal_icam28_interrupt_disable();

		ICAM_SET_VCC_ACTIVE(gx28_icam_reg, UartCtrlStat);

		__hal_icam28_interrupt_enable();

	}
	else if (vcc_switch == TYPE_ICAM28_VCC_OFF)
	{
		__hal_icam28_interrupt_disable();

		ICAM_SET_VCC_INACTIVE(gx28_icam_reg, UartCtrlStat);

		__hal_icam28_interrupt_enable();

	    ICAM28_Config_Info.m_vcc_value = 0;
	}
	else
	{
		return -1;
	}

	return 0 ;
}

int gx28_icam_set_uartguardtime(int id,unsigned char guardtime)
{
	ICAM_SET_UART_GUARD_TIME(gx28_icam_reg, guardtime);

	return 0 ;
}

int gx28_icam_set_resetcard(int id,unsigned char reset_switch)
{
	ICAM28_Config_Info.m_reset_card_time ++ ;

	__hal_icam28_interrupt_disable();

	ICAM_SET_SMARTCARD_SET(gx28_icam_reg, UartCtrlStat);

	__hal_icam28_interrupt_enable();

//	gx_printf(" reset card  \n");

	return 0;
}

int gx28_icam_set_uartcommand(int id ,TYPE_ICAM28_UartCommand_t *pCommand)
{
	int i = 0;

	TYPE_ICAM28_UartCommand_t   UartCommand  ;

	memcpy(&UartCommand,pCommand,sizeof(TYPE_ICAM28_UartCommand_t));

	if (UartCommand.m_IO_VIA_C7 == 1)
	{
		if (UartCommand.m_C7_ON == 1)
		{
			UartCommand.m_C7_ON = 0;//忽略
		}
		i++;
	}
	if (UartCommand.m_IO_VIA_C4 == 1)
	{
		if (UartCommand.m_C4_ON == 1)
		{
			UartCommand.m_C4_ON = 0;//忽略
		}
		if (UartCommand.m_C7_ON == 1 || UartCommand.m_C8_ON == 1)
		{
			return -1;
		}
		i++;
	}
	if (UartCommand.m_IO_VIA_C8 == 1)
	{
		if (UartCommand.m_C8_ON == 1)
		{
			UartCommand.m_C8_ON = 0;//忽略
		}
		if (UartCommand.m_C7_ON == 1 || UartCommand.m_C4_ON == 1)
		{
			return -1;
		}
		i++;
	}
	if (i != 1)//IO_VIA_C7 IO_VIA_C4 IO_VIA_C8只能有一个为1
	{
		return -1;
	}

//	HAL_ICAM28_SetUartCommond(UartCommand);

	if (UartCommand.m_IO_VIA_C7 == 1)
	{
		if (UartCommand.m_C4_ON == 1 && UartCommand.m_C8_ON == 1)
		{
			ICAM_SET_UART_COM_VALUE(gx28_icam_reg, 0x60);
		}
		else if (UartCommand.m_C4_ON == 1 && UartCommand.m_C8_ON == 0)
		{
			ICAM_SET_UART_COM_VALUE(gx28_icam_reg, 0x20);
		}
		else if (UartCommand.m_C4_ON == 0 && UartCommand.m_C8_ON == 1)
		{
			ICAM_SET_UART_COM_VALUE(gx28_icam_reg, 0x40);
		}
		else
		{
			ICAM_SET_UART_COM_VALUE(gx28_icam_reg, 0x00);
		}
	}
	else if (UartCommand.m_IO_VIA_C4 == 1)
	{
		ICAM_SET_UART_COM_VALUE(gx28_icam_reg, 0x08);
	}
	else if (UartCommand.m_IO_VIA_C8 == 1)
	{
		ICAM_SET_UART_COM_VALUE(gx28_icam_reg, 0x10);
	}

	return 0 ;
}

int gx28_icam_send_receive(int id ,GxIcamProperty_SendAndReceive *pSendAndRece)
{
	int  ret = 0;
	int TimeOut ;
	TYPE_ICAM28_UartStatus_t  UartStauts ;

	if ( pSendAndRece->m_n_send > MAX_ICAM_COMMUNICATION_BUFF_SIZE ||
		 pSendAndRece->m_n_rece > MAX_ICAM_COMMUNICATION_BUFF_SIZE )
	{
		gx_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!! send & rece paramer error  \n");
		return -1 ;
	}

	Comm_Unit.m_unit_send.m_p_user_buff = pSendAndRece->m_p_send ;
	Comm_Unit.m_unit_send.m_size        = pSendAndRece->m_n_send ;
	Comm_Unit.m_unit_send.m_index       = 0 ;

	gx_copy_from_user(Comm_Unit.m_unit_send.m_buff,pSendAndRece->m_p_send,pSendAndRece->m_n_send);

	Comm_Unit.m_unit_rece.m_p_user_buff = pSendAndRece->m_p_rece;
	Comm_Unit.m_unit_rece.m_size        = pSendAndRece->m_n_rece ;
	Comm_Unit.m_unit_rece.m_index       = 0 ;

	Comm_Unit.m_work_status       = COMMUNICATION_TRANSFER_STATUS ;

	Comm_Unit.m_UseFlowControl    = pSendAndRece->m_flow_control ;
	Comm_Unit.m_UseNullFilter     = pSendAndRece->m_null_filter  ;

	Comm_Unit.m_unit_send.m_index++;
	if (pSendAndRece->m_n_send == 1)
	{
		Comm_Unit.m_work_status = COMMUNICATION_RECEIVE_STATUS;
	}

	__hal_icam28_flowcontrol(Comm_Unit.m_UseFlowControl) ;

	UartStauts.m_TxReady = 0;
	UartStauts.m_CardInsert = 0 ;
	TimeOut = 0 ;
	while(1)
	{
		if (TimeOut >= 10000 )
		{
			break;
		}
		TimeOut++ ;
		ret = HAL_ICAM28_GetUartStatue(&UartStauts);
		if (!UartStauts.m_CardInsert)
		{
			gx_printf(" send & rece UartStauts. CardInsert %d  \n",UartStauts.m_CardInsert);
			TimeOut = 10000 ;
			break;
		}

		if ( !UartStauts.m_TxReady )
		{
			continue ;
		}
		else
		{
			break;
		}
	}

	if (TimeOut < 10000)
	{
		ICAM_SET_UART_DATA(gx28_icam_reg, Comm_Unit.m_unit_send.m_buff[0]);
	}
	else
	{
		gx_printf(" send & rece txready %d  \n",UartStauts.m_TxReady);
		return -1;
	}
	return 0;
}

int gx28_icam_receive(int id ,GxIcamProperty_Receive *pRece)
{
	int  ret = 0;

	if ( pRece->m_n_rece > MAX_ICAM_COMMUNICATION_BUFF_SIZE )
	{
		return -1 ;
	}

	Comm_Unit.m_unit_send.m_size  = 0 ;
	Comm_Unit.m_unit_send.m_index = 0 ;

	Comm_Unit.m_unit_rece.m_size  = pRece->m_n_rece ;
	Comm_Unit.m_unit_rece.m_index = 0 ;

	Comm_Unit.m_unit_rece.m_p_user_buff = pRece->m_p_rece ;

	Comm_Unit.m_work_status       = COMMUNICATION_RECEIVE_STATUS ;

	Comm_Unit.m_UseFlowControl    = pRece->m_flow_control ;
	Comm_Unit.m_UseNullFilter     = pRece->m_null_filter  ;

     /*test_printf("CORE_ICAM28_Receive %d,%d, 0x%x ,%d \n",Comm_Unit.m_UseNullFilter,Comm_Unit.m_UseFlowControl,						(unsigned int)Comm_Unit.m_unit_rece.m_p_user_buff,Comm_Unit.m_unit_rece.m_size);*/


	ret = __hal_icam28_flowcontrol(Comm_Unit.m_UseFlowControl) ;


	return ret;
}

int gx28_icam_card_insert_remove(int id,unsigned char bRemove)
{
	int ret = 0 ;

	if (_icam_reg_ops && _icam_reg_ops->card_insert_remove)
	{
		ret = _icam_reg_ops->card_insert_remove(id,bRemove);
	}

	if (bRemove)
	{
		__hal_icam28_interrupt_disable();

		UartCtrlStat = 0 ;


		ICAM28_Config_Info.m_vcc_value = 0;
	//	memset ((char *)&ICAM28_Config_Info, 0x0, sizeof(HAL_ICAM28_Config_t));

	//  20151026 ,lawrence 
	//	modify by resaen: 3v-5v flag m_vcc_level also set to 0, if change to 3v , remove and insert card ,the 
	//	change to 5v , but it correct is 3v . so it can not memset. 

		Comm_Unit.m_read_event   = 0 ;
		Comm_Unit.m_write_event  = 0 ;

	    //error_printf("clear all \n");

		__hal_icam28_interrupt_enable();
	}

	return 0;
}

int gx28_icam_set_controlword(int id,GxIcamProperty_ControlWord *p_ControlWord)
{
	int ret =0;

	unsigned int op_mode  ;
	unsigned int op_value ;
	unsigned int PidIndex ;

	ret |= HAL_ICAM28_SetFunctionalData(p_ControlWord->m_cw_data);

	op_mode = HAL_COMMAND_MODE_SELF_SETTING ;

	op_value = p_ControlWord->m_command_value ;

	if ( p_ControlWord->m_cw_time == 1 )
	{
		op_value |= 0x1<<31 ;
	}
	else if ( p_ControlWord->m_cw_time == 6 )
	{
		PidIndex = (p_ControlWord->m_desc_id<<1)|(p_ControlWord->m_even_odd == TYPE_ICAM28_POLARITY_ODD);
		op_value |= (PidIndex<<8)  ;
	}
	else if ( p_ControlWord->m_cw_time == 0xffff )
	{
		op_mode = HAL_COMMAND_MODE_BY_PASS ;

		ret = HAL_ICAM28_SetFunctionalCWCommand(op_mode,op_value) ;

		ret |= HAL_ICAM28_WaitICAMAcceptAcpuCommandStatue(0);

		if (ret != 0 )
		{
			gx_printf("gx28_icam_set_controlword error F 0x%x \n",ret);
		}
		return ret ;
	}
	else
	{

	}

	ret |= HAL_ICAM28_SetCommand_WaitOK(op_mode,op_value);

	if (ret != 0 )
	{
		gx_printf("gx28_icam_set_controlword error S 0x%x \n",ret);
	}
	return ret ;
}

int gx28_icam_set_config(int id,GxIcamProperty_ConfigInfo *p_Config)
{
	ICAM28_Config_Info.m_reset_card_time = p_Config->m_reset_card_time ;

	/*for future*/
#ifdef ICAM_ENABLE_INTTERUPT_SEEK_INTERRUPT	
	if ( p_Config->m_icam_frequency == 0x1234567 && p_Config->m_sc_clock_frequency == 0x7654321 )
	{
		if ( p_Config->m_icam_buard_rate_reg_value >= 0 )
		{
			__hal_icam28_set_interrupt_seek_interrupt_time(p_Config->m_icam_buard_rate_reg_value) ;
		}
	}
#endif

	return 0 ;
}

int gx28_icam_get_uartstatue(int id,TYPE_ICAM28_UartStatus_t  *p_uart_status)
{
	TYPE_ICAM28_UartStatus_t  hal_status ;

	memset(&hal_status,0,sizeof(TYPE_ICAM28_UartStatus_t));

	HAL_ICAM28_GetUartStatue(&hal_status);

	memcpy(p_uart_status,&hal_status,sizeof(TYPE_ICAM28_UartStatus_t));

	return 0 ;
}

int gx28_icam_get_uartcommond(int id ,TYPE_ICAM28_UartCommand_t *pCommand)
{
	unsigned int uUartCommand ;
	unsigned int uValue ;
	unsigned char uc4,uc8 ;

	uUartCommand = ICAM_GET_UART_COM_VALUE(gx28_icam_reg);

	uValue = (uUartCommand>>3)&0x3 ;

	if (uValue == 0 ) //c7
	{
		uc4 = (uUartCommand>>5)&0x1 ;
		uc8 = (uUartCommand>>6)&0x1 ;

		if ( uc4 & uc8 )
		{
			pCommand->m_IO_VIA_C7 = 1 ;
			pCommand->m_C4_ON     = 1 ;
			pCommand->m_C8_ON     = 1 ;
		}
		else if (uc4)
		{
			pCommand->m_IO_VIA_C7 = 1 ;
			pCommand->m_C4_ON     = 1 ;
		}
		else if (uc8)
		{
			pCommand->m_IO_VIA_C7 = 1 ;
			pCommand->m_C8_ON     = 1 ;
		}
		else
		{
			pCommand->m_IO_VIA_C7 = 1 ;
		}
	}
	else if (uValue == 1 ) //c4
	{
		pCommand->m_IO_VIA_C4 = 1 ;
	}
	else if (uValue == 2 ) //c8
	{
		pCommand->m_IO_VIA_C8 = 1 ;
	}

	return 0 ;
}

int gx28_icam_get_card_event(int id ,unsigned char *p_event)
{
	TYPE_ICAM28_UartStatus_t UartStatus ;
	unsigned int event ;

	event = *p_event ;

	if  ( (event !=  EVENT_CARD_INSERT) && (event != EVENT_CARD_REMOVE))
	{
		gx_printf("get card event is not same %d \n",event);

		return -1 ;
	}

//	gx_printf("############### set event %d \n",event);

	UartStatus.m_CardInsert = 0;
	UartStatus.m_TxReady    = 0;
	UartStatus.m_RxReady    = 0;
	UartStatus.m_OverFlow   = 0;
	UartStatus.m_TxError    = 0;
	UartStatus.m_RxError    = 0;

	HAL_ICAM28_SetUartIntMask(UartStatus);
	HAL_ICAM28_GetUartStatue( &UartStatus);

	if (UartStatus.m_CardInsert == 0)
	{
		*p_event =  EVENT_CARD_REMOVE ;
	//	gx_printf("############### set event remove \n");
	}
	else if ( event == EVENT_CARD_INSERT )
	{
		*p_event =  EVENT_CARD_INSERT;
	//	gx_printf("############### set event insert \n");
	}
	else
	{
		*p_event =  EVENT_CARD_ERROR;
	//	gx_printf("############### set event error \n");
	}

//	gx_printf("############### set event 0x%x , %d \n",(int)p_event,*p_event);

	return 0 ;

}

int gx28_icam_get_event(int id ,unsigned char *p_event)
{
	if ( Comm_Unit.m_read_event != Comm_Unit.m_write_event )
	{
		if ( Comm_Unit.m_buff_event[Comm_Unit.m_read_event] == HAL_EVENT_DETECT_VCC_ON ||
			 Comm_Unit.m_buff_event[Comm_Unit.m_read_event] == HAL_EVENT_DETECT_VCC_OFF )
		{
			/*
			event just get vcc off,vcc on  20160323 
			event_card_insert = vcc_off  event_card_remove = vcc_on
			*/
			if ( Comm_Unit.m_buff_event[Comm_Unit.m_read_event] == HAL_EVENT_DETECT_VCC_OFF )
			{
				*p_event =  EVENT_CARD_INSERT ;

			//	gx_printf("############### get event %d \n",EVENT_CARD_INSERT);
			}
			else if ( Comm_Unit.m_buff_event[Comm_Unit.m_read_event] == HAL_EVENT_DETECT_VCC_ON )
			{
				*p_event =  EVENT_CARD_REMOVE ;

			//	gx_printf("############### get event %d \n",EVENT_CARD_REMOVE);
			}
			else
			{
			//	gx_printf("############### get event error \n");

				return -1 ;
			}
		}
		else if (Comm_Unit.m_buff_event[Comm_Unit.m_read_event] == HAL_EVENT_DATA_RECEIVE_OK )
		{
			*p_event =  EVENT_DATA_RECEIVE_OK;

			gx_copy_to_user((void*)(Comm_Unit.m_unit_rece.m_p_user_buff),
				(const void*)(&Comm_Unit.m_unit_rece.m_buff),Comm_Unit.m_unit_rece.m_size);
		}
		else if (Comm_Unit.m_buff_event[Comm_Unit.m_read_event] == HAL_EVENT_DATA_OVER_FLOW )
		{
			*p_event =  EVENT_DATA_OVER_FLOW;
		}
		else if (Comm_Unit.m_buff_event[Comm_Unit.m_read_event] == HAL_EVENT_DATA_BAD_PARITY )
		{
			*p_event =  EVENT_DATA_BAD_PARITY;
		}
		else
		{
			return -1 ;
		}

		Comm_Unit.m_read_event = (Comm_Unit.m_read_event + 1) % MAX_ICAM_EVENT_BUFF_SIZE;

		return 0 ;
	}
	else
	{
		return -1 ;
	}
}

int gx28_icam_get_chipproperties(int id ,GxIcamProperty_ChipProperties *p_Properties)
{
	int ret = 0 ;

	unsigned int ID_H,ID_L;

	ID_H = ICAM_GET_PUBLIC_ID_HIGH(gx28_icam_reg);
	ID_L = ICAM_GET_PUBLIC_ID_LOW(gx28_icam_reg);

	p_Properties->m_chip_id[0] = ID_H ;
	p_Properties->m_chip_id[1] = ID_L ;

	p_Properties->m_icam_version = ICAM_GET_VERSION(gx28_icam_reg);

	ret |= HAL_ICAM28_GetHashKey(p_Properties->m_hash_key);

	return ret ;
}

int gx28_icam_get_responsetochallenge(int id ,GxIcamProperty_ResponseToChallenge *p_ResponseToChallenge)
{
	int ret = 0;
	int i ;
	unsigned int cw_mode ;
	int default_time = 0 ;
	int first_time = 1 ;

	default_time = p_ResponseToChallenge->m_default_time ;

	for(i=0;i<default_time;i++)
	{
		if (first_time)
		{
			first_time = 0 ;
			cw_mode = HAL_COMMAND_MODE_RESTART ;
		}
		else
		{
			cw_mode = HAL_COMMAND_MODE_NORMAL ;
		}

		ret |= HAL_ICAM28_SetDefaultData();

		ret |= HAL_ICAM28_SetCommand_WaitOK(cw_mode,0);

		if ( ret != 0 )
			return -1 ;
	}

	if ( p_ResponseToChallenge->m_user_enable )
	{
		if (first_time)
		{
			first_time = 0 ;
			cw_mode = HAL_COMMAND_MODE_RESTART ;
		}
		else
		{
			cw_mode = HAL_COMMAND_MODE_NORMAL ;
		}

		ret |= HAL_ICAM28_SetFunctionalData(p_ResponseToChallenge->m_user_buff);

		ret |= HAL_ICAM28_SetCommand_WaitOK(cw_mode,0);

		if ( ret != 0 )
		{
			return -1 ;
		}
	}

	cw_mode = HAL_COMMAND_MODE_NONCE ;

	ret |= HAL_ICAM28_SetFunctionalData(p_ResponseToChallenge->m_nonce);

	ret |= HAL_ICAM28_SetCommand_GetData_WaitOK(cw_mode,0,p_ResponseToChallenge->m_responce);

	return ret ;

}
int gx28_icam_get_encryptdata(int id ,GxIcamProperty_EncryptData *p_EncryptData)
{
	int ret = 0;
	int i ;
	unsigned int cw_mode ;
	int default_time = 0 ;
	int first_time = 1 ;

	default_time = p_EncryptData->m_default_time ;

	for(i=0;i<default_time;i++)
	{
		if (first_time)
		{
			first_time = 0 ;
			cw_mode = HAL_COMMAND_MODE_RESTART ;
		}
		else
		{
			cw_mode = HAL_COMMAND_MODE_NORMAL ;
		}

		ret |= HAL_ICAM28_SetDefaultData();

		ret |= HAL_ICAM28_SetCommand_WaitOK(cw_mode,0);

		if ( ret != 0 )
		{
			return -1 ;
		}
	}

	cw_mode = HAL_COMMAND_MODE_NONCE ;

	ret |= HAL_ICAM28_SetFunctionalData(p_EncryptData->m_nonce);

	ret |= HAL_ICAM28_SetCommand_GetData_WaitOK(cw_mode,0,p_EncryptData->m_responce);

	return ret ;
}

int gx28_icam_get_read_register(int id ,unsigned int register_offset , unsigned int *pData)
{
	unsigned int value ;

	if ( register_offset % 4 )
	{
		return -1 ;
	}

	value = ICAM_READ_REGISTER((unsigned int)(gx28_icam_reg), register_offset);

	*pData = value ;

	return 0 ;
}


int gx28_icam_get_write_register(int id ,unsigned int register_offset , unsigned int value)
{
	if ( register_offset % 4 )
	{
		return -1 ;
	}

	ICAM_WRITE_REGISTER((unsigned int)(gx28_icam_reg), register_offset, value);

	return 0 ;
}

int gx28_icam_get_config(int id ,GxIcamProperty_ConfigInfo *p_Config)
{
	p_Config->m_reset_card_time = ICAM28_Config_Info.m_reset_card_time  ;
	p_Config->m_icam_frequency  = ICAM28_Config_Info.m_icam_frequency  ;
	p_Config->m_sc_clock_frequency = ICAM28_Config_Info.m_clock ;
	p_Config->m_icam_buard_rate_reg_value = ICAM28_Config_Info.m_icam_buard_rate_reg_value ;
	p_Config->m_buard_rate_factor  = ICAM28_Config_Info.m_icam_buard_rate_factory ;

	return 0 ;
}


#define GX_OTP_CON_REG         (*(volatile unsigned int*)0xa0F80080)  //OTP control register
#define GX_OTP_CFG_REG         (*(volatile unsigned int*)0xa0F80084)  //OTP config register
#define GX_OTP_STA_REG         (*(volatile unsigned int*)0xa0F80088)  //OTP state register

#define GX_OTP_WAIT_REG_DELAY  (8000000)

int _gx_otp_wait_reg_delay(unsigned int bit, unsigned int wait_flag)
{
	volatile unsigned int delay ;

	delay = GX_OTP_WAIT_REG_DELAY ;

	while(1)
	{
		if (  (GX_OTP_STA_REG & (0x1<<bit)) == wait_flag )
			break;

		if (delay <= 0 )
		{
		//	error_printf("delay timeout \n");
			return -1 ;
		}

		delay -- ;
//		printf("delay %d \n",delay);
	}
	return 0 ;
}

int gx28_icam_get_read_otp(int id ,unsigned int start_addr ,int num ,unsigned char *data)
{
	int i;
	unsigned int otp_con_reg;
	start_addr  = start_addr << 3;

	//while(!(GX_OTP_STA_REG & (1<<10))); //check if OTP Controller is ready
	if ( _gx_otp_wait_reg_delay(10,(1<<10)) < 0 )
	{
	//	error_printf("otp read bit 10 delay failer \n");
		return -1 ;
	}

	otp_con_reg = 0x43000000 | (0x1fff & start_addr);  //start address  
	for (i = 0; i < num; i++)
	{
		//while(GX_OTP_STA_REG & 0x100); //check if OTP Controller is busy now; 1 for busy
		if ( _gx_otp_wait_reg_delay(8,0) < 0 )
		{
		//	error_printf("otp read bit 8 delay failer \n");
			return -1 ;
		}

		GX_OTP_CON_REG = otp_con_reg | (0x1 << 14); //set READEN
		GX_OTP_CON_REG = otp_con_reg;         //clear READEN

		//while(!(GX_OTP_STA_REG & 0x200)); //chekc if OTP data is ready
		if ( _gx_otp_wait_reg_delay(9,(1<<9)) < 0 )
		{
		//	error_printf("otp read bit 9 delay failer \n");
			return -1 ;
		}

		data[i] = (unsigned char)(GX_OTP_STA_REG & 0xFF);
		otp_con_reg = otp_con_reg + 8;
	}

	return 0 ;
}
int gx28_icam_get_write_otp(int id ,unsigned int start_addr ,int num , unsigned char *data)
{
	int i;
	unsigned int otp_con_reg;

	start_addr  = start_addr << 3;

	//while(!(GX_OTP_STA_REG & (1<<10))); //check if OTP Controller is ready
	if ( _gx_otp_wait_reg_delay(10,(1<<10)) < 0 )
	{
	//	error_printf("otp write bit 10 delay failer \n");
		return -1 ;
	}

	otp_con_reg = 0x43000000 | (0x1fff & start_addr);  //start address  
	for (i = 0; i < num ; i++)
	{
		otp_con_reg &= ~(0xff<<16);
		otp_con_reg |= data[i] << 16;

		//while(GX_OTP_STA_REG & 0x100); //check if OTP Controller is busy now; 1 for busy
		if ( _gx_otp_wait_reg_delay(8,0) < 0 )
		{
		//	error_printf("otp write bit 8 delay failer \n");
			return -1 ;
		}

		GX_OTP_CON_REG = otp_con_reg | (0x1 << 15); //set BURN
		GX_OTP_CON_REG = otp_con_reg;         //clear READEN
		otp_con_reg = otp_con_reg + 8;
	}

	return 0 ;
}

#ifdef ICAM_ENABLE_INTTERUPT_SEEK_INTERRUPT

static inline int gx28_icam_proc_other_interrupt(unsigned int IntType,int (*callback)(unsigned int event,void *priv),void *priv)
{
	TYPE_ICAM28_UartStatus_t IntBits;
	int item_IntType ;
	int app_event ;
	unsigned int sys_event ;

	switch (IntType)
	{
		case ICAM_INT_TYPE_DETECT:
		{
			if ( ICAM28_Config_Info.m_vcc_value )
			{
				item_IntType = ICAM_INT_TYPE_VCCON ;
			}
			else
			{
				item_IntType = ICAM_INT_TYPE_VCCOFF ;
			}

			IntBits.m_CardInsert = 1;
			IntBits.m_TxReady    = 1;
			IntBits.m_RxReady    = 1;
			IntBits.m_OverFlow   = 1;
			IntBits.m_TxError    = 1;
			IntBits.m_RxError    = 1;

			HAL_ICAM28_SetUartIntMask(IntBits);
			ICAM_SET_VCC_INACTIVE(gx28_icam_reg, UartCtrlStat);

			ICAM28_Config_Info.m_vcc_value = 0;

			if ( item_IntType == ICAM_INT_TYPE_VCCON )
			{
				app_event = HAL_EVENT_DETECT_VCC_ON ;
				sys_event = EVENT_ICAM_CARD_INSERT  ;
			}
			else // ICAM_INT_TYPE_VCCOFF:
			{
				app_event = HAL_EVENT_DETECT_VCC_OFF ;
				sys_event = EVENT_ICAM_CARD_REMOVE   ;
			}
		}
		break;

		case ICAM_INT_TYPE_OVERFLOW:
		{
			app_event = HAL_EVENT_DATA_OVER_FLOW ;
			sys_event = EVENT_ICAM_DATA_OVERFLOW ;
		}
	    break;

	case ICAM_INT_TYPE_PARITY_ERROR:
		{
			app_event = HAL_EVENT_DATA_BAD_PARITY ;
			sys_event = EVENT_ICAM_DATA_PARITY_ERROR ;
		}
	    break;

	default:
		return -1 ;
	}

	Comm_Unit.m_buff_event[Comm_Unit.m_write_event] = app_event    ;
	Comm_Unit.m_write_event = (Comm_Unit.m_write_event + 1) % MAX_ICAM_EVENT_BUFF_SIZE;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
	if (callback)
		callback(sys_event,priv);
#endif

	return 0 ;
}

static inline int gx28_icam_proc_send_data(int (*callback)(unsigned int event,void *priv),void *priv)
{
	unsigned char Data ;

	if ((Comm_Unit.m_work_status != COMMUNICATION_TRANSFER_STATUS) ||
	    (Comm_Unit.m_unit_send.m_index >= Comm_Unit.m_unit_send.m_size))
	{
		return -1 ;
	}

	Data = Comm_Unit.m_unit_send.m_buff[Comm_Unit.m_unit_send.m_index];
	if (Comm_Unit.m_unit_send.m_index+1 == Comm_Unit.m_unit_send.m_size)
	{
		Comm_Unit.m_work_status = COMMUNICATION_RECEIVE_STATUS;
		if (Comm_Unit.m_UseFlowControl)
		{
			flowc_reset(gx28_icam_reg, UartCtrlStat);
		}

		ICAM_SET_UART_DATA(gx28_icam_reg, Data);

		return 0 ;
	}
	else
	{
		Comm_Unit.m_unit_send.m_index++;

		ICAM_SET_UART_DATA(gx28_icam_reg, Data);

		return (Comm_Unit.m_unit_send.m_size - Comm_Unit.m_unit_send.m_index) ;
	}

	return 0 ;
}

static inline int gx28_icam_proc_rece_data(unsigned char Data,int (*callback)(unsigned int event,void *priv),void *priv)
{
//	gx_printf("while rece :  %d ,0x%x \n",IntType,Data);

	if ( (Comm_Unit.m_work_status != COMMUNICATION_RECEIVE_STATUS) ||
		 ( Comm_Unit.m_unit_rece.m_index >= Comm_Unit.m_unit_rece.m_size))
	{
		//	gx_printf("error rece mode 0x%x ,%d , %d ,%d , fc %d\n", Data,Comm_Unit.m_work_status,
		//			Comm_Unit.m_unit_rece.m_index,Comm_Unit.m_unit_rece.m_size,Comm_Unit.m_UseFlowControl);
		//	error_printf("e  %x \n", Data);
		return -1 ;
	}

#ifdef DEBUG_COMMUNITION_ISR_RECE
	gx_printf("r  %x \n", Data);
#endif

	Comm_Unit.m_unit_rece.m_buff[Comm_Unit.m_unit_rece.m_index++] = Data ;

	if (Comm_Unit.m_unit_rece.m_index == Comm_Unit.m_unit_rece.m_size)
	{
		if (Comm_Unit.m_UseNullFilter && Data == 0x60)
		{
			Comm_Unit.m_unit_rece.m_index--;
					//test_printf("throw 0x60\n");
			if (Comm_Unit.m_UseFlowControl)
			{
				flowc_clear(gx28_icam_reg, UartCtrlStat);
				flowc_reset(gx28_icam_reg, UartCtrlStat);
			}

			return 0 ;
		}
		else
		{
			Comm_Unit.m_buff_event[Comm_Unit.m_write_event] = HAL_EVENT_DATA_RECEIVE_OK    ;
			Comm_Unit.m_write_event=(Comm_Unit.m_write_event+1) % MAX_ICAM_EVENT_BUFF_SIZE ;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
			if (callback)
				callback(EVENT_ICAM_DATA_RECE,priv);
#endif
			/*
			test_printf("rece ok %d,%d \n",Comm_Unit.m_read_event,Comm_Unit.m_write_event);
			for(i=0 ;i<Comm_Unit.m_unit_rece.m_size;i++)
			{
				test_printf("0x%x  ",Comm_Unit.m_unit_rece.m_p_user_buff[0]);
			}
			test_printf("\n");
			*/

			return 0 ;
		}
	}
	else
	{
		if ( Comm_Unit.m_UseFlowControl )
		{
			flowc_clear(gx28_icam_reg, UartCtrlStat);
			flowc_reset(gx28_icam_reg, UartCtrlStat);
		}

		return ( Comm_Unit.m_unit_rece.m_size - Comm_Unit.m_unit_rece.m_index );
	}

	return 0 ;
}

int gx28_icam_interrupt(int id,int (*callback)(unsigned int event,void *priv),void *priv)
{
	unsigned char Data = 0 ;
	int      ret  = 0 ;
	unsigned int IntType ;

	int left ;
	volatile int j = 0 ;
	unsigned int waittime ;

#ifndef ICAM_CLOSE_AUTO_SETTING_SEEK_INTTERUPT_TIME	
	waittime = __hal_icam28_get_interrupt_seek_interrupt_time();
	if (waittime >= ICAM_MAX_INTTERUPT_SEEK_INTERRUPT_TIME)
	{
		waittime = ICAM_DEFAULT_INTTERUPT_SEEK_INTERRUPT_TIME ;
	}
#else
	waittime = ICAM_DEFAULT_INTTERUPT_SEEK_INTERRUPT_TIME ;
#endif

	j =  0 ;

	while(1)
	{
		if (j>waittime)
		{
			//if (i<=0)
			//	gx_printf("exit interrup %d, %d,waittime %d  \n",i,left,waittime);
			return 0 ;
		}
		j++ ;

		ret = HAL_ICAM28_GetSmartCardInterrupt(&IntType,&Data);
		if ( ret != 0 )
		{
		//	gx_printf("icam interrutp is empty !!!!!!!!!!!!!!!!!!!!!!!\n");
			continue ;
		}

		if ( ( ICAM_INT_TYPE_RECEIVER == IntType) || ( ICAM_INT_TYPE_TRANSMIT == IntType) )
		{
			if ( ICAM_INT_TYPE_RECEIVER == IntType )
			{
				left = gx28_icam_proc_rece_data(Data,callback,priv);
			}
			else
			{
				left = gx28_icam_proc_send_data(callback,priv);
			}

			if (left <=0 )
				return 0 ;
		}
		else
		{
			gx28_icam_proc_other_interrupt(IntType,callback,priv);

			return 0 ;
		}
	}

	return 0 ;
}

#else

int gx28_icam_interrupt(int id,int (*callback)(unsigned int event,void *priv),void *priv)
{
	unsigned char Data = 0 ;
	TYPE_ICAM28_UartStatus_t IntBits;
	int      ret  = 0 ;
	unsigned int IntType ;

	ret = HAL_ICAM28_GetSmartCardInterrupt(&IntType,&Data);
	if ( ret != 0 )
	{
		return -1 ;
	}

	if (IntType == ICAM_INT_TYPE_DETECT)
	{
		if ( ICAM28_Config_Info.m_vcc_value )
		{
			IntType = ICAM_INT_TYPE_VCCON ;
		}
		else
		{
			IntType = ICAM_INT_TYPE_VCCOFF ;
		}

		IntBits.m_CardInsert = 1;
		IntBits.m_TxReady    = 1;
		IntBits.m_RxReady    = 1;
		IntBits.m_OverFlow   = 1;
		IntBits.m_TxError    = 1;
		IntBits.m_RxError    = 1;

		HAL_ICAM28_SetUartIntMask(IntBits);
		ICAM_SET_VCC_INACTIVE(gx28_icam_reg, UartCtrlStat);

		ICAM28_Config_Info.m_vcc_value = 0;

	//	DBGPrint("***************************************** vcc 0\n");
	}

	///////////////////////////////////////////

	switch (IntType)
	{
	case ICAM_INT_TYPE_RECEIVER:
		{
			if ( (Comm_Unit.m_work_status != COMMUNICATION_RECEIVE_STATUS) ||
					( Comm_Unit.m_unit_rece.m_index >= Comm_Unit.m_unit_rece.m_size))
			{
				//	error_printf("error rece mode 0x%x ,%d , %d ,%d \n", Data,DcbUartCtrl.UartBlock.WorkStatus,
				//	DcbUartCtrl.UartBlock.RxBlock.ByteIndex,DcbUartCtrl.UartBlock.RxBlock.TRxByteNum);
				//	error_printf("e  %x \n", Data);

				return -1 ;
			}

			#ifdef DEBUG_COMMUNITION_ISR_RECE
			gx_printf("r  %x \n", Data);
			#endif

			Comm_Unit.m_unit_rece.m_buff[Comm_Unit.m_unit_rece.m_index++] = Data ;

			if (Comm_Unit.m_unit_rece.m_index == Comm_Unit.m_unit_rece.m_size)
			{
				if (Comm_Unit.m_UseNullFilter && Data == 0x60)
				{
					Comm_Unit.m_unit_rece.m_index--;
					//test_printf("throw 0x60\n");
					if (Comm_Unit.m_UseFlowControl)
					{
						flowc_clear(gx28_icam_reg, UartCtrlStat);
						flowc_reset(gx28_icam_reg, UartCtrlStat);
					}

					return 0 ;
				}
				else
				{
					Comm_Unit.m_buff_event[Comm_Unit.m_write_event] = HAL_EVENT_DATA_RECEIVE_OK    ;
					Comm_Unit.m_write_event=(Comm_Unit.m_write_event+1) % MAX_ICAM_EVENT_BUFF_SIZE;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
					if (callback)
						callback(EVENT_ICAM_DATA_RECE,priv);
#endif

					/*
					test_printf("rece ok %d,%d \n",Comm_Unit.m_read_event,Comm_Unit.m_write_event);
					for(i=0 ;i<Comm_Unit.m_unit_rece.m_size;i++)
					{
						test_printf("0x%x  ",Comm_Unit.m_unit_rece.m_p_user_buff[0]);
					}
					test_printf("\n");
					*/
				}
			}
			else
			{
				if ( Comm_Unit.m_UseFlowControl )
				{
					flowc_clear(gx28_icam_reg, UartCtrlStat);
					flowc_reset(gx28_icam_reg, UartCtrlStat);
				}
			}
		}
		break;

	case ICAM_INT_TYPE_TRANSMIT:
		{
			if (Comm_Unit.m_work_status != COMMUNICATION_TRANSFER_STATUS ||
					(Comm_Unit.m_unit_send.m_index >= Comm_Unit.m_unit_send.m_size))
			{
				/*
	       		 error_printf("error read mode 0x%x ,%d , %d ,%d \n", Data,DcbUartCtrl.UartBlock.WorkStatus,
					DcbUartCtrl.UartBlock.TxBlock.ByteIndex,DcbUartCtrl.UartBlock.TxBlock.TRxByteNum);	*/

				return -1 ;
			}

			#ifdef  DEBUG_COMMUNITION_ISR_READ
			gx_printf(" transfer %d: 0x%x\n", Comm_Unit.m_unit_send.m_index,
	                Comm_Unit.m_unit_send.m_buff[Comm_Unit.m_unit_send.m_index]);
			#endif

			Data = Comm_Unit.m_unit_send.m_buff[Comm_Unit.m_unit_send.m_index];
			if (Comm_Unit.m_unit_send.m_index+1 == Comm_Unit.m_unit_send.m_size)
			{
				Comm_Unit.m_work_status = COMMUNICATION_RECEIVE_STATUS;
				if (Comm_Unit.m_UseFlowControl)
				{
					flowc_reset(gx28_icam_reg, UartCtrlStat);
				}
			}
			else
			{
				Comm_Unit.m_unit_send.m_index++;
			}

			ICAM_SET_UART_DATA(gx28_icam_reg, Data);
		}
		break;
	case ICAM_INT_TYPE_VCCON:
		{
			Comm_Unit.m_buff_event[Comm_Unit.m_write_event] = HAL_EVENT_DETECT_VCC_ON    ;
			Comm_Unit.m_write_event = (Comm_Unit.m_write_event + 1) % MAX_ICAM_EVENT_BUFF_SIZE;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
			if (callback)
				callback(EVENT_ICAM_CARD_INSERT,priv);
#endif

		}
	    break;

	case ICAM_INT_TYPE_VCCOFF:
		{
			Comm_Unit.m_buff_event[Comm_Unit.m_write_event] = HAL_EVENT_DETECT_VCC_OFF    ;
			Comm_Unit.m_write_event = (Comm_Unit.m_write_event + 1) % MAX_ICAM_EVENT_BUFF_SIZE;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
			if (callback)
				callback(EVENT_ICAM_CARD_REMOVE,priv);
#endif

		}
	    break;

	case ICAM_INT_TYPE_OVERFLOW:
		{
			Comm_Unit.m_buff_event[Comm_Unit.m_write_event] = HAL_EVENT_DATA_OVER_FLOW    ;
			Comm_Unit.m_write_event = (Comm_Unit.m_write_event + 1) % MAX_ICAM_EVENT_BUFF_SIZE;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
			if (callback)
				callback(EVENT_ICAM_DATA_OVERFLOW,priv);
#endif
		}
	    break;

	case ICAM_INT_TYPE_PARITY_ERROR:
		{
			Comm_Unit.m_buff_event[Comm_Unit.m_write_event] = HAL_EVENT_DATA_BAD_PARITY    ;
			Comm_Unit.m_write_event = (Comm_Unit.m_write_event + 1) % MAX_ICAM_EVENT_BUFF_SIZE;

#ifdef ENABLE_INTTERUPT_SEND_EVENT
			if (callback)
				callback(EVENT_ICAM_DATA_PARITY_ERROR,priv);
#endif

		}
	    break;

	default:
		break;
	//	CAASSERT(0);
	}

	return 0 ;
}
#endif

