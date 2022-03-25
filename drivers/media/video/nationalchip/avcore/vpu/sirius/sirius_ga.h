#ifndef __SIRIUS_GA_H__
#define __SIRIUS_GA_H__

#include "sirius_vpu.h"

typedef enum {
	GA_RET_FAIL = -1,
	GA_RET_OK   =  0,
}GaRet;

extern GaRet siriusga_init(void);
extern GaRet siriusga_cleanup(void);
extern GaRet siriusga_open(void);
extern GaRet siriusga_close(void);
extern GaRet siriusga_blit(GxVpuProperty_Blit *property);
extern GaRet siriusga_dfb_blit(GxVpuProperty_DfbBlit *property);
extern GaRet siriusga_batch_dfb_blit(GxVpuProperty_BatchDfbBlit *property);
extern GaRet siriusga_fillrect(GxVpuProperty_FillRect *property);
extern GaRet siriusga_zoom(GxVpuProperty_ZoomSurface *property);
extern GaRet siriusga_turn(GxVpuProperty_TurnSurface *property);
extern GaRet siriusga_complet(GxVpuProperty_Complet *complet);
extern GaRet siriusga_setmode(GxVpuProperty_SetGAMode *property);
extern GaRet siriusga_wait(GxVpuProperty_WaitUpdate *property);
extern GaRet siriusga_begin(GxVpuProperty_BeginUpdate * property);
extern GaRet siriusga_end(GxVpuProperty_EndUpdate * property);
extern GaRet siriusga_interrupt(int irq);

#endif
