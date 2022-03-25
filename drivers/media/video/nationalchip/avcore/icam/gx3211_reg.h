/*****************************************************************************
*                          CONFIDENTIAL                             
*        Hangzhou GuoXin Science and Technology Co., Ltd.            
*                      (C)2007, All right reserved
******************************************************************************
* Purpose   :   
* Release History:
  VERSION       Date              AUTHOR         Description
  0.1           2014.10.12                  creation
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef _GX3211_ICAM_REG_H_201506120915_
#define _GX3211_ICAM_REG_H_201506120915_

/* Includes */
#include "gxav_bitops.h"

#include "gx3201_demux_regs.h"


/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif



#define  ENABLE_INTTERUPT_SEND_EVENT

//#define  ICAM_ENABLE_INTTERUPT_SEEK_INTERRUPT

//#define  ICAM_CLOSE_AUTO_SETTING_SEEK_INTTERUPT_TIME   // if defeine ,then use default time 


#define SMART_CARD_FREQUENCY      (27000000)

#define ICAM_BASE_ADDR            (0x00f00000)

#define IEMM_DEMUX_REG_ADDR_BASE       (0x04000000 + 0x3720)  //    0x1b00 demux base
#define IEMM_DEMUX_REG_ADDR_OFFSET     (0x3720)   //  (0x04000000 + 0x3720)  //    0x1b00 demux base

#define SMART_CARD_REG_ADDR       (0x00208000)           // 


/* 
------------------------------------------------------------------------------
                     demux emm reg   
------------------------------------------------------------------------------
*/

#define bDEMUX_ICAM_TS_IN_SEL             (1)
#define bDEMUX_ICAM_TS_OUT_SEL            (0)

/* Icam rCAM_CA_INT_STAT*/
#define CLEAR_EMM_BUF_BYTE                (0x10)
#define bDEMUX_ICAM_NEXTADDR              (0)
#define mDEMUX_ICAM_NEXTADDR              (0xf<<bDEMUX_ICAM_NEXTADDR)


/* Icam rCAM_CA_INT*/
#define bDEMUX_ICAM_MASK_RESET            (7)
#define bDEMUX_ICAM_EMM_OVERFLOW          (2)
#define bDEMUX_ICAM_EMM_ARRIVED           (1)

/* Icam rCAM_EMM_PID */
#define bDEMUX_ICAM_EMM_PID               (0)
#define bDEMUX_ICAM_TID_SEL               (14)
#define bDEMUX_ICAM_FILTER_EN             (15)

#define mDEMUX_ICAM_EMM_PID               (0x1fff<<bDEMUX_ICAM_EMM_PID)


#define DEMUX_ICAM_SET_DEMUX1_ICAM_MODE(base)                                 \
do                                                                            \
{                                                                             \
    REG_SET_BIT(&(base->rCAM_EMM_INOUTPUT_CTRL), bDEMUX_ICAM_TS_IN_SEL);      \
    REG_SET_BIT(&(base->rCAM_EMM_INOUTPUT_CTRL), bDEMUX_ICAM_TS_OUT_SEL);     \
}while(0);

#define DEMUX_ICAM_SET_DEMUX2_ICAM_MODE(base)                                 \
do                                                                            \
{                                                                             \
    REG_CLR_BIT(&(base->rCAM_EMM_INOUTPUT_CTRL), bDEMUX_ICAM_TS_IN_SEL);      \
    REG_CLR_BIT(&(base->rCAM_EMM_INOUTPUT_CTRL), bDEMUX_ICAM_TS_OUT_SEL);     \
}while(0);


#define DEMUX_ICAM_SET_EMM_START_ADDR(base, value)                            \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_EMM_BUFFER_ADDR),  value);                       \
}while(0);


#define DEMUX_ICAM_SET_FREQ(base, value)                                      \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rSYNC_BUFFER_FREQ),  value);                          \
}while(0);


#define DEMUX_ICAM_OVERFLOWMASK_ARRIVEDMASK(base)                             \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_CA_INT), 0);                                     \
}while(0);

#define DEMUX_ICAM_OVERFLOWMASK_ARRIVEDUNMASK(base)                           \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_CA_INT), 1<<bDEMUX_ICAM_EMM_ARRIVED);            \
}while(0);

#define DEMUX_ICAM_OVERFLOWUNMASK_ARRIVEDMASK(base)                           \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_CA_INT), 1<<bDEMUX_ICAM_EMM_OVERFLOW);           \
}while(0);

#define DEMUX_ICAM_OVERFLOWUNMASK_ARRIVEDUNMASK(base)                         \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_CA_INT),                                         \
        (1<<bDEMUX_ICAM_EMM_OVERFLOW)|(1<<bDEMUX_ICAM_EMM_ARRIVED));          \
}while(0);

#define DEMUX_ICAM_TID_FILTER_IGNORE(base, index)                             \
do                                                                            \
{                                                                             \
    REG_CLR_BIT(&(base->rCAM_EMM_TID_MODE),  index);                          \
    REG_CLR_BIT(&(base->rCAM_EMM_TID_MODE),  index+16);                       \
}while(0);

#define DEMUX_ICAM_TID_WITHOUT_CA_ADDR(base, index)                           \
do                                                                            \
{                                                                             \
    REG_SET_BIT(&(base->rCAM_EMM_TID_MODE),  index);                          \
    REG_CLR_BIT(&(base->rCAM_EMM_TID_MODE),  index+16);                       \
}while(0);

#define DEMUX_ICAM_TID_WITH_CA_ADDR(base, index)                              \
do                                                                            \
{                                                                             \
    REG_CLR_BIT(&(base->rCAM_EMM_TID_MODE),  index);                          \
    REG_SET_BIT(&(base->rCAM_EMM_TID_MODE),  index+16);                       \
}while(0);

#define DEMUX_ICAM_TID_FILTER_RESERVE(base, index)                            \
do                                                                            \
{                                                                             \
    REG_SET_BIT(&(base->rCAM_EMM_TID_MODE),  index);                          \
    REG_SET_BIT(&(base->rCAM_EMM_TID_MODE),  index+16);                       \
}while(0);

#define DEMUX_ICAM_SET_CA_ADDR_VALUE(base, index, value)                      \
do                                                                            \
{                                                                             \
    if (index<3)                                                              \
        REG_SET_VAL(&(base->rCAM_EMM_DATA_ID_1_3[index]),  value);            \
    else                                                                      \
        REG_SET_VAL(&(base->rCAM_EMM_DATA_ID_4_8[index-3]),  value);          \
}while(0);

#define DEMUX_ICAM_SET_CA_ADDR_MASK(base, index, value)                       \
do                                                                            \
{                                                                             \
    if (index<3)                                                              \
        REG_SET_VAL(&(base->rCAM_EMM_MASK_ID_1_3[index]),  value);            \
    else                                                                      \
        REG_SET_VAL(&(base->rCAM_EMM_MASK_ID_4_8[index-3]),  value);          \
}while(0);

#define DEMUX_ICAM_SET_CA_CTRL_ID(base, value)                                \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_EMM_CTRL_ID), value);                            \
}while(0);

#define DEMUX_ICAM_SET_EMM_PID(base, value)                                   \
do                                                                            \
{                                                                             \
    REG_SET_FIELD(&(base->rCAM_EMM_PID), mDEMUX_ICAM_EMM_PID, value,bDEMUX_ICAM_EMM_PID);\
}while(0);

#define DEMUX_ICAM_PID_FILTER_ENABLE(base)                                    \
do                                                                            \
{                                                                             \
    REG_SET_BIT(&(base->rCAM_EMM_PID),  bDEMUX_ICAM_FILTER_EN);               \
}while(0);

#define DEMUX_ICAM_PID_FILTER_DISABLE(base)                                   \
do                                                                            \
{                                                                             \
    REG_CLR_BIT(&(base->rCAM_EMM_PID),  bDEMUX_ICAM_FILTER_EN);               \
}while(0);

#define DEMUX_ICAM_USE_TID(base)                                              \
do                                                                            \
{                                                                             \
    REG_SET_BIT(&(base->rCAM_EMM_PID),  bDEMUX_ICAM_TID_SEL);                 \
}while(0);

#define DEMUX_ICAM_NONUSE_TID(base)                                           \
do                                                                            \
{                                                                             \
    REG_CLR_BIT(&(base->rCAM_EMM_PID),  bDEMUX_ICAM_TID_SEL);                 \
}while(0);


#define DEMUX_ICAM_GET_ARRIVED_BUF_ORDER(base)                                \
	REG_GET_FIELD(&(base->rCAM_CA_INT_STAT), mDEMUX_ICAM_NEXTADDR, bDEMUX_ICAM_NEXTADDR)

#define DEMUX_ICAM_CLEAR_EMM_BUF(base)                                        \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_CA_INT_STAT),  0x10);                            \
}while(0);


#define DEMUX_ICAM_GET_EMM_OVERFLOW(base)                                     \
	REG_GET_BIT(&(base->rCAM_CA_INT), bDEMUX_ICAM_EMM_OVERFLOW)

#define DEMUX_ICAM_CLEAR_OVERFLOW(base)                                       \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_CA_INT), ((1<<bDEMUX_ICAM_MASK_RESET)|(1<<bDEMUX_ICAM_EMM_OVERFLOW)));\
}while(0);

#define DEMUX_ICAM_GET_EMM_ARRIVED(base)                                      \
	REG_GET_BIT(&(base->rCAM_CA_INT), bDEMUX_ICAM_EMM_ARRIVED)

#define DEMUX_ICAM_CLEAR_ARRIVED(base)                                        \
do                                                                            \
{                                                                             \
    REG_SET_VAL(&(base->rCAM_CA_INT), ((1<<bDEMUX_ICAM_MASK_RESET)|(1<<bDEMUX_ICAM_EMM_ARRIVED))); \
}while(0);

#define DEMUX_ICAM_EMM_TID_MODE_ALL_DISABLE(base) \
	REG_SET_FIELD(&(base->rCAM_EMM_PID),(0x3<<bDEMUX_ICAM_TID_SEL),0,bDEMUX_ICAM_TID_SEL)

#define DEMUX_ICAM_EMM_TID_MODE_ALL_ENABLE(base)  \
	REG_SET_FIELD(&(base->rCAM_EMM_PID),(0x3<<bDEMUX_ICAM_TID_SEL),3,bDEMUX_ICAM_TID_SEL)



typedef struct Reg_Iemm_Demux_s    //0x30-00-37-00
{
    unsigned int            rCAM_EMM_TID_MODE          ;//20
    unsigned int            rCAM_EMM_DATA_ID_1_3[3]    ;//24
    unsigned int            rCAM_EMM_MASK_ID_1_3[3]    ;//30
    unsigned int            rCAM_EMM_PID               ;//3c
    unsigned int            rCAM_EMM_TID               ;//40
    unsigned int            rCAM_CA_INT                ;//44
    unsigned int            rCAM_CA_INT_STAT           ;//48
    unsigned int            rCAM_EMM_DATA_ID_4_8[5]    ;//4c
    unsigned int            rCAM_EMM_MASK_ID_4_8[5]    ;//60
    unsigned int            rCAM_EMM_CTRL_ID           ;//74
    unsigned int            rCAM_EMM_BUFFER_ADDR       ;//78
    unsigned int            rCAM_EMM_INOUTPUT_CTRL     ;//7c
    unsigned int            rSYNC_BUFFER_FREQ          ;//80
}Reg_Iemm_Demux_t;


/* 
------------------------------------------------------------------------------
                    smart card  reg
------------------------------------------------------------------------------
*/

#define COLD_RESET                  (0)
#define HOT_RESET                   (1)
#define SCI_EN                      (10)

#define CARD_HOT_RESET(rp)           REG_SET_BIT(&(rp->SCI_CTL1),HOT_RESET)

#define CARD_COLD_RESET(rp)          REG_SET_BIT(&(rp->SCI_CTL1),COLD_RESET)

#define CARD_CLOCK_OUTPUT(rp)        REG_SET_BIT(&(rp->SCI_CTL2),1)


#define CARD_ENABLE_WORK(rp)      REG_SET_BIT(&(rp->SCI_CTL2),SCI_EN)    //(rp->SCI_CTL2 |=  (1 << SCI_EN))
#define CARD_DISABLE_WORK(rp)     REG_CLR_BIT(&(rp->SCI_CTL2),SCI_EN)    //(rp->SCI_CTL2 &= ~(1 << SCI_EN))



#define SET_BAUDRATE(rp,value)       \
	REG_SET_FIELD(                   \
				&(rp->SCI_CTL3),     \
				0xff<<0 ,            \
				value,0)

typedef struct Smart_Regs_s
{
    unsigned int     SCI_CTL1  ;
    unsigned int     SCI_CTL2  ;
    unsigned int     SCI_CTL3  ;
    unsigned int     SCI_STATUS;
    unsigned int     SCI_INTEN ;
    unsigned int     SCI_EGT   ;
    unsigned int     SCI_TGT   ;
    unsigned int     SCI_WDT   ;
    unsigned int     SCI_TWDT  ;
    unsigned int     SCI_DATA  ;
}Smart_Regs_t;



#ifdef __cplusplus
}
#endif

#endif

