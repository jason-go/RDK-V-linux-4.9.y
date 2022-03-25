#ifndef __GX3201_AUDIO_DEC_H__
#define __GX3201_AUDIO_DEC_H__

#define PCM_MODE_NON_INTERLACE

#ifdef PCM_MODE_NON_INTERLACE
#define AUDIO_MAX_FRAME_SAMPLE_LEN        (1024*2*4) //  sample_num*channel_num*byte_num
#else
#define AUDIO_MAX_FRAME_SAMPLE_LEN        (1024*4*4) //  sample_num*channel_num*byte_num
#endif

#define AUDIODEC_HEADER_SIZE              (0x1000)
#define AUDIODEC_PARAMS_SIZE              (0x1000)
#define AUDIODEC_BYPASS_SIZE              (0x1000)
#define AUDIODEC_ADDEC2_SIZE              (0x1000)
#define AUDIODEC_EXTEND_REG_SIZE          (AUDIODEC_HEADER_SIZE+AUDIODEC_PARAMS_SIZE+AUDIODEC_BYPASS_SIZE+AUDIODEC_ADDEC2_SIZE+AUDIODEC_HEADER_SIZE)
#define AUDIODEC_DECODE_FIFO_SIZE         (0x20000)
#define AUDIODEC_CONVERT_FIFO_SIZE        (0x20000)
#define AUDIODEC_DECODE_MEM_SIZE          (AUDIODEC_EXTEND_REG_SIZE+AUDIODEC_DECODE_FIFO_SIZE)
#define AUDIODEC_CONVERT_MEM_SIZE         (AUDIODEC_EXTEND_REG_SIZE+AUDIODEC_CONVERT_FIFO_SIZE)

#define AUDIODEC_BYPASS_ERROR_RUBBISH     (88)
#define AUDIO_BUF_ADDR_MASK               (0xFFFFFFFF)
#define AUDIO_DATALESS_WRITE_TIME         (0)
#define AUDIO_CHANNEL_NOMAL_OUTPUT        (0x00)
#define AUDIO_CHANNEL_MIX_OUTPUT          (0x01)
#define AUDIO_CHANNEL_MONO_OUTPUT         (0x01)
#define DEFAULE_VALUME                    (32)

enum {
	ALLOW_ERROR_CODE   = 1,
	FORBID_ERROR_CODE  = 3,
	DEFENSE_WATER_MARK = 4
};

enum{
	IDLE = 0,
	FIND_SYNC1_DECODE_INFO,
	DECODE_FRAME,
	REPEAT_PCM,
	MOVE_DATA,
	FIND_SYNC2_DECODE_INFO,
	SPD_PARSE,
	FIND_SYNC_DECODE_AD_INFO,
	DECODE_AD_FRAME,
	DECODE_KEY,
	CONVERT_TO_AC3
};


#endif
