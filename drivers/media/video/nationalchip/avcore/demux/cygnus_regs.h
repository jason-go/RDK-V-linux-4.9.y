#ifndef _CYGNUS_DEMUX_REGS_H_
#define _CYGNUS_DEMUX_REGS_H_

#include "gx3211_regs.h"
#include "sirius_regs.h"
#include "taurus_regs.h"

#define CYGNUS_MAX_TSW_NUM       32

/*
 * TSW SRC
 */

// common
#define CYGNUS_TSW_SRC_SYNC0             0
#define CYGNUS_TSW_SRC_SYNC1             1
#define CYGNUS_TSW_SRC_SYNC2             2
// demux slot
#define CYGNUS_TSW_SRC_TSR0              3
#define CYGNUS_TSW_SRC_TSR1              4
#define CYGNUS_TSW_SRC_DMXSLOT           5
#define CYGNUS_TSW_SRC_NO_TS             6
#define CYGNUS_TSW_SRC_DMXSLOT_NODESC    7
// tsw slot
#define CYGNUS_TSW_SRC_TSR2              3
#define CYGNUS_TSW_SRC_TSR3              4
#define CYGNUS_TSW_SRC_DMX_FOR_TSWSLOT   6

/*
 * TSW SLOT SRC
 */
#define CYGNUS_SRC_TSR_FOR_TSWSLOT       4
struct cygnus_reg_demux {
	unsigned int                        tsw_pid_en[2];
	unsigned int                        reserved;
	unsigned int                        tsw_pid[NUM_PID_BIT];
	struct taurus_reg_demux_slot        tsw_pid_cfg_sel[TAURUS_NUM_PID_CFG_SEL];
	unsigned int                        reserved_a[240];

	unsigned int                        pid_en[2];
	unsigned int                        reserved_0;
	unsigned int                        pid[NUM_PID_BIT];
	struct taurus_reg_demux_slot        pid_cfg_sel[TAURUS_NUM_PID_CFG_SEL];
	unsigned int                        reserved_1[432];

	struct taurus_reg_demux_byte        m_byte_n[NUM_MATCH_BYTE];
	unsigned int                        reserved_2[256];

	unsigned int int_ts_if;     //寄存器由4bit变为3bit
	unsigned int int_ts_if_en;
	unsigned int reserved_3;
	unsigned int int_dmx_df_l;
	unsigned int reserved_4;
	unsigned int int_dmx_df_en_l;
	unsigned int reserved_5;
	unsigned int int_df_crc_l;
	unsigned int reserved_6;
	unsigned int int_df_crc_en_l;
	unsigned int reserved_7;
	unsigned int int_df_buf_alfull_l;
	unsigned int reserved_8;
	unsigned int int_df_buf_alfull_en_l;
	unsigned int reserved_9;
	unsigned int int_df_buf_full_l;
	unsigned int reserved_10;
	unsigned int int_df_buf_full_en_l;
	unsigned int reserved_11;
	unsigned int int_df_buf_overflow_l;
	unsigned int reserved_12;
	unsigned int int_df_buf_overflow_en_l;
	unsigned int int_dmx_av_buf;
	unsigned int int_dmx_av_buf_en;
	unsigned int int_dmx_tsr_buf;  //增加端口锁定
	unsigned int int_dmx_tsr_buf_en;
	unsigned int reserved_13;
	unsigned int int_tsw_buf_pusi_l;
	unsigned int reserved_14;
	unsigned int int_tsw_buf_pusi_en_l;
	unsigned int reserved_15;
	unsigned int int_tsw_buf_alful_l;
	unsigned int reserved_16;
	unsigned int int_tsw_buf_alful_en_l;
	unsigned int reserved_17;
	unsigned int int_tsw_buf_ful_l;
	unsigned int reserved_18;
	unsigned int int_tsw_buf_ful_en_l;
	unsigned int int_dmx_ts;
	unsigned int int_dmx_ts_en;
	unsigned int reserved_19;
	unsigned int int_dmx_cc_l;
	unsigned int reserved_20;
	unsigned int int_dmx_cc_en_l;
	unsigned int int_dmx_index;
	unsigned int reserve_x[143];

	unsigned int av_buf_full_stop_th;
	unsigned int df_buf_full_stop_th;
	unsigned int reserved_21;
	unsigned int df_buf_full_enactive_l;
	unsigned int df_waddr[TAURUS_NUM_DF_CHNL];
	unsigned int reserved_22[32];
	struct reg_demux_avaddr av_addr[NUM_AV_CHNL];
	unsigned int av_buf_wptr_clr;
	unsigned int demux_bigendian;
	unsigned int dmux_cfg;
	unsigned int wen_mask;
	unsigned int av_scr;
	unsigned int reserved_24;
	unsigned int df_on_l;
	unsigned int reserved_25;
	unsigned int df_nequal_on_l;
	unsigned int reserved_26;
	unsigned int df_sel_l;
	unsigned int df_depth[TAURUS_NUM_DF_DEPTH];
	unsigned int reserved_27[TAURUS_NUM_DF_DEPTH+1];
	unsigned int cpu_pid_in_l;
	unsigned int reserved_28;
	unsigned int tsw_pid_in_l;
	unsigned int audio_head_ext;
	unsigned int recover;
	unsigned int pcr_cfg;
	unsigned int stc;
	unsigned int pcr;
	unsigned int pts_cfg;
	unsigned int pts;
	unsigned int reserved_29;
	unsigned int fullstop_l;
	unsigned int reserve_b;
	unsigned int recovery_1;
	unsigned int pcr_cfg_1;
	unsigned int stc_offset_1;
	unsigned int stc_1;
	unsigned int stc_ctrl_1;
	unsigned int stc_out_sel;
	unsigned int reserve_30[39];
	struct reg_demux_ptsaddr pts_addr[NUM_AV_CHNL];
	unsigned int pts_w_addr[NUM_AV_CHNL];
	unsigned int reserve_31;
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
	unsigned int serial_sync_gate;
	unsigned int stc_offset;
	unsigned int reserve_32;
	unsigned int section_syntax_indicator_en_l;

	unsigned int serial_mode;
	unsigned int pts_mode; // 1-3bit 并行时钟反转
	struct reg_demux_rts_buf rts_buf;
	unsigned int tsw_buf_en_0_h;
	unsigned int tsw_buf_en_0_l;
	unsigned int tsw_buf_en_1_h;
	unsigned int tsw_buf_en_1_l;
	unsigned int ts_write_ctrl;
	struct reg_demux_rts_buf rts_buf1;
	struct reg_demux_rts_buf pfilter_rts_buf[2];
	unsigned int int_tsr23_buf; //增加端口锁定
	unsigned int int_tsr23_buf_en;
	unsigned int tsw_buf_en_2_h;
	unsigned int tsw_buf_en_2_l;
	unsigned int tsw_buf_en_3_h;
	unsigned int tsw_buf_en_3_l;
	unsigned int ts_write_ctrl_23;
	unsigned int reserv_34[24];  //增加tsr23 buf enable 和 ctrl
	unsigned int internel_clk_div0;
	unsigned int internel_clk_div1;
	unsigned int reserv_35[2];
	struct reg_demux_tsw_buf tsw01_buf[16]; //tsw buffer改为tsw01共用16路, tsw23共用16路
	struct reg_demux_tsw_buf reserve_tsw01[16]; //tsw buffer改为tsw01共用16路, tsw23共用16路
	struct reg_demux_tsw_buf tsw23_buf[16]; //tsw buffer改为tsw01共用16路, tsw23共用16路
	struct reg_demux_tsw_buf reserve_tsw23[16]; //tsw buffer改为tsw01共用16路, tsw23共用16路
	struct reg_demux_dfaddr df_addr[64];
	unsigned int axi_8to32_cfg;
	unsigned int df_payload_byt_en;
	unsigned int buf_ready;
	unsigned int ts_i_m2ts_sel; //cygnus 去掉删除t2mi功能
	unsigned int ts_pin_mux_sel[4];
	unsigned int discard_pkt_enable;
	unsigned int discard_pkt_cfg[4];
	unsigned int ts_des_flag_clean_enable;
};
#endif
