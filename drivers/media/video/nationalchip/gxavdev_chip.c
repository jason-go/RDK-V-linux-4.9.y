#include "gxavdev.h"
#include "avcore.h"
#include "kernelcalls.h"
#include "gxav_firmware.h"
#include "chip_init.hxx"
#include "firmware_init.hxx"

#define DECLEAR_FW(chip) \
extern int gxav_firmware_register_##chip##_audio_mpeg(void); \
extern int gxav_firmware_register_##chip##_audio_dra(void); \
extern int gxav_firmware_register_##chip##_audio_flac(void); \
extern int gxav_firmware_register_##chip##_audio_vorbis(void); \
extern int gxav_firmware_register_##chip##_audio_opus(void); \
extern int gxav_firmware_register_##chip##_video_avc(void); \
extern int gxav_firmware_register_##chip##_video_hevc(void); \
extern int gxav_firmware_register_##chip##_video_mpeg2(void); \
extern int gxav_firmware_register_##chip##_video_mpeg4(void); \
extern int gxav_firmware_register_##chip##_video_rv(void); \
extern int gxav_firmware_register_##chip##_video_avs(void); \
extern int gxav_firmware_register_##chip##_video_all(void); \
extern int gxav_firmware_register_##chip##_audio_all(void); \
extern int gxav_firmware_register_##chip##_video_all_irdeto(void); \
extern int gxav_firmware_register_##chip##_audio_all_irdeto(void); \
extern int gxav_firmware_register_##chip##_video_all_vmxplus(void); \
extern int gxav_firmware_register_##chip##_audio_all_vmxplus(void);

DECLEAR_FW(gx3211)
DECLEAR_FW(gx6605s)
DECLEAR_FW(sirius)
DECLEAR_FW(taurus)
DECLEAR_FW(gemini)

void gxav_chip_firmware_register(void)
{
#ifdef CONFIG_GEMINI
	gxav_chip_register_gemini();
#ifdef CONFIG_AV_FIRMWARE_VIDEO_ALL
	gxav_firmware_register_gemini_video_all();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_ALL
	gxav_firmware_register_gemini_audio_all();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_MPEG
	gxav_firmware_register_gemini_audio_mpeg();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_DRA
	gxav_firmware_register_gemini_audio_dra();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_FLAC
	gxav_firmware_register_gemini_audio_flac();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_VORBIS
	gxav_firmware_register_gemini_audio_vorbis();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_OPUS
	gxav_firmware_register_gemini_audio_opus();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_SBC
	gxav_firmware_register_gemini_audio_sbc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVC
	gxav_firmware_register_gemini_video_avc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_HEVC
	gxav_firmware_register_gemini_video_hevc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG2
	gxav_firmware_register_gemini_video_mpeg2();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG4
	gxav_firmware_register_gemini_video_mpeg4();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_RV
	gxav_firmware_register_gemini_video_rv();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVS
	gxav_firmware_register_gemini_video_avs();
#endif
#endif

#ifdef CONFIG_TAURUS
	gxav_chip_register_taurus();
#ifdef CONFIG_AV_FIRMWARE_VIDEO_ALL
	gxav_firmware_register_taurus_video_all();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_ALL
	gxav_firmware_register_taurus_audio_all();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_MPEG
	gxav_firmware_register_taurus_audio_mpeg();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_DRA
	gxav_firmware_register_taurus_audio_dra();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_FLAC
	gxav_firmware_register_taurus_audio_flac();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_VORBIS
	gxav_firmware_register_taurus_audio_vorbis();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_OPUS
	gxav_firmware_register_taurus_audio_opus();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_SBC
	gxav_firmware_register_taurus_audio_sbc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVC
	gxav_firmware_register_taurus_video_avc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_HEVC
	gxav_firmware_register_taurus_video_hevc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG2
	gxav_firmware_register_taurus_video_mpeg2();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG4
	gxav_firmware_register_taurus_video_mpeg4();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_RV
	gxav_firmware_register_taurus_video_rv();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVS
	gxav_firmware_register_taurus_video_avs();
#endif
#endif

#ifdef CONFIG_GX3211
	gxav_chip_register_gx3211();
#ifdef CONFIG_AV_FIRMWARE_VIDEO_ALL
	gxav_firmware_register_gx3211_video_all();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_ALL
	gxav_firmware_register_gx3211_audio_all();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_MPEG
	gxav_firmware_register_gx3211_audio_mpeg();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_DRA
	gxav_firmware_register_gx3211_audio_dra();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_FLAC
	gxav_firmware_register_gx3211_audio_flac();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_VORBIS
	gxav_firmware_register_gx3211_audio_vorbis();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_OPUS
	gxav_firmware_register_gx3211_audio_opus();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_SBC
	gxav_firmware_register_gx3211_audio_sbc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVC
	gxav_firmware_register_gx3211_video_avc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_HEVC
	gxav_firmware_register_gx3211_video_hevc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG2
	gxav_firmware_register_gx3211_video_mpeg2();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG4
	gxav_firmware_register_gx3211_video_mpeg4();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_RV
	gxav_firmware_register_gx3211_video_rv();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVS
	gxav_firmware_register_gx3211_video_avs();
#endif
#endif

#ifdef CONFIG_GX6605S
	gxav_chip_register_gx6605s();
#ifdef CONFIG_AV_FIRMWARE_AUDIO_MPEG
	gxav_firmware_register_gx6605s_audio_mpeg();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_DRA
	gxav_firmware_register_gx6605s_audio_dra();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_FLAC
	gxav_firmware_register_gx6605s_audio_flac();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_VORBIS
	gxav_firmware_register_gx6605s_audio_vorbis();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_OPUS
	gxav_firmware_register_gx6605s_audio_opus();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_SBC
	gxav_firmware_register_gx6605s_audio_sbc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVC
	gxav_firmware_register_gx6605s_video_avc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG2
	gxav_firmware_register_gx6605s_video_mpeg2();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG4
	gxav_firmware_register_gx6605s_video_mpeg4();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_RV
	gxav_firmware_register_gx6605s_video_rv();
#endif
#endif

#ifdef CONFIG_SIRIUS
	gxav_chip_register_sirius();
#ifdef CONFIG_AV_FIRMWARE_VIDEO_ALL_VMX
	gxav_firmware_register_sirius_video_all_vmxplus();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_ALL_VMX
	gxav_firmware_register_sirius_audio_all_vmxplus();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_ALL
	gxav_firmware_register_sirius_video_all_irdeto();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_ALL
	gxav_firmware_register_sirius_audio_all_irdeto();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_MPEG
	gxav_firmware_register_sirius_audio_mpeg();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_DRA
	gxav_firmware_register_sirius_audio_dra();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_FLAC
	gxav_firmware_register_sirius_audio_flac();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_VORBIS
	gxav_firmware_register_sirius_audio_vorbis();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_OPUS
	gxav_firmware_register_sirius_audio_opus();
#endif
#ifdef CONFIG_AV_FIRMWARE_AUDIO_SBC
	gxav_firmware_register_sirius_audio_sbc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVC
	gxav_firmware_register_sirius_video_avc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_HEVC
	gxav_firmware_register_sirius_video_hevc();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG2
	gxav_firmware_register_sirius_video_mpeg2();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_MPEG4
	gxav_firmware_register_sirius_video_mpeg4();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_RV
	gxav_firmware_register_sirius_video_rv();
#endif
#ifdef CONFIG_AV_FIRMWARE_VIDEO_AVS
	gxav_firmware_register_sirius_video_avs();
#endif
#endif
}

