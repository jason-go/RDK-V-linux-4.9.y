#ifndef __aspect_ratio_h__
#define __aspect_ratio_h__

#include "gxav_common.h"

typedef enum {
	SAR_NONE,
	SAR_1BY1,
	SAR_12BY11,
	SAR_10BY11,
	SAR_16BY11,
	SAR_40BY33,
	SAR_24BY11,
	SAR_20BY11,
	SAR_32BY11,
	SAR_80BY33,
	SAR_18BY11,
	SAR_15BY11,
	SAR_64BY33,
	SAR_160BY99,
	SAR_RESERVED,
} SampleAspectRatio;

typedef enum {
	ASPECT_RATIO_NONE,
	ASPECT_RATIO_1BY1,
	ASPECT_RATIO_4BY3,
	ASPECT_RATIO_14BY9,
	ASPECT_RATIO_16BY9,
	ASPECT_RATIO_221BY1,
	ASPECT_RATIO_RESERVE
} DispAspectRatio;

int  vd_get_sar_enum(int codec, int aspect_ratio_bit, int pixnum_w, int pixnum_h);
int  vd_get_dar_enum(int codec, int aspect_ratio_bit, int pixnum_w, int pixnum_h);
void vd_get_sar_rational(int codec, int aspect_ratio_bit, int piixnum_w, int pixnum_h, GxAvRational *ra);
void vd_get_dar_rational(int codec, int aspect_ratio_bit, int piixnum_w, int pixnum_h, GxAvRational *ra);
#endif
