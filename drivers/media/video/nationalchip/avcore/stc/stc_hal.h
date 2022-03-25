#ifndef __GX_STC_HAL_H__
#define __GX_STC_HAL_H__

#include "gxav.h"
#include "avcore.h"
#include "kernelcalls.h"

#define IS_PCR_RECOVERY(mode)    (mode == PCR_RECOVER)
#define IS_AVPTS_RECOVERY(mode)  (mode == AVPTS_RECOVER)
#define IS_APTS_RECOVERY(mode)   (mode == APTS_RECOVER)
#define IS_VPTS_RECOVERY(mode)   (mode == VPTS_RECOVER)
#define IS_NO_RECOVERY(mode)     (mode == NO_RECOVER)
#define IS_PURE_APTS_RECOVERY(mode)   (mode == PURE_APTS_RECOVER)
#define IS_FIXD_APTS_RECOVERY(mode)   (mode == FIXD_APTS_RECOVER)
#define IS_FIXD_VPTS_RECOVERY(mode)   (mode == FIXD_VPTS_RECOVER)

struct gxav_stc {
	unsigned int id;
	unsigned int source;
	unsigned int resolution;
    void* priv;
};

struct stc_ops {
	int (*init)(void);
	int (*cleanup)(void);
	int (*open)(int id);
	int (*close)(int id);
	int (*set_source)(int id ,GxSTCProperty_Config* source);
	int (*get_source)(int id ,GxSTCProperty_Config* source);
	int (*set_resolution)(int id, GxSTCProperty_TimeResolution* resolution);
	int (*get_resolution)(int id, GxSTCProperty_TimeResolution* resolution);
	int (*set_time)(int id, GxSTCProperty_Time* time);
	int (*get_time)(int id, GxSTCProperty_Time* time);
	int (*play)(int id );
	int (*stop)(int id );
	int (*pause)(int id );
	int (*resume)(int id );
	int (*read_apts)(int id, unsigned int *value);
	int (*read_pcr) (int id, unsigned int *value);
	int (*write_apts)(int id, unsigned int value, int immd);
	int (*write_vpts)(int id, unsigned int value, int immd);
	int (*invaild_apts)(unsigned int value);
};

extern int gxav_stc_init(struct gxav_module_ops* ops);
extern int gxav_stc_cleanup(void);
extern int gxav_stc_open(int id );
extern int gxav_stc_close(int id );
extern int gxav_stc_set_source(int id ,GxSTCProperty_Config* source);
extern int gxav_stc_get_source(int id, GxSTCProperty_Config* source);
extern int gxav_stc_set_resolution(int id, GxSTCProperty_TimeResolution* resolution);
extern int gxav_stc_get_resolution(int id, GxSTCProperty_TimeResolution* resolution);
extern int gxav_stc_get_base_resolution(int id, GxSTCProperty_TimeResolution* resolution);
extern int gxav_stc_set_time(int id, GxSTCProperty_Time* time);
extern int gxav_stc_get_time(int id, GxSTCProperty_Time* time);
extern int gxav_stc_play(int id );
extern int gxav_stc_stop(int id );
extern int gxav_stc_pause(int id );
extern int gxav_stc_resume(int id );
extern int gxav_stc_read_stc(int id, unsigned int *value);
extern int gxav_stc_read_apts(int id, unsigned int *value);
extern int gxav_stc_read_pcr (int id, unsigned int *value);
extern int gxav_stc_write_apts(int id, unsigned int value, int immd);
extern int gxav_stc_write_vpts(int id, unsigned int value, int immd);
extern int gxav_stc_invaild_apts(unsigned int value);

#endif
