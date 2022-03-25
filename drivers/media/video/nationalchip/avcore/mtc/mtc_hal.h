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


#ifndef  __GX_MTC_HAL_H_201506122019__
#define  __GX_MTC_HAL_H_201506122019__

#include "avcore.h"
#include "gxav_mtc_propertytypes.h"

#ifdef __cplusplus
extern "C" {
#endif


struct mtc_ops {
	int (*init)(void);
	int (*cleanup)(void);
	int (*open)(int id);
	int (*close)(int id);
	int (*updatekey)(int id ,GxMTCProperty_M2M_UpdateKey *p_M2M_UpdateKey);
	int (*set_params_execute)(int id ,GxMTCProperty_M2M_SetParamsAndExecute *p_M2M_SetParamsAndExecute);
	int (*get_event)(int id ,GxMTCProperty_M2M_Event *p_M2M_Event);

	int (*set_config)(int id ,GxMTCProperty_Config *p_config);
	int (*runing)(int id ,GxMTCProperty_Run *p_Run);
};


int mtc_hal_init(struct gxav_module_ops *ops);
int mtc_hal_cleanup(void);
int mtc_hal_open(int id);
int mtc_hal_close(int id);
int mtc_hal_updatekey(int id ,GxMTCProperty_M2M_UpdateKey *p_M2M_UpdateKey);
int mtc_hal_set_params_execute(int id ,GxMTCProperty_M2M_SetParamsAndExecute *p_M2M_SetParamsAndExecute);
int mtc_get_event(int id ,GxMTCProperty_M2M_Event *p_M2M_Event);

int mtc_hal_set_config(int id ,GxMTCProperty_Config *p_config);
int mtc_hal_runing(int id ,GxMTCProperty_Run *p_Run);

#ifdef __cplusplus
}
#endif


#endif

