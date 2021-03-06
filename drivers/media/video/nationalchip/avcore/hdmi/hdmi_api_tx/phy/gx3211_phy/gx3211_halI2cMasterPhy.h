/*
 * @file:halI2cMasterPhy.h
 *
 *  Created on: Jul 1, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 *
 *      NOTE: only write operations are implemented in this module
 *   	This module is used only from PHY GEN2 on
 */

#ifndef HALI2CMASTERPHY_H_
#define HALI2CMASTERPHY_H_

#include "../../util/types.h"

void gx3211_halI2cMasterPhy_SlaveAddress(u16 baseAddr, u8 value);

void gx3211_halI2cMasterPhy_RegisterAddress(u16 baseAddr, u8 value);

void gx3211_halI2cMasterPhy_WriteData(u16 baseAddr, u16 value);

void gx3211_halI2cMasterPhy_WriteRequest(u16 baseAddr);

void gx3211_halI2cMasterPhy_ReadRequest(u16 baseAddr);

u16 gx3211_halI2cMasterPhy_ReadData(u16 baseAddr);

void gx3211_halI2cMasterPhy_FastMode(u16 baseAddr, u8 bit);

void gx3211_halI2cMasterPhy_Reset(u16 baseAddr);

void gx3211_halI2cMasterPhy_MaskInterrupts(u16 baseAddr, u8 mask);

#endif /* HALI2CMASTERPHY_H_ */
