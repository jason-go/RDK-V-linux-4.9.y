#ifndef __GX3201_STC_REG_H__
#define __GX3201_STC_REG_H__

#include "gxav_bitops.h"

#define STC_DTO             (108000000)
#define STC_BASE            (0x4003560)
#define STC_UNIT            (1)

#define BIT_STC_ENABLE      (16)
#define BIT_STC_REVERSE     (22)
#define BIT_STC_PAUSE       (21)
#define BIT_STC_UPDATE      (20)

/* RECOVER register bit definition */
#define BIT_STC_RECOV_BASE_GATE          0
#define BIT_STC_RECOV_BASE_ERR_GATE      8
#define BIT_STC_RECOV_ERR_LOST_CNT       16
#define BIT_STC_RECOV_PCR_FOLLOW_CNT     20
#define BIT_STC_RECOV_BASE_ERR_SWITCH    24
#define BIT_STC_RECOV_STC_LOAD_SWITCH    25
#define BIT_STC_RECOV_TIMING_ERR_SWITCH  26
#define BIT_STC_RECOV_OVER_TIME_SWITCH   27
#define BIT_STC_RECOV_GAIN               28
#define BIT_STC_RECOV_RECOVER_MODE       30

#define STC_RECOV_BASE_GATE              (0xFF << BIT_STC_RECOV_BASE_GATE)
#define STC_RECOV_BASE_ERR_GATE          (0xFF << BIT_STC_RECOV_BASE_ERR_GATE)
#define STC_RECOV_ERR_LOST_CNT           (0x0F << BIT_STC_RECOV_ERR_LOST_CNT)
#define STC_RECOV_PCR_FOLLOW_CNT         (0x0F << BIT_STC_RECOV_PCR_FOLLOW_CNT)
#define STC_RECOV_BASE_ERR_SWITCH        (0x01 << BIT_STC_RECOV_BASE_ERR_SWITCH)
#define STC_RECOV_STC_LOAD_SWITCH        (0x01 << BIT_STC_RECOV_STC_LOAD_SWITCH)
#define STC_RECOV_TIMING_ERR_SWITCH      (0x01 << BIT_STC_RECOV_TIMING_ERR_SWITCH)
#define STC_RECOV_OVER_TIME_SWITCH       (0x01 << BIT_STC_RECOV_OVER_TIME_SWITCH)
#define STC_RECOV_GAIN                   (0x03 << BIT_STC_RECOV_GAIN)
#define STC_RECOV_RECOVER_MODE           (0x03 << BIT_STC_RECOV_RECOVER_MODE)

#define STC_RECOV_CLR_TIMING_ERR_SWITCH  (0x00 << BIT_STC_RECOV_TIMING_ERR_SWITCH)
#define STC_RECOV_CLR_OVER_TIME_SWITCH   (0x00 << BIT_STC_RECOV_OVER_TIME_SWITCH)
#define STC_RECOV_GAIN_0                 (0x00 << BIT_STC_RECOV_GAIN)
#define STC_RECOV_GAIN_1                 (0x01 << BIT_STC_RECOV_GAIN)
#define STC_RECOV_GAIN_2                 (0x02 << BIT_STC_RECOV_GAIN)
#define STC_RECOV_GAIN_3                 (0x03 << BIT_STC_RECOV_GAIN)
#define STC_RECOV_MODE_PCR               (0x00 << BIT_STC_RECOV_RECOVER_MODE)
#define STC_RECOV_MODE_AV_PTS            (0x01 << BIT_STC_RECOV_RECOVER_MODE)
#define STC_RECOV_MODE_PLAYBACK          (0x02 << BIT_STC_RECOV_RECOVER_MODE)
#define STC_RECOV_MODE_NOSYNC            (0x03 << BIT_STC_RECOV_RECOVER_MODE)

struct reg_stc
{
	unsigned int recover;
	unsigned int pcr_cfg;
	unsigned int stc;
	unsigned int pcr;
	unsigned int pts_cfg;
	unsigned int pts;
	unsigned int fullstop_h;
	unsigned int fullstop_l;
	unsigned int reserve_b;
	unsigned int recovery_1;
	unsigned int pcr_cfg_1;
	unsigned int stc_offset_1;
	unsigned int stc_1;
	unsigned int stc_ctrl_1;
	unsigned int stc_out_sel;
	unsigned int reserve_c[39];
	unsigned int pts_w_addr[24];
	unsigned int reserve_d;
	unsigned int video_head11;
	unsigned int video_head12;
	unsigned int pts_buf_wptr_clr;
	unsigned int sync_en;
	unsigned int sync_clk[4];
	unsigned int stream_out_ctrl;
	unsigned int stc_cnt;
	unsigned int audio_dec_apts;
	unsigned int video_head21;
	unsigned int video_head22;
	unsigned int reserve_e;
	unsigned int ts_if_cfg;
	unsigned int dmux_output_clk;
	unsigned int stc_offset;
};

#endif
