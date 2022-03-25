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


#ifndef  __GX_IEMM_HAL_H_201506121827__
#define  __GX_IEMM_HAL_H_201506121827__

#include "avcore.h"
#include "gxav_iemm_propertytypes.h"

#ifdef __cplusplus
extern "C" {
#endif


struct iemm_ops {
	int (*init)(void);
	int (*cleanup)(void);
	int (*open)(int id);
	int (*close)(int id);
	int (*request)(int id,GxIemmProperty_Request *p_Request);
	int (*stop)(int id,GxIemmProperty_Stop *p_Stop);
	int (*run)(int id,GxIemmProperty_Run *p_Run);
	int (*update_read_index)(int id,GxIemmProperty_UpdateReadIndex *p_UpdateReadIndex);
	int (*get_event)(int id ,GxIemmProperty_InterruptEvent *p_InterruptEvent);
	int (*read_data_buff)(int id, GxIemmProperty_ReadDataBuff *p_ReadDataBuff);
	int (*read_data_list)(int id, GxIemmProperty_ReadDataList *p_ReadDataList);
	int (*interrupt)(int id,int (*callback)(unsigned int event,void *priv),void *priv);
};


int iemm_hal_init(struct gxav_module_ops *ops);
int iemm_hal_cleanup(void);
int iemm_hal_open(int id);
int iemm_hal_close(int id);
int iemm_hal_request(int id,GxIemmProperty_Request *p_Request);
int iemm_hal_stop(int id,GxIemmProperty_Stop *p_Stop);
int iemm_hal_run(int id,GxIemmProperty_Run *p_Run);
int iemm_hal_update_read_index(int id,GxIemmProperty_UpdateReadIndex *p_UpdateReadIndex);
int iemm_hal_get_event(int id ,GxIemmProperty_InterruptEvent *p_InterruptEvent);
int iemm_hal_read_data_buff(int id, GxIemmProperty_ReadDataBuff *p_ReadDataBuff);
int iemm_hal_read_data_list(int id, GxIemmProperty_ReadDataList *p_ReadDataList);
int iemm_hal_interrupt(int id ,int (*callback)(unsigned int event,void *priv),void *priv);

#ifdef __cplusplus
}
#endif


#endif

