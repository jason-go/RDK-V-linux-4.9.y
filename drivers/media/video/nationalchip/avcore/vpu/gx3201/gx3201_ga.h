#ifndef __GX3201_GA_H__
#define __GX3201_GA_H__

#include "gx3201_vpu.h"

typedef enum {
	GA_RET_FAIL = -1,
	GA_RET_OK   =  0,
}GaRet;

extern GaRet gx3201ga_init(void);
extern GaRet gx3201ga_cleanup(void);
extern GaRet gx3201ga_open(void);
extern GaRet gx3201ga_close(void);
extern GaRet gx3201ga_blit(GxVpuProperty_Blit *property);
extern GaRet gx3201ga_dfb_blit(GxVpuProperty_DfbBlit *property);
extern GaRet gx3201ga_fillrect(GxVpuProperty_FillRect *property);
extern GaRet gx3201ga_zoom(GxVpuProperty_ZoomSurface *property);
extern GaRet gx3201ga_turn(GxVpuProperty_TurnSurface *property);
extern GaRet gx3201ga_complet(GxVpuProperty_Complet *complet);
extern GaRet gx3201ga_begin(GxVpuProperty_BeginUpdate * property);
extern GaRet gx3201ga_end(GxVpuProperty_EndUpdate * property);
extern GaRet gx3201ga_interrupt(int irq);

#endif
