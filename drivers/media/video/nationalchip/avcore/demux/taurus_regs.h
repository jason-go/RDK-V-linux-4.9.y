#ifndef _TAURUS_DEMUX_REGS_H_
#define _TAURUS_DEMUX_REGS_H_

#include "gx3211_regs.h"
#include "sirius_regs.h"

#define T2MI_BASE_ADDR             (0x04000000+0x8000)
#define TAURUS_DEMUX_BASE_CAS      (0x04000000+0x20000)
#define TAURUS_GSE_BASE_ADDR       (0x04300000)
#define TAURUS_PFILTER_BASE_ADDR   (0x04100000+0x1000)

#define TAURUS_GSE_IRQ             50
#define TAURUS_PFILTER_IRQ         51

#define TAURUS_MAX_DEMUX_UNIT      1
#define TAURUS_MAX_DVR_UNIT        2
#define TAURUS_MAX_FILTER_NUM      32
#define TAURUS_MAX_SLOT_NUM        32
#define TAURUS_MAX_AVBUF_NUM       4
#define TAURUS_MAX_T2MI_PID        2
#define TAURUS_MAX_PFILTER_UNIT    1
#define TAURUS_MAX_PFILTER_PID     32

#define TAURUS_NUM_PID_CFG_SEL     32
#define TAURUS_NUM_DF_CHNL         32
#define TAURUS_NUM_DF_DEPTH        4
#define TAURUS_NUM_TSW_SLOT_SRAM   80

#define TAURUS_TSW_SRC_SYNC0             0
#define TAURUS_TSW_SRC_SYNC1             1
#define TAURUS_TSW_SRC_SYNC2             2
#define TAURUS_TSW_SRC_TSR               3
#define TAURUS_TSW_SRC_DMX_FOR_TSWSLOT   3
#define TAURUS_TSW_SRC_TSR_FOR_TSWSLOT   4
#define TAURUS_TSW_SRC_DMXSLOT           5
#define TAURUS_TSW_SRC_TSWSLOT           6

#define TAURUS_BIT_PID_CFG_TSW_BUF          (23)
#define TAURUS_PID_CFG_TSW_BUF_SEL          (0x1f << TAURUS_BIT_PID_CFG_TSW_BUF)

#define TAURUS_BIT_SYS_SEL(i)               (2*i)
#define TAURUS_MSK_SYS_SEL(i)               (0x3 << TAURUS_BIT_SYS_SEL(i))

struct taurus_reg_demux_slot {
	unsigned int pid_cfg;
	unsigned int pid_sel_l;
};

struct taurus_reg_demux_m01 {
	unsigned int m0_byte_l;
	unsigned int m1_byte_l;
};

struct taurus_reg_demux_byte {
	struct taurus_reg_demux_m01   m_byte[NUM_MATCH_BIT];
};

struct taurus_reg_demux_avaddr {
	unsigned int av_start_addr;
	unsigned int av_buf_size;
	unsigned int av_write_addr;
	unsigned int av_read_addr;
	unsigned int av_almostfull_gate;
	unsigned int reserved;
	unsigned int av_es_start_addr;
	unsigned int av_es_end_addr;
};

struct taurus_reg_demux {
	unsigned int                        tsw_pid_en[2];
	unsigned int                        reserved;
	unsigned int                        tsw_pid[NUM_PID_BIT];
	struct taurus_reg_demux_slot        tsw_pid_cfg_sel[TAURUS_NUM_PID_CFG_SEL];
	unsigned int                        reserved_a[240];

	unsigned int                        pid_en[2];
	unsigned int                        reserved_0;
	unsigned int                        pid[NUM_PID_BIT];
	struct taurus_reg_demux_slot        pid_cfg_sel[TAURUS_NUM_PID_CFG_SEL];
	struct reg_demux_key                key_even_odd[NUM_KEY_PAIR];  // for gx6605s
	unsigned int                        reserved_1[368];

	struct taurus_reg_demux_byte        m_byte_n[NUM_MATCH_BYTE];
	unsigned int                        reserved_2[256];

	unsigned int int_ts_if;
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
	unsigned int int_dmx_tsr_buf;
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
	unsigned int pts_mode;
	struct reg_demux_rts_buf rts_buf;
	unsigned int tsw_buf_en_0_h;
	unsigned int tsw_buf_en_0_l;
	unsigned int tsw_buf_en_1_h;
	unsigned int tsw_buf_en_1_l;
	unsigned int ts_write_ctrl;
	struct reg_demux_rts_buf rts_buf1;
	struct reg_demux_rts_buf pfilter_rts_buf[2];
	unsigned int int_tsr23_buf;
	unsigned int int_tsr23_buf_en;
	unsigned int reserv_33[29];
	unsigned int internel_clk_div0;
	unsigned int internel_clk_div1;
	unsigned int reserv_34[2];
	struct reg_demux_tsw_buf tsw_buf[64];
	struct reg_demux_dfaddr df_addr[64];
	unsigned int axi_8to32_cfg;
	unsigned int df_payload_byt_en;
	unsigned int buf_ready;
	unsigned int ts_i_m2ts_sel;
	unsigned int ts_pin_mux_sel[4];
	unsigned int reserv_35[508];
	unsigned int ts_err_mask[2];
};

struct taurus_reg_t2mi_unit {
	unsigned int t2mi_cfg0;
	unsigned int reserved_0[11];
	unsigned int t2mi_pid;
	unsigned int reserved_1;
	unsigned int t2mi_cfg1;
	unsigned int t2mi_cfg2;
	unsigned int reserved_2[5];
};

struct taurus_reg_t2mi {
	struct taurus_reg_t2mi_unit t2mi0[2];
	unsigned int reserved_0[982];
	struct taurus_reg_t2mi_unit t2mi1[2];
};

typedef union {
	unsigned int value;
	struct {
		unsigned input_mode      : 2;
		unsigned nosync          : 1;
		unsigned novalid         : 1;
		unsigned ts_des_en       : 1;
		unsigned ts_big_endian   : 1;
		unsigned pid_in_sdram    : 1;
		unsigned store_all       : 2;
		unsigned ts_input_sel    : 2;
		unsigned                 : 1;
		unsigned sync_gate       : 4;
		unsigned loss_gate       : 4;
		unsigned pid_endian      : 3;
		unsigned                 : 1;
		unsigned head_endian     : 3;
		unsigned                 : 4;
		unsigned chan_sync_en    : 1;
	} bits;
} rCHAN_CTRL_L_TAURUS;

#define BIT_TAURUS_PFILTER_CTRL0_INPUT_MODE  (0)
#define BIT_TAURUS_PFILTER_CTRL0_NOSYNC      (2)
#define BIT_TAURUS_PFILTER_CTRL0_NOVALID     (3)
#define BIT_TAURUS_PFILTER_CTRL0_PID_BUF_EN  (6)
#define BIT_TAURUS_PFILTER_CTRL0_STORE_MODE  (7)

#define MSK_TAURUS_PFILTER_CTRL0_INPUT_MODE  (0x3<<0)
#define MSK_TAURUS_PFILTER_CTRL0_NOSYNC      (0x1<<2)
#define MSK_TAURUS_PFILTER_CTRL0_NOVALID     (0x1<<3)
#define MSK_TAURUS_PFILTER_CTRL0_STORE_MODE  (0x3<<7)

struct taurus_reg_pid_filter {
	unsigned int pid_en_h;
	unsigned int pid_en_l;
	unsigned int pid[32];
	rCHAN_CTRL_L_TAURUS ctrl0;
	rCHAN_CTRL_H ctrl1;
	unsigned int pid_buf_addr;
	unsigned int pid_buf_len;
	unsigned int buf_header;
	unsigned int buf_full_gate;
	unsigned int buf_rptr;
	rPFilterInt  pfilter_int;
	unsigned int buf_wptr_logic;
	// RO-REG for debug current list_buf
	unsigned int buf_wptr_real;
	unsigned int buf_start_addr;
	unsigned int buf_len;
	unsigned int buf_next_header;
	unsigned int buf_wptr_offset;

	unsigned int buf_state;
	unsigned int cfg_req_timeout_gate;
	unsigned int stream_timeout_gate;
	unsigned int buf_almost_full_gate;

	unsigned int pin_mux_sel;
};

#endif

