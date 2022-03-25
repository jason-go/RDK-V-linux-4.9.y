//------------------------------------------------------------------------------
// File: VpuApi.c
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#include "VpuApiFunc.h"
#include "kernelcalls.h"

#include "firewall.h"
#include "video.h"
#include "gxav_firmware.h"

#include "BitCode.h"
#include "chip_media.h"

#define  VDEC_HW_ADDR_MASK (0xFFFFFFFF)
#define  GX_DDR_BASE_ADDR  (0x00000000)

#define  FIXED_GREEN_DEAD_IN_DEC_CLOSE
//#define  ADD_SEQ_ERR_RETURN
int BIT_BASE     = 0;
int GDMA_BASE    = 0;
int MBC_BASE     = 0;
int NC_Base_Addr = 0;

static PhysicalAddress rdPtrRegAddr[4];
static PhysicalAddress wrPtrRegAddr[4];
static PhysicalAddress disFlagRegAddr[4];

static int vpuDownLoaded ;

static struct bitcode_desc bitcode;

// If a frame is started, pendingInst is set to the proper instance.
static CodecInst * pendingInst;

CodecInst codecInstPool[MAX_NUM_INSTANCE];
PhysicalAddress workBuffer;
PhysicalAddress codeBuffer;
PhysicalAddress paraBuffer;
PhysicalAddress picParaBaseAddr;
PhysicalAddress userDataBufAddr;

#define WORK_BUF_H264_PPS_OFFSET  (0x1400+0x600)
#define WORK_BUF_H264_PPS_SIZE    (0x800)
#define MAX_PPS_NUM               (0x100)
#define MAX_SPS_NUM               (0x020)


PhysicalAddress ppsBuffer;
PhysicalAddress spsBuffer;
int userDataBufSize;

static int g_print_level;

int VPU_IsBusy()
{
	return VpuReadReg(BIT_BUSY_FLAG) != 0;
}

RetCode VPU_Init(PhysicalAddress workBuf, VirtualAddress Virtual_addr, DecOpenParam *pop)
{
	int i;
	int new_load = 1;
	Uint32 data;
	CodecInst * pCodecInst;
	UINT vir_code_buf,vir_work_buf,vir_para_buf;
	UINT code_buf_size;

	int codec;

	g_print_level = LEVEL_ALWAYS;

    Console_Printf(LEVEL_DEBUG, "VPU_Init()\n");

	pendingInst = 0;
	vir_code_buf = Virtual_addr;

	codec = pop->bitstreamFormat;
	if (bitcode_fetch(codec, &bitcode) != 0) {
		return RETCODE_FAILURE;
	}

	if (bitcode.codec == STD_ALL)
		new_load = 0;

	if (new_load == 0) {
		code_buf_size = bitcode.load_offset + bitcode.size;
		if (code_buf_size > CODE_BUF_SIZE)
			gx_printf("\n%s:%d, video firmware buf(120K) is not enough!\n", __func__, __LINE__);
		else
			code_buf_size = CODE_BUF_SIZE;
		codeBuffer = workBuf;
		workBuffer = codeBuffer + code_buf_size;
		paraBuffer = workBuffer + WORK_BUF_SIZE + PARA_BUF2_SIZE;

		vir_work_buf = vir_code_buf + code_buf_size;
		vir_para_buf = vir_work_buf + WORK_BUF_SIZE + PARA_BUF2_SIZE;

		ppsBuffer = vir_work_buf + WORK_BUF_H264_PPS_OFFSET;
		spsBuffer = ppsBuffer    + WORK_BUF_H264_PPS_SIZE;
		//load firmware
		vir_code_buf += bitcode.load_offset;

		VpuWriteReg(BIT_WORK_BUF_ADDR, workBuffer&VDEC_HW_ADDR_MASK);
		VpuWriteReg(BIT_PARA_BUF_ADDR, paraBuffer&VDEC_HW_ADDR_MASK);
		VpuWriteReg(BIT_CODE_BUF_ADDR, codeBuffer&VDEC_HW_ADDR_MASK);

		//add NDS codebuffer size
		VpuWriteReg(BIT_NDS_BUF_SIZE, code_buf_size);
		gxav_firmware_copy((void *)vir_code_buf, bitcode.code, code_buf_size, GXAV_FIRMWARE_VIDEO);
	} else {
		code_buf_size = bitcode.size;
		if (code_buf_size > CODE_BUF_SIZE)
			gx_printf("\n%s:%d, video firmware buf(120K) is not enough!\n", __func__, __LINE__);
		else
			code_buf_size = CODE_BUF_SIZE;
		codeBuffer = workBuf;
		workBuffer = codeBuffer + SINGLE_CODE_BUF_SIZE;
		paraBuffer = workBuffer + WORK_BUF_SIZE + PARA_BUF2_SIZE;

		vir_work_buf = vir_code_buf + SINGLE_CODE_BUF_SIZE;
		vir_para_buf = vir_work_buf + WORK_BUF_SIZE + PARA_BUF2_SIZE;

		ppsBuffer = vir_work_buf + WORK_BUF_H264_PPS_OFFSET;
		spsBuffer = ppsBuffer    + WORK_BUF_H264_PPS_SIZE;
		//load firmware
		codeBuffer -= bitcode.load_offset;

		VpuWriteReg(BIT_WORK_BUF_ADDR, workBuffer&VDEC_HW_ADDR_MASK);
		VpuWriteReg(BIT_PARA_BUF_ADDR, paraBuffer&VDEC_HW_ADDR_MASK);
		VpuWriteReg(BIT_CODE_BUF_ADDR, codeBuffer&VDEC_HW_ADDR_MASK);

		//add NDS codebuffer size
		VpuWriteReg(BIT_NDS_BUF_SIZE, code_buf_size);
		gxav_firmware_copy((void *)vir_code_buf, bitcode.code, code_buf_size, GXAV_FIRMWARE_VIDEO);
	}

	rdPtrRegAddr[0] = BIT_RD_PTR_0;
	rdPtrRegAddr[1] = BIT_RD_PTR_1;
	rdPtrRegAddr[2] = BIT_RD_PTR_2;
	rdPtrRegAddr[3] = BIT_RD_PTR_3;

	wrPtrRegAddr[0] = BIT_WR_PTR_0;
	wrPtrRegAddr[1] = BIT_WR_PTR_1;
	wrPtrRegAddr[2] = BIT_WR_PTR_2;
	wrPtrRegAddr[3] = BIT_WR_PTR_3;

	disFlagRegAddr[0] = BIT_FRM_DIS_FLG_0;
	disFlagRegAddr[1] = BIT_FRM_DIS_FLG_1;
	disFlagRegAddr[2] = BIT_FRM_DIS_FLG_2;
	disFlagRegAddr[3] = BIT_FRM_DIS_FLG_3;

#if  DEBUG_MEM_CHECK
    Console_Printf(LEVEL_ALWAYS, "WorkBuf: %08x, ParaBuf: %08x, CodeBuf: %08x\n", workBuffer, paraBuffer, codeBuffer);
#endif

	data = STREAM_FULL_EMPTY_CHECK_DISABLE << 2;
	data |= STREAM_ENDIAN;
	VpuWriteReg(BIT_BIT_STREAM_CTRL, data);
	VpuWriteReg(BIT_FRAME_MEM_CTRL, (CBCR_INTERLEAVE<<2) | IMAGE_ENDIAN);
	VpuWriteReg(BIT_INT_ENABLE, 0);
	
	paraBuffer = vir_para_buf;

#ifdef USE_INTERRUPT_USE_DATA_REPORT
	enInt |= (1<<INT_BIT_USERDATA);
#endif // USE_INTERRUPT_USE_DATA_REPORT


	pCodecInst = &codecInstPool[0];
	for( i = 0; i < MAX_NUM_INSTANCE; ++i, ++pCodecInst )
	{
		pCodecInst->instIndex = i;
		pCodecInst->inUse = 0;
	}

	if(codec == STD_AVC)
	{
		h264ClrPsBuf(0);
	}

	pendingInst = NULL;

    Console_Printf(LEVEL_DEBUG, "BIT_CUR_PC: %08x\n", VpuReadReg(BIT_CUR_PC));

	if (VpuReadReg(BIT_CUR_PC) != 0) // except system reset, BIT_CUR_PC != 0
		return RETCODE_SUCCESS;

	VpuWriteReg(BIT_CODE_RUN, 0);

	extern unsigned char bitcode_head[];
	unsigned short *head = (unsigned short *)bitcode_head;
	static unsigned char  nds_config_done = 0;

	if (VpuReadReg(BIT_NDS_HEVC_EN_SIRIUS) & 0x8 && nds_config_done == 0) {
		Console_Printf(LEVEL_DEBUG, "waiting nds done VpuReadReg(BIT_NDS_HEVC_EN_SIRIUS):0x%08x\n",VpuReadReg(BIT_NDS_HEVC_EN_SIRIUS));
		while(1)
		{
			nds_config_done = VpuReadReg(BIT_NDS_HEVC_EN_SIRIUS);
			if(0x1 & (nds_config_done >> 4))
			{
				break;
			}
		}
		Console_Printf(LEVEL_DEBUG, "waited nds done\n");
	}

	for( i = 0; i < 2048; ++i)
	{
		data = head[i];
		VpuWriteReg(BIT_CODE_DOWN, (i << 16) | data);
	}

	if( vpuDownLoaded == 0 )
		vpuDownLoaded = 1;

	VpuWriteReg(BIT_BUSY_FLAG, 1);
	VpuWriteReg(BIT_CODE_RUN, 1);
	while (VpuReadReg(BIT_BUSY_FLAG));

	return RETCODE_SUCCESS;
}

RetCode VPU_GetVersionInfo( Uint32 *versionInfo )
{
	Uint32 ver;

	if (VpuReadReg(BIT_CUR_PC) == 0)
		return RETCODE_NOT_INITIALIZED;

	if( pendingInst )
		return RETCODE_FRAME_NOT_COMPLETE;

	VpuWriteReg( RET_VER_NUM , 0 );

	BitIssueCommand( 0, 0, FIRMWARE_GET );

	while (VpuReadReg(BIT_BUSY_FLAG));
	ver = VpuReadReg( RET_VER_NUM );
	Console_Printf(LEVEL_ALWAYS, "VPU_GetVersionInfo(): %08x\n", ver);

	if (ver == 0)
		return RETCODE_FAILURE;

	*versionInfo = ver;

	return RETCODE_SUCCESS;
}

RetCode VPU_DecOpen(DecHandle * pHandle, DecOpenParam * pop)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	int instIdx, value;
	int i;
	RetCode ret;

    Console_Printf(LEVEL_DEBUG, "VPU_DecOpen()\n" );

	if (VpuReadReg(BIT_CUR_PC) == 0){
		return RETCODE_NOT_INITIALIZED;
	}

	ret = CheckDecOpenParam(pop);
	if (ret != RETCODE_SUCCESS) {
		return ret;
	}

	ret = GetCodecInstance(&pCodecInst);
	if (ret == RETCODE_FAILURE) {
		*pHandle = 0;
		return RETCODE_FAILURE;
	}

	*pHandle = pCodecInst;
	instIdx = pCodecInst->instIndex;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	pDecInfo->openParam = *pop;
    Console_Printf(LEVEL_DEBUG, "BitstreamFormat: " );


	if (pop->bitstreamFormat == STD_MPEG4) {
		pCodecInst->codecMode = MP4_DEC;
        Console_Printf(LEVEL_DEBUG, "MP4_DEC\n" );
	}
	else if (pop->bitstreamFormat == STD_AVC) {
		pCodecInst->codecMode = AVC_DEC;
        Console_Printf(LEVEL_DEBUG, "AVC_DEC\n" );
	}
	else if (pop->bitstreamFormat == STD_HEVC) {
		pCodecInst->codecMode = HEVC_DEC;
        Console_Printf(LEVEL_DEBUG, "HEVC_DEC\n" );
	}
	else if (pop->bitstreamFormat == STD_MPEG2) {
		pCodecInst->codecMode = MP2_DEC;
        Console_Printf(LEVEL_DEBUG, "MP2_DEC\n" );
	}
	else if (pop->bitstreamFormat == STD_DIV3) {
		pCodecInst->codecMode = DV3_DEC;
        Console_Printf(LEVEL_DEBUG, "DV3_DEC\n" );
	}
	else if (pop->bitstreamFormat == STD_RV) {
		pCodecInst->codecMode = RV_DEC;
        Console_Printf(LEVEL_DEBUG, "RV_DEC\n" );
	}
	else if (pop->bitstreamFormat == STD_AVS) {
		pCodecInst->codecMode = AVS_DEC;
        Console_Printf(LEVEL_DEBUG, "AVS_DEC\n" );
	}
	else if (pop->bitstreamFormat == STD_MJPG) {
		pCodecInst->codecMode = MJPG_DEC;
        Console_Printf(LEVEL_DEBUG, "MJPG_DEC\n" );
	}

    Console_Printf(LEVEL_DEBUG, "Inst[%x]: BitstreamStartAddr: %08x, BitstreamBufSize: %08x\n",
                    instIdx, pop->bitstreamBuffer, pop->bitstreamBufferSize);


	if (pop->bitstreamFormat == STD_DIV3)
		pDecInfo->picSrcSize = (pop->picWidth << 16) | pop->picHeight;

	pDecInfo->streamWrPtr = pop->bitstreamBuffer;
	pDecInfo->streamRdPtrRegAddr = rdPtrRegAddr[instIdx];
	pDecInfo->streamWrPtrRegAddr = wrPtrRegAddr[instIdx];
	pDecInfo->frameDisplayFlagRegAddr = disFlagRegAddr[instIdx];
	pDecInfo->streamBufStartAddr = pop->bitstreamBuffer;
	pDecInfo->streamBufSize = pop->bitstreamBufferSize;
	pDecInfo->streamBufEndAddr = pop->bitstreamBuffer + pop->bitstreamBufferSize;
	pDecInfo->frameBufPool = 0;

	pDecInfo->rotationEnable = 0;
	pDecInfo->mirrorEnable = 0;
	pDecInfo->mirrorDirection = MIRDIR_NONE;
	pDecInfo->rotationAngle = 0;
	pDecInfo->rotatorOutputValid = 0;
	pDecInfo->rotatorStride = 0;
	pDecInfo->deringEnable	= 0;
	pDecInfo->mjpgClipMode = 0;
	pDecInfo->mjpgSampX = 0;
	pDecInfo->mjpgSampY = 0;
	pDecInfo->secAxiUse.useBitEnable  = 0;
	pDecInfo->secAxiUse.useIpEnable   = 0;
	pDecInfo->secAxiUse.useDbkYEnable = 0;
	pDecInfo->secAxiUse.useDbkCEnable = 0;
	pDecInfo->secAxiUse.useOvlEnable  = 0;
	pDecInfo->secAxiUse.useBtpEnable  = 0;

	pDecInfo->secAxiUse.useHostBitEnable  = 0;
	pDecInfo->secAxiUse.useHostIpEnable   = 0;
	pDecInfo->secAxiUse.useHostDbkYEnable = 0;
	pDecInfo->secAxiUse.useHostDbkCEnable = 0;
	pDecInfo->secAxiUse.useHostOvlEnable  = 0;
	pDecInfo->secAxiUse.useHostBtpEnable  = 0;

	pDecInfo->secAxiUse.bufBitUse    = 0;
	pDecInfo->secAxiUse.bufIpAcDcUse = 0;
	pDecInfo->secAxiUse.bufDbkYUse   = 0;
	pDecInfo->secAxiUse.bufDbkCUse   = 0;
	pDecInfo->secAxiUse.bufOvlUse    = 0;
	pDecInfo->secAxiUse.bufBtpUse    = 0;

	pDecInfo->initialInfoObtained = 0;

	pDecInfo->picParaBaseAddr = 0;
	pDecInfo->userDataBufAddr = 0;
	pDecInfo->frameBufStatEnable = 0;
	pDecInfo->mbParamEnable = 0;
	pDecInfo->mvReportEnable = 0;
	pDecInfo->userDataEnable = 0;
	pDecInfo->userDataBufSize = 0;
    //add for protect and hevc
    value = (pCodecInst->codecMode<<2)| 2;
	VpuWriteReg(BIT_GDI_PROTECT, value);
	//[4:2] codStd: fixed the bug codStd read in GDI
	//[1]: default 1; force axi requence with 64 bit align
	//[0]: default 0; protect the axi write in protected 4th domain as below
	//     if HDL requence the axi addr outside the domain, protect module
	//     will constrain the axi_addr in BIT_GDI_START_ADDR0~3;
	

	//VpuWriteReg(BIT_GDI_START_ADDR0, 0x14000000);
	//VpuWriteReg(BIT_GDI_END_ADDR0, 0x18000000);
	//VpuWriteReg(BIT_GDI_START_ADDR1, 0x14000000);
	//VpuWriteReg(BIT_GDI_END_ADDR1, 0x18000000);
	//VpuWriteReg(BIT_GDI_START_ADDR2, 0x14000000);
	//VpuWriteReg(BIT_GDI_END_ADDR2, 0x18000000);
	//VpuWriteReg(BIT_GDI_START_ADDR3, 0x14000000);
	//VpuWriteReg(BIT_GDI_END_ADDR3, 0x18000000);

	VpuWriteReg(pDecInfo->streamRdPtrRegAddr, pDecInfo->streamBufStartAddr&VDEC_HW_ADDR_MASK);
	VpuWriteReg(pDecInfo->streamWrPtrRegAddr, pDecInfo->streamWrPtr&VDEC_HW_ADDR_MASK);

	VpuWriteReg(pDecInfo->frameDisplayFlagRegAddr, 0);

	value = VpuReadReg( BIT_BIT_STREAM_PARAM );
	if (value & (1 << ( pCodecInst->instIndex + 2)))
		value -= 1 << ( pCodecInst->instIndex + 2);
	VpuWriteReg(BIT_BIT_STREAM_PARAM, value);
	return RETCODE_SUCCESS;
}

RetCode VPU_DecClose(DecHandle handle)
{
	unsigned int i;
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

    Console_Printf(LEVEL_DEBUG, "VPU_DecClose()\n" );

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->initialInfoObtained) {
		if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
			VpuWriteReg(BIT_RUN_AUX_STD, 1);
		else
			VpuWriteReg(BIT_RUN_AUX_STD, 0);

		BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, SEQ_END);
#ifndef  CM16_BUG
		while (VpuReadReg(BIT_BUSY_FLAG))
			;
#endif
	}

#ifdef  FIXED_0x77_DEAD
	for(i=0;i<10;i++)
	{
		if(VpuReadReg(BIT_BUSY_FLAG))
		{
			gx_mdelay(10);
		} else {
			break;
		}
	}
#else
	VpuWriteReg(BIT_GDI_CLOSE,0x11);
	while((VpuReadReg(BIT_GDI_EMPTY)&0xff) != 0x77);
#endif
#ifdef FIXED_GREEN_DEAD_IN_DEC_CLOSE
	for(i=0;i<10;i++)
	{
		gx_mdelay(20);
		if(0x77 != (VpuReadReg(BIT_GDI_EMPTY)&0xff))
		{
#ifdef  FIXED_0x77_DEAD
			if(i==9){
				gx_printf("VPU_DecClose(busy:%d) when BIT_GDI_EMPTY != 0x77, GREEN DEAD may happen, cur_pc:%x!\n",VpuReadReg(BIT_BUSY_FLAG), VpuReadReg(BIT_CUR_PC));
			}
			continue;
#else
			VpuWriteReg(BIT_GDI_CLOSE,0x11);
			while((VpuReadReg(BIT_GDI_EMPTY)&0xff) != 0x77);
			i = 0;
#endif
		}
#ifdef  FIXED_0x77_DEAD
		else {
			break;
		}
#endif
	}
    Console_Printf(LEVEL_DEBUG, "VPU_DecClose: success\n");
#endif
	FreeCodecInstance(pCodecInst);
	
	return RETCODE_SUCCESS;
}

RetCode VPU_DecSetInitialOpt(
		DecHandle handle)
{
	int i;
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	Uint32 val;
	RetCode ret;

    Console_Printf(LEVEL_DEBUG, "VPU_DecSetInitialOpt(): MSG_0(%08x), MSG_1(%08x), MSG_2(%08x)\n",
                    VpuReadReg(BIT_MSG_0), VpuReadReg(BIT_MSG_1), VpuReadReg(BIT_MSG_2));

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->initialInfoObtained) {
		return RETCODE_CALLED_BEFORE;
	}

	for (i = CMD_DEC_SEQ_OPTION; i <= RET_DEC_SEQ_FRATE_DR; i += 4)
	{
		VpuWriteReg(i,0);
	}

	VpuWriteReg(CMD_DEC_SEQ_BB_START, pDecInfo->streamBufStartAddr&VDEC_HW_ADDR_MASK);
	VpuWriteReg(CMD_DEC_SEQ_BB_SIZE, pDecInfo->streamBufSize / 1024); // size in KBytes

#if DEBUG_MEM_CHECK
    Console_Printf(LEVEL_ALWAYS, "VPU_DecSetInitialOpt(): BB_START(%08x), BB_SIZE(%08x)\n",
                    pDecInfo->streamBufStartAddr&VDEC_HW_ADDR_MASK, pDecInfo->streamBufSize);
#endif

#ifdef API_CR
	val  = 0;
	// MPEG2 and JPEG - To support SEQ_INIT user data report
	if(pCodecInst->codecMode == MP2_DEC || pCodecInst->codecMode == AVC_DEC || pCodecInst->codecMode == HEVC_DEC)
	{
		val |= (pDecInfo->userDataReportMode << 10 ) & 0x400;
		val |= (pDecInfo->userDataEnable	 <<  5 ) & 0x20;
		if(pDecInfo->userDataEnable)
		{
			VpuWriteReg(CMD_DEC_SEQ_USER_DATA_BASE_ADDR, pDecInfo->userDataBufAddr);
			VpuWriteReg(CMD_DEC_SEQ_USER_DATA_BUF_SIZE , pDecInfo->userDataBufSize);
		}

        Console_Printf(LEVEL_DEBUG, "VPU_DecSetInitialOpt():: UserDataRptMode: %d, UserDataRptEn: %d, BaseAddr:%08x, BufSize: %08x\n",
                        pDecInfo->userDataReportMode, pDecInfo->userDataEnable, pDecInfo->userDataBufAddr, pDecInfo->userDataBufSize);
	}
#endif
	val  = (7<<11) &0x7800; //add max_layer_id 3bit (0~7)
	val |= (pDecInfo->openParam.reorderEnable << 1) & 0x2;
	val |= (pDecInfo->openParam.outloopDbkEnable & 0x1);
	VpuWriteReg(CMD_DEC_SEQ_OPTION, val);

    Console_Printf(LEVEL_DEBUG, "VPU_DecSetInitialOpt():: SeqOption: %08x\n", val);

	if( pCodecInst->codecMode == MP4_DEC ) {
		VpuWriteReg(CMD_DEC_SEQ_MP4_ASP_CLASS, pDecInfo->openParam.mp4Class);
        Console_Printf(LEVEL_DEBUG, "VPU_DecSetInitialOpt():: Mp4Class: %d\n", pDecInfo->openParam.mp4Class);
	}
	if( pCodecInst->codecMode == MJPG_DEC ) {
		VpuWriteReg( CMD_DEC_SEQ_JPG_THUMB_EN, pDecInfo->openParam.mjpg_thumbNailDecEnable );

		VpuWriteReg(CMD_DEC_SEQ_SAM_XY, (pDecInfo->mjpgSampX<<16) | pDecInfo->mjpgSampY);
		VpuWriteReg(CMD_DEC_SEQ_CLIP_CNT, 0);
		VpuWriteReg(CMD_DEC_SEQ_CLIP_FROM, 0x370000);
		VpuWriteReg(CMD_DEC_SEQ_CLIP_TO, 0x1f8018c);
		VpuWriteReg(CMD_DEC_SEQ_CLIP_MODE, pDecInfo->mjpgClipMode);
        Console_Printf(LEVEL_DEBUG, "VPU_DecSetInitialOpt():: [JPEG] ThumbNailEn: %d, SubSampleX: %d, SubSampleY: %d, ClipMode: %d\n", 
                                pDecInfo->openParam.mjpg_thumbNailDecEnable, pDecInfo->mjpgSampX, pDecInfo->mjpgSampY, pDecInfo->mjpgClipMode);
	}

	VpuWriteReg( CMD_DEC_SEQ_SRC_SIZE, pDecInfo->picSrcSize );
	if (pDecInfo->openParam.bitstreamFormat == STD_HEVC)
		VpuWriteReg(BIT_CODE_PARAM0, 0x4);

	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		VpuWriteReg(BIT_RUN_AUX_STD, 1);
	else
		VpuWriteReg(BIT_RUN_AUX_STD, 0);

	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, SEQ_INIT);

	return RETCODE_SUCCESS;
}

RetCode VPU_DecClrState(
		DecHandle handle,
		DecInitialInfo * info)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	pDecInfo->initialInfoObtained = 0;
    pDecInfo->frameBufPool = 0;
    pendingInst = 0;
	if(pCodecInst->codecMode == AVC_DEC)
	{
		h264ClrPsBuf(1);
	}

	return RETCODE_SUCCESS;
}


RetCode VPU_DecGetInitialInfo(
		DecHandle handle,
		DecInitialInfo * info)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	Uint32 val, val2;
	RetCode ret;

	static char *std_str[] = {
		"avc" , "hevc", "mp2", "mp4" , "rv " , "avs", "mjpg",
	};
	char *std_strings = NULL;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (info == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->initialInfoObtained) {
		return RETCODE_CALLED_BEFORE;
	}

	if (VpuReadReg(RET_DEC_SEQ_SUCCESS) == 0) {
        Console_Printf(LEVEL_ERROR, "%s:: ret_failure\n", __func__);
		return RETCODE_FAILURE;
    }

	std_strings = std_str[pCodecInst->codecMode];
	Console_Printf(LEVEL_ALWAYS, "\n[VpuApi] Std: %s, Ver: 0x%x\n", std_strings, VpuReadReg(BIT_CODE_VERSION));
	val = VpuReadReg(RET_DEC_SEQ_SRC_SIZE);
	info->picWidth = ( (val >> 16) & 0xffff );
	info->picHeight = ( val & 0xffff );

	info->fRateNumerator	= VpuReadReg(RET_DEC_SEQ_FRATE_NR);
	info->fRateDenominator	= VpuReadReg(RET_DEC_SEQ_FRATE_DR);

	if (pCodecInst->codecMode  == MP4_DEC) {
		val = VpuReadReg(RET_DEC_SEQ_INFO);
		info->mp4_shortVideoHeader = (val >> 2) & 1;
		info->mp4_dataPartitionEnable = val & 1;
		info->mp4_reversibleVlcEnable =
			info->mp4_dataPartitionEnable ?
			((val >> 1) & 1) : 0;
		info->h263_annexJEnable = (val >> 3) & 1;

        info->mp4_numWarpPoints  = (val >> 4) & 7;
        info->h263_annexDEnable  = (val >> 7) & 1;
        info->mp4_fixed_vop_rate = (val >> 8) & 1;
        info->mp4_gmc_flag       = (val >> 9) & 1;//gmc
	}
	if(pCodecInst->codecMode  == HEVC_DEC) {
		int bitdepth_l, bitdepth_c;
		val = VpuReadReg(RET_DEC_SEQ_INFO);
		bitdepth_l = val &  0x7;
		bitdepth_c = (val>>3) & 0x7;
		bitdepth_l = SIZE_ALIGN(bitdepth_l, 2);
		bitdepth_c = SIZE_ALIGN(bitdepth_c, 2);
		info->bitdepth = (bitdepth_l >= bitdepth_c) ? bitdepth_l : bitdepth_c;
		info->picXAign = 64;
		info->picYAign = 64;
	} else {
		info->bitdepth = 0;
		info->picXAign = 16;
		info->picYAign = 32;
	}

	info->minFrameBufferCount = VpuReadReg(RET_DEC_SEQ_FRAME_NEED);
	info->frameBufDelay = VpuReadReg(RET_DEC_SEQ_FRAME_DELAY);

	if ((pCodecInst->codecMode == AVC_DEC) ||(pCodecInst->codecMode == HEVC_DEC))
	{
		val = VpuReadReg(RET_DEC_SEQ_CROP_LEFT_RIGHT);
		val2 = VpuReadReg(RET_DEC_SEQ_CROP_TOP_BOTTOM);
		if( val == 0 && val2 == 0 )
		{
			info->picCropRect.left = 0;
			info->picCropRect.right = 0;
			info->picCropRect.top = 0;
			info->picCropRect.bottom = 0;
		}
		else
		{
			info->picCropRect.left = ( (val>>16) & 0xFFFF );
			info->picCropRect.right = info->picWidth - ( ( val & 0xFFFF ) );
			info->picCropRect.top = ( (val2>>16) & 0xFFFF );
			info->picCropRect.bottom = info->picHeight - ( ( val2 & 0xFFFF ) );
		}
		
		val = (info->picWidth * info->picHeight * 3 / 2) / 1024;
		info->normalSliceSize = val / 4;
		info->worstSliceSize = val / 2;
	}
	else
	{
		info->picCropRect.left = 0;
		info->picCropRect.right = 0;
		info->picCropRect.top = 0;
		info->picCropRect.bottom = 0;
	}
	if (pCodecInst->codecMode == MJPG_DEC)
	{
		info->mjpg_thumbNailEnable = ( VpuReadReg(RET_DEC_SEQ_JPG_THUMB_IND) & 0x01 );
		info->mjpg_sourceFormat = ( VpuReadReg(RET_DEC_SEQ_FRAME_FORMAT) & 0x07 );
		if (pDecInfo->openParam.mjpg_thumbNailDecEnable == 1)
			if (info->mjpg_thumbNailEnable == 0)
					return RETCODE_FAILURE;

#ifdef JPEG_DOWNSAMPLE_WORKAROUND
		info->outWidth = info->picWidth;
		info->outHeight = info->picHeight;

		if (pDecInfo->mjpgSampX) {
			info->outWidth = info->picWidth>>pDecInfo->mjpgSampX;
			info->outHeight = info->picHeight>>pDecInfo->mjpgSampY;

		info->picWidth = (info->picWidth>>pDecInfo->mjpgSampX);
		if (info->picWidth & 0xf) {
				info->picWidth >>= 3;
				if (info->mjpg_sourceFormat == MODE420 || info->mjpg_sourceFormat == MODE422)
					info->picWidth >>= 1;

			info->picWidth++;
				info->picWidth <<= 3;
				if (info->mjpg_sourceFormat == MODE420 || info->mjpg_sourceFormat == MODE422)
					info->picWidth <<= 1;
			}
		}
#else
		if (info->mjpg_sourceFormat == MODE444 || 
			info->mjpg_sourceFormat == MODE224 || 
			info->mjpg_sourceFormat == MODE400)
		    info->picWidth = (info->picWidth>>pDecInfo->mjpgSampX) & 0xfff8;
		else
		    info->picWidth = (info->picWidth>>pDecInfo->mjpgSampX) & 0xfff0;
#endif // JPEG_DOWNSAMPLE_WORKAROUND
		info->picHeight = info->picHeight>>pDecInfo->mjpgSampY;
	}

	pDecInfo->initialInfo = *info;
	pDecInfo->initialInfoObtained = 1;

#ifdef API_CR
    val = VpuReadReg(RET_DEC_SEQ_HEADER_REPORT);
	info->profile     = (val >>  0) & 0xFF;
	info->level       = (val >>  8) & 0xFF;
	info->progressive = (val >> 16) & 0x01;
	info->tier        = (val >> 23) & 0x01;
	if (pCodecInst->codecMode == HEVC_DEC) {
		info->chroma_idc  = (val >> 24) & 0x07;     //1--YUV420(support), other--not support
		info->hevcFieldSeqFlag = (val >> 16) & 0x01;//0--frame 1--field
	}
	else {
		info->hevcFieldSeqFlag = 0;
		info->chroma_idc = YUV420;                    //1--YUV420(support), other standard--default support
	}
	info->direct8x8Flag          = (val >> 17) & 0x01;
	info->constraint_set_flag[0] = (val >> 22) & 0x01;
	info->constraint_set_flag[1] = (val >> 21) & 0x01;
	info->constraint_set_flag[2] = (val >> 20) & 0x01;
	info->constraint_set_flag[3] = (val >> 19) & 0x01;
	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		info->progressive = 1;

	val = VpuReadReg(RET_DEC_SEQ_ASPECT);

	info->aspectRateInfo = val;

	val = VpuReadReg(RET_DEC_SEQ_BIT_RATE);
	info->bitRate = val;
#endif

    Console_Printf(LEVEL_DEBUG, "VPU_DecGetInitialInfo():: ret_success\n" );
#if DEBUG_MEM_CHECK
    Console_Printf(LEVEL_ALWAYS, "Width(%d) x Height(%d)\n", info->picWidth, info->picHeight);
#endif
    Console_Printf(LEVEL_DEBUG, "FrameRateInfo: Numerator(%d) x Denominator(%d)\n", info->fRateNumerator, info->fRateDenominator);
    Console_Printf(LEVEL_DEBUG, "MinNumFrameBufNum: %d\n", info->minFrameBufferCount);
    Console_Printf(LEVEL_DEBUG, "AspectRatioInfo: %d, BitRateInfo: %d\n", info->aspectRateInfo, info->bitRate);
    Console_Printf(LEVEL_DEBUG, "Profile: %d, Level: %d, Progressive: %d, Direct8x8: %d, ConstraintFlag[%d/%d/%d/%d]\n",
	                        info->profile, info->level, info->progressive, info->direct8x8Flag, 
                            info->constraint_set_flag[0], info->constraint_set_flag[1], info->constraint_set_flag[2], info->constraint_set_flag[3]);
    Console_Printf(LEVEL_DEBUG, "JpgThumbNailEn: %d, SourceFormat(Chroma): %d\n", info->mjpg_thumbNailEnable, info->mjpg_sourceFormat);
    Console_Printf(LEVEL_DEBUG, "[CropInfo] Left: %d, Right: %d, Top: %d, Bottom: %d\n", 
	                        info->picCropRect.left, info->picCropRect.right, info->picCropRect.top, info->picCropRect.bottom);
    Console_Printf(LEVEL_DEBUG, "[MP4] ShortVideoHeadFlag: %d, DataPartFlag: %d, RvlcFlag: %d, AnnexJFlag: %d\n", 
	                        info->mp4_shortVideoHeader, info->mp4_dataPartitionEnable, info->mp4_reversibleVlcEnable, info->h263_annexJEnable);
    Console_Printf(LEVEL_DEBUG, "MSG_0(%08x), MSG_1(%08x), MSG_2(%08x)\n", VpuReadReg(BIT_MSG_0), VpuReadReg(BIT_MSG_1), VpuReadReg(BIT_MSG_2));
    Console_Printf(LEVEL_DEBUG, "rdPtr: %08x, wrPtr: %08x, eRdPtr: %08x, eRdPtrPrev: %08x\n",
	                VpuReadReg(rdPtrRegAddr[pCodecInst->instIndex]), VpuReadReg(wrPtrRegAddr[pCodecInst->instIndex]), VpuReadReg(BIT_EXACT_RD_PTR), VpuReadReg(BIT_EXACT_RD_PTR_PREV));

#ifdef ADD_SEQ_ERR_RETURN
    if(pCodecInst->codecMode == AVC_DEC)
    {
	//maxLevel4.1: 12288*1024/384/picWidthInMbs/picHeightInMbs
	int picWidthInMbs = (info->picWidth + 15) >> 4;
	int picHeightInMbs = (info->picHeight + 15) >> 4;
	int maxFrameBuffer = 32768/(picWidthInMbs*picHeightInMbs);
	maxFrameBuffer = (maxFrameBuffer > 31) ? 31 : maxFrameBuffer;
	if(info->minFrameBufferCount > (maxFrameBuffer + 3))
	{
		gx_printf("[SEQ][Fail]:%dx%d, minFrameBuf:%d > maxFrameBuffer:%d + 2\n",
				  info->picWidth, info->picHeight, info->minFrameBufferCount, maxFrameBuffer);
		return RETCODE_FAILURE;
	}
    }
#endif
	return RETCODE_SUCCESS;
}

RetCode VPU_DecRegisterFrameBuffer(
		DecHandle handle,
		FrameBuffer * bufArray,
		int num,
		int stride,
		DecBufInfo * pBufInfo)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	int i;
	RetCode ret;

#ifdef	EVB_BUILD
    BYTE frameAddr[32][3][4];
    BYTE colMvAddr[32][4];
#endif

    Console_Printf(LEVEL_DEBUG, "VPU_DecRegisterFrameBuffer()\n" );

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->frameBufPool) {
		return RETCODE_CALLED_BEFORE;
	}

	if (!pDecInfo->initialInfoObtained) {
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	if (bufArray == 0) {
		return RETCODE_INVALID_FRAME_BUFFER;
	}
	if (num < pDecInfo->initialInfo.minFrameBufferCount) {
		return RETCODE_INSUFFICIENT_FRAME_BUFFERS;
	} 
	if (stride < pDecInfo->initialInfo.picWidth ||
			stride % 8 != 0
	   ) {
		return RETCODE_INVALID_STRIDE;
	}

	pDecInfo->frameBufPool = bufArray;
	pDecInfo->numFrameBuffers = num;
	pDecInfo->stride = stride;

	// Skip frame buffer setting in MJPEG
	// It will be set before picture decoding
	if (pDecInfo->openParam.bitstreamFormat == STD_MJPG)
		return RETCODE_SUCCESS;

#ifndef EVB_BUILD	

	// Let the decoder know the addresses of the frame buffers.
	for (i = 0; i < 32; ++i) {
		VpuWriteReg(paraBuffer + i * 3 * 4, bufArray[i].bufY);
		VpuWriteReg(paraBuffer + i * 3 * 4 + 4, bufArray[i].bufCb);
		VpuWriteReg(paraBuffer + i * 3 * 4 + 8, bufArray[i].bufCr);
		if( pDecInfo->openParam.bitstreamFormat == STD_AVC ||
                pDecInfo->openParam.bitstreamFormat == STD_HEVC)
			VpuWriteReg(paraBuffer + ( i + 96 ) * 4, bufArray[i].bufMvCol);
	}
	if(pDecInfo->openParam.bitstreamFormat ==STD_MPEG4)
		VpuWriteReg(paraBuffer + 384, bufArray[0].bufMvCol);
#else

	// Let the decoder know the addresses of the frame buffers.
	for (i=0; i<32; i++) {
		frameAddr[i][0][0] = (bufArray[i].bufY  >> 24) & 0xFF;
        frameAddr[i][0][1] = (bufArray[i].bufY  >> 16) & 0xFF;
        frameAddr[i][0][2] = (bufArray[i].bufY  >>  8) & 0xFF;
        frameAddr[i][0][3] = (bufArray[i].bufY  >>  0) & 0xFF;
        frameAddr[i][1][0] = (bufArray[i].bufCb >> 24) & 0xFF;
        frameAddr[i][1][1] = (bufArray[i].bufCb >> 16) & 0xFF;
        frameAddr[i][1][2] = (bufArray[i].bufCb >>  8) & 0xFF;
        frameAddr[i][1][3] = (bufArray[i].bufCb >>  0) & 0xFF;
        frameAddr[i][2][0] = (bufArray[i].bufCr >> 24) & 0xFF;
        frameAddr[i][2][1] = (bufArray[i].bufCr >> 16) & 0xFF;
        frameAddr[i][2][2] = (bufArray[i].bufCr >>  8) & 0xFF;
        frameAddr[i][2][3] = (bufArray[i].bufCr >>  0) & 0xFF;

		if ( pDecInfo->openParam.bitstreamFormat == STD_AVC ||
                pDecInfo->openParam.bitstreamFormat == STD_HEVC) {
            colMvAddr[i][0] = (bufArray[i].bufMvCol >> 24) & 0xFF;
            colMvAddr[i][1] = (bufArray[i].bufMvCol >> 16) & 0xFF;
            colMvAddr[i][2] = (bufArray[i].bufMvCol >>  8) & 0xFF;
            colMvAddr[i][3] = (bufArray[i].bufMvCol >>  0) & 0xFF;
        }
#if DEBUG_MEM_CHECK
		Console_Printf(LEVEL_ALWAYS, "F[%d]Y:0x%08x, Cb:0x%08x, Col:0x%08x\n", i, bufArray[i].bufY, bufArray[i].bufCb, bufArray[i].bufMvCol);
#endif
	}

	if (pDecInfo->openParam.bitstreamFormat == STD_MPEG4 ||
			pDecInfo->openParam.bitstreamFormat == STD_AVS ||
			pDecInfo->openParam.bitstreamFormat == STD_RV) {
        colMvAddr[0][0] = (bufArray[0].bufMvCol >> 24) & 0xFF;
        colMvAddr[0][1] = (bufArray[0].bufMvCol >> 16) & 0xFF;
        colMvAddr[0][2] = (bufArray[0].bufMvCol >>  8) & 0xFF;
        colMvAddr[0][3] = (bufArray[0].bufMvCol >>  0) & 0xFF;
	}
	
	WriteSdramBurst((BYTE*)frameAddr, paraBuffer,     sizeof(frameAddr), 1);

	if (pDecInfo->openParam.bitstreamFormat == STD_AVC || 
			pDecInfo->openParam.bitstreamFormat == STD_HEVC || 
			pDecInfo->openParam.bitstreamFormat == STD_MPEG4 ||
			pDecInfo->openParam.bitstreamFormat == STD_RV ||
			pDecInfo->openParam.bitstreamFormat == STD_AVS)
		WriteSdramBurst((BYTE*)colMvAddr, paraBuffer+384, sizeof(colMvAddr), 1);
#endif

	// Tell the decoder how much frame buffers were allocated.
	VpuWriteReg(CMD_SET_FRAME_BUF_NUM, num);
	VpuWriteReg(CMD_SET_FRAME_BUF_STRIDE, stride);


#if 1
	{
		int preAddr = 0;
		int axiAddr = 0;
		int MbNumX;
		int hevc_MbNumX;
		int hevc_MbNumY;
		static int axiLimit = 131072 ; 
	
		//Hscale ?
		int oneBTP	= 0;
		int axiValue = 0;
		int MbNumY = ((pDecInfo->initialInfo.picHeight& 0xFFFF) + 15) / 16;

		MbNumX = (pDecInfo->initialInfo.picWidth+15)/16 ;
		hevc_MbNumX = (1920)/16 ;
		hevc_MbNumY = (1088)/16 ;
		// Sec. AXI Base
		//BIT
		if (pDecInfo->secAxiUse.useBitEnable){
			//axiAddr = axiAddr + MbNumX * 128;
			switch (pDecInfo->openParam.bitstreamFormat) 
			{
				case STD_AVC     : axiAddr = axiAddr + MbNumX * 128; break; // AVC
				case STD_RV       : axiAddr = axiAddr + MbNumX * 128; break; // RV
				case STD_AVS     : axiAddr = axiAddr + MbNumX *  128; break; // AVS
				case STD_MPEG2 : axiAddr = axiAddr + MbNumX *   0; break; // MPEG-2
				case STD_HEVC    : axiAddr = axiAddr + hevc_MbNumX *88 + hevc_MbNumY * 48 ; break; // 
				default  : axiAddr = axiAddr + MbNumX *  16; break; // MPEG-4, Divx3
			}
			if(axiAddr < axiLimit){
				VpuWriteReg( CMD_SET_FRAME_AXI_BIT_ADDR,preAddr&VDEC_HW_ADDR_MASK);
				preAddr = axiAddr;
				axiValue = axiValue | 0x0101;
			}
			else{
				axiAddr = preAddr;
				axiValue = axiValue & 0xfefe; // turn off BIT's flag  //add by huxq
			}
		}
		//Intra Prediction, ACDC
		if(pDecInfo->secAxiUse.useIpEnable){
			//axiAddr = axiAddr + MbNumX * 64;
			switch (pDecInfo->openParam.bitstreamFormat) 
			{
				case STD_AVC     : axiAddr = axiAddr + MbNumX *  64; break;
				case STD_RV       : axiAddr = axiAddr + MbNumX *  64; break;
				case STD_AVS     : axiAddr = axiAddr + MbNumX *  64; break;
				case STD_MPEG2 : axiAddr = axiAddr + MbNumX *   0; break;
				case STD_HEVC : axiAddr = axiAddr + hevc_MbNumX * 32; break;
				default                : axiAddr = axiAddr + MbNumX * 128; break;
			}
			if(axiAddr < axiLimit){
				VpuWriteReg( CMD_SET_FRAME_AXI_IPACDC_ADDR,preAddr&VDEC_HW_ADDR_MASK);
				preAddr = axiAddr;
				axiValue = axiValue | 0x0202;
			}
			else{
				axiAddr = preAddr;
				axiValue = axiValue & 0xfdfd; // turn off IP's flag
			}
		}
		//Deblock Chroma
		if(pDecInfo->secAxiUse.useDbkCEnable){
			//axiAddr = axiAddr + MbNumX * 128;
			switch (pDecInfo->openParam.bitstreamFormat) 
			{
				case STD_AVC   : axiAddr = axiAddr + MbNumX * 128; break;
				case STD_RV     : axiAddr = axiAddr + MbNumX * 128; break;
				case STD_AVS   : axiAddr = axiAddr + MbNumX * 128; break;
				case STD_MPEG2   : axiAddr = axiAddr + MbNumX *  64; break;
				case STD_HEVC   : axiAddr = axiAddr + hevc_MbNumX *  88 ; break;
				default  : axiAddr = axiAddr + MbNumX *  64; break;
			}
			if(axiAddr < axiLimit){
				VpuWriteReg( CMD_SET_FRAME_AXI_DBKC_ADDR,preAddr&VDEC_HW_ADDR_MASK);
				preAddr = axiAddr;
				axiValue = axiValue | 0x0808;
			}
			else{
				axiAddr = preAddr;
				axiValue = axiValue & (~0x0808);
			}
		}
		//Deblock Luma
		if(pDecInfo->secAxiUse.useDbkYEnable){
			//axiAddr = axiAddr + MbNumX * 128;
			switch (pDecInfo->openParam.bitstreamFormat) 
			{
				case STD_AVC       : axiAddr = axiAddr + MbNumX * 128; break;
				case STD_RV         : axiAddr = axiAddr + MbNumX * 128; break;
				case STD_AVS       : axiAddr = axiAddr + MbNumX * 128; break;
				case STD_MPEG2   : axiAddr = axiAddr + MbNumX * 128; break;
				case STD_HEVC   : axiAddr = axiAddr + hevc_MbNumX *  72 ; break;
				default  : axiAddr = axiAddr + MbNumX * 128; break;
			}
			if(axiAddr < axiLimit){
				VpuWriteReg( CMD_SET_FRAME_AXI_DBKY_ADDR,preAddr&VDEC_HW_ADDR_MASK);
				preAddr = axiAddr;
				axiValue = axiValue | 0x0404;		
			}
			else{
				axiAddr = preAddr;
				axiValue = axiValue & 0xfbfb;
			}
		}

		if(pDecInfo->secAxiUse.useBtpEnable){
			if((axiAddr&0x000000FF) !=0){
				axiAddr = axiAddr + 256 ;
				axiAddr = axiAddr&0xFFFFFF00 ;
			}
			if((preAddr&0x000000FF) !=0){
				preAddr = preAddr + 256 ;
				preAddr = preAddr&0xFFFFFF00 ;
			}
			oneBTP  = ((MbNumX/16) * MbNumY + 1) * 2;
			oneBTP  = (oneBTP%256) ? ((oneBTP/256)+1)*256 : oneBTP;
			//axiAddr = axiAddr + MbNumX * 0;
			switch (pDecInfo->openParam.bitstreamFormat) 
			{
				case STD_AVC       : axiAddr = axiAddr + MbNumX * 0; break; 
				case STD_RV         : axiAddr = axiAddr + MbNumX * 0; break;
				case STD_AVS       : axiAddr = axiAddr + MbNumX * 0; break;
				case STD_MPEG2   : axiAddr = axiAddr + MbNumX * 0; break;
				case STD_HEVC   : axiAddr = axiAddr + hevc_MbNumY * 152; break;
				default                  : axiAddr = axiAddr + MbNumX * 0; break;
			}
			if(axiAddr < axiLimit){
				VpuWriteReg( CMD_SET_FRAME_AXI_BTP_ADDR,preAddr&VDEC_HW_ADDR_MASK);
				preAddr = axiAddr;
				axiValue = axiValue | 0x2020;		
			}
			else{
				axiAddr = preAddr;
				axiValue = axiValue & 0xdfdf;
			}
		}

		if(pDecInfo->secAxiUse.useOvlEnable){
			//axiAddr = axiAddr + MbNumX * 0;
			switch (pDecInfo->openParam.bitstreamFormat) 
			{
				case STD_AVC       : axiAddr = axiAddr + MbNumX *   0; break;
				case STD_RV         : axiAddr = axiAddr + MbNumX *   0; break;
				case STD_AVS       : axiAddr = axiAddr + MbNumX *   0; break;
				case STD_MPEG2   : axiAddr = axiAddr + MbNumX *   0; break;
				case STD_HEVC   : axiAddr = axiAddr + hevc_MbNumY *   136; break;
				default  : axiAddr = axiAddr + MbNumX *   0; break;
			}
			if(axiAddr < axiLimit){
				VpuWriteReg( CMD_SET_FRAME_AXI_OVL_ADDR,preAddr&VDEC_HW_ADDR_MASK);
				preAddr = axiAddr;
				axiValue = axiValue | 0x1010;			
			}
			else{
				axiAddr = preAddr;
				axiValue = axiValue & 0xefef;
			}
		}
		VpuWriteReg(BIT_AXI_SRAM_USE, axiValue);	//
	}
#else 
	VpuWriteReg( CMD_SET_FRAME_AXI_BIT_ADDR, pDecInfo->secAxiUse.bufBitUse);
	VpuWriteReg( CMD_SET_FRAME_AXI_IPACDC_ADDR, pDecInfo->secAxiUse.bufIpAcDcUse);
	VpuWriteReg( CMD_SET_FRAME_AXI_DBKY_ADDR, pDecInfo->secAxiUse.bufDbkYUse);
	VpuWriteReg( CMD_SET_FRAME_AXI_DBKC_ADDR, pDecInfo->secAxiUse.bufDbkCUse);
	VpuWriteReg( CMD_SET_FRAME_AXI_OVL_ADDR, pDecInfo->secAxiUse.bufOvlUse);
	VpuWriteReg( CMD_SET_FRAME_AXI_BTP_ADDR, pDecInfo->secAxiUse.bufBtpUse);
	VpuWriteReg( CMD_SET_FRAME_MAX_DEC_SIZE, 0);
#endif
	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		VpuWriteReg(BIT_RUN_AUX_STD, 1);
	else
		VpuWriteReg(BIT_RUN_AUX_STD, 0);

	gx_dcache_clean_range(0, 0);
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, SET_FRAME_BUF);

	while (VpuReadReg(BIT_BUSY_FLAG))
		;

    Console_Printf(LEVEL_DEBUG, "FrameBufNum: %d, Stride: %d\n", num, stride);
    Console_Printf(LEVEL_DEBUG, "[2ndAXI] BIT: %08x, AC/DC: %08x, DBK-Y: %08x, DBK-C: %08x, OVL: %08x, BTP: %08x\n", 
	                pDecInfo->secAxiUse.bufBitUse, pDecInfo->secAxiUse.bufIpAcDcUse, pDecInfo->secAxiUse.bufDbkYUse, 
	                pDecInfo->secAxiUse.bufDbkCUse, pDecInfo->secAxiUse.bufOvlUse, pDecInfo->secAxiUse.bufBtpUse); 
    Console_Printf(LEVEL_DEBUG, "FrameDispFlag: %08x\n", disFlagRegAddr[pCodecInst->instIndex]);
    Console_Printf(LEVEL_DEBUG, "MSG_0(%08x), MSG_1(%08x), MSG_2(%08x)\n", VpuReadReg(BIT_MSG_0), VpuReadReg(BIT_MSG_1), VpuReadReg(BIT_MSG_2));

	return RETCODE_SUCCESS;
}

RetCode VPU_DecGetBitstreamBuffer( DecHandle handle,
		PhysicalAddress * prdPrt,
		PhysicalAddress * pwrPtr,
		Uint32 * size)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	PhysicalAddress rdPtr;
	PhysicalAddress wrPtr;
	Uint32 room;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if ( prdPrt == 0 || pwrPtr == 0 || size == 0) {
		return RETCODE_INVALID_PARAM;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	rdPtr = VpuReadReg(pDecInfo->streamRdPtrRegAddr)| GX_DDR_BASE_ADDR;
	wrPtr = pDecInfo->streamWrPtr;
	
	if (wrPtr < rdPtr) {
		room = rdPtr - wrPtr - 1;
	}
	else {
		room = ( pDecInfo->streamBufEndAddr - wrPtr ) + ( rdPtr - pDecInfo->streamBufStartAddr ) - 1;	
	}

	*prdPrt = rdPtr;
	*pwrPtr = wrPtr;
	*size = room;
	
    Console_Printf(LEVEL_DEBUG, "VPU_DecGetBitstreamBuffer(): rdPtr: %08x, wrPtr: %08x, eRdPtr: %08x, eRdPtrPrev: %08x\n",
	                rdPtr, wrPtr, VpuReadReg(BIT_EXACT_RD_PTR), VpuReadReg(BIT_EXACT_RD_PTR_PREV));
    Console_Printf(LEVEL_DEBUG, "MSG_0(%08x), MSG_1(%08x), MSG_2(%08x)\n",
                    VpuReadReg(BIT_MSG_0), VpuReadReg(BIT_MSG_1), VpuReadReg(BIT_MSG_2));

	return RETCODE_SUCCESS;
}


RetCode VPU_DecUpdateBitstreamBuffer(
		DecHandle handle,
		Uint32 size)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	PhysicalAddress wrPtr;
	PhysicalAddress rdPtr;
	RetCode ret;
	int		val = 0;
	int		room = 0;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	wrPtr = pDecInfo->streamWrPtr;

	//gx_printf("rd = 0x%x wr = 0x%x size = 0x%x\n",VpuReadReg(pDecInfo->streamRdPtrRegAddr)| GX_DDR_BASE_ADDR,wrPtr,size);
	if (size == 0) 
	{
		val = VpuReadReg( BIT_BIT_STREAM_PARAM );
		val |= 1 << ( pCodecInst->instIndex + 2);
		VpuWriteReg(BIT_BIT_STREAM_PARAM, val);
        Console_Printf(LEVEL_DEBUG, "VPU_DecUpdateBitstreamBuffer(): StreamEnd value: %08x\n", val);
		return RETCODE_SUCCESS;
	}

	rdPtr = VpuReadReg(pDecInfo->streamRdPtrRegAddr)| GX_DDR_BASE_ADDR;
	if (wrPtr < rdPtr) {
		if (rdPtr <= wrPtr + size)
			return RETCODE_INVALID_PARAM;
	}
	wrPtr += size;

	if (wrPtr > pDecInfo->streamBufEndAddr) {
		room = wrPtr - pDecInfo->streamBufEndAddr;
		wrPtr = pDecInfo->streamBufStartAddr;
		wrPtr += room;
	}
	if (wrPtr == pDecInfo->streamBufEndAddr) {
		wrPtr = pDecInfo->streamBufStartAddr;
	}

	pDecInfo->streamWrPtr = wrPtr;
	VpuWriteReg(pDecInfo->streamWrPtrRegAddr, wrPtr&VDEC_HW_ADDR_MASK);

    Console_Printf(LEVEL_DEBUG, "VPU_DecUpdateBitstreamBuffer(): rdPtr: %08x, wrPtr: %08x, eRdPtr: %08x, eRdPtrPrev: %08x\n",
	                rdPtr, wrPtr, VpuReadReg(BIT_EXACT_RD_PTR), VpuReadReg(BIT_EXACT_RD_PTR_PREV));
    Console_Printf(LEVEL_DEBUG, "MSG_0(%08x), MSG_1(%08x), MSG_2(%08x)\n",
                    VpuReadReg(BIT_MSG_0), VpuReadReg(BIT_MSG_1), VpuReadReg(BIT_MSG_2));

	return RETCODE_SUCCESS;
}

//---- VPU_SLEEP/WAKE
RetCode VPU_SleepWake(DecHandle handle, int iSleepWake)
{
	CodecInst	*pCodecInst ;
	static unsigned int regBk[64];
	int i=0;

	int	RunIndex, RunCodStd ;
	
	pCodecInst	= handle ;
	RunIndex	= pCodecInst->instIndex ;
	RunCodStd	= pCodecInst->codecMode ;

    if (iSleepWake)
        Console_Printf(LEVEL_DEBUG, "VPU_Sleep()\n" );
    else
        Console_Printf(LEVEL_DEBUG, "VPU_Wake()\n" );

	if(iSleepWake==1) {
		for ( i = 0 ; i < 64 ; i++)
			regBk[i] = VpuReadReg(BIT_BASE + 0x100 + (i * 4));
		BitIssueCommand(RunIndex, RunCodStd, VPU_SLEEP);
	}
	else {
		if (VpuReadReg(BIT_CUR_PC) != 0) {
			BitIssueCommand(RunIndex, RunCodStd, VPU_WAKE);
			return RETCODE_SUCCESS;
		}

		for ( i = 0 ; i < 64 ; i++)
			VpuWriteReg(BIT_BASE + 0x100 + (i * 4), regBk[i]);
//		VpuWriteReg(BIT_RESET_CTRL,	   1);
		VpuWriteReg(BIT_CODE_RUN,	   0);
		//---- LOAD BOOT CODE
		{	Uint32 data;
			for( i = 0; i <2048; i++ ) {
			   data = bitcode.code[i];
			   VpuWriteReg(BIT_CODE_DOWN, (i << 16) | data);
			}
		}
		VpuWriteReg(BIT_BUSY_FLAG, 1);
		VpuWriteReg(BIT_CODE_RUN, 1);

 		while (VpuReadReg(BIT_BUSY_FLAG));
 			//Sleep( 1 );

		BitIssueCommand(RunIndex, RunCodStd, VPU_WAKE);
	}

	while( VPU_IsBusy() ) {
		;//Sleep( 1 );
	}

	return RETCODE_SUCCESS;
}

void Vpu_DecSetHostParaAddr(PhysicalAddress baseAddr, PhysicalAddress paraAddr)
{
	BYTE tempBuf[8];					// 64bit bus & endian
	Uint32 val;

    Console_Printf(LEVEL_DEBUG, "Vpu_DecSetHostParaAddr()\n" );

	val = paraAddr;
 	tempBuf[0] = 0;
 	tempBuf[1] = 0;
 	tempBuf[2] = 0;
 	tempBuf[3] = 0;
	tempBuf[4] = (val >> 24) & 0xff;
	tempBuf[5] = (val >> 16) & 0xff;
	tempBuf[6] = (val >> 8) & 0xff;
	tempBuf[7] = (val >> 0) & 0xff;
	WriteSdramBurst((BYTE*)tempBuf, baseAddr,  8, 1);
}

RetCode VPU_DecStartOneFrame(DecHandle handle, DecParam *param, void *vd)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	Uint32 rotMir;
	Uint32 val;
	RetCode ret;
	struct vd_chip_media *chip_media = vd;

    Console_Printf(LEVEL_DEBUG, "VPU_DecStartOneFrame()\n" );

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->frameBufPool == 0) { // This means frame buffers have not been registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	rotMir = 0;
	if (pDecInfo->rotationEnable) {
		rotMir |= 0x10; // Enable rotator
		switch (pDecInfo->rotationAngle) {
			case 0:
				rotMir |= 0x0;
				break;

			case 90:
				rotMir |= 0x1;
				break;

			case 180:
				rotMir |= 0x2;
				break;

			case 270:
				rotMir |= 0x3;
				break;
		}
	}
	if (pDecInfo->mirrorEnable) {
		rotMir |= 0x10; // Enable rotator
		switch (pDecInfo->mirrorDirection) {
			case MIRDIR_NONE :
				rotMir |= 0x0;
				break;

			case MIRDIR_VER :
				rotMir |= 0x4;
				break;

			case MIRDIR_HOR :
				rotMir |= 0x8;
				break;

			case MIRDIR_HOR_VER :
				rotMir |= 0xc;
				break;

		}
	}
	if (pDecInfo->deringEnable) {
		rotMir |= 0x20; // Enable Dering Filter
	}

	// Set frame buffer for MJPEG or Post-processor
	// In case of MJPEG, rotate function exist in it.
	// So, all result data written a frame buffer which set for Post-processor.
	if (rotMir & 0x30 || pDecInfo->openParam.bitstreamFormat == STD_MJPG) // rotator or dering enabled
	{	
		int str_int_fmt;
		int bus_info_addr; 

		Uint32 align_pic_x, align_pic_y;
		Uint32 rot_size_xy;
		Uint32 InterLeave;

		InterLeave = VpuReadReg(BIT_FRAME_MEM_CTRL); // {Interleave[0], endian[1:0]}
		InterLeave = InterLeave >> 2;
		//printf("interleave = %d  !interleave = %x \n", InterLeave, (InterLeave << 14) );
		
		VpuWriteReg(CMD_DEC_PIC_ROT_MODE	, rotMir);				
		if (pDecInfo->openParam.bitstreamFormat == STD_MJPG){
			VpuWriteReg(CMD_DEC_PIC_ROT_INDEX	, (!InterLeave << 16));	// CrFrmIdx[7:0], CbFrmIdx[7:0], YFrmIdx[7:0]
			rot_size_xy = ReadReg(CMD_DEC_PIC_ROT_INDEX);
			//printf("PIC_ROT_INDEX : 0x%x \n", rot_size_xy);
		}
		else
			VpuWriteReg(CMD_DEC_PIC_ROT_INDEX, pDecInfo->rotIndex);
		
		// Set PIC_INFO
		VpuWriteReg(GDI_CONTROL, 1);

		//str_int_fmt : {YuvFormat[19:17], Interleave[16], Stride[15:0] }
		if (pDecInfo->openParam.bitstreamFormat == STD_MJPG) {
			int GdiFormat = pDecInfo->initialInfo.mjpg_sourceFormat;
			if (pDecInfo->rotationAngle == 90 || pDecInfo->rotationAngle == 270) {
				if (GdiFormat == 2 || GdiFormat == 1)
					GdiFormat ^= 3;
			}
			str_int_fmt = (GdiFormat & 0x7) << 17; // YUV format 
		} else 
			str_int_fmt = 0;

		str_int_fmt = str_int_fmt | (InterLeave << 16);	         //uv interleave 
		str_int_fmt = str_int_fmt | (pDecInfo->rotatorStride & 0xFFFF); //stride

		align_pic_x = (pDecInfo->initialInfo.picWidth + 15) & ~15;
		align_pic_y = (pDecInfo->initialInfo.picHeight + 15) & ~15;

		if(rotMir & 0x1) // 90 or 270 degree
			rot_size_xy = ((align_pic_y << 16) + align_pic_y);
		else 
			rot_size_xy = ((align_pic_x << 16) + align_pic_y);

		if (pDecInfo->openParam.bitstreamFormat == STD_MJPG)
			bus_info_addr = 0 * 5 * 4; // rotIdx == 0
		else 
			bus_info_addr = pDecInfo->numFrameBuffers * 5 * 4; // reference + 1

#ifdef	GDI_IF
		VpuWriteReg(GDI_INFO_CONTROL  , str_int_fmt);
		VpuWriteReg(GDI_INFO_PIC_SIZE , rot_size_xy);
		VpuWriteReg(GDI_INFO_BASE_Y   , pDecInfo->rotatorOutput.bufY);
        VpuWriteReg(GDI_INFO_BASE_CB  , pDecInfo->rotatorOutput.bufCb);
        VpuWriteReg(GDI_INFO_BASE_CR  , pDecInfo->rotatorOutput.bufCr);
#endif

		VpuWriteReg(GDI_CONTROL			, 0);
	}else {
		VpuWriteReg(CMD_DEC_PIC_ROT_MODE	, 0);		
	}

#ifdef API_CR
	if (pDecInfo->mbParamEnable || pDecInfo->mvReportEnable || pDecInfo->frameBufStatEnable) 
	{
		VpuWriteReg(CMD_DEC_PIC_PARA_BASE_ADDR, pDecInfo->picParaBaseAddr);

	Vpu_DecSetHostParaAddr(pDecInfo->picParaBaseAddr		, pDecInfo->frameBufStateAddr);
	Vpu_DecSetHostParaAddr(pDecInfo->picParaBaseAddr + 8	, pDecInfo->mbParamDataAddr);
	Vpu_DecSetHostParaAddr(pDecInfo->picParaBaseAddr + 16	, pDecInfo->mvDataAddr);
	}

	if(pDecInfo->userDataEnable) {
		VpuWriteReg(CMD_DEC_PIC_USER_DATA_BASE_ADDR, pDecInfo->userDataBufAddr);
		VpuWriteReg(CMD_DEC_PIC_USER_DATA_BUF_SIZE, pDecInfo->userDataBufSize);
	}
#endif

	val = 0;
	if( param->iframeSearchEnable == 1 ) { // if iframeSearch is Enable, other bit is ignore;
		val |= (pDecInfo->userDataReportMode		<<10 );
		val |= (pDecInfo->frameBufStatEnable		<< 8 );
		val |= (pDecInfo->mbParamEnable				<< 7 );
		val |= (pDecInfo->mvReportEnable			<< 6 );
		val |= (( param->iframeSearchEnable &0x1)   << 2 );
		val |= (pDecInfo->userDataEnable			<< 5 );
	} else {
#ifdef API_CR	
		if (param->skipframeMode) {
			val |= (pDecInfo->userDataReportMode		<<10 );
			val |= (pDecInfo->frameBufStatEnable		<< 8 );
			val |= (pDecInfo->mbParamEnable				<< 7 );
			val |= (pDecInfo->mvReportEnable			<< 6 );
			val |= (param->skipframeMode				<< 3 );			
			val |= (pDecInfo->userDataEnable			<< 5 );
		} else {
			val |= (pDecInfo->userDataReportMode		<<10 );
			val |= (pDecInfo->frameBufStatEnable		<< 8 );
			val |= (pDecInfo->mbParamEnable				<< 7 );
			val |= (pDecInfo->mvReportEnable			<< 6 );
			val |= (pDecInfo->userDataEnable			<< 5 );
		}
#else
		val |= (pDecInfo->userDataReportMode		<<10 );
		val |= (param->skipframeMode				<< 3 );
#endif
	}
		
	// CMD_DEC_PIC_OPTION[11]	= skipDecHeader
	// CMD_DEC_PIC_OPTION[10]	= userDataReportMode
	// CMD_DEC_PIC_OPTION[8]	= frameBufStatEnable
	// CMD_DEC_PIC_OPTION[7]	= mbParamEnable
	// CMD_DEC_PIC_OPTION[6]	= mvReportEnable
	// CMD_DEC_PIC_OPTION[5]	= userDataEnable
	// CMD_DEC_PIC_OPTION[4:3]	= skipframeMode
	// CMD_DEC_PIC_OPTION[2]	= iframeSearchEnable
	// CMD_DEC_PIC_OPTION[1]	= prescanMode
	// CMD_DEC_PIC_OPTION[0]	= prescanEnable
	VpuWriteReg( CMD_DEC_PIC_OPTION, val );
	VpuWriteReg( CMD_DEC_PIC_SKIP_NUM, param->skipframeNum );

	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		VpuWriteReg(BIT_RUN_AUX_STD, 1);
	else
		VpuWriteReg(BIT_RUN_AUX_STD, 0);

	// Secondary channel
	val = ( 
		pDecInfo->secAxiUse.useBitEnable	<< 0 | 
		pDecInfo->secAxiUse.useIpEnable		<< 1 | 
		pDecInfo->secAxiUse.useDbkYEnable	<< 2 |
		pDecInfo->secAxiUse.useDbkCEnable	<< 3 |
		pDecInfo->secAxiUse.useOvlEnable	<< 4 | 
		pDecInfo->secAxiUse.useBtpEnable	<< 5 | 
		pDecInfo->secAxiUse.useHostBitEnable	<<  8 |
		pDecInfo->secAxiUse.useHostIpEnable     <<  9 |
		pDecInfo->secAxiUse.useHostDbkYEnable   << 10 | 
		pDecInfo->secAxiUse.useHostDbkCEnable   << 11 | 
		pDecInfo->secAxiUse.useHostOvlEnable    << 12 |
		pDecInfo->secAxiUse.useHostBtpEnable    << 13
	);	
		
	VpuWriteReg( BIT_AXI_SRAM_USE, val);

    Console_Printf(LEVEL_DEBUG, "2ndAXI SRAM usage: %08x\n", val );
    Console_Printf(LEVEL_DEBUG, "PicOption[Rpt]: FrmBufStatRptEn: %d, MbParamRptEn: %d, MvRptEn: %d\n", 
		pDecInfo->frameBufStatEnable, pDecInfo->mbParamEnable, pDecInfo->mvReportEnable);
    Console_Printf(LEVEL_DEBUG, "FrameSkip: iFrameSearch: %d, SkipFrameMode: %d, SkipFrameNum: %d\n", 
		param->iframeSearchEnable, param->skipframeMode, param->skipframeNum);
    Console_Printf(LEVEL_DEBUG, "UserDataEn: %d, UserDataRptMode: %d\n", 
        pDecInfo->userDataEnable, pDecInfo->userDataReportMode);
    Console_Printf(LEVEL_DEBUG, "RotationMode: RotEn: %d, MirrorEn: %d, DeringEn: %d, RotMode: %d, MirrorMode: %d\n", 
	    pDecInfo->rotationEnable, pDecInfo->mirrorEnable, pDecInfo->deringEnable, pDecInfo->rotationAngle, pDecInfo->mirrorDirection);
    Console_Printf(LEVEL_DEBUG, "DispFlag: %08x\n", VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));
    Console_Printf(LEVEL_DEBUG, "VPU_DecStartOneFrame(): MSG_0(%08x), MSG_1(%08x), MSG_2(%08x)\n",
                    VpuReadReg(BIT_MSG_0), VpuReadReg(BIT_MSG_1), VpuReadReg(BIT_MSG_2));

	chip_media->chip_media_state = CHIP_MEDIA_RUNNING;
#ifdef FIXED_0x77_DEAD
	pendingInst = pCodecInst;
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, PIC_RUN);
#else
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, PIC_RUN);

	pendingInst = pCodecInst;
#endif

	return RETCODE_SUCCESS;
}

RetCode VPU_DecGetOutputInfo(
		DecHandle handle,
		DecOutputInfo * info)
{
	CodecInst * pCodecInst;
	DecInfo	  *	pDecInfo;
	RetCode		ret;
	Uint32		val = 0;
	Uint32		val2 = 0;

    Console_Printf(LEVEL_DEBUG, "VPU_DecGetOutputInfo(): FrameNum(%d)\n", VpuReadReg(RET_DEC_PIC_FRAME_NUM));

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (info == 0)
		return RETCODE_INVALID_PARAM;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pendingInst == 0)
		return RETCODE_WRONG_CALL_SEQUENCE;

	if (pCodecInst != pendingInst)
		return RETCODE_INVALID_HANDLE;

	val	= VpuReadReg(RET_DEC_PIC_SUCCESS);
	info->decodingSuccess = (val & 0x01);

	if( pCodecInst->codecMode == AVC_DEC )
	{
		info->notSufficientPsBuffer = (val >> 3) & 0x1;
		info->notSufficientSliceBuffer = (val >> 2) & 0x1;
	}
	if( pCodecInst->codecMode == MP4_DEC )
		info->mp4PackedPBframe = ((val >> 16) & 0x01);
	
	val = VpuReadReg(RET_DEC_PIC_SIZE);				// decoding picture size
	info->decPicHeight	= val & 0xFFFF;
	info->decPicWidth	= (val>>16) & 0xFFFF;
#ifdef PIC_ASPECT_F_RATE
	info->picAspectRatio	= VpuReadReg(RET_DEC_PIC_ASPECT);	
	info->picFrateNr		= VpuReadReg(RET_DEC_PIC_FRATE_NR);
    info->picFrateDrPrev    = info->picFrateDr; // newly added
	info->picFrateDr		= VpuReadReg(RET_DEC_PIC_FRATE_DR);
#endif
	if((pCodecInst->codecMode == AVC_DEC ) || (pCodecInst->codecMode == HEVC_DEC))
	{
		val = VpuReadReg(RET_DEC_PIC_CROP_LEFT_RIGHT);				// frame crop information(left, right)
		val2 = VpuReadReg(RET_DEC_PIC_CROP_TOP_BOTTOM);				// frame crop information(top, bottom)
		if( val == 0 && val2 == 0 )
		{
			info->decPicCrop.left = 0;
			info->decPicCrop.right = 0;
			info->decPicCrop.top = 0;
			info->decPicCrop.bottom	= 0;
		}
		else
		{
			info->decPicCrop.left = ( (val>>16) & 0xFFFF );
			info->decPicCrop.right = info->decPicWidth - ( val & 0xFFFF );
			info->decPicCrop.top = ( (val2>>16) & 0xFFFF );
			info->decPicCrop.bottom	= info->decPicHeight - (val2 & 0xFFFF);
		}
	}

	val = VpuReadReg(RET_DEC_PIC_TYPE);
	info->picType = val & 7;
	info->picTypeFirst = (val & 0x38) >> 3;
	info->nalType = (val >> 6) & 0xff;

#ifndef API_CR	
	info->interlacedFrame = (val >> 16) & 0x1;	
#else

	info->interlacedFrame	= (val >> 18) & 0x1;
	info->pictureStructure  = (val >> 19) & 0x0003;	// MbAffFlag[17], FieldPicFlag[16]
	info->topFieldFirst     = (val >> 21) & 0x0001;	// TopFieldFirst[18]
	info->repeatFirstField  = (val >> 22) & 0x0001;
	info->progressiveFrame  = (val >> 23) & 0x0003;
	info->fieldSequence     = (val >> 25) & 0x0007;
	info->h264PicStruct		= (val >> 28) & 0x000F;
	info->idr_first = (val & 0x80) >> 7;
	info->idr_second = (val & 0x40) >> 6;

	// Frame Buffer Status
	if (pDecInfo->frameBufStatEnable) {
		int size, paraInfo, address;
		
		BYTE tempBuf[8];
		memset(tempBuf, 0, 8);
		
		ReadSdramBurst(tempBuf, pDecInfo->picParaBaseAddr, 8, 1);
		
		val =	((tempBuf[0]<<24) & 0xFF000000) |
				((tempBuf[1]<<16) & 0x00FF0000) |
				((tempBuf[2]<< 8) & 0x0000FF00) |
				((tempBuf[3]<< 0) & 0x000000FF);
		address = ((tempBuf[4]<<24) & 0xFF000000) |
				((tempBuf[5]<<16) & 0x00FF0000) |
				((tempBuf[6]<< 8) & 0x0000FF00) |
				((tempBuf[7]<< 0) & 0x000000FF);

		paraInfo = (val >> 24) & 0xFF;
		size = (val >>  0) & 0xFFFFFF;
		
		if (paraInfo == PARA_TYPE_FRM_BUF_STATUS) {
			info->decOutputExtData.frameBufDataSize = size;
			info->decOutputExtData.frameBufStatDataAddr = pDecInfo->frameBufStateAddr;
		} else {
			// VPU does not write data
			info->decOutputExtData.frameBufDataSize = 0;
			info->decOutputExtData.frameBufStatDataAddr = 0;
		}
	}

	// Mb Param
	if (pDecInfo->mbParamEnable) {
		int size, paraInfo, address;
		
		BYTE tempBuf[8];
		memset(tempBuf, 0, 8);
		
		ReadSdramBurst(tempBuf, pDecInfo->picParaBaseAddr + 8, 8, 1);

		val =	((tempBuf[0]<<24) & 0xFF000000) |
				((tempBuf[1]<<16) & 0x00FF0000) |
				((tempBuf[2]<< 8) & 0x0000FF00) |
				((tempBuf[3]<< 0) & 0x000000FF);
		address = ((tempBuf[4]<<24) & 0xFF000000) |
				((tempBuf[5]<<16) & 0x00FF0000) |
				((tempBuf[6]<< 8) & 0x0000FF00) |
				((tempBuf[7]<< 0) & 0x000000FF);

		paraInfo = (val >> 24) & 0xFF;
		size = (val >>  0) & 0x00FFFF;
		
		if (paraInfo == PARA_TYPE_MB_PARA) {
			info->decOutputExtData.mbParamDataSize = size;
			info->decOutputExtData.mbParamDataAddr = pDecInfo->mbParamDataAddr;
		} else {
			// VPU does not write data
			info->decOutputExtData.mbParamDataSize = 0;
			info->decOutputExtData.mbParamDataAddr = 0;
		}
	}

	// Motion Vector
	if (pDecInfo->mvReportEnable) {
		int size, paraInfo, address, mvNumPerMb;

		BYTE tempBuf[8];
		memset(tempBuf, 0, 8);

		ReadSdramBurst(tempBuf, pDecInfo->picParaBaseAddr + 16, 8, 1);

		val =	((tempBuf[0]<<24) & 0xFF000000) |
				((tempBuf[1]<<16) & 0x00FF0000) |
				((tempBuf[2]<< 8) & 0x0000FF00) |
				((tempBuf[3]<< 0) & 0x000000FF);
		address = ((tempBuf[4]<<24) & 0xFF000000) |
				((tempBuf[5]<<16) & 0x00FF0000) |
				((tempBuf[6]<< 8) & 0x0000FF00) |
				((tempBuf[7]<< 0) & 0x000000FF);

		paraInfo	= (val >> 24) & 0xFF;
		mvNumPerMb	= (val >> 16) & 0xFF;
		size		= (val >>  0) & 0xFFFF;

		if (paraInfo == PARA_TYPE_MV) {
			info->decOutputExtData.mvDataSize = size;
			info->decOutputExtData.mvNumPerMb = mvNumPerMb;
			info->decOutputExtData.mvDataAddr = pDecInfo->mvDataAddr;
		} else {
			// VPU does not write data
			info->decOutputExtData.mvNumPerMb = 0;
			info->decOutputExtData.mvDataAddr = 0;
		}
	}
#endif

	info->indexFrameDecoded = VpuReadReg(RET_DEC_PIC_CUR_IDX);
	info->indexFrameDisplay = VpuReadReg(RET_DEC_PIC_FRAME_IDX);
	info->indexFrameDecoded = info->indexFrameDecoded > 32 ? -1 : info->indexFrameDecoded;
	info->indexFrameDisplay = info->indexFrameDisplay > 32 ? -1 : info->indexFrameDisplay;


	info->userdataSegNum  = VpuReadReg(BIT_WR_PTR_1);
	info->numOfErrMBs     = VpuReadReg(RET_DEC_PIC_ERR_MB);
	info->decodingSuccess = (VpuReadReg(RET_DEC_PIC_SUCCESS) & 0x01);

	info->decPicPoc = VpuReadReg(RET_DEC_PIC_POC);
	if(pCodecInst->codecMode == HEVC_DEC || pCodecInst->codecMode == AVC_DEC)
	{
		val = VpuReadReg(BIT_WR_PTR_2);
		info->hdr_param.colour_primaries        = (val >> 16) & 0xff;
		info->hdr_param.matrix_coeffs           =  val & 0xff;
		info->hdr_param.transfer_characteristic = (val >> 8) & 0xff;
	}

#if DEBUG_MEM_CHECK
	if((pCodecInst->codecMode == HEVC_DEC) && VpuReadReg(BIT_MSG_0))	//CHECK_ERR
	{
		*(volatile unsigned int *)0xa4003508 = 1<<30;
		Console_Printf(LEVEL_ALWAYS, "CHECK_ERR:%d, CHECK_ID:%d, CHECK_ADDR:0x%08x\n", VpuReadReg(BIT_MSG_0), VpuReadReg(BIT_MSG_2), VpuReadReg(BIT_MSG_1));
		Console_Printf(LEVEL_ALWAYS, "stop demux esv import , please dump the esv memory:wrPtr:0x%08x\n", VpuReadReg(BIT_WR_PTR_0));
		//while(1);
	}
#endif
	pendingInst = 0;

    Console_Printf(LEVEL_DEBUG, "VPU_DecGetOutputInfo(): FrameNum(%d)/ret_success(%d)\n", VpuReadReg(RET_DEC_PIC_FRAME_NUM), info->decodingSuccess);
    Console_Printf(LEVEL_DEBUG, "DecodedIndex: %d, DisplayIndex: %d, NumErrMbs: %d\n", 
	                info->indexFrameDecoded, info->indexFrameDisplay, info->numOfErrMBs);
    Console_Printf(LEVEL_DEBUG, "Width(%d) x Height(%d)\n", info->decPicWidth, info->decPicHeight);
    Console_Printf(LEVEL_DEBUG, "FrameRateInfo: Numerator(%d) x Denominator(%d)\n", info->picFrateNr, info->picFrateDr);
    Console_Printf(LEVEL_DEBUG, "AspectRatioInfo: %d\n", info->picAspectRatio);
    Console_Printf(LEVEL_DEBUG, "[CropInfo] Left: %d, Right: %d, Top: %d, Bottom: %d\n", 
	                info->decPicCrop.left, info->decPicCrop.right, info->decPicCrop.top, info->decPicCrop.bottom);
    Console_Printf(LEVEL_DEBUG, "[PicType] PicType: %d, PicTypeFirst: %d, InterlacedFrame: %d, PicStruct: %d\n", 
	                info->picType, info->picTypeFirst, info->interlacedFrame, info->pictureStructure);
    Console_Printf(LEVEL_DEBUG, "[PicType] TopFieldFirst: %d, RepeatFirstField: %d, ProgressiveFrame: %d, FieldSequence: %d\n", 
	                info->topFieldFirst, info->repeatFirstField, info->progressiveFrame, info->fieldSequence);
    Console_Printf(LEVEL_DEBUG, "[PicType] AvcPicStruct: %d, IdrFirst: %d, IdrSecond: %d\n", 
	                info->h264PicStruct, info->idr_first, info->idr_second);
    Console_Printf(LEVEL_DEBUG, "MSG_0(%08x), MSG_1(%08x), MSG_2(%08x)\n",
                    VpuReadReg(BIT_MSG_0), VpuReadReg(BIT_MSG_1), VpuReadReg(BIT_MSG_2));
    Console_Printf(LEVEL_DEBUG, "rdPtr: %08x, wrPtr: %08x, eRdPtr: %08x, eRdPtrPrev: %08x\n",
	                VpuReadReg(rdPtrRegAddr[pCodecInst->instIndex]), VpuReadReg(wrPtrRegAddr[pCodecInst->instIndex]), VpuReadReg(BIT_EXACT_RD_PTR), VpuReadReg(BIT_EXACT_RD_PTR_PREV));
    Console_Printf(LEVEL_DEBUG, "DispFlag: %08x\n", VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));

	return RETCODE_SUCCESS;
}

RetCode VPU_DecBitBufferFlush(DecHandle handle)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->frameBufPool == 0) { // This means frame buffers have not been registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		VpuWriteReg(BIT_RUN_AUX_STD, 1);
	else
		VpuWriteReg(BIT_RUN_AUX_STD, 0);

	Console_Printf(LEVEL_DEBUG, "VPU_DecBitBufferFlush [0x%08x]\n", pDecInfo->streamWrPtr + 1);
	VpuWriteReg(pDecInfo->streamRdPtrRegAddr, pDecInfo->streamWrPtr + 1);
	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, DEC_BUF_FLUSH);

	while (VpuReadReg(BIT_BUSY_FLAG))
		;

	VpuWriteReg(pDecInfo->frameDisplayFlagRegAddr, 0);

	return RETCODE_SUCCESS;
}

RetCode VPU_DecResetRdPtr(DecHandle handle, PhysicalAddress newRdPtr,PhysicalAddress newWrPtr )
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

    Console_Printf(LEVEL_DEBUG, "VPU_DecResetRdPtr(): newRdPtr: %08x\n", newRdPtr );

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;
	
	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}
	
	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	
	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		VpuWriteReg(BIT_RUN_AUX_STD, 1);
	else
		VpuWriteReg(BIT_RUN_AUX_STD, 0);

	///< delete to
	VpuWriteReg(RET_DEC_PIC_FRAME_IDX, BIT_FRM_DIS_FLG_0);
	
	if(newRdPtr != 0) 	
		VpuWriteReg(pDecInfo->streamRdPtrRegAddr, newRdPtr&VDEC_HW_ADDR_MASK);

	if(newWrPtr != 0) {
		VpuWriteReg(pDecInfo->streamWrPtrRegAddr, newWrPtr&VDEC_HW_ADDR_MASK);
		pDecInfo->streamWrPtr = newWrPtr;
	}

	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, DEC_BUF_FLUSH);
	
	while (VpuReadReg(BIT_BUSY_FLAG))
		;
	
	return RETCODE_SUCCESS;
}

RetCode VPU_DecResyncSeq(DecHandle handle)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

    Console_Printf(LEVEL_DEBUG, "VPU_DecResyncSeq()\n" );

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		VpuWriteReg(BIT_RUN_AUX_STD, 1);
	else
		VpuWriteReg(BIT_RUN_AUX_STD, 0);

	BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, DEC_RESYNC_SEQ);
	
	return RETCODE_SUCCESS;
}

RetCode VPU_DecSetDispFlag(DecHandle handle, int index)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

	int val = 1;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

    Console_Printf(LEVEL_DEBUG, "VPU_DecSetDispFlag(): Before setting: %08x\n", VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));

	if (pDecInfo->frameBufPool == 0) { // This means frame buffers have not been registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	if ((index < 0) || (index > (pDecInfo->numFrameBuffers - 1)))
		return	RETCODE_INVALID_PARAM;

	val <<= index;
	val = (val | VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));
	VpuWriteReg(pDecInfo->frameDisplayFlagRegAddr, val );

    Console_Printf(LEVEL_DEBUG, "VPU_DecSetDispFlag(): After setting: %08x\n", VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));

	return RETCODE_SUCCESS;
}

RetCode VPU_DecClrDispFlag(DecHandle handle, int index)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

	int val = 1;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;

    Console_Printf(LEVEL_DEBUG, "VPU_DecClrDispFlag(): Before clearing: %08x\n", VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));

	if (pDecInfo->frameBufPool == 0) { // This means frame buffers have not been registered.
		return RETCODE_WRONG_CALL_SEQUENCE;
	}

	if ((index < 0) || (index > (pDecInfo->numFrameBuffers - 1)))
		return	RETCODE_INVALID_PARAM;

    val <<= index;
 	val = ~val;
 	val = (val & VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));
 	VpuWriteReg(pDecInfo->frameDisplayFlagRegAddr, val );

    Console_Printf(LEVEL_DEBUG, "VPU_DecClrDispFlag(): After clearing: %08x\n", VpuReadReg(pDecInfo->frameDisplayFlagRegAddr));

	return RETCODE_SUCCESS;
}

RetCode VPU_DecGiveCommand(
		DecHandle handle,
		CodecCommand cmd,
		void * param)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	switch (cmd) 
	{
		case ENABLE_ROTATION :
			{
				if (!pDecInfo->rotatorOutputValid) {
					return RETCODE_ROTATOR_OUTPUT_NOT_SET;
				}
				if (pDecInfo->rotatorStride == 0) {
					return RETCODE_ROTATOR_STRIDE_NOT_SET;
				}
				pDecInfo->rotationEnable = 1;
				break;
			}

		case DISABLE_ROTATION :
			{
				pDecInfo->rotationEnable = 0;
				break;
			}

		case ENABLE_MIRRORING :
			{
				if (!pDecInfo->rotatorOutputValid) {
					return RETCODE_ROTATOR_OUTPUT_NOT_SET;
				}
				if (pDecInfo->rotatorStride == 0) {
					return RETCODE_ROTATOR_STRIDE_NOT_SET;
				}
				pDecInfo->mirrorEnable = 1;
				break;
			}
		case DISABLE_MIRRORING :
			{
				pDecInfo->mirrorEnable = 0;
				break;
			}
		case SET_MIRROR_DIRECTION :
			{

				MirrorDirection mirDir;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				mirDir = *(MirrorDirection *)param;
				if (!(MIRDIR_NONE <= mirDir && mirDir <= MIRDIR_HOR_VER)) {
					return RETCODE_INVALID_PARAM;
				}
				pDecInfo->mirrorDirection = mirDir;

				break;
			}
		case SET_ROTATION_ANGLE :
			{
				int angle;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				angle = *(int *)param;
				if (angle != 0 && angle != 90 &&
						angle != 180 && angle != 270) {
					return RETCODE_INVALID_PARAM;
				}
				if (pDecInfo->rotatorStride != 0) {				
					if (angle == 90 || angle ==270) {
						if (pDecInfo->initialInfo.picHeight > pDecInfo->rotatorStride) {
							return RETCODE_INVALID_PARAM;
						}
					} else {
						if (pDecInfo->initialInfo.picWidth > pDecInfo->rotatorStride) {
							return RETCODE_INVALID_PARAM;
						}
					}
				}
							
				pDecInfo->rotationAngle = angle;
				break;
			}

		case SET_ROTATOR_OUTPUT :
			{
				FrameBuffer * frame;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				frame = (FrameBuffer *)param;
				pDecInfo->rotatorOutput = *frame;
				pDecInfo->rotatorOutputValid = 1;
				break;
			}

		case SET_ROTATOR_INDEX:
			{
				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				pDecInfo->rotIndex = *(int *)param;
				break;
			}

		case SET_ROTATOR_STRIDE :
			{
				int stride;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				stride = *(int *)param;
				if (stride % 8 != 0 || stride == 0) {
					return RETCODE_INVALID_STRIDE;
				}
				if (pDecInfo->rotationAngle == 90 || pDecInfo->rotationAngle == 270) {
					if (pDecInfo->initialInfo.picHeight > stride) {
						return RETCODE_INVALID_STRIDE;
					}
				} else {
					if (pDecInfo->initialInfo.picWidth > stride) {
						return RETCODE_INVALID_STRIDE;
					}					
				}

				pDecInfo->rotatorStride = stride;
				break;
			}
		case DEC_SET_SPS_RBSP:
			{
				if (pCodecInst->codecMode != AVC_DEC) {
					return RETCODE_INVALID_COMMAND;
				}
				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				SetParaSet(handle, 0, param);
				break;
			}

		case DEC_SET_PPS_RBSP:
			{
				if (pCodecInst->codecMode != AVC_DEC) {
					return RETCODE_INVALID_COMMAND;
				}
				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				SetParaSet(handle, 1, param);
				break;
			}
		case ENABLE_DERING :
			{
				if (!pDecInfo->rotatorOutputValid) {
					return RETCODE_ROTATOR_OUTPUT_NOT_SET;
				}
				if (pDecInfo->rotatorStride == 0) {
					return RETCODE_ROTATOR_STRIDE_NOT_SET;
				}
				pDecInfo->deringEnable = 1;
				break;
			}

		case DISABLE_DERING :
			{
				pDecInfo->deringEnable = 0;
				break;
			}
		case SET_SEC_AXI:
			{
				SecAxiUse secAxiUse;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				secAxiUse = *(SecAxiUse *)param;

				pDecInfo->secAxiUse.useBitEnable = (secAxiUse.useBitEnable & 0x1);
				pDecInfo->secAxiUse.useIpEnable = (secAxiUse.useIpEnable & 0x1);
				pDecInfo->secAxiUse.useDbkYEnable = (secAxiUse.useDbkYEnable & 0x1);
				pDecInfo->secAxiUse.useDbkCEnable = (secAxiUse.useDbkCEnable & 0x1);
				pDecInfo->secAxiUse.useOvlEnable = (secAxiUse.useOvlEnable & 0x1);
				pDecInfo->secAxiUse.useBtpEnable = (secAxiUse.useBtpEnable & 0x1);

				pDecInfo->secAxiUse.useHostBitEnable = (secAxiUse.useHostBitEnable & 0x1);
				pDecInfo->secAxiUse.useHostIpEnable = (secAxiUse.useHostIpEnable & 0x1);
				pDecInfo->secAxiUse.useHostDbkYEnable = (secAxiUse.useHostDbkYEnable & 0x1);
				pDecInfo->secAxiUse.useHostDbkCEnable = (secAxiUse.useHostDbkCEnable & 0x1);
				pDecInfo->secAxiUse.useHostOvlEnable = (secAxiUse.useHostOvlEnable & 0x1);
				pDecInfo->secAxiUse.useHostBtpEnable = (secAxiUse.useHostOvlEnable & 0x1);

				pDecInfo->secAxiUse.bufBitUse = secAxiUse.bufBitUse;
				pDecInfo->secAxiUse.bufIpAcDcUse = secAxiUse.bufIpAcDcUse;
				pDecInfo->secAxiUse.bufDbkYUse = secAxiUse.bufDbkYUse;
				pDecInfo->secAxiUse.bufDbkCUse = secAxiUse.bufDbkCUse;
				pDecInfo->secAxiUse.bufOvlUse = secAxiUse.bufOvlUse;
				pDecInfo->secAxiUse.bufBtpUse = secAxiUse.bufBtpUse;

				break;
			}
		case ENABLE_REP_BUFSTAT:
			{
				pDecInfo->frameBufStatEnable = 1;
				break;
			}
		case DISABLE_REP_BUFSTAT:
			{
				pDecInfo->frameBufStatEnable = 0;
				break;
			}
		case ENABLE_REP_MV:
			{
				pDecInfo->mvReportEnable = 1;
				break;
			}
		case DISABLE_REP_MV:
			{
				pDecInfo->mvReportEnable = 0;
				break;
			}
		case ENABLE_REP_USERDATA:
			{
				if (!pDecInfo->userDataBufAddr) {
					return RETCODE_USERDATA_BUF_NOT_SET;
				}
				if (pDecInfo->userDataBufSize == 0) {
					return RETCODE_USERDATA_BUF_NOT_SET;
				}
				pDecInfo->userDataEnable = 1;
				break;
			}
		case DISABLE_REP_USERDATA:
			{
				pDecInfo->userDataEnable = 0;
				break;
			}
		case SET_ADDR_REP_PICPARAM:
			{
				PhysicalAddress picParaBaseAddr;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				picParaBaseAddr = *(PhysicalAddress *)param;
				if (picParaBaseAddr % 8 != 0 || picParaBaseAddr == 0) {
					return RETCODE_INVALID_PARAM;
				}

				pDecInfo->picParaBaseAddr = picParaBaseAddr;
				break;
			}
		case SET_ADDR_REP_USERDATA:
			{
				PhysicalAddress userDataBufAddr;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				userDataBufAddr = *(PhysicalAddress *)param;
				if (userDataBufAddr % 8 != 0 || userDataBufAddr == 0) {
					return RETCODE_INVALID_PARAM;
				}

				pDecInfo->userDataBufAddr = userDataBufAddr;
				break;
			}
		case SET_SIZE_REP_USERDATA:
			{
				PhysicalAddress userDataBufSize;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				userDataBufSize = *(PhysicalAddress *)param;

				pDecInfo->userDataBufSize = userDataBufSize;
				break;
			}
			
		case SET_USERDATA_REPORT_MODE:
			{
				int userDataMode;

				userDataMode = *(int *)param;
				if (userDataMode != 1 && userDataMode != 0) {
					return RETCODE_INVALID_PARAM;
				}
				pDecInfo->userDataReportMode = userDataMode;
				break;
			}
		case SET_ADDR_REP_MB_PARAM:
			{
				PhysicalAddress mbParamDataAddr;

				if (param == 0) {
					return RETCODE_INVALID_PARAM;
				}
				mbParamDataAddr = *(PhysicalAddress *)param;
				if (mbParamDataAddr % 8 != 0 || mbParamDataAddr == 0) {
					return RETCODE_INVALID_PARAM;
				}

				pDecInfo->mbParamDataAddr = mbParamDataAddr;
				break;

			}
		case SET_ADDR_REP_MV_DATA:
			{
				PhysicalAddress mvDataAddr = *(PhysicalAddress *)param;
				if (param == 0)
					return RETCODE_INVALID_PARAM;
				if (mvDataAddr % 8 != 0 || mvDataAddr == 0)
					return RETCODE_INVALID_PARAM;

				pDecInfo->mvDataAddr = mvDataAddr;
				break;

			}
		case SET_ADDR_REP_BUF_STATE:
			{
				PhysicalAddress frameBufStateAddr = *(PhysicalAddress *)param;
				if (param == 0)
					return RETCODE_INVALID_PARAM;
				if (frameBufStateAddr % 8 != 0 || frameBufStateAddr == 0)
					return RETCODE_INVALID_PARAM;

				pDecInfo->frameBufStateAddr = frameBufStateAddr;
				break;
			}
 		case SET_MJPEG_CLIPMODE:
 			{
 				int clipMode = *(int *)param;
 				pDecInfo->mjpgClipMode = clipMode;
 				break;
 			}
		case SET_MJPEG_SAMPLE_X:
			{
				int sampleRatioX = *(int *)param;
				pDecInfo->mjpgSampX = sampleRatioX;
				break;
			}
		case SET_MJPEG_SAMPLE_Y:
			{
				int sampleRatioY = *(int *)param;
				pDecInfo->mjpgSampY = sampleRatioY;
				break;
			}

		default:
			return RETCODE_INVALID_COMMAND;
	}
	return RETCODE_SUCCESS;
}

RetCode VPU_DecReadExactRdPrt(DecHandle handle, PhysicalAddress *prdEndPrt, PhysicalAddress *prdStPrt)
{
	RetCode ret;

    Console_Printf(LEVEL_DEBUG, "VPU_DecReadExactRdPrt()\n" );

	if (prdEndPrt==0 && prdStPrt==0) {
		return RETCODE_INVALID_PARAM;
	}
  
	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS){
		return ret;
	}
	if(prdEndPrt != NULL)
	 	*prdEndPrt = VpuReadReg(BIT_EXACT_RD_PTR) | GX_DDR_BASE_ADDR;

	if(prdStPrt != NULL)
 	    *prdStPrt  = VpuReadReg(BIT_EXACT_RD_PTR_PREV) | GX_DDR_BASE_ADDR;

	return RETCODE_SUCCESS;
}

RetCode VPU_DecSetEscSeqInit( DecHandle handle, int escape )
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;
	int val;

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;

	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	
	val = VpuReadReg(CMD_DEC_SEQ_INIT_ESCAPE);
	val |= 1 << ( pCodecInst->instIndex + 2);

	///<??added by chenshu

#ifdef  FIXED_0x77_DEAD
#else
	VpuWriteReg(BIT_GDI_CLOSE,0x11);
#endif
	while((VpuReadReg(BIT_GDI_EMPTY)&0xff) != 0x77);
	
	VpuWriteReg(CMD_DEC_SEQ_INIT_ESCAPE, val );	

    Console_Printf(LEVEL_DEBUG, "VPU_DecSetEscSeqInit(): %08x\n", val );

	return RETCODE_SUCCESS;
}

RetCode VPU_DecRecoverEOS(DecHandle handle)
{
	CodecInst * pCodecInst;
	DecInfo * pDecInfo;
	RetCode ret;

    Console_Printf(LEVEL_DEBUG, "VPU_DecRecoverEOS()\n" );

	ret = CheckDecInstanceValidity(handle);
	if (ret != RETCODE_SUCCESS)
		return ret;
	
	if (pendingInst) {
		return RETCODE_FRAME_NOT_COMPLETE;
	}
	
	pCodecInst = handle;
	pDecInfo = &pCodecInst->CodecInfo.decInfo;
	
	if (pDecInfo->openParam.bitstreamFormat == STD_DIV3)
		VpuWriteReg(BIT_RUN_AUX_STD, 1);
	else
		VpuWriteReg(BIT_RUN_AUX_STD, 0);
	
	//BitIssueCommand(pCodecInst->instIndex, pCodecInst->codecMode, DEC_RECOVER_EOS);
	
	return RETCODE_SUCCESS;
}

void    h264ClrPsBuf(int reStart)
{
	int i;
	int val = VpuReadReg(BIT_RD_PTR_1);
	for (i = 0; i < MAX_PPS_NUM ; i ++)
	{
		VpuWriteMem(ppsBuffer+(i<<3),0);
		VpuWriteMem(ppsBuffer+(i<<3)+4,0);
	}
	for (i = 0; i < MAX_SPS_NUM ; i ++)
	{
		if((val != i) | (reStart == 0))
		{
			VpuWriteMem(spsBuffer+(i<<3),0);
			VpuWriteMem(spsBuffer+(i<<3)+4,0);
		}
	}
}
