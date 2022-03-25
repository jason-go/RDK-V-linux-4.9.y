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


#ifndef  __GX_DESCRAMBLER_HAL_H_201506121955__
#define  __GX_DESCRAMBLER_HAL_H_201506121955__

#include "avcore.h"
#include "gxav_descrambler_propertytypes.h"

#ifdef __cplusplus
extern "C" {
#endif


struct descrambler_ops {
	int (*init)(void);
	int (*cleanup)(void);
	int (*open)(int id);
	int (*close)(int id);
	int (*free)(int id ,int desc_id);
	int (*link)(int id ,int slot_id ,int desc_id,int valid_flag);
	int (*user_alloc)(int id ,int desc_id);
	int (*default_alloc)(int id ,int *p_desc_id);
	int (*get_status)(int id ,int *p_count,int *p_status);
};


int descrambler_hal_init(struct gxav_module_ops *ops);
int descrambler_hal_cleanup(void);
int descrambler_hal_open(int id);
int descrambler_hal_close(int id);
int descrambler_hal_free(int id ,int desc_id);
int decrambler_hal_link(int id ,int slot_id ,int desc_id,int valid_flag);
int descrambler_hal_user_alloc(int id ,int desc_id);
int descrambler_hal_default_alloc(int id ,int *p_desc_id);
int descrambler_hal_get_status(int id ,int *p_count,int *p_status);


#ifdef __cplusplus
}
#endif


#endif

