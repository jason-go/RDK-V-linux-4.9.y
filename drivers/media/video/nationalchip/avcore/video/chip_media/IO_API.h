//------------------------------------------------------------------------------
// File: IO_API.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef _DEVICEINTERFACE_H_
#define _DEVICEINTERFACE_H_

/*--------------------------------------------------------------------
Include files
--------------------------------------------------------------------*/
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>

#define FILE_DEVICE_HPI					0x8000
#define HPI_IOCTL(index)				CTL_CODE(FILE_DEVICE_HPI, index, METHOD_BUFFERED, FILE_READ_DATA)
#define IOCTL_READ_DWORD				CTL_CODE(FILE_DEVICE_HPI, 0x800, METHOD_IN_DIRECT, FILE_READ_ACCESS)
#define IOCTL_WRITE_DWORD				CTL_CODE(FILE_DEVICE_HPI, 0x801, METHOD_OUT_DIRECT, FILE_WRITE_ACCESS)
#define IOCTL_CREATE_SHARE_MEMORY		CTL_CODE(FILE_DEVICE_HPI, 0x802, METHOD_OUT_DIRECT, FILE_READ_ACCESS)


#if defined (__cplusplus)
extern "C"{
#endif 
	
	INT		InitIoDevice();	
	VOID	UninitIoDevice();
	VOID	ResetDevice();
	UINT	ReadReg( INT addr );
	VOID	WriteReg( INT addr, UINT data );
	
	VOID	WriteSdramBurst( UCHAR buf[], INT addr, INT byteSize, BOOL bIsBigEndian );
	VOID	ReadSdramBurst( UCHAR buf[], INT addr, INT byteSize, BOOL bIsBigEndian );
	
	VOID	ReadSdram( BYTE	*buf, INT addr, INT byteSize, BOOL bIsBigEndian );
	VOID	WriteSdram( UCHAR *buf, INT addr, INT byteSize, BOOL bIsBigEndian );
	
	VOID	SetSdram( int ByteAddr, int ByteSize, BYTE data );
	VOID	WriteMem( int addr, __int64 data);
	
	UINT	ReadHPITimingReg(INT addr);
	VOID	WriteHPITimingReg(INT addr, UINT data);

	UINT	ReadRegLimit(INT addr, UINT *data);
	UINT	WriteRegLimit(INT addr, UINT data);

#if defined (__cplusplus)
}
#endif 

#endif //#ifndef _DEVICEINTERFACE_H_
