#ifndef __SIRIUS_DEBUG_H__
#define __SIRIUS_DEBUG_H__

/* 是否开启通道录制功能，可以调试数据是否错误
 * linux：register -d <phys addr> -n <size> -o <file>
 * ecos : gdb
 *        dump binary memory <file> <start addr> <end addr>
 * 通道有: SRC_R0, SRC_R1, SRC_2, SRC_3
 */
#define DEBUG_AOUT_RECORD
#define DEBUG_AOUT_RECORD_SRC  SRC_R0
#define DEBUG_AOUT_RECORD_SIZE (512*1024)

/* AUDIO_OUT_I2S0_PTS_SYNC: R0 pts sync
 * AUDIO_OUT_I2S1_PTS_SYNC: R1 pts sync
 * AUDIO_OUT_SPD0_PTS_SYNC: R2 pts sync
 * AUDIO_OUT_SPD1_PTS_SYNC: R3 pts sync
 */
#define DEBUG_AOUT_PTS_SYNC (0)

#define PTS_SYNC_LOG(...) do {                          \
	if (body->debug || (debug_aout_sync & body->src)) { \
		gxlog_i(LOG_AOUT, __VA_ARGS__);                 \
	}                                                   \
} while(0)

#endif
