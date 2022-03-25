#ifndef _FIRMWARE_H_
#define _FIRMWARE_H_

#ifdef __cplusplus
extern "C"{
#endif

#define GXAV_MAX_FIRMWARE_NUM     32

typedef enum gxav_load_flag {
	GXAV_LOAD_START        = 0x1,
	GXAV_LOAD_CONTINUE     = 0x2,
} GxAvLoadFlag;

typedef enum {
	GXAV_FIRMWARE_AUDIO,
	GXAV_FIRMWARE_VIDEO
} GxAvFirmwareType;

typedef enum gxav_firmware_id {
	GXAV_FMID_DOLBY = 0x0     ,
	GXAV_FMID_AUDIO_ALL       ,
	GXAV_FMID_AUDIO_PCM       ,
	GXAV_FMID_AUDIO_VORBIS    ,
	GXAV_FMID_AUDIO_AVS       ,
	GXAV_FMID_AUDIO_DRA       ,
	GXAV_FMID_AUDIO_DTS       ,
	GXAV_FMID_AUDIO_MPEG      ,
	GXAV_FMID_AUDIO_RA_AAC    ,
	GXAV_FMID_AUDIO_RA_RA8LBR ,
	GXAV_FMID_AUDIO_FLAC      ,
	GXAV_FMID_AUDIO_OPUS      ,
	GXAV_FMID_AUDIO_SBC       ,
	GXAV_FMID_AUDIO_FOV       ,

	GXAV_FMID_VIDEO_ALL = 0x100,
	GXAV_FMID_VIDEO_AVC        ,
	GXAV_FMID_VIDEO_AVS        ,
	GXAV_FMID_VIDEO_HEVC       ,
	GXAV_FMID_VIDEO_MPEG2      ,
	GXAV_FMID_VIDEO_MPEG4      ,
	GXAV_FMID_VIDEO_RV         ,
} GxAvDecoderType;

struct gxav_firmware_info {
	enum gxav_firmware_id func_id;
	unsigned char* func_addr;
	unsigned int   func_size;
	unsigned int   load_size;
	unsigned char  active;
	unsigned char  used;
	unsigned char  fix;  // if fix=1, don't free.
};

typedef struct gxav_firmware_load_info {
	unsigned char* addr;
	unsigned int   func_len;
	unsigned int   func_id;
	unsigned int   size;
	unsigned int   active;
	GxAvLoadFlag   flag;
} GxAvFirmwareLoad;

struct gxav_codec_register {
	unsigned char*       start;
	unsigned int         size;
	unsigned int         type;
};

struct gxav_codec_info {
	GxAvDecoderType type;
	char            name[20];
	char            date[16];
	char            version[16];
	char            second[16];
};

int  gxav_firmware_init(void);
void gxav_firmware_uninit(void);
int  gxav_firmware_copy(void *dst, void *src, unsigned int size, GxAvFirmwareType type);
int  gxav_firmware_load(void * info, void *(*copy)(void *, const void *, int));
int  gxav_firmware_unload(unsigned int id);
int  gxav_firmware_fetch(unsigned int func_id, unsigned int*addr, unsigned int *size);
int  gxav_firmware_add_static(struct gxav_firmware_info *firmware);
int  gxav_firmware_register(struct gxav_codec_register *load);
int  gxav_firmware_check_static(void);
int  gxav_firmware_get_info(struct gxav_codec_info *info);
int  gxav_copy_firmware_info(struct gxav_codec_info *firmware_info, unsigned char* addr, int type);
int  gxav_firmware_check_dolby_workbuf(unsigned int func_id, unsigned char *firmware_addr, unsigned int *workbuf_size);
int  gxav_firmware_check_all_workbuf  (unsigned int func_id, unsigned char *firmware_addr, unsigned int *workbuf_size);
int  gxav_firmware_check_mpeg_workbuf (unsigned int func_id,
		unsigned char *firmware_addr, unsigned int *workbuf_size, unsigned int *support_convert_code);

#ifdef __cplusplus
}
#endif

#endif

