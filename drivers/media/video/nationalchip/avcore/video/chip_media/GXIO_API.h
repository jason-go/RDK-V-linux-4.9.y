//------------------------------------------------------------------------------
// File: IO_API.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _GXIO_API_H_
#define _GXIO_API_H_

#include "kernelcalls.h"

#if defined (__cplusplus)
extern "C"{
#endif 

typedef void  VOID;
typedef int  INT;
//typedef int  BOOL;
typedef unsigned int * PUINT;
typedef unsigned char * PBYTE;
	
void WriteSdramBurst( unsigned char buf[], int addr, int byteSize, int bIsBigEndian );
void ReadSdramBurst( unsigned char buf[], int addr, int byteSize, int bIsBigEndian );
#define VpuWriteReg(ADDR, DATA)	gx_iowrite32( DATA, ADDR ) // system register write
#define VpuWriteMem(ADDR, DATA)	gx_iowrite32( DATA, ADDR ) // system memory write
#define VpuReadReg(ADDR)        gx_ioread32( ADDR )	   // system register write
#define VpuReadMem(ADDR)        gx_ioread32( ADDR )	   // system memory read

#define WriteReg(ADDR, DATA)	gx_iowrite32( DATA, ADDR ) // system register write
#define ReadReg(ADDR)        gx_ioread32( ADDR )	   // system register write

#if defined (__cplusplus)
}
#endif 

#endif //#ifndef _GXIO_API_H_
