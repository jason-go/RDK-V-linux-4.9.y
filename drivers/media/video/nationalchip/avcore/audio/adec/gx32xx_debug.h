#ifndef __GX32XX_DEBUG_H__
#define __GX32XX_DEBUG_H__

/* 是否开启固件检测，用于检测固件代码是否被修改
 */
//#define DEBUG_CHECK_WORKBUF

/* 是否开启功能录制功能，可以调试数据是否错误
 * linux：register -d <phys addr> -n <size> -o <file>
 * ecos : gdb
 *        dump binary memory <file> <start addr> <end addr>
 */
#define DEBUG_ADEC_RECORD
#define DEBUG_ADEC_RECORD_SIZE (512*1024)

/* DEBUG_ADEC_FIND_PTS: 开启某个功能查询pts调试手段，在发现pts经常性出错时开启调试
 * DEBUG_ADEC_FIX_PTS : 开启某个功能修复pts调试手段，可以查看解码是否出错，pts是否跳变信息
 */
#define DEBUG_ADEC_FIND_PTS (0)  //DECODE_0_ID,...
#define DEBUG_ADEC_FIX_PTS  (0)  //DECODE_0_ID,...
#define DEBUG_ADEC_ORPRINT  (0)  //DECODE_0_ID,...

#define DECODE_ESA_LOG(...)      do {                                \
	if ((audiodec->dec_id == debug_adec_id) &&                       \
			((debug_adec_esa) ||                                     \
			 (audiodec->debug.value & AUDIODEC_DEBUG_DECODE_ESA))) { \
		gxlog_i(LOG_ADEC, __VA_ARGS__);                              \
	}                                                                \
} while(0)

#define DECODE_TASK_LOG(...)     do {                                \
	if ((audiodec->dec_id == debug_adec_id) &&                       \
			((debug_adec_task) ||                                    \
			 (audiodec->debug.value & AUDIODEC_DEBUG_DECODE_TASK))) {\
		gxlog_i(LOG_ADEC, __VA_ARGS__);                              \
	}                                                                \
} while(0)

#define BYPASS_ESA_LOG(...)      do {                                \
	if ((audiodec->dec_id == debug_adec_id) &&                       \
			((debug_adec_esa) ||                                     \
			 (audiodec->debug.value & AUDIODEC_DEBUG_BYPASS_ESA))) { \
		gxlog_i(LOG_ADEC, __VA_ARGS__);                              \
	}                                                                \
} while(0)

#define BYPASS_TASK_LOG(...)     do {                                \
	if ((audiodec->dec_id == debug_adec_id) &&                       \
			((debug_adec_task) ||                                    \
			 (audiodec->debug.value & AUDIODEC_DEBUG_BYPASS_TASK))) {\
		gxlog_i(LOG_ADEC, __VA_ARGS__);                              \
	}                                                                \
} while(0)

#define CONVERT_ESA_LOG(...)     do {                                \
	if ((audiodec->dec_id == debug_adec_id) &&                       \
			((debug_adec_esa) ||                                     \
			 (audiodec->debug.value & AUDIODEC_DEBUG_CONVERT_ESA))) {\
		gxlog_i(LOG_ADEC, __VA_ARGS__);                              \
	}                                                                \
} while(0)

#define CONVERT_TASK_LOG(...)    do {                                \
	if ((audiodec->dec_id == debug_adec_id) &&                       \
			((debug_adec_task) ||                                    \
			 (audiodec->debug.value & AUDIODEC_DEBUG_CONVERT_TASK))) { \
		gxlog_i(LOG_ADEC, __VA_ARGS__);                              \
	}                                                                \
} while(0)

#define FIND_PTS_LOG(...) do {                                                 \
	unsigned char flags = 0;                                                   \
	if (debug_adec_findpts & debug_type)                                       \
		flags = 1;                                                             \
	if (audiodec->debug.value & AUDIODEC_DEBUG_DECODE_FIND_PTS) {              \
		if ((debug_type == DECODE_0_ID) || (debug_type == DECODE_1_ID))        \
			flags = 1;                                                         \
	}                                                                          \
	if (audiodec->debug.value & AUDIODEC_DEBUG_BYPASS_FIND_PTS) {              \
		if (debug_type == BYPASS_0_ID)                                         \
			flags = 1;                                                         \
	}                                                                          \
	if (audiodec->debug.value & AUDIODEC_DEBUG_CONVERT_FIND_PTS) {             \
		if (debug_type == CONVERT_0_ID)                                        \
			flags = 1;                                                         \
	}                                                                          \
	if (flags) {                                                               \
		gxlog_i(LOG_ADEC, __VA_ARGS__);                                        \
	}                                                                          \
} while(0)


#define FIX_PTS_LOG(...) do {                                                  \
	unsigned char flags = 0;                                                   \
	if (debug_adec_fixpts & debug_type)                                        \
		flags = 1;                                                             \
	if (audiodec->debug.value & AUDIODEC_DEBUG_DECODE_FIX_PTS) {               \
		if ((debug_type == DECODE_0_ID) || (debug_type == DECODE_1_ID))        \
			flags = 1;                                                         \
	}                                                                          \
	if (audiodec->debug.value & AUDIODEC_DEBUG_BYPASS_FIX_PTS) {               \
		if (debug_type == BYPASS_0_ID)                                         \
			flags = 1;                                                         \
	}                                                                          \
	if (audiodec->debug.value & AUDIODEC_DEBUG_CONVERT_FIX_PTS) {              \
		if (debug_type == CONVERT_0_ID)                                        \
			flags = 1;                                                         \
	}                                                                          \
	if (flags) {                                                               \
		gxlog_i(LOG_ADEC, __VA_ARGS__);                                        \
	}                                                                          \
} while(0)

#endif
