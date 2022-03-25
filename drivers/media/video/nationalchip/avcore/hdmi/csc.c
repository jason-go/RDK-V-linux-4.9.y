#include "csc.h"

typedef enum {
	YUV2RGB = 0,
	YUV2YUV,
} ColorSpaceType_t;

#define DEFAULT            ( 0)
#define ITU_R_BT_709_6     ( 1)
#define ITU_R_BT_470_6_M   ( 4)
#define ITU_R_BT_601_7_625 ( 5)
#define ITU_R_BT_601_7_525 ( 6)
#define SMPTE_ST_240       ( 7)
#define GENERICFILM        ( 8)
#define ITU_R_BT_2020_2    ( 9)
#define SMPTE_RP_431_2     (11)
#define SMPTE_EG_432_1     (12)

struct csc_param_unit {
	int scale;
	int coef00;
	int coef01;
	int coef02;
	int coef03;
	int coef10;
	int coef11;
	int coef12;
	int coef13;
	int coef20;
	int coef21;
	int coef22;
	int coef23;
};

struct csc_param_unit csc_params[2][3][10] = {
	// HDMI RGB Encoding out 
	{
		//sdtv 525 system
		{
			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 1 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x7fbf,
				.coef02 = 0x354c,
				.coef10 = 0x2000,
				.coef11 = 0x78f5,
				.coef12 = 0x6fda,
				.coef20 = 0x2000,
				.coef21 = 0x39ad,
				.coef22 = 0x0004,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 4 */
			{
				.scale  = 0x02,
				.coef00 = 0x10e4,
				.coef01 = 0x7f8b,
				.coef02 = 0x2847,
				.coef10 = 0x0f88,
				.coef11 = 0x7c24,
				.coef12 = 0x73d1,
				.coef20 = 0x119d,
				.coef21 = 0x2062,
				.coef22 = 0x7fde,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 5 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0007,
				.coef02 = 0x37b7,
				.coef10 = 0x2000,
				.coef11 = 0x78f5,
				.coef12 = 0x6f25,
				.coef20 = 0x2000,
				.coef21 = 0x38fb,
				.coef22 = 0x7fd7,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 6 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x3148,
				.coef10 = 0x2000,
				.coef11 = 0x78f2,
				.coef12 = 0x7112,
				.coef20 = 0x2000,
				.coef21 = 0x3927,
				.coef22 = 0x0000,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 7 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x3148,
				.coef10 = 0x2000,
				.coef11 = 0x78f2,
				.coef12 = 0x7112,
				.coef20 = 0x2000,
				.coef21 = 0x3927,
				.coef22 = 0x0000,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 8 */
			{
				.scale  = 0x02,
				.coef00 = 0x10e4,
				.coef01 = 0x007d,
				.coef02 = 0x2555,
				.coef10 = 0x0f88,
				.coef11 = 0x7b9c,
				.coef12 = 0x74b1,
				.coef20 = 0x119d,
				.coef21 = 0x2260,
				.coef22 = 0x0002,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 9 */
			{
				.scale  = 0x02,
				.coef00 = 0x1000,
				.coef01 = 0x7f2c,
				.coef02 = 0x2f1e,
				.coef10 = 0x1000,
				.coef11 = 0x7c2e,
				.coef12 = 0x71ab,
				.coef20 = 0x1000,
				.coef21 = 0x20f8,
				.coef22 = 0x007d,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 11 */
			{
				.scale  = 0x01,
				.coef00 = 0x1c13,
				.coef01 = 0x012a,
				.coef02 = 0x3f9d,
				.coef10 = 0x21c2,
				.coef11 = 0x78c5,
				.coef12 = 0x6cb1,
				.coef20 = 0x1b60,
				.coef21 = 0x37b4,
				.coef22 = 0x004e,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = 12 */
			{
				.scale  = 0x02,
				.coef00 = 0x1000,
				.coef01 = 0x00aa,
				.coef02 = 0x21e8,
				.coef10 = 0x1000,
				.coef11 = 0x7be3,
				.coef12 = 0x75b5,
				.coef20 = 0x1000,
				.coef21 = 0x1fb0,
				.coef22 = 0x002a,
			},

			/* hdmi colour space out is RGB */
			/* 480i 480p tv format out */
			/* src_primaries = others */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x3148,
				.coef10 = 0x2000,
				.coef11 = 0x78f2,
				.coef12 = 0x7112,
				.coef20 = 0x2000,
				.coef21 = 0x3927,
				.coef22 = 0x0000,
			},

		},
		//sdtv 625 system
		{
			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 1 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x7fc1,
				.coef02 = 0x2e92,
				.coef10 = 0x2000,
				.coef11 = 0x7a24,
				.coef12 = 0x715a,
				.coef20 = 0x2000,
				.coef21 = 0x3ad1,
				.coef22 = 0x002d,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 4 */
			{
				.scale  = 0x02,
				.coef00 = 0x10c7,
				.coef01 = 0x7f96,
				.coef02 = 0x2331,
				.coef10 = 0x0f97,
				.coef11 = 0x7ccc,
				.coef12 = 0x74f2,
				.coef20 = 0x11a6,
				.coef21 = 0x2106,
				.coef22 = 0x7ffc,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 5 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x30ae,
				.coef10 = 0x2000,
				.coef11 = 0x7a22,
				.coef12 = 0x70b5,
				.coef20 = 0x2000,
				.coef21 = 0x3a1b,
				.coef22 = 0x0000,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 6 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x7ffa,
				.coef02 = 0x2b0f,
				.coef10 = 0x2000,
				.coef11 = 0x7a20,
				.coef12 = 0x7275,
				.coef20 = 0x2000,
				.coef21 = 0x3a49,
				.coef22 = 0x0025,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 7 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x7ffa,
				.coef02 = 0x2b0f,
				.coef10 = 0x2000,
				.coef11 = 0x7a20,
				.coef12 = 0x7275,
				.coef20 = 0x2000,
				.coef21 = 0x3a49,
				.coef22 = 0x0025,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 8 */
			{
				.scale  = 0x02,
				.coef00 = 0x10c7,
				.coef01 = 0x0069,
				.coef02 = 0x209f,
				.coef10 = 0x0f97,
				.coef11 = 0x7c55,
				.coef12 = 0x75be,
				.coef20 = 0x11a6,
				.coef21 = 0x230e,
				.coef22 = 0x001f,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 9 */
			{
				.scale  = 0x02,
				.coef00 = 0x1000,
				.coef01 = 0x7f43,
				.coef02 = 0x292b,
				.coef10 = 0x1000,
				.coef11 = 0x7cd7,
				.coef12 = 0x7301,
				.coef20 = 0x1000,
				.coef21 = 0x219e,
				.coef22 = 0x00a4,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 11 */
			{
				.scale  = 0x01,
				.coef00 = 0x1c93,
				.coef01 = 0x00ff,
				.coef02 = 0x3795,
				.coef10 = 0x218e,
				.coef11 = 0x79f4,
				.coef12 = 0x6e7d,
				.coef20 = 0x1b45,
				.coef21 = 0x38cf,
				.coef22 = 0x0080,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = 12 */
			{
				.scale  = 0x02,
				.coef00 = 0x1000,
				.coef01 = 0x0091,
				.coef02 = 0x1da1,
				.coef10 = 0x1000,
				.coef11 = 0x7c8f,
				.coef12 = 0x76aa,
				.coef20 = 0x1000,
				.coef21 = 0x2051,
				.coef22 = 0x0044,
			},

			/* hdmi colour space out is RGB */
			/* 576i 576p tv format out */
			/* src_primaries = others */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x30ae,
				.coef10 = 0x2000,
				.coef11 = 0x7a22,
				.coef12 = 0x70b5,
				.coef20 = 0x2000,
				.coef21 = 0x3a1b,
				.coef22 = 0x0000,
			},

		},
		//hdtv 
		{
			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 1 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x3144,
				.coef10 = 0x2000,
				.coef11 = 0x7a24,
				.coef12 = 0x715a,
				.coef20 = 0x2000,
				.coef21 = 0x3a0e,
				.coef22 = 0x0000,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 4 */
			{
				.scale  = 0x02,
				.coef00 = 0x10d4,
				.coef01 = 0x7fb5,
				.coef02 = 0x253a,
				.coef10 = 0x0f97,
				.coef11 = 0x7ccc,
				.coef12 = 0x74f2,
				.coef20 = 0x11a0,
				.coef21 = 0x2098,
				.coef22 = 0x7fda,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 5 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0042,
				.coef02 = 0x337f,
				.coef10 = 0x2000,
				.coef11 = 0x7a22,
				.coef12 = 0x70b5,
				.coef20 = 0x2000,
				.coef21 = 0x395a,
				.coef22 = 0x7fd2,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 6 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x003b,
				.coef02 = 0x2d8e,
				.coef10 = 0x2000,
				.coef11 = 0x7a20,
				.coef12 = 0x7275,
				.coef20 = 0x2000,
				.coef21 = 0x3987,
				.coef22 = 0x7ffc,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 7 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x003b,
				.coef02 = 0x2d8e,
				.coef10 = 0x2000,
				.coef11 = 0x7a20,
				.coef12 = 0x7275,
				.coef20 = 0x2000,
				.coef21 = 0x3987,
				.coef22 = 0x7ffc,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 8 */
			{
				.scale  = 0x02,
				.coef00 = 0x10d4,
				.coef01 = 0x0097,
				.coef02 = 0x2282,
				.coef10 = 0x0f97,
				.coef11 = 0x7c55,
				.coef12 = 0x75be,
				.coef20 = 0x11a0,
				.coef21 = 0x2299,
				.coef22 = 0x0000,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 9 */
			{
				.scale  = 0x02,
				.coef00 = 0x1000,
				.coef01 = 0x7f5f,
				.coef02 = 0x2b8e,
				.coef10 = 0x1000,
				.coef11 = 0x7cd7,
				.coef12 = 0x7301,
				.coef20 = 0x1000,
				.coef21 = 0x212f,
				.coef22 = 0x007b,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 11 */
			{
				.scale  = 0x01,
				.coef00 = 0x1c5a,
				.coef01 = 0x014e,
				.coef02 = 0x3acd,
				.coef10 = 0x218e,
				.coef11 = 0x79f4,
				.coef12 = 0x6e7d,
				.coef20 = 0x1b58,
				.coef21 = 0x3812,
				.coef22 = 0x004a,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 12 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x017c,
				.coef02 = 0x3eb0,
				.coef10 = 0x2000,
				.coef11 = 0x791f,
				.coef12 = 0x6d55,
				.coef20 = 0x2000,
				.coef21 = 0x3fca,
				.coef22 = 0x004f,
			},

			/* hdmi colour space out is RGB */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = others */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x3264,
				.coef10 = 0x2000,
				.coef11 = 0x7a02,
				.coef12 = 0x7106,
				.coef20 = 0x2000,
				.coef21 = 0x3b61,
				.coef22 = 0x0000,
			},

		},
	},
	// HDMI YCC444 Encoding out 
	{
		//sdtv 525 system
		{
			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 1 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x204b,
				.coef12 = 0x0002,
				.coef20 = 0x0000,
				.coef21 = 0x7fd6,
				.coef22 = 0x229b,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 4 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x01c4,
				.coef11 = 0x2443,
				.coef12 = 0x7fd9,
				.coef20 = 0x0122,
				.coef21 = 0x7f67,
				.coef22 = 0x344e,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 5 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x1fe7,
				.coef12 = 0x7fe9,
				.coef20 = 0x0000,
				.coef21 = 0x0004,
				.coef22 = 0x242d,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 6 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2000,
				.coef12 = 0x0000,
				.coef20 = 0x0000,
				.coef21 = 0x0000,
				.coef22 = 0x2000,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 7 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2000,
				.coef12 = 0x0000,
				.coef20 = 0x0000,
				.coef21 = 0x0000,
				.coef22 = 0x2000,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 8 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x01c4,
				.coef11 = 0x267e,
				.coef12 = 0x0003,
				.coef20 = 0x0122,
				.coef21 = 0x00a2,
				.coef22 = 0x307b,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 9 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x24ea,
				.coef12 = 0x008d,
				.coef20 = 0x0000,
				.coef21 = 0x7eed,
				.coef22 = 0x3d30,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 11 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x7d78,
				.coef11 = 0x1f30,
				.coef12 = 0x002c,
				.coef20 = 0x7d83,
				.coef21 = 0x00c2,
				.coef22 = 0x294e,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = 12 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x237c,
				.coef12 = 0x002f,
				.coef20 = 0x0000,
				.coef21 = 0x00dc,
				.coef22 = 0x2c09,
			},

			/* hdmi colour space out type is YCC444 */
			/* 480i 480p tv format out */
			/* src_primaries = others */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2000,
				.coef12 = 0x0000,
				.coef20 = 0x0000,
				.coef21 = 0x0000,
				.coef22 = 0x2000,
			},

		},
		//sdtv 625 system
		{
			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 1 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2064,
				.coef12 = 0x0018,
				.coef20 = 0x0000,
				.coef21 = 0x7fd7,
				.coef22 = 0x1e9d,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 4 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x01c6,
				.coef11 = 0x245f,
				.coef12 = 0x7ffb,
				.coef20 = 0x0100,
				.coef21 = 0x7f74,
				.coef22 = 0x2e44,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 5 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2000,
				.coef12 = 0x0000,
				.coef20 = 0x0000,
				.coef21 = 0x0000,
				.coef22 = 0x2000,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 6 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2019,
				.coef12 = 0x0014,
				.coef20 = 0x0000,
				.coef21 = 0x7ffc,
				.coef22 = 0x1c4e,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 7 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2019,
				.coef12 = 0x0014,
				.coef20 = 0x0000,
				.coef21 = 0x7ffc,
				.coef22 = 0x1c4e,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 8 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x01c6,
				.coef11 = 0x269c,
				.coef12 = 0x0022,
				.coef20 = 0x0100,
				.coef21 = 0x008a,
				.coef22 = 0x2ae3,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 9 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2506,
				.coef12 = 0x00b5,
				.coef20 = 0x0000,
				.coef21 = 0x7f08,
				.coef22 = 0x3620,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 11 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x7d75,
				.coef11 = 0x1f49,
				.coef12 = 0x0046,
				.coef20 = 0x7dcc,
				.coef21 = 0x00a7,
				.coef22 = 0x248a,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = 12 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2398,
				.coef12 = 0x004b,
				.coef20 = 0x0000,
				.coef21 = 0x00be,
				.coef22 = 0x26f3,
			},

			/* hdmi colour space out type is YCC444 */
			/* 576i 576p tv format out */
			/* src_primaries = others */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2000,
				.coef12 = 0x0000,
				.coef20 = 0x0000,
				.coef21 = 0x0000,
				.coef22 = 0x2000,
			},

		},
		//hdtv 
		{
			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 1 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2000,
				.coef12 = 0x0000,
				.coef20 = 0x0000,
				.coef21 = 0x0000,
				.coef22 = 0x2000,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 4 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x01c0,
				.coef11 = 0x23ef,
				.coef12 = 0x7fd6,
				.coef20 = 0x010e,
				.coef21 = 0x7f9f,
				.coef22 = 0x305d,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 5 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x1f9d,
				.coef12 = 0x7fe7,
				.coef20 = 0x0000,
				.coef21 = 0x002b,
				.coef22 = 0x2173,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 6 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x1fb6,
				.coef12 = 0x7ffe,
				.coef20 = 0x0000,
				.coef21 = 0x0027,
				.coef22 = 0x1d96,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 7 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x1fb6,
				.coef12 = 0x7ffe,
				.coef20 = 0x0000,
				.coef21 = 0x0027,
				.coef22 = 0x1d96,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 8 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x01c0,
				.coef11 = 0x2625,
				.coef12 = 0x0000,
				.coef20 = 0x010e,
				.coef21 = 0x00c4,
				.coef22 = 0x2cd4,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 9 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2495,
				.coef12 = 0x0087,
				.coef20 = 0x0000,
				.coef21 = 0x7f2e,
				.coef22 = 0x3894,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 11 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x7d7e,
				.coef11 = 0x1ee8,
				.coef12 = 0x0029,
				.coef20 = 0x7db0,
				.coef21 = 0x00d9,
				.coef22 = 0x2632,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = 12 */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2329,
				.coef12 = 0x002b,
				.coef20 = 0x0000,
				.coef21 = 0x00f7,
				.coef22 = 0x28b8,
			},

			/* hdmi colour space out type is YCC444 */
			/* 1080i 720p  1080p tv format out */
			/* src_primaries = others */
			{
				.scale  = 0x01,
				.coef00 = 0x2000,
				.coef01 = 0x0000,
				.coef02 = 0x0000,
				.coef10 = 0x0000,
				.coef11 = 0x2000,
				.coef12 = 0x0000,
				.coef20 = 0x0000,
				.coef21 = 0x0000,
				.coef22 = 0x2000,
			},
		},
	},
};


void CscReg2Int(struct csc_param_unit *param)
{
    if (param->coef00 >= 0x4000) { param->coef00 = param->coef00 - 0x8000;} 
    if (param->coef01 >= 0x4000) { param->coef01 = param->coef01 - 0x8000;}
    if (param->coef02 >= 0x4000) { param->coef02 = param->coef02 - 0x8000;}
    if (param->coef03 >= 0x4000) { param->coef03 = param->coef03 - 0x8000;}
    if (param->coef10 >= 0x4000) { param->coef10 = param->coef10 - 0x8000;}
    if (param->coef11 >= 0x4000) { param->coef11 = param->coef11 - 0x8000;}
    if (param->coef12 >= 0x4000) { param->coef12 = param->coef12 - 0x8000;}
    if (param->coef13 >= 0x4000) { param->coef13 = param->coef13 - 0x8000;}
    if (param->coef20 >= 0x4000) { param->coef20 = param->coef20 - 0x8000;}
    if (param->coef21 >= 0x4000) { param->coef21 = param->coef21 - 0x8000;}
    if (param->coef22 >= 0x4000) { param->coef22 = param->coef22 - 0x8000;}
    if (param->coef23 >= 0x4000) { param->coef23 = param->coef23 - 0x8000;}
}

void CscInt2Reg(struct csc_param_unit *param, int type, int reg[3][4])
{
	int max_abs;
	int i,j;
	int scale_correct;

    if (param->coef00<0) { reg[0][0] = - param->coef00;} else { reg[0][0] = param->coef00;}
    if (param->coef01<0) { reg[0][1] = - param->coef01;} else { reg[0][1] = param->coef01;}
    if (param->coef02<0) { reg[0][2] = - param->coef02;} else { reg[0][2] = param->coef02;}
    if (param->coef03<0) { reg[0][3] = - param->coef03;} else { reg[0][3] = param->coef03;}
    if (param->coef10<0) { reg[1][0] = - param->coef10;} else { reg[1][0] = param->coef10;}
    if (param->coef11<0) { reg[1][1] = - param->coef11;} else { reg[1][1] = param->coef11;}
    if (param->coef12<0) { reg[1][2] = - param->coef12;} else { reg[1][2] = param->coef12;}
    if (param->coef13<0) { reg[1][3] = - param->coef13;} else { reg[1][3] = param->coef13;}
    if (param->coef20<0) { reg[2][0] = - param->coef20;} else { reg[2][0] = param->coef20;}
    if (param->coef21<0) { reg[2][1] = - param->coef21;} else { reg[2][1] = param->coef21;}
    if (param->coef22<0) { reg[2][2] = - param->coef22;} else { reg[2][2] = param->coef22;}
    if (param->coef23<0) { reg[2][3] = - param->coef23;} else { reg[2][3] = param->coef23;}

    max_abs = 0;
    for (i = 0 ; i < 3 ; i++) {
		for (j = 0 ; j < 4 ; j++)
			if (reg[i][j] > max_abs)
				max_abs = reg[i][j];
    }
    if (max_abs >= 16384)
		scale_correct = max_abs/16384;
	else
		scale_correct = 0;

    param->scale = param->scale + scale_correct;

    param->coef00 = param->coef00 >> scale_correct; 
    param->coef01 = param->coef01 >> scale_correct;
    param->coef02 = param->coef02 >> scale_correct;
    param->coef03 = param->coef03 >> scale_correct;
    param->coef10 = param->coef10 >> scale_correct;
    param->coef11 = param->coef11 >> scale_correct;
    param->coef12 = param->coef12 >> scale_correct;
    param->coef13 = param->coef13 >> scale_correct;
    param->coef20 = param->coef20 >> scale_correct;
    param->coef21 = param->coef21 >> scale_correct;
    param->coef22 = param->coef22 >> scale_correct;
    param->coef23 = param->coef23 >> scale_correct;

    if (param->coef00 < 0) { param->coef00 = 32768 + param->coef00;} 
    if (param->coef01 < 0) { param->coef01 = 32768 + param->coef01;}
    if (param->coef02 < 0) { param->coef02 = 32768 + param->coef02;}
    if (param->coef03 < 0) { param->coef03 = 32768 + param->coef03;}
    if (param->coef10 < 0) { param->coef10 = 32768 + param->coef10;}
    if (param->coef11 < 0) { param->coef11 = 32768 + param->coef11;}
    if (param->coef12 < 0) { param->coef12 = 32768 + param->coef12;}
    if (param->coef13 < 0) { param->coef13 = 32768 + param->coef13;}
    if (param->coef20 < 0) { param->coef20 = 32768 + param->coef20;}
    if (param->coef21 < 0) { param->coef21 = 32768 + param->coef21;}
    if (param->coef22 < 0) { param->coef22 = 32768 + param->coef22;}
    if (param->coef23 < 0) { param->coef23 = 32768 + param->coef23;}

    param->coef00 = param->coef00 & 0x7fff; 
    param->coef01 = param->coef01 & 0x7fff;
    param->coef02 = param->coef02 & 0x7fff;
    param->coef03 = param->coef03 & 0x7fff;
    param->coef10 = param->coef10 & 0x7fff;
    param->coef11 = param->coef11 & 0x7fff;
    param->coef12 = param->coef12 & 0x7fff;
    param->coef13 = param->coef13 & 0x7fff;
    param->coef20 = param->coef20 & 0x7fff;
    param->coef21 = param->coef21 & 0x7fff;
    param->coef22 = param->coef22 & 0x7fff;
    param->coef23 = param->coef23 & 0x7fff;

    if (type == YUV2RGB) {
        reg[0][0] = param->coef10;
        reg[0][1] = param->coef12;
        reg[0][2] = param->coef11;
        reg[0][3] = param->coef13;

        reg[1][0] = param->coef00;
        reg[1][1] = param->coef02;
        reg[1][2] = param->coef01;
        reg[1][3] = param->coef03;

        reg[2][0] = param->coef20;
        reg[2][1] = param->coef22;
        reg[2][2] = param->coef21;
        reg[2][3] = param->coef23;
    } else {
        reg[0][0] = param->coef00;
        reg[0][1] = param->coef01;
        reg[0][2] = param->coef02;
        reg[0][3] = param->coef03;

        reg[1][0] = param->coef10;
        reg[1][1] = param->coef11;
        reg[1][2] = param->coef12;
        reg[1][3] = param->coef13;

        reg[2][0] = param->coef20;
        reg[2][1] = param->coef21;
        reg[2][2] = param->coef22;
        reg[2][3] = param->coef23;
    }
}

void CalcDC(struct csc_param_unit *csc)
{
	csc->coef03 = (16<<(14 - csc->scale)) - csc->coef00*16 - csc->coef01*128 - csc->coef02*128;
	csc->coef13 = (16<<(14 - csc->scale)) - csc->coef10*16 - csc->coef11*128 - csc->coef12*128;
	csc->coef23 = (16<<(14 - csc->scale)) - csc->coef20*16 - csc->coef21*128 - csc->coef22*128;

	csc->coef03 = csc->coef03 >> 12;
	csc->coef13 = csc->coef13 >> 12;
	csc->coef23 = csc->coef23 >> 12;
}

void CalcBrightness(struct csc_param_unit *csc,int csctype, int brightness)
{
	brightness = (brightness-50) * 32 / 50;

	if(csc->scale > 2)
	   brightness = brightness >> (csc->scale - 2);
	else
	   brightness = brightness << (2 - csc->scale);

	if(csctype == YUV2RGB) {
	   csc->coef03 = csc->coef03 + brightness;
	   csc->coef13 = csc->coef13 + brightness;
	   csc->coef23 = csc->coef23 + brightness;
	} else {
	   csc->coef03 = csc->coef03 + brightness;
	}
}

void CalcSaturation(struct csc_param_unit *csc,int csctype, int saturation)
{
	saturation = saturation*60/100+20;

	if (csctype == YUV2YUV) {
		csc->coef10 = csc->coef10 * saturation / 50 ;
		csc->coef11 = csc->coef11 * saturation / 50 ;
		csc->coef12 = csc->coef12 * saturation / 50 ;
		csc->coef20 = csc->coef20 * saturation / 50 ;
		csc->coef21 = csc->coef21 * saturation / 50 ;
		csc->coef22 = csc->coef22 * saturation / 50 ;
	} else {
		csc->coef01 = csc->coef01 * saturation / 50 ;
		csc->coef02 = csc->coef02 * saturation / 50 ;
		csc->coef11 = csc->coef11 * saturation / 50 ;
		csc->coef12 = csc->coef12 * saturation / 50 ;
		csc->coef21 = csc->coef21 * saturation / 50 ;
		csc->coef22 = csc->coef22 * saturation / 50 ;
	}
}

void CalcContrast(struct csc_param_unit *csc, int csctype, int contrast)
{
     contrast = contrast*60/100+20;
     csc->coef00 = csc->coef00 * contrast / 50 ;
     csc->coef01 = csc->coef01 * contrast / 50 ;
     csc->coef02 = csc->coef02 * contrast / 50 ;
     csc->coef10 = csc->coef10 * contrast / 50 ;
     csc->coef11 = csc->coef11 * contrast / 50 ;
     csc->coef12 = csc->coef12 * contrast / 50 ;
     csc->coef20 = csc->coef20 * contrast / 50 ;
     csc->coef21 = csc->coef21 * contrast / 50 ;
     csc->coef22 = csc->coef22 * contrast / 50 ;
}

int GetColourPrimariesId(int std)
{
	switch(std) {
		case ITU_R_BT_709_6     : return 0;
		case ITU_R_BT_470_6_M   : return 1;
		case ITU_R_BT_601_7_625 : return 2;
		case ITU_R_BT_601_7_525 : return 3;
		case SMPTE_ST_240       : return 4;
		case GENERICFILM        : return 5;
		case ITU_R_BT_2020_2    : return 6;
		case SMPTE_RP_431_2     : return 7;
		case SMPTE_EG_432_1     : return 8;
		default: return 9;
	}
}

int* get_csc_params(int colorprimaries, HdmiFormat_t outFormat, int contrast, int brightness, int saturation, int rgb_flag)
{
	int i, j, k;
	int reg[3][4];
	struct csc_param_unit csc = {0};

	int ColorSpaceType = (rgb_flag ? YUV2RGB : YUV2YUV);
	int ColourPrimariesID = GetColourPrimariesId(colorprimaries);

	static int params[25];

	csc = csc_params[ColorSpaceType][outFormat][ColourPrimariesID];

	CscReg2Int(&csc);
	CalcContrast(&csc, ColorSpaceType, contrast);
	CalcSaturation(&csc, ColorSpaceType, saturation);
	CalcDC(&csc);
	CalcBrightness(&csc, ColorSpaceType, brightness);
	CscInt2Reg(&csc, ColorSpaceType, reg);

	params[0] = csc.scale;
	k = 1;
	for (i = 0 ; i < 3 ; i++) {
		for (j = 0 ; j < 4 ; j++) {
			params[k++] = ((reg[i][j]&0xff00) >> 8);
			params[k++] = ((reg[i][j]&0x00ff) >> 0);
		}
	}

	return params;
}

