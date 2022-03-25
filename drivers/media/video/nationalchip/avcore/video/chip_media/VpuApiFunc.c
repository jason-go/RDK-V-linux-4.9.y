//------------------------------------------------------------------------------
// File: VpuApiFunc.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#include "VpuApiFunc.h"
#include "RegDefine.h"
 

extern CodecInst codecInstPool[MAX_NUM_INSTANCE];
extern PhysicalAddress workBuffer;
extern PhysicalAddress codeBuffer;
extern PhysicalAddress paraBuffer;

void BitIssueCommand(int instIdx, int cdcMode, int cmd)
{
	VpuWriteReg(BIT_BUSY_FLAG, 1);
	VpuWriteReg(BIT_RUN_INDEX, instIdx);
	VpuWriteReg(BIT_RUN_COD_STD, cdcMode);
	VpuWriteReg(BIT_RUN_COMMAND, cmd);
}

/*
 * GetCodecInstance() obtains a instance.
 * It stores a pointer to the allocated instance in *ppInst
 * and returns RETCODE_SUCCESS on success.
 * Failure results in 0(null pointer) in *ppInst and RETCODE_FAILURE.
 */

RetCode GetCodecInstance(CodecInst ** ppInst)
{
	int i;
	CodecInst * pCodecInst;

	pCodecInst = &codecInstPool[0];
	for (i = 0; i < MAX_NUM_INSTANCE; ++i, ++pCodecInst) {
		if (!pCodecInst->inUse)
			break;
	}

	if (i == MAX_NUM_INSTANCE) {
		*ppInst = 0;
		return RETCODE_FAILURE;
	}

	pCodecInst->inUse = 1;
	*ppInst = pCodecInst;
	return RETCODE_SUCCESS;
}

void FreeCodecInstance(CodecInst * pCodecInst)
{
	pCodecInst->inUse = 0;
}

RetCode CheckInstanceValidity(CodecInst * pci)
{
	CodecInst * pCodecInst;
	int i;

	pCodecInst = &codecInstPool[0];
	for (i = 0; i < MAX_NUM_INSTANCE; ++i, ++pCodecInst) {
		if (pCodecInst == pci)
			return RETCODE_SUCCESS;
	}
	return RETCODE_INVALID_HANDLE;
}

RetCode CheckDecInstanceValidity(DecHandle handle)
{
	CodecInst * pCodecInst;
	RetCode ret;

	pCodecInst = handle;
	ret = CheckInstanceValidity(pCodecInst);
	if (ret != RETCODE_SUCCESS) {
		return RETCODE_INVALID_HANDLE;
	}
	if (!pCodecInst->inUse) {
		return RETCODE_INVALID_HANDLE;
	}
	if (pCodecInst->codecMode != AVC_DEC && 
			pCodecInst->codecMode != HEVC_DEC &&
			pCodecInst->codecMode != MP2_DEC &&
			pCodecInst->codecMode != MP4_DEC &&
			pCodecInst->codecMode != DV3_DEC &&
			pCodecInst->codecMode != RV_DEC  &&
			pCodecInst->codecMode != AVS_DEC &&
			pCodecInst->codecMode != MJPG_DEC) {
		return RETCODE_INVALID_HANDLE;
	}

	return RETCODE_SUCCESS;
}

RetCode CheckDecOpenParam(DecOpenParam * pop)
{
	if (pop == 0) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBuffer % 4) { // not 4-bit aligned
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamBufferSize % 1024 ||
			pop->bitstreamBufferSize < 1024 ||
			pop->bitstreamBufferSize > (16383 * 1024) ) {
		return RETCODE_INVALID_PARAM;
	}
	if (pop->bitstreamFormat != STD_AVC
			&& pop->bitstreamFormat != STD_HEVC
			&& pop->bitstreamFormat != STD_MPEG2
			&& pop->bitstreamFormat != STD_MPEG4
			&& pop->bitstreamFormat != STD_DIV3
			&& pop->bitstreamFormat != STD_RV
			&& pop->bitstreamFormat != STD_AVS
			&& pop->bitstreamFormat != STD_MJPG
	   ) {
		return RETCODE_INVALID_PARAM;
	}

	if( pop->outloopDbkEnable == 1 && !(pop->bitstreamFormat == STD_MPEG4 || pop->bitstreamFormat == STD_MPEG2 || pop->bitstreamFormat == STD_DIV3)) {
		return RETCODE_INVALID_PARAM;
	}

	return RETCODE_SUCCESS;
}


int DecBitstreamBufEmpty(DecInfo * pDecInfo)
{
	return ( VpuReadReg(pDecInfo->streamRdPtrRegAddr) == VpuReadReg(pDecInfo->streamWrPtrRegAddr) );
}

void SetParaSet(DecHandle handle, int paraSetType, DecParamSet * para)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	int i;
	Uint32 * src;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	src = para->paraSet;
	for (i = 0; i < para->size; i += 4) {
		VpuWriteReg(paraBuffer + i, *src++);
	}
	VpuWriteReg(CMD_DEC_PARA_SET_TYPE, paraSetType); // 0: SPS, 1: PPS
	VpuWriteReg(CMD_DEC_PARA_SET_SIZE, para->size);

	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		VpuWriteReg(BIT_RUN_AUX_STD, 1);
	else
		VpuWriteReg(BIT_RUN_AUX_STD, 0);

#ifdef _CDB
	while ( VPU_IsBusy() ) ;
#endif // _CDB
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, DEC_PARA_SET);
#ifdef _CDB
	while ( VPU_IsBusy() ) ;
#else
	while (VpuReadReg(BIT_BUSY_FLAG));
#endif // _CDB
}
