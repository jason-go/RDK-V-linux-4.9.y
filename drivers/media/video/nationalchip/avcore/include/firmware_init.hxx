#ifndef GXFIRMWARE_INIT_H
#define GXFIRMWARE_INIT_H

#ifdef ECOS_OS
#define DEFINE_FIRMWARE(chip, name)                              \
    extern "C" int gxav_firmware_register_##chip##_##name(void); \
    class cyg_firmware_class_##chip##_##name {                   \
        public:                                                  \
            cyg_firmware_class_##chip##_##name(void) {           \
                gxav_firmware_register_##chip##_##name();        \
            }                                                    \
    }

#else
#define DEFINE_FIRMWARE(chip, name)                              \
    extern int gxav_firmware_register_##chip##_##name(void);

#define REGISTER_AUDIO_FIRMWARE(chip, name) do {                 \
        gxav_firmware_register_##chip##_audio_##name();          \
} while(0)
#define REGISTER_VIDEO_FIRMWARE(chip, name) do {                 \
        gxav_firmware_register_##chip##_video_##name();          \
} while(0)
#endif

#define DEFINE_CHIP_FIRMWARE(chip)       \
DEFINE_FIRMWARE(chip, audio_all       ); \
DEFINE_FIRMWARE(chip, audio_all_fw    ); \
DEFINE_FIRMWARE(chip, audio_dts       ); \
DEFINE_FIRMWARE(chip, audio_dra       ); \
DEFINE_FIRMWARE(chip, audio_mpeg      ); \
DEFINE_FIRMWARE(chip, audio_mpeg_mini ); \
DEFINE_FIRMWARE(chip, audio_ra_aac    ); \
DEFINE_FIRMWARE(chip, audio_ra_ra8lbr ); \
DEFINE_FIRMWARE(chip, audio_flac      ); \
DEFINE_FIRMWARE(chip, audio_vorbis    ); \
DEFINE_FIRMWARE(chip, audio_opus      ); \
DEFINE_FIRMWARE(chip, audio_sbc       ); \
DEFINE_FIRMWARE(chip, audio_fov       ); \
DEFINE_FIRMWARE(chip, video_all       ); \
DEFINE_FIRMWARE(chip, video_avc       ); \
DEFINE_FIRMWARE(chip, video_avs       ); \
DEFINE_FIRMWARE(chip, video_hevc      ); \
DEFINE_FIRMWARE(chip, video_mpeg2     ); \
DEFINE_FIRMWARE(chip, video_mpeg4     ); \
DEFINE_FIRMWARE(chip, video_rv        );

DEFINE_CHIP_FIRMWARE(gx3201)
DEFINE_CHIP_FIRMWARE(gx3211)
DEFINE_CHIP_FIRMWARE(gx3113c)
DEFINE_CHIP_FIRMWARE(gx6605s)
DEFINE_CHIP_FIRMWARE(taurus)
DEFINE_CHIP_FIRMWARE(gemini)
DEFINE_CHIP_FIRMWARE(sirius)
DEFINE_CHIP_FIRMWARE(cygnus)
#endif

