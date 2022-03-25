/*
 * @file:halSourcePhy.h
 *
 *  Created on: Jul 1, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02

 */

#ifndef HALSOURCEPHY_H_
#define HALSOURCEPHY_H_

#include "../../util/types.h"

void gx3211_halSourcePhy_PowerDown(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_EnableTMDS(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_Gen2PDDQ(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_Gen2TxPowerOn(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_Gen2EnHpdRxSense(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_DataEnablePolarity(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_InterfaceControl(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_TestClear(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_TestEnable(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_TestClock(u16 baseAddr, u8 bit);

void gx3211_halSourcePhy_TestDataIn(u16 baseAddr, u8 data);

u8 gx3211_halSourcePhy_TestDataOut(u16 baseAddr);

u8 gx3211_halSourcePhy_PhaseLockLoopState(u16 baseAddr);

u8 gx3211_halSourcePhy_HotPlugState(u16 baseAddr);

void gx3211_halSourcePhy_InterruptMask(u16 baseAddr, u8 value);

u8 gx3211_halSourcePhy_InterruptMaskStatus(u16 baseAddr, u8 mask);

void gx3211_halSourcePhy_InterruptPolarity(u16 baseAddr, u8 bitShift, u8 value);

u8 gx3211_halSourcePhy_InterruptPolarityStatus(u16 baseAddr, u8 mask);


#endif /* HALSOURCEPHY_H_ */
