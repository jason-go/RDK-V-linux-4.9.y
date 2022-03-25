#ifndef _GX3211X_DEMUX_REGS_H_
#define _GX3211X_DEMUX_REGS_H_

#define GXAV_DEMUX_BASE_SYS                 (0x04000000+0x1b00)
#define GXAV_DEMUX_BASE_SYS1                (0x04000000+0x11b00)

#define DMX_IRQ0     33
#define DMX_IRQ1     41

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

#define BIT_DEMUX_TS_CLK_ERR               (24)
#define DEMUX_TS_CLK_ERR                   (0xf<<24)

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
#define DEMUX_PID_CFG_TS_OUT_EN            (0x01 << BIT_DEMUX_PID_CFG_TS_OUT_EN)
#define DEMUX_PID_CFG_TS_DES_OUT_EN        (0x01 << BIT_DEMUX_PID_CFG_TS_DES_OUT_EN)
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
#define BIT_DEMUX_CFG_AUTO_START           29

#define DEMUX_CFG_TIMING_GATE              (0x0F << BIT_DEMUX_CFG_TIMING_GATE)
#define DEMUX_CFG_COUNT_GATE               (0x0F << BIT_DEMUX_CFG_COUNT_GATE)
#define DEMUX_CFG_LOSS_GATE                (0x0F << BIT_DEMUX_CFG_LOSS_GATE)
#define DEMUX_CFG_LOCK_GATE                (0x0F << BIT_DEMUX_CFG_LOCK_GATE)
#define DEMUX_CFG_TS_SEL2                  (0x03 << BIT_DEMUX_CFG_TS_SEL_2)
#define DEMUX_CFG_TS_SEL1                  (0x03 << BIT_DEMUX_CFG_TS_SEL_1)
#define DEMUX_CFG_AUTO_START               (0x01 << BIT_DEMUX_CFG_AUTO_START)

#define DEMUX_CFG_DMUX2_SEL_TS1            (0x00 << BIT_DEMUX_CFG_TS_SEL_2)
#define DEMUX_CFG_DMUX2_SEL_TS2            (0x01 << BIT_DEMUX_CFG_TS_SEL_2)
#define DEMUX_CFG_DMUX2_SEL_TS3            (0x02 << BIT_DEMUX_CFG_TS_SEL_2)
#define DEMUX_CFG_DMUX2_SEL_SDRAM          (0x03 << BIT_DEMUX_CFG_TS_SEL_2)

#define DEMUX_CFG_DMUX1_SEL_TS1            (0x00 << BIT_DEMUX_CFG_TS_SEL_1)
#define DEMUX_CFG_DMUX1_SEL_TS2            (0x01 << BIT_DEMUX_CFG_TS_SEL_1)
#define DEMUX_CFG_DMUX1_SEL_TS3            (0x02 << BIT_DEMUX_CFG_TS_SEL_1)
#define DEMUX_CFG_DMUX1_SEL_SDRAM          (0x03 << BIT_DEMUX_CFG_TS_SEL_1)

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

/*DEMUX_BUF_ID register bit definition*/
#define BIT_DEMUX_AV_BUF_ID                0//16
#define BIT_DEMUX_SPDIF_BUF_ID             8
#define BIT_DEMUX_PTS_BUF_ID               16//0

#define DEMUX_AV_BUF_ID                    (0xff << BIT_DEMUX_AV_BUF_ID)
#define DEMUX_SPDIF_BUF_ID                 (0xff << BIT_DEMUX_SPDIF_BUF_ID)
#define DEMUX_PTS_BUF_ID                   (0xff << BIT_DEMUX_PTS_BUF_ID)

/*RST_SDC_CTRL register bit definition*/
#define BIT_DEMUX_READ_STOP                25
#define BIT_DEMUX_READ_EN                  24
#define BIT_DEMUX_ES_ID                    16

#define DEMUX_RST_ES_ID                    (0xff << BIT_DEMUX_ES_ID)

/* register bit definition*/
#define BIT_DEMUX_LEVEL_LOCK_PORT1         20           //dvbs
#define BIT_DEMUX_LEVEL_LOCK_PORT2         21           //ts1
#define BIT_DEMUX_LEVEL_LOCK_PORT3         22           //ts2
#define BIT_DEMUX_LEVEL_LOCK_SRAM0         7            //ts from sram
#define BIT_DEMUX_LEVEL_LOCK_SRAM1         23           //ts from sram

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

struct reg_demux_rts_buf {
	unsigned int rts_sdc_addr;
	unsigned int rts_sdc_size;
	unsigned int rts_sdc_wptr;
	unsigned int rts_sdc_rptr;
	unsigned int rts_almost_empty_gate;
	unsigned int rts_sdc_ctrl;
};

struct reg_demux_tsw_buf {
	unsigned int tsw_buf_start_addr;
	unsigned int tsw_buf_len;
	unsigned int tsw_write_addr;
	unsigned int tsw_read_addr;
	unsigned int tsw_almost_full_gate;
	unsigned int tsw_reserve[3];
};

struct gx3211_reg_demux {
	unsigned int reserved_tsw_slot[320];

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
	struct reg_demux_rts_buf rts_buf;
	unsigned int tsw_buf_en_0_h;
	unsigned int tsw_buf_en_0_l;
	unsigned int tsw_buf_en_1_h;
	unsigned int tsw_buf_en_1_l;
	unsigned int ts_write_ctrl;
	unsigned int reserv_g[18];
	unsigned int int_tsr23_buf; // reserve
	unsigned int reserv_h[34];
	struct reg_demux_tsw_buf tsw_buf[64];
	struct reg_demux_dfaddr df_addr[64];
	unsigned int axi_8to32_cfg;
};

#endif

