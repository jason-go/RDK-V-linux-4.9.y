#include "kernelcalls.h"
#include "gxav_firmware.h"
#include "gxav_m2m.h"
#include "gxav_audiodec_propertytypes.h"
#include "avcore.h"
#include "profile.h"
#include "firewall.h"
#include "include/audio_common.h"

#ifdef CONFIG_AV_MODULE_FIRMWARE

#define gxav_firmware_malloc(size)  (size) > MAX_KMALLOC_SIZE ? gx_page_malloc(size): gx_malloc(size)
#define gxav_firmware_free(p,size)  (size) > MAX_KMALLOC_SIZE ? gx_page_free(p, size): gx_free(p)

static struct gxav_firmware_info firmware[GXAV_MAX_FIRMWARE_NUM];
static struct gxav_firmware_info *static_firmware[GXAV_MAX_FIRMWARE_NUM] = {NULL, };

gx_mutex_t firmware_lock;

static char gx3201_firmware_tag[4]  = {0xa5, 0x96, 0xc3, 0xe1};
static char gx3211_firmware_tag[4]  = {0xd3, 0x28, 0xb1, 0x17};
static char gx3113c_firmware_tag[4] = {0x33, 0x31, 0x31, 0x76};
static char gx6605s_firmware_tag[4] = {0x36, 0x36, 0x30, 0x88};
static char sirius_firmware_tag[4]  = {0x53, 0x69, 0x72, 0x69};
static char taurus_firmware_tag[4]  = {0x74, 0x61, 0x75, 0x72};
static char gemini_firmware_tag[4]  = {0x67, 0x65, 0x6d, 0x69};

int gxav_copy_firmware_info(struct gxav_codec_info *firmware_info, unsigned char* addr, int type)
{
	gx_memset(firmware_info, 0, sizeof(struct gxav_codec_info));
	firmware_info->type = type;

	switch (type) {
		case GXAV_FMID_DOLBY:
			if (addr[0xe0] == 0xfe)
				if (addr[0xe1] == 0x01)
					strcpy(firmware_info->name, "DOLBY-UDC");
				else
					strcpy(firmware_info->name, "DOLBY-DCV");
			else
				strcpy(firmware_info->name, "DOLBY");
			break;
		case GXAV_FMID_AUDIO_RA_AAC:
			strcpy(firmware_info->name, "RAAC");
			break;
		case GXAV_FMID_AUDIO_RA_RA8LBR:
			strcpy(firmware_info->name, "RA8LBR");
			break;
		case GXAV_FMID_AUDIO_DRA:
			strcpy(firmware_info->name, "DRA");
			break;
		case GXAV_FMID_AUDIO_MPEG:
			if (addr[0xe0] == 0xf0)
				strcpy(firmware_info->name, "MPEG-MINI");
			else
				strcpy(firmware_info->name, "MPEG");
			break;
		case GXAV_FMID_AUDIO_PCM:
			strcpy(firmware_info->name, "PCM");
			break;
		case GXAV_FMID_AUDIO_FLAC:
			strcpy(firmware_info->name, "FLAC");
			break;
		case GXAV_FMID_AUDIO_VORBIS:
			strcpy(firmware_info->name, "VORBIS");
			break;
		case GXAV_FMID_AUDIO_DTS:
			strcpy(firmware_info->name, "DTS");
			break;
		case GXAV_FMID_AUDIO_OPUS:
			strcpy(firmware_info->name, "OPUS");
			break;
		case GXAV_FMID_AUDIO_SBC:
			strcpy(firmware_info->name, "SBC");
			break;
		case GXAV_FMID_AUDIO_FOV:
			strcpy(firmware_info->name, "FLAC.OPUS.VORBIS");
			break;
		case GXAV_FMID_AUDIO_ALL:
			//0xe0: 0xfe - 包含dolby固件
			//0xe1: 0x01 -   包含udc固件, 其他 - 包含dcv固件
			//0xe3: 0x01 -    支持AD功能，其他 - 不支持AD功能
			if (addr[0xe0] == 0xfe)
				if (addr[0xe2] == 0x01)
					strcpy(firmware_info->name, "ALL-DOLBY(T)");
				else
					strcpy(firmware_info->name, "ALL-DOLBY");
			else
				strcpy(firmware_info->name, "ALL");
			break;
		default:
			return -1;
	}

	gx_sprintf(firmware_info->date,    "%02d%02d.%02d.%02d", addr[0xd0], addr[0xd1], addr[0xd2], addr[0xd3]);
	gx_sprintf(firmware_info->second,  "%02d:%02d:%02d",     addr[0xcd], addr[0xce], addr[0xcf]);
	gx_sprintf(firmware_info->version, "%x.%x.%x.%x",        addr[0xd4], addr[0xd5], addr[0xd6], addr[0xd7]);

	return 0;
}

static int gxav_check_ac3_firmware(unsigned char* firmware_addr, int type)
{
	if (type == GXAV_FMID_DOLBY) {
		char* firmware_tag = NULL;
		if (CHIP_IS_GX3201)
			firmware_tag = gx3201_firmware_tag;
		else if (CHIP_IS_GX3211)
			firmware_tag = gx3211_firmware_tag;
		else if (CHIP_IS_GX3113C)
			firmware_tag = gx3113c_firmware_tag;
		else if (CHIP_IS_GX6605S)
			firmware_tag = gx6605s_firmware_tag;
		else if (CHIP_IS_SIRIUS)
			firmware_tag = sirius_firmware_tag;
		else if (CHIP_IS_TAURUS)
			firmware_tag = taurus_firmware_tag;
		else if (CHIP_IS_GEMINI)
			firmware_tag = gemini_firmware_tag;
		else {
			gx_printf("Register Failed, ac3 firmware error\n");
			return -1;
		}

		if (firmware_addr[0xe0] != 0xff && firmware_addr[0xe0] != 0xfe) {
			gx_printf("Register Failed, ac3 firmware error\n");
			return -1;
		}

		if(firmware_addr[0xf0] != firmware_tag[0] && firmware_addr[0xf1] != firmware_tag[1] &&
				firmware_addr[0xf2] != firmware_tag[2] && firmware_addr[0xf3] != firmware_tag[3] &&
				firmware_addr[0xe0] == 0xff){
			gx_printf("Register Failed, ac3 firmware not this chip\n");
			return -1;
		}

#ifndef NO_OS
		if(gxav_audiodec_get_dolby_type() == 0 && firmware_addr[0xe0] == 0xfe){
			gx_printf("Register Failed, NOT -D chip, firmware is hard decoder\n");
			return -1;
		}
#endif
	}

	return 0;
}

int gxav_firmware_init(void)
{
	gx_memset(firmware, 0, sizeof(firmware));
	gx_mutex_init(&firmware_lock);

	return 0;
}

void gxav_firmware_uninit(void)
{
	int i;

	for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
		if (firmware[i].func_addr)
			gxav_firmware_free(firmware[i].func_addr, firmware[i].func_size);
	}

	gx_memset(&firmware, 0, sizeof(firmware));

	gx_mutex_destroy(&firmware_lock);
}

int gxav_firmware_fetch(unsigned int func_id, unsigned int *addr, unsigned int *size)
{
	int i, ret = 0;

	if (!addr || !size)
		return -1;

	gx_mutex_lock(&firmware_lock);

	for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
		if (firmware[i].func_id == func_id && firmware[i].active == 1) {
			*size = firmware[i].func_size;
			*addr = (unsigned int)firmware[i].func_addr;
			goto out;
		}
	}

	for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
		if(static_firmware[i])
		{
			if (static_firmware[i]->func_id == func_id && static_firmware[i]->active == 1) {
				*size = static_firmware[i]->func_size;
				*addr = (unsigned int)static_firmware[i]->func_addr;
				goto out;
			}
		}
	}
	ret = -1;
out:
	gx_mutex_unlock(&firmware_lock);

	return ret;
}

int gxav_firmware_unload(unsigned int id)
{
	int i;

	gx_mutex_lock(&firmware_lock);

	for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
		if (firmware[i].func_id == id && firmware[i].active == 1 && firmware[i].func_addr) {

			gxav_firmware_free(firmware[i].func_addr, firmware[i].func_size);

			gx_memset(&firmware[i], 0, sizeof(struct gxav_firmware_info));

			gx_mutex_unlock(&firmware_lock);

			return 0;
		}
	}

	gx_mutex_unlock(&firmware_lock);

	return -1;
}

int gxav_firmware_add_static(struct gxav_firmware_info *firmware)
{
	int i = 0;

	GXAV_DBG("%s: (firmware=%p, firmware->func_id=0x%x)\n", __FUNCTION__, firmware, firmware->func_id);

	while (i < GXAV_MAX_FIRMWARE_NUM) {
		if (static_firmware[i] == NULL) {
			static_firmware[i] = firmware;
			static_firmware[i]->active = 1;
			return 0;
		}
		i++;
	}

	return -1;
}

int gxav_firmware_check_static(void)
{
	int i = 0;

	while (i < GXAV_MAX_FIRMWARE_NUM) {
		if (static_firmware[i] && static_firmware[i]->active == 1) {
			if (gxav_check_ac3_firmware(static_firmware[i]->func_addr, static_firmware[i]->func_id) < 0) {
				static_firmware[i]->active = 0;
				static_firmware[i] = NULL;
			}
		}
		i++;
	}
	return 0;
}

int gxav_firmware_check_mpeg_workbuf(unsigned int func_id,
		unsigned char *firmware_addr, unsigned int *workbuf_size, unsigned int *support_convert_code)
{
	if (func_id == GXAV_FMID_AUDIO_MPEG) {
		if (firmware_addr[0xe0] == 0xf0) {
			*workbuf_size = 0xb0000;
			*support_convert_code = 0;
		}
	}

	return 0;
}

int gxav_firmware_check_dolby_workbuf(unsigned int func_id,
		unsigned char *firmware_addr, unsigned int *workbuf_size)
{
	if (func_id == GXAV_FMID_DOLBY) {
		if ((firmware_addr[0xe0] == 0xfe) &&
				(firmware_addr[0xe1] == 0x01))
			*workbuf_size = 0x180000;
	}

	return 0;
}

int gxav_firmware_check_all_workbuf(unsigned int func_id,
		unsigned char *firmware_addr, unsigned int *workbuf_size)
{
	if (func_id == GXAV_FMID_AUDIO_ALL) {
		if (firmware_addr[0xe0] == 0xfe) {
			if (firmware_addr[0xe3] == 0x1)
				*workbuf_size = 0x280000;
			else
				*workbuf_size = 0x240000;
		}
	}

	return 0;
}

int gxav_firmware_load(void *info, void *(*copy) (void *, const void *, int))
{
	int i;
	int ret = -1;
	unsigned char *func_addr;

	struct gxav_firmware_load_info *load = info;

	if (load == NULL || load->addr == NULL)
		return -1;

	if (copy == NULL)
		copy = gx_sdc_memcpy;

	gx_mutex_lock(&firmware_lock);

	switch (load->flag) {
		case GXAV_LOAD_START:
			for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
				if (firmware[i].func_id == load->func_id) {
					if (firmware[i].func_addr)
						gxav_firmware_free(firmware[i].func_addr, firmware[i].func_size);
					gx_memset(&firmware[i], 0, sizeof(struct gxav_firmware_info));
				};

				if (firmware[i].used == 0) {
					firmware[i].func_id = load->func_id;
					firmware[i].func_size = load->func_len;
					firmware[i].load_size = load->size;

					firmware[i].func_addr = gxav_firmware_malloc(load->func_len);
					if (firmware[i].func_addr) {
						copy(firmware[i].func_addr, load->addr, load->size);
						firmware[i].used = 1;
						break;
					}
				}
			}
			if(i >= GXAV_MAX_FIRMWARE_NUM) {
				gx_printf("%s(),flag=%d,all firmware scan return -1\n",__func__,load->flag);
				goto out;
			}

			break;
		case GXAV_LOAD_CONTINUE:
			for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
				if (firmware[i].func_id == load->func_id) {
					func_addr = firmware[i].func_addr + firmware[i].load_size;
					firmware[i].load_size += load->size;

					if (firmware[i].load_size > firmware[i].func_size) {
						gx_printf("%s(),flag=%d,firmware func_size=0x%x < load_size=0x%x return -1\n",
								__func__,load->flag,firmware[i].func_size,firmware[i].load_size);

						gxav_firmware_free(firmware[i].func_addr, firmware[i].func_size);
						gx_memset(&firmware[i], 0, sizeof(struct gxav_firmware_info));
						goto out;
					} else if (firmware[i].load_size == firmware[i].func_size && load->size == 0 && load->active == 1) {
						firmware[i].active = 1;
						break;
					} else {
						copy(func_addr, load->addr, load->size);
						break;
					}
				}
			}
			if(i >= GXAV_MAX_FIRMWARE_NUM) {
				gx_printf("%s(),flag=%d,all firmware scan return -1\n",__func__,load->flag);
				goto out;
			}

			break;
		default:
			gx_printf("the parameter which or's flag is wrong, please set the right flag\n");
			goto out;
	}

	if (firmware[i].load_size == firmware[i].func_size && load->flag == 1) {
		GXAV_DBG("%s(),flag=%d,firmware func_size=0x%x = load_size=0x%x from start,return 0\n",
				__func__,load->flag,firmware[i].func_size,firmware[i].load_size);
		firmware[i].active = 1;
	}

	ret = 0;
out:
	gx_mutex_unlock(&firmware_lock);

	return ret;
}

int gxav_firmware_register(struct gxav_codec_register *load)
{
	int i;

	if (load == NULL || load->start == NULL || load->size >= 2*1024*1024)
		return -1;

	gx_mutex_lock(&firmware_lock);

	for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
		if (firmware[i].used == 0) {

			firmware[i].func_addr = gxav_firmware_malloc(load->size);

			if(firmware[i].func_addr == NULL){
				gx_mutex_unlock(&firmware_lock);
				return -1;
			}

			gx_copy_from_user(firmware[i].func_addr, load->start, load->size);
			if(gxav_check_ac3_firmware(firmware[i].func_addr, load->type) < 0){
				gxav_firmware_free(firmware[i].func_addr, load->size);
				gx_mutex_unlock(&firmware_lock);
				return -1;
			}

			firmware[i].used = 1;
			firmware[i].active = 1;
			firmware[i].func_id = load->type;
			firmware[i].func_size = load->size;

			gx_mutex_unlock(&firmware_lock);
			return 0;
		}
	}

	gx_mutex_unlock(&firmware_lock);
	return -1;
}

int gxav_firmware_get_info(struct gxav_codec_info *firmware_info)
{
	int i = 0, ret = -1;
	struct gxav_codec_info ;

	if (firmware_info == NULL)
		return -1;

	gx_mutex_lock(&firmware_lock);
	for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
		if (firmware[i].active &&
				(firmware[i].func_id == firmware_info->type)) {
			ret = gxav_copy_firmware_info(firmware_info, firmware[i].func_addr, firmware[i].func_id);
			goto find_out;
		}
	}

	for (i = 0; i < GXAV_MAX_FIRMWARE_NUM; i++) {
		if (static_firmware[i] && static_firmware[i]->active &&
				(static_firmware[i]->func_id == firmware_info->type)) {
			ret = gxav_copy_firmware_info(firmware_info, static_firmware[i]->func_addr, static_firmware[i]->func_id);
			goto find_out;
		}
	}

find_out:
	gx_mutex_unlock(&firmware_lock);

	return ret;
}

int gxav_firmware_copy(void *dst, void *src, unsigned int size, GxAvFirmwareType type)
{
	int protect_buffer = (type == GXAV_FIRMWARE_AUDIO) ? GXAV_BUFFER_AUDIO_FIRMWARE: GXAV_BUFFER_VIDEO_FIRMWARE;
	int protect_flag = gxav_firewall_buffer_protected(protect_buffer);

	if (protect_flag) {
		int ret = 0;
		static unsigned afw = 0, vfw = 0;
		if (type == GXAV_FIRMWARE_AUDIO && afw == 0) {
			ret = gxm2m_firmware_copy((unsigned int)dst, (unsigned int)src, size, type);
			afw = 1;
		}
		else if (type == GXAV_FIRMWARE_VIDEO && vfw == 0) {
			ret = gxm2m_firmware_copy((unsigned int)dst, (unsigned int)src, size, type);
			vfw = 1;
		}
		return ret;
	}
	else {
		gx_dcache_clean_range(0, 0);
		gx_memcpy(dst, src, size);
		gx_dcache_clean_range(0, 0);
		return 0;
	}
}

#endif
