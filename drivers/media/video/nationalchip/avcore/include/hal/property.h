#ifndef __AV_HAL_PROPERTY_H__
#define __AV_HAL_PROPERTY_H__

#include "hal/propertytypes.h"
#include "gxav_common.h"

extern int gxav_tee_ioctl(unsigned int cmd, void *data, size_t size);

#define AVHAL_MASK_MOD       (0x3f)
#define AVHAL_MASK_MODE      (0x3)
#define AVHAL_MASK_SIZE      (0xffff)
#define AVHAL_CNTL_NONE      (0x00000000)
#define AVHAL_CNTL_READ      (0x1)
#define AVHAL_CNTL_WRITE     (0x2)
#define AVHAL_CNTL_RW        (AVHAL_CNTL_READ | AVHAL_CNTL_WRITE)

#define AVHAL_CNTL(mode, mod, cmd, size) \
    ((unsigned long)((size & AVHAL_MASK_SIZE) << 16) | (((mode & AVHAL_MASK_MODE) << 14)  | (((mod & AVHAL_MASK_MOD) << 8) | (cmd))))

#define AVHAL_CNTL_MOD(x)    ((x >> 8)  & AVHAL_MASK_MOD)
#define AVHAL_CNTL_SIZE(x)   ((x >> 16)0xff & AVHAL_MASK_SIZE)
#define AVHAL_CNTL_MODE(x)   ((x) & AVHAL_CNTL_RW)
#define AVHAL_CNTL_CMD(x)    ((x) & 0xff)

#define AVHAL_CNTLN(mod,  cmd)     AVHAL_CNTL(AVHAL_CNTL_NONE,  mod, cmd, 0)
#define AVHAL_CNTLR(mod,  cmd, t)  AVHAL_CNTL(AVHAL_CNTL_READ,  mod, cmd, sizeof(t))
#define AVHAL_CNTLW(mod,  cmd, t)  AVHAL_CNTL(AVHAL_CNTL_WRITE, mod, cmd, sizeof(t))
#define AVHAL_CNTLRW(mod, cmd, t)  AVHAL_CNTL(AVHAL_CNTL_RW,    mod, cmd, sizeof(t))

// HDMI
#define AVHAL_HDMI_OPEN                   AVHAL_CNTLW (GXAV_MOD_HDMI, 0x1,  struct avhal_hdmi_property_open               )
#define AVHAL_HDMI_DETECT_HOTPLUG         AVHAL_CNTLRW(GXAV_MOD_HDMI, 0x2,  struct avhal_hdmi_property_detect_hotplug     )
#define AVHAL_HDMI_READ_EDID              AVHAL_CNTLRW(GXAV_MOD_HDMI, 0x3,  struct avhal_hdmi_property_read_edid          )
#define AVHAL_HDMI_AUDIO_CODES_GET        AVHAL_CNTLRW(GXAV_MOD_HDMI, 0x4,  struct avhal_hdmi_property_audio_codes_get    )
#define AVHAL_HDMI_AUDIOOUT_SET           AVHAL_CNTLW (GXAV_MOD_HDMI, 0x5,  struct avhal_hdmi_property_audioout_set       )
#define AVHAL_HDMI_AUDIOOUT_MUTE          AVHAL_CNTLW (GXAV_MOD_HDMI, 0x6,  struct avhal_hdmi_property_audioout_mute      )
#define AVHAL_HDMI_AUDIOOUT_RESET         AVHAL_CNTLW (GXAV_MOD_HDMI, 0x7,  struct avhal_hdmi_property_audioout_reset     )
#define AVHAL_HDMI_AV_MUTE                AVHAL_CNTLW (GXAV_MOD_HDMI, 0x8,  struct avhal_hdmi_property_av_mute            )
#define AVHAL_HDMI_POWERDOWN              AVHAL_CNTLW (GXAV_MOD_HDMI, 0x9,  struct avhal_hdmi_property_powerdown          )
#define AVHAL_HDMI_READY                  AVHAL_CNTLW (GXAV_MOD_HDMI, 0xa,  struct avhal_hdmi_property_ready              )
#define AVHAL_HDMI_AUDIOSAMPLE_CHANGE     AVHAL_CNTLW (GXAV_MOD_HDMI, 0xb,  struct avhal_hdmi_property_audiosample_change )
#define AVHAL_HDMI_ACR_ENABLE             AVHAL_CNTLW (GXAV_MOD_HDMI, 0xc,  struct avhal_hdmi_property_acr_enable         )
#define AVHAL_HDMI_BLACK_ENABLE           AVHAL_CNTLW (GXAV_MOD_HDMI, 0xd,  struct avhal_hdmi_property_black_enable       )
#define AVHAL_HDMI_VIDEOOUT_SET           AVHAL_CNTLW (GXAV_MOD_HDMI, 0xe,  struct avhal_hdmi_property_videoout_set       )
#define AVHAL_HDMI_VIDEOOUT_GET           AVHAL_CNTLRW(GXAV_MOD_HDMI, 0xf,  struct avhal_hdmi_property_videoout_get       )
#define AVHAL_HDMI_HDCP_ENABLE_AUTH       AVHAL_CNTLW (GXAV_MOD_HDMI, 0x11, struct avhal_hdmi_property_hdcp_enable_auth   )
#define AVHAL_HDMI_HDCP_DISABLE_AUTH      AVHAL_CNTLW (GXAV_MOD_HDMI, 0x12, struct avhal_hdmi_property_hdcp_disable_auth  )
#define AVHAL_HDMI_ENABLE                 AVHAL_CNTLW (GXAV_MOD_HDMI, 0x13, struct avhal_hdmi_property_enable             )
#define AVHAL_HDMI_SET_BRIGHTNESS         AVHAL_CNTLW (GXAV_MOD_HDMI, 0x14, struct avhal_hdmi_property_set_brightness     )
#define AVHAL_HDMI_SET_SATURATION         AVHAL_CNTLW (GXAV_MOD_HDMI, 0x15, struct avhal_hdmi_property_set_saturation     )
#define AVHAL_HDMI_SET_CONTRAST           AVHAL_CNTLW (GXAV_MOD_HDMI, 0x16, struct avhal_hdmi_property_set_contrast       )
#define AVHAL_HDMI_GET_VERSION            AVHAL_CNTLRW(GXAV_MOD_HDMI, 0x17, struct avhal_hdmi_property_get_version        )
#define AVHAL_HDMI_SET_ENCODING_OUT       AVHAL_CNTLW (GXAV_MOD_HDMI, 0x18, struct avhal_hdmi_property_set_encoding_out   )
#define AVHAL_HDMI_SET_CGMSA_PERMISSION   AVHAL_CNTLW (GXAV_MOD_HDMI, 0x19, struct avhal_hdmi_property_set_cgmsa_permission)






// CLOCK
#define AVHAL_CLOCK_COLD_RST                       AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x1, struct avhal_clock_property_cold_rst                       )
#define AVHAL_CLOCK_HOT_RST_SET                    AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x2, struct avhal_clock_property_hot_rst_set                    )
#define AVHAL_CLOCK_HOT_RST_CLR                    AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x3, struct avhal_clock_property_hot_rst_clr                    )
#define AVHAL_CLOCK_SETCLOCK                       AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x4, struct avhal_clock_property_setclock                       )
#define AVHAL_CLOCK_SOURCE_ENABLE                  AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x5, struct avhal_clock_property_source_enable                  )
#define AVHAL_CLOCK_AUDIOOUT_DACINSIDE             AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x6, struct avhal_clock_property_audioout_dacinside             )
#define AVHAL_CLOCK_AUDIOOUT_DACINSIDE_MUTE        AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x7, struct avhal_clock_property_audioout_dacinside_mute        )
#define AVHAL_CLOCK_AUDIOOUT_DACINSIDE_SLOW_ENABLE AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x8, struct avhal_clock_property_audioout_dacinside_slow_enable )
#define AVHAL_CLOCK_AUDIOOUT_DACINSIDE_SLOW_CONFIG AVHAL_CNTLW(GXAV_MOD_CLOCK, 0x9, struct avhal_clock_property_audioout_dacinside_slow_config )
#define AVHAL_CLOCK_MULTIPLEX_PINSEL               AVHAL_CNTLW(GXAV_MOD_CLOCK, 0xa, struct avhal_clock_property_multiplex_pinsel               )
#define AVHAL_CLOCK_SET_VIDEO_DAC_CLOCK_SOURCE     AVHAL_CNTLW(GXAV_MOD_CLOCK, 0xb, struct avhal_clock_property_set_video_dac_clock_source     )
#define AVHAL_CLOCK_MODULE_ENABLE                  AVHAL_CNTLW(GXAV_MOD_CLOCK, 0xc, struct avhal_clock_property_module_enable                  )
#define AVHAL_CLOCK_GETCONFIG                      AVHAL_CNTLW(GXAV_MOD_CLOCK, 0xd, struct avhal_clock_property_getconfig                      )






// Secure
#define AVHAL_SECURE_ENCRYPT                  AVHAL_CNTLW(GXAV_MOD_SECURE, 0x1, struct avhal_secure_property_encrypt            )
#define AVHAL_SECURE_DECRYPT                  AVHAL_CNTLW(GXAV_MOD_SECURE, 0x2, struct avhal_secure_property_decrypt            )
#define AVHAL_SECURE_GET_PROTECT_BUFFER       AVHAL_CNTLR(GXAV_MOD_SECURE, 0x3, struct avhal_secure_property_get_protect_buffer )
#define AVHAL_SECURE_QUERY_ACCESS_ALIGN       AVHAL_CNTLR(GXAV_MOD_SECURE, 0x4, struct avhal_secure_property_query_access_align )
#define AVHAL_SECURE_QUERY_PROTECT_ALIGN      AVHAL_CNTLR(GXAV_MOD_SECURE, 0x5, struct avhal_secure_property_query_protect_align)
#define AVHAL_SECURE_SET_HDCP_STATE           AVHAL_CNTLW(GXAV_MOD_SECURE, 0x6, struct avhal_secure_property_hdcp_state         )
#define AVHAL_SECURE_REGISTER_READ            AVHAL_CNTLW(GXAV_MOD_SECURE, 0x7, struct avhal_secure_property_register_read      )
#define AVHAL_SECURE_REGISTER_WRITE           AVHAL_CNTLW(GXAV_MOD_SECURE, 0x8, struct avhal_secure_property_register_write     )
#define AVHAL_SECURE_GET_HDR_STATUS           AVHAL_CNTLW(GXAV_MOD_SECURE, 0x9, struct avhal_secure_property_get_hdr            )
#define AVHAL_SECURE_GET_MACROVISION_STATUS   AVHAL_CNTLW(GXAV_MOD_SECURE, 0xa, struct avhal_secure_property_get_macrovision    )
#define AVHAL_SECURE_GET_HEVC_STATUS          AVHAL_CNTLW(GXAV_MOD_SECURE, 0xb, struct avhal_secure_property_get_hevc           )
#define AVHAL_SECURE_OTP_READ                 AVHAL_CNTLW(GXAV_MOD_SECURE, 0xc, struct avhal_secure_property_otp_read           )



#endif
