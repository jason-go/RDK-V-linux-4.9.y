#ifndef __GX3211_GA_H__
#define __GX3211_GA_H__

#include "gx3211_vpu.h"

typedef enum {
	GA_RET_FAIL = -1,
	GA_RET_OK   =  0,
}GaRet;

extern GaRet gx3211ga_init(void);
extern GaRet gx3211ga_cleanup(void);
extern GaRet gx3211ga_open(void);
extern GaRet gx3211ga_close(void);
extern GaRet gx3211ga_blit(GxVpuProperty_Blit *property);
extern GaRet gx3211ga_dfb_blit(GxVpuProperty_DfbBlit *property);
extern GaRet gx3211ga_batch_dfb_blit(GxVpuProperty_BatchDfbBlit *property);
extern GaRet gx3211ga_fillrect(GxVpuProperty_FillRect *property);
extern GaRet gx3211ga_zoom(GxVpuProperty_ZoomSurface *property);
extern GaRet gx3211ga_turn(GxVpuProperty_TurnSurface *property);
extern GaRet gx3211ga_complet(GxVpuProperty_Complet *complet);
extern GaRet gx3211ga_setmode(GxVpuProperty_SetGAMode *property);
extern GaRet gx3211ga_wait(GxVpuProperty_WaitUpdate *property);
extern GaRet gx3211ga_begin(GxVpuProperty_BeginUpdate * property);
extern GaRet gx3211ga_end(GxVpuProperty_EndUpdate * property);
extern GaRet gx3211ga_interrupt(int irq);

#endif
