#include <kernelcalls.h>

#ifndef TEE_OS
#if (GX_DEBUG_PRINTF_LEVEL > 0)
static char s_map_level_init = 0;
static GxSeLogType s_map_level[GXSE_LOG_MOD_MAX];

static void _gxlog_init(void)
{
	if (!s_map_level_init) {
		unsigned char module = 0;
		for (module = 0; module < GXSE_LOG_MOD_MAX; module++) {
			switch (GX_DEBUG_PRINTF_LEVEL) {
			case GXSE_DEBUG_LEVEL_0:
				s_map_level[module] = 0;
				break;
			case GXSE_DEBUG_LEVEL_1:
				s_map_level[module] = GXSE_LOG_ERROR;
				break;
			case GXSE_DEBUG_LEVEL_2:
				s_map_level[module] = GXSE_LOG_ERROR | GXSE_LOG_INFO;
				break;
			case GXSE_DEBUG_LEVEL_3:
				s_map_level[module] = GXSE_LOG_ERROR | GXSE_LOG_INFO | GXSE_LOG_WARNING;
				break;
			case GXSE_DEBUG_LEVEL_4:
				s_map_level[module] = GXSE_LOG_ERROR | GXSE_LOG_INFO | GXSE_LOG_WARNING | GXSE_LOG_DEBUG;
				break;
			default:
				s_map_level[module] = GXSE_LOG_ERROR | GXSE_LOG_INFO;
				break;
			}
		}
		s_map_level_init = 1;
	}
}

void gxse_log_config(GxSeLogType l, GxSeLogModule m)
{
	if (m >= GXSE_LOG_MOD_MAX)
		return;

	_gxlog_init();
	s_map_level[m] = l;
	return;
}

void gxse_log_printf(const char *function, unsigned int line, GxSeLogType l, GxSeLogModule m, const char* fmt, ...)
{
	va_list args;
	char buf[512];
	char lChar;
	const char *mString = NULL;

	if (m >= GXSE_LOG_MOD_MAX)
		return;

	_gxlog_init();
	if (!(s_map_level[m] & l))
		return;

	switch (m) {
	case GXSE_LOG_MOD_DEV:
		mString = "DEV";
		break;
	case GXSE_LOG_MOD_HAL:
		mString = "HAL";
		break;
	case GXSE_LOG_MOD_KLM:
		mString = "KLM";
		break;
	case GXSE_LOG_MOD_CRYPTO:
		mString = "CIPHER";
		break;
	case GXSE_LOG_MOD_DGST:
		mString = "DGST";
		break;
	case GXSE_LOG_MOD_AKCIPHER:
		mString = "AKCIPHER";
		break;
	case GXSE_LOG_MOD_SECURE:
		mString = "SECURE";
		break;
	case GXSE_LOG_MOD_MISC_FIREWALL:
		mString = "MFW";
		break;
	case GXSE_LOG_MOD_MISC_CHIP:
		mString = "MCHIP";
		break;
	case GXSE_LOG_MOD_MISC_SCI:
		mString = "MSCI";
		break;
	case GXSE_LOG_MOD_MISC_OTP:
		mString = "MOTP";
		break;
	case GXSE_LOG_MOD_MISC_RNG:
		mString = "MRNG";
		break;
	default:
		return;
	}

	switch (l) {
	case GXSE_LOG_INFO:
		lChar = 'I';
		break;
	case GXSE_LOG_WARNING:
		lChar = 'W';
		break;
	case GXSE_LOG_ERROR:
		lChar = 'E';
		break;
	case GXSE_LOG_DEBUG:
		lChar = 'D';
		break;
	default:
		return;
	}

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	gx_printf("%c/[%s]-[%s:%d]: %s", lChar, mString, function, line, buf);

	return;
}
#endif

int gxse_debug_level_config(GxSecureDebug *config)
{
#if (GX_DEBUG_PRINTF_LEVEL > 0)
	GxSeLogType level = 0;

	if (NULL == config)
		return -1;

	if (config->module >= GXSE_LOG_MOD_MAX)
		return -1;

	switch (config->level) {
		case GXSE_DEBUG_LEVEL_0:
			level = 0;
			break;
		case GXSE_DEBUG_LEVEL_1:
			level = GXSE_LOG_ERROR;
			break;
		case GXSE_DEBUG_LEVEL_2:
			level = GXSE_LOG_ERROR | GXSE_LOG_INFO;
			break;
		case GXSE_DEBUG_LEVEL_3:
			level = GXSE_LOG_ERROR | GXSE_LOG_INFO | GXSE_LOG_WARNING;
			break;
		case GXSE_DEBUG_LEVEL_4:
			level = GXSE_LOG_ERROR | GXSE_LOG_INFO | GXSE_LOG_WARNING | GXSE_LOG_DEBUG;
			break;
		default:
			return -1;
	}

	gxse_log_config(level, config->module);
#endif
	(void)config;

	return 0;
}

#endif
