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
#ifndef _GX3211_DESCRAMBLER_REG_H_201506122003_
#define _GX3211_DESCRAMBLER_REG_H_201506122003_

/* Includes */
#include "gxav_bitops.h"

#include "gx3201_demux_regs.h"

/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define DESC_DEMUX_REG_ADDR       (0x04000000 + 0x2000)


#define DESC_NUM_PID_CFG_SEL      64    // num of PID_CFG & PID_SEL
#define DESC_NUM_DESCRAMBLER      26    //

#define DESC_BIT_DEMUX_PID_CFG_KEY_ID           12
#define DESC_DEMUX_PID_CFG_KEY_ID               (0x1F << DESC_BIT_DEMUX_PID_CFG_KEY_ID)

#define DESC_BIT_DEMUX_PID_CFG_EVEN_KEY_VALID   16
#define DESC_BIT_DEMUX_PID_CFG_ODD_KEY_VALID    17



struct reg_desc_demux_slot {
	unsigned int pid_cfg;
	unsigned int pid_cfg_l;
	unsigned int pid_sel_h;
	unsigned int pid_sel_l;
};

typedef struct DESC_Demux_Regs_s
{
	unsigned int reverser1[32];//352 -320
	struct reg_desc_demux_slot pid_cfg_sel[DESC_NUM_PID_CFG_SEL];
	unsigned int reverser2[1059];
	unsigned int wen_mask;
}DESC_Demux_Regs_t;




#ifdef __cplusplus
}
#endif

#endif

