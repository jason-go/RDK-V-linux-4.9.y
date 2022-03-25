#ifndef __GX3201_HDMI_H__
#define __GX3201_HDMI_H__

#include "include/audio_common.h"
#include "hdmi_hal.h"
#include "gxav_vout_propertytypes.h"

#define HDMI_LEN_BKSV   0x05
#define HDMI_LEN_BCAP   0x01
#define HDMI_LEN_AINFO  0x01
#define HDMI_LEN_AN     0x08
#define HDMI_LEN_AKSV   0x05
#define HDMI_LEN_Ri     0x02
#define HDMI_LEN_Pj     0x01
#define HDMI_LEN_BSTA   0x02
#define HDMI_LEN_V      0x04
#define HDMI_OFST_BKSV  0x00
#define HDMI_OFST_BCAP  0x40
#define HDMI_OFST_AINFO 0x15
#define HDMI_OFST_AN    0x18
#define HDMI_OFST_AKSV  0x10
#define HDMI_OFST_Ri    0x08
#define HDMI_OFST_Pj    0x0a
#define HDMI_OFST_BSTA  0x41
#define HDMI_OFST_KFIFO 0x43
#define HDMI_OFST_V     0x20

#endif
