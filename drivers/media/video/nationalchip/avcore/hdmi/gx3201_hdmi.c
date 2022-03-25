#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gxav_hdcp_key.h"
#include "gxav_event_type.h"
#include "clock_hal.h"
#include "gx3201_hdmi.h"
#include "gx3201_clock_reg.h"
#include "hdmi_module.h"

#define HDMI_HDCP_INITED       0
#define HDMI_HDCP_READY        1
#define HDMI_HDCP_READ_BKSV0   2
#define HDMI_HDCP_READ_BCAP    3
#define HDMI_HDCP_WRITE_AINFO  4
#define HDMI_HDCP_PREP_AN      5
#define HDMI_HDCP_WRITE_AN     6
#define HDMI_HDCP_WRITE_AKSV   7
#define HDMI_HDCP_READ_BKSV1   8
#define HDMI_HDCP_SET_AUTH     9
#define HDMI_HDCP_READ_BRI     10
#define HDMI_HDCP_AUTH_WAIT    11
#define HDMI_HDCP_AUTH_DONE    12

struct hdcp_rd{
	unsigned char temp0;
	unsigned char temp1;
	unsigned char temp2;
	unsigned char temp3;
	unsigned char temp4;
};

struct hdcp_wr{
	unsigned char data0;
	unsigned char data1;
	unsigned char data2;
	unsigned char data3;
	unsigned char data4;
	unsigned char data5;
	unsigned char data6;
	unsigned char data7;
};
extern volatile struct clock_regs *gx3201_clock_reg;

static unsigned char gx3201_hdmi_edid_data[128*8];
static struct videoout_hdmi_edid gx3201_edid_save;
static unsigned int gx3201_hdmi_out_mode = HDMI_RGB_OUT;
static unsigned int  gx3201_hdmi_audio_set_save = 0; // channel|sanplefre|audio_source
static int hdmi_hotplug_in = 0;
static int hdmi_hdcp_status = HDMI_HDCP_AUTH_DONE;
static unsigned char hdmi_init_mask0 = 0;
static unsigned char hdmi_init_mask1 = 0;
static unsigned int hdmi_edid_read_done = 0;
static void gx3201_hdmi_audiosample_change(GxAudioSampleFre samplefre, unsigned int cnum);
static void require_read_edid(int edid_current);

static void gx3201_hdmi_write(unsigned char addr, unsigned char data)
{
	if (addr==0x0) {
		// R/W System control reg address 0x00
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 0);// clock =1
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<11), 0, 11);// address=0x00
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<3), data, 3);// write data
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 0);// clock =0
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1);// we=1
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 0);// clock =1
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 0);// clock =0
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1);// we=0
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 0);// clock =1
	} else {
		// R/W System control reg address 0x01~0xff
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<11), addr, 11);// address
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<11), addr, 11);// address
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<11), addr, 11);// address
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<11), addr, 11);// address
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<3), data, 3);	// write data
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<3), data, 3);	// write data
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<3), data, 3);	// write data
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<3), data, 3);	// write data
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1); // we=1
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1); // we=1
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1); // we=1
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1); // we=1
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1); // we=0
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1); // we=0
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1); // we=0
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 1); // we=0
	}
}

static unsigned char gx3201_hdmi_read(unsigned char addr)
{
	unsigned char read_val;

	if (addr!=0x80) {
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<11), addr, 11);// address
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 2);// edid=1
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 2);// edid=0
	} else {
		// R/W System control reg address 0x80
		REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<11), 0x80, 11);// address=0x80
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 2);// edid=1
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 2);// edid=0
	}

	read_val = (unsigned char)(REG_GET_VAL(&(gx3201_clock_reg->cfg_hdmi_base)));

	return read_val;
}

static void hdmi_read_edid_data(unsigned int seg_addr)
{
	unsigned int i;

	// R/W System control reg address 0x80
	REG_SET_FIELD(&(gx3201_clock_reg->cfg_hdmi_sel), (0xff<<11), 0x80, 11);

	for (i=0;i<128;i++) {
		REG_SET_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 2);// edid=1
		gx3201_hdmi_edid_data[seg_addr+i] = (unsigned char)(REG_GET_VAL(&(gx3201_clock_reg->cfg_hdmi_base)));
		REG_CLR_BIT(&(gx3201_clock_reg->cfg_hdmi_sel), 2);// edid=0
	}
}

static unsigned char hdmi_edid_check_sum(unsigned int seg_addr)
{
	unsigned char temp = 0;
	unsigned int i;

	for (i=0;i<128;i++) {
		temp = temp + gx3201_hdmi_edid_data[seg_addr + i];
	}

	return temp;
}

#define EDID_WORD ((edid_current % 2) ? 0x80 : 0x00)
#define EDID_SEG  ( edid_current / 2 )
#define EDID_EXT   gx3201_hdmi_edid_data[126]

static int gx3201_hdmi_read_edid(struct videoout_hdmi_edid *edid)
{
	return gx3201_edid_save.audio_codes;
}

static unsigned int gx3201_hdmi_audio_codes_get(void)
{
	return (gx3201_edid_save.audio_codes);
}

static int hdmi_video_set_mode(GxVideoOutProperty_Mode vout_mode)
{
	int hdmi_mode = -1;

	if (gx3201_hdmi_out_mode & HDMI_RGB_OUT) {
		hdmi_mode = HDMI_RGB_OUT;
	} else if (gx3201_hdmi_out_mode & HDMI_YUV_422) {
		hdmi_mode = HDMI_YUV_422;
	} else if (gx3201_hdmi_out_mode & HDMI_YUV_444) {
		hdmi_mode = HDMI_YUV_444;
	}

	switch(hdmi_mode){
	case HDMI_RGB_OUT:
		if ((GXAV_VOUT_480I == vout_mode) ||
				(GXAV_VOUT_576I == vout_mode)) {
			// TV
			gx3201_hdmi_write(0x64 ,0x00);
			gx3201_hdmi_write(0x65 ,0x00);
			// HDMI内部处理
			gx3201_hdmi_write(0x15,0x00);
			gx3201_hdmi_write(0x16,0x35);
			gx3201_hdmi_write(0x17,0x22);
			gx3201_hdmi_write(0xd3,0xa4);
			gx3201_hdmi_write(0xd4,0x10);
			gx3201_hdmi_write(0x3b,0x01);
		} else if ((GXAV_VOUT_480P == vout_mode) ||
				(GXAV_VOUT_576P == vout_mode) ||
				(GXAV_VOUT_720P_50HZ == vout_mode) ||
				(GXAV_VOUT_720P_60HZ == vout_mode) ||
				(GXAV_VOUT_1080I_50HZ == vout_mode)) {
			// TV
			gx3201_hdmi_write(0x64 ,0x00);
			gx3201_hdmi_write(0x65 ,0x00);
			// HDMI内部处理
			gx3201_hdmi_write(0x15,0x00);
			gx3201_hdmi_write(0x16,0x37);
			gx3201_hdmi_write(0x17,0x22);
			gx3201_hdmi_write(0xd3,0xa5);
			gx3201_hdmi_write(0xd4,0x00);
			gx3201_hdmi_write(0x3b,0x01);
		} else if ((GXAV_VOUT_1080I_60HZ == vout_mode) ||
				(GXAV_VOUT_1080P_50HZ == vout_mode) ||
				(GXAV_VOUT_1080P_60HZ == vout_mode)) {
			// TV
			gx3201_hdmi_write(0x64 ,0x00);
			gx3201_hdmi_write(0x65 ,0x58);
			// HDMI内部处理
			gx3201_hdmi_write(0x15,0x00);
			gx3201_hdmi_write(0x16,0x37);
			gx3201_hdmi_write(0x17,0x22);
			gx3201_hdmi_write(0xd3,0xa5);
			gx3201_hdmi_write(0xd4,0x00);
			gx3201_hdmi_write(0x3b,0x01);
		}
		break;
	case HDMI_YUV_444:
		if ((GXAV_VOUT_480I == vout_mode) ||
				(GXAV_VOUT_576I == vout_mode)) {
			// TV
			gx3201_hdmi_write(0x64 ,0x40);
			gx3201_hdmi_write(0x65 ,0x98);
			// HDMI内部处理
			gx3201_hdmi_write(0x15,0x00);
			gx3201_hdmi_write(0x16,0x75);
			gx3201_hdmi_write(0x17,0x22);
			gx3201_hdmi_write(0xd3,0xa0);
			gx3201_hdmi_write(0xd4,0x10);
			gx3201_hdmi_write(0x3b,0x00);
		} else if ((GXAV_VOUT_480P == vout_mode) ||
				(GXAV_VOUT_576P == vout_mode) ||
				(GXAV_VOUT_720P_50HZ == vout_mode) ||
				(GXAV_VOUT_720P_60HZ == vout_mode) ||
				(GXAV_VOUT_1080I_50HZ == vout_mode) ||
				(GXAV_VOUT_1080I_60HZ == vout_mode) ||
				(GXAV_VOUT_1080P_50HZ == vout_mode) ||
				(GXAV_VOUT_1080P_60HZ == vout_mode)) {
			// TV
			gx3201_hdmi_write(0x64 ,0x40);
			gx3201_hdmi_write(0x65 ,0x98);
			// HDMI内部处理
			gx3201_hdmi_write(0x15,0x00);
			gx3201_hdmi_write(0x16,0x77);
			gx3201_hdmi_write(0x17,0x22);
			gx3201_hdmi_write(0xd3,0xa1);
			gx3201_hdmi_write(0xd4,0x00);
			gx3201_hdmi_write(0x3b,0x00);
		}
		break;
	case HDMI_YUV_422:
		if ((GXAV_VOUT_480I == vout_mode) ||
				(GXAV_VOUT_576I == vout_mode)) {
			// TV
			gx3201_hdmi_write(0x64 ,0x20);
			gx3201_hdmi_write(0x65 ,0x98);
			// HDMI内部处理
			gx3201_hdmi_write(0x15,0x00);
			gx3201_hdmi_write(0x16,0xb5);
			gx3201_hdmi_write(0x17,0x22);
			gx3201_hdmi_write(0xd3,0xa0);
			gx3201_hdmi_write(0xd4,0x10);
			gx3201_hdmi_write(0x3b,0x00);
		} else if ((GXAV_VOUT_480P == vout_mode) ||
				(GXAV_VOUT_576P == vout_mode) ||
				(GXAV_VOUT_720P_50HZ == vout_mode) ||
				(GXAV_VOUT_720P_60HZ == vout_mode) ||
				(GXAV_VOUT_1080I_50HZ == vout_mode) ||
				(GXAV_VOUT_1080I_60HZ == vout_mode) ||
				(GXAV_VOUT_1080P_50HZ == vout_mode) ||
				(GXAV_VOUT_1080P_60HZ == vout_mode)) {
			// TV
			gx3201_hdmi_write(0x64 ,0x20);
			gx3201_hdmi_write(0x65 ,0x98);
			// HDMI内部处理
			gx3201_hdmi_write(0x15,0x00);
			gx3201_hdmi_write(0x16,0xb7);
			gx3201_hdmi_write(0x17,0x22);
			gx3201_hdmi_write(0xd3,0xa1);
			gx3201_hdmi_write(0xd4,0x00);
			gx3201_hdmi_write(0x3b,0x00);
		}
		break;
	default:
		break;
	}

	return 0;
}

int gx3201_hdmi_detect_hotplug(void)
{
	return hdmi_hotplug_in;
}

void gx3201_hdmi_clock_set(GxVideoOutProperty_Mode mode)
{
	gx3201_hdmi_write(0x93,0xf8);		// close EDID interrupt mask

	switch(mode){
	case GXAV_VOUT_480I:
	case GXAV_VOUT_576I:
	case GXAV_VOUT_480P:
	case GXAV_VOUT_576P:
		gx3201_hdmi_write(0x56,0x10);
		gx3201_hdmi_write(0x57,0x00);
		gx3201_hdmi_write(0x58,0x40);
		gx3201_hdmi_write(0x59,0xa8);
		gx3201_hdmi_write(0x5a,0x0e);
		gx3201_hdmi_write(0x5b,0x38);
		gx3201_hdmi_write(0x5c,0x0e);
		gx3201_hdmi_write(0x5d,0x31);
		gx3201_hdmi_write(0x5e,0x00);
		break;
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080I_60HZ:
		gx3201_hdmi_write(0x56,0x15);
		gx3201_hdmi_write(0x57,0x00);
		gx3201_hdmi_write(0x58,0x40);
		gx3201_hdmi_write(0x59,0xac);
		gx3201_hdmi_write(0x5a,0x0e);
		gx3201_hdmi_write(0x5b,0x3f);
		gx3201_hdmi_write(0x5c,0x0e);
		gx3201_hdmi_write(0x5d,0x30);
		gx3201_hdmi_write(0x5e,0x00);
		break;
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080P_60HZ:
		gx3201_hdmi_write(0x56,0x1a);
		gx3201_hdmi_write(0x57,0x00);
		gx3201_hdmi_write(0x58,0x40);
		gx3201_hdmi_write(0x59,0xac);
		gx3201_hdmi_write(0x5a,0x0e);
		gx3201_hdmi_write(0x5b,0x3f);
		gx3201_hdmi_write(0x5c,0x0e);
		gx3201_hdmi_write(0x5d,0x30);
		gx3201_hdmi_write(0x5e,0x00);
		break;
	default:
		break;
	}

	gx3201_hdmi_write(0x00,0x4d);
	gx_mdelay(2);


	gx3201_hdmi_write(0x00,0x45);
	gx_mdelay(2);

	gx3201_hdmi_write(0x00,0x85);
	gx3201_hdmi_write(0x45,0x00);

	if (gx3201_edid_save.audio_codes == 0)
		require_read_edid(0);
}

void gx3201_hdmi_videoout_set(GxVideoOutProperty_Mode vout_mode)
{
	gx3201_hdmi_write(0x93,0xf8); // close EDID interrupt mask
	gx3201_hdmi_write(0x5f ,0x06);// AVI set
	gx3201_hdmi_write(0x60 ,0x82);// InfoFrame Type
	gx3201_hdmi_write(0x61 ,0x02);// Version 1
	gx3201_hdmi_write(0x62 ,0x0d);// Length of AVI InfoFrame
	gx3201_hdmi_write(0x17,0x22); // wanglx
	gx_mdelay(2);
	hdmi_video_set_mode(vout_mode);

	switch(vout_mode)
	{
		case GXAV_VOUT_480I:
			gx3201_hdmi_write(0x66 ,0x10);
			gx3201_hdmi_write(0x67 ,0x06);
			gx3201_hdmi_write(0x68 ,0x01);
			gx3201_hdmi_write(0x30 ,0x33);
			gx3201_hdmi_write(0x31 ,0xb4);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x06);//htotal msb
			gx3201_hdmi_write(0x33 ,0x14);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x01);//hblank msb
			gx3201_hdmi_write(0x35 ,0xee);//hdlay lsb
			gx3201_hdmi_write(0x36 ,0x00);//hdlay msb
			gx3201_hdmi_write(0x37 ,0x7c);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0x0d);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x02);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x16);//vblank
			gx3201_hdmi_write(0x3e ,0x15);//vdlay
			gx3201_hdmi_write(0x3f ,0x03);//vduration
			break;
		case GXAV_VOUT_576I:
			gx3201_hdmi_write(0x66 ,0x10);
			gx3201_hdmi_write(0x67 ,0x15);
			gx3201_hdmi_write(0x68 ,0x01);
			gx3201_hdmi_write(0x30 ,0x03);
			gx3201_hdmi_write(0x31 ,0xC0);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x06);//htotal msb
			gx3201_hdmi_write(0x33 ,0x20);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x01);//hblank msb
			gx3201_hdmi_write(0x35 ,0x08);//d8);//hdlay lsb
			gx3201_hdmi_write(0x36 ,0x01);//hdlay msb
			gx3201_hdmi_write(0x37 ,0x7E);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0x71);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x02);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x19);//vblank
			gx3201_hdmi_write(0x3e ,0x14);//17);//16);//vdlay
			gx3201_hdmi_write(0x3f ,0x03);//vduration
			break;
		case GXAV_VOUT_480P:
			gx3201_hdmi_write(0x66 ,0x10);
			gx3201_hdmi_write(0x67 ,0x03);
			gx3201_hdmi_write(0x30 ,0x61);
			gx3201_hdmi_write(0x31 ,0x5a);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x03);//htotal msb
			gx3201_hdmi_write(0x33 ,0x8a);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x00);//hblank msb
			gx3201_hdmi_write(0x35 ,0x5a);//hdlay  lsb
			gx3201_hdmi_write(0x36 ,0x00);//hdlay  msb
			gx3201_hdmi_write(0x37 ,0x3e);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0x0d);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x02);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x2d);//vblank
			gx3201_hdmi_write(0x3e ,0x10);//vdlay
			gx3201_hdmi_write(0x3f ,0x06);//vdlay
			break;
		case GXAV_VOUT_576P:
			gx3201_hdmi_write(0x66 ,0x10);
			gx3201_hdmi_write(0x67 ,0x12);
			gx3201_hdmi_write(0x30 ,0x01);
			gx3201_hdmi_write(0x31 ,0x60);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x03);//htotal msb
			gx3201_hdmi_write(0x33 ,0x90);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x00);//hblank msb
			gx3201_hdmi_write(0x35 ,0x84);//5f);//0x65);//hdlay  lsb
			gx3201_hdmi_write(0x36 ,0x00);//hdlay  msb
			gx3201_hdmi_write(0x37 ,0x40);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0x71);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x02);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x31);//vblank
			gx3201_hdmi_write(0x3e ,0x2c);//vdlay
			gx3201_hdmi_write(0x3f ,0x05);//vdlay
			break;
		case GXAV_VOUT_720P_50HZ:
			gx3201_hdmi_write(0x66 ,0x10);
			gx3201_hdmi_write(0x67 ,0x13);

			gx3201_hdmi_write(0x30 ,0x0D);
			gx3201_hdmi_write(0x31 ,0xBC);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x07);//htotal msb
			gx3201_hdmi_write(0x33 ,0xBC);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x02);//hblank msb
			gx3201_hdmi_write(0x35 ,0x04);//1e);//0x28);//hdlay lsb
			gx3201_hdmi_write(0x36 ,0x01);//hdlay msb
			gx3201_hdmi_write(0x37 ,0x28);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0xEE);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x02);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x1E);//vblank
			gx3201_hdmi_write(0x3e ,0x19);//vdlay
			gx3201_hdmi_write(0x3f ,0x05);//vduration
			break;
		case GXAV_VOUT_720P_60HZ:
			gx3201_hdmi_write(0x66 ,0x10);
			gx3201_hdmi_write(0x67 ,0x04);

			gx3201_hdmi_write(0x30 ,0x0D);
			gx3201_hdmi_write(0x31 ,0x72);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x06);//htotal msb
			gx3201_hdmi_write(0x33 ,0x72);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x01);//hblank msb
			gx3201_hdmi_write(0x35 ,0x04);//1e);//0x29);//hdlay lsb
			gx3201_hdmi_write(0x36 ,0x01);//hdlay msb
			gx3201_hdmi_write(0x37 ,0x28);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0xEE);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x02);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x1E);//vblank
			gx3201_hdmi_write(0x3e ,0x19);//vdlay
			gx3201_hdmi_write(0x3f ,0x05);//vduration
			break;
		case GXAV_VOUT_1080I_50HZ:
			gx3201_hdmi_write(0x66 ,0x10);
			gx3201_hdmi_write(0x67 ,0x14);

			gx3201_hdmi_write(0x30 ,0x0F);
			gx3201_hdmi_write(0x31 ,0x50);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x0A);//htotal msb
			gx3201_hdmi_write(0x33 ,0xd0);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x02);//hblank msb
			gx3201_hdmi_write(0x35 ,0xc0);//d3);//0xD2);//hdlay lsb
			gx3201_hdmi_write(0x36 ,0x00);//hdlay msb
			gx3201_hdmi_write(0x37 ,0x2C);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0x65);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x04);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x16);//vblank
			gx3201_hdmi_write(0x3e ,0x14);//vdlay
			gx3201_hdmi_write(0x3f ,0x05);//vduration
			break;
		case GXAV_VOUT_1080I_60HZ:
			gx3201_hdmi_write(0x66 ,0x00);
			gx3201_hdmi_write(0x67 ,0x05);

			gx3201_hdmi_write(0x30 ,0x0F);
			gx3201_hdmi_write(0x31 ,0x98);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x08);//htotal msb
			gx3201_hdmi_write(0x33 ,0x18);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x01);//hblank msb
			gx3201_hdmi_write(0x35 ,0xc0);//D3);//hdlay lsb
			gx3201_hdmi_write(0x36 ,0x00);//hdlay msb
			gx3201_hdmi_write(0x37 ,0x2C);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0x65);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x04);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x16);//vblank
			gx3201_hdmi_write(0x3e ,0x14);//vdlay
			gx3201_hdmi_write(0x3f ,0x05);//vduration
			break;
		case GXAV_VOUT_1080P_50HZ:
			gx3201_hdmi_write(0x66 ,0x00);
			gx3201_hdmi_write(0x67 ,0x1f);


			gx3201_hdmi_write(0x30 ,0x0D);
			gx3201_hdmi_write(0x31 ,0x50);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x0A);//htotal msb
			gx3201_hdmi_write(0x33 ,0xD0);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x02);//hblank msb
			gx3201_hdmi_write(0x35 ,0xc0);//E1);//hdlay lsb
			gx3201_hdmi_write(0x36 ,0x00);//hdlay msb
			gx3201_hdmi_write(0x37 ,0x2C);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0x65);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x04);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x2D);//vblank
			gx3201_hdmi_write(0x3e ,0x29);//vdlay
			gx3201_hdmi_write(0x3f ,0x05);//vduration
			break;
		case GXAV_VOUT_1080P_60HZ:
			gx3201_hdmi_write(0x66 ,0x00);
			gx3201_hdmi_write(0x67 ,0x10);

			gx3201_hdmi_write(0x30 ,0x0D);
			gx3201_hdmi_write(0x31 ,0x98);//htotal lsb
			gx3201_hdmi_write(0x32 ,0x08);//htotal msb
			gx3201_hdmi_write(0x33 ,0x18);//hblank lsb
			gx3201_hdmi_write(0x34 ,0x01);//hblank msb
			gx3201_hdmi_write(0x35 ,0xc0);//E1);//hdlay lsb
			gx3201_hdmi_write(0x36 ,0x00);//hdlay msb
			gx3201_hdmi_write(0x37 ,0x2C);//hduration lsb
			gx3201_hdmi_write(0x38 ,0x00);//hduration msb
			gx3201_hdmi_write(0x39 ,0x65);//vtotal lsb
			gx3201_hdmi_write(0x3a ,0x04);//vtotal msb
			gx3201_hdmi_write(0x3d ,0x2D);//vblank
			gx3201_hdmi_write(0x3e ,0x29);//vdlay
			gx3201_hdmi_write(0x3f ,0x05);//vduration
			break;
		default:
			break;
	}
}

void gx3201_hdmi_audioout_set(unsigned int audio_source)
{
	int hdmi_vedio_mode_change = 0;

	if (audio_source==3) {// vedio hdmi set audio,make sure do not change the actial audio info
		audio_source = (gx3201_hdmi_audio_set_save&0xff);
		hdmi_vedio_mode_change = 1;
	} else {
		gx3201_hdmi_audio_set_save &=~0xff;
		gx3201_hdmi_audio_set_save |=(audio_source&0xff);
	}

	if (audio_source==0) {
		// AUDIO_OUT_SPEAKER
		gx3201_hdmi_write(0x01,0x00);
		gx3201_hdmi_write(0x02,0x10);// 32khz:0x10 || 44.1khz:0x18 || 48khz:0x18
		gx3201_hdmi_write(0x03,0x00);// 32khz:0x00 || 44.1khz:0x80 || 48khz:0x00
		gx3201_hdmi_write(0x0a,0x00);// mclk select & 256fs  //4-3bit I2S:0x0 || spdif:0x1
		// 2-0bit I2S_256fs:0x05 || I2S_384fs:0x06  || I2S_512fs:0x07
		gx3201_hdmi_write(0x0b,0x40);
		gx3201_hdmi_write(0x0c,0x04);// 1-0bit i2s:0||right:1||left:2
		// 5-2bit 0x1:up to 2ch ||0x3 :up to 4ch ||0x7 :up to 6ch ||0xf :up to 8ch
		gx3201_hdmi_write(0x0d,0x00);//valid dsd input
		gx3201_hdmi_write(0x10,0x00);//no swap
		gx3201_hdmi_write(0x11,0x0d);//v_bit=0;orign freq not define
		gx3201_hdmi_write(0x12,0x00);
		gx3201_hdmi_write(0x13,0x00);
		gx3201_hdmi_write(0x14,0x0b);// max word length 24,current valid 24
		// 3-0bit 16bit:0x2 || 18bit:0x4 || 20bit:0xa || 24bit:0xb
		gx3201_hdmi_write(0x15,(gx3201_hdmi_read(0x15)&0x0f)|0x30);// sample frequence 32K
		//7:4bit  32khz:0x3 || 44.1khz:0x0 || 48khz:0x2 || 96khz:0xa
	} else if (audio_source==1) {
		// AUDIO_OUT_SPDIF
		gx3201_hdmi_write(0x01,0x00);
		gx3201_hdmi_write(0x02,0x10);
		gx3201_hdmi_write(0x03,0x00);
		gx3201_hdmi_write(0x0a,0x08);// mclk select & 256fs
		gx3201_hdmi_write(0x0b,0x60);
		gx3201_hdmi_write(0x0c,0x04);// 2ch & standard i2s mode
		gx3201_hdmi_write(0x0d,0x00);// valid dsd input
		gx3201_hdmi_write(0x10,0x00);// no swap
		gx3201_hdmi_write(0x11,0x0d);// v_bit=0;orign freq not define
		gx3201_hdmi_write(0x12,0x00);
		gx3201_hdmi_write(0x13,0x00);
		gx3201_hdmi_write(0x14,0x0b);// source num=1;max word length 24,current valid 24
		gx3201_hdmi_write(0x15,(gx3201_hdmi_read(0x15)&0x0f)|0x30);// sample frequence 48K
	}

	if (hdmi_vedio_mode_change) // vedio hdmi set audio,make sure do not change the actial audio info
		gx3201_hdmi_audiosample_change((gx3201_hdmi_audio_set_save>>8)&0xff,(gx3201_hdmi_audio_set_save>>16)&0xff);
}

/*
 * set black screen for hdmi out
 */
static void gx3201_hdmi_black_enable(int enable)
{
	unsigned char temp0 = 0;

	if (enable) {
		temp0 = gx3201_hdmi_read(0x45);
		gx3201_hdmi_write(0x45, (temp0 | 0x01));
	} else {
		temp0 = gx3201_hdmi_read(0x45);
		gx3201_hdmi_write(0x45, (temp0 & 0xfe));
	}
}

static void gx3201_hdmi_audioout_mute(int enable)
{
	unsigned char temp0 = 0;

	if (enable) {
		temp0 = gx3201_hdmi_read(0x45);
		gx3201_hdmi_write(0x45, (temp0 | 0x02));
	} else {
		temp0 = gx3201_hdmi_read(0x45);
		gx3201_hdmi_write(0x45, (temp0 & 0xfd));
	}
}

static void gx3201_hdmi_cold_reset_set(void)
{
	gx_interrupt_mask(54);
	gxav_clock_hot_rst_set(MODULE_TYPE_HDMI);
}

static void gx3201_hdmi_mode_select(void)
{
	gx3201_hdmi_write(0x00,0x2d);
	gx_mdelay(2);
	gx3201_hdmi_write(0x00,0x21);
	gx_mdelay(2);
}

static void gx3201_hdmi_cold_reset_clr(void)
{
	gxav_clock_hot_rst_clr(MODULE_TYPE_HDMI);
	gx3201_hdmi_mode_select();
	gx_interrupt_unmask(54);
}

static int gx3201_hdmi_init(void)
{
	hdmi_hdcp_status = HDMI_HDCP_AUTH_DONE;
	hdmi_hotplug_in  = 0;
	hdmi_edid_read_done  = 0;
	return 0;
}

static void require_read_edid(int edid_current)
{
	gx3201_hdmi_write(0x92,0x06);// opend EDID interrupt mask
	gx3201_hdmi_write(0xc6,0x80);
	gx3201_hdmi_write(0xc5,EDID_WORD);// set EDID word address
	gx3201_hdmi_write(0xc4,EDID_SEG );// set EDID segment point
}

static int start_read_edid(int* edid_current)
{
	static unsigned char edid_size=0;
	static unsigned int tag_code=0,len1=0,len2=0,offset=0,d_offset=0,acode=0,audio_codes=0;

	if (*edid_current == 0) {
		edid_size = EDID_EXT;
		tag_code=0,len1=0,len2=0,offset=0,d_offset=0,acode=0,audio_codes=0;
		HDMI_DBG("EDID edid_size %d !!! \n",edid_size+1);
	}

	if ((*edid_current > 0)&&
			(gx3201_hdmi_edid_data[*edid_current * 128 + 0] == 0x02)&&
			(gx3201_hdmi_edid_data[*edid_current * 128 + 1] == 0x03)){
		gx3201_hdmi_out_mode = HDMI_RGB_OUT;
		if (gx3201_hdmi_edid_data[*edid_current * 128 + 3]&0x10) {
			gx3201_hdmi_out_mode |= HDMI_YUV_422;
		}

		if (gx3201_hdmi_edid_data[*edid_current * 128 + 3]&0x20) {
			gx3201_hdmi_out_mode |= HDMI_YUV_444;
		}

		d_offset = gx3201_hdmi_edid_data[*edid_current * 128 + 2];
		for (offset = 4;offset < d_offset;) {
			tag_code = gx3201_hdmi_edid_data[*edid_current * 128 + offset]&0xe0;
			len1 = gx3201_hdmi_edid_data[*edid_current * 128 + offset]&0x1f;
			HDMI_DBG("%d,%d,tag_code=%x,len1=%d======\n",offset,d_offset,tag_code,len1);
			if ((gx3201_hdmi_edid_data[*edid_current * 128 + offset]&0xe0) == 0x20) {
				offset+=1;
				len2 = len1 + offset;
				for (;offset<len2;offset+=3) {
					acode = (gx3201_hdmi_edid_data[*edid_current * 128 + offset]>>3)&0xf;
					if (acode==0x1)
						audio_codes |= EDID_AUDIO_LINE_PCM;
					else if (acode==0x2)
						audio_codes |= EDID_AUDIO_AC3;
					else if (acode==0xa)
						audio_codes |= EDID_AUDIO_EAC3;
					else if (acode==0x6)
						audio_codes |= EDID_AUDIO_AAC;
					else if (acode==0x7)
						audio_codes |= EDID_AUDIO_DTS;
				}

				if (len1==0) {
					audio_codes |= EDID_AUDIO_LINE_PCM;
					HDMI_DBG("EDID read audio, len1 = zero !!!!! \n");
				}
				gx3201_edid_save.audio_codes = audio_codes;
				HDMI_DBG("EDID read audio, %d ###### \n",audio_codes);
				return 0;
			} else
				offset +=(len1+1);
		}
	}

	if ((*edid_current < edid_size) && (*edid_current < 7)) {
		(*edid_current)++ ;
		return 1;
	} else {
		HDMI_DBG("hdmi edid error, cur = %d, size = %d\n", *edid_current, edid_size);
	}

	return 0;
}

int gx3201_hdmi_plug_interrupt(int isr_94)
{
	unsigned char val;
	static int edid_current = 0;
	static int edid_retry = 0;
	static int edid_read_init = 0;
	static int hdmi_msens_in = 0;
	int event = 0;

	gx3201_hdmi_write(0x92, 0x00);
	gx3201_hdmi_write(0x93, 0x00);
	gx3201_hdmi_write(0x96, 0x00);
	gx3201_hdmi_write(0x97, 0x00);

	gx_mdelay(1);

	if (isr_94 & 0xc0) {
		val = gx3201_hdmi_read(0xdf);
		HDMI_DBG("#df: %x\n", val);

		if (0x80 == (val & 0x80)) {
			if (hdmi_hotplug_in == 0) {
				HDMI_DBG("hdmi hotplug in\n");
				event |= EVENT_HDMI_HOTPLUG_IN;
			}
			hdmi_hotplug_in = 1;
		} else {
			if (hdmi_hotplug_in) {
				HDMI_DBG("hdmi hotplug out\n");
				event |= EVENT_HDMI_HOTPLUG_OUT;
			}
			hdmi_edid_read_done  = 0;
			hdmi_hotplug_in  = 0;
		}

		if (0x40 == (val & 0x40)) {
			hdmi_msens_in = 1;
		}else if(0x00 == (val & 0x80)) {
			hdmi_msens_in = 0;
		}

		if (0xc0 == (val & 0xc0)) {
			if(edid_read_init == 0) {
				edid_retry = 10;
				edid_current = 0;
				require_read_edid(edid_current);
				edid_read_init = 1;
			}
		} else {
			edid_read_init = 0;
			gx3201_edid_save.audio_codes = 0;
		}
	}

	gx3201_hdmi_write(0x94, 0xc0);
	if (edid_retry > 0) {
		if (isr_94 & 0x2) {
			if (edid_retry-- > 0) {// edid error
				HDMI_DBG("hdmi edid retry : %d\n", edid_retry);
				require_read_edid(edid_current);
			} else {
				HDMI_DBG("hdmi edid error\n");
			}
		} else if (isr_94 & 0x4) {// edid ready
			unsigned char temp;
			hdmi_read_edid_data(edid_current * 128);
			temp = hdmi_edid_check_sum(edid_current * 128);

			HDMI_DBG("temp %x edid_current %x\n", temp, edid_current);
			if (temp == 0x00) {
				int ret = start_read_edid(&edid_current);
				if (ret != 0) {
					require_read_edid(edid_current);
					edid_retry--;
				} else {// got edid
					HDMI_DBG("hdmi edid = %d\n", gx3201_edid_save.audio_codes);
					edid_retry = 10;
					edid_current = 0;
					hdmi_edid_read_done  = 1;
					event |= EVENT_HDMI_READ_EDID;
				}
			} else {
				edid_retry--;
				require_read_edid(edid_current);
			}
		}
	}
	gx3201_hdmi_write(0x94, 0x06);

	gx3201_hdmi_write(0x95, 0xFF);
	gx3201_hdmi_write(0x98, 0xFF);
	gx3201_hdmi_write(0x99, 0xFF);

	if (hdmi_msens_in == 0)
		gx3201_hdmi_write(0x92, 0xc6);
	else
		gx3201_hdmi_write(0x92, 0x86);

	return event;
}

static void gx3201_hdmi_audioout_reset(unsigned int flg)
{
	if (flg) {
		gx3201_hdmi_write(0x45,0x04);
	} else {
		gx3201_hdmi_write(0x45,0x00);
	}
}

static void gx3201_hdmi_audiosample_change(GxAudioSampleFre samplefre, unsigned int cnum)
{
	unsigned char hdmi_source;

	unsigned char regs_value[][4] = {
		{1, 0x18, 0x80, 0x00}, // AUDIO_SAMPLE_FRE_44KDOT1HZ
		{1, 0x18, 0x00, 0x20}, // AUDIO_SAMPLE_FRE_48KHZ
		{1, 0x10, 0x00, 0x30}, // AUDIO_SAMPLE_FRE_32KHZ
		{0,                 }, // AUDIO_SAMPLE_FRE_22KDOT05HZ,
		{0,                 }, // AUDIO_SAMPLE_FRE_24KHZ,
		{0,                 }, // AUDIO_SAMPLE_FRE_16KHZ,
		{1, 0x30, 0x00, 0xa0}, // AUDIO_SAMPLE_FRE_96KHZ,

		{1, 0x31, 0x00, 0x80}, // AUDIO_SAMPLE_FRE_88KDOT2HZ, 0x31, 0x00,
		{1, 0x40, 0x00, 0xe0}, // AUDIO_SAMPLE_FRE_128KHZ,
		{1, 0x62, 0x00, 0xc0}, // AUDIO_SAMPLE_FRE_176KDOT4HZ,
		{1, 0x60, 0x00, 0xe0}, // AUDIO_SAMPLE_FRE_192KHZ,
		{1, 0x20, 0x00, 0xb0}, // AUDIO_SAMPLE_FRE_64KHZ,//0
		{0,                 }, // AUDIO_SAMPLE_FRE_12KHZ,
		{0,                 }, // AUDIO_SAMPLE_FRE_11KDOT025HZ,
		{0,                 }, // AUDIO_SAMPLE_FRE_9KDOT6HZ,
		{0,                 }, // AUDIO_SAMPLE_FRE_8KHZ
	};

	if (samplefre >= AUDIO_SAMPLE_FRE_44KDOT1HZ && samplefre <= AUDIO_SAMPLE_FRE_8KHZ) {
		if (regs_value[samplefre][0] == 1) {
			gx3201_hdmi_write(0x02, regs_value[samplefre][1]);
			gx3201_hdmi_write(0x03, regs_value[samplefre][2]);
			gx_udelay(100);
			gx3201_hdmi_write(0x15, (gx3201_hdmi_read(0x15) & 0x0f) | regs_value[samplefre][3]);
			gx3201_hdmi_audio_set_save &=~(0xffff<<8);
			gx3201_hdmi_audio_set_save |=(samplefre&0xff)<<8;
			gx3201_hdmi_audio_set_save |=(cnum&0xff)<<16;
		}
	}

	hdmi_source = gx3201_hdmi_read(0x0a);
	if (0== ((hdmi_source>>3)&0x3)) {
		switch(cnum)
		{
		case AUDIO_SIGNAL_CHANNEL_NUM:
		case AUDIO_TWO_CHANNEL_NUM:
			gx3201_hdmi_write(0x0c,(gx3201_hdmi_read(0x0c)& ~(0xf<<2)) | (0x1<<2));
			gx3201_hdmi_write(0x5f ,0x08);// Audio InfoFrame
			gx3201_hdmi_write(0x60 ,0x84);// InfoFrame Type
			gx3201_hdmi_write(0x61 ,0x01);// Version 1
			gx3201_hdmi_write(0x62 ,0x0a);// Length of Audio InfoFrame
			gx3201_hdmi_write(0x64 ,0x01);// CT3 CT2 CT1 CT0 F13=0 CC2 CC1 CC0
			gx3201_hdmi_write(0x65 ,0x00);// F27=0 F26=0 F25=0 SF2 SF1 SF0 SS1 SS0
			gx3201_hdmi_write(0x66 ,0x00);// F37=0 F36=0 F35=0 F34=0 F33=0 F32=0 F31=0 F30=0
			gx3201_hdmi_write(0x67 ,0x00);// CA7 CA6 CA5 CA4 CA3 CA2 CA1 CA0
			gx3201_hdmi_write(0x68 ,0x00);// DM_INH LSV3 LSV2 LSV1 LSV0 F52=0 F51=0 F50=0
			gx3201_hdmi_write(0x69 ,0x00);// F67=0 F66=0 F65=0 F64=0 F63=0 F62=0 F61=0 F60=0
			gx3201_hdmi_write(0x6a ,0x00);// F77=0 F76=0 F75=0 F74=0 F73=0 F72=0 F71=0 F70=0
			gx3201_hdmi_write(0x6b ,0x00);// F87=0 F86=0 F85=0 F84=0 F83=0 F82=0 F81=0 F80=0
			gx3201_hdmi_write(0x6c ,0x00);// F97=0 F96=0 F95=0 F94=0 F93=0 F92=0 F91=0 F90=0
			gx3201_hdmi_write(0x6d ,0x00);// F107=0 F106=0 F105=0 F104=0 F103=0 F102=0 F101=0 F100=0
			break;
		case AUDIO_THREE_CHANNEL_NUM:
		case AUDIO_FOUR_CHANNEL_NUM:
			gx3201_hdmi_write(0x0c,(gx3201_hdmi_read(0x0c)& ~(0xf<<2)) | (0x3<<2));
			gx3201_hdmi_write(0x5f ,0x08);// Audio InfoFrame
			gx3201_hdmi_write(0x60 ,0x84);// InfoFrame Type
			gx3201_hdmi_write(0x61 ,0x01);// Version 1
			gx3201_hdmi_write(0x62 ,0x0a);// Length of Audio InfoFrame
			gx3201_hdmi_write(0x64 ,0x03);// CT3 CT2 CT1 CT0 F13=0 CC2 CC1 CC0
			gx3201_hdmi_write(0x65 ,0x00);// F27=0 F26=0 F25=0 SF2 SF1 SF0 SS1 SS0
			gx3201_hdmi_write(0x66 ,0x00);// F37=0 F36=0 F35=0 F34=0 F33=0 F32=0 F31=0 F30=0
			gx3201_hdmi_write(0x67 ,0x06);// CA7 CA6 CA5 CA4 CA3 CA2 CA1 CA0
			gx3201_hdmi_write(0x68 ,0x00);// DM_INH LSV3 LSV2 LSV1 LSV0 F52=0 F51=0 F50=0
			gx3201_hdmi_write(0x69 ,0x00);// F67=0 F66=0 F65=0 F64=0 F63=0 F62=0 F61=0 F60=0
			gx3201_hdmi_write(0x6a ,0x00);// F77=0 F76=0 F75=0 F74=0 F73=0 F72=0 F71=0 F70=0
			gx3201_hdmi_write(0x6b ,0x00);// F87=0 F86=0 F85=0 F84=0 F83=0 F82=0 F81=0 F80=0
			gx3201_hdmi_write(0x6c ,0x00);// F97=0 F96=0 F95=0 F94=0 F93=0 F92=0 F91=0 F90=0
			gx3201_hdmi_write(0x6d ,0x00);// F107=0 F106=0 F105=0 F104=0 F103=0 F102=0 F101=0 F100=0
			break;
		case AUDIO_FIVE_CHANNEL_NUM:
		case AUDIO_SIX_CHANNEL_NUM:
			gx3201_hdmi_write(0x0c,(gx3201_hdmi_read(0x0c)& ~(0xf<<2)) | (0x7<<2));
			gx3201_hdmi_write(0x5f ,0x08);// Audio InfoFrame
			gx3201_hdmi_write(0x60 ,0x84);// InfoFrame Type
			gx3201_hdmi_write(0x61 ,0x01);// Version 1
			gx3201_hdmi_write(0x62 ,0x0a);// Length of Audio InfoFrame
			gx3201_hdmi_write(0x64 ,0x05);// CT3 CT2 CT1 CT0 F13=0 CC2 CC1 CC0
			gx3201_hdmi_write(0x65 ,0x00);// F27=0 F26=0 F25=0 SF2 SF1 SF0 SS1 SS0
			gx3201_hdmi_write(0x66 ,0x00);// F37=0 F36=0 F35=0 F34=0 F33=0 F32=0 F31=0 F30=0
			gx3201_hdmi_write(0x67 ,0x0b);// CA7 CA6 CA5 CA4 CA3 CA2 CA1 CA0
			gx3201_hdmi_write(0x68 ,0x00);// DM_INH LSV3 LSV2 LSV1 LSV0 F52=0 F51=0 F50=0
			gx3201_hdmi_write(0x69 ,0x00);// F67=0 F66=0 F65=0 F64=0 F63=0 F62=0 F61=0 F60=0
			gx3201_hdmi_write(0x6a ,0x00);// F77=0 F76=0 F75=0 F74=0 F73=0 F72=0 F71=0 F70=0
			gx3201_hdmi_write(0x6b ,0x00);// F87=0 F86=0 F85=0 F84=0 F83=0 F82=0 F81=0 F80=0
			gx3201_hdmi_write(0x6c ,0x00);// F97=0 F96=0 F95=0 F94=0 F93=0 F92=0 F91=0 F90=0
			gx3201_hdmi_write(0x6d ,0x00);// F107=0 F106=0 F105=0 F104=0 F103=0 F102=0 F101=0 F100=0
			break;
		case AUDIO_SEVEN_CHANNEL_NUM:
		case AUDIO_EIGHT_CHANNEL_NUM:
			gx3201_hdmi_write(0x0c,(gx3201_hdmi_read(0x0c)& ~(0xf<<2)) | (0xf<<2));
			gx3201_hdmi_write(0x5f ,0x08);// Audio InfoFrame
			gx3201_hdmi_write(0x60 ,0x84);// InfoFrame Type
			gx3201_hdmi_write(0x61 ,0x01);// Version 1
			gx3201_hdmi_write(0x62 ,0x0a);// Length of Audio InfoFrame
			gx3201_hdmi_write(0x64 ,0x07);// CT3 CT2 CT1 CT0 F13=0 CC2 CC1 CC0
			gx3201_hdmi_write(0x65 ,0x00);// F27=0 F26=0 F25=0 SF2 SF1 SF0 SS1 SS0
			gx3201_hdmi_write(0x66 ,0x00);// F37=0 F36=0 F35=0 F34=0 F33=0 F32=0 F31=0 F30=0
			gx3201_hdmi_write(0x67 ,0x13);// CA7 CA6 CA5 CA4 CA3 CA2 CA1 CA0
			gx3201_hdmi_write(0x68 ,0x00);// DM_INH LSV3 LSV2 LSV1 LSV0 F52=0 F51=0 F50=0
			gx3201_hdmi_write(0x69 ,0x00);// F67=0 F66=0 F65=0 F64=0 F63=0 F62=0 F61=0 F60=0
			gx3201_hdmi_write(0x6a ,0x00);// F77=0 F76=0 F75=0 F74=0 F73=0 F72=0 F71=0 F70=0
			gx3201_hdmi_write(0x6b ,0x00);// F87=0 F86=0 F85=0 F84=0 F83=0 F82=0 F81=0 F80=0
			gx3201_hdmi_write(0x6c ,0x00);// F97=0 F96=0 F95=0 F94=0 F93=0 F92=0 F91=0 F90=0
			gx3201_hdmi_write(0x6d ,0x00);// F107=0 F106=0 F105=0 F104=0 F103=0 F102=0 F101=0 F100=0
			break;
		default:
			gx3201_hdmi_write(0x0c,(gx3201_hdmi_read(0x0c)& ~(0xf<<2)) | (0x1<<2));
			gx3201_hdmi_write(0x5f ,0x08);// Audio InfoFrame
			gx3201_hdmi_write(0x60 ,0x84);// InfoFrame Type
			gx3201_hdmi_write(0x61 ,0x01);// Version 1
			gx3201_hdmi_write(0x62 ,0x0a);// Length of Audio InfoFrame
			gx3201_hdmi_write(0x64 ,0x00);// CT3 CT2 CT1 CT0 F13=0 CC2 CC1 CC0
			gx3201_hdmi_write(0x65 ,0x00);// F27=0 F26=0 F25=0 SF2 SF1 SF0 SS1 SS0
			gx3201_hdmi_write(0x66 ,0x00);// F37=0 F36=0 F35=0 F34=0 F33=0 F32=0 F31=0 F30=0
			gx3201_hdmi_write(0x67 ,0x00);// CA7 CA6 CA5 CA4 CA3 CA2 CA1 CA0
			gx3201_hdmi_write(0x68 ,0x00);// DM_INH LSV3 LSV2 LSV1 LSV0 F52=0 F51=0 F50=0
			gx3201_hdmi_write(0x69 ,0x00);// F67=0 F66=0 F65=0 F64=0 F63=0 F62=0 F61=0 F60=0
			gx3201_hdmi_write(0x6a ,0x00);// F77=0 F76=0 F75=0 F74=0 F73=0 F72=0 F71=0 F70=0
			gx3201_hdmi_write(0x6b ,0x00);// F87=0 F86=0 F85=0 F84=0 F83=0 F82=0 F81=0 F80=0
			gx3201_hdmi_write(0x6c ,0x00);// F97=0 F96=0 F95=0 F94=0 F93=0 F92=0 F91=0 F90=0
			gx3201_hdmi_write(0x6d ,0x00);// F107=0 F106=0 F105=0 F104=0 F103=0 F102=0 F101=0 F100=0
			break;
		}
	} else if (1 == ((hdmi_source>>3)&0x3)) {
		if ((samplefre==AUDIO_SAMPLE_FRE_176KDOT4HZ)||(samplefre==AUDIO_SAMPLE_FRE_192KHZ))
			gx3201_hdmi_write(0x0a,0x0d);// mclk select & 256fs
		else
			gx3201_hdmi_write(0x0a,0x08);// mclk not select
		gx3201_hdmi_write(0x0c,(gx3201_hdmi_read(0x0c)& (~(0xf<<2))) | (0x1<<2));
		gx3201_hdmi_write(0x5f ,0x08);// Audio InfoFrame
		gx3201_hdmi_write(0x60 ,0x84);// InfoFrame Type
		gx3201_hdmi_write(0x61 ,0x01);// Version 1
		gx3201_hdmi_write(0x62 ,0x0a);// Length of Audio InfoFrame
		gx3201_hdmi_write(0x64 ,0x01);// CT3 CT2 CT1 CT0 F13=0 CC2 CC1 CC0
		gx3201_hdmi_write(0x65 ,0x00);// F27=0 F26=0 F25=0 SF2 SF1 SF0 SS1 SS0
		gx3201_hdmi_write(0x66 ,0x00);// F37=0 F36=0 F35=0 F34=0 F33=0 F32=0 F31=0 F30=0
		gx3201_hdmi_write(0x67 ,0x00);// CA7 CA6 CA5 CA4 CA3 CA2 CA1 CA0
		gx3201_hdmi_write(0x68 ,0x00);// DM_INH LSV3 LSV2 LSV1 LSV0 F52=0 F51=0 F50=0
		gx3201_hdmi_write(0x69 ,0x00);// F67=0 F66=0 F65=0 F64=0 F63=0 F62=0 F61=0 F60=0
		gx3201_hdmi_write(0x6a ,0x00);// F77=0 F76=0 F75=0 F74=0 F73=0 F72=0 F71=0 F70=0
		gx3201_hdmi_write(0x6b ,0x00);// F87=0 F86=0 F85=0 F84=0 F83=0 F82=0 F81=0 F80=0
		gx3201_hdmi_write(0x6c ,0x00);// F97=0 F96=0 F95=0 F94=0 F93=0 F92=0 F91=0 F90=0
		gx3201_hdmi_write(0x6d ,0x00);// F107=0 F106=0 F105=0 F104=0 F103=0 F102=0 F101=0 F100=0
	}
}

/* ======================================================================
 *                               HDCP
 * ====================================================================== */
// ======================================================================
//  HDCP KEY 排列： Total 314bytes
//                  5bytes KSV1 + 2bytes 0
//                + 5bytes KSV2 + 2bytes 0
//                + 280bytes KEY
//                + 20bytes HASH
//  HASH信息可能不会存于otp中，需要软件通过一定的算法计算出来，按照
//  以下的顺序填充到HDCP RAM中去
// ======================================================================

static void hdmi_dis_int(void)
{
	gx3201_hdmi_write(0x92, 0x00);
	gx3201_hdmi_write(0x93, 0x00);
	gx3201_hdmi_write(0x96, 0x00);
	gx3201_hdmi_write(0x97, 0x00);

	gx3201_hdmi_write(0x94, 0xff);
	gx3201_hdmi_write(0x95, 0xff);
	gx3201_hdmi_write(0x98, 0xff);
	gx3201_hdmi_write(0x99, 0xff);
}

static void hdmi_hdcp_write_byte(unsigned int addr , unsigned char data)
{
	REG_SET_VAL((volatile unsigned int *) (0xa030a1e8), ((addr << 16) + (data << 8)));
	gx_mdelay(1);
}

static int hdmi_hdcp_write_key(void)
{
	unsigned int  hdcp_key = 0;
	unsigned char *hdmi_hdcp_key    = 0;
	unsigned int  hdmi_hdcp_key_len = 0;
	unsigned int i = 0;

	gxav_hdcp_key_fecth(&hdcp_key, &hdmi_hdcp_key_len);
	hdmi_hdcp_key = (unsigned char*)hdcp_key;
	if ((hdmi_hdcp_key_len == 0) || (hdmi_hdcp_key == 0))
		return -1;

	for (i = 0; i <= hdmi_hdcp_key_len; i++)//write key to hdmi key buff
		hdmi_hdcp_write_byte(i, hdmi_hdcp_key[i]);

	return 0;
}

static void hdmi_setup_timers_4_sim(GxVideoOutProperty_Mode vout_mode)
{
	unsigned int t_100ms;
	unsigned int t_5sec ;
	unsigned int vid    ;
	unsigned int temp   ;

	t_100ms = 0;
	t_5sec = 0;

	switch(vout_mode){
	case GXAV_VOUT_480I: vid = 0x06; break;
	case GXAV_VOUT_576I: vid = 0x15; break;
	case GXAV_VOUT_480P: vid = 0x03; break;
	case GXAV_VOUT_576P: vid = 0x12; break;
	case GXAV_VOUT_720P_50HZ: vid = 0x13; break;
	case GXAV_VOUT_720P_60HZ: vid = 0x04; break;
	case GXAV_VOUT_1080I_50HZ: vid = 0x14; break;
	case GXAV_VOUT_1080I_60HZ: vid = 0x05; break;
	case GXAV_VOUT_1080P_50HZ: vid = 0x1f; break;
	case GXAV_VOUT_1080P_60HZ: vid = 0x10; break;
	default: vid = 0x14; break;
	}

	// Set to wait at least 100ms before R0 read after aksv write
	// interlace - 60Hz => 13, 50Hz => 11, 24Hz => 5
	// progressive - 60Hz => 7, 50Hz => 6, 24Hz => 3

	switch (vid)
	{
		// 60Hz progressive, 30Hz interlace =>
	case 1:		case 2:		case 3:		case 4:		case 8:
	case 9:		case 12:	case 13:	case 14:	case 15:
	case 16:	case 35:	case 36:
		t_100ms = 7;
		t_5sec = 301;
		break;

		// 60Hz interlace, 120Hz progressive
	case 5:		case 6:		case 7:		case 10:	case 11:
	case 47:	case 48:	case 49:
		t_100ms = 13;
		t_5sec = 601;
		break;

		// 50Hz progressive
	case 17:	case 18:	case 19:	case 23:	case 24:
	case 27:	case 28:	case 29:	case 30:	case 31:
	case 37:	case 38:
		t_100ms = 6;
		t_5sec = 251;
		break;

		// 50Hz interlace, 100Hz progressive
	case 20:	case 21:	case 22:	case 25:	case 26:
	case 39:	case 41:	case 42:	case 43:
		t_100ms = 11;
		t_5sec = 501;
		break;

		// 100Hz interlace, 200Hz progressive
	case 40:	case 44:	case 45:	case 52:	case 53:
		t_100ms = 21;
		t_5sec = 1001;
		break;

		// 24Hz/25Hz progressive
	case 32:
	case 33:
		t_100ms = 3;
		t_5sec = 126;
		break;

		// 30Hz progressive
	case 34:
		t_100ms = 4;
		t_5sec = 151;
		break;

		// 120Hz interlace, 240Hz progressive
	case 46:	case 50:	case 51:	case 56:	case 57:
		t_100ms = 25;
		t_5sec = 1201;
		break;

		// 200Hz interlace
	case 54:	case 55:
		t_100ms = 41;
		t_5sec = 2001;
		break;

		// 240Hz interlace
	case 58:	case 59:
		t_100ms = 49;
		t_5sec = 2451;
		break;

	default:
		t_100ms = 50;
		t_5sec = 2500;
		break;
	}

	// TEMP FIX : DELAY 20% for 5sec timer
	t_5sec = t_5sec * 6/5;

	// === CHIP 1 (SN) ===
	if (t_100ms > 15)
		t_100ms = 15; // max 4 bits
	if (t_5sec > 1023)
		t_5sec = 1023; // max 10 bits

	temp = ((t_5sec >> 2) & 0xC0) | t_100ms;
	gx3201_hdmi_write(0xC9, temp);
	// 5 sec timer
	temp = t_5sec & 0xFF;
	gx3201_hdmi_write(0xCA, temp);

	return ;
}

static void hdmi_en_hdcp_int(void)
{
	gx3201_hdmi_write(0x92, 0x00); // Disable other interrupt (hot plug, msens, vsync)
	gx3201_hdmi_write(0x93, 0xff); // Enable HDCP interrupt
	gx3201_hdmi_write(0xd0, 0x48); // Set blist chk skip
}

static void hdmi_en_hdcp_sof_int(unsigned char exp0,unsigned char exp1)
{
	gx3201_hdmi_write(0x96, exp0);
	gx3201_hdmi_write(0x97, exp1);
}

static void hdmi_dis_hdcp_sof_int(void)
{
	gx3201_hdmi_write(0x96, 0x00);
	gx3201_hdmi_write(0x97, 0x00);
}

static unsigned char hdmi_chk_sa_int_regs(unsigned char exp2,
		unsigned char exp3, unsigned char temp0, unsigned char temp1)
{
	unsigned char val_ok;

	if (temp0 & 0x80) {        HDMI_DBG("\n    sf_mode_ready    \n");
	} else if (temp0 & 0x40) { HDMI_DBG("\n    ri_ready    \n");
	} else if (temp0 & 0x20) { HDMI_DBG("\n    pj_ready    \n");
	} else if (temp0 & 0x10) { HDMI_DBG("\n    an_ready    \n");
	} else if (temp0 & 0x08) { HDMI_DBG("\n    sha1_ready    \n");
	} else if (temp0 & 0x04) { HDMI_DBG("\n    enc_en    \n");
	} else if (temp0 & 0x02) { HDMI_DBG("\n    enc_dis_avmute    \n");
	} else if (temp0 & 0x01) { HDMI_DBG("\n    enc_dis_no_avmute    \n");
	}

	if (temp1 & 0x80) {        HDMI_DBG("\n    i2c_ack    \n");
	} else if (temp1 & 0x40) { HDMI_DBG("\n    i2c_err_ack    \n");
	} else if (temp1 & 0x20) { HDMI_DBG("\n    ri_save_ready    \n");
	} else if (temp1 & 0x10) { HDMI_DBG("\n    pj_save_ready    \n");
	} else if (temp1 & 0x08) { HDMI_DBG("\n    fr_cnt_update    \n");
	} else if (temp1 & 0x04) { HDMI_DBG("\n    bad interrupt    \n");
	} else if (temp1 & 0x02) { HDMI_DBG("\n    bad interrupt    \n");
	} else if (temp1 & 0x01) { HDMI_DBG("\n    bad interrupt    \n");
	}

	val_ok = ((exp2 & temp0) != 0x00) | ((exp3 & temp1) != 0x00);

	return val_ok;
}

static void hdmi_hdcp_en_rd(unsigned char length, unsigned char offset, unsigned char no_read)
{
	hdmi_dis_int();
	hdmi_en_hdcp_sof_int(0x00,0xc0);
	gx3201_hdmi_write(0x9e, (length & 0xff));
	gx3201_hdmi_write(0xa0, (offset & 0xff));
	gx3201_hdmi_write(0xa1, ((no_read << 2) + 1));
}

static void hdmi_hdcp_rd_data(struct hdcp_rd* rd)
{
	if (rd) {
		rd->temp0 = gx3201_hdmi_read(0xa2);
		rd->temp1 = gx3201_hdmi_read(0xa3);
		rd->temp2 = gx3201_hdmi_read(0xa4);
		rd->temp3 = gx3201_hdmi_read(0xa5);
		rd->temp4 = gx3201_hdmi_read(0xa6);
	}
}

static unsigned int hdmi_check_bksv(unsigned int bksv0,
		unsigned int bksv1, unsigned int bksv2, unsigned int bksv3, unsigned int bksv4)
{
	unsigned int shift_cnt;
	unsigned char Hi_cnt = 0;

	for (shift_cnt=0;shift_cnt<=39;shift_cnt++) {
		if (0 == (shift_cnt>>3)) {
			if (bksv0%2) Hi_cnt++ ;
			bksv0 = bksv0 >> 1;
		} else if (1 == (shift_cnt>>3)) {
			if (bksv1%2) Hi_cnt++ ;
			bksv1 = bksv1 >> 1;
		} else if (2 == (shift_cnt>>3)) {
			if (bksv2%2) Hi_cnt++ ;
			bksv2 = bksv2 >> 1;
		} else if (3 == (shift_cnt>>3)) {
			if (bksv3%2) Hi_cnt++ ;
			bksv3 = bksv3 >> 1;
		} else if (4 == (shift_cnt>>3)) {
			if (bksv4%2) Hi_cnt++ ;
			bksv4 = bksv4 >> 1;
		}
	}

	if (20 == Hi_cnt)
		return 1;
	else
		return 0;
}

static void hdmi_hdcp_en_wr(unsigned char length, unsigned char offset)
{
	gx3201_hdmi_write(0x9e, (length & 0xff));
	gx3201_hdmi_write(0xa0, (offset & 0xff));

	hdmi_en_hdcp_sof_int(0x00, 0xc0);
	gx3201_hdmi_write(0xa1, 0x02);
}

static void hdmi_hdcp_wr_data(struct hdcp_wr* wr)
{
	if (wr) {
		gx3201_hdmi_write(0xa7, wr->data0);
		gx3201_hdmi_write(0xa8, wr->data1);
		gx3201_hdmi_write(0xa9, wr->data2);
		gx3201_hdmi_write(0xaa, wr->data3);
		gx3201_hdmi_write(0xab, wr->data4);
		gx3201_hdmi_write(0xac, wr->data5);
		gx3201_hdmi_write(0xad, wr->data6);
		gx3201_hdmi_write(0xae, wr->data7);
	}
}

static void hdmi_output_turn_on(void)
{
	unsigned char temp0 = 0;

	temp0 = gx3201_hdmi_read(0x45);
	gx3201_hdmi_write(0x45,(temp0 & 0xfc));// Open Video/Audiooutput
	temp0 = gx3201_hdmi_read(0x45);
	gx3201_hdmi_write(0x45,(temp0 | 0x02));// audio reset/release
	gx_mdelay(1);                   // followed by 1ms wait time
	gx3201_hdmi_write(0x45,(temp0 & 0xfd));// audio reset/release
}

static void hdmi_output_turn_off(void)
{
	unsigned char temp0 = 0;

	temp0 = gx3201_hdmi_read(0x45);
	gx3201_hdmi_write(0x45,(temp0|0x03));// DisableVideo/Audiooutput

	gx3201_hdmi_write(0x00,0x4d);// mode_d and PLLA/B reset
	gx_udelay(100);       // followed by 100us wait time
	gx3201_hdmi_write(0x00,0x49);// mode_d and PLLB reset
	gx_udelay(100);       // followed by 100us wait time
	gx3201_hdmi_write(0x00,0x41);// mode_d and PLLA/B reset release
	gx3201_hdmi_write(0x00,0x81);// PS moded -> e
}

static int gx3201_hdmi_hdcp_disable_auth(void)
{
	unsigned char temp0 = 0;

	hdmi_init_mask0 = gx3201_hdmi_read(0x92);
	hdmi_init_mask1 = gx3201_hdmi_read(0x93);
	hdmi_dis_int();
	hdmi_output_turn_off();

	temp0 = gx3201_hdmi_read(0xaf);
	gx3201_hdmi_write(0xaf, (temp0 | 0x08));
	gx3201_hdmi_write(0x9a, 0x00);

	hdmi_dis_hdcp_sof_int();
	gx3201_hdmi_write(0x92, hdmi_init_mask0);
	gx3201_hdmi_write(0x93, hdmi_init_mask1);
	hdmi_output_turn_on();

	hdmi_hdcp_status = HDMI_HDCP_AUTH_DONE;
	return 0;
}


#if 0
static int gx3201_hdmi_hdcp_enable_auth(GxVideoOutProperty_Mode vout_mode)
{
	if ((hdmi_hotplug_in == 0)||
			(hdmi_hdcp_status != HDMI_HDCP_AUTH_DONE))
		return 0;

	if (hdmi_edid_read_done == 0)
		return 0;

	hdmi_init_mask0 = gx3201_hdmi_read(0x92);
	hdmi_init_mask1 = gx3201_hdmi_read(0x93);

	gx3201_hdmi_write(0x9a, 0x00);

	hdmi_dis_int();

	hdmi_output_turn_off();

	hdmi_hdcp_write_key();

	hdmi_setup_timers_4_sim(vout_mode);

	hdmi_en_hdcp_int();

	gx3201_hdmi_write(0xcf, 0x01);

	hdmi_hdcp_status = HDMI_HDCP_READY;

	hdmi_en_hdcp_sof_int(0x80, 0x00);
	gx3201_hdmi_write(0x9a, 0x80);

	return 0;
}
#endif

static int hdmi_hdcp_check_ksv_right(unsigned char* data, int len)
{
	int i1 = 0, i2 = 0;
	int i, j;

	for (i = 0; i < len; i++) {
		for (j = 0; j < 8; j++) {
			if(data[i] & (1 << j))
				i1++;
			else
				i2++;
		}
	}

	HDMI_DBG("check ksv: i1 %d, i2 %d\n", i1, i2);
	return ((i1 == i2)?1:0);
}

#if 0
static int gx3201_hdmi_hdcp_start_auth(void)
{
	gx_mdelay(100);
	hdmi_hdcp_en_rd(HDMI_LEN_Ri, HDMI_OFST_Ri, 0);
	hdmi_hdcp_status = HDMI_HDCP_READ_BRI;
	return 0;
}
#endif

static int gx3201_hdmi_hdcp_interrupt(unsigned char isr_98, unsigned char isr_99)
{
	int event = 0, ret = 0;
	struct hdcp_rd rd;
	struct hdcp_wr wr;

	gx_memset(&wr, 0, sizeof(struct hdcp_wr));
	gx_memset(&rd, 0, sizeof(struct hdcp_rd));

	switch(hdmi_hdcp_status){
	case HDMI_HDCP_READY:
		{
			if (hdmi_chk_sa_int_regs(0x80, 0x00, isr_98, isr_99)) {
				hdmi_hdcp_en_rd(HDMI_LEN_BKSV, HDMI_OFST_BKSV, 0);
				hdmi_hdcp_status = HDMI_HDCP_READ_BKSV0;
			}
			break;
		}
	case HDMI_HDCP_READ_BKSV0:
		{
			if (hdmi_chk_sa_int_regs(0x00, 0xc0, isr_98, isr_99)) {
				hdmi_hdcp_rd_data(&rd);
				ret = hdmi_check_bksv(rd.temp0, rd.temp1, rd.temp2, rd.temp3, rd.temp4);
				if (ret == 0) {
					event |= EVENT_HDMI_HDCP_FAIL;
					goto hdcp_result;
				}

				HDMI_DBG ("SFA: Bksv = %02x%02x%02x%02x%02x\n", rd.temp4, rd.temp3, rd.temp2, rd.temp1, rd.temp0);

				hdmi_hdcp_en_rd(HDMI_LEN_BCAP, HDMI_OFST_BCAP, 0);
				hdmi_hdcp_status = HDMI_HDCP_READ_BCAP;
			}
			break;
		}
	case HDMI_HDCP_READ_BCAP:
		{
			if (hdmi_chk_sa_int_regs(0x00, 0xc0, isr_98, isr_99)) {
				unsigned char temp  = 0;

				hdmi_hdcp_rd_data(&rd);
				gx3201_hdmi_write(0xe0, rd.temp0); // Load Bcaps
				temp = gx3201_hdmi_read(0x9a);     // Set register
				if (rd.temp0 & 0x40) {
					gx3201_hdmi_write(0x9a, (temp | 0x04));
				}
				wr.data0 = 2;

				hdmi_hdcp_wr_data(&wr);
				hdmi_hdcp_en_wr(HDMI_LEN_AINFO, HDMI_OFST_AINFO);
				hdmi_hdcp_status = HDMI_HDCP_WRITE_AINFO;
			}
			break;
		}
	case HDMI_HDCP_WRITE_AINFO:
		{
			if (hdmi_chk_sa_int_regs(0x00, 0xc0, isr_98, isr_99)) {
				unsigned char temp  = 0;

				hdmi_dis_hdcp_sof_int();
				hdmi_en_hdcp_sof_int(0x10,0x00);
				temp = gx3201_hdmi_read(0x9a);
				gx3201_hdmi_write(0x9a,(temp | 0x10));
				hdmi_hdcp_status = HDMI_HDCP_PREP_AN;
			}
			break;
		}
	case HDMI_HDCP_PREP_AN:
		{
			if (hdmi_chk_sa_int_regs(0x10, 0x00, isr_98, isr_99)) {
				wr.data0 = gx3201_hdmi_read(0xe8);
				wr.data1 = gx3201_hdmi_read(0xe9);
				wr.data2 = gx3201_hdmi_read(0xea);
				wr.data3 = gx3201_hdmi_read(0xeb);
				wr.data4 = gx3201_hdmi_read(0xec);
				wr.data5 = gx3201_hdmi_read(0xed);
				wr.data6 = gx3201_hdmi_read(0xee);
				wr.data7 = gx3201_hdmi_read(0xef);

				HDMI_DBG ("SFA: An = %02x%02x%02x%02x%02x%02x%02x%02x \n",
						wr.data7, wr.data6, wr.data5, wr.data4, wr.data3, wr.data2, wr.data1, wr.data0);

				hdmi_hdcp_wr_data(&wr);
				hdmi_hdcp_en_wr(HDMI_LEN_AN, HDMI_OFST_AN);
				hdmi_hdcp_status = HDMI_HDCP_WRITE_AN;
			}
			break;
		}
	case HDMI_HDCP_WRITE_AN:
		{
			if (hdmi_chk_sa_int_regs(0x00, 0xc0, isr_98, isr_99)) {
				unsigned char data[5];

				hdmi_dis_hdcp_sof_int();
				gx3201_hdmi_write(0xd2, 0x10);

				data[0] = wr.data0 = gx3201_hdmi_read(0xbf);
				data[1] = wr.data1 = gx3201_hdmi_read(0xc0);
				data[2] = wr.data2 = gx3201_hdmi_read(0xc1);
				data[3] = wr.data3 = gx3201_hdmi_read(0xc2);
				data[4] = wr.data4 = gx3201_hdmi_read(0xc3);

				HDMI_DBG ("SFA: Aksv = %02x%02x%02x%02x%02x \n",
						wr.data4, wr.data3, wr.data2, wr.data1, wr.data0);

				if (hdmi_hdcp_check_ksv_right(data, 5) == 0) {
					event |= EVENT_HDMI_HDCP_FAIL;
					goto hdcp_result;
				}

				hdmi_hdcp_wr_data(&wr);
				hdmi_hdcp_en_wr(HDMI_LEN_AKSV, HDMI_OFST_AKSV);
				hdmi_hdcp_status = HDMI_HDCP_WRITE_AKSV;
			}
			break;
		}
	case HDMI_HDCP_WRITE_AKSV:
		{
			if (hdmi_chk_sa_int_regs(0x00, 0xc0, isr_98, isr_99)) {

				hdmi_dis_hdcp_sof_int();
				hdmi_hdcp_en_rd(HDMI_LEN_BKSV, HDMI_OFST_BKSV, 0);
				hdmi_hdcp_status = HDMI_HDCP_READ_BKSV1;
			}
			break;
		}
	case HDMI_HDCP_READ_BKSV1:
		{
			if (hdmi_chk_sa_int_regs(0x00, 0xc0, isr_98, isr_99)) {
				unsigned char temp = 0;
				unsigned char data[5];

				hdmi_hdcp_rd_data(&rd);// Read Bksv
				data[0] = rd.temp0;
				data[1] = rd.temp1;
				data[2] = rd.temp2;
				data[3] = rd.temp3;
				data[4] = rd.temp4;

				HDMI_DBG ("SFA: Bksv = %02x%02x%02x%02x%02x\n",
						rd.temp4, rd.temp3, rd.temp2, rd.temp1, rd.temp0);

				if (hdmi_hdcp_check_ksv_right(data, 5) == 0) {
					event |= EVENT_HDMI_HDCP_FAIL;
					goto hdcp_result;
				}

				// Load Bksv
				gx3201_hdmi_write(0xe3,rd.temp0);
				gx3201_hdmi_write(0xe4,rd.temp1);
				gx3201_hdmi_write(0xe5,rd.temp2);
				gx3201_hdmi_write(0xe6,rd.temp3);
				gx3201_hdmi_write(0xe7,rd.temp4);

				// Check blacklist
				// Start auth
				hdmi_en_hdcp_sof_int(0x40,0x00);// enable ri interrupt
				temp = gx3201_hdmi_read(0x9a);
				gx3201_hdmi_write(0x9a, (temp | 0x02));
				hdmi_hdcp_status = HDMI_HDCP_SET_AUTH;
			}
			break;
		}
	case HDMI_HDCP_SET_AUTH:
		{
			if (hdmi_chk_sa_int_regs(0x40, 0x00, isr_98, isr_99)) {
				event |= EVENT_HDMI_START_AUTH;
			}
			break;
		}
	case HDMI_HDCP_READ_BRI:
		{
			if (hdmi_chk_sa_int_regs(0x00, 0xc0, isr_98, isr_99)) {
				unsigned char temp0 = 0;
				unsigned char temp1 = 0;

				hdmi_hdcp_rd_data(&rd); // Read Ri
				temp0 = gx3201_hdmi_read(0xd5);
				temp1 = gx3201_hdmi_read(0xd6);// Get A_ri

				HDMI_DBG("SFA: B_Ri = %02x%02x\n", rd.temp1, rd.temp0);
				HDMI_DBG("SFA: A_Ri = %02x%02x\n", temp1, temp0);

				if (((rd.temp1 << 8) + rd.temp0) != ((temp1 <<8) + temp0)) {
					HDMI_DBG("Ri mismatch!\n");
					event |= EVENT_HDMI_HDCP_FAIL;
					goto hdcp_result;
				} else {
					HDMI_DBG("hdmi hdcp wait auth !\n");
					hdmi_en_hdcp_int();
					temp0 = gx3201_hdmi_read(0x9a);
					gx3201_hdmi_write(0x9a, (temp0|0x20)); // Set Auth bit
					hdmi_hdcp_status = HDMI_HDCP_AUTH_WAIT;
				}
			}
			break;
		}
	}

	return event;

hdcp_result:
	hdmi_dis_hdcp_sof_int();
	gx3201_hdmi_write(0x92, hdmi_init_mask0);
	gx3201_hdmi_write(0x93, hdmi_init_mask1);
	hdmi_output_turn_on();
	hdmi_hdcp_status = HDMI_HDCP_AUTH_DONE;
	return event;
}

static int gx3201_hdmi_interrupt(void)
{
	int event = 0;
	unsigned char val = gx3201_hdmi_read(0x00);
	unsigned char isr_type0 = gx3201_hdmi_read(0x94);
	unsigned char isr_type1 = gx3201_hdmi_read(0x95);
	unsigned char isr_type2 = gx3201_hdmi_read(0x98);
	unsigned char isr_type3 = gx3201_hdmi_read(0x99);

	HDMI_DBG("#00: %x, #92h: %x, #94h: %x, #95h: %x, #98h: %x, #99h: %x\n",
			val, gx3201_hdmi_read(0x92), isr_type0, isr_type1, isr_type2, isr_type3);

	if ((val & 0x01) == 0) {
		val |= 0x1;
		gx3201_hdmi_write(0x00, val); // H VAILD
	}

	if ((val & 0x10) == 0x10) {
		val &= ~(0x10);
		val |= 0x80;
		gx3201_hdmi_write(0x00, val); // H VAILD
	}

	if (isr_type0 && 0xff) {
		event |= gx3201_hdmi_plug_interrupt(isr_type0);
	}

	if (isr_type1 & 0x10) {
		if (hdmi_hdcp_status == HDMI_HDCP_AUTH_WAIT) {
			gx3201_hdmi_write(0x95, 0xff); // clear #95h interrupt

			hdmi_dis_hdcp_sof_int();
			gx3201_hdmi_write(0x92, hdmi_init_mask0);
			gx3201_hdmi_write(0x93, hdmi_init_mask1);
			hdmi_output_turn_on();
			HDMI_DBG("hdmi hdcp auth done\n");
			event |= EVENT_HDMI_HDCP_SUCCESS;
			hdmi_hdcp_status = HDMI_HDCP_AUTH_DONE;
		}
	}

	if ((isr_type2 & 0xff) || (isr_type3 &0xff)) {
		gx3201_hdmi_write(0x98, 0xff);
		gx3201_hdmi_write(0x99, 0xff);
		event |= gx3201_hdmi_hdcp_interrupt(isr_type2, isr_type3);
	}

	return event;
}

static int gx3201_hdmi_open(unsigned delay_ms)
{
	return 0;
}

static int gx3201_hdmi_uninit(void)
{
	return 0;
}

static void gx3201_hdmi_enable(int enable)
{
}

static int gx3201_hdmi_get_version(GxVideoOutProperty_HdmiVersion *version)
{
	version->hdmi_major    = 0x1;
	version->hdmi_minor    = 0x3;
	version->hdmi_revision = 0xa;
	version->hdcp_major    = 0x1;
	version->hdcp_minor    = 0x2;
	version->hdcp_revision = 0x0;

	return 0;
}

static struct gxav_hdmi_module gx3201_hdmi_ops = {
	.init               = gx3201_hdmi_init ,
	.uninit             = gx3201_hdmi_uninit ,
	.open               = gx3201_hdmi_open ,
	.detect_hotplug     = gx3201_hdmi_detect_hotplug ,
	.read_edid          = gx3201_hdmi_read_edid ,
	.audio_codes_get    = gx3201_hdmi_audio_codes_get ,
	.clock_set          = gx3201_hdmi_clock_set ,
	.audioout_set       = gx3201_hdmi_audioout_set ,
	.audioout_reset     = gx3201_hdmi_audioout_reset ,
	.audiosample_change = gx3201_hdmi_audiosample_change ,
	.black_enable       = gx3201_hdmi_black_enable ,
	.audioout_mute      = gx3201_hdmi_audioout_mute ,
	.videoout_set       = gx3201_hdmi_videoout_set ,
	//.hdcp_enable        = gx3201_hdmi_hdcp_enable_auth ,
	.hdcp_disable       = gx3201_hdmi_hdcp_disable_auth ,
	//.hdcp_start         = gx3201_hdmi_hdcp_start_auth ,
	.interrupt          = gx3201_hdmi_interrupt ,
	.enable             = gx3201_hdmi_enable ,
	.read               = gx3201_hdmi_read ,
	.write              = gx3201_hdmi_write ,
	.cold_reset_set     = gx3201_hdmi_cold_reset_set ,
	.cold_reset_clr     = gx3201_hdmi_cold_reset_clr ,
	.get_version        = gx3201_hdmi_get_version,
};


struct gxav_module_ops gx3201_hdmi_module = {
	.module_type = GXAV_MOD_HDMI,
	.count = 1,
	.irqs = {54, -1},
	.irq_names = {"hdmi"},
	.event_mask = 0xffffffff,
	.init = gx_hdmi_init,
	.cleanup = gx_hdmi_uninit,
	.open = gx_hdmi_open,
	.close = gx_hdmi_close,
	.set_property = gx_hdmi_set_property,
	.get_property = gx_hdmi_get_property,
	.interrupts[54] = gx_hdmi_interrupt,
	.priv = &gx3201_hdmi_ops,
};



