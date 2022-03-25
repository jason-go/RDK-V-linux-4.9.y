#include "BitCode.h"
#include "kernelcalls.h"

static struct {
	CodStd          codec;
	GxAvDecoderType firmware_id;
	int             registed;
	unsigned        size;
	unsigned        def_size;
	unsigned        addr;
} bitcodes[] = {
	{.codec = STD_ALL,   .firmware_id = GXAV_FMID_VIDEO_ALL   , .registed = 0, .size = 0, .def_size = 120*1024, .addr = 0},
	{.codec = STD_MPEG2, .firmware_id = GXAV_FMID_VIDEO_MPEG2 , .registed = 0, .size = 0, .def_size =  12*1024, .addr = 0},
	{.codec = STD_AVS  , .firmware_id = GXAV_FMID_VIDEO_AVS   , .registed = 0, .size = 0, .def_size =  12*1024, .addr = 0},
	{.codec = STD_AVC  , .firmware_id = GXAV_FMID_VIDEO_AVC   , .registed = 0, .size = 0, .def_size =  32*1024, .addr = 0},
	{.codec = STD_MPEG4, .firmware_id = GXAV_FMID_VIDEO_MPEG4 , .registed = 0, .size = 0, .def_size =  28*1024, .addr = 0},
	{.codec = STD_RV   , .firmware_id = GXAV_FMID_VIDEO_RV    , .registed = 0, .size = 0, .def_size =   8*1024, .addr = 0},
	{.codec = STD_HEVC , .firmware_id = GXAV_FMID_VIDEO_HEVC  , .registed = 0, .size = 0, .def_size =  16*1024, .addr = 0},
};

void bitcode_peek_all(void)
{
	int i;
	int max_i = sizeof(bitcodes)/sizeof(bitcodes[0]);
	unsigned size, addr;

	for (i = 0; i < max_i; i++) {
		if(gxav_firmware_fetch(bitcodes[i].firmware_id, &addr, &size) == 0) {
			bitcodes[i].registed = 1;
			bitcodes[i].size     = size;
			bitcodes[i].addr     = addr;
		}
	}
}

int bitcode_fetch(int codec, struct bitcode_desc *desc)
{
	int i, max_i, ret = -1;
	unsigned load_off;

	if (codec == STD_DIV3) {
		codec = STD_MPEG4;
	}

	load_off = 0;
	max_i = sizeof(bitcodes)/sizeof(bitcodes[0]);
	for (i = 1; i < max_i; i++) {
		if (bitcodes[i].codec != codec) {
			load_off += bitcodes[i].registed ? bitcodes[i].size : bitcodes[i].def_size;
		} else {
			if (bitcodes[i].registed == 0) {
				//try video.bin
				i = max_i;
			}
			break;
		}
	}
	//try to fetch video.bin
	if (i == max_i) {
		if (bitcodes[0].registed) {
			i = load_off = 0;
		}
	}

	if (desc && (i != max_i)) {
		desc->codec       = codec;
		desc->code        = (unsigned short*)bitcodes[i].addr;
		desc->size        = bitcodes[i].size;
		desc->load_offset = load_off;
		ret = 0;
	} else {
		gx_printf("\n[BitCode] fetch firmware failed!\n");
	}

	return ret;
}

