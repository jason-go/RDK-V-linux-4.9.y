#ifndef __GXSE_LOG_H__
#define __GXSE_LOG_H__

#include "gxse_debug_level.h"

typedef enum {
	GXSE_LOG_ERROR   = (1<<0),
	GXSE_LOG_INFO    = (1<<1),
	GXSE_LOG_WARNING = (1<<2),
	GXSE_LOG_DEBUG   = (1<<3),
} GxSeLogType;

#if (GX_DEBUG_PRINTF_LEVEL == 0) || defined (CPU_SCPU)
#define gxse_log_config(level, module) ((void)0)
#define gxlog_e(...)                   ((void)0)
#define gxlog_i(...)                   ((void)0)
#define gxlog_w(...)                   ((void)0)
#define gxlog_d(...)                   ((void)0)
#else
#ifndef TEE_OS
void gxse_log_config(GxSeLogType type, GxSeLogModule module);
void gxse_log_printf(const char *function, unsigned int line,
		GxSeLogType type, GxSeLogModule module, const char *fmt, ...);
#else
#define gxse_log_printf(func, line, level, module, fmt, args...) gx_printf(fmt, ##args)
#endif

#if (GX_DEBUG_PRINTF_LEVEL == 1)
#define gxlog_e(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_ERROR,   __VA_ARGS__)
#define gxlog_i(...)     ((void)0)
#define gxlog_w(...)     ((void)0)
#define gxlog_d(...)     ((void)0)
#endif
#if (GX_DEBUG_PRINTF_LEVEL == 2)
#define gxlog_e(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_ERROR,   __VA_ARGS__)
#define gxlog_i(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_INFO,    __VA_ARGS__)
#define gxlog_w(...)     ((void)0)
#define gxlog_d(...)     ((void)0)
#endif
#if (GX_DEBUG_PRINTF_LEVEL == 3)
#define gxlog_e(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_ERROR,   __VA_ARGS__)
#define gxlog_i(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_INFO,    __VA_ARGS__)
#define gxlog_w(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_WARNING, __VA_ARGS__)
#define gxlog_d(...)     ((void)0)
#endif
#if (GX_DEBUG_PRINTF_LEVEL >= 4)
#define gxlog_e(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_ERROR,   __VA_ARGS__)
#define gxlog_i(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_INFO,    __VA_ARGS__)
#define gxlog_w(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_WARNING, __VA_ARGS__)
#define gxlog_d(...)     gxse_log_printf(__func__, __LINE__, GXSE_LOG_DEBUG,   __VA_ARGS__)
#endif
#endif

int gxse_debug_level_config(GxSecureDebug *config);
#endif
