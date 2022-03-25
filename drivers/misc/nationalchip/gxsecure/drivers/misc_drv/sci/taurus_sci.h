#ifndef __TAURUS_SCI_H__
#define __TAURUS_SCI_H__

#include "gxmisc_sci_common.h"


extern struct sci_priv taurus_sci_priv;
extern GxSeModuleHwObj taurus_sci_hwobj;
extern GxSeModuleDevOps taurus_sci_devops;

// reg ops
int32_t taurus_sci_hw_init(void *vreg, void *priv);
int32_t taurus_sci_set_clk_fb(void *vreg, void *priv, uint32_t enable);
int32_t taurus_sci_set_rxfifo_gate(void *vreg, void *priv, uint32_t gate);
int32_t taurus_sci_set_txfifo_gate(void *vreg, void *priv, uint32_t gate);

#endif
