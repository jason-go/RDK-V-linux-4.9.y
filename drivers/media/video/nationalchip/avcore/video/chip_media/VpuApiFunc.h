//------------------------------------------------------------------------------
// File: VpuApiFunc.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef VPUAPI_UTIL_H_INCLUDED
#define VPUAPI_UTIL_H_INCLUDED

#include "VpuApi.h"
#include "RegDefine.h"



#define MAX_NUM_INSTANCE 4

enum {
	AVC_DEC = 0,
	HEVC_DEC = 1,		//add the HEVC define 
	MP2_DEC = 2,
	MP4_DEC = 3,
	DV3_DEC = 3,
	RV_DEC = 4,
	AVS_DEC = 5,
	MJPG_DEC = 6
};

typedef enum {
	MODE420 = 0,
	MODE422 = 1,
	MODE224 = 2,
	MODE444 = 3,
	MODE400 = 4
} JpuJpgMode;

typedef enum {
	SEQ_INIT = 1,
	SEQ_END = 2,
	PIC_RUN = 3,
	SET_FRAME_BUF = 4,
	ENCODE_HEADER = 5,
	ENC_PARA_SET = 6,
	DEC_RECOVER_EOS = 6,
	DEC_PARA_SET = 7,
	DEC_BUF_FLUSH = 8,
	RC_CHANGE_PARAMETER	= 9,
	DEC_RESYNC_SEQ = 9,
	VPU_SLEEP = 10,
	VPU_WAKE = 11,
	SEQ_CHANGED = 12,
	ESBUF_EMPTY = 14,
	FIRMWARE_GET = 0xf
} VpuCommand;

#define MAX_ENC_PIC_WIDTH	720
#define MAX_ENC_PIC_HEIGHT	576
#define MAX_ENC_MJPG_PIC_WIDTH		8192
#define MAX_ENC_MJPG_PIC_HEIGHT	8192

typedef struct {
	EncOpenParam openParam;
	EncInitialInfo initialInfo;
	PhysicalAddress streamRdPtr;
	PhysicalAddress streamRdPtrRegAddr;
	PhysicalAddress streamWrPtrRegAddr;
	PhysicalAddress streamBufStartAddr;
	PhysicalAddress streamBufEndAddr;
	int streamBufSize;
	FrameBuffer * frameBufPool;
	int numFrameBuffers;
	int stride;
	int rotationEnable;
	int mirrorEnable;
	MirrorDirection mirrorDirection;
	int rotationAngle;
	int initialInfoObtained;
	int ringBufferEnable;
} EncInfo;

typedef struct {
	DecOpenParam openParam;
	DecInitialInfo initialInfo;
	
	PhysicalAddress streamWrPtr;
	PhysicalAddress streamRdPtrRegAddr;
	PhysicalAddress streamWrPtrRegAddr;
	PhysicalAddress streamBufStartAddr;
	PhysicalAddress streamBufEndAddr;
	PhysicalAddress	frameDisplayFlagRegAddr;
	int streamBufSize;
	FrameBuffer * frameBufPool;
	int numFrameBuffers;
	FrameBuffer * recFrame;
	int stride;
	int rotationEnable;
	int rotIndex;
	int mirrorEnable;
	int	deringEnable;
	MirrorDirection mirrorDirection;
	int rotationAngle;
	FrameBuffer rotatorOutput;
	int rotatorStride;
	int rotatorOutputValid;	
	int initialInfoObtained;
	int picSrcSize;
	SecAxiUse secAxiUse;

	int mjpgSampX;
	int mjpgSampY;
	int mjpgClipMode;	
#ifdef	API_CR
	PhysicalAddress picParaBaseAddr;
	PhysicalAddress userDataBufAddr;

	PhysicalAddress frameBufStateAddr;
	PhysicalAddress mbParamDataAddr;
	PhysicalAddress mvDataAddr;

	int frameBufStatEnable;				// Frame Buffer Status
	int mbParamEnable;					// Mb Param for Error Concealment
	int mvReportEnable;					// Motion vector
	int userDataEnable;					// User Data
	int userDataBufSize;
	int userDataReportMode;				// User Data report mode (0 : interrupt mode, 1 interrupt disable mode)
#endif
} DecInfo;


typedef struct CodecInst {
	int instIndex;
	int inUse;
	int codecMode;
	union {
		EncInfo encInfo;
		DecInfo decInfo;
	} CodecInfo;
} CodecInst;


#ifdef __cplusplus
extern "C" {
#endif	
	void	BitIssueCommand(int instIdx, int cdcMode, int cmd);

	RetCode GetCodecInstance(CodecInst ** ppInst);
	void	FreeCodecInstance(CodecInst * pCodecInst);
	RetCode CheckInstanceValidity(CodecInst * pci);

	RetCode CheckEncInstanceValidity(EncHandle handle);
	RetCode CheckEncOpenParam(EncOpenParam * pop);
	RetCode CheckEncParam(CodecInst * pCodecInst, EncParam * param);
	void	EncodeHeader(EncHandle handle, EncHeaderParam * encHeaderParam);
	void	GetParaSet(EncHandle handle, int paraSetType, EncParamSet * para);
	RetCode SetGopNumber(EncHandle handle, Uint32 *gopNumber);
	RetCode SetIntraQp(EncHandle handle, Uint32 *intraQp);
	RetCode SetBitrate(EncHandle handle, Uint32 *bitrate);
	RetCode SetFramerate(EncHandle handle, Uint32 *framerate);
	RetCode SetIntraRefreshNum(EncHandle handle, Uint32 *pIntraRefreshNum);
	RetCode SetSliceMode(EncHandle handle, EncSliceMode *pSliceMode);
	RetCode SetHecMode(EncHandle handle, int mode);

	RetCode CheckDecInstanceValidity(DecHandle handle);
	RetCode CheckDecOpenParam(DecOpenParam * pop);
	int	DecBitstreamBufEmpty(DecInfo * pDecInfo);
	void	SetParaSet(DecHandle handle, int paraSetType, DecParamSet * para);	
	
	
#ifdef __cplusplus
}
#endif

#endif // endif VPUAPI_UTIL_H_INCLUDED
