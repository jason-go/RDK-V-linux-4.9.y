#include "kernelcalls.h"
#include "log_printf.h"
#include "gxav_debug_level.h"

#if (GX_DEBUG_PRINTF_LEVEL > 0)

static enum log_level s_map_level[LOG_MAX_MOD];
static char s_map_level_init = 0;
static char *s_printf_buf = NULL;

static void _gxlog_init(void)
{
	if (!s_map_level_init) {
		unsigned char module = 0;

		for (module = 0; module < LOG_MAX_MOD; module++)
			s_map_level[module] = LOG_INFO|LOG_ERROR;
		s_printf_buf = gx_malloc(1024);
		s_map_level_init = 1;
	}
}

void gxav_config(enum log_level level, enum log_module module)
{
	if (module >= LOG_MAX_MOD)
		return;

	_gxlog_init();
	s_map_level[module] = level;
	return;
}

void gxav_printf(enum log_level level, enum log_module module, const char *fmt, ...)
{
	va_list args;
	const char *m_string = NULL;
	const char *l_string = NULL;

	if (module >= LOG_MAX_MOD)
		return;

	_gxlog_init();
	if (!(s_map_level[module] & level))
		return;

	switch (module) {
	case LOG_SDC:
		m_string = "sdc";
		break;
	case LOG_ADEC:
		m_string = "adec";
		break;
	case LOG_AOUT:
		m_string = "aout";
		break;
	case LOG_VDEC:
		m_string = "vdec";
		break;
	case LOG_VOUT:
		m_string = "vout";
		break;
	case LOG_HDMI:
		m_string = "hdmi";
		break;
	case LOG_VPU:
		m_string = "vpu";
		break;
	case LOG_DEMUX:
		m_string = "demux";
		break;
	case LOG_JDEC:
		m_string = "jdec";
		break;
	case LOG_STC:
		m_string = "stc";
		break;
	case LOG_MTC:
		m_string = "mtc";
		break;
	case LOG_ICAM:
		m_string = "icam";
		break;
	case LOG_IEMM:
		m_string = "iemm";
		break;
	case LOG_DESC:
		m_string = "desc";
		break;
	case LOG_GP:
		m_string = "gp";
		break;
	case LOG_DVR:
		m_string = "dvr";
		break;
	case LOG_CLK:
		m_string = "clk";
		break;
	case LOG_SCU:
		m_string = "scu";
		break;
	default:
		return;
	}

	switch (level) {
	case LOG_INFO:
		l_string = "info";
		break;
	case LOG_ERROR:
		l_string = "error";
		break;
	case LOG_WARNING:
		l_string = "warning";
		break;
	case LOG_DEBUG:
		l_string = "debug";
		break;
	default:
		return;
	}

	if (s_printf_buf != NULL) {
		s_printf_buf[0] = '\0';
		va_start(args, fmt);
		gx_vsprintf(s_printf_buf, fmt, args);
		va_end(args);
		gx_trace("[%s]-[%s]: %s", m_string, l_string, s_printf_buf);
	}

	return;
}

#endif

int gxav_debug_level_config(struct gxav_debug_config *config)
{
#if (GX_DEBUG_PRINTF_LEVEL > 0)
	enum log_module module;
	enum log_level  level;

	switch (config->module) {
	case GXAV_MOD_SDC:
		module = LOG_SDC;
		break;
	case GXAV_MOD_HDMI:
		module = LOG_HDMI;
		break;
	case GXAV_MOD_DEMUX:
		module = LOG_DEMUX;
		break;
	case GXAV_MOD_VIDEO_DECODE:
		module = LOG_VDEC;
		break;
	case GXAV_MOD_AUDIO_DECODE:
		module = LOG_ADEC;
		break;
	case GXAV_MOD_VPU:
		module = LOG_VPU;
		break;
	case GXAV_MOD_AUDIO_OUT:
		module = LOG_AOUT;
		break;
	case GXAV_MOD_VIDEO_OUT:
		module = LOG_VOUT;
		break;
	case GXAV_MOD_JPEG_DECODER:
		module = LOG_JDEC;
		break;
	case GXAV_MOD_STC:
		module = LOG_STC;
		break;
	case GXAV_MOD_MTC:
		module = LOG_MTC;
		break;
	case GXAV_MOD_ICAM:
		module = LOG_ICAM;
		break;
	case GXAV_MOD_IEMM:
		module = LOG_IEMM;
		break;
	case GXAV_MOD_DESCRAMBLER:
		module = LOG_DESC;
		break;
	case GXAV_MOD_GP:
		module = LOG_GP;
		break;
	case GXAV_MOD_DVR:
		module = LOG_DVR;
		break;
	case GXAV_MOD_CLOCK:
		module = LOG_CLK;
		break;
	case GXAV_MOD_SECURE:
		module = LOG_SCU;
		break;
	default:
		return -1;
	}

	switch (config->level) {
		case GXAV_DEBUG_LEVEL_0:
			level = 0;
			break;
		case GXAV_DEBUG_LEVEL_1:
			level = LOG_ERROR;
			break;
		case GXAV_DEBUG_LEVEL_2:
			level = LOG_ERROR|LOG_INFO;
			break;
		case GXAV_DEBUG_LEVEL_3:
			level = LOG_ERROR|LOG_INFO|LOG_WARNING;
			break;
		case GXAV_DEBUG_LEVEL_4:
			level = LOG_ERROR|LOG_INFO|LOG_WARNING|LOG_DEBUG;
			break;
		default:
			return -1;
	}

	gxav_config(level, module);

#endif

	return 0;
}

