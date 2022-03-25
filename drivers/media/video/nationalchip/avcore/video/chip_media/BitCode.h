#ifndef __BIT_CODE_H__
#define __BIT_CODE_H__

#include "video.h"
#include "gxav_firmware.h"
#include "gxav_common.h"

struct bitcode_desc {
	CodStd   codec;
	unsigned short *code;
	unsigned size;
	unsigned load_offset;
};

void bitcode_peek_all(void);
int  bitcode_fetch(int codec, struct bitcode_desc *desc);

#endif
