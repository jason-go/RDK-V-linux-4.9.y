/*
 * demux_regs.h for hw demux .
 *
 * This file defines  micro-definitions for user.
 * History:
 *      29-OCT-2008 create this file
 */

#ifndef _GX3211_DEMUX_REGS_H
#define _GX3211_DEMUX_REGS_H

#define GXAV_DEMUX_BASE_SYS                 (0x04000000+0x2000)
#define GXAV_DEMUX_BASE_SYS1                (0x04000000+0x12000)
#define CHIP_CONFIG_BASE_ADDRESS            0x0050a000

/* Number of Demux Resource the chip supported */
#define NUM_PID_SRAM                       1343   // size of PID SRAM
#define NUM_MATCH_SRAM                     512   // size of MATCH SRAM
#define NUM_PID_BIT                        13    // bits of pid
#define NUM_PID_CFG_SEL                    64    // num of PID_CFG & PID_SEL
#define NUM_KEY_PAIR                       16    // gx3211...have 12 pair of keys
#define NUM_MATCH_BIT                      8
#define NUM_MATCH_BYTE                     16
#define NUM_DF_DEPTH                       8
#define NUM_DF_CHNL                        64
#define NUM_AV_CHNL                        8
#define NUM_CLK_CHNL                       4
#define NUM_DEMUX_BUF                      8

/* VIDEO_HEAD: mpeg2 & avs */
#define MPEG2_HEAD1                        0x00000100
#define MPEG2_HEAD2                        0x00000100

#define AVS_HEAD1                          0x000001B3
#define AVS_HEAD2                          0x000001B6

/* PID_CFG register bit definition */
#define BIT_DEMUX_PID_CFG_AV_NUM           0
#define BIT_DEMUX_PID_CFG_TYPE_SEL         3
#define BIT_DEMUX_PID_CFG_AV_TYPE          4
#define BIT_DEMUX_PID_CFG_PTS_BYPASS       6
#define BIT_DEMUX_PID_CFG_LINE_EN          7
#define BIT_DEMUX_PID_CFG_FILTER_PSI       8
#define BIT_DEMUX_PID_CFG_STOP_PER_UNIT    9
#define BIT_DEMUX_PID_CFG_INT_PER_UNIT     10
#define BIT_DEMUX_PID_CFG_FULL_STOP        11
#define BIT_DEMUX_PID_CFG_KEY_ID           12
#define BIT_DEMUX_PID_CFG_EVEN_KEY_VALID   16
#define BIT_DEMUX_PID_CFG_ODD_KEY_VALID    17
#define BIT_DEMUX_PID_CFG_DSC_TYPE         18
#define BIT_DEMUX_PID_CFG_CRC_DISABLE      19
#define BIT_DEMUX_PID_CFG_DUP_DISCARD      20
#define BIT_DEMUX_PID_CFG_ERR_DISCARD      21
#define BIT_DEMUX_PID_CFG_STORE_TS         22
#define BIT_DEMUX_PID_CFG_PRIVATE_TS       23
#define BIT_DEMUX_PID_CFG_EXT_AV_NUM       24
#define BIT_DEMUX_PID_CFG_PLAY_RECORD      27
#define BIT_DEMUX_PID_CFG_PTS_TO_SDRAM     28
#define BIT_DEMUX_PID_CFG_AV_OUT_EN        29
#define BIT_DEMUX_PID_CFG_TS_OUT_EN        30
#define BIT_DEMUX_PID_CFG_TS_DES_OUT_EN    31
#define BIT_DEMUX_PID_CFG_TSW_BUF          0

#define DEMUX_PID_CFG_AV_NUM               (0x07 << BIT_DEMUX_PID_CFG_AV_NUM)
#define DEMUX_PID_CFG_TYPE_SEL             (0x01 << BIT_DEMUX_PID_CFG_TYPE_SEL)
#define DEMUX_PID_CFG_AV_TYPE              (0x03 << BIT_DEMUX_PID_CFG_AV_TYPE)
#define DEMUX_PID_CFG_PTS_BYPASS           (0x01 << BIT_DEMUX_PID_CFG_PTS_BYPASS)
#define DEMUX_PID_CFG_LINE_EN              (0x01 << BIT_DEMUX_PID_CFG_LINE_EN)
#define DEMUX_PID_CFG_FILTER_PSI           (0x01 << BIT_DEMUX_PID_CFG_FILTER_PSI)
#define DEMUX_PID_CFG_STOP_PER_UNIT        (0x01 << BIT_DEMUX_PID_CFG_STOP_PER_UNIT)
#define DEMUX_PID_CFG_INT_PER_UNIT         (0x01 << BIT_DEMUX_PID_CFG_INT_PER_UNIT)
#define DEMUX_PID_CFG_FULL_STOP            (0x01 << BIT_DEMUX_PID_CFG_FULL_STOP)
#define DEMUX_PID_CFG_KEY_ID               (0x0F << BIT_DEMUX_PID_CFG_KEY_ID)
#define DEMUX_PID_CFG_EVEN_KEY_VALID       (0x01 << BIT_DEMUX_PID_CFG_EVEN_KEY_VALID)
#define DEMUX_PID_CFG_ODD_KEY_VALID        (0x01 << BIT_DEMUX_PID_CFG_ODD_KEY_VALID)
#define DEMUX_PID_CFG_DSC_TYPE             (0x01 << BIT_DEMUX_PID_CFG_DSC_TYPE)
#define DEMUX_PID_CFG_CRC_DISABLE          (0x01 << BIT_DEMUX_PID_CFG_CRC_DISABLE)
#define DEMUX_PID_CFG_DUP_DISCARD          (0x01 << BIT_DEMUX_PID_CFG_DUP_DISCARD)
#define DEMUX_PID_CFG_ERR_DISCARD          (0x01 << BIT_DEMUX_PID_CFG_ERR_DISCARD)
#define DEMUX_PID_CFG_STORE_TS             (0x01 << BIT_DEMUX_PID_CFG_STORE_TS)
#define DEMUX_PID_CFG_PRIVATE_TS           (0x01 << BIT_DEMUX_PID_CFG_PRIVATE_TS)
#define DEMUX_PID_CFG_EXT_AV_MUN           (0x07 << BIT_DEMUX_PID_CFG_EXT_AV_NUM)
#define DEMUX_PID_CFG_PLAY_RECORD          (0x01 << BIT_DEMUX_PID_CFG_PLAY_RECORD)
#define DEMUX_PID_CFG_PTS_TO_SDRAM         (0x01 << BIT_DEMUX_PID_CFG_PTS_TO_SDRAM)
#define DEMUX_PID_CFG_AV_OUT_EN            (0x01 << BIT_DEMUX_PID_CFG_AV_OUT_EN)
#define DEMUX_PID_CFG_TSW_BUF_SEL          (0x3f << BIT_DEMUX_PID_CFG_TSW_BUF)

#define DEMUX_PID_CFG_TYPE_VIDEO           (0x00 << BIT_DEMUX_PID_CFG_AV_TYPE)
#define DEMUX_PID_CFG_TYPE_AUDIO           (0x01 << BIT_DEMUX_PID_CFG_AV_TYPE)
#define DEMUX_PID_CFG_TYPE_SPDIF           (0x02 << BIT_DEMUX_PID_CFG_AV_TYPE)
#define DEMUX_PID_CFG_TYPE_MAUDIO          (0x03 << BIT_DEMUX_PID_CFG_AV_TYPE)

/* DMX_CFG register bit definition */
#define BIT_DEMUX_CFG_TIMING_GATE          0
#define BIT_DEMUX_CFG_COUNT_GATE           4
#define BIT_DEMUX_CFG_LOSS_GATE            8
#define BIT_DEMUX_CFG_LOCK_GATE            12
#define BIT_DEMUX_CFG_TS_SEL_2             16
#define BIT_DEMUX_CFG_TS_SEL_1             18
#define BIT_DEMUX_CFG_STREAM_IN_SEL        20
#define BIT_DEMUX_CFG_FIFO_MODE            21
#define BIT_DEMUX_CFG_VALID_EN_TS1         23
#define BIT_DEMUX_CFG_VALID_EN_TS2         24
#define BIT_DEMUX_CFG_TS_I_MODE            26
#define BIT_DEMUX_CFG_TS_II_MODE           27
#define BIT_DEMUX_CFG_PROC_START           28
#define BIT_DEMUX_CFG_AUTO_START           29
#define BIT_DEMUX_CFG_DEBUG                30

#define DEMUX_CFG_TIMING_GATE              (0x0F << BIT_DEMUX_CFG_TIMING_GATE)
#define DEMUX_CFG_COUNT_GATE               (0x0F << BIT_DEMUX_CFG_COUNT_GATE)
#define DEMUX_CFG_LOSS_GATE                (0x0F << BIT_DEMUX_CFG_LOSS_GATE)
#define DEMUX_CFG_LOCK_GATE                (0x0F << BIT_DEMUX_CFG_LOCK_GATE)
#define DEMUX_CFG_TS_SEL2                  (0x03 << BIT_DEMUX_CFG_TS_SEL_2)
#define DEMUX_CFG_TS_SEL1                  (0x03 << BIT_DEMUX_CFG_TS_SEL_1)
#define DEMUX_CFG_STREAM_IN_SEL            (0x01 << BIT_DEMUX_CFG_STREAM_IN_SEL)
#define DEMUX_CFG_FIFO_MODE                (0x03 << BIT_DEMUX_CFG_FIFO_MODE)
#define DEMUX_CFG_STREAM_OUT_EN            (0x01 << BIT_DEMUX_CFG_VALID_EN_TS1)
#define DEMUX_CFG_STREAM_OUT_SEL           (0x01 << BIT_DEMUX_CFG_VALID_EN_TS2)
#define DEMUX_CFG_TS_I_MODE                (0x01 << BIT_DEMUX_CFG_TS_I_MODE)
#define DEMUX_CFG_TS_II_MODE               (0x01 << BIT_DEMUX_CFG_TS_II_MODE)
#define DEMUX_CFG_PROC_START               (0x01 << BIT_DEMUX_CFG_PROC_START)
#define DEMUX_CFG_AUTO_START               (0x01 << BIT_DEMUX_CFG_AUTO_START)
#define DEMUX_CFG_DEBUG                    (0x01 << BIT_DEMUX_CFG_DEBUG)

#define DEMUX_CFG_DMUX2_SEL_TS1            (0x00 << BIT_DEMUX_CFG_TS_SEL_2)
#define DEMUX_CFG_DMUX2_SEL_TS2            (0x01 << BIT_DEMUX_CFG_TS_SEL_2)
#define DEMUX_CFG_DMUX2_SEL_TS3            (0x02 << BIT_DEMUX_CFG_TS_SEL_2)
#define DEMUX_CFG_DMUX2_SEL_SDRAM          (0x03 << BIT_DEMUX_CFG_TS_SEL_2)

#define DEMUX_CFG_DMUX1_SEL_TS1            (0x00 << BIT_DEMUX_CFG_TS_SEL_1)
#define DEMUX_CFG_DMUX1_SEL_TS2            (0x01 << BIT_DEMUX_CFG_TS_SEL_1)
#define DEMUX_CFG_DMUX1_SEL_TS3            (0x02 << BIT_DEMUX_CFG_TS_SEL_1)
#define DEMUX_CFG_DMUX1_SEL_SDRAM          (0x03 << BIT_DEMUX_CFG_TS_SEL_1)

#define DEMUX_CFG_FIFO_FOR_ALL             (0x00 << BIT_DEMUX_CFG_FIFO_MODE)
#define DEMUX_CFG_FIFO_FOR_TS1             (0x10 << BIT_DEMUX_CFG_FIFO_MODE)
#define DEMUX_CFG_FIFO_FOR_TS2             (0x01 << BIT_DEMUX_CFG_FIFO_MODE)

/* AV_SCR register bit definition */
#define BIT_DEMUX_AV_SCR_DMUX1             0
#define BIT_DEMUX_AV_SCR_DMUX2             8

#define DEMUX_AV_SCR_DMUX1                 (0xFF << BIT_DEMUX_AV_SCR_DMUX1)
#define DEMUX_AV_SCR_DMUX2                 (0xFF << BIT_DEMUX_AV_SCR_DMUX2)

/* AUDIO_HEAD_EXT register bit definition */
#define BIT_DEMUX_SPDIF_HEAD               0
#define BIT_DEMUX_SPDIF_HEAD_FLAG          16
#define BIT_DEMUX_AUDIO_HEAD_EXT           20
#define BIT_DEMUX_AUDIO_HEAD_EXT_FLAG      24

#define DEMUX_SPDIF_HEAD                   (0xFFFF << BIT_DEMUX_SPDIF_HEAD)
#define DEMUX_SPDIF_HEAD_FLAG              (0x01 << BIT_DEMUX_SPDIF_HEAD_FLAG)
#define DEMUX_AUDIO_HEAD_EXT               (0x0F << BIT_DEMUX_AUDIO_HEAD_EXT)
#define DEMUX_AUDIO_HEAD_EXT_FLAG          (0x01 << BIT_DEMUX_AUDIO_HEAD_EXT_FLAG)

/* PCR_CFG register bit definition */
#define BIT_DEMUX_PCR_CFG_PID              0
#define BIT_DEMUX_PCR_CFG_SEL              13
#define BIT_DEMUX_PCR_CFG_INTIME_GATE      16
#define BIT_DEMUX_PCR_CFG_OVERTIME_GATE    24

#define DEMUX_PCR_CFG_PID                  (0x1FFF << BIT_DEMUX_PCR_CFG_PID)
#define DEMUX_PCR_CFG_SEL                  (0x01 << BIT_DEMUX_PCR_CFG_SEL)
#define DEMUX_PCR_CFG_INTIME_GATE          (0xFF << BIT_DEMUX_PCR_CFG_INTIME_GATE)
#define DEMUX_PCR_CFG_OVERTIME_GATE        (0xFF << BIT_DEMUX_PCR_CFG_OVERTIME_GATE)

/* PTS_CFG register bit definition */
#define BIT_DEMUX_PTS_CFG_VIEW_SEL         0
#define BIT_DEMUX_PTS_CFG_RECOVER_SEL      4

#define DEMUX_PTS_CFG_VIEW_SEL             (0x07 << BIT_DEMUX_PTS_CFG_VIEW_SEL)
#define DEMUX_PTS_CFG_RECOVER_SEL          (0x07 << BIT_DEMUX_PTS_CFG_RECOVER_SEL)

/* INT_DMX_TS register bit definition */
#define BIT_DEMUX_TS_I_SPLICE_A_NEG        0
#define BIT_DEMUX_TS_I_SPLICE_A_POS        1
#define BIT_DEMUX_TS_I_SPLICE_V_NEG        2
#define BIT_DEMUX_TS_I_SPLICE_V_POS        3
#define BIT_DEMUX_TS_I_IND_ERR             4
#define BIT_DEMUX_TS_I_CNT_ERR             5
#define BIT_DEMUX_TS_I_SYNC_LOSS           6
#define BIT_DEMUX_TS_I_TIMING_ERR          7
#define BIT_DEMUX_TS_I_LOCK                8
#define BIT_DEMUX_TS_I_LOSS                9
#define BIT_DEMUX_TS_I_FIFO_FULL           10
#define BIT_DEMUX_TS_I_PARA_FULL           11
#define BIT_DEMUX_TS_II_SPLICE_A_NEG       12
#define BIT_DEMUX_TS_II_SPLICE_A_POS       13
#define BIT_DEMUX_TS_II_SPLICE_V_NEG       14
#define BIT_DEMUX_TS_II_SPLICE_V_POS       15
#define BIT_DEMUX_TS_II_IND_ERR            16
#define BIT_DEMUX_TS_II_CNT_ERR            17
#define BIT_DEMUX_TS_II_SYNC_LOSS          18
#define BIT_DEMUX_TS_II_TIMING_ERR         19
#define BIT_DEMUX_TS_II_LOCK               20
#define BIT_DEMUX_TS_II_LOSS               21
#define BIT_DEMUX_TS_II_FIFO_FULL          22
#define BIT_DEMUX_TS_II_PARA_FULL          23
#define BIT_DEMUX_TS_PCR_OVERTIME          24
#define BIT_DEMUX_TS_PCR_REINTIME          25
#define BIT_DEMUX_TS_I_SYNC_LOCK           28    // just query bit, not interrupt
#define BIT_DEMUX_TS_II_SYNC_LOCK          29

#define DEMUX_TS_I_SPLICE_A_NEG            (0x01 << BIT_DEMUX_TS_I_SPLICE_A_NEG)
#define DEMUX_TS_I_SPLICE_A_POS            (0x01 << BIT_DEMUX_TS_I_SPLICE_A_POS)
#define DEMUX_TS_I_SPLICE_V_NEG            (0x01 << BIT_DEMUX_TS_I_SPLICE_V_NEG)
#define DEMUX_TS_I_SPLICE_V_POS            (0x01 << BIT_DEMUX_TS_I_SPLICE_V_POS)
#define DEMUX_TS_I_IND_ERR                 (0x01 << BIT_DEMUX_TS_I_IND_ERR)
#define DEMUX_TS_I_CNT_ERR                 (0x01 << BIT_DEMUX_TS_I_CNT_ERR)
#define DEMUX_TS_I_SYNC_LOSS               (0x01 << BIT_DEMUX_TS_I_SYNC_LOSS)
#define DEMUX_TS_I_TIMING_ERR              (0x01 << BIT_DEMUX_TS_I_TIMING_ERR)
#define DEMUX_TS_I_LOCK                    (0x01 << BIT_DEMUX_TS_I_LOCK)
#define DEMUX_TS_I_LOSS                    (0x01 << BIT_DEMUX_TS_I_LOSS)
#define DEMUX_TS_I_FIFO_FULL               (0x01 << BIT_DEMUX_TS_I_FIFO_FULL)
#define DEMUX_TS_I_PARA_FULL               (0x01 << BIT_DEMUX_TS_I_PARA_FULL)
#define DEMUX_TS_II_SPLICE_A_NEG           (0x01 << BIT_DEMUX_TS_II_SPLICE_A_NEG)
#define DEMUX_TS_II_SPLICE_A_POS           (0x01 << BIT_DEMUX_TS_II_SPLICE_A_POS)
#define DEMUX_TS_II_SPLICE_V_NEG           (0x01 << BIT_DEMUX_TS_II_SPLICE_V_NEG)
#define DEMUX_TS_II_SPLICE_V_POS           (0x01 << BIT_DEMUX_TS_II_SPLICE_V_POS)
#define DEMUX_TS_II_IND_ERR                (0x01 << BIT_DEMUX_TS_II_IND_ERR)
#define DEMUX_TS_II_CNT_ERR                (0x01 << BIT_DEMUX_TS_II_CNT_ERR)
#define DEMUX_TS_II_SYNC_LOSS              (0x01 << BIT_DEMUX_TS_II_SYNC_LOSS)
#define DEMUX_TS_II_TIMING_ERR             (0x01 << BIT_DEMUX_TS_II_TIMING_ERR)
#define DEMUX_TS_II_LOCK                   (0x01 << BIT_DEMUX_TS_II_LOCK)
#define DEMUX_TS_II_LOSS                   (0x01 << BIT_DEMUX_TS_II_LOSS)
#define DEMUX_TS_II_FIFO_FULL              (0x01 << BIT_DEMUX_TS_II_FIFO_FULL)
#define DEMUX_TS_II_PARA_FULL              (0x01 << BIT_DEMUX_TS_II_PARA_FULL)
#define DEMUX_TS_PCR_OVERTIME              (0x01 << BIT_DEMUX_TS_PCR_OVERTIME)
#define DEMUX_TS_PCR_REINTIME              (0x01 << BIT_DEMUX_TS_PCR_REINTIME)
#define DEMUX_TS_I_SYNC_LOCK               (0x01 << BIT_DEMUX_TS_I_SYNC_LOCK)
#define DEMUX_TS_II_SYNC_LOCK              (0x01 << BIT_DEMUX_TS_II_SYNC_LOCK)

/* SYNC_EN register bit definition */
#define DEMUX_SYNC_1_EN                    (0x01 << 0)
#define DEMUX_SYNC_2_EN                    (0x01 << 1)
#define DEMUX_SYNC_3_EN                    (0x01 << 2)
#define DEMUX_SYNC_4_EN                    (0x01 << 3)

/*DEMUX_BUF_ID register bit definition*/
#define BIT_DEMUX_AV_BUF_ID                0//16
#define BIT_DEMUX_SPDIF_BUF_ID             8
#define BIT_DEMUX_PTS_BUF_ID               16//0

#define DEMUX_AV_BUF_ID                    (0xff << BIT_DEMUX_AV_BUF_ID)
#define DEMUX_SPDIF_BUF_ID                 (0xff << BIT_DEMUX_SPDIF_BUF_ID)
#define DEMUX_PTS_BUF_ID                   (0xff << BIT_DEMUX_PTS_BUF_ID)

/*STC_COUNT register*/
#define BIT_STC_COUNT_EN                   16
#define BIT_STC_COUNT_VALUE                0



/*RST_SDC_CTRL register bit definition*/
#define BIT_DEMUX_READ_STOP                25
#define BIT_DEMUX_READ_EN                  24
#define BIT_DEMUX_ES_ID                    16

#define DEMUX_RST_ES_ID                    (0xff << BIT_DEMUX_ES_ID)

/* register bit definition*/
#define BIT_DEMUX1_SERIAL_MODE             0
#define BIT_DEMUX2_SERIAL_MODE             2

#define DEMUX_SERIAL_MODE_HIGH              0x01
#define DEMUX1_SERIAL_STREAM_MODE          (0x11 << BIT_DEMUX1_SERIAL_MODE)
#define DEMUX2_SERIAL_STREAM_MODE          (0x11 << BIT_DEMUX2_SERIAL_MODE)

/* register bit definition*/
#define BIT_DEMUX_LEVEL_LOCK_PORT1         20           //dvbs
#define BIT_DEMUX_LEVEL_LOCK_PORT2         21           //ts1
#define BIT_DEMUX_LEVEL_LOCK_PORT3         22           //ts2
#define BIT_DEMUX_LEVEL_LOCK_PORT4         23           //ts from sram

#define BIT_DEMUX_INDICATE_LOCK_PORT1      0           //dvbs
#define BIT_DEMUX_INDICATE_LOCK_PORT2      1           //ts1
#define BIT_DEMUX_INDICATE_LOCK_PORT3      2           //ts2
#define BIT_DEMUX_INDICATE_LOCK_PORT4      3           //ts from sram

/********************************chip config***********************************/
#define MPEG2_HOT_RESET_SET                 (CHIP_CONFIG_BASE_ADDRESS + 0x0010)
#define MPEG2_HOT_RESET_CLR                 (CHIP_CONFIG_BASE_ADDRESS + 0x0014)
#define BIT_HOT_SDC_MPEG2_DISP_RST_N         31
#define BIT_HOT_SYS_DEMUX_RST_N              29
#define BIT_HOT_SDC_DEMUX_RST_N              28 //
#define BIT_HOT_SDC_MPEG2_RST_N              14
#define BIT_HOT_SDC_VIDEO_INPUT_RST_N        9 //
#define BIT_HOT_AUDIO_PLAY_SPD_RST_N         7
#define BIT_HOT_AUDIO_PLAY_I2S_RST_N         6



struct reg_demux_pid_enable {
	unsigned int pid_en_h;
	unsigned int pid_en_l;
};

struct reg_demux_pid {
	unsigned int pid_h;
	unsigned int pid_l;
};

struct reg_demux_slot {
	unsigned int pid_cfg;
	unsigned int pid_cfg_l;
	unsigned int pid_sel_h;
	unsigned int pid_sel_l;
};

struct reg_demux_key {
	unsigned int key_odd_high;
	unsigned int key_odd_low;
	unsigned int key_even_high;
	unsigned int key_even_low;
};

struct reg_demux_m01 {
	unsigned int m0_byte_h;
	unsigned int m0_byte_l;
	unsigned int m1_byte_h;
	unsigned int m1_byte_l;
};

struct reg_demux_byte {
	struct reg_demux_m01   m_byte[NUM_MATCH_BIT];
};

struct reg_demux_dfaddr {
	unsigned int df_start_addr;
	unsigned int df_buf_size;
	unsigned int df_write_addr;
	unsigned int df_read_addr;
	unsigned int df_almost_full_gate;
	unsigned int df_reserve[3];
};

struct reg_demux_avaddr {
	unsigned int av_start_addr;
	unsigned int av_buf_size;
	unsigned int av_write_addr;
	unsigned int av_read_addr;
	unsigned int av_almostfull_gate;
	unsigned int av_fullstop_gate;
	unsigned int av_es_start_addr;
	unsigned int av_es_end_addr;
};

struct reg_demux_ptsaddr {
	unsigned int pts_buf_addr;
	unsigned int pts_buf_len;
};

struct reg_demux_tsw_buf {
	unsigned int tsw_buf_start_addr;
	unsigned int tsw_buf_len;
	unsigned int tsw_write_addr;
	unsigned int tsw_read_addr;
	unsigned int tsw_almost_full_gate;
	unsigned int tsw_reserve[3];
};

struct reg_demux_cam {
	unsigned int cam_emm_tid_mode;
	unsigned int cam_emm_data_id1[3];
	unsigned int cam_emm_mask_id1[3];
	unsigned int cam_emm_pid;
	unsigned int cam_emm_tid;
	unsigned int cam_ca_int;
	unsigned int cam_emm_int_stat;
	unsigned int cam_emm_data_id4[5];
	unsigned int cam_emm_mask_id4[5];
	unsigned int cam_emm_ctrl_id;
	unsigned int cam_emm_buff_addr;
	unsigned int emm_icam_inoutput_ctrl;
	unsigned int sync_buffer_freq;
};

struct reg_demux {
	struct reg_demux_pid_enable pid_en[2];
	unsigned int reserved[2];
	struct reg_demux_pid pid[NUM_PID_BIT];
	struct reg_demux_slot pid_cfg_sel[NUM_PID_CFG_SEL];
	unsigned int reserved_a[32];
	struct reg_demux_key key_even_odd[NUM_KEY_PAIR];
	unsigned int reserved_b[128];

	struct reg_demux_byte m_byte_n[NUM_MATCH_BYTE];

	unsigned int int_ts_if;
	unsigned int int_ts_if_en;
	unsigned int int_dmx_df_h;
	unsigned int int_dmx_df_l;
	unsigned int int_dmx_df_en_h;
	unsigned int int_dmx_df_en_l;
	unsigned int int_df_crc_h;
	unsigned int int_df_crc_l;
	unsigned int int_df_crc_en_h;
	unsigned int int_df_crc_en_l;
	unsigned int int_df_buf_alfull_h;
	unsigned int int_df_buf_alfull_l;
	unsigned int int_df_buf_alfull_en_h;
	unsigned int int_df_buf_alfull_en_l;
	unsigned int int_df_buf_full_h;
	unsigned int int_df_buf_full_l;
	unsigned int int_df_buf_full_en_h;
	unsigned int int_df_buf_full_en_l;
	unsigned int int_df_buf_overflow_h;
	unsigned int int_df_buf_overflow_l;
	unsigned int int_df_buf_overflow_en_h;
	unsigned int int_df_buf_overflow_en_l;
	unsigned int int_dmx_av_buf;
	unsigned int int_dmx_av_buf_en;
	unsigned int int_dmx_tsr_buf;
	unsigned int int_dmx_tsr_buf_en;
	unsigned int int_tsw_buf_pusi_h;
	unsigned int int_tsw_buf_pusi_l;
	unsigned int int_tsw_buf_pusi_en_h;
	unsigned int int_tsw_buf_pusi_en_l;
	unsigned int int_tsw_buf_alful_h;
	unsigned int int_tsw_buf_alful_l;
	unsigned int int_tsw_buf_alful_en_h;
	unsigned int int_tsw_buf_alful_en_l;
	unsigned int int_tsw_buf_ful_h;
	unsigned int int_tsw_buf_ful_l;
	unsigned int int_tsw_buf_ful_en_h;
	unsigned int int_tsw_buf_ful_en_l;
	unsigned int int_dmx_ts;
	unsigned int int_dmx_ts_en;
	unsigned int int_dmx_cc_h;
	unsigned int int_dmx_cc_l;
	unsigned int int_dmx_cc_en_h;
	unsigned int int_dmx_cc_en_l;
	unsigned int int_dmx_index;
	unsigned int reserve_x[143];
	unsigned int av_buf_full_stop_th;
	unsigned int df_buf_full_stop_th;
	unsigned int df_buf_full_enactive_h;
	unsigned int df_buf_full_enactive_l;
	unsigned int df_waddr[NUM_DF_CHNL];
	struct reg_demux_avaddr av_addr[NUM_AV_CHNL];
	unsigned int av_buf_wptr_clr;
	unsigned int demux_bigendian;
	unsigned int dmux_cfg;
	unsigned int wen_mask;
	unsigned int av_scr;
	unsigned int df_on_h;
	unsigned int df_on_l;
	unsigned int df_nequal_on_h;
	unsigned int df_nequal_on_l;
	unsigned int df_sel_h;
	unsigned int df_sel_l;
	unsigned int df_depth[NUM_DF_DEPTH];
	unsigned int cpu_pid_in_h;
	unsigned int cpu_pid_in_l;
	unsigned int tsw_pid_in_h;
	unsigned int tsw_pid_in_l;
	unsigned int audio_head_ext;
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
	struct reg_demux_ptsaddr pts_addr[NUM_AV_CHNL];
	unsigned int pts_w_addr[NUM_AV_CHNL];
	unsigned int reserve_d;
	unsigned int video_head11;
	unsigned int video_head12;
	unsigned int pts_buf_wptr_clr;
	unsigned int sync_en;
	unsigned int sync_clk[NUM_CLK_CHNL];
	unsigned int stream_out_ctrl;
	unsigned int stc_cnt;
	unsigned int audio_dec_apts;
	unsigned int video_head21;
	unsigned int video_head22;
	unsigned int reserve_e;
	unsigned int ts_if_cfg;
	unsigned int dmux_output_clk;
	unsigned int stc_offset;
	unsigned int section_syntax_indicator_en_h;
	unsigned int section_syntax_indicator_en_l;
	unsigned int pts_mode;
	unsigned int reserve_f;
	unsigned int rts_sdc_addr;
	unsigned int rts_sdc_size;
	unsigned int rts_sdc_wptr;
	unsigned int rts_sdc_rptr;
	unsigned int rts_almost_empty_gate;
	unsigned int rts_sdc_ctrl;
	unsigned int tsw_buf_en_0_h;
	unsigned int tsw_buf_en_0_l;
	unsigned int tsw_buf_en_1_h;
	unsigned int tsw_buf_en_1_l;
	unsigned int ts_write_ctrl;
	unsigned int reserv_g;

	struct reg_demux_cam cam[2];

	unsigned int reserv_h[2];
	struct reg_demux_tsw_buf tsw_buf[64];
	struct reg_demux_dfaddr df_addr[64];
	unsigned int axi_8to32_cfg;
};


#endif /*_GX3211_DEMUX_REGS_H*/

