#ifndef __CYGNUS_HALPHY_H__
#define __CYGNUS_HALPHY_H__

#include "../../util/types.h"

u8 cygnus_halSourcePhy_HotPlugState(u16 baseAddr);

void cygnus_halSourcePhy_InterruptMask(u16 baseAddr, u8 mask);

void cygnus_halSourcePhy_InterruptPolarity(u16 baseAddr, u8 bitShift, u8 value);

u8 cygnus_halSourcePhy_InterruptPolarityStatus(u16 baseAddr, u8 mask);

#endif /* __CYGNUS_HALPHY_H__ */