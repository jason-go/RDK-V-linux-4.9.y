#include "cygnus_halPhy.h"
#include "../../bsp/access.h"
#include "../../util/log.h"

/* register offsets */
static const u8 PHY_STAT0 = 0x04;
static const u8 PHY_MASK0 = 0x06;
static const u8 PHY_POL0 = 0x07;

u8 cygnus_halSourcePhy_HotPlugState(u16 baseAddr)
{
	LOG_TRACE();
	return access_CoreRead((baseAddr + PHY_STAT0), 1, 1);
}

void cygnus_halSourcePhy_InterruptMask(u16 baseAddr, u8 mask)
{
	LOG_TRACE1(mask);
	access_CoreWriteByte(mask, (baseAddr + PHY_MASK0));
}

void cygnus_halSourcePhy_InterruptPolarity(u16 baseAddr, u8 bitShift, u8 value)
{
	LOG_TRACE2(bitShift, value);
	access_CoreWrite(value, (baseAddr + PHY_POL0), bitShift, 1);
}

u8 cygnus_halSourcePhy_InterruptPolarityStatus(u16 baseAddr, u8 mask)
{
	LOG_TRACE1(mask);
	return access_CoreReadByte(baseAddr + PHY_POL0) & mask;
}