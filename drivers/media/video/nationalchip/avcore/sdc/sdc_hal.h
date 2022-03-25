#ifndef __GX_SDC_HAL_H__
#define __GX_SDC_HAL_H__

#include "avcore.h"

#define SDC_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[SDC][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)

#ifdef GX_SDC_DEBUG
	#define SDC_DBG(fmt, args...)   do{\
		gx_printf("\n[SDC][%s():%d]: ", __func__, __LINE__);\
		gx_printf(fmt, ##args);\
	}while(0)
#else
	#define SDC_DBG(fmt, args...)   ((void)0)
#endif

struct sdc_ops {
    int (*init)(void);
    int (*uninit)(void);
    int (*open)(void* priv);
    int (*close)(void);
    int (*apply)(int channel_id, unsigned int size);
    int (*free)(int channel_id);
    int (*gate_set)(int channel_id, unsigned int empty, unsigned int full);
    int (*gate_get)(int channel_id, unsigned int *empty, unsigned int *full);
    int (*algate_set)(int channel_id, unsigned int almost_empty, unsigned int almost_full);
    int (*algate_get)(int channel_id, unsigned int *almost_empty, unsigned int *almost_full);
    int (*length_set)(int channel_id, unsigned int len, GxAvSdcOPMode mode);
    int (*length_get)(int channel_id, unsigned int *len);
    int (*free_get)(int channel_id, unsigned int *len);
    int (*buffer_reset)(int channel_id);
    int (*buffer_flush)(int channel_id);
	int (*rwaddr_get)(int channel_id, unsigned int *rd_addr, unsigned int *wr_addr);
	int (*Rptr_Set)(int channel_id, unsigned int rd_ptr);
	int (*Wptr_Set)(int channel_id, unsigned int wr_ptr);
};

extern int gxav_sdc_init(struct gxav_module_ops* ops);
extern int gxav_sdc_uninit(void);
extern int gxav_sdc_open(void* priv);
extern int gxav_sdc_close(void);
extern int gxav_sdc_apply(int channel_id, unsigned int size);
extern int gxav_sdc_free(int channel_id);
extern int gxav_sdc_gate_set(int channel_id, unsigned int empty, unsigned int full);
extern int gxav_sdc_gate_get(int channel_id, unsigned int *empty, unsigned int *full);
extern int gxav_sdc_algate_set(int channel_id, unsigned int almost_empty, unsigned int almost_full);
extern int gxav_sdc_algate_get(int channel_id, unsigned int *almost_empty, unsigned int *almost_full);
extern int gxav_sdc_length_set(int channel_id, unsigned int len, GxAvSdcOPMode mode);
extern int gxav_sdc_length_get(int channel_id, unsigned int *len);
extern int gxav_sdc_free_get(int channel_id, unsigned int *len);
extern int gxav_sdc_buffer_reset(int channel_id);
extern int gxav_sdc_buffer_flush(int channel_id);
extern int gxav_sdc_rwaddr_get(int channel_id, unsigned int *rd_addr,unsigned int *wr_addr);
extern int gxav_sdc_Rptr_Set(int channel_id, unsigned int rd_ptr);
extern int gxav_sdc_Wptr_Set(int channel_id, unsigned int wr_ptr);




#endif

