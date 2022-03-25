#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gxav_event_type.h"
#include "gxav_common.h"
#include "gxav_firmware.h"
#include "avcore.h"
#include "include/audio_module.h"
#include "include/audio_common.h"
#include "gxav_audiodec_propertytypes.h"
#include "adec/adec.h"
#include "sdc_hal.h"
#include "stc_hal.h"
#include "firewall.h"

#include "gx32xx_dec.h"
#include "gx32xx_reg.h"
#include "gx32xx_debug.h"
#include "porting.h"
#include "profile.h"
#include "log_printf.h"

#define ALIGN_LENGTH   16
#define ALIGN_MASK  0xfffffff0

//#define CONFIG_AUDIO_DEC_BIG_ENDIAN
#ifdef CONFIG_AUDIO_DEC_BIG_ENDIAN
#define GX_AUDIO_SET_VAL_E(reg,val) do {             \
	unsigned int tmpVal = val;             \
	gx_iowrite32(tmpVal, reg);             \
}while(0)
#define GX_AUDIO_GET_ENDIAN_32(reg)     (reg)
#else
#define GX_AUDIO_SET_VAL_E      GX_SET_VAL_E
#define GX_AUDIO_GET_ENDIAN_32  GET_ENDIAN_32
#endif

#define DECODE_0_ID  (1<<0) //解码0功能
#define BYPASS_0_ID  (1<<1) //透传0功能
#define CONVERT_0_ID (1<<1) //转码0功能
#define DECODE_1_ID  (1<<3) //解码1功能

//set *0xa030a148|=(1<<6), 开启固件打印
struct audio_decoder{
	unsigned int                   reg_base_addr;
	struct audiodec_regs          *reg;
	struct audiodec_params_info   *dec_params;
	struct gxav_audio_syncinfo    *sync_info;
	struct gxav_audio_convertinfo *conv_info;
	unsigned int                   extend_reg_addr;
	unsigned int                   extend_reg_size;
};

struct audio_firmware{
#define MAX_CODEC_IN_FMID   16
	unsigned int            loaded;
	GxAvDecoderType         firmware_id;
	unsigned int            firmware_addr;
	unsigned int            firmware_size;
	unsigned int            min_work_buf;
	unsigned int            support_convert_code;
	GxAudioDecProperty_Type codec_type[MAX_CODEC_IN_FMID];
};

#ifdef DEBUG_CHECK_WORKBUF
static int _check_audiodec_firmware(struct gxav_audiodec* audiodec, const char* name, int line);
#endif

#ifdef DEBUG_ADEC_RECORD
static unsigned int   record_buffer_size = DEBUG_ADEC_RECORD_SIZE;
static unsigned char *record_buffer      = NULL;
static unsigned int   record_size        = 0;
static unsigned int   record_over_flags  = 1;
static unsigned int   record_dec_id      = 1;
#endif

static char debug_adec_findpts = DEBUG_ADEC_FIND_PTS;
static char debug_adec_fixpts  = DEBUG_ADEC_FIX_PTS;
static char debug_adec_orprint = DEBUG_ADEC_ORPRINT;
static char debug_adec_esa     = 0;
static char debug_adec_task    = 0;
static char debug_adec_id      = 0;

static unsigned int firmware_protect_flag  = 0;
static unsigned int pcmbuffer_protect_flag = 0;
static unsigned int esabuffer_protect_flag = 0;
static unsigned int workbuf_protect_addr = 0;
static unsigned int workbuf_protect_size = 0;
static char DolbyChip = 0xff;
static struct audio_decoder  audio_dec[2] = {
	{
		.reg_base_addr = 0x04600000,
		.reg           = NULL,
		.dec_params    = NULL,
		.sync_info     = NULL,
		.conv_info     = NULL,
	},
	{
		.reg_base_addr = 0x04600000,
		.reg           = NULL,
		.dec_params    = NULL,
		.sync_info     = NULL,
		.conv_info     = NULL,
	}
};

static struct audio_firmware audio_fim[] = {
	{0, GXAV_FMID_DOLBY,           0, 0, 0x100000,  0, {CODEC_AC3, CODEC_EAC3,-1}},
	{0, GXAV_FMID_AUDIO_VORBIS,    0, 0, 0x100000,  0, {CODEC_VORBIS, -1}},
	{0, GXAV_FMID_AUDIO_OPUS,      0, 0, 0x100000,  0, {CODEC_OPUS,   -1}},
	{0, GXAV_FMID_AUDIO_AVS,       0, 0,      0x0,  0, {-1}},
	{0, GXAV_FMID_AUDIO_DRA,       0, 0,  0x80000,  0, {CODEC_DRA, -1}},
	{0, GXAV_FMID_AUDIO_MPEG,      0, 0, 0x100000,  1,
		{CODEC_MPEG12A, CODEC_MPEG4_AAC, CODEC_DTS, CODEC_PCM, -1}},
	{0, GXAV_FMID_AUDIO_RA_AAC,    0, 0, 0x100000,  0, {CODEC_RA_AAC, -1}},
	{0, GXAV_FMID_AUDIO_RA_RA8LBR, 0, 0, 0x100000,  0, {CODEC_RA_RA8LBR, -1}},
	{0, GXAV_FMID_AUDIO_FLAC,      0, 0,  0x80000,  0, {CODEC_FLAC,-1}},
	{0, GXAV_FMID_AUDIO_SBC,       0, 0,  0x80000,  0, {CODEC_SBC,   -1}},
	{0, GXAV_FMID_AUDIO_FOV,       0, 0, 0x100000,  0, {CODEC_FLAC, CODEC_OPUS, CODEC_VORBIS, -1}},
	{0, GXAV_FMID_AUDIO_ALL,       0, 0, 0x1d0000,  0,
		{CODEC_MPEG12A, CODEC_MPEG4_AAC, CODEC_DRA,  CODEC_DTS, CODEC_FLAC,
		 CODEC_VORBIS,  CODEC_OPUS,      CODEC_EAC3, CODEC_AC3, CODEC_PCM,
		 CODEC_SBC, -1}},
};

static int boost_index_db[51] = {
	-96, -31, -30, -29, -28, -27, -26, -25, -24, -23,
	-22, -21, -20, -19, -18, -17, -16, -15, -14, -13,
	-12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,
	-2,   -1,   0,   1,   2,   3,   4,   5,   6,   7,
	8,    9,  10,  11,  12,  13,  14,  15,  16,  17,
	18
};

struct gxav_module_ops gx3201_audiodec_module;

#define LITTLE_TO_CPU(a) ((((a) & 0xff) << 24) | ((((a) >> 8) & 0xff) << 16) \
		| ((((a) >> 16) & 0xff) << 8) | (((a) >> 24) & 0xff))

#define DECODE_ESA_READ_ADDR_UPDATA_LAST_BYTE(audiodec) \
	do{ \
		if(audiodec->decode_esa_read_addr == 0) {  \
			audiodec->decode_esa_read_addr = audiodec->decode_esa_size - 1; \
		}else \
		audiodec->decode_esa_read_addr --; \
	}while(0)

#define DECODE_ESA_READ_ADDR_UPDATA_NEXT_BYTE(audiodec) \
	do{ \
		audiodec->decode_esa_read_addr ++; \
		if(audiodec->decode_esa_read_addr == audiodec->decode_esa_size) { \
			audiodec->decode_esa_read_addr = 0; \
		} \
	}while(0)

#define DECODE_PTS_READ_ADDR_UPDATA_NEXT_BYTE(audiodec) \
	do{ \
		audiodec->decode_pts_read_addr ++; \
		if(audiodec->decode_pts_read_addr == audiodec->decode_pts_size) { \
			audiodec->decode_pts_read_addr = 0; \
		} \
	}while(0)

#define CONVERT_ESA_READ_ADDR_UPDATA_NEXT_BYTE(audiodec) \
	do{ \
		audiodec->convert_esa_read_addr ++; \
		if(audiodec->convert_esa_read_addr == audiodec->convert_esa_size) { \
			audiodec->convert_esa_read_addr = 0; \
		} \
	}while(0)

#define PCM_WRITE_ADDR_UPDATA_NEXT_BYTE(audiodec) \
	do{ \
		audiodec->out_pcm_writed_addr ++;  \
		if(audiodec->out_pcm_writed_addr == audiodec->out_pcm_size) \
		audiodec->out_pcm_writed_addr = 0; \
	}while(0)

#define DEGRADE_WRITE_ADDR_UPDATA_NEXT_BYTE(audiodec) \
	do{ \
		audiodec->out_degrade_writed_addr ++;  \
		if(audiodec->out_degrade_writed_addr == audiodec->out_degrade_size) \
		audiodec->out_degrade_writed_addr = 0; \
	}while(0)

#define BYPASS_ESA_READ_ADDR_UPDATA_NEXT_BYTE(audiodec) \
	do{ \
		audiodec->bypass_esa_read_addr ++;  \
		if(audiodec->bypass_esa_read_addr == audiodec->bypass_esa_size) \
		audiodec->bypass_esa_read_addr = 0; \
	}while(0)

#define CONVERT_WRITE_ADDR_UPDATA_NEXT_BYTE(audiodec) \
	do{ \
		audiodec->out_convert_writed_addr ++;  \
		if(audiodec->out_convert_writed_addr == audiodec->out_convert_size) \
		audiodec->out_convert_writed_addr = 0; \
	}while(0)

int gx3201_audiodec_get_max_channel(struct gxav_audiodec* audiodec, unsigned int *max_pcm_channel)
{
	*max_pcm_channel = audiodec->max_channel_num;

	return 0;
}

int gx3201_audiodec_get_fifo(struct gxav_audiodec* audiodec, GxAvPinId pin_id, struct gxfifo** fifo)
{
	if ((pin_id == GXAV_PIN_PCM) || (pin_id == GXAV_PIN_ADPCM))
		*fifo = audiodec->pcm_fifo;
	else if (pin_id == GXAV_PIN_AC3) {
		if (audiodec->dec_type == CODEC_EAC3)
			*fifo = audiodec->degrade_fifo;
		else if (audiodec->dec_type == CODEC_AC3)
			*fifo = audiodec->bypass_fifo;
		else if (audiodec->dec_type == CODEC_MPEG4_AAC)
			*fifo = audiodec->encode_fifo;
		else
			*fifo = NULL;
	} else if (pin_id == GXAV_PIN_EAC3 || pin_id == GXAV_PIN_AAC) {
		if (audiodec->dec_type == CODEC_EAC3 || audiodec->dec_type == CODEC_MPEG4_AAC)
			*fifo = audiodec->bypass_fifo;
		else
			*fifo = NULL;
	} else if (pin_id == GXAV_PIN_DTS) {
		if (audiodec->dec_type == CODEC_DTS)
			*fifo = audiodec->bypass_fifo;
		else
			*fifo = NULL;
	}

	return 0;
}

int gx3201_audiodec_set_mono(struct gxav_audiodec *audiodec, int mono_en)
{
	audiodec->mono_en = mono_en;
	audiodec->mono_en_active = 1;
	return 0;
}

int gx3201_audiodec_set_pcminfo(struct gxav_audiodec *audiodec, GxAudioDecProperty_PcmInfo *info)
{
	audiodec->pcm_info   = *info;
	audiodec->pcm_active = 1;
	return 0;
}

static void _get_audiodec_dec_mode_string(GxAudioDecProperty_Mode dec_mode, char *dec_mode_string)
{
	if (dec_mode == DECODE_MODE)
		strcpy(dec_mode_string, "decode");
	else if (dec_mode == BYPASS_MODE)
		strcpy(dec_mode_string, "bypass");
	else if (dec_mode == (DECODE_MODE|BYPASS_MODE))
		strcpy(dec_mode_string, "dec+byp");
	else if (dec_mode == (DECODE_MODE|CONVERT_MODE))
		strcpy(dec_mode_string, "dec+cov");
	else if (dec_mode == (DECODE_MODE|BYPASS_MODE|CONVERT_MODE))
		strcpy(dec_mode_string, "dec+byp+cov");
	else
		strcpy(dec_mode_string, "unkown");

	return;
}

static void _get_audiodec_dec_type_string(GxAudioDecProperty_Type dec_type, char *dec_type_string)
{
	switch (dec_type) {
	case CODEC_MPEG12A:
		strcpy(dec_type_string, "mpeg12");
		break;
	case CODEC_AVSA:
		strcpy(dec_type_string, "avsa");
		break;
	case CODEC_AC3:
		strcpy(dec_type_string, "dolby");
		break;
	case CODEC_EAC3:
		strcpy(dec_type_string, "dolby+");
		break;
	case CODEC_RA_AAC:
		strcpy(dec_type_string, "ra-aac");
		break;
	case CODEC_RA_RA8LBR:
		strcpy(dec_type_string, "ra-ra8lbr");
		break;
	case CODEC_DRA:
		strcpy(dec_type_string, "dra");
		break;
	case CODEC_MPEG4_AAC:
		strcpy(dec_type_string, "mpeg4-aac");
		break;
	case CODEC_WMA:
		strcpy(dec_type_string, "wma");
		break;
	case CODEC_VORBIS:
		strcpy(dec_type_string, "vorbis");
		break;
	case CODEC_OPUS:
		strcpy(dec_type_string, "opus");
		break;
	case CODEC_SBC:
		strcpy(dec_type_string, "sbc");
		break;
	case CODEC_PCM:
		strcpy(dec_type_string, "pcm");
		break;
	case CODEC_DTS:
		strcpy(dec_type_string, "dts");
		break;
	case CODEC_FLAC:
		strcpy(dec_type_string, "flac");
		break;
	default:
		strcpy(dec_type_string, "unkown");
		break;
	}

	return;
}

static unsigned int _get_audiodec_ormem_size(void)
{
	unsigned int fim_idx = 0;
	unsigned int ormem_size = 0;

	for (; fim_idx < sizeof(audio_fim)/sizeof(struct audio_firmware); fim_idx++) {
		if (audio_fim[fim_idx].loaded == 1 &&
				ormem_size <= audio_fim[fim_idx].min_work_buf)
			ormem_size = audio_fim[fim_idx].min_work_buf;
	}

	return ormem_size;
}

static int _load_audiodec_firmware(struct gxav_audiodec *audiodec)
{
	unsigned int mcubase_addr;
	unsigned int firmware_addr;
	unsigned int firmware_size;
	unsigned int ormem_size = _get_audiodec_ormem_size();
	struct gxav_codec_info firmware_info;
	char dec_type_string[16] = {0};
	char dec_mode_string[16] = {0};

	mcubase_addr  = audiodec->or_virt_addr;
	firmware_addr = audiodec->firmware_addr;
	firmware_size = audiodec->firmware_size;
	if(firmware_addr == 0 || firmware_size == 0) {
		gxlog_e(LOG_ADEC,
				"%s %d [addr: %x size: %d]\n",
				__func__, __LINE__, firmware_addr, firmware_size);
		return -1;
	}

	if(firmware_size >= ormem_size) {
		gxlog_e(LOG_ADEC,
				"%s %d [size: 0x%x >= 0x%x]\n",
				__func__, __LINE__, firmware_size, ormem_size);
		return -1;
	}

	if (firmware_protect_flag) {
		//sirius 只能保护firmware size,否则workbuf or不可写操作
		//taurus 保护大小gxloader决定, 因此保护大小ormem最为合适
		if (CHIP_IS_SIRIUS) {
			unsigned int align_size = gxav_firewall_protect_align();
			unsigned int align_firmware_size = 0;

			if (firmware_size%align_size)
				align_firmware_size =(firmware_size/align_size + 1) * align_size;
			else
				align_firmware_size = firmware_size;
			OR_SET_MCU_PROTECT_WORKBUF_SIZE (audiodec->audiodec_reg, ormem_size);
			OR_SET_MCU_POINT_TO_SDRAM_1_SIZE(audiodec->audiodec_reg, align_firmware_size);
			gxav_firewall_register_buffer(GXAV_BUFFER_AUDIO_FIRMWARE, audiodec->or_phys_addr, align_firmware_size);
		} else if (CHIP_IS_TAURUS || CHIP_IS_GEMINI || CHIP_IS_CYGNUS) {
			OR_SET_MCU_PROTECT_WORKBUF_SIZE (audiodec->audiodec_reg, ormem_size);
			OR_SET_MCU_POINT_TO_SDRAM_1_SIZE(audiodec->audiodec_reg, ormem_size);
			gxav_firewall_register_buffer(GXAV_BUFFER_AUDIO_FIRMWARE, audiodec->or_phys_addr, ormem_size);
		} else {
			OR_SET_MCU_POINT_TO_SDRAM_1_SIZE(audiodec->audiodec_reg, ormem_size);
			gxav_firewall_register_buffer(GXAV_BUFFER_AUDIO_FIRMWARE, audiodec->or_phys_addr, ormem_size);
		}
	}

	gxav_firmware_copy((void *)mcubase_addr, (void *)firmware_addr, firmware_size, GXAV_FIRMWARE_AUDIO);
	gxlog_d(LOG_ADEC,
			"%s %d [mcu addr: 0x%x, fw addr: 0x%x size: 0x%x]\n",
			__func__, __LINE__, mcubase_addr, firmware_addr, firmware_size);

	gxav_copy_firmware_info(&firmware_info,
			(unsigned char*)audiodec->firmware_addr, audiodec->firmware_id);
#ifdef CHECK_AUDIODEC_WORK_BUF
	_check_audiodec_firmware(audiodec, __FUNCTION__, __LINE__);
#endif

	_get_audiodec_dec_type_string(audiodec->dec_type, dec_type_string);
	_get_audiodec_dec_mode_string(audiodec->dec_mode, dec_mode_string);
	gxlog_i(LOG_ADEC, "[audio]: %s - %s\n", dec_type_string, dec_mode_string);
	gxlog_i(LOG_ADEC, "[audio]: %s - %s - %s %s\n",
			firmware_info.name, firmware_info.version, firmware_info.date, firmware_info.second);
	gxlog_i(LOG_ADEC, "[audio]: 0x%x - 0x%x - 0x%x\n", firmware_size, ormem_size, workbuf_protect_size);

	return 0;
}

static int _fetch_audiodec_firmware(struct gxav_audiodec *audiodec, GxAudioDecProperty_Type type)
{
	unsigned int fim_idx = 0;
	unsigned int firmware_id = -1;

	if (firmware_protect_flag)
		firmware_id = GXAV_FMID_AUDIO_ALL;

	for (; fim_idx < sizeof(audio_fim)/sizeof(struct audio_firmware); fim_idx++) {
		if ((audio_fim[fim_idx].loaded == 1) &&
				(firmware_id == -1 || audio_fim[fim_idx].firmware_id == firmware_id)) {
			unsigned int codec_idx = 0;
			for (;codec_idx < MAX_CODEC_IN_FMID; codec_idx++) {
				if (audio_fim[fim_idx].codec_type[codec_idx] == type) {
					audiodec->firmware_addr = audio_fim[fim_idx].firmware_addr;
					audiodec->firmware_size = audio_fim[fim_idx].firmware_size;
					audiodec->firmware_id   = audio_fim[fim_idx].firmware_id;

					if (audiodec->firmware_id == GXAV_FMID_AUDIO_ALL) {
						if (((unsigned char*)audiodec->firmware_addr)[0xe0] == 0xfe)
							audiodec->all_firmware_supoort_dolby = 1;
						if (((unsigned char*)audiodec->firmware_addr)[0xe2] == 0x01)
							audiodec->all_firmware_test_dolby = 1;
					}

					if ((audiodec->firmware_id == GXAV_FMID_DOLBY) &&
							((unsigned char*)audiodec->firmware_addr)[0xe0] == 0xfe)
						audiodec->dolby_firmware_support_dolby_plus = 1;
					return 0;
				} else if (audio_fim[fim_idx].codec_type[codec_idx] == -1)
					break;
			}
		}
	}

	return -1;
}

static int _support_audiodec_ad(GxAudioDecProperty_Type type)
{
	unsigned int fim_idx = 0;
	unsigned int firmware_id = -1;

	if (firmware_protect_flag)
		firmware_id = GXAV_FMID_AUDIO_ALL;

	for (; fim_idx < sizeof(audio_fim)/sizeof(struct audio_firmware); fim_idx++) {
		if ((audio_fim[fim_idx].loaded == 1) &&
				(firmware_id == -1 || audio_fim[fim_idx].firmware_id == firmware_id)) {
			unsigned int codec_idx = 0;
			for (;codec_idx < MAX_CODEC_IN_FMID; codec_idx++) {
				if (audio_fim[fim_idx].codec_type[codec_idx] == type) {
					unsigned char *firmware_addr = (unsigned char *)audio_fim[fim_idx].firmware_addr;
					if (audio_fim[fim_idx].firmware_id == GXAV_FMID_AUDIO_MPEG) {
						if (firmware_addr[0xe0] != 0xf0)
							return 1;
					} else if (audio_fim[fim_idx].firmware_id == GXAV_FMID_AUDIO_ALL) {
						if ((type == CODEC_EAC3) || (type == CODEC_AC3)) {
							if (firmware_addr[0xe3] == 0x01)
								return 1;
						} else
							return 1;
					}
					return 0;
				} else if (audio_fim[fim_idx].codec_type[codec_idx] == -1)
					break;
			}
		}
	}

	return 0;
}

static void _scan_audiodec_firmware(void)
{
	unsigned int addr = 0;
	unsigned int size = 0;
	unsigned int idx = 0;

	for(;idx < sizeof(audio_fim)/sizeof(struct audio_firmware); idx++) {
		int ret = gxav_firmware_fetch(audio_fim[idx].firmware_id, &addr, &size);
		if (ret == 0 && addr && size) {
			if (firmware_protect_flag) {
				if (audio_fim[idx].firmware_id == GXAV_FMID_AUDIO_ALL) {
					audio_fim[idx].loaded = 1;
					audio_fim[idx].firmware_addr = addr;
					audio_fim[idx].firmware_size = size;
					gxav_firmware_check_all_workbuf(audio_fim[idx].firmware_id,
							(unsigned char*)addr, &audio_fim[idx].min_work_buf);
				}
			} else {
				audio_fim[idx].loaded = 1;
				audio_fim[idx].firmware_addr = addr;
				audio_fim[idx].firmware_size = size;
				if (audio_fim[idx].firmware_id == GXAV_FMID_DOLBY) {
					gxav_firmware_check_dolby_workbuf(audio_fim[idx].firmware_id,
							(unsigned char*)addr, &audio_fim[idx].min_work_buf);
				} else if (audio_fim[idx].firmware_id == GXAV_FMID_AUDIO_ALL) {
					gxav_firmware_check_all_workbuf(audio_fim[idx].firmware_id,
							(unsigned char*)addr, &audio_fim[idx].min_work_buf);
				} else if (audio_fim[idx].firmware_id == GXAV_FMID_AUDIO_MPEG) {
					gxav_firmware_check_mpeg_workbuf(audio_fim[idx].firmware_id,
							(unsigned char*)addr, &audio_fim[idx].min_work_buf, &audio_fim[idx].support_convert_code);
				}
			}
		}
	}
	return;
}

static void _set_dolby_header_info(struct gxav_audiodec* audiodec, struct dolby_info_header* dolby_header)
{
	dolby_header->b_exist          = LITTLE_TO_CPU(1          ); //-b
	dolby_header->pcmwordtype      = LITTLE_TO_CPU(0          );
	dolby_header->c_exist          = LITTLE_TO_CPU(0          ); //-c
	dolby_header->kmode            = LITTLE_TO_CPU(0          );
	dolby_header->d_exist          = LITTLE_TO_CPU(0          ); //-d
	dolby_header->f_exist          = LITTLE_TO_CPU(0          ); //-f
	dolby_header->f                = LITTLE_TO_CPU(0          );
	dolby_header->dp_exist         = LITTLE_TO_CPU(0          );
	dolby_header->p                = LITTLE_TO_CPU(0          );
	dolby_header->h_exist          = LITTLE_TO_CPU(0          ); //-h
	dolby_header->i_exist          = LITTLE_TO_CPU(0          ); //-i
	dolby_header->k_exist          = LITTLE_TO_CPU(0          ); //-k
	dolby_header->compmode         = LITTLE_TO_CPU(0          );
	dolby_header->l_exist          = LITTLE_TO_CPU(0          ); //-l
	dolby_header->outlfe           = LITTLE_TO_CPU(0          );
	dolby_header->m_exist          = LITTLE_TO_CPU(0          ); //-m
	dolby_header->mr_exist         = LITTLE_TO_CPU(0          ); //-mr
	dolby_header->m                = LITTLE_TO_CPU(0          );
	dolby_header->n_exist          = LITTLE_TO_CPU(0          ); //-n
	dolby_header->outnchans        = LITTLE_TO_CPU(0          );
	dolby_header->o_exist          = LITTLE_TO_CPU(0          ); //-o
	dolby_header->oc_exist         = LITTLE_TO_CPU(0          );
	dolby_header->op_exist         = LITTLE_TO_CPU(0          );
	dolby_header->od_exist         = LITTLE_TO_CPU(0          );
	dolby_header->p_exist          = LITTLE_TO_CPU(0          ); //-p
	dolby_header->pcmscale         = LITTLE_TO_CPU(0          ); //((x + 0.005) * 100)
	dolby_header->q_exist          = LITTLE_TO_CPU(0          ); //-q
	dolby_header->r_exist          = LITTLE_TO_CPU(0          ); //-r
	dolby_header->framestart       = LITTLE_TO_CPU(0          );
	dolby_header->frameend         = LITTLE_TO_CPU(0xffffffff );
	dolby_header->s_exist          = LITTLE_TO_CPU(0          ); //-s
	dolby_header->stereomode       = LITTLE_TO_CPU(0          );
	dolby_header->u_exist          = LITTLE_TO_CPU(0          ); //-u
	dolby_header->dualmode         = LITTLE_TO_CPU(0          );
	dolby_header->v_exist          = LITTLE_TO_CPU(0          ); //-v
	dolby_header->verbose          = LITTLE_TO_CPU(0          );
	dolby_header->w_exist          = LITTLE_TO_CPU(0          ); //-w
	dolby_header->upsample         = LITTLE_TO_CPU(0          );
	dolby_header->x_exist          = LITTLE_TO_CPU(0          ); //-x
	dolby_header->dynscalehigh     = LITTLE_TO_CPU(0          ); //((x + 0.005) * 100)
	dolby_header->y_exist          = LITTLE_TO_CPU(0          ); //-y
	dolby_header->dynscalelow      = LITTLE_TO_CPU(0          ); //((x + 0.005) * 100     );
	dolby_header->z_exist          = LITTLE_TO_CPU(0          ); //-z
	dolby_header->zp_exist         = LITTLE_TO_CPU(0          );
	dolby_header->zd_exist         = LITTLE_TO_CPU(0          );
	dolby_header->num_exist        = LITTLE_TO_CPU(0          ); //how much -0~7
	dolby_header->chanval[0]       = LITTLE_TO_CPU(0          ); //-0
	dolby_header->chanval[1]       = LITTLE_TO_CPU(0          ); //-1
	dolby_header->chanval[2]       = LITTLE_TO_CPU(0          ); //-2
	dolby_header->chanval[3]       = LITTLE_TO_CPU(0          ); //-3
	dolby_header->chanval[4]       = LITTLE_TO_CPU(0          ); //-4
	dolby_header->chanval[5]       = LITTLE_TO_CPU(0          ); //-5
	dolby_header->chanval[6]       = LITTLE_TO_CPU(0          ); //-6
	dolby_header->chanval[7]       = LITTLE_TO_CPU(0          ); //-7
	dolby_header->routing_index[0] = LITTLE_TO_CPU(0          );
	dolby_header->routing_index[1] = LITTLE_TO_CPU(0          );
	dolby_header->routing_index[2] = LITTLE_TO_CPU(0          );
	dolby_header->routing_index[3] = LITTLE_TO_CPU(0          );
	dolby_header->routing_index[4] = LITTLE_TO_CPU(0          );
	dolby_header->routing_index[5] = LITTLE_TO_CPU(0          );
	dolby_header->routing_index[6] = LITTLE_TO_CPU(0          );
	dolby_header->routing_index[7] = LITTLE_TO_CPU(0          );
	dolby_header->byterev          = LITTLE_TO_CPU(0          );
	dolby_header->ddoutbaseaddr    = LITTLE_TO_CPU(0xc08ba000 ); //0xc7a00000  0xc08ba000
	dolby_header->ddoutlength      = LITTLE_TO_CPU((500*1024) );
	dolby_header->ddoutid          = LITTLE_TO_CPU(4          ); //8

	if ((audiodec->dec_type == CODEC_EAC3) &&
			((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE)) {
		dolby_header->z_exist = LITTLE_TO_CPU(0);
		dolby_header->zd_exist = LITTLE_TO_CPU(0);
	} else {
		dolby_header->z_exist = LITTLE_TO_CPU(1);
		dolby_header->zd_exist = LITTLE_TO_CPU(1);
	}
}

static int _get_audiodec_sdram_freq(void)
{
	switch(gxcore_chip_probe())
	{
	case GXAV_ID_GX3211:
		return 240000000;
	case GXAV_ID_GX3201:
	case GXAV_ID_GX6605S:
	case GXAV_ID_SIRIUS:
	case GXAV_ID_TAURUS:
	case GXAV_ID_GEMINI:
	case GXAV_ID_CYGNUS:
		return 237000000;
	case GXAV_ID_GX3113C:
		return 144000000;
	default:
		return 0;
	}
}

static int _config_audiodec_address(struct gxav_audiodec* audiodec, GxAudioDecProperty_Config *config)
{
	unsigned int mcu_point_sdram;
	unsigned int addr_offset = 0;
	struct audiodec_params_info *params  = NULL;
	struct audiodec_regs* audiodec_reg   = audiodec->audiodec_reg;
	unsigned int ormem_size = _get_audiodec_ormem_size();

	if (workbuf_protect_addr) {
		if (config->ormem_start != NULL)
			gxlog_w(LOG_ADEC,
					"ormem addr (%p) replace (%p)\n",
					config->ormem_start,
					(void*)gx_phys_to_virt(workbuf_protect_addr));
		config->ormem_start = (void*)gx_phys_to_virt(workbuf_protect_addr);
		config->ormem_size  = workbuf_protect_size + AUDIODEC_EXTEND_REG_SIZE;
	}

	if (config->ormem_start == NULL ||
			config->ormem_size < (ormem_size + AUDIODEC_EXTEND_REG_SIZE)) {
		gxlog_e(LOG_ADEC, "%s %d [%x %x %x %x]\n",
				__func__, __LINE__,
				workbuf_protect_size,
				config->ormem_size,
				ormem_size,
				AUDIODEC_EXTEND_REG_SIZE);
		return -1;
	}

	//ormem start info
	audiodec->or_virt_addr = (unsigned int)(config->ormem_start);
	audiodec->or_phys_addr = (unsigned int)gx_virt_to_phys((unsigned int)(config->ormem_start));
	OR_SET_PRG_START_ADDR(audiodec_reg, audiodec->or_phys_addr);

	audiodec->regmem_virt_addr = audiodec->or_virt_addr + ormem_size;
	audiodec->regmem_virt_size = AUDIODEC_EXTEND_REG_SIZE;
	audio_dec[0].extend_reg_addr = audiodec->regmem_virt_addr;
	audio_dec[0].extend_reg_size = audiodec->regmem_virt_size;
	audio_dec[1].extend_reg_addr = audiodec->regmem_virt_addr;
	audio_dec[1].extend_reg_size = audiodec->regmem_virt_size;

	//decoder info
	addr_offset = ormem_size;
	audiodec->dec_params = (void*)(audiodec->or_virt_addr + addr_offset);
	mcu_point_sdram = (audiodec->or_phys_addr + addr_offset);
	OR_SET_MCU_POINT_TO_SDRAM_1_ADDR(audiodec_reg, (mcu_point_sdram & AUDIO_BUF_ADDR_MASK));
	params = (struct audiodec_params_info *)(audiodec->dec_params);
	GX_AUDIO_SET_VAL_E(&(params->or_clk), _get_audiodec_sdram_freq());
	GX_AUDIO_SET_VAL_E(&(params->dataless_wait_time), AUDIO_DATALESS_WRITE_TIME);
	GX_AUDIO_SET_VAL_E(&(params->frame_nums),        0);
	GX_AUDIO_SET_VAL_E(&(params->extern_downmix_en), 0);
	GX_AUDIO_SET_VAL_E(&(params->ad_ctrl_vol),       DEFAULE_VALUME);
	GX_AUDIO_SET_VAL_E(&(params->dec_mov_ac3_data),  0);
	GX_AUDIO_SET_VAL_E(&(params->dec_volume),        DEFAULE_VALUME);
	GX_AUDIO_SET_VAL_E(&(params->dec_volume2),       DEFAULE_VALUME);
	GX_AUDIO_SET_VAL_E(&(params->anti_error_dec),    FORBID_ERROR_CODE);
	GX_AUDIO_SET_VAL_E(&(params->convert_en),        0);
	GX_AUDIO_SET_VAL_E(&(params->aac_parse_en),      0);
	GX_AUDIO_SET_VAL_E(&(params->aac_downsrc_en),    0);
	GX_AUDIO_SET_VAL_E(&(params->mag_value),         0);
	GX_AUDIO_SET_VAL_E(&(params->mag_value_ad),      0);
	GX_AUDIO_SET_VAL_E(&(params->or_key_type),       0);
	GX_AUDIO_SET_VAL_E(&(params->or_chip_type),      0xffffffff);
	GX_AUDIO_SET_VAL_E(&(params->dec_vol_tab_set_en),2 );
	audio_dec[1].dec_params = audio_dec[0].dec_params = audiodec->dec_params;

	//decoder header info
	addr_offset = ormem_size + AUDIODEC_PARAMS_SIZE;
	audiodec->dec_header = (void*)(audiodec->or_virt_addr + addr_offset);

	//sync info
	addr_offset = ormem_size + AUDIODEC_HEADER_SIZE + AUDIODEC_PARAMS_SIZE;
	mcu_point_sdram = (audiodec->or_phys_addr + addr_offset);
	audiodec->syncinfo = (struct gxav_audio_syncinfo *)(audiodec->or_virt_addr + addr_offset);
	OR_SET_MCU_POINT_TO_SDRAM_2_ADDR(audiodec_reg, (mcu_point_sdram & AUDIO_BUF_ADDR_MASK));
	gx_memset(audiodec->syncinfo, 0, sizeof(struct gxav_audio_syncinfo));
	audiodec->syncinfo->stream_big_or_little = GX_AUDIO_GET_ENDIAN_32(0);
	audio_dec[0].sync_info = audio_dec[1].sync_info = audiodec->syncinfo;

	//该基础器用于第2路解码,必须基于第1路解码
	addr_offset = ormem_size + AUDIODEC_HEADER_SIZE + AUDIODEC_PARAMS_SIZE + AUDIODEC_BYPASS_SIZE;
	mcu_point_sdram = (audiodec->or_phys_addr + addr_offset);
	audio_dec[1].reg = (struct audiodec_regs *)(audiodec->or_virt_addr + addr_offset);
	OR_SET_MCU_POINT_TO_SDRAM_3_ADDR(audiodec_reg, (mcu_point_sdram & AUDIO_BUF_ADDR_MASK));
	gx_memset(audio_dec[1].reg, 0, sizeof(struct audiodec_regs));

	//convert info
	addr_offset = ormem_size + 2*AUDIODEC_HEADER_SIZE + AUDIODEC_PARAMS_SIZE + AUDIODEC_BYPASS_SIZE;
	mcu_point_sdram = (audiodec->or_phys_addr + addr_offset);
	audiodec->convertinfo = (struct gxav_audio_convertinfo *)(audiodec->or_virt_addr + addr_offset);
	OR_SET_MCU_POINT_TO_SDRAM_4_ADDR(audiodec_reg, (mcu_point_sdram & AUDIO_BUF_ADDR_MASK));
	gx_memset(audiodec->convertinfo, 0, sizeof(struct gxav_audio_convertinfo));
	audiodec->convertinfo->convert_num          = GX_AUDIO_GET_ENDIAN_32(1);
	audiodec->convertinfo->stream_big_or_little = GX_AUDIO_GET_ENDIAN_32(0);
	audio_dec[0].conv_info = audio_dec[1].conv_info = audiodec->convertinfo;

	return 0;
}

static int _config_audiodec_header(struct gxav_audiodec *audiodec, GxAudioDecProperty_Config *config)
{
	struct audiodec_params_info *params = NULL;
	struct audiodec_regs* audiodec_reg  = audiodec->audiodec_reg;
	int anti_error_level = FORBID_ERROR_CODE;

	switch (config->anti_error_code) {
	case 0:
		anti_error_level = ALLOW_ERROR_CODE;
		break;
	case 1:
		anti_error_level = FORBID_ERROR_CODE;
		break;
	case 2:
		anti_error_level = DEFENSE_WATER_MARK;
		break;
	default:
		anti_error_level = FORBID_ERROR_CODE;
		break;
	}

	params = (struct audiodec_params_info *)(audiodec->dec_params);
	switch(config->type) {
	case CODEC_MPEG12A:
		OR_SET_CODEC_TYPE(audiodec_reg, 1);
		GX_AUDIO_SET_VAL_E(&(params->header_addr), 0);
		GX_AUDIO_SET_VAL_E(&(params->anti_error_dec), anti_error_level);
		break;
	case CODEC_DRA:
		OR_SET_CODEC_TYPE(audiodec_reg, 8);
		GX_AUDIO_SET_VAL_E(&(params->header_addr), 0);
		break;
	case CODEC_RA_AAC:
	case CODEC_RA_RA8LBR:
		{
			unsigned int header_address = 0;

			OR_SET_CODEC_TYPE(audiodec_reg, (audiodec->firmware_id==GXAV_FMID_AUDIO_RA_AAC)?3:4);
			if (config->u.real.header_size == 0) {
				gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
				return -1;
			}
			gx_memcpy(audiodec->dec_header, &config->u.real.header[0], config->u.real.header_size);
			header_address = (unsigned int)(audiodec->dec_header) - audiodec->or_virt_addr;
			GX_AUDIO_SET_VAL_E(&(params->header_addr), (header_address & AUDIO_BUF_ADDR_MASK));
		}
		break;
	case CODEC_MPEG4_AAC:
		OR_SET_CODEC_TYPE(audiodec_reg, 5);
		GX_AUDIO_SET_VAL_E(&(params->header_addr), 0);
		break;
	case CODEC_AC3:
	case CODEC_EAC3:
		{
			if (audiodec->firmware_id == GXAV_FMID_AUDIO_MPEG) {
				OR_SET_CODEC_TYPE(audiodec_reg, 1);
				GX_AUDIO_SET_VAL_E(&(params->header_addr), 0);
				GX_AUDIO_SET_VAL_E(&(params->anti_error_dec), anti_error_level);
			} else {
				unsigned int header_address = 0;
				struct dolby_info_header dolby_header;

				OR_SET_CODEC_TYPE(audiodec_reg, 6);
				_set_dolby_header_info(audiodec, &dolby_header);
				gx_memcpy(audiodec->dec_header, &dolby_header, sizeof(struct dolby_info_header));
				header_address = (unsigned int)(audiodec->dec_header) - audiodec->or_virt_addr;
				GX_AUDIO_SET_VAL_E(&(params->header_addr), (header_address & AUDIO_BUF_ADDR_MASK));
				audiodec->syncinfo->bypass_code_type = GX_AUDIO_GET_ENDIAN_32(0x0);
			}
		}
		break;
	case CODEC_DTS:
		{
			OR_SET_CODEC_TYPE(audiodec_reg, 1);
			GX_AUDIO_SET_VAL_E(&(params->header_addr), 0);
			audiodec->syncinfo->bypass_code_type = GX_AUDIO_GET_ENDIAN_32(0x1);
		}
		break;
	case CODEC_VORBIS:
		OR_SET_CODEC_TYPE(audiodec_reg, 10);
		GX_SET_VAL_E(&(params->header_addr), 0);
		break;
	case CODEC_OPUS:
		OR_SET_CODEC_TYPE(audiodec_reg, 11);
		GX_SET_VAL_E(&(params->header_addr), 0);
		break;
	case CODEC_FLAC:
		OR_SET_CODEC_TYPE(audiodec_reg, 12);
		GX_SET_VAL_E(&(params->header_addr), 0);
		break;
	case CODEC_PCM:
		OR_SET_CODEC_TYPE(audiodec_reg, 14);
		GX_SET_VAL_E(&(params->header_addr), 0);
		break;
	case CODEC_SBC:
		OR_SET_CODEC_TYPE(audiodec_reg, 15);
		GX_SET_VAL_E(&(params->header_addr), 0);
		break;
	default:
		break;
	}

	return 0;
}

static int _config_audiodec_decmode(struct gxav_audiodec *audiodec, GxAudioDecProperty_Config *config)
{
	if ((config->mode < DECODE_MODE) || (config->mode > (DECODE_MODE|BYPASS_MODE|CONVERT_MODE))) {
		gxlog_e(LOG_ADEC, "%s %d [decode mode config error]\n", __func__, __LINE__);
		return -1;
	}

	if (config->type == CODEC_DTS) {
		if (_fetch_audiodec_firmware(audiodec, config->type) < 0) {
			gxlog_e(LOG_ADEC, "%s %d [not find firmware]\n", __func__, __LINE__);
			return -1;
		}
		if (config->mode == DECODE_MODE) {
			gxlog_e(LOG_ADEC, "%s %d [only bypass mode]\n", __func__, __LINE__);
			return -1;
		}
		audiodec->dec_type = config->type;
		audiodec->dec_mode = BYPASS_MODE;
	} else if ((config->type == CODEC_EAC3) || (config->type == CODEC_AC3)) {
		if (_fetch_audiodec_firmware(audiodec, config->type) != 0) {
			//no dolby firmware, select mpeg2 firmware to bypass
			if (_fetch_audiodec_firmware(audiodec, CODEC_MPEG12A) < 0) {
				gxlog_e(LOG_ADEC, "%s %d [not find firmware]\n", __func__, __LINE__);
				return -1;
			}
			if (config->mode == DECODE_MODE) {
				gxlog_e(LOG_ADEC, "%s %d [only bypass mode]\n", __func__, __LINE__);
				return -1;
			}
			audiodec->dec_type = config->type;
			audiodec->dec_mode = BYPASS_MODE;
		} else {
			if (audiodec->firmware_id == GXAV_FMID_AUDIO_ALL) {
				if (audiodec->all_firmware_supoort_dolby) {
					audiodec->dec_type = config->type;
					audiodec->dec_mode = config->mode;
				} else {
					if (config->mode == DECODE_MODE) {
						gxlog_e(LOG_ADEC, "%s %d [not support decode]\n", __func__, __LINE__);
						return -1;
					}
					audiodec->dec_type = config->type;
					audiodec->dec_mode = BYPASS_MODE;
				}
			} else if(audiodec->firmware_id == GXAV_FMID_DOLBY) {
				if (audiodec->dolby_firmware_support_dolby_plus) {
					audiodec->dec_type = config->type;
					audiodec->dec_mode = config->mode;
				} else {
					if (config->type == CODEC_EAC3) {
						if (config->mode == DECODE_MODE) {
							gxlog_e(LOG_ADEC, "%s %d [not support decode]\n", __func__, __LINE__);
							return -1;
						}
						audiodec->dec_type = config->type;
						audiodec->dec_mode = BYPASS_MODE;
					} else {
						audiodec->dec_type = config->type;
						audiodec->dec_mode = config->mode;
					}
				}
			} else {
				audiodec->dec_type = config->type;
				audiodec->dec_mode = BYPASS_MODE;
			}
		}

		if (audiodec->dec_mode == (BYPASS_MODE|DECODE_MODE)) {
			unsigned int ormem_size = _get_audiodec_ormem_size();
			if (config->ormem_size >= (ormem_size + AUDIODEC_DECODE_MEM_SIZE)) {
				audiodec->decode_channel_buffer = audiodec->or_virt_addr + ormem_size + AUDIODEC_EXTEND_REG_SIZE;
				audiodec->decode_channel_size   = AUDIODEC_DECODE_FIFO_SIZE;
			} else {
				audiodec->decode_channel_buffer = 0;
				audiodec->decode_channel_size   = 0;
				audiodec->dec_mode = DECODE_MODE;
			}
		}
	} else if (config->type == CODEC_MPEG4_AAC) {
		struct audiodec_params_info *params = (struct audiodec_params_info *)(audiodec->dec_params);

		if (_fetch_audiodec_firmware(audiodec, config->type) < 0) {
			gxlog_e(LOG_ADEC, "%s %d [not find firmware]\n", __func__, __LINE__);
			return -1;
		}

		audiodec->dec_type  = config->type;
		audiodec->dec_mode  = config->mode;

		if ((audiodec->dec_mode & CONVERT_MODE) == CONVERT_MODE) {
			unsigned int ormem_size = _get_audiodec_ormem_size();

			if (config->ormem_size >= (ormem_size + AUDIODEC_CONVERT_MEM_SIZE)) {
				audiodec->convert_channel_buffer = audiodec->or_virt_addr + ormem_size + AUDIODEC_EXTEND_REG_SIZE;
				audiodec->convert_channel_size   = AUDIODEC_CONVERT_FIFO_SIZE;
				GX_AUDIO_SET_VAL_E(&(params->convert_en), 1);
				audiodec->dec_mode |= DECODE_MODE; //convert need decode
			} else {
				audiodec->convert_channel_buffer = 0;
				audiodec->convert_channel_size   = 0;
				audiodec->dec_mode &= ~CONVERT_MODE;
			}
		}

		if ((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE) {
			GX_AUDIO_SET_VAL_E(&(params->aac_parse_en), 1);
			audiodec->dec_mode |= DECODE_MODE; //bypass need decode
		}

		if (audiodec->dec_mode == 0) {
			gxlog_e(LOG_ADEC, "%s %d [not support decode]\n", __func__, __LINE__);
			return -1;
		}
	} else {
		if (config->mode == BYPASS_MODE) {
			gxlog_e(LOG_ADEC, "%s %d [only decode mode]\n", __func__, __LINE__);
			return -1;
		}

		if (_fetch_audiodec_firmware(audiodec, config->type) < 0) {
			gxlog_e(LOG_ADEC, "%s %d [not find firmware]\n", __func__, __LINE__);
			return -1;
		}
		audiodec->dec_mode = DECODE_MODE;
		audiodec->dec_type = config->type;
	}

	if (_load_audiodec_firmware(audiodec) < 0)
		return -1;

	return 0;
}

static int _config_audiodec_downmix(struct gxav_audiodec* audiodec, int downmix)
{
	struct audiodec_params_info *decctrl_params = NULL;

	decctrl_params = (struct audiodec_params_info *)(audiodec->dec_params);
	if (decctrl_params) {
		audiodec->downmix = (audiodec->dec_type == CODEC_DRA)?1:downmix;
		if (audiodec->downmix)
			GX_AUDIO_SET_VAL_E((unsigned int)(&(decctrl_params->extern_downmix_en)), AUDIO_CHANNEL_MIX_OUTPUT);
		else
			GX_AUDIO_SET_VAL_E((unsigned int)(&(decctrl_params->extern_downmix_en)), AUDIO_CHANNEL_NOMAL_OUTPUT);
	}
	return 0;
}

static int _config_audiodec_mono(struct gxav_audiodec* audiodec, int mono_en)
{
	struct audiodec_params_info *decctrl_params = NULL;

	decctrl_params = (struct audiodec_params_info *)(audiodec->dec_params);
	if (decctrl_params) {
		audiodec->mono_en = mono_en;
		if (audiodec->mono_en)
			GX_AUDIO_SET_VAL_E((unsigned int)(&(decctrl_params->extern_mono_en)), AUDIO_CHANNEL_MONO_OUTPUT);
		else
			GX_AUDIO_SET_VAL_E((unsigned int)(&(decctrl_params->extern_mono_en)), AUDIO_CHANNEL_NOMAL_OUTPUT);
	}
	return 0;
}

static int _config_audiodec_volumedb(struct gxav_audiodec* audiodec, int volumedb)
{
	struct audiodec_params_info *decctrl_params = NULL;

	decctrl_params = (struct audiodec_params_info *)(audiodec->dec_params);
	if(decctrl_params){
		GX_AUDIO_SET_VAL_E(&(decctrl_params->dec_volume),  volumedb);
		GX_AUDIO_SET_VAL_E(&(decctrl_params->dec_volume2), volumedb);
		audiodec->volumedb = volumedb;
	}
	return 0;
}

static int _config_audiodec_boostdb(struct gxav_audiodec* audiodec, int boostdb)
{
	struct audiodec_params_info *decctrl_params = NULL;

	decctrl_params = (struct audiodec_params_info *)(audiodec->dec_params);
	if(decctrl_params){
		if (audiodec->dec_id == 0)
			GX_AUDIO_SET_VAL_E(&(decctrl_params->mag_value),  boostdb);
		else if (audiodec->dec_id == 1)
			GX_AUDIO_SET_VAL_E(&(decctrl_params->mag_value_ad), boostdb);
		audiodec->boostdb = boostdb;
	}
	return 0;
}

static void _check_audiodec_chip_type(struct audiodec_regs* audiodec_reg)
{
	struct gxav_audiodec* audiodec;
	GxAudioDecProperty_Config* config;
	unsigned int ormem_size = 0, free_flags = 0;

	if (DolbyChip != 0xff)
		return;

	audiodec = gx_mallocz(sizeof(struct gxav_audiodec));
	config = gx_mallocz(sizeof(GxAudioDecProperty_Config));

	ormem_size = _get_audiodec_ormem_size();
	audiodec->audiodec_reg = audiodec_reg;
	if (workbuf_protect_addr) {
		free_flags = 0;
		config->ormem_size  = workbuf_protect_size + AUDIODEC_EXTEND_REG_SIZE;
		config->ormem_start = (void*)gx_phys_to_virt(workbuf_protect_addr);
	} else {
		free_flags = 1;
		config->ormem_size  = ormem_size + AUDIODEC_EXTEND_REG_SIZE;
		config->ormem_start = gx_page_malloc(config->ormem_size);
	}
	config->mode = DECODE_MODE|BYPASS_MODE;
	config->type = CODEC_AC3;
	config->anti_error_code = 1;
	if (config->ormem_start) {
		int ret0 = 0, ret1 = 0, retry = 100;

		_scan_audiodec_firmware();
		if (_config_audiodec_address(audiodec, config) < 0)
			goto CHECK_CHIP_EXIT;

		if (_config_audiodec_decmode(audiodec, config) < 0)
			goto CHECK_CHIP_EXIT;

		OR_SET_AUDIO_PRINT(audiodec_reg, debug_adec_orprint);
		OR_CLR_OTP_ERR_INT_STATUS0(audiodec_reg);
		OR_CLR_OTP_ERR_INT_STATUS1(audiodec_reg);
		ret0 = OR_GET_OTP_ERR_INT_STATUS0(audiodec_reg);
		ret1 = OR_GET_OTP_ERR_INT_STATUS1(audiodec_reg);
		OR_SET_DEC_START(audiodec_reg);

		while(retry > 0){
			ret0 = OR_GET_OTP_ERR_INT_STATUS0(audiodec_reg);
			ret1 = OR_GET_OTP_ERR_INT_STATUS1(audiodec_reg);
			if((ret0 == 0) && (ret1 == 0)){
				retry--;
				gx_mdelay(10);
			}else
				break;
		}
		if (ret0 == 1) {
			struct audiodec_params_info *params = (struct audiodec_params_info *)(audiodec->dec_params);
			gx_dcache_inv_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);
			DolbyChip = 0;
			gxlog_i(LOG_ADEC, "Audio D Chip Not [0x%0x]\n", params->or_key_type);
		} else if(ret1 == 1) {
			DolbyChip = 1;
			gxlog_i(LOG_ADEC, "Audio D Chip Yes\n");
		} else {
			DolbyChip = 0;
			gxlog_i(LOG_ADEC, "Audio Chip Probe Unknow Type (Timeout ?)\n");
		}

		OR_CLR_OTP_ERR_INT_STATUS0(audiodec_reg);
		OR_CLR_OTP_ERR_INT_STATUS1(audiodec_reg);
		OR_SET_CPU_RST_OR(audiodec_reg);
		while (!(OR_GET_OR_RST_OK_INT_STATUS(audiodec_reg))) {//解决音频固件总线死机问题
			int epc = REG_GET_VAL(&(audiodec_reg->audio_mcu_expc));
			if(epc == 0)
				break;
			gxlog_d(LOG_ADEC, "%s %d [epc: 0x%x]\n", __func__, __LINE__, epc);
			gx_mdelay(10);
		}
		OR_SET_DEC_RESET(audiodec_reg);

CHECK_CHIP_EXIT:
		if (free_flags)
			gx_page_free(config->ormem_start, config->ormem_size);
	}
	gx_free(config);
	gx_free(audiodec);
	if (DolbyChip == 0xff) {
		DolbyChip = 0;
		gxlog_i(LOG_ADEC, "Audio Chip Probe Config Error\n");
	}
}

int _check_audiodec_active_params(struct gxav_audiodec* audiodec)
{
	if (audiodec->volumedb_active) {
		_config_audiodec_volumedb(audiodec, audiodec->volumedb);
		audiodec->volumedb_active = 0;
	}
	if (audiodec->boostdb_active) {
		_config_audiodec_boostdb(audiodec, audiodec->boostdb);
		audiodec->boostdb_active = 0;
	}
	if (audiodec->downmix_active) {
		_config_audiodec_downmix(audiodec, audiodec->downmix);
		audiodec->downmix_active = 0;
	}
	if (audiodec->mono_en_active) {
		_config_audiodec_mono(audiodec, audiodec->mono_en);
		audiodec->mono_en_active = 0;
	}
	return 0;
}

#ifdef CHECK_AUDIODEC_WORK_BUF
static int _check_audiodec_firmware(struct gxav_audiodec* audiodec, const char* name, int line)
{
	unsigned int i = 0, flag = 0;

	for (i = 0; i < audiodec->firmware_size; i++) {
		unsigned char *or_data = (unsigned char *)(audiodec->or_virt_addr);
		unsigned char *fw_data = (unsigned char *)(audiodec->firmware_addr);
		if (or_data[i] != fw_data[i]) {
			int j = 0;
			gxlog_d(LOG_ADEC, "--------------err: %s %d--------------\n", name, line);
			gxlog_d(LOG_ADEC, "saddr: %p %p\n", or_data, fw_data);
			for (j = 0; j < 8; j++) {
				gxlog_d(LOG_ADEC, "%02d: (0x%02x, 0x%02x)\n",
						(i + j), or_data[i + j], fw_data[i + j]);
			}
			flag = 1;
			i += 7;
		}
	}

	if (flag) {
		while(1) {
			gx_msleep(100);
		}
	}

	return 0;
}
#endif

static void _set_audiodec_finish(struct gxav_audiodec *audiodec)
{
	audiodec->dec_state = AUDIODEC_OVER;
	audiodec->dec_errno = AUDIODEC_ERR_NONE;
	gxav_stc_invaild_apts(0);
}

static int _find_audiodec_pts(struct gxav_audiodec *audiodec,
		struct gxav_apts_comparectrl *apts_compare_ctrl, int debug_type)
{
	int pts = -1;
	int find_pts_count = 0;
	int skip_pts_count = 0;
	unsigned int pts_num;
	unsigned int pts_sdram_addr = 0;
	unsigned int right_pts_sdram_addr = 0;
	volatile unsigned int *pts_read_addr;
	volatile unsigned int *pts_end_addr;

	pts_read_addr = (volatile unsigned int *)(apts_compare_ctrl->pts_buf_target_addr);
	pts_end_addr  = (volatile unsigned int *)(apts_compare_ctrl->ptsbuf_end_addr);

	if(apts_compare_ctrl->esaStartAddr > apts_compare_ctrl->esaEndAddr){
		apts_compare_ctrl->esa_buf_count++;
		apts_compare_ctrl->esa_buf_count &= 0x0f;
	}

	while(1)
	{
		gxav_sdc_length_get(apts_compare_ctrl->pts_buf_id, &pts_num);
		if (pts_num < 8) {
			break;
		}

		if ((((int)pts_read_addr) & 0x7fffffff) >= (((int)pts_end_addr) & 0x7fffffff)) {
			pts_read_addr = (volatile unsigned int *)(apts_compare_ctrl->ptsbuf_start_addr);
			pts_end_addr  = (volatile unsigned int *)(apts_compare_ctrl->ptsbuf_end_addr);
		}

		pts_sdram_addr = ((unsigned int)(*pts_read_addr))&0x7fffffff;
		if (pts_sdram_addr < apts_compare_ctrl->esabuf_start_addr ||
				pts_sdram_addr > apts_compare_ctrl->esabuf_end_addr) {
			pts_read_addr++;
			pts_read_addr++;
			gxav_sdc_length_set(apts_compare_ctrl->pts_buf_id, 8, GXAV_SDC_READ);
			continue;
		}

		if(pts_sdram_addr < apts_compare_ctrl->pts_sdram_addr_save)
		{
			apts_compare_ctrl->pts_esa_buf_count++;
			apts_compare_ctrl->pts_esa_buf_count &= 0x0f;
		}
		apts_compare_ctrl->pts_sdram_addr_save = pts_sdram_addr;


		if(apts_compare_ctrl->esaStartAddr == apts_compare_ctrl->esaEndAddr)
		{
			break;
		}
		else if(apts_compare_ctrl->esaStartAddr < apts_compare_ctrl->esaEndAddr)
		{
			if((pts_sdram_addr >= apts_compare_ctrl->esaStartAddr) &&
					(pts_sdram_addr < apts_compare_ctrl->esaEndAddr) &&
					(apts_compare_ctrl->esa_buf_count == apts_compare_ctrl->pts_esa_buf_count))
			{
				pts_read_addr++;
				pts = (int)(*pts_read_addr);
				pts_read_addr++;
				gxav_sdc_length_set(apts_compare_ctrl->pts_buf_id, 8, GXAV_SDC_READ);
				right_pts_sdram_addr = pts_sdram_addr;
				find_pts_count++;
			}
			else if(((pts_sdram_addr < apts_compare_ctrl->esaStartAddr) &&
						(apts_compare_ctrl->pts_esa_buf_count == apts_compare_ctrl->esa_buf_count)))
			{
				pts_read_addr++;
				pts_read_addr++;
				gxav_sdc_length_set(apts_compare_ctrl->pts_buf_id, 8, GXAV_SDC_READ);
				skip_pts_count++;
			}
			else
			{//right
				break;
			}
		}
		else if(apts_compare_ctrl->esaStartAddr > apts_compare_ctrl->esaEndAddr)
		{
			if(((pts_sdram_addr >= apts_compare_ctrl->esaStartAddr) &&
						(((apts_compare_ctrl->pts_esa_buf_count + 1) & 0x0f) == apts_compare_ctrl->esa_buf_count)) ||
					((pts_sdram_addr < apts_compare_ctrl->esaEndAddr) &&
					 (apts_compare_ctrl->pts_esa_buf_count == apts_compare_ctrl->esa_buf_count)))
			{
				pts_read_addr++;
				pts = (int)(*pts_read_addr);
				pts_read_addr++;
				gxav_sdc_length_set(apts_compare_ctrl->pts_buf_id, 8, GXAV_SDC_READ);
				right_pts_sdram_addr = pts_sdram_addr;
				find_pts_count++;
			}
			else if((pts_sdram_addr >= apts_compare_ctrl->esaEndAddr) &&
					(pts_sdram_addr <  apts_compare_ctrl->esaStartAddr) &&
					(((apts_compare_ctrl->pts_esa_buf_count + 1) & 0x0f) == apts_compare_ctrl->esa_buf_count))
			{
				pts_read_addr++;
				pts_read_addr++;
				gxav_sdc_length_set(apts_compare_ctrl->pts_buf_id, 8, GXAV_SDC_READ);
				skip_pts_count++;
			}
			else
			{
				break;
			}
		}
	}

	if (apts_compare_ctrl->esaStartAddr > apts_compare_ctrl->esaEndAddr) {
		apts_compare_ctrl->pts_esa_buf_count   = apts_compare_ctrl->esa_buf_count;
		apts_compare_ctrl->pts_sdram_addr_save = 0;
	}

	FIND_PTS_LOG("src %d err %d - %x, 0x%08x, 0x%08x, 0x%08x, (%2d %2d), (%d %d) \n",
			audiodec->dec_id,
			apts_compare_ctrl->error, debug_type,
			find_pts_count ? right_pts_sdram_addr : pts_sdram_addr,
			apts_compare_ctrl->esaStartAddr,
			apts_compare_ctrl->esaEndAddr,
			apts_compare_ctrl->esa_buf_count,
			apts_compare_ctrl->pts_esa_buf_count,
			find_pts_count, skip_pts_count);

	apts_compare_ctrl->pts_buf_target_addr  = (unsigned int)pts_read_addr;
	apts_compare_ctrl->pts_buf_end_addr     = (unsigned int)pts_end_addr;
	apts_compare_ctrl->esaStartAddr         = apts_compare_ctrl->esaEndAddr;

	return (pts & 0x1) ? pts : -1;
}

static int _fix_audiodec_pts(struct gxav_audiodec *audiodec,
		struct gxav_apts_comparectrl *apts_compare_ctrl, int pts, int debug_type)
{
	int  no_fix_pts = -1;
	char fix_flag   = 0;

	if (pts == 0) {
		pts = -1;
	}

	no_fix_pts = pts;
	//fix pts by frame unit time
	if (1) {
		int stc_time;
		GxSTCProperty_TimeResolution resolution;
		gxav_stc_get_base_resolution(0, &resolution);
		stc_time = (resolution.freq_HZ/1000);
		if(pts == -1){
			if((apts_compare_ctrl->last_pts != -1) && (apts_compare_ctrl->unit_time != -1)){
				pts = apts_compare_ctrl->last_pts + apts_compare_ctrl->unit_time;
				fix_flag |= (1<<0);
			}
		}

		if((pts != -1) && (apts_compare_ctrl->last_pts != -1) &&
				(apts_compare_ctrl->unit_time > 0)){
			int dis_pts = pts - apts_compare_ctrl->last_pts;
			int tmp_pts1 = apts_compare_ctrl->unit_time_dis/1000;
			int tmp_pts2 = apts_compare_ctrl->unit_time_dis%1000;
			if(abs(dis_pts) <= (apts_compare_ctrl->unit_time+tmp_pts1)){
				pts += (apts_compare_ctrl->unit_time + tmp_pts1 - dis_pts);
				apts_compare_ctrl->unit_time_dis = tmp_pts2;
				fix_flag |= (1<<1);
			}else if(abs(dis_pts) <= 2*(apts_compare_ctrl->unit_time + tmp_pts2)){
				pts -= (dis_pts - (apts_compare_ctrl->unit_time + tmp_pts1));
				apts_compare_ctrl->unit_time_dis = tmp_pts2;
				fix_flag |= (1<<2);
			}else{
				apts_compare_ctrl->unit_time_dis = 0;
				fix_flag |= (1<<3);
			}
		}else
			apts_compare_ctrl->unit_time_dis = 0;

		if(apts_compare_ctrl->sample_fre > 0){
			unsigned int unit_len  = 1000*apts_compare_ctrl->pcm_len;
			unsigned int unit_time = unit_len/apts_compare_ctrl->sample_fre;

			apts_compare_ctrl->unit_time = unit_time*stc_time;
			unit_len -= (unit_time*apts_compare_ctrl->sample_fre);
			unit_time = (1000*unit_len)/apts_compare_ctrl->sample_fre;
			apts_compare_ctrl->unit_time_dis += unit_time*stc_time;
		}else
			apts_compare_ctrl->unit_time = -1;
	}

	FIX_PTS_LOG("src %d err %d - %x, %d, %4u, %4u, %10u, %10u, %d\n",
			audiodec->dec_id,
			apts_compare_ctrl->error, debug_type,
			fix_flag, apts_compare_ctrl->pcm_len, apts_compare_ctrl->unit_time_dis,
			(pts==-1)?0:pts, (no_fix_pts==-1)?0:no_fix_pts,
			((pts!=-1)&&(apts_compare_ctrl->last_pts!=-1))?(pts-apts_compare_ctrl->last_pts):-1);

	apts_compare_ctrl->last_pts = pts;

	return pts;
}

static int _find_audiodec_ctrlinfo(struct gxav_audiodec* audiodec, struct gxav_apts_comparectrl *apts_compare_ctrl)
{
	unsigned int ctrl_fade_byte = 0;
	unsigned int ctrl_pan_byte  = 0;
	unsigned int ctrl_esa_buf_addr  = 0;
	unsigned int start_addr = 0;
	unsigned int end_addr = 0;

	if (audiodec->dec_id == 0)
		return 0;

	if(apts_compare_ctrl->esaStartAddr > apts_compare_ctrl->esaEndAddr){
		apts_compare_ctrl->total_esa_buf_count++;
		start_addr = (apts_compare_ctrl->total_esa_buf_count - 1)*apts_compare_ctrl->esa_buf_size
			+ apts_compare_ctrl->esaStartAddr - apts_compare_ctrl->esabuf_start_addr;
		end_addr   = (apts_compare_ctrl->total_esa_buf_count)*apts_compare_ctrl->esa_buf_size
			+ apts_compare_ctrl->esaEndAddr - apts_compare_ctrl->esabuf_start_addr;
	}else{
		start_addr = (apts_compare_ctrl->total_esa_buf_count)*apts_compare_ctrl->esa_buf_size
			+ apts_compare_ctrl->esaStartAddr - apts_compare_ctrl->esabuf_start_addr;
		end_addr   = (apts_compare_ctrl->total_esa_buf_count)*apts_compare_ctrl->esa_buf_size
			+ apts_compare_ctrl->esaEndAddr - apts_compare_ctrl->esabuf_start_addr;
	}

	if(audiodec->ctrlinfo_fifo == NULL)
		return -1;

	while(1) {
		if (apts_compare_ctrl->ad_ctrl_invaild) {
			ctrl_esa_buf_addr  = apts_compare_ctrl->ad_ctrl_esa_buf_addr;
			ctrl_fade_byte     = apts_compare_ctrl->ad_ctrl_fade_byte;
			ctrl_pan_byte      = apts_compare_ctrl->ad_ctrl_pan_byte;
		} else if(gxfifo_len(audiodec->ctrlinfo_fifo) >= sizeof(GxAudioDecProperty_ContrlInfo)) {
			GxAudioDecProperty_ContrlInfo ctrlinfo;
			struct audiodec_params_info *decctrl_params = NULL;
			int value = 0;

			gxfifo_get(audiodec->ctrlinfo_fifo, &ctrlinfo, sizeof(GxAudioDecProperty_ContrlInfo));
			ctrl_esa_buf_addr = (unsigned int)ctrlinfo.esa_size;
			ctrl_fade_byte    = ctrlinfo.ad_fade_byte;
			ctrl_pan_byte     = ctrlinfo.ad_pan_byte;
			if (ctrlinfo.ad_text_tag == 0x12345678) {
				decctrl_params = (struct audiodec_params_info *)(audiodec->dec_params);
				if (decctrl_params) {
					if (audiodec->decode_ctrlinfo_enable) {
						value |= (ctrl_pan_byte&0xff);
						value |= ((ctrl_fade_byte&0xff)<<8);
						GX_AUDIO_SET_VAL_E(&(decctrl_params->ad_ctrl_vol), value);
						gxlog_d(LOG_ADEC, "%s %d [pan: %x, fade:%x]\n",
								__func__, __LINE__, ctrl_pan_byte, ctrl_fade_byte);
					} else
						GX_AUDIO_SET_VAL_E(&(decctrl_params->ad_ctrl_vol), 0);
				}
			}
		} else
			return -1;

		if(start_addr < ctrl_esa_buf_addr && (end_addr+1) >= ctrl_esa_buf_addr){
			apts_compare_ctrl->ad_ctrl_invaild = 0;
			break;
		}else if(ctrl_esa_buf_addr <= start_addr){
			apts_compare_ctrl->ad_ctrl_invaild = 0;
			continue;
		}else if(ctrl_esa_buf_addr > (end_addr+1)){
			apts_compare_ctrl->ad_ctrl_invaild = 1;
			apts_compare_ctrl->ad_ctrl_esa_buf_addr = ctrl_esa_buf_addr;
			apts_compare_ctrl->ad_ctrl_fade_byte    = ctrl_fade_byte;
			apts_compare_ctrl->ad_ctrl_pan_byte     = ctrl_pan_byte;
			break;
		}
	}
	return 0;
}

static int _consume_data_len(unsigned int cur_r_addr,
		unsigned int cur_w_addr, unsigned int last_r_addr, unsigned int buffer_size)
{
	if((cur_r_addr == cur_w_addr) && (((cur_r_addr + 1)%buffer_size) == last_r_addr))
		return buffer_size;
	else
		return (cur_r_addr + 1 - last_r_addr + buffer_size)%buffer_size;
}

static int _fill_decode_coding_data(struct gxav_audiodec *audiodec)
{
	unsigned int esa_len, or_esa_len;
	unsigned int or_esa_w_addr;
	unsigned int esa_size = audiodec->decode_esa_size;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	struct audiodec_regs *addec_reg    = audiodec->addec_reg;

	if (audiodec->dec_id == 0)
		or_esa_w_addr = OR_GET_NORMAL_SAVED_ESAW_ADDR(audiodec_reg);
	else
		or_esa_w_addr = GX_AUDIO_GET_ENDIAN_32(OR_GET_NORMAL_SAVED_ESAW_ADDR(addec_reg));

	or_esa_len = (or_esa_w_addr-audiodec->decode_esa_read_addr+esa_size+1)%esa_size;
	gxav_sdc_length_get(audiodec->decode_esa_id, &esa_len);

	if (gxav_firewall_access_align()) {
		if (esa_len > ALIGN_LENGTH) {
			unsigned int mask_len = esa_len;
			unsigned int write_addr = ((((audiodec->decode_esa_read_addr + mask_len) % esa_size) & ALIGN_MASK) + esa_size - 1)%esa_size;
			mask_len = (write_addr + 1 + esa_size - audiodec->decode_esa_read_addr) % esa_size;

			esa_len = mask_len;
		} else
			esa_len = 0;
	}

	if (esa_len < (or_esa_len + audiodec->decode_need_data_num))
		audiodec->decode_esa_writed_addr = audiodec->decode_esa_read_addr;
	else
		audiodec->decode_esa_writed_addr = (audiodec->decode_esa_read_addr + esa_len - 1)%esa_size;

	if (audiodec->decode_esa_writed_addr != audiodec->decode_esa_read_addr) {
		audiodec->decode_esa_empty = 0;
		DECODE_TASK_LOG("[decode (%d)] start task[%x %x %d]\n",
				audiodec->dec_id, audiodec->decode_esa_writed_addr, esa_len, audiodec->decode_need_data_num);
		if (audiodec->dec_id == 0) {
			if(audiodec->dec_type == CODEC_RA_AAC || audiodec->dec_type == CODEC_RA_RA8LBR) {
				gxav_sdc_rwaddr_get(audiodec->decode_pts_id, NULL, &audiodec->decode_pts_writed_addr);
				audiodec->decode_pts_writed_addr = (audiodec->decode_pts_writed_addr+audiodec->decode_pts_size-1)%audiodec->decode_pts_size;
				OR_SET_COM_SOURCE_W_SADDR(audiodec_reg, audiodec->decode_pts_writed_addr);
			}
			OR_SET_NORMAL_ESA_W_ADDR(audiodec_reg, audiodec->decode_esa_writed_addr);
		} else if (audiodec->dec_id == 1)
			OR_SET_NORMAL_ESA_W_ADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32(audiodec->decode_esa_writed_addr));
		audiodec->decode_need_data_num = (esa_len - or_esa_len);
	} else {
		DECODE_TASK_LOG("[decode (%d)] not enough data\n", audiodec->dec_id);
		audiodec->decode_esa_empty = 1;
		audiodec->decode_data_operate = PCM_FILL_DATA;
	}

	return 0;
}

static int decode_key_complete_isr(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	OR_INT_DECODE_KEY_DISABLE(audiodec_reg);
	OR_CLR_DECODE_KEY_INT_STATUS(audiodec_reg);

	return EVENT_AUDIO_KEY;
}

static int _start_decode_coding_data(struct gxav_audiodec *audiodec)
{
	unsigned int compress_len, less_len = 0;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	struct audiodec_regs *addec_reg    = audiodec->addec_reg;

	if (audiodec->update && (audiodec->dec_state == AUDIODEC_OVER))
		return -1;

	gxav_sdc_length_get(audiodec->decode_esa_id, &compress_len);

	if (gxav_firewall_access_align())
		less_len = ALIGN_LENGTH;

	if (((compress_len > less_len) && (!audiodec->decode_first_flag)) ||
			((compress_len > audiodec->enough_data) && (audiodec->decode_first_flag))) {
		unsigned int pcm_space = 0;
		unsigned char start_to_decode = 1;
		unsigned int pcm_node_free_len = 0;
		unsigned int pcm_min_free_len  = 2*AUDIO_MAX_FRAME_SAMPLE_LEN;

		audiodec->decode_esa_empty  = 0;
		audiodec->decode_first_flag = 0;
		gxav_sdc_free_get(audiodec->out_pcm_id, &pcm_space);
		pcm_space = pcm_space - (audiodec->max_channel_num - 1)*audiodec->out_pcm_size;

		pcm_node_free_len = gxfifo_freelen(audiodec->pcm_fifo);
		pcm_min_free_len  = (audiodec->dec_type == CODEC_FLAC)?(2*pcm_min_free_len):pcm_min_free_len;
		start_to_decode  &= (pcm_space >= pcm_min_free_len)?1:0;
		start_to_decode  &= (pcm_node_free_len >= 2*sizeof(GxAudioFrameNode))?1:0;

		if ((audiodec->dec_type == CODEC_EAC3) &&
				((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE)){
			unsigned int enc_node_free_len = gxfifo_freelen(audiodec->degrade_fifo);
			unsigned int enc_space = 0;

			gxav_sdc_free_get(audiodec->out_degrade_id, &enc_space);
			start_to_decode &= (enc_space >= 4096)?1:0;
			start_to_decode &= (enc_node_free_len >= 2*sizeof(GxAudioFrameNode))?1:0;
		}

		if ((audiodec->dec_type == CODEC_MPEG4_AAC) &&
				((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE)) {
			unsigned int enc_node_free_len = gxfifo_freelen(audiodec->bypass_fifo);
			unsigned int enc_space = 0;

			gxav_sdc_free_get(audiodec->out_bypass_id, &enc_space);
			start_to_decode &= (enc_space >= 4096)?1:0;
			start_to_decode &= (enc_node_free_len >= 2*sizeof(GxAudioFrameNode))?1:0;
		}

		if ((audiodec->dec_type == CODEC_MPEG4_AAC) &&
				((audiodec->dec_mode & CONVERT_MODE) == CONVERT_MODE)) {
			unsigned int convert_esa_free_len  = 0;
			unsigned int convert_esa_len       = 0;
			unsigned int convert_pts_free_len  = 0;

			gxav_sdc_length_get(audiodec->convert_esa_id, &convert_esa_len);
			gxav_sdc_free_get(audiodec->convert_pts_id, &convert_pts_free_len);
			convert_esa_free_len = audiodec->convert_esa_size - convert_esa_len;
			start_to_decode &= (convert_esa_free_len >= (2*AUDIO_MAX_FRAME_SAMPLE_LEN))?1:0;
			start_to_decode &= (convert_pts_free_len >= 16)?1:0;
		}

		if (start_to_decode) {
			unsigned int esa_writed_addr = 0;
			unsigned int decode_esa_size = audiodec->decode_esa_size;

			_check_audiodec_active_params(audiodec);
			audiodec->decode_pcm_full = 0;
			gxav_sdc_rwaddr_get(audiodec->decode_esa_id, NULL, &esa_writed_addr);
			if (gxav_firewall_access_align())
				esa_writed_addr &= ALIGN_MASK;
			audiodec->decode_esa_writed_addr = (decode_esa_size+esa_writed_addr-1)%decode_esa_size;


			while(OR_GET_MCU_TASK_START_TO_WORK(audiodec_reg));
			//解码结束以后读指针需要向前移动一比特，因此需要更新读指针
			if (audiodec->dec_id == 0) {
				OR_SET_NORMAL_FRAME_S_ADDR(audiodec_reg, audiodec->decode_esa_read_addr);
				OR_SET_NORMAL_PCM_W_SADDR(audiodec_reg, (audiodec->out_pcm_writed_addr)&0xfffffffc);
				OR_SET_NORMAL_DEC_RESTART_OR_CONTINUE(audiodec_reg, audiodec->decode_restart_continue);
				OR_SET_NORMAL_FRAME_NUM(audiodec_reg, audiodec->decode_frame_num);
				OR_SET_NORMAL_ESA_W_ADDR(audiodec_reg, audiodec->decode_esa_writed_addr);
				audiodec->decode_frame_param += audiodec->decode_frame_num;

				if ((audiodec->dec_type == CODEC_EAC3) &&
						((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE)) {
					struct audiodec_params_info *decctrl_params = NULL;

					decctrl_params = (struct audiodec_params_info *)(audiodec->dec_params);
					if (decctrl_params)
						GX_AUDIO_SET_VAL_E(&(decctrl_params->dec_mov_ac3_data), 1);
					OR_SET_COM_TARGET_W_SADDR(audiodec_reg, audiodec->out_degrade_writed_addr);
				}

				if ((audiodec->dec_type == CODEC_RA_AAC) ||
						(audiodec->dec_type == CODEC_RA_RA8LBR)) {
					gxav_sdc_rwaddr_get(audiodec->decode_pts_id, NULL, &audiodec->decode_pts_writed_addr);
					audiodec->decode_pts_writed_addr = (audiodec->decode_pts_size+audiodec->decode_pts_writed_addr-1)%audiodec->decode_pts_size;
					OR_SET_COM_SOURCE_S_ADDR(audiodec_reg,audiodec->decode_pts_read_addr);
					OR_SET_COM_SOURCE_W_SADDR(audiodec_reg,audiodec->decode_pts_writed_addr);
				}

				if (audiodec->dec_type == CODEC_PCM) {
					struct audiodec_params_info *decctrl_params = NULL;

					decctrl_params = (struct audiodec_params_info *)(audiodec->dec_params);
					if (decctrl_params) {
						GX_AUDIO_SET_VAL_E(&(decctrl_params->pcm_sample_rate), audiodec->pcm_info.samplefre);
						GX_AUDIO_SET_VAL_E(&(decctrl_params->pcm_channel_num), audiodec->pcm_info.channelnum);
						GX_AUDIO_SET_VAL_E(&(decctrl_params->pcm_bit_width),   audiodec->pcm_info.bitwidth);
						GX_AUDIO_SET_VAL_E(&(decctrl_params->pcm_interlace),   audiodec->pcm_info.intelace);
						GX_AUDIO_SET_VAL_E(&(decctrl_params->pcm_endian),      audiodec->pcm_info.endian);
						GX_AUDIO_SET_VAL_E(&(decctrl_params->pcm_float_en),    audiodec->pcm_info.float_en);
					}
				}

				gx_dcache_clean_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);
				DECODE_TASK_LOG("[decode (%d)] start decode (%x %x)\n",
						audiodec->dec_id, audiodec->decode_esa_read_addr, audiodec->decode_esa_writed_addr);
				OR_INT_DECODE_FINISH_ENABLE(audiodec_reg);
				OR_SET_DECODE_TASK(audiodec_reg, DECODE_FRAME);
				OR_SET_MCU_TASK_START_TO_WORK(audiodec_reg);
			} else if (audiodec->dec_id == 1) {
				OR_SET_NORMAL_FRAME_S_ADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32(audiodec->decode_esa_read_addr));
				OR_SET_NORMAL_PCM_W_SADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32((audiodec->out_pcm_writed_addr)&0xfffffffc));
				OR_SET_ADUIO2_DEC_RESTART_OR_CONTINUE(audiodec_reg, audiodec->decode_restart_continue);
				OR_SET_NORMAL_ESA_W_ADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32(audiodec->decode_esa_writed_addr));

				gx_dcache_clean_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);
				DECODE_TASK_LOG("[decode (%d)] start decode (%x %x)\n",
						audiodec->dec_id, audiodec->decode_esa_read_addr, audiodec->decode_esa_writed_addr);
				OR_INT_AUDIO2_DECODE_FINISH_ENABLE(audiodec_reg);
				OR_SET_DECODE_TASK(audiodec_reg, DECODE_AD_FRAME);
				OR_SET_MCU_TASK_START_TO_WORK(audiodec_reg);
			}
			audiodec->decode_need_data_num = 0;
			return 0;
		}else{
			DECODE_TASK_LOG("[decode (%d)] pcm full\n", audiodec->dec_id);
			audiodec->decode_pcm_full = 1;
			return -1;
		}
	}else{
		DECODE_TASK_LOG("[decode (%d)] not enough esa (update %d)\n", audiodec->dec_id, audiodec->update);
		if (audiodec->update && (audiodec->dec_mode == DECODE_MODE)) {
			_set_audiodec_finish(audiodec);
		} else {
			audiodec->decode_esa_empty = 1;
			audiodec->decode_data_operate = PCM_DEC_FRAME;
		}
		return -1;
	}
}

static int decode_writed_callback(unsigned int id, unsigned int lenx, unsigned int overflow, void *arg)
{
	int ret = -1;
	unsigned int len = 0;
	struct gxav_audiodec *audiodec = (struct gxav_audiodec*)arg;

	if (audiodec == NULL)
		return -1;

	gxav_sdc_length_get(id, &len);
	DECODE_ESA_LOG("[decode (%d)] overflow %d, esa %d\n", audiodec->dec_id, overflow, (len - lenx));

#ifdef DEBUG_ADEC_RECORD
	if ((audiodec->dec_id == record_dec_id) &&
			(record_over_flags == 0)) {
		if ((record_size + lenx) <= record_buffer_size) {
			unsigned int w_ptr = 0;
			unsigned int virt_s_addr = (unsigned int)gx_phys_to_virt(audiodec->decode_esa_start_addr);
			unsigned int virt_e_addr = virt_s_addr + audiodec->decode_esa_size;

			gx_dcache_inv_range(virt_s_addr, virt_e_addr);
			gxav_sdc_rwaddr_get(audiodec->decode_esa_id, NULL, &w_ptr);
			w_ptr = (w_ptr + audiodec->decode_esa_size - lenx) % audiodec->decode_esa_size;

			if ((w_ptr + lenx) > audiodec->decode_esa_size) {
				unsigned int tmp_addr = w_ptr;
				unsigned int tmp_size = (audiodec->decode_esa_size - w_ptr);

				gx_memcpy((unsigned char*)record_buffer + record_size,
						(unsigned char *)virt_s_addr + tmp_addr, tmp_size);
				record_size += tmp_size;
				tmp_addr = 0;
				tmp_size = lenx - tmp_size;
				gx_memcpy((unsigned char*)record_buffer + record_size,
						(unsigned char *)virt_s_addr + tmp_addr, tmp_size);
				record_size += tmp_size;
			} else {
				gx_memcpy((unsigned char*)record_buffer + record_size,
						(unsigned char *)virt_s_addr + w_ptr, lenx);
				record_size += lenx;
			}
			gxlog_i(LOG_ADEC, "addr: %x, size: %d\n", audiodec->decode_esa_start_addr, record_size);
		} else {
			gxlog_i(LOG_ADEC, "=======> ESA: addr 0x%x, size %d\n",
					gx_virt_to_phys((unsigned int)record_buffer), record_size);
			record_over_flags = 1;
		}
	}
#endif

	if(overflow){
		audiodec->dec_state = AUDIODEC_ERROR;
		audiodec->dec_errno = AUDIODEC_ERR_ESA_OVERFLOW;
		gxlog_d(LOG_ADEC, "%s %d overflow %d\n", __FUNCTION__, __LINE__, overflow);
		return 0;
	}

	if(audiodec->dec_state != AUDIODEC_READY &&
			audiodec->dec_state != AUDIODEC_RUNNING){
		return 0;
	}

	if(audiodec->decode_esa_empty)
	{
		switch(audiodec->decode_data_operate)
		{
		case PCM_DEC_FRAME:
			ret = _start_decode_coding_data(audiodec);
			break;
		case PCM_FILL_DATA:
			ret = _fill_decode_coding_data(audiodec);
			break;
		default:
			break;
		}
	}

	return ret;
}

static int decode_readed_callback(unsigned int id, unsigned int len, unsigned int underflow, void *arg)
{
	int ret=-1;
	struct gxav_audiodec *audiodec = (struct gxav_audiodec*)arg;

	if(audiodec->dec_state != AUDIODEC_READY &&
			audiodec->dec_state != AUDIODEC_RUNNING){
		return 0;
	}

	if(audiodec->decode_pcm_full){
		ret = _start_decode_coding_data(audiodec);
	}

	return ret;
}

static int decode_need_data_isr(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	struct audiodec_regs *addec_reg    = audiodec->addec_reg;

	DECODE_TASK_LOG("[decode (%d)] need data isr\n", audiodec->dec_id);

	if (audiodec->dec_id == 0) {
		OR_CLR_AUDIO1_NEED_DATA_INT_STATUS(audiodec_reg);
		audiodec->decode_need_data_num += OR_GET_NORMAL_NEED_DATA_NUM(audiodec_reg);
	} else if (audiodec->dec_id == 1){
		OR_CLR_AUDIO2_NEED_DATA_INT_STATUS(audiodec_reg);
		audiodec->decode_need_data_num += GET_ENDIAN_16(OR_GET_AUDIO2_NEED_DATA_NUM(addec_reg));
	}
	_fill_decode_coding_data(audiodec);
	return 0;
}

static int decode_finish_dec_isr(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	DECODE_TASK_LOG("[decode (%d)] finish isr\n", audiodec->dec_id);

	_set_audiodec_finish(audiodec);
	if (audiodec->dec_id == 0)
		OR_CLR_AUDIO1_DATA_EMPTY_FINISH_INT_STATUS(audiodec_reg);
	else if (audiodec->dec_id == 1)
		OR_CLR_AUDIO2_DATA_EMPTY_FINISH_INT_STATUS(audiodec_reg);

	return 0;
}

static int decode_find_header_isr(struct gxav_audiodec *audiodec)
{
	int ret = 0, len;
	unsigned int last_esa_read_addr = audiodec->decode_esa_read_addr;
	unsigned int cur_esa_writed_addr = audiodec->decode_esa_writed_addr;
	unsigned int decode_esa_size  = audiodec->decode_esa_size;
	struct gxav_apts_comparectrl *apts_compare_ctrl = &audiodec->decode_apts_ctrl;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	struct audiodec_regs *addec_reg    = audiodec->addec_reg;

	DECODE_TASK_LOG("[decode (%d)] find header isr\n", audiodec->dec_id);

	if (audiodec->dec_id == 0) {
		OR_CLR_AUDIO1_SYNC_FINISH_INT_STATUS(audiodec_reg);
		audiodec->decode_esa_read_addr = OR_GET_NORMAL_ESA_R_ADDR(audiodec_reg);
		audiodec->decode_framelength = OR_GET_DEC_FRAMELENGTH(audiodec_reg);
		audiodec->decode_samplefre   = OR_GET_SAMPLE_RATE(audiodec_reg);
		audiodec->decode_channelnum  = OR_GET_AUDIO_CHANNEL(audiodec_reg);
		audiodec->decode_bitrate     = OR_GET_BIT_RATE(audiodec_reg);
	} else if (audiodec->dec_id == 1) {
		OR_CLR_AUDIO2_SYNC_FINISH_INT_STATUS(audiodec_reg);
		audiodec->decode_esa_read_addr = GX_AUDIO_GET_ENDIAN_32(OR_GET_NORMAL_ESA_R_ADDR(addec_reg));
		audiodec->decode_framelength = GET_ENDIAN_16(OR_GET_AUDIO2_DEC_FRAMELENGTH(addec_reg));
		audiodec->decode_samplefre   = GX_AUDIO_GET_ENDIAN_32(OR_GET_SAMPLE_RATE(addec_reg));
		audiodec->decode_channelnum  = OR_GET_AUDIO2_CHANNEL(addec_reg);
		audiodec->decode_bitrate     = GX_AUDIO_GET_ENDIAN_32(OR_GET_BIT_RATE(addec_reg));
	}

	len = _consume_data_len(audiodec->decode_esa_read_addr, cur_esa_writed_addr, last_esa_read_addr, decode_esa_size);
	gxav_sdc_length_set(audiodec->decode_esa_id, len, GXAV_SDC_READ);

	apts_compare_ctrl->esaEndAddr = audiodec->decode_esa_read_addr+audiodec->decode_esa_start_addr;//相对地址->绝对地址
	apts_compare_ctrl->frame_size = audiodec->decode_framelength;
	apts_compare_ctrl->bitrate    = audiodec->decode_bitrate;
	DECODE_ESA_READ_ADDR_UPDATA_NEXT_BYTE(audiodec);

	_start_decode_coding_data(audiodec);

	return ret;
}

static int decode_dec_complete_isr(struct gxav_audiodec *audiodec)
{
	int ret = 0, unfix_pts = 0, len = 0;
	unsigned int last_esa_read_addr = audiodec->decode_esa_read_addr;
	unsigned int last_pts_read_addr = audiodec->decode_pts_read_addr;
	unsigned int last_pcm_write_addr = audiodec->out_pcm_writed_addr;
	unsigned int last_degrade_write_addr = audiodec->out_degrade_writed_addr;
	unsigned int last_samplefre = audiodec->decode_samplefre;
	unsigned int last_channelnum = audiodec->decode_channelnum;
	unsigned int cur_esa_writed_addr = audiodec->decode_esa_writed_addr;
	unsigned int cur_pts_writed_addr = audiodec->decode_pts_writed_addr;
	unsigned int decode_esa_size = audiodec->decode_esa_size;
	unsigned int decode_pts_size = audiodec->decode_pts_size;
	unsigned int compress_frame_size = 0;
	unsigned int pts_save = (audiodec->stream_type == STREAM_TS) ? 1 : 0;
	struct gxav_apts_comparectrl *apts_compare_ctrl = &audiodec->decode_apts_ctrl;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	struct audiodec_regs *addec_reg    = audiodec->addec_reg;
	GxAudioFrameNode node_pcm;
	GxAudioFrameNode node_degrade;

	gx_dcache_inv_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);

	if (audiodec->dec_id == 0) {
		OR_INT_DECODE_FINISH_DISABLE(audiodec_reg);
		OR_CLR_AUDIO1_DECODE_FINISH_INT_STATUS(audiodec_reg);
		audiodec->decode_err  = OR_GET_NORMAL_ERR_INDEX(audiodec_reg);

		if(audiodec->dec_type == CODEC_RA_RA8LBR || audiodec->dec_type == CODEC_RA_AAC) {
			pts_save = 1;
			unfix_pts = OR_GET_NORMAL_RA_APTSADDR(audiodec_reg);
			unfix_pts = (unfix_pts!=-1) ? (unfix_pts>>3) : -1;
			if(OR_GET_MCU_RA_ESA_CNT_UPDATA(audiodec_reg)) {
				audiodec->decode_esa_read_addr = OR_GET_NORMAL_ESA_R_ADDR(audiodec_reg);
				audiodec->decode_pts_read_addr = OR_GET_NORMAL_COM_SOURCE_R_ADDR(audiodec_reg);

				len = _consume_data_len(audiodec->decode_esa_read_addr, cur_esa_writed_addr, last_esa_read_addr, decode_esa_size);
				gxav_sdc_length_set(audiodec->decode_esa_id, len, GXAV_SDC_READ);
				compress_frame_size = len;

				len = _consume_data_len(audiodec->decode_pts_read_addr, cur_pts_writed_addr, last_pts_read_addr, decode_pts_size);
				gxav_sdc_length_set(audiodec->decode_pts_id, len, GXAV_SDC_READ);
				DECODE_PTS_READ_ADDR_UPDATA_NEXT_BYTE(audiodec);
			}
			else
				DECODE_ESA_READ_ADDR_UPDATA_LAST_BYTE(audiodec);
		} else {
			audiodec->decode_esa_read_addr = OR_GET_NORMAL_ESA_R_ADDR(audiodec_reg);
			len = _consume_data_len(audiodec->decode_esa_read_addr, cur_esa_writed_addr, last_esa_read_addr, decode_esa_size);
			gxav_sdc_length_set(audiodec->decode_esa_id, len, GXAV_SDC_READ);
			compress_frame_size = len;
		}
	} else if (audiodec->dec_id == 1) {
		OR_INT_AUDIO2_DECODE_FINISH_DISABLE(audiodec_reg);
		OR_CLR_AUDIO2_DECODE_FINISH_INT_STATUS(audiodec_reg);
		audiodec->decode_esa_read_addr = GX_AUDIO_GET_ENDIAN_32(OR_GET_NORMAL_ESA_R_ADDR(addec_reg));
		audiodec->decode_err  = ((GET_ENDIAN_16(OR_GET_AUDIO2_ERR_INDEX(addec_reg)))&0x7FFF);
		len = _consume_data_len(audiodec->decode_esa_read_addr, cur_esa_writed_addr, last_esa_read_addr, decode_esa_size);
		gxav_sdc_length_set(audiodec->decode_esa_id, len, GXAV_SDC_READ);
		compress_frame_size = len;
	}

	DECODE_TASK_LOG("[decode (%d)] complete isr (%d %d)\n", audiodec->dec_id, audiodec->decode_err, len);

	if (!audiodec->decode_err) {
		unsigned int new_pcm_writed_addr = 0;
		unsigned int out_pcm_size = audiodec->out_pcm_size;

		if (audiodec->dec_id == 0) {
			new_pcm_writed_addr = OR_GET_NORMAL_PCM_W_ADDR(audiodec_reg);
		} else if (audiodec->dec_id == 1) {
			new_pcm_writed_addr = GX_AUDIO_GET_ENDIAN_32(addec_reg->audio_mcu_pcm_w_addr);
		}

		new_pcm_writed_addr = (new_pcm_writed_addr + 1) % out_pcm_size;
		if (new_pcm_writed_addr == last_pcm_write_addr) {
			gxlog_w(LOG_ADEC,
					"=====> DECODE (id %d) last_pcm_waddr: %x, new_pcm_waddr %x\n",
					audiodec->dec_id, last_pcm_write_addr, new_pcm_writed_addr);
			audiodec->decode_err = 1;
		}
	}

	if (!audiodec->decode_err) {
		unsigned int out_pcm_size = audiodec->out_pcm_size;

		gx_memset(&node_pcm, 0, sizeof(GxAudioFrameNode));
		audiodec->decode_restart_continue = DEC_CONTINUE;
		if (audiodec->dec_state != AUDIODEC_ERROR &&
				audiodec->dec_state != AUDIODEC_RUNNING &&
				!(audiodec->dec_state == AUDIODEC_OVER && audiodec->update)) {
			ret = EVENT_AUDIO_FRAMEINFO;
			audiodec->dec_state = AUDIODEC_RUNNING;
			audiodec->dec_errno = AUDIODEC_ERR_NONE;
		}
		audiodec->decode_frame_cnt++;

		if (audiodec->dec_id == 0) {
			audiodec->decode_framelength = OR_GET_DEC_FRAMELENGTH(audiodec_reg);
			audiodec->decode_samplefre   = OR_GET_SAMPLE_RATE(audiodec_reg);
			audiodec->decode_channelnum  = OR_GET_AUDIO_CHANNEL(audiodec_reg);
			audiodec->decode_bitrate     = OR_GET_BIT_RATE(audiodec_reg);
			audiodec->out_pcm_writed_addr = OR_GET_NORMAL_PCM_W_ADDR(audiodec_reg);
		} else if (audiodec->dec_id == 1) {
			audiodec->decode_framelength = GET_ENDIAN_16(OR_GET_AUDIO2_DEC_FRAMELENGTH(addec_reg));
			audiodec->decode_samplefre   = GX_AUDIO_GET_ENDIAN_32(OR_GET_SAMPLE_RATE(addec_reg));
			audiodec->decode_channelnum  = OR_GET_AUDIO2_CHANNEL(addec_reg);
			audiodec->decode_bitrate     = GX_AUDIO_GET_ENDIAN_32(OR_GET_BIT_RATE(addec_reg));
			audiodec->out_pcm_writed_addr = GX_AUDIO_GET_ENDIAN_32(addec_reg->audio_mcu_pcm_w_addr);
			if (audiodec->stream_type == STREAM_TS) {
				struct audiodec_params_info *decctrl_params = (struct audiodec_params_info *)(audiodec->dec_params);
				unsigned int ctrlinfo_value = GX_AUDIO_GET_ENDIAN_32(addec_reg->audio_mcu_com_source_r_addr);
				unsigned long long ctrlinfo_pts  = 0;
				GxSTCProperty_TimeResolution resolution;

				gxav_stc_get_base_resolution(0, &resolution);
				ctrlinfo_pts  = (GX_AUDIO_GET_ENDIAN_32(addec_reg->audio_mcu_high_apts));
				ctrlinfo_pts  = (ctrlinfo_pts << 32);
				ctrlinfo_pts |= (GX_AUDIO_GET_ENDIAN_32(addec_reg->audio_mcu_low_apts));
				if (audiodec->decode_ctrlinfo_enable) {
					GX_AUDIO_SET_VAL_E(&(decctrl_params->ad_ctrl_vol), ctrlinfo_value);
				}

				if (resolution.freq_HZ == 2000) {
					unfix_pts = do_div(ctrlinfo_pts, 90);
					unfix_pts = (int)(unfix_pts << 1);
				} else {
					unfix_pts = (int)(ctrlinfo_pts >> 1);
				}
			}
		}

		if ((audiodec->decode_bitrate == 0) && (audiodec->decode_framelength == 0)) {
			if(audiodec->decode_framelength == 0)
				audiodec->decode_framelength = compress_frame_size;
			audiodec->decode_bitrate = compress_frame_size*250;
		}

		if (audiodec->decode_samplefre == 0)
			audiodec->decode_samplefre = 48000;

		if((audiodec->dec_type == CODEC_EAC3) &&
				((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE)){
			unsigned int out_degrade_size = audiodec->out_degrade_size;
			unsigned int degrade_len = 0;

			gx_memset(&node_degrade, 0, sizeof(GxAudioFrameNode));
			audiodec->out_degrade_writed_addr = OR_GET_COM_TARGET_EADDR(audiodec_reg);
			degrade_len = (audiodec->out_degrade_writed_addr-last_degrade_write_addr+out_degrade_size+1)%out_degrade_size;
			node_degrade.frame_s_addr = last_degrade_write_addr;
			node_degrade.frame_e_addr = audiodec->out_degrade_writed_addr;
			node_degrade.frame_size   = degrade_len;
			node_degrade.frame_type   = AUDIO_TYPE_AC3;
			node_degrade.endian       = 1;
			DEGRADE_WRITE_ADDR_UPDATA_NEXT_BYTE(audiodec);
			gxav_sdc_length_set(audiodec->out_degrade_id, degrade_len, GXAV_SDC_WRITE);
		}

		len = (audiodec->out_pcm_writed_addr-last_pcm_write_addr+out_pcm_size+1)%out_pcm_size;
		gxav_sdc_length_set(audiodec->out_pcm_id, len, GXAV_SDC_WRITE);

		node_pcm.frame_s_addr = last_pcm_write_addr;
		node_pcm.frame_e_addr = audiodec->out_pcm_writed_addr;
		node_pcm.sample_fre = audiodec->decode_samplefre;
		node_pcm.channel_num = audiodec->decode_channelnum;
		node_pcm.audio_standard = OR_GET_AUDIO_STANDARD(audiodec_reg);
		node_pcm.dolby_type     = OR_GET_AUDIO_DOLBY_TYPE(audiodec_reg);
		node_pcm.dolby_version  = OR_GET_AUDIO_DOLBY_VERSION(audiodec_reg);
		if((last_samplefre != audiodec->decode_samplefre) ||
				(last_channelnum != audiodec->decode_channelnum))
			node_pcm.sample_fre_change = 1;

		//解码默认为非交织的，32位表示一个样点
		//frame_size = sample_rate*采样时间*采样位深/8*通道数 (bytes)
		node_pcm.frame_size = (node_pcm.frame_e_addr+out_pcm_size-node_pcm.frame_s_addr+1)%out_pcm_size;
		audiodec->pcm_frame_len = node_pcm.frame_size;
		node_pcm.frame_size = node_pcm.frame_size >> 2;
		node_pcm.endian    = 1;
		node_pcm.interlace = 0;

		switch(audiodec->dec_type)
		{
		case CODEC_AC3:
		case CODEC_EAC3:
			node_pcm.sample_data_len = 0x3;
			break;
		default:
			node_pcm.sample_data_len = 0xc;
			break;
		}
		PCM_WRITE_ADDR_UPDATA_NEXT_BYTE(audiodec);

		if ((audiodec->dec_type != CODEC_MPEG4_AAC) &&
				(audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE) {
			//保持解码与过滤一致性
			if (audiodec->decode_frame_num == 0) {
				audiodec->decode_frame_param = 0;
				audiodec->decode_frame_num   = 1;
			} else {
				if (node_pcm.frame_size / audiodec->decode_frame_num >= 0x600) {
					audiodec->decode_frame_param = 0;
					audiodec->decode_frame_num   = 1;
				} else if (node_pcm.frame_size / audiodec->decode_frame_num >= 0x300) {
					audiodec->decode_frame_param = audiodec->decode_frame_param%2;
					audiodec->decode_frame_num   = 2 - audiodec->decode_frame_param;
				} else if (node_pcm.frame_size / audiodec->decode_frame_num >= 0x200) {
					audiodec->decode_frame_param = audiodec->decode_frame_param%3;
					audiodec->decode_frame_num   = 3 - audiodec->decode_frame_param;
				} else if (node_pcm.frame_size / audiodec->decode_frame_num >= 0x100) {
					audiodec->decode_frame_param = audiodec->decode_frame_param%6;
					audiodec->decode_frame_num   = 6 - audiodec->decode_frame_param;
				} else {
					audiodec->decode_frame_param = 0;
					audiodec->decode_frame_num   = 1;
				}
			}
		}
	} else {
		audiodec->decode_restart_continue = DEC_RESTART;
		node_pcm.frame_size = 0;
	}

	if (node_pcm.channel_num == 0) {
		gxlog_w(LOG_ADEC, "=====> DECODE (id %d) channel num err\n");
		audiodec->decode_err = 1;
	}

	apts_compare_ctrl->pcm_len     = node_pcm.frame_size;
	apts_compare_ctrl->sample_fre  = audiodec->decode_samplefre;
	apts_compare_ctrl->esaEndAddr = audiodec->decode_esa_read_addr+audiodec->decode_esa_start_addr;//相对地址->绝对地址
	apts_compare_ctrl->frame_size = compress_frame_size;
	apts_compare_ctrl->bitrate    = audiodec->decode_bitrate;
	apts_compare_ctrl->error      = audiodec->decode_err;
	DECODE_ESA_READ_ADDR_UPDATA_NEXT_BYTE(audiodec);

	_find_audiodec_ctrlinfo(audiodec, apts_compare_ctrl);
	if (pts_save == 0) {
		unfix_pts = _find_audiodec_pts(audiodec,
				apts_compare_ctrl, (audiodec->dec_id == 0)?DECODE_0_ID:DECODE_1_ID);
	}
	node_pcm.pts = _fix_audiodec_pts(audiodec,
			apts_compare_ctrl, unfix_pts, (audiodec->dec_id == 0)?DECODE_0_ID:DECODE_1_ID);

	if(!audiodec->decode_err){
		if (audiodec->dec_type == CODEC_MPEG4_AAC) {
			if ((audiodec->dec_mode & CONVERT_MODE) == CONVERT_MODE) {
				unsigned int esa_w_addr = 0;

				if (last_samplefre != audiodec->decode_samplefre) {
					audiodec->convertinfo->sampling_rate = GX_AUDIO_GET_ENDIAN_32(audiodec->decode_samplefre);
				}

				gxav_sdc_rwaddr_get(audiodec->convert_esa_id, NULL, &esa_w_addr);
				*(volatile unsigned int*)audiodec->convert_pts_writed_addr = audiodec->convert_esa_start_addr +
					esa_w_addr%audiodec->convert_esa_size;
				audiodec->convert_pts_writed_addr += 4;
				if(audiodec->convert_pts_writed_addr >= (audiodec->convert_apts_ctrl.ptsbuf_end_addr + 1))
					audiodec->convert_pts_writed_addr = audiodec->convert_apts_ctrl.ptsbuf_start_addr;
				*(volatile unsigned int*)audiodec->convert_pts_writed_addr = node_pcm.pts;
				audiodec->convert_pts_writed_addr += 4;
				if(audiodec->convert_pts_writed_addr >= (audiodec->convert_apts_ctrl.ptsbuf_end_addr + 1))
					audiodec->convert_pts_writed_addr = audiodec->convert_apts_ctrl.ptsbuf_start_addr;

				gxav_sdc_length_set(audiodec->convert_pts_id, 8, GXAV_SDC_WRITE);
				gxav_sdc_length_set(audiodec->convert_esa_id, node_pcm.frame_size << 2, GXAV_SDC_WRITE);
			}

			if ((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE) {
				struct gxav_audio_syncinfo *audio_syncinfo = audiodec->syncinfo;

				if (!GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->unsupport_streamtype)) {
					GxAudioFrameNode node;

					memset(&node, 0, sizeof(GxAudioFrameNode));
					node.frame_s_addr = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->new_frame_start_addr);
					node.frame_e_addr = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->new_frame_end_addr);
					node.frame_size   = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_size);
					node.frame_bsmod  = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_bsmod);
					node.frame_add_num = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_add_num);
					node.sample_fre = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->sampling_rate);
					node.frame_type = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_type);
					node.frame_bytes = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_bytes);
					if (node.sample_fre == 0)
						node.sample_fre = 48000;
					node.pts = node_pcm.pts;
					if (audiodec->bypass_sample_rate != node.sample_fre) {
						node.sample_fre_change = 1;
						audiodec->bypass_sample_rate = node.sample_fre;
					}

					if (node.frame_size > 0) {
						gxav_sdc_length_set(audiodec->out_bypass_id, node.frame_size, GXAV_SDC_WRITE);
						gxfifo_put(audiodec->bypass_fifo, &node, sizeof(GxAudioFrameNode));
					}
				}
			}
		}

		gxfifo_put(audiodec->pcm_fifo, &node_pcm, sizeof(GxAudioFrameNode));
		if((audiodec->dec_type == CODEC_EAC3) &&
				((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE)){
			if (node_degrade.frame_size > 0) {
				node_degrade.sample_fre  = node_pcm.sample_fre;
				node_degrade.pts         = node_pcm.pts;
				node_degrade.sample_fre_change = node_pcm.sample_fre_change;

				gxfifo_put(audiodec->degrade_fifo, &node_degrade, sizeof(GxAudioFrameNode));
			}
		}
	}
	_start_decode_coding_data(audiodec);

	return ret;
}

static int _fill_bypass_coding_data(struct gxav_audiodec *audiodec)
{
	unsigned int esa_len, or_esa_len;
	unsigned int or_esa_w_addr;
	unsigned int esa_size = audiodec->bypass_esa_size;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	gxav_sdc_length_get(audiodec->bypass_esa_id, &esa_len);
	or_esa_w_addr = OR_GET_SPD_SAVED_ESAW_ADDR(audiodec_reg);
	or_esa_len = (or_esa_w_addr-audiodec->bypass_esa_read_addr+esa_size+1)%esa_size;

	if(esa_len < (or_esa_len + audiodec->bypass_need_data_num))
		audiodec->bypass_esa_writed_addr = audiodec->bypass_esa_read_addr;
	else
		audiodec->bypass_esa_writed_addr = (audiodec->bypass_esa_read_addr+esa_len-1)%esa_size;

	if(audiodec->bypass_esa_writed_addr != audiodec->bypass_esa_read_addr){
		audiodec->bypass_esa_empty = 0;
		BYPASS_TASK_LOG("[bypass (%d)] start task\n", audiodec->dec_id);
		OR_SET_SPD_ESA_W_ADDR(audiodec_reg, audiodec->bypass_esa_writed_addr);
		return 0;
	}else{
		BYPASS_TASK_LOG("[bypass (%d)] not enough data\n", audiodec->dec_id);
		audiodec->bypass_esa_empty = 1;
		audiodec->bypass_data_operate = SPD_FILL_DATA;
		return -1;
	}
}

static int _start_bypass_coding_data(struct gxav_audiodec *audiodec)
{
	unsigned int compress_len;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	if (audiodec->update && (audiodec->dec_state == AUDIODEC_OVER))
		return -1;

	gxav_sdc_length_get(audiodec->bypass_esa_id, &compress_len);

	if (((compress_len > 0) && (!audiodec->bypass_first_flag)) ||
			((compress_len > audiodec->enough_data) && (audiodec->bypass_first_flag))) {
		struct gxav_audio_syncinfo *audio_syncinfo = audiodec->syncinfo;
		unsigned int bypass_esa_free_len = 0;
		unsigned int bypass_pts_free_len = 0;
		unsigned int out_bypass_free_len  = 0;
		unsigned int enc_node_free_len  = 0;
		unsigned char start_to_bypass    = 1;
		unsigned int frame_size = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_size);

		frame_size = frame_size>0?frame_size:2048;
		audiodec->bypass_esa_empty  = 0;
		audiodec->bypass_first_flag = 0;
		if((audiodec->dec_mode & DECODE_MODE) == DECODE_MODE){
			gxav_sdc_free_get(audiodec->decode_esa_id, &bypass_esa_free_len);
			gxav_sdc_free_get(audiodec->decode_pts_id, &bypass_pts_free_len);
			start_to_bypass &= (bypass_pts_free_len >= 16)?1:0;
			if(6*frame_size < audiodec->bypass_esa_size)
				start_to_bypass &= (bypass_esa_free_len >= 6*frame_size)?1:0;
			else
				start_to_bypass &= (bypass_esa_free_len >= 2*frame_size)?1:0;
		}

		gxav_sdc_free_get(audiodec->out_bypass_id, &out_bypass_free_len);
		enc_node_free_len = gxfifo_freelen(audiodec->bypass_fifo);
		start_to_bypass &= (enc_node_free_len >= 2*(sizeof(GxAudioFrameNode)))?1:0;
		start_to_bypass &= (out_bypass_free_len >= 2*frame_size)?1:0;

		BYPASS_TASK_LOG("[bypass (%d)] start:%d\n", audiodec->dec_id, start_to_bypass);

		if(start_to_bypass){
			unsigned int esa_writed_addr = 0;
			unsigned int esa_size = audiodec->bypass_esa_size;

			audiodec->bypass_fifo_full = 0;
			gxav_sdc_rwaddr_get(audiodec->bypass_esa_id, NULL, &esa_writed_addr);
			audiodec->bypass_esa_writed_addr = (esa_size+esa_writed_addr-1)%esa_size;

			while(OR_GET_MCU_TASK_START_TO_WORK(audiodec_reg));
			OR_SET_SPD_FRAME_S_ADDR(audiodec_reg, audiodec->bypass_esa_read_addr);
			OR_SET_SPD_ESA_W_ADDR(audiodec_reg, audiodec->bypass_esa_writed_addr);

			gx_dcache_clean_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);
			OR_INT_SPD_PARSE_DONE_ENABLE(audiodec_reg);
			OR_SET_DECODE_TASK(audiodec_reg, SPD_PARSE);
			OR_SET_MCU_TASK_START_TO_WORK(audiodec_reg);
			return 0;
		}else{
			audiodec->bypass_fifo_full = 1;
			return -1;
		}
	}else{
		BYPASS_TASK_LOG("[bypass (%d)] not enough esa (update %d)\n", audiodec->dec_id, audiodec->update);
		if (audiodec->update && (audiodec->dec_mode == BYPASS_MODE)) {
			_set_audiodec_finish(audiodec);
		} else {
			audiodec->bypass_esa_empty = 1;
			audiodec->bypass_data_operate = SPD_PARSE_FRAME;
		}
		return -1;
	}
}

int bypass_writed_callback(unsigned int id, unsigned int lenx, unsigned int overflow, void *arg)
{
	int ret=-1;
	unsigned int len = 0;
	struct gxav_audiodec *audiodec = (struct gxav_audiodec*)arg;

	if (audiodec == NULL)
		return -1;

	if(audiodec->dec_state != AUDIODEC_READY &&
			audiodec->dec_state != AUDIODEC_RUNNING){
		return 0;
	}

	gxav_sdc_length_get(id, &len);
	BYPASS_ESA_LOG("[bypass (%d)] overflow %d, esa %d\n", audiodec->dec_id, overflow, (len - lenx));

	if (overflow) {
		audiodec->dec_state = AUDIODEC_ERROR;
		audiodec->dec_errno = AUDIODEC_ERR_ESA_OVERFLOW;
		return 0;
	}

	if(audiodec->bypass_esa_empty){
		switch(audiodec->bypass_data_operate)
		{
		case SPD_PARSE_FRAME:
			ret = _start_bypass_coding_data(audiodec);
			break;
		case SPD_FILL_DATA:
			ret = _fill_bypass_coding_data(audiodec);
			break;
		default:
			break;
		}
	}

	return ret;
}

static int bypass_readed_callback(unsigned int id, unsigned int lenx, unsigned int underflow, void *arg)
{
	int ret=-1;
	struct gxav_audiodec *audiodec = (struct gxav_audiodec*)arg;

	if(audiodec->dec_state != AUDIODEC_READY &&
			audiodec->dec_state != AUDIODEC_RUNNING){
		return 0;
	}

	if(audiodec->bypass_fifo_full){
		ret = _start_bypass_coding_data(audiodec);
	}

	return ret;
}

static int bypass_need_data_isr(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	BYPASS_TASK_LOG("[bypass (%d)] need data isr\n", audiodec->dec_id);

	OR_CLR_SPD_NEED_DATA_INT_STATUS(audiodec_reg);
	audiodec->bypass_need_data_num = OR_GET_SPD_NEED_DATA_NUM(audiodec_reg);
	_fill_bypass_coding_data(audiodec);

	return 0;
}

static int bypass_finish_dec_isr(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	BYPASS_TASK_LOG("[bypass (%d)] finish isr\n", audiodec->dec_id);

	_set_audiodec_finish(audiodec);
	OR_CLR_SPD_DATA_EMPTY_FINISH_INT_STATUS(audiodec_reg);

	return 0;
}

static int bypass_find_header_isr(struct gxav_audiodec *audiodec)
{
	int len = 0;
	unsigned int last_esa_read_addr  = audiodec->bypass_esa_read_addr;
	unsigned int cur_esa_writed_addr = audiodec->bypass_esa_writed_addr;
	unsigned int esa_size  = audiodec->bypass_esa_size;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	OR_CLR_SPD_SYNC_FINISH_INT_STATUS(audiodec_reg);

	audiodec->bypass_esa_read_addr = OR_GET_SPD_ESA_R_ADDR(audiodec_reg);
	len = _consume_data_len(audiodec->bypass_esa_read_addr, cur_esa_writed_addr, last_esa_read_addr, esa_size);
	gxav_sdc_length_set(audiodec->bypass_esa_id, len, GXAV_SDC_READ);

	BYPASS_ESA_READ_ADDR_UPDATA_NEXT_BYTE(audiodec);

	BYPASS_TASK_LOG("[bypass (%d)] find header isr\n", audiodec->dec_id);

	_start_bypass_coding_data(audiodec);

	return 0;
}

static int bypass_complete_parse_isr(struct gxav_audiodec *audiodec)
{
	int ret       = 0;
	int unfix_pts = 0;
	int frame_count;
	unsigned int res_len;
	GxAudioFrameNode node;
	unsigned int tmp_read_addr = 0;
	unsigned int last_esa_read_addr  = audiodec->bypass_esa_read_addr;
	unsigned int cur_esa_writed_addr = audiodec->bypass_esa_writed_addr;
	unsigned int esa_size  = audiodec->bypass_esa_size;
	struct gxav_apts_comparectrl *nor_apts_compare_ctrl = &audiodec->decode_apts_ctrl;
	struct gxav_apts_comparectrl *apts_compare_ctrl = &audiodec->bypass_apts_ctrl;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	struct gxav_audio_syncinfo* audio_syncinfo = audiodec->syncinfo;

	BYPASS_TASK_LOG("[bypass (%d)] complete isr\n", audiodec->dec_id);

	gx_dcache_inv_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);

	OR_INT_SPD_PARSE_DONE_DISABLE(audiodec_reg);
	OR_CLR_SPD_PARSE_DONE_INT_STATUS(audiodec_reg);

	audiodec->bypass_esa_read_addr = OR_GET_SPD_ESA_R_ADDR(audiodec_reg);
	audiodec->bypass_err  = OR_GET_PARSE_ERR_INDEX(audiodec_reg);

	frame_count = _consume_data_len(audiodec->bypass_esa_read_addr, cur_esa_writed_addr, last_esa_read_addr, esa_size);
	gxav_sdc_length_get(audiodec->bypass_esa_id, &res_len);

	gxav_sdc_rwaddr_get(audiodec->bypass_esa_id, &tmp_read_addr, NULL);
	if((tmp_read_addr + frame_count + esa_size - 1)%esa_size != audiodec->bypass_esa_read_addr) {
		gxlog_w(LOG_ADEC,
				"read addr %x, %x %x\n",
				audiodec->bypass_esa_read_addr, tmp_read_addr, frame_count);
	}
	if(res_len < frame_count){
		gxlog_w(LOG_ADEC,
				"frame_count %x, res_len %x, bypass_err %d\n",
				res_len, frame_count, audiodec->bypass_err);
		frame_count = res_len;
	}
	gxav_sdc_length_set(audiodec->bypass_esa_id, frame_count, GXAV_SDC_READ);

	if (!audiodec->bypass_err) {
		unsigned int frame_s_addr  = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->new_frame_start_addr);
		unsigned int frame_e_addr  = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->new_frame_end_addr);
		unsigned int frame_size    = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_size);
		unsigned int frame_add_num = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_add_num);
		unsigned int buffer_size   = audiodec->out_bypass_size;

		if ((frame_size + frame_add_num) !=
				((frame_e_addr + 1 + buffer_size - frame_s_addr) % buffer_size)) {
			gxlog_w(LOG_ADEC,
					"=====> BYPASS (id %d) frame_s_addr: %x, frame_e_addr %x, frame_size %x frame_add_num %x\n",
					audiodec->dec_id, frame_s_addr, frame_e_addr, frame_size, frame_add_num);
			audiodec->bypass_err = 1;
		}
	}

	if(!audiodec->bypass_err){
		gx_memset(&node, 0, sizeof(GxAudioFrameNode));
		node.frame_s_addr  = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->new_frame_start_addr);
		node.frame_e_addr  = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->new_frame_end_addr);
		node.frame_size    = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_size);
		node.frame_bsmod   = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_bsmod);
		node.frame_add_num = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_add_num);
		node.sample_fre    = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->sampling_rate);
		node.frame_type    = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_type);
		node.frame_bytes   = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_bytes);
		node.endian        = 1;
		if (node.sample_fre == 0)
			node.sample_fre = 48000;

		if(audiodec->dec_state != AUDIODEC_RUNNING &&
				audiodec->dec_state != AUDIODEC_ERROR)
		{
			if((audiodec->dec_mode & DECODE_MODE) != DECODE_MODE)
				ret = EVENT_AUDIO_FRAMEINFO;
			audiodec->dec_state = AUDIODEC_RUNNING;
			audiodec->dec_errno = AUDIODEC_ERR_NONE;
		}
		audiodec->bypass_frame_cnt++;
		if((node.sample_fre != audiodec->bypass_sample_rate) ||
				(node.frame_type != audiodec->bypass_frame_type))
			node.sample_fre_change = 1;

		gxav_sdc_length_set(audiodec->out_bypass_id, node.frame_size+node.frame_add_num, GXAV_SDC_WRITE);
	}
	apts_compare_ctrl->esaEndAddr = audiodec->bypass_esa_read_addr+audiodec->bypass_esa_start_addr;//相对地址->绝对地址
	apts_compare_ctrl->frame_size = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_size);
	apts_compare_ctrl->bitrate    = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->bit_rate);
	apts_compare_ctrl->error      = audiodec->bypass_err;
	apts_compare_ctrl->sample_fre = GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->sampling_rate);
	if (apts_compare_ctrl->sample_fre == 0)
		apts_compare_ctrl->sample_fre = 48000;

	if (GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_type) == AUDIO_TYPE_AC3)
		apts_compare_ctrl->pcm_len    = AC3_FRAME_LENGTH;
	else if (GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_type) == AUDIO_TYPE_EAC3)
		apts_compare_ctrl->pcm_len    = E_AC3_FRAME_LENGTH;
	else if (GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_type) == AUDIO_TYPE_DTS)
		apts_compare_ctrl->pcm_len    = node.frame_bytes;

	audiodec->bypass_samplefre = apts_compare_ctrl->sample_fre;
	if (GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->frame_type) == AUDIO_TYPE_DTS) {
		audiodec->bypass_bitrate = apts_compare_ctrl->bitrate / 1000;
	} else
		audiodec->bypass_bitrate = apts_compare_ctrl->bitrate;

	BYPASS_ESA_READ_ADDR_UPDATA_NEXT_BYTE(audiodec);

	unfix_pts = _find_audiodec_pts(audiodec, apts_compare_ctrl, BYPASS_0_ID);
	node.pts  =  _fix_audiodec_pts(audiodec, apts_compare_ctrl, unfix_pts, BYPASS_0_ID);

	if ((!audiodec->bypass_err) || (audiodec->bypass_err == AUDIODEC_BYPASS_ERROR_RUBBISH)) {
		if((audiodec->dec_mode & DECODE_MODE) == DECODE_MODE){
			*(volatile unsigned int*)audiodec->decode_pts_writed_addr = audiodec->decode_esa_start_addr +
				GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->esa_start_addr);
			audiodec->decode_pts_writed_addr += 4;
			if(audiodec->decode_pts_writed_addr >= (nor_apts_compare_ctrl->ptsbuf_end_addr + 1))
				audiodec->decode_pts_writed_addr = nor_apts_compare_ctrl->ptsbuf_start_addr;
			*(volatile unsigned int*)audiodec->decode_pts_writed_addr = node.pts;
			audiodec->decode_pts_writed_addr += 4;
			if(audiodec->decode_pts_writed_addr >= (nor_apts_compare_ctrl->ptsbuf_end_addr + 1))
				audiodec->decode_pts_writed_addr = nor_apts_compare_ctrl->ptsbuf_start_addr;
			gxav_sdc_length_set(audiodec->decode_pts_id, 8, GXAV_SDC_WRITE);

			gxav_sdc_length_set(audiodec->decode_esa_id,
					node.frame_size + GX_AUDIO_GET_ENDIAN_32(audio_syncinfo->rubish_data_len), GXAV_SDC_WRITE);
		}

		if ((!audiodec->bypass_err) && (node.frame_size+node.frame_add_num > 0)) {
			audiodec->bypass_sample_rate = node.sample_fre;
			audiodec->bypass_frame_type  = node.frame_type;
			gxfifo_put(audiodec->bypass_fifo, &node, sizeof(GxAudioFrameNode));
		}
	}

	_start_bypass_coding_data(audiodec);

	return ret;
}

static int _fill_convert_coding_data(struct gxav_audiodec *audiodec)
{
	unsigned int esa_len, or_esa_len;
	unsigned int or_esa_w_addr;
	unsigned int esa_size = audiodec->convert_esa_size;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	gxav_sdc_length_get(audiodec->convert_esa_id, &esa_len);
	or_esa_w_addr = OR_GET_SPD_SAVED_ESAW_ADDR(audiodec_reg);
	or_esa_len    = (or_esa_w_addr-audiodec->convert_esa_read_addr+esa_size+1)%esa_size;

	if (esa_len < (or_esa_len + audiodec->convert_need_data_num))
		audiodec->convert_esa_writed_addr = audiodec->convert_esa_read_addr;
	else
		audiodec->convert_esa_writed_addr = (audiodec->convert_esa_read_addr+esa_len-1)%esa_size;

	if (audiodec->convert_esa_writed_addr != audiodec->convert_esa_read_addr) {
		audiodec->convert_esa_empty = 0;
		CONVERT_TASK_LOG("[convert (%d)] start task\n", audiodec->dec_id);
		OR_SET_SPD_ESA_W_ADDR(audiodec_reg, audiodec->convert_esa_writed_addr);
		return 0;
	} else {
		CONVERT_TASK_LOG("[convert (%d)] not enough data\n", audiodec->dec_id);
		audiodec->convert_esa_empty = 1;
		audiodec->convert_data_operate = CVT_FILL_DATA;
		return -1;
	}
	return 0;
}

static int _start_convert_coding_data(struct gxav_audiodec *audiodec)
{
	unsigned int compress_len = 0;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	if (audiodec->update && (audiodec->dec_state == AUDIODEC_OVER))
		return -1;

	gxav_sdc_length_get(audiodec->convert_esa_id, &compress_len);

	if (((compress_len > 0) && (!audiodec->convert_first_flag)) ||
			((compress_len > audiodec->enough_data) && (audiodec->convert_first_flag))) {
		struct gxav_audio_convertinfo *audio_convertinfo = audiodec->convertinfo;
		unsigned int out_convert_free_len = 0;
		unsigned int cvt_node_free_len    = 0;
		unsigned char start_to_convert    = 1;
		unsigned int frame_size = GX_AUDIO_GET_ENDIAN_32(audio_convertinfo->frame_size);

		frame_size = (frame_size > 0) ? frame_size : 2048;
		audiodec->convert_esa_empty  = 0;
		audiodec->convert_first_flag = 0;

		gxav_sdc_free_get(audiodec->out_convert_id, &out_convert_free_len);

		cvt_node_free_len = gxfifo_freelen(audiodec->encode_fifo);
		start_to_convert &= (cvt_node_free_len >= 2*(sizeof(GxAudioFrameNode)))?1:0;
		start_to_convert &= (out_convert_free_len >= 2*frame_size)?1:0;

		CONVERT_TASK_LOG("[convert (%d)] start:%d\n", audiodec->dec_id, start_to_convert);

		if(start_to_convert){
			unsigned int esa_writed_addr = 0;
			unsigned int esa_size = audiodec->convert_esa_size;

			audiodec->convert_fifo_full = 0;
			gxav_sdc_rwaddr_get(audiodec->convert_esa_id, NULL, &esa_writed_addr);
			audiodec->convert_esa_writed_addr = (esa_size+esa_writed_addr-1)%esa_size;

			while(OR_GET_MCU_TASK_START_TO_WORK(audiodec_reg));
			OR_SET_SPD_FRAME_S_ADDR(audiodec_reg, audiodec->convert_esa_read_addr);
			OR_SET_SPD_ESA_W_ADDR(audiodec_reg, audiodec->convert_esa_writed_addr);
			OR_SET_COM_TARGET_W_SADDR(audiodec_reg, audiodec->out_convert_writed_addr);

			gx_dcache_clean_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);
			OR_INT_CONVERT_FINISH_ENABLE(audiodec_reg);
			OR_SET_DECODE_TASK(audiodec_reg, CONVERT_TO_AC3);
			OR_SET_MCU_TASK_START_TO_WORK(audiodec_reg);
			return 0;
		}else{
			audiodec->convert_fifo_full = 1;
			return -1;
		}
	}else{
		CONVERT_TASK_LOG("[convert (%d)] not enough esa (update %d)\n", audiodec->dec_id, audiodec->update);
		audiodec->convert_esa_empty = 1;
		audiodec->convert_data_operate = CVT_ENC_FRAME;
		return -1;
	}

	return 0;
}

static int convert_writed_callback(unsigned int id, unsigned int lenx, unsigned int overflow, void *arg)
{
	int ret=-1;
	unsigned int len = 0;
	struct gxav_audiodec *audiodec = (struct gxav_audiodec*)arg;

	if(audiodec->dec_state != AUDIODEC_READY &&
			audiodec->dec_state != AUDIODEC_RUNNING){
		return 0;
	}

	gxav_sdc_length_get(id, &len);
	CONVERT_ESA_LOG("[convert (%d)] overflow %d, esa %d\n", audiodec->dec_id, overflow, (len - lenx));

	if (overflow) {
		audiodec->dec_state = AUDIODEC_ERROR;
		audiodec->dec_errno = AUDIODEC_ERR_ESA_OVERFLOW;
		gxlog_d(LOG_ADEC, "%s %d overflow %d\n", __func__, __LINE__, overflow);
		return 0;
	}
	if (audiodec->convert_esa_empty) {
		switch(audiodec->convert_data_operate)
		{
		case CVT_ENC_FRAME:
			ret = _start_convert_coding_data(audiodec);
			break;
		case CVT_FILL_DATA:
			ret = _fill_convert_coding_data(audiodec);
			break;
		default:
			break;
		}
	}

	return ret;
	return 0;
}

static int convert_readed_callback(unsigned int id, unsigned int lenx, unsigned int underflow, void *arg)
{
	int ret=-1;
	struct gxav_audiodec *audiodec = (struct gxav_audiodec*)arg;

	if(audiodec->dec_state != AUDIODEC_READY &&
			audiodec->dec_state != AUDIODEC_RUNNING){
		return 0;
	}
	if(audiodec->convert_fifo_full){
		ret = _start_convert_coding_data(audiodec);
	}
	return ret;
}

static int convert_need_data_isr(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	CONVERT_TASK_LOG("[convert (%d)] need data isr\n", audiodec->dec_id);

	OR_CLR_CONVERT_NEED_DATA_INT_STATUS(audiodec_reg);
	audiodec->convert_need_data_num = OR_GET_SPD_NEED_DATA_NUM(audiodec_reg);

	_fill_convert_coding_data(audiodec);

	return 0;
}

static int convert_finish_enc_isr(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	CONVERT_TASK_LOG("[convert (%d)] finish isr\n", audiodec->dec_id);

	_set_audiodec_finish(audiodec);
	OR_CLR_CONVERT_DATA_EMPTY_FINISH_INT_STATUS(audiodec_reg);
	return 0;
}

static int convert_complete_parse_isr(struct gxav_audiodec *audiodec)
{
	int ret = 0;
	int len = 0;
	int unfix_pts = 0;
	GxAudioFrameNode node;
	unsigned int last_esa_read_addr = audiodec->convert_esa_read_addr;
	unsigned int cur_esa_writed_addr = audiodec->convert_esa_writed_addr;
	unsigned int last_convert_write_addr = audiodec->out_convert_writed_addr;

	unsigned int convert_esa_size = audiodec->convert_esa_size;
	struct gxav_apts_comparectrl *apts_compare_ctrl = &audiodec->convert_apts_ctrl;
	struct gxav_audio_convertinfo *audio_convertinfo = audiodec->convertinfo;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	unsigned int frame_size = GX_AUDIO_GET_ENDIAN_32(audio_convertinfo->frame_size);

	CONVERT_TASK_LOG("[convert (%d)] complete isr\n", audiodec->dec_id);

	gx_dcache_inv_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);

	gx_memset(&node, 0, sizeof(GxAudioFrameNode));
	OR_INT_CONVERT_FINISH_DISABLE(audiodec_reg);
	OR_CLR_CONVERT_FINISH_INT_STATUS(audiodec_reg);

	audiodec->convert_esa_read_addr = OR_GET_SPD_ESA_R_ADDR(audiodec_reg);
	audiodec->convert_err  = OR_GET_PARSE_ERR_INDEX(audiodec_reg);

	len = _consume_data_len(audiodec->convert_esa_read_addr, cur_esa_writed_addr, last_esa_read_addr, convert_esa_size);
	gxav_sdc_length_set(audiodec->convert_esa_id, len, GXAV_SDC_READ);

	apts_compare_ctrl->esaEndAddr = audiodec->convert_esa_read_addr+audiodec->convert_esa_start_addr;//相对地址->绝对地址
	apts_compare_ctrl->frame_size = frame_size;
	apts_compare_ctrl->bitrate    = 640000;
	apts_compare_ctrl->sample_fre = audiodec->decode_samplefre;
	apts_compare_ctrl->error      = audiodec->convert_err;
	apts_compare_ctrl->pcm_len    = AC3_FRAME_LENGTH;

	CONVERT_ESA_READ_ADDR_UPDATA_NEXT_BYTE(audiodec);

	unfix_pts = _find_audiodec_pts(audiodec, apts_compare_ctrl, CONVERT_0_ID);
	node.pts  =  _fix_audiodec_pts(audiodec, apts_compare_ctrl, unfix_pts, CONVERT_0_ID);

	if (!audiodec->convert_err) {
		unsigned int out_convert_size = audiodec->out_convert_size;
		unsigned int convert_len = 0;

		audiodec->convert_frame_cnt++;
		audiodec->out_convert_writed_addr = OR_GET_COM_TARGET_EADDR(audiodec_reg);

		convert_len = (audiodec->out_convert_writed_addr-last_convert_write_addr+out_convert_size+1)%out_convert_size;
		node.frame_s_addr = last_convert_write_addr;
		node.frame_e_addr = audiodec->out_convert_writed_addr;
		node.frame_size   = convert_len;
		node.frame_type   = AUDIO_TYPE_AC3;
		node.sample_fre   = audiodec->decode_samplefre;
		node.frame_bsmod  = 0;//GX_AUDIO_GET_ENDIAN_32(audio_convertinfo->bsmod);
		node.endian       = 1;
		if (audiodec->convert_sample_rate != node.sample_fre) {
			node.sample_fre_change = 1;
			audiodec->convert_sample_rate = node.sample_fre;
		}
		CONVERT_WRITE_ADDR_UPDATA_NEXT_BYTE(audiodec);
		gxav_sdc_length_set(audiodec->out_convert_id, convert_len, GXAV_SDC_WRITE);

		gxfifo_put(audiodec->encode_fifo, &node, sizeof(GxAudioFrameNode));
	}

	_start_convert_coding_data(audiodec);

	return ret;
}

static int gx3201_audiodec_init(void)
{
	//0xa030a148开启固件打印信息
	unsigned int reg_map_addr = 0;
	struct gx_mem_info fwmem = {0};
#define XMAX(a, b) ((a > b) ? a : b)

	_scan_audiodec_firmware();
	firmware_protect_flag  = gxav_firewall_buffer_protected(GXAV_BUFFER_AUDIO_FIRMWARE) ? 1 : 0;
	pcmbuffer_protect_flag = gxav_firewall_buffer_protected(GXAV_BUFFER_AUDIO_FRAME)    ? 1 : 0;
	esabuffer_protect_flag = gxav_firewall_buffer_protected(GXAV_BUFFER_DEMUX_ES)       ? 1 : 0;
	if (gx_mem_info_get("afwmem", &fwmem) == 0) {
		unsigned int i = 0;
		unsigned int max_size = 0;

		for (i = 0; i < sizeof(audio_fim)/sizeof(struct audio_firmware); i++) {
			if (firmware_protect_flag) {
				if (audio_fim[i].firmware_id == GXAV_FMID_AUDIO_ALL) {
					max_size = XMAX(max_size, audio_fim[i].min_work_buf);
					break;
				}
			} else {
				max_size = XMAX(max_size, audio_fim[i].min_work_buf);
			}
		}

		if ((max_size <= 0) || ((max_size + AUDIODEC_EXTEND_REG_SIZE) >= fwmem.size)) {
			gxlog_w(LOG_ADEC,
					"workbuf size(%x %x %x)\n",
					max_size, AUDIODEC_EXTEND_REG_SIZE, fwmem.size);
		}
		workbuf_protect_size = max_size;
		workbuf_protect_addr = fwmem.start;
	}

	if(NULL == gx_request_mem_region(audio_dec[0].reg_base_addr, sizeof(struct audiodec_regs))){
		gxlog_e(LOG_ADEC, "%s %d [mem region failed]\n", __func__, __LINE__);
		return -1;
	}

	reg_map_addr = (unsigned int)gx_ioremap(audio_dec[0].reg_base_addr, sizeof(struct audiodec_regs));
	if(!reg_map_addr) {
		gx_release_mem_region(audio_dec[0].reg_base_addr, sizeof(struct audiodec_regs));
		gxlog_e(LOG_ADEC, "%s %d [reg addr error]\n", __func__, __LINE__);
		return -1;
	}
	audio_dec[0].reg = (struct audiodec_regs*)reg_map_addr;

	if (CHIP_IS_SIRIUS)
		OR_SET_FIRMWARE_ENDIAN(audio_dec[0].reg, 3);
	else
		OR_SET_FIRMWARE_ENDIAN(audio_dec[0].reg, 0);

	if (gxav_firewall_access_align()) {
		OR_SET_AUDIO2_DATA_ENCRYPTED(audio_dec[0].reg, 1);
		OR_SET_NORMAL_DATA_ENCRYPTED(audio_dec[0].reg, 1);
		if (CHIP_IS_SIRIUS)
			OR_SET_NORMAL_AXI_8TO32_CTRL(audio_dec[0].reg, 5);
	} else {
		OR_SET_AUDIO2_DATA_ENCRYPTED(audio_dec[0].reg, 0);
		OR_SET_NORMAL_DATA_ENCRYPTED(audio_dec[0].reg, 0);
	}

	if (!firmware_protect_flag) {
		_check_audiodec_chip_type(audio_dec[0].reg);
		gxav_firmware_check_static();
	}

	return 0;
}

static int gx3201_audiodec_open(struct gxav_audiodec *audiodec)
{
	if(audiodec->dec_id == 0){
		audiodec->audiodec_reg = audio_dec[0].reg;

		audiodec->decode_channel_size    = 0;
		audiodec->decode_channel_buffer  = 0;
		audiodec->convert_channel_size   = 0;
		audiodec->convert_channel_buffer = 0;
		audiodec->boostdb                = 0;
		audiodec->volumedb               = 3; //audio play upsamplerate need 3db, 24khz audio
		audiodec->enough_data            = 0;

		audiodec->pcm_fifo     = (struct gxfifo*)gx_mallocz(sizeof(struct gxfifo));
		audiodec->bypass_fifo  = (struct gxfifo*)gx_mallocz(sizeof(struct gxfifo));
		audiodec->degrade_fifo = (struct gxfifo*)gx_mallocz(sizeof(struct gxfifo));
		audiodec->encode_fifo  = (struct gxfifo*)gx_mallocz(sizeof(struct gxfifo));
		if((audiodec->pcm_fifo == NULL) ||
				(audiodec->bypass_fifo == NULL) ||
				(audiodec->degrade_fifo == NULL)) {
			if(audiodec->pcm_fifo){
				gx_free(audiodec->pcm_fifo);
				audiodec->pcm_fifo = NULL;
			}
			if(audiodec->bypass_fifo){
				gx_free(audiodec->bypass_fifo);
				audiodec->bypass_fifo = NULL;
			}
			if(audiodec->degrade_fifo){
				gx_free(audiodec->degrade_fifo);
				audiodec->degrade_fifo = NULL;
			}
			if(audiodec->encode_fifo){
				gx_free(audiodec->encode_fifo);
				audiodec->encode_fifo = NULL;
			}
			return -1;
		}

		gxfifo_init(audiodec->pcm_fifo,    NULL, sizeof(GxAudioFrameNode)*20);
		gxfifo_init(audiodec->bypass_fifo, NULL, sizeof(GxAudioFrameNode)*20);
		gxfifo_init(audiodec->degrade_fifo,NULL, sizeof(GxAudioFrameNode)*20);
		gxfifo_init(audiodec->encode_fifo, NULL, sizeof(GxAudioFrameNode)*20);
	}else if(audiodec->dec_id == 1){
		audiodec->pcm_fifo      = (struct gxfifo*)gx_mallocz(sizeof(struct gxfifo));
		audiodec->ctrlinfo_fifo = (struct gxfifo*)gx_mallocz(sizeof(struct gxfifo));
		if((audiodec->pcm_fifo == NULL) || (audiodec->ctrlinfo_fifo == NULL)){
			if(audiodec->pcm_fifo){
				gx_free(audiodec->pcm_fifo);
				audiodec->pcm_fifo = NULL;
			}
			if(audiodec->ctrlinfo_fifo){
				gx_free(audiodec->ctrlinfo_fifo);
				audiodec->ctrlinfo_fifo = NULL;
			}
			return -1;
		}
		gxfifo_init(audiodec->pcm_fifo,      NULL, sizeof(GxAudioFrameNode)*20);
		gxfifo_init(audiodec->ctrlinfo_fifo, NULL, sizeof(GxAudioDecProperty_ContrlInfo)*1024);
		audiodec->decode_ctrlinfo_enable = 1;
	}else
		return -1;

	return 0;
}

static int gx3201_audiodec_config(struct gxav_audiodec *audiodec, GxAudioDecProperty_Config *config)
{
	if ((config->type < CODEC_MPEG12A) || (config->type > CODEC_SBC)) {
		gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if(audiodec->dec_id == 0){
		if(_config_audiodec_address(audiodec, config) < 0){
			gxlog_e(LOG_ADEC, "%s %d [config adrress failed]\n", __func__, __LINE__);
			return -1;
		}

		if(_config_audiodec_decmode(audiodec, config) < 0){
			gxlog_e(LOG_ADEC, "%s %d [config firmware failed]\n", __func__, __LINE__);
			return -1;
		}

		if(_config_audiodec_header(audiodec, config) < 0){
			gxlog_e(LOG_ADEC, "%s %d [config header failed]\n", __func__, __LINE__);
			return -1;
		}

		if(audiodec->pcm_fifo)
			gxfifo_reset(audiodec->pcm_fifo);
		if(audiodec->bypass_fifo)
			gxfifo_reset(audiodec->bypass_fifo);
		if(audiodec->degrade_fifo)
			gxfifo_reset(audiodec->degrade_fifo);
		if(audiodec->encode_fifo)
			gxfifo_reset(audiodec->encode_fifo);

		audiodec->pcm_frame_len     = 0;
		audiodec->downmix =  (audiodec->dec_type == CODEC_PCM) ? 1 : config->down_mix;
		audiodec->downmix_active   = 1;
		audiodec->volumedb_active  = 1;
		audiodec->mono_en_active   = 1;
		audiodec->boostdb_active   = 1;
		audiodec->enough_data      = config->enough_data;
		_check_audiodec_active_params(audiodec);

		audiodec->decode_esa_empty   = 0;
		audiodec->decode_pcm_full    = 0;
		audiodec->bypass_esa_empty   = 0;
		audiodec->bypass_fifo_full   = 0;
		audiodec->decode_samplefre   = 0;
		audiodec->decode_channelnum  = 0;
		audiodec->decode_bitrate     = 0;
		audiodec->decode_first_flag  = 1;
		audiodec->bypass_sample_rate = 0;
		audiodec->bypass_frame_type  = 0;
		audiodec->bypass_first_flag  = 1;
		audiodec->bypass_samplefre   = 0;
		audiodec->bypass_bitrate     = 0;
		audiodec->pcm_active         = 0;
		audiodec->stream_type        = STREAM_ES;
		audiodec->stream_pid         = -1;
	}else if(audiodec->dec_id == 1){
		struct gxav_audiodec *audiodec0 = gxav_audiodec_find_instance(0);

		if (audiodec0 == NULL) {
			gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
			return -1;
		}

		if (audiodec0->firmware_id == GXAV_FMID_AUDIO_MPEG) {
			if ((config->type != CODEC_MPEG12A) && (config->type != CODEC_MPEG4_AAC)) {
				gxlog_e(LOG_ADEC,
						"%s %d [only support mpeg12/aac]\n",
						__func__, __LINE__);
				return -1;
			}
		} else if (audiodec0->firmware_id == GXAV_FMID_DOLBY) {
			if ((config->type != CODEC_AC3) && (config->type != CODEC_EAC3)) {
				gxlog_e(LOG_ADEC,
						"%s %d [only support only support ac3/eac3]\n",
						__func__, __LINE__);
				return -1;
			}
		} else if (audiodec0->firmware_id == GXAV_FMID_AUDIO_ALL) {
			if ((config->type != CODEC_MPEG12A) && (config->type != CODEC_MPEG4_AAC) &&
					(config->type != CODEC_AC3) && (config->type != CODEC_EAC3)) {
				gxlog_e(LOG_ADEC,
						"%s %d [only support mpeg12/aac]\n",
						__func__, __LINE__);
				return -1;
			}
		} else
			return -1;

		audiodec->dec_type     = audiodec0->dec_type;
		audiodec->dec_mode     = audiodec0->dec_mode;
		audiodec->addec_reg    = audio_dec[1].reg;
		audiodec->audiodec_reg = audio_dec[0].reg;
		audiodec->dec_params   = audio_dec[1].dec_params;
		audiodec->enough_data  = config->enough_data;
		audiodec->regmem_virt_addr = audio_dec[1].extend_reg_addr;
		audiodec->regmem_virt_size = audio_dec[1].extend_reg_size;

		if(audiodec->pcm_fifo)
			gxfifo_reset(audiodec->pcm_fifo);
		if(audiodec->ctrlinfo_fifo)
			gxfifo_reset(audiodec->ctrlinfo_fifo);
		audiodec->decode_esa_empty = 0;
		audiodec->decode_pcm_full = 0;
		audiodec->decode_samplefre  = 0;
		audiodec->decode_channelnum = 0;
		audiodec->decode_bitrate    = 0;
		audiodec->decode_first_flag = 1;
		audiodec->pcm_active        = 0;
		audiodec->stream_type       = config->stream_type;
		audiodec->stream_pid        = config->stream_pid;
		gxlog_i(LOG_ADEC, "audio description pid: %d\n", audiodec->stream_pid);
	}

	audiodec->update    = 0;
	audiodec->dec_state = AUDIODEC_INITED;
	audiodec->dec_errno = AUDIODEC_ERR_NONE;
	if (audiodec->dec_type == CODEC_PCM)
		audiodec->max_channel_num = 2;
	else
		audiodec->max_channel_num = config->down_mix?2:8;//downmix, max only output two channel
	return 0;
}

static int gx3201_audiodec_get_outinfo(struct gxav_audiodec *audiodec, GxAudioDecProperty_OutInfo *info)
{
	if (audiodec->dec_id == 0) {
		if (audiodec->dec_type == CODEC_EAC3) {
			if (audiodec->dec_mode == BYPASS_MODE)
				info->outdata_type = EAC3_TYPE;
			else if (audiodec->dec_mode == DECODE_MODE)
				info->outdata_type = PCM_TYPE;
			else if (audiodec->dec_mode == (BYPASS_MODE|DECODE_MODE))
				info->outdata_type = (PCM_TYPE|AC3_TYPE|EAC3_TYPE);
		} else if (audiodec->dec_type == CODEC_AC3) {
			if (audiodec->dec_mode == BYPASS_MODE)
				info->outdata_type = AC3_TYPE;
			else if (audiodec->dec_mode == DECODE_MODE)
				info->outdata_type = PCM_TYPE;
			else if (audiodec->dec_mode == (BYPASS_MODE|DECODE_MODE))
				info->outdata_type = (PCM_TYPE|AC3_TYPE);
		} else if (audiodec->dec_type == CODEC_DTS) {
			info->outdata_type = DTS_TYPE;
		} else if (audiodec->dec_type == CODEC_MPEG4_AAC) {
			info->outdata_type = PCM_TYPE;
			if ((audiodec->dec_mode & CONVERT_MODE) == CONVERT_MODE)
				info->outdata_type |= AC3_TYPE;
			if ((audiodec->dec_mode & BYPASS_MODE ) == BYPASS_MODE)
				info->outdata_type |= AAC_TYPE;
		} else
			info->outdata_type = PCM_TYPE;
	}else if(audiodec->dec_id == 1){
		info->outdata_type = ADPCM_TYPE;
	}

	return 0;
}

static int _config_bypass_input_fifo(struct gxav_audiodec *audiodec, struct audiodec_fifo *fifo)
{
	unsigned int  esa_buf_size   = (fifo->buffer_end_addr - fifo->buffer_start_addr + 1);

	audiodec->bypass_esa_id         = fifo->channel_id;
	audiodec->bypass_pts_id         = fifo->channel_pts_id;
	audiodec->bypass_esa_start_addr = fifo->buffer_start_addr;
	audiodec->bypass_esa_size       = esa_buf_size;
	audiodec->bypass_esa_read_addr  = 0;
	audiodec->bypass_esa_writed_addr= 0;

	audiodec->bypass_apts_ctrl.esa_buf_id          = fifo->channel_id;
	audiodec->bypass_apts_ctrl.esaStartAddr        = fifo->buffer_start_addr;
	audiodec->bypass_apts_ctrl.esabuf_start_addr   = fifo->buffer_start_addr;
	audiodec->bypass_apts_ctrl.esabuf_end_addr     = fifo->buffer_end_addr;

	audiodec->bypass_apts_ctrl.pts_buf_id          = fifo->channel_pts_id;
	audiodec->bypass_apts_ctrl.ptsbuf_start_addr   = fifo->pts_start_addr;
	audiodec->bypass_apts_ctrl.ptsbuf_end_addr     = fifo->pts_end_addr;
	audiodec->bypass_apts_ctrl.pts_buf_target_addr = fifo->pts_start_addr;
	audiodec->bypass_apts_ctrl.pts_buf_end_addr    = fifo->pts_start_addr;
	audiodec->bypass_apts_ctrl.esa_buf_size        = esa_buf_size;
	audiodec->bypass_apts_ctrl.esa_buf_count       = 0;
	audiodec->bypass_apts_ctrl.ad_ctrl_invaild     = 0;
	audiodec->bypass_apts_ctrl.total_esa_buf_count = 0;
	audiodec->bypass_apts_ctrl.pts_esa_buf_count   = 0;
	audiodec->bypass_apts_ctrl.pts_sdram_addr_save = 0;
	audiodec->bypass_apts_ctrl.last_pts  = -1;
	audiodec->bypass_apts_ctrl.unit_time = -1;
	audiodec->bypass_apts_ctrl.unit_time_dis = 0;

	return 0;
}

static int _config_decode_input_fifo(struct gxav_audiodec *audiodec, struct audiodec_fifo *fifo)
{
	unsigned int  esa_buf_size   = (fifo->buffer_end_addr - fifo->buffer_start_addr + 1);
	unsigned int  pts_buf_size   = (fifo->pts_end_addr    - fifo->pts_start_addr    + 1);

	audiodec->decode_esa_id           = fifo->channel_id;
	audiodec->decode_esa_start_addr   = fifo->buffer_start_addr;
	audiodec->decode_esa_size         = esa_buf_size;
	audiodec->decode_esa_read_addr    = 0;
	audiodec->decode_esa_writed_addr  = 0;

	audiodec->decode_pts_id           = fifo->channel_pts_id;
	audiodec->decode_pts_writed_addr  = fifo->pts_start_addr;
	audiodec->decode_pts_read_addr    = 0;
	audiodec->decode_pts_size         = pts_buf_size;

	audiodec->decode_apts_ctrl.esa_buf_id          = fifo->channel_id;
	audiodec->decode_apts_ctrl.esaStartAddr        = fifo->buffer_start_addr;
	audiodec->decode_apts_ctrl.esabuf_start_addr   = fifo->buffer_start_addr;
	audiodec->decode_apts_ctrl.esabuf_end_addr     = fifo->buffer_end_addr;
	audiodec->decode_apts_ctrl.pts_buf_id          = fifo->channel_pts_id;
	audiodec->decode_apts_ctrl.ptsbuf_start_addr   = fifo->pts_start_addr;
	audiodec->decode_apts_ctrl.ptsbuf_end_addr     = fifo->pts_end_addr;
	audiodec->decode_apts_ctrl.pts_buf_target_addr = fifo->pts_start_addr;
	audiodec->decode_apts_ctrl.pts_buf_end_addr    = fifo->pts_start_addr;
	audiodec->decode_apts_ctrl.esa_buf_size        = esa_buf_size;
	audiodec->decode_apts_ctrl.esa_buf_count       = 0;
	audiodec->decode_apts_ctrl.total_esa_buf_count = 0;
	audiodec->decode_apts_ctrl.ad_ctrl_invaild     = 0;
	audiodec->decode_apts_ctrl.pts_esa_buf_count   = 0;
	audiodec->decode_apts_ctrl.pts_sdram_addr_save = 0;
	audiodec->decode_apts_ctrl.last_pts  = -1;
	audiodec->decode_apts_ctrl.unit_time = -1;
	audiodec->decode_apts_ctrl.unit_time_dis = 0;

	return 0;
}

static int _config_convert_input_fifo(struct gxav_audiodec *audiodec, struct audiodec_fifo *fifo)
{
	unsigned int buf_esa_size = (fifo->buffer_end_addr - fifo->buffer_start_addr + 1);

	audiodec->convert_esa_id           = fifo->channel_id;
	audiodec->convert_esa_start_addr   = fifo->buffer_start_addr;
	audiodec->convert_esa_size         = buf_esa_size/2;
	audiodec->convert_esa_read_addr    = 0;
	audiodec->convert_esa_writed_addr  = 0;
	audiodec->convert_pts_id           = fifo->channel_pts_id;
	audiodec->convert_pts_read_addr    = 0;
	audiodec->convert_pts_writed_addr  = fifo->pts_start_addr;

	audiodec->convert_apts_ctrl.esa_buf_id          = fifo->channel_id;
	audiodec->convert_apts_ctrl.esaStartAddr        = fifo->buffer_start_addr;
	audiodec->convert_apts_ctrl.esabuf_start_addr   = fifo->buffer_start_addr;
	audiodec->convert_apts_ctrl.esabuf_end_addr     = fifo->buffer_end_addr;
	audiodec->convert_apts_ctrl.pts_buf_id          = fifo->channel_pts_id;
	audiodec->convert_apts_ctrl.ptsbuf_start_addr   = fifo->pts_start_addr;
	audiodec->convert_apts_ctrl.ptsbuf_end_addr     = fifo->pts_end_addr;
	audiodec->convert_apts_ctrl.pts_buf_target_addr = fifo->pts_start_addr;
	audiodec->convert_apts_ctrl.pts_buf_end_addr    = fifo->pts_start_addr;
	audiodec->convert_apts_ctrl.esa_buf_size        = buf_esa_size/2;
	audiodec->convert_apts_ctrl.esa_buf_count       = 0;
	audiodec->convert_apts_ctrl.total_esa_buf_count = 0;
	audiodec->convert_apts_ctrl.ad_ctrl_invaild     = 0;
	audiodec->convert_apts_ctrl.pts_esa_buf_count   = 0;
	audiodec->convert_apts_ctrl.pts_sdram_addr_save = 0;
	audiodec->convert_apts_ctrl.last_pts            = -1;
	audiodec->convert_apts_ctrl.unit_time           = -1;
	audiodec->convert_apts_ctrl.unit_time_dis       = 0;

	return 0;
}

static struct audiodec_fifo* _alloc_iner_fifo(struct gxav_device *dev, void *buffer, unsigned int size)
{
	struct audiodec_fifo* fifo = gx_mallocz(sizeof(struct audiodec_fifo));

	if (fifo == NULL)
		return NULL;

	fifo->channel = gxav_channel_apply(dev, buffer, size, GXAV_PTS_FIFO);
	if (fifo->channel == NULL) {
		gx_free(fifo);
		gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return NULL;
	}

	gxav_channel_get_phys(fifo->channel,
			&(fifo->buffer_start_addr), &(fifo->buffer_end_addr), &(fifo->buffer_id));
	fifo->pts_start_addr = (unsigned int) fifo->channel->pts_buffer;
	fifo->pts_end_addr   = (unsigned int)(fifo->channel->pts_buffer + fifo->channel->pts_size -1);
	fifo->pts_buffer_id  = fifo->channel->pts_channel_id + 1;
	fifo->channel_id     = gxav_channel_id_get    (fifo->channel);
	fifo->channel_pts_id = gxav_channel_pts_id_get(fifo->channel);

	return fifo;
}

static int gx3201_audiodec_link_fifo(struct gxav_audiodec *audiodec, struct audiodec_fifo *fifo)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	if (fifo == NULL) {
		gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (GXAV_PIN_INPUT == fifo->direction) {
		if (GXAV_PIN_ESA == fifo->pin_id) {
			unsigned int move_type = MOVE_128KHZ;
			struct gxav_audio_syncinfo* audio_syncinfo = audiodec->syncinfo;

			if (audiodec->dec_type == CODEC_DTS) {
				if (audiodec->dec_mode == BYPASS_MODE) {
					fifo->channel->indata     = audiodec;
					fifo->channel->incallback = bypass_writed_callback;
					_config_bypass_input_fifo(audiodec, fifo);
					OR_SET_SPD_BUFFER_START_ADDR(audiodec_reg, audiodec->bypass_esa_start_addr);
					OR_SET_SPD_BUFFER_SIZE      (audiodec_reg, audiodec->bypass_esa_size);
					audio_syncinfo->movetype = GX_AUDIO_GET_ENDIAN_32(move_type);
				}
			} else if ((audiodec->dec_type == CODEC_AC3) || (audiodec->dec_type == CODEC_EAC3)) {
				if (audiodec->dec_mode == BYPASS_MODE) {
					fifo->channel->indata     = audiodec;
					fifo->channel->incallback = bypass_writed_callback;
					_config_bypass_input_fifo(audiodec, fifo);
					OR_SET_SPD_BUFFER_START_ADDR(audiodec_reg, audiodec->bypass_esa_start_addr);
					OR_SET_SPD_BUFFER_SIZE      (audiodec_reg, audiodec->bypass_esa_size);
					audio_syncinfo->movetype = GX_AUDIO_GET_ENDIAN_32(move_type);
				} else if (audiodec->dec_mode == DECODE_MODE) {
					fifo->channel->indata     = audiodec;
					fifo->channel->incallback = decode_writed_callback;
					_config_decode_input_fifo(audiodec, fifo);
					OR_SET_NORMAL_ESA_BUFFER_START_ADDR(audiodec_reg, audiodec->decode_esa_start_addr);
					OR_SET_NORMAL_ESA_BUFFER_SIZE      (audiodec_reg, audiodec->decode_esa_size);
				} else if (audiodec->dec_mode == (BYPASS_MODE|DECODE_MODE)) {
					fifo->channel->indata     = audiodec;
					fifo->channel->incallback = bypass_writed_callback;
					_config_bypass_input_fifo(audiodec, fifo);
					OR_SET_SPD_BUFFER_START_ADDR(audiodec_reg, audiodec->bypass_esa_start_addr);
					OR_SET_SPD_BUFFER_SIZE      (audiodec_reg, audiodec->bypass_esa_size);

					audiodec->decode_fifo = _alloc_iner_fifo(fifo->channel->dev,
							(void *)audiodec->decode_channel_buffer, audiodec->decode_channel_size);
					if (audiodec->decode_fifo == NULL) {
						audiodec->dec_mode = BYPASS_MODE;
						return -1;
					}
					audiodec->decode_fifo->channel->outdata     = audiodec;
					audiodec->decode_fifo->channel->outcallback = bypass_readed_callback;
					audiodec->decode_fifo->channel->indata     = audiodec;
					audiodec->decode_fifo->channel->incallback = decode_writed_callback;
					audiodec->decode_fifo_num++;
					_config_decode_input_fifo(audiodec, audiodec->decode_fifo);
					OR_SET_NORMAL_ESA_BUFFER_START_ADDR(audiodec_reg, audiodec->decode_esa_start_addr);
					OR_SET_NORMAL_ESA_BUFFER_SIZE      (audiodec_reg, audiodec->decode_esa_size);

					audio_syncinfo->movetype = GX_AUDIO_GET_ENDIAN_32(MOVE_ALL);
				}
			} else if (audiodec->dec_type == CODEC_MPEG4_AAC) {
				fifo->channel->indata     = audiodec;
				fifo->channel->incallback = decode_writed_callback;
				_config_decode_input_fifo(audiodec, fifo);
				OR_SET_NORMAL_ESA_BUFFER_START_ADDR(audiodec_reg, audiodec->decode_esa_start_addr);
				OR_SET_NORMAL_ESA_BUFFER_SIZE      (audiodec_reg, audiodec->decode_esa_size);

				if ((audiodec->dec_mode & CONVERT_MODE) == CONVERT_MODE) {
					audiodec->convert_fifo = _alloc_iner_fifo(fifo->channel->dev,
							(void *)audiodec->convert_channel_buffer, audiodec->convert_channel_size);
					if (audiodec->convert_fifo == NULL) {
						audiodec->dec_mode &= ~CONVERT_MODE;
						return -1;
					}
					audiodec->convert_fifo->channel->outdata     = audiodec;
					audiodec->convert_fifo->channel->outcallback = decode_readed_callback;
					audiodec->convert_fifo->channel->indata      = audiodec;
					audiodec->convert_fifo->channel->incallback  = convert_writed_callback;
					_config_convert_input_fifo(audiodec, audiodec->convert_fifo);
					OR_SET_SPD_BUFFER_START_ADDR(audiodec_reg, audiodec->convert_esa_start_addr);
					OR_SET_SPD_BUFFER_SIZE      (audiodec_reg, audiodec->convert_esa_size);
				}
			} else if (audiodec->dec_type == CODEC_PCM) {
				fifo->channel->indata     = audiodec;
				fifo->channel->incallback = decode_writed_callback;
				_config_decode_input_fifo(audiodec, fifo);
				OR_SET_NORMAL_ESA_BUFFER_START_ADDR(audiodec_reg, audiodec->decode_esa_start_addr);
				OR_SET_NORMAL_ESA_BUFFER_SIZE      (audiodec_reg, audiodec->decode_esa_size);
			} else if ((audiodec->dec_type == CODEC_RA_AAC)||(audiodec->dec_type == CODEC_RA_RA8LBR)) {
				fifo->channel->indata     = audiodec;
				fifo->channel->incallback = decode_writed_callback;
				_config_decode_input_fifo(audiodec, fifo);
				OR_SET_COM_SOURCE_BUFFER_START_ADDR(audiodec_reg, audiodec->decode_pts_writed_addr&0x1fffffff);
				OR_SET_COM_SOURCE_BUFFER_SIZE      (audiodec_reg, audiodec->decode_pts_size);
				OR_SET_NORMAL_ESA_BUFFER_START_ADDR(audiodec_reg, audiodec->decode_esa_start_addr);
				OR_SET_NORMAL_ESA_BUFFER_SIZE      (audiodec_reg, audiodec->decode_esa_size);
			} else {
				fifo->channel->indata     = audiodec;
				fifo->channel->incallback = decode_writed_callback;
				_config_decode_input_fifo(audiodec, fifo);
				OR_SET_NORMAL_ESA_BUFFER_START_ADDR(audiodec_reg, audiodec->decode_esa_start_addr);
				OR_SET_NORMAL_ESA_BUFFER_SIZE      (audiodec_reg, audiodec->decode_esa_size);
			}
		} else if (GXAV_PIN_ADESA == fifo->pin_id) {
			struct audiodec_regs* addec_reg = audiodec->addec_reg;

			fifo->channel->indata     = audiodec;
			fifo->channel->incallback = decode_writed_callback;
			_config_decode_input_fifo(audiodec, fifo);
			OR_SET_NORMAL_ESA_BUFFER_START_ADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32(audiodec->decode_esa_start_addr));
			OR_SET_NORMAL_ESA_BUFFER_SIZE      (addec_reg, GX_AUDIO_GET_ENDIAN_32(audiodec->decode_esa_size));
		} else {
			gxlog_e(LOG_ADEC, "%s %d [pin_id: %d]\n", __func__, __LINE__, fifo->pin_id);
			return -1;
		}
	} else if (GXAV_PIN_OUTPUT == fifo->direction) {
		if (GXAV_PIN_PCM == fifo->pin_id) {
			unsigned int pcm_start_addr = fifo->buffer_start_addr;
			unsigned int pcm_end_addr   = fifo->buffer_end_addr;
			unsigned int pcm_buf_size   = (pcm_end_addr-pcm_start_addr+1);
			unsigned char pcm_buf_id    = fifo->channel_id;

			if (pcmbuffer_protect_flag) {
				OR_SET_MCU_PROTECT_PCMBUF_S_ADDR(audiodec_reg, pcm_start_addr);
				OR_SET_MCU_PROTECT_PCMBUF_SIZE  (audiodec_reg, pcm_buf_size);
				gxav_firewall_register_buffer(GXAV_BUFFER_AUDIO_FRAME, pcm_start_addr, pcm_buf_size);
			}

			OR_SET_PCM_MODE_NON_INTERLACE(audiodec_reg);
			pcm_buf_size = pcm_buf_size/audiodec->max_channel_num;

			audiodec->out_pcm_id         = pcm_buf_id;
			audiodec->out_pcm_size       = pcm_buf_size;
			audiodec->out_pcm_start_addr = pcm_start_addr;
			audiodec->out_pcm_writed_addr= 0;
			gxav_sdc_buffer_reset(pcm_buf_id);

			OR_SET_PCM_BUFFER_START_ADDR(audiodec_reg, pcm_start_addr);
			OR_SET_PCM_BUFFER_SIZE(audiodec_reg, pcm_buf_size);
			fifo->channel->outdata     = audiodec;
			fifo->channel->outcallback = decode_readed_callback;
		} else if(GXAV_PIN_AC3 == fifo->pin_id) {
			if (audiodec->dec_type == CODEC_AC3) {
				struct gxav_audio_syncinfo* audio_syncinfo = audiodec->syncinfo;
				unsigned int bypass_start_addr = fifo->buffer_start_addr;
				unsigned int bypass_end_addr   = fifo->buffer_end_addr;
				unsigned int bypass_buf_size   = (bypass_end_addr-bypass_start_addr+1);
				unsigned char bypass_buf_id    = fifo->channel_id;

				gxav_sdc_buffer_reset(bypass_buf_id);
				audiodec->out_bypass_id               = bypass_buf_id;
				audiodec->out_bypass_size             = bypass_buf_size;
				audio_syncinfo->out_bypass_id         = bypass_buf_id;
				audio_syncinfo->out_bypass_start_addr = GX_AUDIO_GET_ENDIAN_32(bypass_start_addr);
				audio_syncinfo->out_bypass_size       = GX_AUDIO_GET_ENDIAN_32(bypass_buf_size);

				fifo->channel->outdata     = audiodec;
				fifo->channel->outcallback = bypass_readed_callback;
			} else if (audiodec->dec_type == CODEC_EAC3) {
				unsigned int degrade_start_addr = fifo->buffer_start_addr;
				unsigned int degrade_end_addr   = fifo->buffer_end_addr;
				unsigned int degrade_buf_size   = (degrade_end_addr-degrade_start_addr+1);
				unsigned char degrade_buf_id    = fifo->channel_id;

				audiodec->out_degrade_id         = degrade_buf_id;
				audiodec->out_degrade_start_addr = degrade_start_addr;
				audiodec->out_degrade_size       = degrade_buf_size;
				audiodec->out_degrade_writed_addr= 0;
				OR_SET_COM_TARGET_BUFFER_START_ADDR(audiodec_reg, degrade_start_addr);
				OR_SET_COM_TARGET_BUFFER_SIZE(audiodec_reg, degrade_buf_size);

				fifo->channel->outdata     = audiodec;
				fifo->channel->outcallback = decode_readed_callback;
			} else if (audiodec->dec_type == CODEC_MPEG4_AAC) {
				unsigned int convert_start_addr = fifo->buffer_start_addr;
				unsigned int convert_end_addr   = fifo->buffer_end_addr;
				unsigned int convert_buf_size   = (convert_end_addr - convert_start_addr + 1);
				unsigned char convert_buf_id    = fifo->channel_id;

				gxav_sdc_buffer_reset(convert_buf_id);

				audiodec->out_convert_id          = convert_buf_id;
				audiodec->out_convert_start_addr  = convert_start_addr;
				audiodec->out_convert_size        = convert_buf_size;
				audiodec->out_convert_writed_addr = 0;
				OR_SET_COM_TARGET_BUFFER_START_ADDR(audiodec_reg, convert_start_addr);
				OR_SET_COM_TARGET_BUFFER_SIZE(audiodec_reg, convert_buf_size);

				fifo->channel->outdata     = audiodec;
				fifo->channel->outcallback = convert_readed_callback;
			}
		} else if(GXAV_PIN_EAC3 == fifo->pin_id || GXAV_PIN_DTS == fifo->pin_id) {
			if (audiodec->dec_type == CODEC_EAC3 || audiodec->dec_type == CODEC_DTS) {
				struct gxav_audio_syncinfo* audio_syncinfo = audiodec->syncinfo;
				unsigned int bypass_start_addr = fifo->buffer_start_addr;
				unsigned int bypass_end_addr   = fifo->buffer_end_addr;
				unsigned int bypass_buf_size   = (bypass_end_addr-bypass_start_addr+1);
				unsigned char bypass_buf_id    = fifo->channel_id;

				gxav_sdc_buffer_reset(bypass_buf_id);
				audiodec->out_bypass_id               = bypass_buf_id;
				audiodec->out_bypass_size             = bypass_buf_size;
				audio_syncinfo->out_bypass_id         = bypass_buf_id;
				audio_syncinfo->out_bypass_start_addr = GX_AUDIO_GET_ENDIAN_32(bypass_start_addr);
				audio_syncinfo->out_bypass_size       = GX_AUDIO_GET_ENDIAN_32(bypass_buf_size);

				fifo->channel->outdata     = audiodec;
				fifo->channel->outcallback = bypass_readed_callback;
			}
		} else if(GXAV_PIN_ADPCM == fifo->pin_id) {
			struct audiodec_regs* addec_reg = audiodec->addec_reg;
			unsigned int pcm_start_addr = fifo->buffer_start_addr;
			unsigned int pcm_end_addr   = fifo->buffer_end_addr;
			unsigned int pcm_buf_size   = (pcm_end_addr-pcm_start_addr+1);
			unsigned char pcm_buf_id    = fifo->channel_id;

			OR_SET_PCM_MODE_NON_INTERLACE(addec_reg);
			pcm_buf_size = pcm_buf_size/audiodec->max_channel_num;

			audiodec->out_pcm_id         = pcm_buf_id;
			audiodec->out_pcm_start_addr = pcm_start_addr;
			audiodec->out_pcm_size       = pcm_buf_size;
			audiodec->out_pcm_writed_addr= 0;
			OR_SET_PCM_BUFFER_START_ADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32(pcm_start_addr));
			OR_SET_PCM_BUFFER_SIZE(addec_reg, GX_AUDIO_GET_ENDIAN_32(pcm_buf_size));
			fifo->channel->outdata     = audiodec;
			fifo->channel->outcallback = decode_readed_callback;
		} else if (GXAV_PIN_AAC == fifo->pin_id) {
			struct gxav_audio_syncinfo* audio_syncinfo = audiodec->syncinfo;
			unsigned int bypass_start_addr = fifo->buffer_start_addr;
			unsigned int bypass_end_addr   = fifo->buffer_end_addr;
			unsigned int bypass_buf_size   = (bypass_end_addr - bypass_start_addr + 1);
			unsigned char bypass_buf_id    = fifo->channel_id;

			gxav_sdc_buffer_reset(bypass_buf_id);
			audiodec->out_bypass_id               = bypass_buf_id;
			audiodec->out_bypass_size             = bypass_buf_size;
			audio_syncinfo->out_bypass_id         = bypass_buf_id;
			audio_syncinfo->out_bypass_start_addr = GX_AUDIO_GET_ENDIAN_32(bypass_start_addr);
			audio_syncinfo->out_bypass_size       = GX_AUDIO_GET_ENDIAN_32(bypass_buf_size);
			fifo->channel->outdata     = audiodec;
			fifo->channel->outcallback = decode_readed_callback;
		} else {
			gxlog_e(LOG_ADEC, "%s %d [pin_id: %d]\n", __func__, __LINE__, fifo->pin_id);
			return -1;
		}
	}
	else {
		gxlog_e(LOG_ADEC, "%s %d [direction %d]\n", __func__, __LINE__, fifo->direction);
		return -1;
	}

	return 0;
}

static int gx3201_audiodec_start(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	unsigned char id = audiodec->dec_id;

	if(audiodec->dec_state != AUDIODEC_INITED){
		gxlog_e(LOG_ADEC, "%s %d [state %d]\n", __func__, __LINE__, audiodec->dec_state);
		return -1;
	}

	gxlog_d(LOG_ADEC, "sys  reg addr: 0x%p 0x%x\n",
			audio_dec[id].reg, audio_dec[id].reg_base_addr);
	gxlog_d(LOG_ADEC, "ctrl reg addr: 0x%p 0x%x\n",
			audio_dec[id].dec_params, gx_virt_to_phys((unsigned int)audio_dec[id].dec_params));
	gxlog_d(LOG_ADEC, "sync reg addr: 0x%p 0x%x\n",
			audio_dec[id].sync_info,  gx_virt_to_phys((unsigned int)audio_dec[id].sync_info));
	gxlog_d(LOG_ADEC, "conv reg addr: 0x%x 0x%x\n",
			audio_dec[id].conv_info,  gx_virt_to_phys((unsigned int)audio_dec[id].conv_info));

	gx_dcache_clean_range(audiodec->regmem_virt_addr, audiodec->regmem_virt_addr + audiodec->regmem_virt_size);
	if(audiodec->dec_id == 0){
		OR_SET_AUDIO_PRINT(audiodec_reg, debug_adec_orprint);
		OR_SET_SPD_DATA_EMPTY(audiodec_reg, 0);//clr data empty
		OR_SET_NORMAL_DATA_EMPTY(audiodec_reg, 0);
		OR_CLR_ALL_INT_STATUS(audiodec_reg);
		OR_SET_MCU_HW_INT_ENABLE(audiodec_reg);
		OR_CLR_CPU_RST_OR(audiodec_reg);
		OR_SET_DEC_START(audiodec_reg);
		while(!(OR_GET_OR_INIT_OK_INT_STATUS(audiodec_reg))){
			if (audiodec->firmware_id == GXAV_FMID_AUDIO_ALL) {
				if (audiodec->all_firmware_supoort_dolby &&
						!audiodec->all_firmware_test_dolby) {
					unsigned int ret0 = OR_GET_OTP_ERR_INT_STATUS0(audiodec_reg);
					if (ret0 == 1) {
						struct audiodec_params_info *params = (struct audiodec_params_info *)(audiodec->dec_params);
						gxlog_e(LOG_ADEC,
								"%s %d [0x%x] [0x%x] [ALL-DOLBY firmware, chip not support]\n",
								__func__, __LINE__, params->or_key_type, params->or_chip_type);
						return -1;
					}
				}
			}
			gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		}
		OR_SET_MCU_HW_INT_DISABLE(audiodec_reg);

#ifdef CHECK_AUDIODEC_WORK_BUF
		_check_audiodec_firmware(audiodec, __FUNCTION__, __LINE__);
#endif
		audiodec->decode_frame_num    = 1;
		audiodec->decode_frame_param  = 0;
		audiodec->decode_restart_continue = DEC_RESTART;
		audiodec->decode_frame_cnt    = 0;
		audiodec->bypass_frame_cnt    = 0;
		audiodec->convert_frame_cnt   = 0;
		audiodec->dec_state = AUDIODEC_READY;
		audiodec->dec_errno = AUDIODEC_ERR_NONE;

		if((audiodec->dec_mode & DECODE_MODE) == DECODE_MODE){
			OR_INT_OTP_ERR_ENABLE(audiodec_reg);
			OR_INT_DECODE_ERR_ENABLE(audiodec_reg);
			OR_INT_DECODE_FINISH_ENABLE(audiodec_reg);
			OR_INT_NORMAL_NEED_DATA_ENABLE(audiodec_reg);
			OR_INT_NORMAL_DATA_EMPTY_FINISH_ENABLE(audiodec_reg);
			OR_INT_NORMAL_SYNC_FINISH_ENABLE(audiodec_reg);
			_start_decode_coding_data(audiodec);
		}

		if((audiodec->dec_type != CODEC_MPEG4_AAC) &&
				((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE)){
			OR_INT_SPD_PARSE_DONE_ENABLE(audiodec_reg);
			OR_INT_SPD_NEED_DATA_ENABLE(audiodec_reg);
			OR_INT_SPD_DATA_EMPTY_FINISH_ENABLE(audiodec_reg);
			OR_INT_SPD_SYNC_FINISH_ENABLE(audiodec_reg);
			_start_bypass_coding_data(audiodec);
		}

		if((audiodec->dec_type == CODEC_MPEG4_AAC) &&
				(audiodec->dec_mode & CONVERT_MODE) == CONVERT_MODE){
			OR_INT_CONVERT_FINISH_ENABLE(audiodec_reg);
			OR_INT_CONVERT_NEED_DATA_ENABLE(audiodec_reg);
			OR_INT_CONVERT_DATA_EMPTY_FINISH_ENABLE(audiodec_reg);
			_start_convert_coding_data(audiodec);
		}

	}else if(audiodec->dec_id == 1){
		audiodec->decode_restart_continue = DEC_RESTART;
		audiodec->decode_frame_cnt    = 0;
		audiodec->dec_state = AUDIODEC_READY;
		audiodec->dec_errno = AUDIODEC_ERR_NONE;

		OR_SET_AUDIO2_DATA_EMPTY(audiodec_reg, 0);
		OR_INT_AUDIO2_DECODE_FINISH_ENABLE(audiodec_reg);
		OR_INT_AUDIO2_NEED_DATA_ENABLE(audiodec_reg);
		OR_INT_AUDIO2_DATA_EMPTY_FINISH_ENABLE(audiodec_reg);
		OR_INT_AUDIO2_SYNC_FINISH_ENABLE(audiodec_reg);
		if (audiodec->stream_type == STREAM_TS) {
			struct audiodec_regs* addec_reg = audiodec->addec_reg;

			GX_AUDIO_SET_VAL_E(&(addec_reg->audio_mcu_stream_pid),  audiodec->stream_pid);
			GX_AUDIO_SET_VAL_E(&(addec_reg->audio_mcu_stream_type), 1);
		} else {
			struct audiodec_regs* addec_reg = audiodec->addec_reg;
			GX_AUDIO_SET_VAL_E(&(addec_reg->audio_mcu_stream_type), 0);
		}
		_start_decode_coding_data(audiodec);
	}
	return 0;
}

static int gx3201_audiodec_pause(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	if ((audiodec->dec_state == AUDIODEC_RUNNING) ||
			(audiodec->dec_state == AUDIODEC_READY)) {
		audiodec->dec_state = AUDIODEC_PAUSED;
		audiodec->dec_errno = AUDIODEC_ERR_NONE;

		if(audiodec->dec_id == 0){
			OR_INT_SPD_PARSE_DONE_DISABLE(audiodec_reg);
			OR_INT_SPD_NEED_DATA_DISABLE(audiodec_reg);
			OR_INT_SPD_DATA_EMPTY_FINISH_DISABLE(audiodec_reg);
			OR_INT_SPD_SYNC_FINISH_DISABLE(audiodec_reg);
			OR_INT_DECODE_FINISH_DISABLE(audiodec_reg);
			OR_INT_NORMAL_NEED_DATA_DISABLE(audiodec_reg);
			OR_INT_NORMAL_DATA_EMPTY_FINISH_DISABLE(audiodec_reg);
			OR_INT_NORMAL_SYNC_FINISH_DISABLE(audiodec_reg);
		}else if(audiodec->dec_id == 1){
			OR_INT_AUDIO2_DECODE_FINISH_DISABLE(audiodec_reg);
			OR_INT_AUDIO2_NEED_DATA_DISABLE(audiodec_reg);
			OR_INT_AUDIO2_DATA_EMPTY_FINISH_DISABLE(audiodec_reg);
			OR_INT_AUDIO2_SYNC_FINISH_DISABLE(audiodec_reg);
		}
	}
	return 0;
}

static int gx3201_audiodec_resume(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	if ((audiodec->dec_state == AUDIODEC_PAUSED) ||
			(audiodec->dec_state == AUDIODEC_READY)) {
		audiodec->dec_state = AUDIODEC_RUNNING;
		audiodec->dec_errno = AUDIODEC_ERR_NONE;

		if(audiodec->dec_id == 0){
			OR_INT_SPD_PARSE_DONE_ENABLE(audiodec_reg);
			OR_INT_SPD_NEED_DATA_ENABLE(audiodec_reg);
			OR_INT_SPD_DATA_EMPTY_FINISH_ENABLE(audiodec_reg);
			OR_INT_SPD_SYNC_FINISH_ENABLE(audiodec_reg);
			OR_INT_DECODE_FINISH_ENABLE(audiodec_reg);
			OR_INT_NORMAL_NEED_DATA_ENABLE(audiodec_reg);
			OR_INT_NORMAL_DATA_EMPTY_FINISH_ENABLE(audiodec_reg);
			OR_INT_NORMAL_SYNC_FINISH_ENABLE(audiodec_reg);
			if (audiodec->decode_esa_empty) {
				if (PCM_FILL_DATA == audiodec->decode_data_operate)
					_fill_decode_coding_data(audiodec);
				else if (PCM_DEC_FRAME == audiodec->decode_data_operate)
					_start_decode_coding_data(audiodec);
			}
			if (audiodec->bypass_esa_empty) {
				if (SPD_FILL_DATA == audiodec->bypass_data_operate)
					_fill_bypass_coding_data(audiodec);
				else if (SPD_PARSE_FRAME == audiodec->bypass_data_operate)
					_start_bypass_coding_data(audiodec);
			}
			if (audiodec->convert_esa_empty) {
				if (CVT_FILL_DATA == audiodec->convert_data_operate)
					_fill_convert_coding_data(audiodec);
				else if (CVT_ENC_FRAME == audiodec->convert_data_operate)
					_start_convert_coding_data(audiodec);
			}
		}else if(audiodec->dec_id == 1){
			OR_INT_AUDIO2_DECODE_FINISH_ENABLE(audiodec_reg);
			OR_INT_AUDIO2_NEED_DATA_ENABLE(audiodec_reg);
			OR_INT_AUDIO2_DATA_EMPTY_FINISH_ENABLE(audiodec_reg);
			OR_INT_AUDIO2_SYNC_FINISH_ENABLE(audiodec_reg);
			if (audiodec->decode_esa_empty) {
				if (PCM_FILL_DATA == audiodec->decode_data_operate)
					_fill_decode_coding_data(audiodec);
				else if (PCM_DEC_FRAME == audiodec->decode_data_operate)
					_start_decode_coding_data(audiodec);
			}
		}
	}
	return 0;
}

static int gx3201_audiodec_interrupt(struct gxav_audiodec *audiodec)
{
	int ret = 0;
	unsigned int dec_int    = 0;
	unsigned int dec_int_en = 0;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	dec_int    = OR_GET_INT_STATUS(audiodec_reg);
	dec_int_en = OR_GET_INT_EN_VAL(audiodec_reg);

	if (dec_int & dec_int_en & 0x17f104f) {
		struct gxav_audiodec *audiodec0 = gxav_audiodec_find_instance(0);

		if (audiodec0) {
			if ((audiodec0->dec_state != AUDIODEC_READY) &&
					(audiodec0->dec_state != AUDIODEC_RUNNING) &&
					(audiodec0->dec_state != AUDIODEC_PAUSED)) {
				OR_CLR_ALL_INT_STATUS(audiodec_reg);
				return ret;
			}

			if (dec_int & dec_int_en & 0x40) {
				struct audiodec_params_info *params = (struct audiodec_params_info *)(audiodec->dec_params);

				OR_INT_OTP_ERR_DISABLE(audiodec_reg);
				OR_CLR_OTP_ERR_INT_STATUS0(audiodec_reg);
				ret |= EVENT_AUDIO_DECODE_ERROR;
				audiodec0->dec_state = AUDIODEC_RUNNING;
				audiodec0->dec_errno = AUDIODEC_ERR_UNSUPPORT_CODECTYPE;
				gx_dcache_inv_range(audiodec->regmem_virt_addr,
						audiodec->regmem_virt_addr + audiodec->regmem_virt_size);
				gxlog_i(LOG_ADEC, "[%x, %x]\n", params->or_key_type, params->or_chip_type);
			}

			if (dec_int & dec_int_en & 0x1000000) {
				OR_INT_DECODE_ERR_DISABLE(audiodec_reg);
				ret |= EVENT_AUDIO_DECODE_ERROR;
				audiodec0->dec_state = AUDIODEC_RUNNING;
				audiodec0->dec_errno = AUDIODEC_ERR_UNSUPPORT_CODECTYPE;
				if ((audiodec0->dec_mode & DECODE_MODE) == DECODE_MODE) {
					GxAvAudioDataEvent data_event = 0;

					data_event |= PCM_ERROR;
					if (audiodec->dec_type == CODEC_EAC3)
						data_event |= AC3_ERROR;
					gxav_audioout_set_event(audiodec->audioout_id, data_event);
				}
			}

			if (dec_int & dec_int_en & (0x1000))
				ret |= decode_key_complete_isr(audiodec0);

			if (dec_int & dec_int_en & 0x1)
				ret |= decode_dec_complete_isr(audiodec0);
			if (dec_int & dec_int_en & 0x2)
				ret |= decode_need_data_isr(audiodec0);
			if (dec_int & dec_int_en & 0x4)
				ret |= decode_finish_dec_isr(audiodec0);
			if (dec_int & dec_int_en & 0x8)
				ret |= decode_find_header_isr(audiodec0);

			if (dec_int & dec_int_en & 0x10000)
				ret |= bypass_complete_parse_isr(audiodec0);
			if (dec_int & dec_int_en & 0x20000)
				ret |= bypass_need_data_isr(audiodec0);
			if (dec_int & dec_int_en & 0x40000)
				ret |= bypass_finish_dec_isr(audiodec0);
			if (dec_int & dec_int_en & 0x80000)
				ret |= bypass_find_header_isr(audiodec0);

			if (dec_int & dec_int_en & 0x100000)
				ret |= convert_complete_parse_isr(audiodec0);
			if (dec_int & dec_int_en & 0x200000)
				ret |= convert_need_data_isr(audiodec0);
			if (dec_int & dec_int_en & 0x400000)
				ret |= convert_finish_enc_isr(audiodec0);
		}
	}

	if (dec_int & dec_int_en & 0xf00) {
		struct gxav_audiodec* audiodec1 = gxav_audiodec_find_instance(1);
		if (audiodec1) {
			if ((audiodec1->dec_state != AUDIODEC_READY) &&
					(audiodec1->dec_state != AUDIODEC_RUNNING) &&
					(audiodec1->dec_state != AUDIODEC_PAUSED)) {
				return ret;
			}

			if (dec_int & dec_int_en & 0x100)
				ret = decode_dec_complete_isr(audiodec1);
			if (dec_int & dec_int_en & 0x200)
				ret = decode_need_data_isr(audiodec1);
			if (dec_int & dec_int_en & 0x400)
				ret = decode_finish_dec_isr(audiodec1);
			if (dec_int & dec_int_en & 0x800)
				ret = decode_find_header_isr(audiodec1);
		}
	}

	return ret;
}

static int gx3201_audiodec_stop(struct gxav_audiodec *audiodec)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;
	int times;

	if((audiodec->dec_state != AUDIODEC_STOPED) &&
			(audiodec->dec_state != AUDIODEC_INITED))
	{
		audiodec->dec_state = AUDIODEC_STOPED;

		if(audiodec->dec_id == 0){
			OR_INT_DECODE_FINISH_DISABLE(audiodec_reg);
			OR_INT_NORMAL_NEED_DATA_DISABLE(audiodec_reg);
			OR_INT_NORMAL_DATA_EMPTY_FINISH_DISABLE(audiodec_reg);
			OR_INT_NORMAL_SYNC_FINISH_DISABLE(audiodec_reg);

			OR_INT_SPD_PARSE_DONE_DISABLE(audiodec_reg);
			OR_INT_SPD_NEED_DATA_DISABLE(audiodec_reg);
			OR_INT_SPD_DATA_EMPTY_FINISH_DISABLE(audiodec_reg);
			OR_INT_SPD_SYNC_FINISH_DISABLE(audiodec_reg);
			OR_INT_DECODE_KEY_DISABLE(audiodec_reg);

			OR_CLR_OR_INIT_OK_INT_STATUS(audiodec_reg);
#ifdef CHECK_AUDIODEC_WORK_BUF
			_check_audiodec_firmware(audiodec, __FUNCTION__, __LINE__);
#endif
			OR_SET_MCU_HW_RST_ENABLE(audiodec_reg);
			OR_SET_CPU_RST_OR(audiodec_reg);
			times = 0;
			while (times++ < 100) {//解决音频固件总线死机问题
				int epc    = REG_GET_VAL(&(audiodec_reg->audio_mcu_expc));
				int status = OR_GET_OR_RST_OK_INT_STATUS(audiodec_reg);

				if(status || (epc == 0))
					break;
				gxlog_d(LOG_ADEC,
						"%s %d [try: %d status: %d epc:0x%x]\n",
						__func__, __LINE__, times, status, epc);
				gx_msleep(5);
			}
			OR_SET_MCU_HW_RST_DISABLE(audiodec_reg);
#ifdef CHECK_AUDIODEC_WORK_BUF
			_check_audiodec_firmware(audiodec, __FUNCTION__, __LINE__);
#endif

			OR_SET_DECODE_TASK(audiodec_reg, IDLE);
			OR_SET_DEC_RESET(audiodec_reg);
			OR_SET_SPD_DATA_EMPTY(audiodec_reg, 0);//clr data empty
			OR_SET_NORMAL_DATA_EMPTY(audiodec_reg, 0);
			if(audiodec->pcm_fifo)
				gxfifo_reset(audiodec->pcm_fifo);
			if(audiodec->bypass_fifo)
				gxfifo_reset(audiodec->bypass_fifo);
			if(audiodec->degrade_fifo)
				gxfifo_reset(audiodec->degrade_fifo);
			if(audiodec->encode_fifo)
				gxfifo_reset(audiodec->encode_fifo);
		}else if(audiodec->dec_id == 1){
			OR_INT_AUDIO2_DECODE_FINISH_DISABLE(audiodec_reg);
			OR_INT_AUDIO2_NEED_DATA_DISABLE(audiodec_reg);
			OR_INT_AUDIO2_DATA_EMPTY_FINISH_DISABLE(audiodec_reg);
			OR_INT_AUDIO2_SYNC_FINISH_DISABLE(audiodec_reg);

			OR_SET_DECODE_TASK(audiodec_reg, IDLE);
			OR_SET_AUDIO2_DATA_EMPTY(audiodec_reg, 0);//clr data empty
			if(audiodec->pcm_fifo)
				gxfifo_reset(audiodec->pcm_fifo);
			if(audiodec->ctrlinfo_fifo)
				gxfifo_reset(audiodec->ctrlinfo_fifo);
		}
	}

	return 0;
}

static int gx3201_audiodec_unlink_fifo(struct gxav_audiodec *audiodec, struct audiodec_fifo *fifo)
{
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	if (fifo == NULL) {
		gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
	if (GXAV_PIN_INPUT == fifo->direction) {
		if(GXAV_PIN_ESA == fifo->pin_id) //esa
		{
			struct audiodec_fifo* decode_fifo  = audiodec->decode_fifo;
			struct audiodec_fifo* convert_fifo = audiodec->convert_fifo;
			OR_SET_NORMAL_ESA_BUFFER_START_ADDR(audiodec_reg, 0x0);
			OR_SET_NORMAL_ESA_BUFFER_SIZE(audiodec_reg, 0x0);
			audiodec->decode_esa_id = -1;
			audiodec->decode_pts_id = -1;

			OR_SET_SPD_BUFFER_START_ADDR(audiodec_reg, 0);
			OR_SET_SPD_BUFFER_SIZE(audiodec_reg, 0);
			audiodec->bypass_esa_id = -1;
			audiodec->bypass_pts_id = -1;

			if ((audiodec->dec_type == CODEC_RA_AAC)||(audiodec->dec_type == CODEC_RA_RA8LBR)) {
				OR_SET_COM_SOURCE_BUFFER_START_ADDR(audiodec_reg, 0x0);
				OR_SET_COM_SOURCE_BUFFER_SIZE(audiodec_reg, 0x0);
			}

			if(decode_fifo){
				if(decode_fifo->channel){
					gxav_channel_free(decode_fifo->channel->dev, decode_fifo->channel);
					decode_fifo->channel->incallback = NULL;
					decode_fifo->channel->indata = NULL;
					decode_fifo->channel->outcallback = NULL;
					decode_fifo->channel->outdata = NULL;
					decode_fifo->channel = NULL;
				}
				gx_free(decode_fifo);
				audiodec->decode_fifo = NULL;
				audiodec->decode_fifo_num--;
			}

			if(convert_fifo){
				if(convert_fifo->channel){
					gxav_channel_free(convert_fifo->channel->dev, convert_fifo->channel);
					convert_fifo->channel->incallback = NULL;
					convert_fifo->channel->indata = NULL;
					convert_fifo->channel->outcallback = NULL;
					convert_fifo->channel->outdata = NULL;
					convert_fifo->channel = NULL;
				}
				gx_free(convert_fifo);
				audiodec->convert_fifo = NULL;
			}
			fifo->channel->incallback = NULL;
			fifo->channel->indata = NULL;
		}
		else if(GXAV_PIN_ADESA == fifo->pin_id)
		{
			struct audiodec_regs* addec_reg = audiodec->addec_reg;

			OR_SET_NORMAL_ESA_BUFFER_START_ADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32(0x0));
			OR_SET_NORMAL_ESA_BUFFER_SIZE(addec_reg, GX_AUDIO_GET_ENDIAN_32(0x0));
			audiodec->decode_esa_id = -1;
			audiodec->decode_pts_id = -1;
			fifo->channel->incallback = NULL;
			fifo->channel->indata = NULL;
		}
		else{
			gxlog_e(LOG_ADEC, "%s %d [pin_id: %d]\n", __func__, __LINE__, fifo->pin_id);
			return -1;
		}
	}
	else if (GXAV_PIN_OUTPUT == fifo->direction) {
		if(GXAV_PIN_PCM == fifo->pin_id)//pcm
		{
			if (pcmbuffer_protect_flag) {
				OR_SET_MCU_PROTECT_PCMBUF_S_ADDR(audiodec_reg, 0x0);
				OR_SET_MCU_PROTECT_PCMBUF_SIZE  (audiodec_reg, 0x0);
			}

			OR_SET_PCM_BUFFER_START_ADDR(audiodec_reg, 0x0);
			OR_SET_PCM_BUFFER_SIZE(audiodec_reg, 0x0);
			audiodec->out_pcm_id = -1;
			fifo->channel->outcallback = NULL;
			fifo->channel->outdata = NULL;
		}else if(GXAV_PIN_AC3 == fifo->pin_id){
			if (audiodec->dec_type == CODEC_AC3) {
				struct gxav_audio_syncinfo* audio_syncinfo = audiodec->syncinfo;

				audiodec->out_bypass_id               = -1;
				audiodec->out_bypass_size             =  0;
				audio_syncinfo->out_bypass_id         = -1;
				audio_syncinfo->out_bypass_start_addr = GX_AUDIO_GET_ENDIAN_32(0x0);
				audio_syncinfo->out_bypass_size       = GX_AUDIO_GET_ENDIAN_32(0x0);
			} else if (audiodec->dec_type == CODEC_EAC3 ||
					audiodec->dec_type == CODEC_MPEG4_AAC) {
				OR_SET_COM_TARGET_BUFFER_START_ADDR(audiodec_reg, 0x0);
				OR_SET_COM_TARGET_BUFFER_SIZE(audiodec_reg, 0x0);
			}
		} else if(GXAV_PIN_EAC3 == fifo->pin_id || GXAV_PIN_DTS == fifo->pin_id) {
			if (audiodec->dec_type == CODEC_EAC3 || audiodec->dec_type == CODEC_DTS) {
				struct gxav_audio_syncinfo* audio_syncinfo = audiodec->syncinfo;

				audiodec->out_bypass_id               = -1;
				audiodec->out_bypass_size             =  0;
				audio_syncinfo->out_bypass_id         = -1;
				audio_syncinfo->out_bypass_start_addr = GX_AUDIO_GET_ENDIAN_32(0x0);
				audio_syncinfo->out_bypass_size       = GX_AUDIO_GET_ENDIAN_32(0x0);
			}
		}else if(GXAV_PIN_ADPCM == fifo->pin_id){
			struct audiodec_regs* addec_reg = audiodec->addec_reg;

			OR_SET_PCM_BUFFER_START_ADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32(0x0));
			OR_SET_PCM_BUFFER_SIZE(addec_reg, GX_AUDIO_GET_ENDIAN_32(0x0));
			audiodec->out_pcm_id = -1;
			fifo->channel->outcallback = NULL;
			fifo->channel->outdata = NULL;
		} else if (GXAV_PIN_AAC == fifo->pin_id) {
			struct gxav_audio_syncinfo* audio_syncinfo = audiodec->syncinfo;

			audiodec->out_bypass_id               = -1;
			audiodec->out_bypass_size             =  0;
			audio_syncinfo->out_bypass_id         = -1;
			audio_syncinfo->out_bypass_start_addr = GX_AUDIO_GET_ENDIAN_32(0x0);
			audio_syncinfo->out_bypass_size       = GX_AUDIO_GET_ENDIAN_32(0x0);
		}
	}
	else {
		gxlog_e(LOG_ADEC, "%s %d [pin_id: %d]\n", __func__, __LINE__, fifo->direction);
		return -1;
	}

	return 0;
}

static int gx3201_audiodec_write_ctrlinfo(struct gxav_audiodec* audiodec, GxAudioDecProperty_ContrlInfo* ctrlinfo)
{
	int len = 0;
	if(audiodec->ctrlinfo_fifo == NULL)
		return -1;

	ctrlinfo->ad_text_tag = 0x12345678;
	if(gxfifo_freelen(audiodec->ctrlinfo_fifo) >= sizeof(GxAudioDecProperty_ContrlInfo)){
		len = gxfifo_put(audiodec->ctrlinfo_fifo, ctrlinfo, sizeof(GxAudioDecProperty_ContrlInfo));
		return len;
	}

	return 0;
}

static int gx3201_audiodec_update(struct gxav_audiodec *audiodec)
{
	unsigned int esa_writed_addr = 0;
	struct audiodec_regs *audiodec_reg = audiodec->audiodec_reg;

	if (audiodec->dec_id == 0)
	{
		unsigned int decode_esa_len = 0, bypass_esa_len = 0;
		gxav_sdc_length_get(audiodec->decode_esa_id, &decode_esa_len);
		gxav_sdc_length_get(audiodec->bypass_esa_id, &bypass_esa_len);

		if ((audiodec->dec_type == CODEC_MPEG4_AAC)) {
			if((audiodec->dec_mode & DECODE_MODE) == DECODE_MODE){
				unsigned int decode_esa_size = audiodec->decode_esa_size;

				if ((decode_esa_len == 0) ||
						((decode_esa_len <= audiodec->enough_data) && (audiodec->decode_first_flag))) {
					_set_audiodec_finish(audiodec);
					audiodec->update = 1;
					return 0;
				}
				gxav_sdc_rwaddr_get(audiodec->decode_esa_id, NULL, &esa_writed_addr);
				audiodec->decode_esa_writed_addr = (decode_esa_size+esa_writed_addr-1)%decode_esa_size;
				OR_SET_NORMAL_ESA_W_ADDR(audiodec_reg, audiodec->decode_esa_writed_addr);
				OR_SET_NORMAL_DATA_EMPTY(audiodec_reg, 1);
			}
			// need convert
		} else {
			if ((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE) {
				unsigned int bypass_esa_size = audiodec->bypass_esa_size;

				if ((bypass_esa_len == 0) ||
						((bypass_esa_len <= audiodec->enough_data) && (audiodec->bypass_first_flag))) {
					_set_audiodec_finish(audiodec);
					audiodec->update = 1;
					return 0;
				}
				gxav_sdc_rwaddr_get(audiodec->bypass_esa_id, NULL, &esa_writed_addr);
				audiodec->bypass_esa_writed_addr = (bypass_esa_size+esa_writed_addr-1)%bypass_esa_size;
				OR_SET_SPD_ESA_W_ADDR(audiodec_reg, audiodec->bypass_esa_writed_addr);
				OR_SET_SPD_DATA_EMPTY(audiodec_reg, 1);
			} else if ((audiodec->dec_mode & DECODE_MODE) == DECODE_MODE) {
				unsigned int decode_esa_size = audiodec->decode_esa_size;

				if ((decode_esa_len == 0) ||
						((decode_esa_len <= audiodec->enough_data) && (audiodec->decode_first_flag))) {
					_set_audiodec_finish(audiodec);
					audiodec->update = 1;
					return 0;
				}
				gxav_sdc_rwaddr_get(audiodec->decode_esa_id, NULL, &esa_writed_addr);
				audiodec->decode_esa_writed_addr = (decode_esa_size+esa_writed_addr-1)%decode_esa_size;
				OR_SET_NORMAL_ESA_W_ADDR(audiodec_reg, audiodec->decode_esa_writed_addr);
				OR_SET_NORMAL_DATA_EMPTY(audiodec_reg, 1);
			}
		}
	} else if(audiodec->dec_id == 1){
		struct audiodec_regs *addec_reg = audiodec->addec_reg;
		unsigned int decode_esa_size = audiodec->decode_esa_size;

		gxav_sdc_rwaddr_get(audiodec->decode_esa_id, NULL, &esa_writed_addr);
		audiodec->decode_esa_writed_addr = (decode_esa_size+esa_writed_addr-1)%decode_esa_size;
		OR_SET_NORMAL_ESA_W_ADDR(addec_reg, GX_AUDIO_GET_ENDIAN_32(audiodec->decode_esa_writed_addr));
		OR_SET_AUDIO2_DATA_EMPTY(audiodec_reg, 1);
	}
	audiodec->update = 1;
	return 0;
}

static int gx3201_audiodec_get_errno(struct gxav_audiodec *audiodec, GxAudioDecProperty_Errno *err_no)
{
	err_no->err_no = audiodec->dec_errno;
	return 0;
}

static int gx3201_audiodec_get_state(struct gxav_audiodec *audiodec, GxAudioDecProperty_State *state)
{
	state->state    = audiodec->dec_state;
	state->err_code = audiodec->dec_errno;
	return 0;
}

static int gx3201_audiodec_get_frameinfo(struct gxav_audiodec *audiodec, GxAudioDecProperty_FrameInfo * frameinfo)
{
	if ((audiodec->dec_mode & DECODE_MODE) == DECODE_MODE) {
		frameinfo->data_type        = PCM_TYPE;
		frameinfo->channelnum       = audiodec->decode_channelnum;
		frameinfo->samplefre        = audiodec->decode_samplefre;
		frameinfo->bitrate          = audiodec->decode_bitrate;
		frameinfo->decode_frame_cnt = audiodec->decode_frame_cnt;
	} else if ((audiodec->dec_mode & BYPASS_MODE) == BYPASS_MODE) {
		frameinfo->data_type        = audiodec->dec_type;
		frameinfo->channelnum       = 0;
		frameinfo->samplefre        = audiodec->bypass_samplefre;
		frameinfo->bitrate          = audiodec->bypass_bitrate * 1000;
		frameinfo->decode_frame_cnt = audiodec->bypass_frame_cnt;
	}

	return 0;
}

static int gx3201_audiodec_support_ad(GxAudioDecProperty_SupportAD *sup)
{
	if (sup->mode == DECODE_MODE) {
		_scan_audiodec_firmware();
		sup->support = _support_audiodec_ad(sup->type);
	} else
		sup->support = 0;

	return 0;
}

static int gx3201_audiodec_get_cap(GxAudioDecProperty_Capability *cap)
{
	switch (cap->type) {
	case CODEC_MPEG12A:
	case CODEC_AVSA:
	case CODEC_DRA:
	case CODEC_WMA:
	case CODEC_VORBIS:
	case CODEC_FLAC:
	case CODEC_OPUS:
	case CODEC_SBC:
	case CODEC_PCM:
		cap->channelnum = 2;
		break;
	case CODEC_MPEG4_AAC:
	case CODEC_AC3:
	case CODEC_EAC3:
	case CODEC_DTS:
	case CODEC_RA_AAC:
	case CODEC_RA_RA8LBR:
		cap->channelnum = 6;
		break;
	default:
		gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
	cap->samplefre =
		(SAMPLEFRE_44KDOT1HZ | SAMPLEFRE_48KHZ | SAMPLEFRE_32KHZ |
		 SAMPLEFRE_22KDOT05HZ | SAMPLEFRE_24KHZ | SAMPLEFRE_96KHZ |
		 SAMPLEFRE_16KHZ | SAMPLEFRE_12KHZ | SAMPLEFRE_11KDOT025HZ |
		 SAMPLEFRE_9KDOT6HZ | SAMPLEFRE_8KHZ);

	if (workbuf_protect_addr) {
		cap->workbuf_size = 0;
	} else {
		_scan_audiodec_firmware();
		cap->workbuf_size = _get_audiodec_ormem_size() + AUDIODEC_EXTEND_REG_SIZE;
		if (firmware_protect_flag) {
			cap->workbuf_size += AUDIODEC_DECODE_FIFO_SIZE;
		} else {
			if ((cap->type == CODEC_AC3) || (cap->type == CODEC_EAC3)) {
				cap->workbuf_size += AUDIODEC_DECODE_FIFO_SIZE;
			} else if (cap->type == CODEC_MPEG4_AAC) {
				unsigned int fim_idx = 0;
				for (; fim_idx < sizeof(audio_fim)/sizeof(struct audio_firmware); fim_idx++) {
					if (audio_fim[fim_idx].firmware_id == GXAV_FMID_AUDIO_MPEG) {
						if (audio_fim[fim_idx].support_convert_code)
							cap->workbuf_size += AUDIODEC_CONVERT_FIFO_SIZE;
						break;
					}
				}
			}
		}
	}

	return 0;
}

static int gx3201_audiodec_get_frame(struct gxav_audiodec *audiodec, GxAudioDecProperty_FrameData* frame)
{
	return 0;
}

static int gx3201_audiodec_clr_frame(struct gxav_audiodec *audiodec, GxAudioDecProperty_FrameData* frame)
{
	return 0;
}

static int gx3201_audiodec_decode_key(struct gxav_audiodec *audiodec, GxAudioDecProperty_DecodeKey* key)
{
	struct audiodec_regs* audiodec_reg  = audiodec->audiodec_reg;
	struct audiodec_params_info *params = audiodec->dec_params;
	unsigned int offset = 0;

	if ((audiodec->dec_state == AUDIODEC_INITED) ||
			(audiodec->dec_state ==AUDIODEC_STOPED) ||
			(audiodec->dec_state ==AUDIODEC_ERROR) ||
			(audiodec->dec_state ==AUDIODEC_OVER))
		return -1;

	while(OR_GET_MCU_TASK_START_TO_WORK(audiodec_reg));

	gx_memcpy(params->scr_key+offset, key->map_key,   sizeof(key->map_key));
	offset += sizeof(key->map_key);
	gx_memcpy(params->scr_key+offset, key->rand_data, sizeof(key->rand_data));
	offset += sizeof(key->rand_data);
	gx_memcpy(params->scr_key+offset, key->map,     sizeof(key->map));
	offset += sizeof(key->map);
	gx_memcpy(params->scr_key+offset, key->chip_id, sizeof(key->chip_id));
	offset += sizeof(key->chip_id);
	gx_memcpy(params->scr_key+offset, key->scr_key, sizeof(key->scr_key));

	OR_INT_DECODE_KEY_ENABLE(audiodec_reg);
	OR_SET_DECODE_TASK(audiodec_reg, DECODE_KEY);
	OR_SET_MCU_TASK_START_TO_WORK(audiodec_reg);

	return 0;
}

static int gx3201_audiodec_get_key(struct gxav_audiodec *audiodec, GxAudioDecProperty_GetKey* key)
{
	struct audiodec_params_info *params = audiodec->dec_params;

	gx_memcpy(key->dec_key, params->dec_key, sizeof(key->dec_key));
	key->dec_result = GX_AUDIO_GET_ENDIAN_32(params->des_key_result);
	return 0;
}

static int gx3201_audiodec_boost_volume(struct gxav_audiodec *audiodec, GxAudioDecProperty_BoostVolume* boost)
{
	boost->index = (boost->index > BOOST_VOLUME_MAX_INDEX) ? BOOST_VOLUME_MAX_INDEX : boost->index;
	boost->index = (boost->index < BOOST_VOLUME_MIN_INDEX) ? BOOST_VOLUME_MIN_INDEX : boost->index;
	audiodec->boostdb = boost_index_db[boost->index];
	audiodec->boostdb_active = 1;
	return 0;
}

static int gx3201_audiodec_set_volume(struct gxav_audiodec *audiodec, int volumedb)
{
	audiodec->volumedb = volumedb;
	audiodec->volumedb_active = 1;
	return 0;
}

static int gx3201_audiodec_set_ad_ctrlinfo(struct gxav_audiodec *audiodec, int enable)
{
	audiodec->decode_ctrlinfo_enable = enable;
	return 0;
}


static int gx3201_audiodec_close(struct gxav_audiodec *audiodec)
{
	gx3201_audiodec_stop(audiodec);

	if(audiodec->dec_id == 0){
		if(audiodec->pcm_fifo){
			gxfifo_free(audiodec->pcm_fifo);
			gx_free(audiodec->pcm_fifo);
			audiodec->pcm_fifo = NULL;
		}
		if(audiodec->bypass_fifo){
			gxfifo_free(audiodec->bypass_fifo);
			gx_free(audiodec->bypass_fifo);
			audiodec->bypass_fifo = NULL;
		}
		if(audiodec->degrade_fifo){
			gxfifo_free(audiodec->degrade_fifo);
			gx_free(audiodec->degrade_fifo);
			audiodec->degrade_fifo = NULL;
		}
		if(audiodec->encode_fifo){
			gxfifo_free(audiodec->encode_fifo);
			gx_free(audiodec->encode_fifo);
			audiodec->encode_fifo = NULL;
		}
	}else if(audiodec->dec_id == 1){
		if(audiodec->pcm_fifo){
			gxfifo_free(audiodec->pcm_fifo);
			gx_free(audiodec->pcm_fifo);
			audiodec->pcm_fifo = NULL;
		}

		if(audiodec->ctrlinfo_fifo){
			gxfifo_free(audiodec->ctrlinfo_fifo);
			gx_free(audiodec->ctrlinfo_fifo);
		}
	}
	audiodec->addec_reg = NULL;
	audiodec->audiodec_reg = NULL;

	return 0;
}

static int gx3201_audiodec_uninit(void)
{
	unsigned int reg_map_addr = 0;

	reg_map_addr = (unsigned int)audio_dec[0].reg;
	gx_iounmap(reg_map_addr);
	gx_release_mem_region(audio_dec[0].reg_base_addr, sizeof(struct audiodec_regs));
	audio_dec[0].reg = NULL;
	return 0;
}

static int gx3201_audiodec_get_dolby_type(void)
{
	return DolbyChip;
}

static int gx3201_audiodec_set_dump(struct gxav_audiodec *audiodec, int enable)
{
#ifdef DEBUG_ADEC_RECORD
	if (enable == 1) {
		if (!record_buffer)
			record_buffer = gx_page_malloc(record_buffer_size);
		record_size       = 0;
		record_over_flags = 0;
	} else {
		record_over_flags = 1;
		if (record_buffer) {
			gx_page_free(record_buffer, record_size);
			record_buffer = NULL;
		}
	}
#endif
	return 0;
}

static int gx3201_audiodec_get_dump(struct gxav_audiodec *audiodec, GxAvDebugDump *dump)
{
#ifdef DEBUG_ADEC_RECORD
	dump->status = record_over_flags;
	dump->buffer = record_buffer;
	dump->size   = record_size;
#endif
	return 0;
}

static int gx32xx_audiodec_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	audio_dec[0].reg_base_addr = res->regs[0];

	gx3201_audiodec_module.irqs[0] = res->irqs[0];
	gx3201_audiodec_module.interrupts[res->irqs[0]] = gx_audiodec_interrupt;

	return 0;
}

static struct audiodec_hal_ops gx3201_audiodec_ops = {
	.init            = gx3201_audiodec_init,
	.uninit          = gx3201_audiodec_uninit,
	.open            = gx3201_audiodec_open,
	.close           = gx3201_audiodec_close,
	.link            = gx3201_audiodec_link_fifo,
	.unlink          = gx3201_audiodec_unlink_fifo,
	.config          = gx3201_audiodec_config,
	.run             = gx3201_audiodec_start,
	.stop            = gx3201_audiodec_stop,
	.pause           = gx3201_audiodec_pause,
	.resume          = gx3201_audiodec_resume,
	.update          = gx3201_audiodec_update,
	.get_outinfo     = gx3201_audiodec_get_outinfo,
	.get_frameinfo   = gx3201_audiodec_get_frameinfo,
	.get_state       = gx3201_audiodec_get_state,
	.get_error       = gx3201_audiodec_get_errno,
	.get_cap         = gx3201_audiodec_get_cap,
	.get_frame       = gx3201_audiodec_get_frame,
	.get_fifo        = gx3201_audiodec_get_fifo,
	.get_max_channel = gx3201_audiodec_get_max_channel,
	.write_ctrlinfo  = gx3201_audiodec_write_ctrlinfo,
	.get_dolbytype   = gx3201_audiodec_get_dolby_type,
	.clr_frame       = gx3201_audiodec_clr_frame,
	.decode_key      = gx3201_audiodec_decode_key,
	.get_key         = gx3201_audiodec_get_key,
	.boost_volume    = gx3201_audiodec_boost_volume,
	.set_volume      = gx3201_audiodec_set_volume,
	.set_ad_ctrlinfo = gx3201_audiodec_set_ad_ctrlinfo,
	.set_mono        = gx3201_audiodec_set_mono,
	.set_pcminfo     = gx3201_audiodec_set_pcminfo,
	.irq             = gx3201_audiodec_interrupt,
	.set_dump        = gx3201_audiodec_set_dump,
	.get_dump        = gx3201_audiodec_get_dump,
	.support_ad      = gx3201_audiodec_support_ad,
};

struct gxav_module_ops gx3201_audiodec_module = {
	.module_type = GXAV_MOD_AUDIO_DECODE,
	.count = 2,
	.event_mask = EVENT_AUDIO_FRAMEINFO|EVENT_AUDIO_KEY|EVENT_AUDIO_DECODE_ERROR,
	.irqs = {37, -1},
	.irq_names = {"adec"},
	.setup = gx32xx_audiodec_setup,
	.init = gx_audiodec_init,
	.cleanup = gx_audiodec_uninit,
	.open = gx_audiodec_open,
	.close = gx_audiodec_close,
	.set_property = gx_audiodec_set_property,
	.get_property = gx_audiodec_get_property,
	.write_ctrlinfo = gx_audiodec_write_ctrlinfo,
	.interrupts[37] = gx_audiodec_interrupt,
	.priv = &gx3201_audiodec_ops
};

