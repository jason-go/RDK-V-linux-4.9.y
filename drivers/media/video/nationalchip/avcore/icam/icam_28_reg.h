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


#ifndef  __GX_ICAM_28_REG_H_201506120935__
#define  __GX_ICAM_28_REG_H_201506120935__

#include "gxav_bitops.h"

#ifdef __cplusplus
extern "C" {
#endif


/* rCOMMON */
#define bICAM_COMMOND_RESTART       (31)
#define bICAM_COMMOND_PID_INDEX     (8)
#define bICAM_COMMOND_OPCODE        (0)

#define mICAM_COMMOND_PID_INDEX       (0x1F<<bICAM_COMMOND_PID_INDEX)
#define mICAM_COMMOND_OPCODE          (0xFF<<bICAM_COMMOND_OPCODE)

#define CW_OPCODE       (0)//(1)
#define NONCE_OPCODE    (1)//(2)

/* rINTERRUPT  */
#define bICAM_INTC_ILLEGAL_SEQ      (4)
#define bICAM_INTC_ILLEGAL_DATA     (3)
#define bICAM_INTC_ILLEGAL_OTP      (2)
#define bICAM_INTC_ILLEGAL_KTE      (1)
#define bICAM_INTC_UART             (0)

/* rINTERRUPT_MASK  */
#define bICAM_INTC_MASK_ILLEGAL_SEQ      (4)
#define bICAM_INTC_MASK_ILLEGAL_DATA     (3)
#define bICAM_INTC_MASK_ILLEGAL_OTP      (2)
#define bICAM_INTC_MASK_ILLEGAL_KTE      (1)

/* rSTATUS*/
#define bICAM_STATUS_OTP_NDS_MODE       (3)
#define bICAM_STATUS_SER_FAIL           (2)
#define bICAM_STATUS_INVALID_OTP        (1)
#define bICAM_STATUS_BUSY               (0)

/* rUART_DATA */
#define bICAM_UART_DATA        (0)
#define mICAM_UART_DATA        (0xFF<<bICAM_UART_DATA)

/* rUART_CTRL_STAT */
#define bICAM_UART_TX_READY        (0)
#define bICAM_UART_RX_READY        (1)
#define bICAM_UART_INTC_PENDING    (2)
#define bICAM_UART_DIRECTION       (3)
#define bICAM_UART_DETECT          (4)

#define bICAM_UART_SC_RESET                (0)
#define bICAM_UART_CONVENTION_DIRECTON     (1)
#define bICAM_UART_MANUAL_FLOW_CONTROL     (2)
#define bICAM_UART_VCC_ACTIVE              (4)
#define bICAM_UART_VCC_3V_5V               (5)
#define bICAM_UART_AUTO_FLOW_CONTROL       (6)

/* rUART_BAUD_RATE_L */
#define bICAM_UART_BAUD_L        (0)
#define mICAM_UART_BAUD_L        (0xFF<<bICAM_UART_BAUD_L)

/* rUART_BAUD_RATE_H */
#define bICAM_UART_BAUD_H        (0)
#define mICAM_UART_BAUD_H        (0xFF<<bICAM_UART_BAUD_H)

/* rUART_COM  */
#define bICAM_UART_METHOD_CONTROL    (3)
#define bICAM_UART_VALUE_C4          (5)
#define bICAM_UART_VALUE_C8          (6)

#define mICAM_UART_METHOD_CONTROL    (0x3<<bICAM_UART_METHOD_CONTROL)

/* rUART_INT  */
#define bICAM_UART_INT_TRANSMIT        (0)
#define bICAM_UART_INT_RECEIVE         (1)
#define bICAM_UART_INT_DETECT          (2)
#define bICAM_UART_INT_OVERFLOW        (3)
#define bICAM_UART_TRANSMIT_ERROR      (4)
#define bICAM_UART_RECEIVE_ERROR       (5)
#define bICAM_UART_INT_MASK_OR_RESET   (7)

/* rUART_GUARD_TIME */
#define bICAM_UART_GUART_TIME         (0)
#define mICAM_UART_GUART_TIME         (0xFF<<bICAM_UART_GUART_TIME)



#define flowc_clear(base,value) do{               \
	value &= (~(1<<2));                           \
    REG_SET_VAL(&(base->rUART_CTRL_STAT), value); \
}while(0);

#define flowc_reset(base,value) do{               \
	value |= (1<<2);                              \
    REG_SET_VAL(&(base->rUART_CTRL_STAT), value); \
}while(0);


/* rDATA */
#define ICAM_SET_DATA_WORD(base, index, value)          \
do                                                      \
{                                                       \
	REG_SET_VAL(&(base->rDATA[index]), value);          \
}while(0);

#define ICAM_GET_DATA_WORD(base, index)       \
	REG_GET_VAL(&(base->rDATA[index]))

/* rCOMMOND */
#define ICAM_SET_COMMAND_CW_WORD(base, restart, pid)          \
do                                                            \
{                                                             \
	REG_SET_VAL(&(base->rCOMMAND), (restart<<bICAM_COMMOND_RESTART) |\
			(pid<<bICAM_COMMOND_PID_INDEX)| (CW_OPCODE<<bICAM_COMMOND_OPCODE));\
}while(0);

#define ICAM_SET_COMMAND_CW_WORD_2(base, restart, pid)          \
do                                                                      \
{                                                                       \
	REG_SET_VAL(&(base->rCOMMAND), (0x1<<24)|(pid<<bICAM_COMMOND_PID_INDEX)|\
			(CW_OPCODE<<bICAM_COMMOND_OPCODE));           \
}while(0);

#define ICAM_SET_COMMAND_CW_WORD_3(base, value)          \
do                                                       \
{                                                        \
	REG_SET_VAL(&(base->rCOMMAND), value);           \
}while(0);



#define ICAM_SET_COMMAND_NONCE_WORD(base)                                \
do                                                                       \
{                                                                        \
	REG_SET_VAL(&(base->rCOMMAND), (NONCE_OPCODE<<bICAM_COMMOND_OPCODE));\
}while(0);

#define ICAM_SET_COMMAND_OPCODE(base, value)           \
    REG_SET_FIELD(                                     \
          &(base->rCOMMAND),                           \
          mICAM_COMMOND_OPCODE,                        \
          (value & 0xFF) , bICAM_COMMOND_OPCODE)

#define ICAM_SET_COMMAND_PID_INDEX(base, value)           \
    REG_SET_FIELD(                                        \
          &(base->rCOMMAND),                              \
          mICAM_COMMOND_PID_INDEX,                        \
          (value & 0x1F) , bICAM_COMMOND_PID_INDEX)

#define ICAM_SET_COMMAND_RESTART(base)                       \
do                                                           \
{                                                            \
	REG_SET_BIT(&(base->rCOMMAND), bICAM_COMMOND_RESTART);   \
}while(0);

/* rVERSION */
#define ICAM_GET_VERSION(base)           REG_GET_VAL(&(base->rVERSION))

/* rID_H */
#define ICAM_GET_PUBLIC_ID_HIGH(base)    REG_GET_VAL(&(base->rID_H))

/* rID_L */
#define ICAM_GET_PUBLIC_ID_LOW(base)     REG_GET_VAL(&(base->rID_L))

/* rINTERRUPT */
#define ICAM_GET_INT_STATUS(base)        REG_GET_VAL(&(base->rINTERRUPT))

#define ICAM_CLR_ILLEGAL_INT(base, bit)     \
do                                          \
{                                           \
	REG_SET_BIT(&(base->rINTERRUPT), bit);  \
}while(0);

/* rSTATUS */
#define ICAM_GET_OTP_MODE(base)          \
	REG_GET_BIT(&(base->rSTATUS), bICAM_STATUS_OTP_NDS_MODE)

#define ICAM_GET_OTP_SERIAL_FAIL_MODE(base)         \
	REG_GET_BIT(&(base->rSTATUS), bICAM_STATUS_SER_FAIL)

#define ICAM_GET_OTP_FAIL_MODE(base)         \
	REG_GET_BIT(&(base->rSTATUS), bICAM_STATUS_INVALID_OTP)

#define ICAM_GET_ICAM_STATUS(base)         \
	REG_GET_BIT(&(base->rSTATUS), bICAM_STATUS_BUSY)

/* rKT_STATUS */
#define ICAM_GET_KT_STATUS(base, index)          \
	REG_GET_BIT(&(base->rKT_STATUS), index)

/* rUART_DATA */
#define ICAM_SET_UART_DATA(base, value)            \
    REG_SET_FIELD(                                 \
          &(base->rUART_DATA),                     \
          mICAM_UART_DATA,                         \
          (value & 0xFF) ,bICAM_UART_DATA)

#define ICAM_GET_UART_DATA(base)      \
    REG_GET_FIELD(                    \
          &(base->rUART_DATA),        \
          mICAM_UART_DATA,            \
          bICAM_UART_DATA)

/* rUART_CTRL_STAT */

#define ICAM_GET_UART_ALL_STATUE(base)          \
	REG_GET_VAL(&(base->rUART_CTRL_STAT))

#define ICAM_GET_UART_TX_READY(base)          \
	REG_GET_BIT(&(base->rUART_CTRL_STAT), bICAM_UART_TX_READY)

#define ICAM_GET_UART_RX_READY(base)          \
	REG_GET_BIT(&(base->rUART_CTRL_STAT), bICAM_UART_RX_READY)

#define ICAM_GET_UART_INTC_PENDING(base)          \
	REG_GET_BIT(&(base->rUART_CTRL_STAT), bICAM_UART_INTC_PENDING)

#define ICAM_GET_UART_DETECT_OFFLINE(base)          \
	REG_GET_BIT(&(base->rUART_CTRL_STAT), bICAM_UART_DETECT)

#define ICAM_GET_UART_DIRCETION(base)          \
	REG_GET_BIT(&(base->rUART_CTRL_STAT), bICAM_UART_DIRECTION)

#define ICAM_SET_SMARTCARD_CLR(base, value)            \
do                                                     \
{                                                      \
    value &= ~(1<<bICAM_UART_SC_RESET);                \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);      \
}while(0);

#define ICAM_SET_SMARTCARD_SET(base, value)            \
do                                                     \
{                                                      \
    value |= (1<<bICAM_UART_SC_RESET);                 \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);      \
	value &= ~(1<<bICAM_UART_SC_RESET);                \
}while(0);

#define ICAM_SET_UART_CONVENTION_MSB_FIRST(base, value)         \
do                                                              \
{                                                               \
    value &= ~(1<<bICAM_UART_CONVENTION_DIRECTON);              \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_UART_CONVENTION_LSB_FIRST(base, value)         \
do                                                              \
{                                                               \
    value |= (1<<bICAM_UART_CONVENTION_DIRECTON);               \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_MANU_FLOWCONTROL_ACTIVE(base, value)           \
do                                                              \
{                                                               \
    value |= (1<<bICAM_UART_MANUAL_FLOW_CONTROL);               \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_MANU_FLOWCONTROL_INACTIVE(base, value)         \
do                                                              \
{                                                               \
    value &= ~(1<<bICAM_UART_MANUAL_FLOW_CONTROL);              \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_VCC_ACTIVE(base, value)                        \
do                                                              \
{                                                               \
    value |= (1<<bICAM_UART_VCC_ACTIVE);                        \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_VCC_INACTIVE(base, value)                      \
do                                                              \
{                                                               \
    value &= ~(1<<bICAM_UART_VCC_ACTIVE);                       \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_VCC_3V(base, value)                            \
do                                                              \
{                                                               \
    value |= (1<<bICAM_UART_VCC_3V_5V);                         \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_VCC_5V(base, value)                            \
do                                                              \
{                                                               \
    value &= ~(1<<bICAM_UART_VCC_3V_5V);                        \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_AUTO_FLOWCONTROL_ACTIVE(base, value)           \
do                                                              \
{                                                               \
    value |= (1<<bICAM_UART_AUTO_FLOW_CONTROL);                 \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_FLOWCONTROL_NOT_ACTIVE(base, value)            \
do                                                              \
{                                                               \
    value &= ~(1<<bICAM_UART_MANUAL_FLOW_CONTROL);              \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

#define ICAM_SET_FLOWCONTROL_ACTIVE(base, value)                \
do                                                              \
{                                                               \
    value |= (1<<bICAM_UART_MANUAL_FLOW_CONTROL);               \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);


#define ICAM_SET_AUTO_FLOWCONTROL_INACTIVE(base, value)         \
do                                                              \
{                                                               \
    value &= ~(1<<bICAM_UART_AUTO_FLOW_CONTROL);                \
	REG_SET_VAL(&(base->rUART_CTRL_STAT), value);               \
}while(0);

/* rUART_BAUD_RATE_H */
#define ICAM_SET_UART_BAUD_RATE_H(base, value)         \
    REG_SET_FIELD(                                     \
          &(base->rUART_BAUD_RATE_H),                  \
          mICAM_UART_BAUD_H,                           \
          (value & 0xFF) , bICAM_UART_BAUD_H)

/* rUART_BAUD_RATE_L */
#define ICAM_SET_UART_BAUD_RATE_L(base, value)         \
    REG_SET_FIELD(                                     \
          &(base->rUART_BAUD_RATE_L),                  \
          mICAM_UART_BAUD_L,                           \
          (value & 0xFF) , bICAM_UART_BAUD_L)

/* rUART_COM */
#define ICAM_SET_UART_COM_METHOD(base, value)          \
    REG_SET_FIELD(                                     \
          &(base->rUART_COM),                          \
          mICAM_UART_METHOD_CONTROL,                   \
          (value & 0x3) , bICAM_UART_METHOD_CONTROL)

#define ICAM_GET_UART_COM_METHOD(base)        \
    REG_GET_FIELD(                            \
          &(base->rUART_COM),                 \
          mICAM_UART_METHOD_CONTROL,          \
          bICAM_UART_METHOD_CONTROL)

#define ICAM_SET_UART_C4_VALUE(base)                        \
do                                                          \
{                                                           \
	REG_SET_BIT(&(base->rUART_COM), bICAM_UART_VALUE_C4);   \
}while(0);

#define ICAM_SET_UART_C8_VALUE(base)                        \
do                                                          \
{                                                           \
	REG_SET_BIT(&(base->rUART_COM), bICAM_UART_VALUE_C8);   \
}while(0);

#define ICAM_SET_UART_COM_VALUE(base, value)                \
do                                                          \
{                                                           \
	REG_SET_VAL(&(base->rUART_COM), value);                 \
}while(0);


#define ICAM_GET_UART_COM_VALUE(base)  REG_GET_VAL(&(base->rUART_COM))


/* rUART_INT */
#define ICAM_GET_INT_UART_STATUS(base)       REG_GET_VAL(&(base->rUART_INT))

#define ICAM_GET_UART_TX_ERROR(base)          \
	REG_GET_BIT(&(base->rUART_INT), bICAM_UART_TRANSMIT_ERROR)

#define ICAM_GET_UART_RX_ERROR(base)          \
	REG_GET_BIT(&(base->rUART_INT), bICAM_UART_RECEIVE_ERROR)

#define ICAM_GET_UART_OVERFLOW(base)          \
	REG_GET_BIT(&(base->rUART_INT), bICAM_UART_INT_OVERFLOW)

//	REG_SET_BIT(base->rUART_INT, bICAM_UART_INT_MASK_OR_RESET);

#define ICAM_CLR_UART_INT(base, bit)                                            \
do                                                                              \
{                                                                               \
	REG_SET_VAL(&(base->rUART_INT),(1<<bit)|(1<<bICAM_UART_INT_MASK_OR_RESET)); \
}while(0);

//	REG_CLR_BIT(base->rUART_INT, bICAM_UART_INT_MASK_OR_RESET);

#define ICAM_SET_UART_INT_MASK(base, value)            \
do                                                     \
{                                                      \
	REG_SET_VAL(&(base->rUART_INT), value);            \
}while(0);

/* rUART_GUARD_TIME */
#define ICAM_SET_UART_GUARD_TIME(base, value)          \
    REG_SET_FIELD(                                     \
          &(base->rUART_GUARD_TIME),                   \
          mICAM_UART_GUART_TIME,                       \
          (value & 0xFF) , bICAM_UART_GUART_TIME)


#define ICAM_READ_REGISTER(addr, offset)     REG_GET_VAL((addr+offset));


#define ICAM_WRITE_REGISTER(addr, offset,value)        \
do                                                     \
{                                                      \
	REG_SET_VAL((addr+offset), value);                 \
}while(0);


#define ICAM_SET_INTTERRUPT_MASK(base, value)  REG_SET_VAL(&(base->rINTERRUPT_MASK), value)


/* Private types/constants ------------------------------------------------ */
typedef struct Reg_NdsFunction_s{
	unsigned int rDATA[4];//0X0000
	unsigned int rCOMMAND;//0X0010
	unsigned int rVERSION;//0X0014
	unsigned int rID_H;
	unsigned int rID_L;
	unsigned int rINTERRUPT;
	unsigned int rINTERRUPT_MASK;
	unsigned int rSTATUS;
	unsigned int rKT_STATUS;//0X002C
	unsigned int rRESERVED[52];
	unsigned int rUART_DATA;//0X0100
	unsigned int rUART_CTRL_STAT;
	unsigned int rUART_BAUD_RATE_L;
	unsigned int rUART_BAUD_RATE_H;
	unsigned int rUART_COM;
	unsigned int rUART_INT;
	unsigned int rUART_GUARD_TIME;//0X0118
}Reg_NdsFunction_t;




#ifdef __cplusplus
}
#endif


#endif

