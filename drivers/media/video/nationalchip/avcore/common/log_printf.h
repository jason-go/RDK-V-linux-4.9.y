#ifndef __GX_LOG_PRINTF_H__
#define __GX_LOG_PRINTF_H__

enum log_module {
	LOG_SDC     =  0,
	LOG_ADEC    =  1,
	LOG_AOUT    =  2,
	LOG_VDEC    =  3,
	LOG_VOUT    =  4,
	LOG_HDMI    =  5,
	LOG_VPU     =  6,
	LOG_DEMUX   =  7,
	LOG_JDEC    =  8,
	LOG_STC     =  9,
	LOG_MTC     = 10,
	LOG_ICAM    = 11,
	LOG_IEMM    = 12,
	LOG_DESC    = 13,
	LOG_GP      = 14,
	LOG_DVR     = 15,
	LOG_CLK     = 16,
	LOG_SCU     = 17,
	LOG_MAX_MOD = 18,
};

enum log_level {
	LOG_ERROR   = (0x1 << 0),
	LOG_INFO    = (0x1 << 1),
	LOG_WARNING = (0x1 << 2),
	LOG_DEBUG   = (0x1 << 3)
};


#if (GX_DEBUG_PRINTF_LEVEL == 0)
#define gxav_config(level, module)  ((void)0)
#define gxlog_e(...)                 ((void)0)
#define gxlog_i(...)                 ((void)0)
#define gxlog_w(...)                 ((void)0)
#define gxlog_d(...)                 ((void)0)
#else
void gxav_config(enum log_level level, enum log_module module);
void gxav_printf(enum log_level level, enum log_module module, const char *fmt, ...);
#if (GX_DEBUG_PRINTF_LEVEL == 1)
#define gxlog_e(...)     gxav_printf(LOG_ERROR,   __VA_ARGS__)
#define gxlog_i(...)     ((void)0)
#define gxlog_w(...)     ((void)0)
#define gxlog_d(...)     ((void)0)
#endif
#if (GX_DEBUG_PRINTF_LEVEL == 2)
#define gxlog_e(...)     gxav_printf(LOG_ERROR,   __VA_ARGS__)
#define gxlog_i(...)     gxav_printf(LOG_INFO,    __VA_ARGS__)
#define gxlog_w(...)     ((void)0)
#define gxlog_d(...)     ((void)0)
#endif
#if (GX_DEBUG_PRINTF_LEVEL == 3)
#define gxlog_e(...)     gxav_printf(LOG_ERROR,   __VA_ARGS__)
#define gxlog_i(...)     gxav_printf(LOG_INFO,    __VA_ARGS__)
#define gxlog_w(...)     gxav_printf(LOG_WARNING, __VA_ARGS__)
#define gxlog_d(...)     ((void)0)
#endif
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
#define gxlog_e(...)     gxav_printf(LOG_ERROR,   __VA_ARGS__)
#define gxlog_i(...)     gxav_printf(LOG_INFO,    __VA_ARGS__)
#define gxlog_w(...)     gxav_printf(LOG_WARNING, __VA_ARGS__)
#define gxlog_d(...)     gxav_printf(LOG_DEBUG,   __VA_ARGS__)
#endif
#endif

#endif
