#ifndef __GX3113c_GA_H__
#define __GX3113c_GA_H__

#include "gx3113c_vpu.h"

typedef enum {
	GA_RET_FAIL = -1,
	GA_RET_OK   =  0,
}GaRet;

extern GaRet gx3113cga_init(void);
extern GaRet gx3113cga_cleanup(void);
extern GaRet gx3113cga_open(void);
extern GaRet gx3113cga_close(void);
extern GaRet gx3113cga_blit(GxVpuProperty_Blit *property);
extern GaRet gx3113cga_dfb_blit(GxVpuProperty_DfbBlit *property);
extern GaRet gx3113cga_fillrect(GxVpuProperty_FillRect *property);
extern GaRet gx3113cga_zoom(GxVpuProperty_ZoomSurface *property);
extern GaRet gx3113cga_turn(GxVpuProperty_TurnSurface *property);
extern GaRet gx3113cga_complet(GxVpuProperty_Complet *complet);
extern GaRet gx3113cga_begin(GxVpuProperty_BeginUpdate * property);
extern GaRet gx3113cga_end(GxVpuProperty_EndUpdate * property);
extern GaRet gx3113cga_interrupt(int irq);

#endif
