/*
 * @file:halI2cMasterPhy.c
 *
 *  Created on: Jul 1, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */
#include "gx3211_halI2cMasterPhy.h"
#include "../../bsp/access.h"
#include "../../util/log.h"
#include "kernelcalls.h"

/* register offsets */
static const u8 PHY_I2CM_SLAVE = 0x00;
static const u8 PHY_I2CM_ADDRESS = 0x01;
static const u8 PHY_I2CM_DATAO = 0x02;
static const u8 PHY_I2CM_DATAI = 0x04;
static const u8 PHY_I2CM_OPERATION = 0x06;
static const u8 PHY_I2CM_INT = 0x07;
static const u8 PHY_I2CM_CTLINT = 0x8;
static const u8 PHY_I2CM_DIV = 0x09;
static const u8 PHY_I2CM_SOFTRSTZ = 0x0A;

void gx3211_halI2cMasterPhy_SlaveAddress(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWrite(value, (baseAddr + PHY_I2CM_SLAVE), 0, 7);
}

void gx3211_halI2cMasterPhy_RegisterAddress(u16 baseAddr, u8 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte(value, (baseAddr + PHY_I2CM_ADDRESS));
}

void gx3211_halI2cMasterPhy_WriteData(u16 baseAddr, u16 value)
{
	LOG_TRACE1(value);
	access_CoreWriteByte((u8) (value >> 8), (baseAddr + PHY_I2CM_DATAO + 0));
	access_CoreWriteByte((u8) (value >> 0), (baseAddr + PHY_I2CM_DATAO + 1));
}

u16 gx3211_halI2cMasterPhy_ReadData(u16 baseAddr)
{
	LOG_TRACE();
	return ((u16)(access_CoreReadByte(baseAddr + PHY_I2CM_DATAI + 0) << 8) | access_CoreReadByte(baseAddr + PHY_I2CM_DATAI + 1));
}

void gx3211_halI2cMasterPhy_ReadRequest(u16 baseAddr)
{
	LOG_TRACE();
	access_CoreWrite(1, (baseAddr + PHY_I2CM_OPERATION), 0, 1);
}

void gx3211_halI2cMasterPhy_WriteRequest(u16 baseAddr)
{
	LOG_TRACE();
	access_CoreWrite(1, (baseAddr + PHY_I2CM_OPERATION), 4, 1);
}

void gx3211_halI2cMasterPhy_FastMode(u16 baseAddr, u8 bit)
{
	LOG_TRACE1(bit);
	access_CoreWrite(bit, (baseAddr + PHY_I2CM_DIV), 0, 1);
}

void gx3211_halI2cMasterPhy_Reset(u16 baseAddr)
{
	LOG_TRACE();
	access_CoreWrite(0, (baseAddr + PHY_I2CM_SOFTRSTZ), 0, 1);
}

void gx3211_halI2cMasterPhy_MaskInterrupts(u16 baseAddr, u8 mask)
{
	LOG_TRACE1(mask);
	access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_INT), 2, 1);
	//access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_CTLINT), 2, 1);
	//access_CoreWrite(mask ? 1 : 0, (baseAddr + PHY_I2CM_CTLINT), 6, 1);
}

