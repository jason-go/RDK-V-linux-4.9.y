//------------------------------------------------------------------------------
// File: IO_API.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#include "kernelcalls.h"
#include "GXIO_API.h"
#include "VpuApi.h"

void ReadSdramBurst(unsigned char *buf, int addr, int byteSize, int bIsBigEndian)
{
	INT             i;
	UINT            dword1, dword2, dword3;
	PUINT           pBuf = 0;

	//	Read_HPI_Burst_Byte(buf, addr, byteSize);
	gx_memcpy((BYTE*)buf, (BYTE*)addr, byteSize);

	pBuf   = (PUINT) buf;
	switch(bIsBigEndian) {
		case 1:	// 64b LITTLE  //64 big
			for (i=0; i<byteSize/4; i+=2) {
				dword1 = pBuf[i];
				dword2  = ( dword1 >> 24) & 0xFF;
				dword2 |= ((dword1 >> 16) & 0xFF) <<  8;
				dword2 |= ((dword1 >>  8) & 0xFF) << 16;
				dword2 |= ((dword1 >>  0) & 0xFF) << 24;
				dword3 = dword2;
				dword1  = pBuf[i+1];
				dword2  = ( dword1 >> 24) & 0xFF;
				dword2 |= ((dword1 >> 16) & 0xFF) <<  8;
				dword2 |= ((dword1 >>  8) & 0xFF) << 16;
				dword2 |= ((dword1 >>  0) & 0xFF) << 24;
				pBuf[i]   = dword2;
				pBuf[i+1] = dword3;
			}
			break;
		case 0:	// 64b BIG  //64 little
			break;
		case 3:	// 32b LITTLE  //32 big
			for (i=0; i<byteSize/4; i++) {
				dword1 = pBuf[i];
				dword2  = ( dword1 >> 24) & 0xFF;
				dword2 |= ((dword1 >> 16) & 0xFF) <<  8;
				dword2 |= ((dword1 >>  8) & 0xFF) << 16;
				dword2 |= ((dword1 >>  0) & 0xFF) << 24;
				pBuf[i] = dword2;
			}
			break;
		case 2:	// 32b BIG  //32 little
			for (i=0; i<byteSize/4; i+=2) {
				dword1 = pBuf[i];
				dword2 = pBuf[i+1];
				pBuf[i]   = dword2;
				pBuf[i+1] = dword1;
			}
			break;
	}

}

void WriteSdramBurst (unsigned char buf[], int addr, int byteSize, int bIsBigEndian)
{
	INT				i;
	UINT            dword1, dword2, dword3;
	PUINT           pBuf;
	// static int      LeBufSize;
	// static PBYTE    LeBuf;

	pBuf   = (PUINT) buf;

	switch(bIsBigEndian) {
		case 1:	// 64b LITTLE  //should be big
			for (i=0; i<byteSize/4; i+=2) {
				dword1 = pBuf[i];
				dword2  = ( dword1 >> 24) & 0xFF;
				dword2 |= ((dword1 >> 16) & 0xFF) <<  8;
				dword2 |= ((dword1 >>  8) & 0xFF) << 16;
				dword2 |= ((dword1 >>  0) & 0xFF) << 24;
				dword3 = dword2;
				dword1  = pBuf[i+1];
				dword2  = ( dword1 >> 24) & 0xFF;
				dword2 |= ((dword1 >> 16) & 0xFF) <<  8;
				dword2 |= ((dword1 >>  8) & 0xFF) << 16;
				dword2 |= ((dword1 >>  0) & 0xFF) << 24;
				pBuf[i]   = dword2;
				pBuf[i+1] = dword3;
			}
			break;
		case 0:	// 64b BIG //should be little
			break;
		case 3:	// 32b LITTLE  //32 big
			for (i=0; i<byteSize/4; i++) {
				dword1 = pBuf[i];
				dword2  = ( dword1 >> 24) & 0xFF;
				dword2 |= ((dword1 >> 16) & 0xFF) <<  8;
				dword2 |= ((dword1 >>  8) & 0xFF) << 16;
				dword2 |= ((dword1 >>  0) & 0xFF) << 24;
				pBuf[i] = dword2;
			}
			break;
		case 2:	// 32b BIG  //32 little
			for (i=0; i<byteSize/4; i+=2) {
				dword1 = pBuf[i];
				dword2 = pBuf[i+1];
				pBuf[i]   = dword2;
				pBuf[i+1] = dword1;
			}
			break;
	}

	gx_memcpy((BYTE*)addr, (BYTE*)buf, byteSize);
	//  Write_HPI_Burst_Byte(addr, byteSize, (BYTE*)buf);
}
