#ifndef __CYGNUS_SCI_H__
#define __CYGNUS_SCI_H__

#include "gxmisc_sci_common.h"

// reg ops
int32_t cygnus_sci_hw_init(void *vreg, void *priv);
int32_t cygnus_sci_set_clk_fb(void *vreg, void *priv, uint32_t enable);
int32_t cygnus_sci_set_rxfifo_gate(void *vreg, void *priv, uint32_t gate);
int32_t cygnus_sci_set_txfifo_gate(void *vreg, void *priv, uint32_t gate);

#endif
