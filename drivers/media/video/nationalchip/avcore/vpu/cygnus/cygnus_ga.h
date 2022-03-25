#ifndef __CYGNUS_GA_H__
#define __CYGNUS_GA_H__

#include "cygnus_vpu.h"

typedef enum {
	GA_RET_FAIL = -1,
	GA_RET_OK   =  0,
}GaRet;

extern GaRet cygnusga_init(void);
extern GaRet cygnusga_cleanup(void);
extern GaRet cygnusga_open(void);
extern GaRet cygnusga_close(void);
extern GaRet cygnusga_blit(GxVpuProperty_Blit *property);
extern GaRet cygnusga_dfb_blit(GxVpuProperty_DfbBlit *property);
extern GaRet cygnusga_batch_dfb_blit(GxVpuProperty_BatchDfbBlit *property);
extern GaRet cygnusga_fillrect(GxVpuProperty_FillRect *property);
extern GaRet cygnusga_zoom(GxVpuProperty_ZoomSurface *property);
extern GaRet cygnusga_turn(GxVpuProperty_TurnSurface *property);
extern GaRet cygnusga_complet(GxVpuProperty_Complet *complet);
extern GaRet cygnusga_setmode(GxVpuProperty_SetGAMode *property);
extern GaRet cygnusga_wait(GxVpuProperty_WaitUpdate *property);
extern GaRet cygnusga_begin(GxVpuProperty_BeginUpdate * property);
extern GaRet cygnusga_end(GxVpuProperty_EndUpdate * property);
extern GaRet cygnusga_interrupt(int irq);

#endif
