//------------------------------------------------------------------------------
// File: VpuApi.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------
#ifndef VPUAPI_H_INCLUDED
#define VPUAPI_H_INCLUDED

#define NEW_VERSION_BUG
#define DEBUG_MEM_CHECK (0)

#define  CM16_BUG	
#define	FIXED_0x77_DEAD

//#define _CDB
#define RPR_SUPPORT
#define EVB_BUILD
#define	BODAHX6

#define PCI2HPI							0
#define USB2HPI							1
#define HPI_DEVICE_TYPE					PCI2HPI

#ifndef WIN32
#undef  HPI_DEVICE_TYPE
#define HPI_DEVICE_TYPE 0xff
#endif

enum {
	LEVEL_DEBUG,
	LEVEL_WARNING,
	LEVEL_ERROR,
	LEVEL_ALWAYS,
	LEVEL_SILENCE,
};

#define Console_Printf(level, fmt, args...) \
do { \
	if (level >= g_print_level) \
		gx_printf(fmt, ##args);  \
} while(0)

//------------------------------------------------------------------------------
// system dependent definition
//------------------------------------------------------------------------------
#if HPI_DEVICE_TYPE == USB2HPI || HPI_DEVICE_TYPE == PCI2HPI
#include "IO_API.h"
#else
#include "GXIO_API.h"
#endif

#if HPI_DEVICE_TYPE == USB2HPI || HPI_DEVICE_TYPE == PCI2HPI
	#define VpuWriteReg( ADDR, DATA )	WriteReg( ADDR, DATA ) // system register write
	#define VpuReadReg( ADDR )			ReadReg( ADDR )		   // system register write
	#define VpuWriteMem( ADDR, DATA )	WriteReg( ADDR, DATA ) // system memory write
	#define VpuReadMem( ADDR )			ReadReg( ADDR )		   // system memory read
#else
	//#error have to implement VpuWriteReg, VpuReadReg, VpuWriteMem and VpuReadMem according to your system!
#endif

#define API_CR
#define	PIC_ASPECT_F_RATE
#define JPEG_DOWNSAMPLE_WORKAROUND

#ifdef API_CR
	//#define	USE_INTERRUPT_USE_DATA_REPORT
	#define	INT_BIT_USERDATA	13
#endif

#define	ADDR_BIT_STREAM			0x080000
#define	LARGE_STREAM_BUF_SIZE	0x050000
#define	SMALL_STREAM_BUF_SIZE	0x050000
	
#define	ADDR_BIT_STREAM_1		0x000000
#define	ADDR_BIT_STREAM_2		0x040000
#define	ADDR_BIT_STREAM_3		0x080000
#define	ADDR_BIT_STREAM_4		0x0C0000
#define	STREAM_BUF_SIZE_1		0x040000
#define	STREAM_BUF_SIZE_2		0x040000
#define	STREAM_BUF_SIZE_3		0x040000
#define	STREAM_BUF_SIZE_4		0x040000

#define	ADDR_BIT_WORK			0x150000
#define ADDR_FRAME_BASE			0x200000


#define	SLICE_SAVE_SIZE			0
#define PS_SAVE_SIZE			0
// SRAM Total Size = 0x20000
#define	DBKY_INTERNAL_USE_BUF		0x000000	// DBKY_SIZE = MAX_WIDTH*16
#define	DBKC_INTERNAL_USE_BUF		0x008000	// DBKC_SIZE = MAX_WIDTH*16
#define	BIT_INTERNAL_USE_BUF		0x010000	// BIT_SIZE  = MAX_WIDTH/16*128
#define IPACDC_INTERNAL_USE_BUF		0x014000	// IP_SIZE   = MAX_WIDTH/16*128
#define	OVL_INTERNAL_USE_BUF		0x018000	// OVL_SIZE  = MAX_WIDTH*5
#define	BTP_INTERNAL_USE_BUF		0x01D000	// BTP_SIZE  = {(MAX_WIDTH/256)*(MAX_HEIGHT/16) + 1}*6

#define MAX_FRAME_BASE			0x4000000   // 64 MB
#ifdef API_CR
	#define SIZE_PARA_BUF					0x100
	#define	SIZE_USER_BUF					0x10000
	#define	SIZE_MV_DATA					0x20000
	#define	SIZE_MB_DATA					0x4000
	#define	SIZE_FRAME_BUF_STAT				0x100
	#define	USER_DATA_INFO_OFFSET			8*17
	#define	ADDR_PIC_PARA_BASE_ADDR			ADDR_FRAME_BASE + 1920*1088*3/2*7
	#define	ADDR_USER_BASE_ADDR				ADDR_PIC_PARA_BASE_ADDR 		+ SIZE_PARA_BUF
	#define	ADDR_MV_BASE_ADDR				ADDR_USER_BASE_ADDR				+ SIZE_USER_BUF
	#define	ADDR_MB_BASE_ADDR				ADDR_MV_BASE_ADDR				+ SIZE_MV_DATA
	#define	ADDR_FRAME_BUF_STAT_BASE_ADDR	ADDR_MB_BASE_ADDR				+ SIZE_MB_DATA
	#define ADDR_END_OF_RPT_BUF				ADDR_FRAME_BUF_STAT_BASE_ADDR 	+ SIZE_FRAME_BUF_STAT
	//#define	SIZE_USER_BUF				USER_DATA_INFO_OFFSET+32
#endif

#define CM_WORK_BUFFER_SIZE \
	(CODE_BUF_SIZE + WORK_BUF_SIZE + PARA_BUF2_SIZE + PARA_BUF_SIZE + AXI_BUFFER_SIZE)
#define SINGLE_CODE_BUF_SIZE  (  32 * 1024 )
#define CODE_BUF_SIZE         ( 120 * 1024 )
#define WORK_BUF_SIZE         ( 156 * 1024 )
#define PARA_BUF2_SIZE        ( 0 * 1024 )
#define PARA_BUF_SIZE         ( 4 * 1024 )
#define AXI_BUFFER_SIZE       ( 84 * 1024)

#define IMAGE_ENDIAN			0		// 0 (64 bit little endian), 1 (64 bit big endian), 2 (32 bit swaped little endian), 3 (32 bit swaped big endian)
#define STREAM_ENDIAN			0       // 0 (64 bit little endian), 1 (64 bit big endian), 2 (32 bit swaped little endian), 3 (32 bit swaped big endian)
#define	CBCR_INTERLEAVE			1		// 0 (chroma seperate mode), 1 (chroma interleave mode)

#define	MC_DMA_PIPE_OPT_EN		1		// MC cache
#define	MC_DMA_CACHE_EN			1		// MC cache

#define	USE_BIT_INTERNAL_BUF	1
#define USE_IP_INTERNAL_BUF		1
#define	USE_DBKY_INTERNAL_BUF	1
#define	USE_DBKC_INTERNAL_BUF	1
#define	USE_OVL_INTERNAL_BUF	1
#define	USE_BTP_INTERNAL_BUF	1
#define	USE_ME_INTERNAL_BUF		1

#define USE_HOST_BIT_INTERNAL_BUF	1
#define USE_HOST_IP_INTERNAL_BUF	1
#define USE_HOST_DBKY_INTERNAL_BUF	1
#define USE_HOST_DBKC_INTERNAL_BUF	1
#define	USE_HOST_OVL_INTERNAL_BUF	1
#define	USE_HOST_BTP_INTERNAL_BUF	1
#define USE_HOST_ME_INTERNAL_BUF	1

#define STREAM_FULL_EMPTY_CHECK_DISABLE 0

//#define REPORT_HEADER_INFO

typedef unsigned char	Uint8;
typedef unsigned int	Uint32;
typedef unsigned short	Uint16;
typedef Uint32 PhysicalAddress;
typedef Uint32 VirtualAddress;

typedef unsigned int    UI64;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;

//------------------------------------------------------------------------------
// common struct and definition
//------------------------------------------------------------------------------
typedef enum {
	YUVNOMO,
	YUV420,
	YUV422T,
	YUV422H,
	YUV444,
} ChromaIdc;
typedef enum {
	PICTYPE_INTRA,
	PICTYPE_FORWARD_PRED,
	PICTYPE_BI_PRED
} PictureType;

typedef enum {
	SKIP_DISABLE,
	SKIP_PB_FRAME,
	SKIP_B_FRAME,
	SKIP_UNCONDITIONAL
} SkipMode;

typedef enum {
	TRICKMODE_CONTINUE,
	TRICKMODE_RANDOM_ACCESS,
	TRICKMODE_INTRA_FRAME_SEARCH,
	TRICKMODE_SKIP_FRAMES,
	TRICKMODE_VPU_SLEEP,
	TRICKMODE_VPU_WAKE
} TrickMode;

typedef enum {
	STD_AVC,
	STD_HEVC,
	STD_MPEG2,
	STD_MPEG4,
	STD_H263,
	STD_DIV3,
	STD_RV,
	STD_AVS,
	STD_MJPG,
	STD_ALL,
} CodStd;

typedef enum {
	RETCODE_SUCCESS,
	RETCODE_FAILURE,
	RETCODE_INVALID_HANDLE,
	RETCODE_INVALID_PARAM,
	RETCODE_INVALID_COMMAND,
	RETCODE_ROTATOR_OUTPUT_NOT_SET,
	RETCODE_ROTATOR_STRIDE_NOT_SET,
	RETCODE_FRAME_NOT_COMPLETE,
	RETCODE_INVALID_FRAME_BUFFER,
	RETCODE_INSUFFICIENT_FRAME_BUFFERS,
	RETCODE_INVALID_STRIDE,
	RETCODE_WRONG_CALL_SEQUENCE,
	RETCODE_CALLED_BEFORE,
	RETCODE_NOT_INITIALIZED,
	RETCODE_USERDATA_BUF_NOT_SET,
	RETCODE_VPU_PENDING
} RetCode;

extern	PhysicalAddress picParaBaseAddr;
extern	PhysicalAddress userDataBufAddr;
extern	int userDataBufSize;

typedef enum {
	ENABLE_ROTATION,
	DISABLE_ROTATION,
	ENABLE_MIRRORING,
	DISABLE_MIRRORING,
 	SET_MJPEG_CLIPMODE,
	SET_MJPEG_SAMPLE_X,
	SET_MJPEG_SAMPLE_Y,
	SET_MIRROR_DIRECTION,
	SET_ROTATION_ANGLE,
	SET_ROTATOR_OUTPUT,
	SET_ROTATOR_INDEX,
	SET_ROTATOR_STRIDE,
	DEC_SET_SPS_RBSP,
	DEC_SET_PPS_RBSP,
	ENABLE_DERING,
	DISABLE_DERING,
	SET_SEC_AXI,
	ENABLE_REP_BUFSTAT,
	DISABLE_REP_BUFSTAT,
	ENABLE_REP_MV,
	DISABLE_REP_MV,
	ENABLE_REP_USERDATA,
	DISABLE_REP_USERDATA,
	SET_ADDR_REP_PICPARAM,
	SET_ADDR_REP_USERDATA,
	SET_SIZE_REP_USERDATA,
	SET_USERDATA_REPORT_MODE,
	SET_ADDR_REP_BUF_STATE,
	SET_ADDR_REP_MB_PARAM,
	SET_ADDR_REP_MV_DATA
	/*   Encoder Command
	ENC_GET_SPS_RBSP,
	ENC_GET_PPS_RBSP,
	ENC_PUT_MP4_HEADER,
	ENC_PUT_AVC_HEADER,
	ENC_SET_SEARCHRAM_PARAM,
	ENC_GET_VOS_HEADER,
	ENC_GET_VO_HEADER,
	ENC_GET_VOL_HEADER,
	ENC_SET_INTRA_MB_REFRESH_NUMBER,
	ENC_ENABLE_HEC,
	ENC_DISABLE_HEC,
	ENC_SET_SLICE_INFO,
	ENC_SET_GOP_NUMBER,
	ENC_SET_INTRA_QP,
	ENC_SET_BITRATE,
	ENC_SET_FRAME_RATE
	*/
} CodecCommand;

typedef enum {
	MIRDIR_NONE,
	MIRDIR_VER,
	MIRDIR_HOR,
	MIRDIR_HOR_VER
} MirrorDirection;


typedef struct {
	PhysicalAddress bufY;
	PhysicalAddress bufCb;
	PhysicalAddress bufCr;
	PhysicalAddress bufMvCol;
	PhysicalAddress bufUserData;
} FrameBuffer;

typedef struct {
    Uint32    left;
    Uint32    top;
    Uint32    right;
    Uint32    bottom;
} Rect;

typedef struct {
    Uint8    colour_primaries;
    Uint8    transfer_characteristic;
    Uint8    matrix_coeffs;
} Hdr;

typedef	struct {
	int useBitEnable;
	int useIpEnable;
	int useDbkYEnable;
	int useDbkCEnable;
	int useOvlEnable;
	int useBtpEnable;
	int useMeEnable;

	int useHostBitEnable;
	int useHostIpEnable;
	int useHostDbkYEnable;
	int useHostDbkCEnable;
	int useHostOvlEnable;
	int useHostBtpEnable;
	int useHostMeEnable;
	
	PhysicalAddress bufBitUse;
	PhysicalAddress bufIpAcDcUse;
	PhysicalAddress bufDbkYUse;
	PhysicalAddress bufDbkCUse;
	PhysicalAddress bufOvlUse;
	PhysicalAddress bufBtpUse;
} SecAxiUse;

typedef struct {
	Uint32 * paraSet;
	int size;
} DecParamSet;

struct CodecInst;

//------------------------------------------------------------------------------
// decode struct and definition
//------------------------------------------------------------------------------

typedef struct CodecInst DecInst;
typedef DecInst * DecHandle;

typedef struct {
	CodStd bitstreamFormat;
	PhysicalAddress bitstreamBuffer;
	int bitstreamBufferSize;
	int outloopDbkEnable;
	int reorderEnable;
	int picWidth;
	int picHeight;
	int mjpg_thumbNailDecEnable;
	int mp4Class;
 	int mjpegClipMode;
	int mjpegSampleX;
	int mjpegSampleY;
} DecOpenParam;

typedef struct {
#ifdef JPEG_DOWNSAMPLE_WORKAROUND
	int outWidth;
	int outHeight;
#endif // JPEG_DOWNSAMPLE_WORKAROUND
	int picWidth;			// {(PicX+15)/16} * 16
	int picHeight;			// {(PicY+15)/16} * 16
	Rect picCropRect;
	int mp4_dataPartitionEnable;
	int mp4_reversibleVlcEnable;
	int mp4_shortVideoHeader;
	int h263_annexJEnable;
	int minFrameBufferCount;
	int frameBufDelay;
	int	nextDecodedIdxNum;
	int	normalSliceSize;
	int	worstSliceSize;
	int	mjpg_thumbNailEnable;
	int	mjpg_sourceFormat;
#ifdef API_CR
    int profile;
    int level;
	int tier;
	int chroma_idc;
    int progressive;
	int hevcFieldSeqFlag;
	int fRateNumerator; 
	int fRateDenominator;
	int constraint_set_flag[4];
	int direct8x8Flag;
	int aspectRateInfo;
	int bitRate;
#endif
    int mp4_fixed_vop_rate; // newly added
    int mp4_numWarpPoints; // newly added
    int h263_annexDEnable; // newly added
	int mp4_gmc_flag;
	int bitdepth;
	int picXAign;	//for register frame Buffer
	int picYAign;	//for register frame Buffer
} DecInitialInfo;

typedef	struct{
#ifdef RPR_SUPPORT
	int maxDecMbNum;
	int maxDecMbX;
	int maxDecMbY;
#endif // RPR_SUPPORT
} DecBufInfo;

#ifdef	API_CR

typedef enum {
		PARA_TYPE_FRM_BUF_STATUS	= 1,
		PARA_TYPE_MB_PARA		= 2,
		PARA_TYPE_MV			= 4,
} ExtParaType;

#endif

typedef struct {
	int dispReorderBuf;
	int iframeSearchEnable;
	int skipframeMode;
	int skipframeNum;
} DecParam;

#ifdef	API_CR
typedef struct {
	int frameBufDataSize;			// Frame Buffer Status
	PhysicalAddress frameBufStatDataAddr;
	
	int mbParamDataSize;			// Mb Param for Error Concealment
	PhysicalAddress mbParamDataAddr;
	
	int mvDataSize;				// Motion vector
	int mvNumPerMb;
	PhysicalAddress mvDataAddr;
		
	int userDataNum;			// User Data
	int userDataSize;
	int userDataBufFull;

} DecOutputExtData;
#endif

typedef struct {
	Hdr	hdr_param;
	int indexFrameDisplay;
	int indexFrameDecoded;
	int picType;
	int picTypeFirst;
	int nalType;
	int numOfErrMBs;
	int hScaleFlag;
	int vScaleFlag;
	int	notSufficientPsBuffer;
	int	notSufficientSliceBuffer;
	int	decodingSuccess;
	int interlacedFrame;
	int mp4PackedPBframe;
	int h264PicStruct;
	int idr_first;
	int idr_second;

#ifdef API_CR
    int pictureStructure;
    int topFieldFirst;
    int repeatFirstField;
    int progressiveFrame;
    int fieldSequence;
	DecOutputExtData decOutputExtData;
#endif
	int packedMode;

	int decPicHeight;
	int decPicWidth;

#ifdef PIC_ASPECT_F_RATE
	int picAspectRatio;
	int picFrateNr;
	int	picFrateDr;
    int picFrateDrPrev;
#endif

	Rect decPicCrop;
	int  decPicPoc;
	int  userdataSegNum;
} DecOutputInfo;

//------------------------------------------------------------------------------
// encode struct and definition
//------------------------------------------------------------------------------

typedef struct CodecInst EncInst;
typedef EncInst * EncHandle;

typedef struct {
	int mp4_dataPartitionEnable;
	int mp4_reversibleVlcEnable;
	int mp4_intraDcVlcThr;
	int	mp4_hecEnable;
	int mp4_verid;
} EncMp4Param;

typedef struct {
	int h263_annexJEnable;
	int h263_annexKEnable;
	int h263_annexTEnable;
} EncH263Param;

typedef struct {
	int avc_constrainedIntraPredFlag;
	int avc_disableDeblk;
	int avc_deblkFilterOffsetAlpha;
	int avc_deblkFilterOffsetBeta;
	int avc_chromaQpOffset;
	int avc_audEnable;
	int avc_fmoEnable;
	int avc_fmoSliceNum;
	int avc_fmoType;
	int	avc_fmoSliceSaveBufSize;
} EncAvcParam;

typedef struct {
	int mjpg_sourceFormat;
	int mjpg_restartInterval;
	int mjpg_thumbNailEnable;
	int mjpg_thumbNailWidth;
	int mjpg_thumbNailHeight;
	Uint8 * mjpg_hufTable;
	Uint8 * mjpg_qMatTable;
} EncMjpgParam;

typedef struct{
	int sliceMode;
	int sliceSizeMode;
	int sliceSize;
} EncSliceMode;

typedef struct {
	PhysicalAddress bitstreamBuffer;
	Uint32 bitstreamBufferSize;
	CodStd bitstreamFormat;
	
	int picWidth;
	int picHeight;
	int bitRate;
	int initialDelay;
	int vbvBufferSize;
	int enableAutoSkip;
	int gopSize;

	EncSliceMode slicemode;
	
	int intraRefresh;
	
	int sliceReport;
	int mbReport;
	int mbQpReport;
	int rcIntraQp;	
	int ringBufferEnable;
	union {
		EncMp4Param mp4Param;
		EncH263Param h263Param;
		EncAvcParam avcParam;
		EncMjpgParam mjpgParam;		
	} EncStdParam;
} EncOpenParam;

typedef struct {
	int minFrameBufferCount;
} EncInitialInfo;

typedef struct {
	FrameBuffer * sourceFrame;
	int forceIPicture;
	int skipPicture;
	int quantParam;
	int	picStreamBufferSize;
} EncParam;

typedef struct {
	PhysicalAddress bitstreamBuffer;
	Uint32 bitstreamSize;
	int bitstreamWrapAround; 
	int picType;
	int numOfSlices;
	PhysicalAddress sliceInfo;
	PhysicalAddress mbInfo;
	PhysicalAddress mbQpInfo;
} EncOutputInfo;

typedef struct {
	PhysicalAddress paraSet;
	int size;
} EncParamSet;

typedef struct {
	PhysicalAddress searchRamAddr;
	int SearchRamSize;
} SearchRamParam;

typedef struct {
	PhysicalAddress buf;
	int size;
	int headerType;
} EncHeaderParam;

typedef enum {
	VOL_HEADER,
	VOS_HEADER,
	VIS_HEADER
} Mp4HeaderType;

typedef enum {
	SPS_RBSP,
	PPS_RBSP
} AvcHeaderType;	

typedef struct {
	Uint32 gopNumber;
	Uint32 intraQp;
	Uint32 bitrate;
	Uint32 framerate;
} stChangeRcPara;

#ifdef __cplusplus
extern "C" {
#endif

	int		VPU_IsBusy(void);
	RetCode VPU_Init(PhysicalAddress workBuf,VirtualAddress Virtual_addr,DecOpenParam * pop);
	RetCode VPU_GetVersionInfo( Uint32 *versionInfo );

	// function for encode

	// function for decode
	RetCode VPU_DecOpen(DecHandle *, DecOpenParam *);
	RetCode VPU_DecClose(DecHandle);
	RetCode VPU_DecSetInitialOpt( 
		DecHandle handle);
	RetCode VPU_DecClrState(
		DecHandle handle,
		DecInitialInfo * info);
	RetCode VPU_DecGetInitialInfo(
		DecHandle handle,
		DecInitialInfo * info);
	RetCode VPU_DecRegisterFrameBuffer(
		DecHandle handle,
		FrameBuffer * bufArray,
		int num,
		int stride,
		DecBufInfo * pBufInfo);
	RetCode VPU_DecGetBitstreamBuffer(
		DecHandle handle,
		PhysicalAddress * prdPrt,
		PhysicalAddress * pwrPtr,
		Uint32 * size );
	RetCode VPU_DecUpdateBitstreamBuffer(
		DecHandle handle,
		Uint32 size);
	RetCode VPU_SleepWake(
		DecHandle handle,
		int iSleepWake);
	RetCode VPU_DecStartOneFrame(
		DecHandle handle,
		DecParam *param,
		void *vd);

	RetCode VPU_DecGetOutputInfo(
		DecHandle handle,
		DecOutputInfo * info);
	RetCode VPU_DecBitBufferFlush(
		DecHandle handle);
	RetCode VPU_DecSetDispFlag(
		DecHandle handle, int index);
	RetCode VPU_DecClrDispFlag(
		DecHandle handle, int index);
	RetCode VPU_DecGiveCommand(
		DecHandle handle,
		CodecCommand cmd,
		void * parameter);	
	RetCode VPU_DecResetRdPtr(
		DecHandle handle, 
		PhysicalAddress newRdPtr,
		PhysicalAddress newWrPtr);
	RetCode VPU_DecResyncSeq(
		DecHandle handle);

	RetCode VPU_DecReadExactRdPrt(
		DecHandle handle, 
		PhysicalAddress *prdEndPrt,
		PhysicalAddress *prdStPrt);
	RetCode VPU_DecSetEscSeqInit( 
		DecHandle handle, 
		int escape );

	void    h264ClrPsBuf(int reStart);


#ifdef _CDB
	int BitWaitTrap();
#endif // _CDB

#ifdef __cplusplus
}
#endif

#endif
