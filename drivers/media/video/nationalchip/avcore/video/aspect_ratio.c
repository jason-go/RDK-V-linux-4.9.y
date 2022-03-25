#include "kernelcalls.h"
#include "gxav_common.h"

#include "aspect_ratio.h"
#include "chip_media/VpuApi.h"

static struct {
	SampleAspectRatio sar;
	GxAvRational      ra;
} sar2ra[] = {
	{SAR_NONE    , {  0,  0} },
	{SAR_1BY1    , {  1,  1} },
	{SAR_12BY11  , { 12, 11} },
	{SAR_10BY11  , { 10, 11} },
	{SAR_16BY11  , { 16, 11} },
	{SAR_40BY33  , { 40, 33} },
	{SAR_24BY11  , { 24, 11} },
	{SAR_20BY11  , { 20, 11} },
	{SAR_32BY11  , { 32, 11} },
	{SAR_80BY33  , { 80, 33} },
	{SAR_18BY11  , { 18, 11} },
	{SAR_15BY11  , { 15, 11} },
	{SAR_64BY33  , { 64, 33} },
	{SAR_160BY99 , {160, 99} },
	{SAR_RESERVED, {  0,  0} },
};

static struct {
	DispAspectRatio dar;
	GxAvRational    ra;
} dar2ra[] = {
	{ASPECT_RATIO_NONE   , {  0, 0} },
	{ASPECT_RATIO_1BY1   , {  1, 1} },
	{ASPECT_RATIO_4BY3   , {  4, 3} },
	{ASPECT_RATIO_14BY9  , { 14, 9} },
	{ASPECT_RATIO_16BY9  , { 16, 9} },
	{ASPECT_RATIO_221BY1 , {221, 1} },
	{ASPECT_RATIO_RESERVE, {  0, 0} },
};

static struct {
	SampleAspectRatio sar;
	int width;
	int height;
	DispAspectRatio dar;
} avc_sar2dar[] = {
	{0,0,0,0},
	{SAR_1BY1    , 1280 , 720  , ASPECT_RATIO_16BY9 },
	{SAR_1BY1    , 1920 , 1080 , ASPECT_RATIO_16BY9 },
	{SAR_1BY1    , 1920 , 1084 , ASPECT_RATIO_16BY9 },
	{SAR_1BY1    , 1920 , 1088 , ASPECT_RATIO_16BY9 },
	{SAR_1BY1    , 640  , 480  , ASPECT_RATIO_4BY3  },
	{SAR_12BY11  , 720  , 576  , ASPECT_RATIO_4BY3  },
	{SAR_12BY11  , 352  , 288  , ASPECT_RATIO_4BY3  },
	{SAR_10BY11  , 720  , 480  , ASPECT_RATIO_4BY3  },
	{SAR_10BY11  , 352  , 240  , ASPECT_RATIO_4BY3  },
	{SAR_16BY11  , 720  , 576  , ASPECT_RATIO_16BY9 },
	{SAR_16BY11  , 540  , 576  , ASPECT_RATIO_4BY3  },
	{SAR_16BY11  , 544  , 576  , ASPECT_RATIO_4BY3  },
	{SAR_40BY33  , 720  , 480  , ASPECT_RATIO_16BY9 },
	{SAR_40BY33  , 540  , 480  , ASPECT_RATIO_4BY3  },
	{SAR_24BY11  , 352  , 576  , ASPECT_RATIO_4BY3  },
	{SAR_24BY11  , 480  , 576  , ASPECT_RATIO_16BY9 },
	{SAR_20BY11  , 352  , 480  , ASPECT_RATIO_4BY3  },
	{SAR_20BY11  , 480  , 480  , ASPECT_RATIO_16BY9 },
	{SAR_32BY11  , 352  , 576  , ASPECT_RATIO_16BY9 },
	{SAR_80BY33  , 352  , 480  , ASPECT_RATIO_16BY9 },
	{SAR_18BY11  , 480  , 576  , ASPECT_RATIO_4BY3  },
	{SAR_15BY11  , 480  , 480  , ASPECT_RATIO_4BY3  },
	{SAR_64BY33  , 540  , 576  , ASPECT_RATIO_16BY9 },
	{SAR_160BY99 , 540  , 480  , ASPECT_RATIO_16BY9 },
}, mpeg4_sar2dar[] = {
	{0,0,0,0},
	{SAR_1BY1    , 0, 0, ASPECT_RATIO_RESERVE} ,
	{SAR_12BY11  , 720  , 576  , ASPECT_RATIO_4BY3  },
	{SAR_10BY11  , 720  , 480  , ASPECT_RATIO_4BY3  },
	{SAR_16BY11  , 720  , 576  , ASPECT_RATIO_16BY9 },
	{SAR_40BY33  , 720  , 480  , ASPECT_RATIO_16BY9 },
 };

static DispAspectRatio avs_mpeg2_dar[] = {
	ASPECT_RATIO_NONE,
	ASPECT_RATIO_1BY1,
	ASPECT_RATIO_4BY3,
	ASPECT_RATIO_16BY9,
	ASPECT_RATIO_221BY1
};

static int av_gcd(int a, int b)
{
    if (b)
        return av_gcd(b, a % b);
    else
        return a;
}

static int av_reduce(int *dst_num, int *dst_den,
              int num, int den, int max)
{
    GxAvRational a0 = { 0, 1 }, a1 = { 1, 0 };
    int sign = (num < 0) ^ (den < 0);
    int gcd = av_gcd(GX_ABS(num), GX_ABS(den));

    if (gcd) {
        num = GX_ABS(num) / gcd;
        den = GX_ABS(den) / gcd;
    }
    if (num <= max && den <= max) {
        a1  = (GxAvRational) { num, den };
        den = 0;
    }

    while (den) {
        unsigned int x        = num / den;
        int next_den  = num - den * x;
        int a2n       = x * a1.num + a0.num;
        int a2d       = x * a1.den + a0.den;

        if (a2n > max || a2d > max) {
            if (a1.num) x =           ((max - a0.num) / a1.num);
            if (a1.den) x = GX_MIN(x, (unsigned int)((max - a0.den) / a1.den));

            if (den * (2 * x * a1.den + a0.den) > num * a1.den)
                a1 = (GxAvRational) { x * a1.num + a0.num, x * a1.den + a0.den };
            break;
        }

        a0  = a1;
        a1  = (GxAvRational) { a2n, a2d };
        num = den;
        den = next_den;
    }

    *dst_num = sign ? -a1.num : a1.num;
    *dst_den = a1.den;

    return den == 0;
}

static void sar__enum_to_rational(SampleAspectRatio sar, GxAvRational *ra)
{
	int i;

	if (ra) {
        for (i = 0; i < sizeof(sar2ra)/sizeof(sar2ra[0]); i++) {
            if (sar2ra[i].sar == sar) {
                *ra = sar2ra[i].ra;
            }
        }
	}
}

static int sar__rational_to_enum(GxAvRational *ra)
{
    int i, ret = SAR_NONE;

	if (ra) {
        for (i = 0; i < sizeof(sar2ra)/sizeof(sar2ra[0]); i++) {
            if (sar2ra[i].ra.num == ra->num && sar2ra[i].ra.den == ra->den) {
                ret = sar2ra[i].sar;
            }
        }
	}

    return ret;
}

static void dar__enum_to_rational(DispAspectRatio dar, GxAvRational *ra)
{
	int i;

	if (ra) {
        for (i = 0; i < sizeof(dar2ra)/sizeof(dar2ra[0]); i++) {
            if (dar2ra[i].dar == dar) {
                *ra = dar2ra[i].ra;
            }
        }
	}
}

static int dar__rational_to_enum(GxAvRational *ra)
{
    int i, ret = ASPECT_RATIO_NONE;

	if (ra) {
        for (i = 0; i < sizeof(dar2ra)/sizeof(dar2ra[0]); i++) {
            if (dar2ra[i].ra.num == ra->num && dar2ra[i].ra.den == ra->den) {
                ret = dar2ra[i].dar;
            }
        }
	}

    return ret;
}

static int calc_sar_rational(GxAvRational dar, GxAvRational par, GxAvRational *sar)
{
	int ret = -1;

    if (sar && dar.num && par.num) {
		ret = av_reduce(&sar->num, &sar->den, dar.num*par.den, dar.den*par.num, 1024*1024);
    }

	return ret;
}

static int calc_dar_rational(GxAvRational sar, GxAvRational par, GxAvRational *dar)
{
	int ret = -1;

    if (dar && par.den && sar.den) {
		ret = av_reduce(&dar->num, &dar->den, par.num*sar.num, par.den*sar.den, 1024*1024);
    }

	return ret;
}

void vd_get_sar_rational(int codec, int ratio_bit, int pixnum_w, int pixnum_h, GxAvRational *sar)
{
	int i, sar_code = SAR_NONE;
	GxAvRational dar = {0, 0};
	GxAvRational par = {pixnum_w, pixnum_h};

	if (sar) {
		sar->num = sar->den = 0;
	}
	if (codec == STD_MPEG2 || codec == STD_AVS) {
		sar_code = ratio_bit&0xf;
		if (sar_code == 1) {
			sar_code = SAR_1BY1;
		} else if (sar_code < sizeof(avs_mpeg2_dar)/sizeof(avs_mpeg2_dar[0])) {
			int dar_code = avs_mpeg2_dar[sar_code];
			dar__enum_to_rational(dar_code, &dar);
			goto CALC;
		}
	} else if ( codec == STD_MPEG4 || codec == STD_H263 || codec == STD_DIV3) {
		sar_code = ratio_bit&0xf;
		if (sar_code<sizeof(mpeg4_sar2dar)/sizeof(mpeg4_sar2dar[0])) {
			sar_code = mpeg4_sar2dar[sar_code].sar;
		} else if (sar_code == 0xf) {
			//if par == 1, then sar equal dar
			sar_code = SAR_NONE;
			par.num = par.den = 1;
			dar.num = (ratio_bit >>  4)&0xff;
			dar.den = (ratio_bit >> 12)&0xff;
			goto CALC;
		}
	} else if (codec == STD_AVC || codec == STD_HEVC) {
		if (ratio_bit < sizeof(avc_sar2dar)/sizeof(avc_sar2dar[0])) {
			for (i = 0; i < sizeof(avc_sar2dar)/sizeof(avc_sar2dar[0]); i++) {
				if (avc_sar2dar[i].sar == ratio_bit) {
					sar_code = avc_sar2dar[i].sar;
				}
			}
		} else {
			//if par == 1, then sar equal dar
			sar_code = SAR_NONE;
			par.num = par.den = 1;
			dar.num = (ratio_bit >> 16)&0xffff;
			dar.den = (ratio_bit >>  0)&0xffff;
            goto CALC;
		}
	}

CALC:
	if (sar_code != SAR_NONE) {
		sar__enum_to_rational(sar_code, sar);
	} else {
		calc_sar_rational(dar, par, sar);
	}
}

void vd_get_dar_rational(int codec, int ratio_bit, int pixnum_w, int pixnum_h, GxAvRational *dar)
{
	int i, dar_code = ASPECT_RATIO_NONE;
	int sar_code = SAR_NONE;
	GxAvRational sar = {0, 0};
	GxAvRational par = {pixnum_w, pixnum_h};

	if (dar) {
		dar->num = dar->den = 0;
	}
	if (codec == STD_MPEG2 || codec == STD_AVS) {
		sar_code = ratio_bit&0xf;
		if (sar_code == 1) {
			sar.num = 1, sar.den = 1;
			goto CALC;
		} else if (sar_code < sizeof(avs_mpeg2_dar)/sizeof(avs_mpeg2_dar[0])) {
			dar_code = avs_mpeg2_dar[sar_code];
		}
	} else if (codec == STD_MPEG4 || codec == STD_H263 || codec == STD_DIV3) {
		sar_code = ratio_bit&0xf;
		if (sar_code == 1) {
			sar.num = 1, sar.den = 1;
			goto CALC;
		} else if (sar_code<sizeof(mpeg4_sar2dar)/sizeof(mpeg4_sar2dar[0])) {
			dar_code = mpeg4_sar2dar[sar_code].dar;
		} else if (sar_code == 0xf) {
			sar.num = (ratio_bit >>  4)&0xff;
			sar.den = (ratio_bit >> 12)&0xff;
			goto CALC;
		}
	} else if (codec == STD_AVC || codec == STD_HEVC) {
		if (ratio_bit < sizeof(avc_sar2dar)/sizeof(avc_sar2dar[0])) {
			for (i = 0; i < sizeof(avc_sar2dar)/sizeof(avc_sar2dar[0]); i++) {
				if (avc_sar2dar[i].sar == ratio_bit) {
					if (avc_sar2dar[i].width == pixnum_w && avc_sar2dar[i].height == pixnum_h) {
						dar_code = avc_sar2dar[i].dar;
					} else {
                        sar_code = avc_sar2dar[i].sar;
                    }
				}
			}
			if(sar_code != SAR_NONE && dar_code == ASPECT_RATIO_NONE) {
                sar__enum_to_rational(sar_code, &sar);
                goto CALC;
			}
		} else {
			sar.num = (ratio_bit >> 16)&0xffff;
			sar.den = (ratio_bit >>  0)&0xffff;
            goto CALC;
		}
	}

CALC:
	if (dar_code != ASPECT_RATIO_NONE) {
		dar__enum_to_rational(dar_code, dar);
	} else {
		calc_dar_rational(sar, par, dar);
	}
}

int vd_get_sar_enum(int codec, int ratio_bit, int pixnum_w, int pixnum_h)
{
	int ret = SAR_NONE;
	GxAvRational sar = {0, 0};

	vd_get_sar_rational(codec, ratio_bit, pixnum_w, pixnum_h, &sar);
	ret = sar__rational_to_enum(&sar);

	return ret;
}

int vd_get_dar_enum(int codec, int ratio_bit, int pixnum_w, int pixnum_h)
{
	int ret = ASPECT_RATIO_NONE;
	GxAvRational dar = {0, 0};

	vd_get_dar_rational(codec, ratio_bit, pixnum_w, pixnum_h, &dar);
	ret = dar__rational_to_enum(&dar);

	return ret;
}

