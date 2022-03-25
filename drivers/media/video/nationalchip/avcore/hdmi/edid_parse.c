#include "gx3211_hdmi.h"

typedef enum { 
	BLK_RESV,
	BLK_ADB,
	BLK_VDB,
	BLK_VSDB,
	BLK_SADB,
	BLK_VDDB,
	BLK_RESV1,
	BLK_UET,
} BlockType;

static u8 get_typeof_block(u8 code)
{
	u8 ret = 0;

	if ((code>>5) >= BLK_RESV && (code>>5) <= BLK_UET)
		ret = (code>>5);

	return ret;
}

static int video_block_parse(struct gxav_hdmidev *dev, u8 *data)
{
	int i, ret = 0;
	u8 data_len = 0;
	u8  video_id_code = 0;

	static struct {
		const char              *video_id_code_desc;
		GxVideoOutProperty_Mode gx_mode;
	} vic_tab[] = {
		{""                                  , GXAV_VOUT_NULL_MAX  },
		{"640x480p    60hz  4:3  1:1"        , GXAV_VOUT_NULL_MAX  },
		{"720x480p    60hz  4:3  8:9"        , GXAV_VOUT_NULL_MAX  },
		{"720x480p    60hz 16:9 32:27"       , GXAV_VOUT_480P      },
		{"1280x720p   60hz 16:9  1:1"        , GXAV_VOUT_720P_60HZ },
		{"1920x1080i  60hz 16:9  1:1"        , GXAV_VOUT_1080I_60HZ},
		{"720x480i    60hz  4:3  8:9"        , GXAV_VOUT_NULL_MAX  },
		{"720x480i    60hz 16:9 32:27"       , GXAV_VOUT_480I      },
		{"720x240p    60hz  4:3  4:9"        , GXAV_VOUT_NULL_MAX  },
		{"720x240p    60hz 16:9 16:27"       , GXAV_VOUT_NULL_MAX  },
		{"2880x480i   60hz  4:3  2:9-20:9"   , GXAV_VOUT_NULL_MAX  },
		{"2880x480i   60hz 16:9  8:27-80:27" , GXAV_VOUT_NULL_MAX  },
		{"2880x240p   60hz  4:3  1:9-10:9"   , GXAV_VOUT_NULL_MAX  },
		{"2880x240p   60hz 16:9  4:27-40:27" , GXAV_VOUT_NULL_MAX  },
		{"1440x480p   60hz  4:3  4:9"        , GXAV_VOUT_NULL_MAX  },
		{"1440x480p   60hz 16:9 16:27"       , GXAV_VOUT_NULL_MAX  },
		{"1920x1080p  60hz 16:9  1:1"        , GXAV_VOUT_1080P_60HZ},

		{"720x576p    50hz  4:3 16:15"       , GXAV_VOUT_NULL_MAX  },
		{"720x576p    50hz 16:9 64:45"       , GXAV_VOUT_576P      },
		{"1280x720p   50hz 16:9  1:1"        , GXAV_VOUT_720P_50HZ },
		{"1920x1080i  50hz 16:9  1:1"        , GXAV_VOUT_1080I_50HZ},
		{"720x576i    50hz  4:3 16:15"       , GXAV_VOUT_NULL_MAX  },
		{"720x576i    50hz 16:9 64:45"       , GXAV_VOUT_576I      },
		{"720x288p    50hz  4:3  8:15"       , GXAV_VOUT_NULL_MAX  },
		{"720x288p    50hz 16:9 32:45"       , GXAV_VOUT_NULL_MAX  },
		{"2880x576i   50hz  4:3  2:15-20:15" , GXAV_VOUT_NULL_MAX  },
		{"2880x576i   50hz 16:9 16:45-160:45", GXAV_VOUT_NULL_MAX  },
		{"2880x288p   50hz  4:3  1:15-10:15" , GXAV_VOUT_NULL_MAX  },
		{"2880x288p   50hz 16:9  8:45-80:45" , GXAV_VOUT_NULL_MAX  },
		{"1440x576p   50hz  4:3  8:15"       , GXAV_VOUT_NULL_MAX  },
		{"1440x576p   50hz 16:9 32:45"       , GXAV_VOUT_NULL_MAX  },
		{"1920x1080p  50hz 16:9  1:1"        , GXAV_VOUT_1080P_50HZ},

		{"1920x1080p  24hz 16:9  1:1"        , GXAV_VOUT_NULL_MAX  },
		{"1920x1080p  25hz 16:9  1:1"        , GXAV_VOUT_NULL_MAX  },
		{"1920x1080p  30hz 16:9  1:1"        , GXAV_VOUT_NULL_MAX  },

		{"2880x480p   60hz  4:3  2:9"        , GXAV_VOUT_NULL_MAX  },
		{"2880x480p   60hz 16:9  8:27"       , GXAV_VOUT_NULL_MAX  },
		{"2880x576p   50hz  4:3  4:15"       , GXAV_VOUT_NULL_MAX  },
		{"2880x576p   50hz 16:9 16:45"       , GXAV_VOUT_NULL_MAX  },
		{"1920x1080i  50hz 16:9  1:1"        , GXAV_VOUT_NULL_MAX  },
	};

	if (data[0] >> 5 != 0x02)
		goto out;
	data_len = data[0] & 0x1f;

	for (i = 0; i < data_len; i++) {
		video_id_code = data[i+1] & 0x7f;
		if (video_id_code < sizeof(vic_tab)/sizeof(vic_tab[0])) {
			//gx_printf("[%d] %s\n", video_id_code, vic_tab[video_id_code].video_id_code_desc);
			if (vic_tab[video_id_code].gx_mode != GXAV_VOUT_NULL_MAX)
				ret |= (1<<vic_tab[video_id_code].gx_mode);
		}
	}

out:
	dev->edid_info.tv_cap = ret;
	return 0;
}

static int audio_block_parse(struct gxav_hdmidev *dev, u8 *data)
{
	return 0;
}

static int vender_block_parse(struct gxav_hdmidev *dev, u8 *data)
{
	if (data[1] == 0x03 && data[2] == 0x0c && data[3] == 0x00) {
		dev->edid_info.is_hdmi = 1;
		//printf("\nYES, It is an HDMI device\n");
	} else {
		dev->edid_info.is_hdmi = 0;
	}

	return 0;
}

static int speaker_block_parse(struct gxav_hdmidev *dev, u8 *data)
{
	return 0;
}

static int dtc_block_parse(struct gxav_hdmidev *dev, u8 *data)
{
	return 0;
}

static int uet_block_parse(struct gxav_hdmidev *dev, u8 *data)
{
	return 0;
}

int edid_parse(struct gxav_hdmidev *dev, u8 *edid_data)
{
	unsigned ret = 0;
	u8 cnt = 0;
	u8 blk_len  = 0;
	u8 blk_type = 0;
	u8 dtd_start = 0;

	u8 *cea_861d = NULL, *block_data = NULL;

	if (edid_data != NULL) {
		cea_861d = edid_data;
		if (cea_861d[0] != 0x02 || cea_861d[1] != 0x03)
			goto out;

		dtd_start = cea_861d[2];
		cnt = 4;
		block_data = &cea_861d[4];
		do {
			blk_type = get_typeof_block(block_data[0]);
			blk_len  = block_data[0] & 0x1f;
			gx_printf("data[0] = %hhu\n", block_data[0]);
			switch(blk_type) {
			case BLK_ADB:
				audio_block_parse(dev, block_data);
				break;
			case BLK_VDB:
				video_block_parse(dev, block_data);
				break;
			case BLK_VSDB:
				vender_block_parse(dev, block_data);
				break;
			case BLK_SADB:
				speaker_block_parse(dev, block_data);
				break;
			case BLK_VDDB:
				dtc_block_parse(dev, block_data);
				break;
			case BLK_UET:
				uet_block_parse(dev, block_data);
				break;
			default:
				break;
			}

			cnt += (1 + blk_len);
			block_data = &cea_861d[cnt];
		} while(cnt < dtd_start);
	}

out:
	return ret;
}

#if 0
int main(int argc, char **argv)
{
	char edid[] = {
		0x02, 0x03, 0x20, 0xf1, 0x4d, 0x05, 0x02, 0x03, 0x04, 0x07, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 
		0x16, 0x1f, 0x23, 0x09, 0x57, 0x07, 0x83, 0x01, 0x00, 0x00, 0x65, 0x03, 0x0c, 0x00, 0x30, 0x00, 
		0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0x65, 0xcc, 0x21, 0x00, 
		0x00, 0x18, 0x01, 0x1d, 0x00, 0xbc, 0x52, 0xd0, 0x1e, 0x20, 0xb8, 0x28, 0x55, 0x40, 0x32, 0xcc, 
		0x31, 0x00, 0x00, 0x1e, 0x01, 0x1d, 0x80, 0xd0, 0x72, 0x1c, 0x16, 0x20, 0x10, 0x2c, 0x25, 0x80, 
		0x32, 0xcc, 0x31, 0x00, 0x00, 0x9e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e, 
		0x96, 0x00, 0x32, 0xcc, 0x31, 0x00, 0x00, 0x18, 0x8c, 0x0a, 0xd0, 0x90, 0x20, 0x40, 0x31, 0x20, 
		0x0c, 0x40, 0x55, 0x00, 0x32, 0xcc, 0x31, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf4,
	};

	edid_parse(struct gxav_hdmidev *dev, u8* edid);

	return 0;
}
#endif
