#ifndef _SIRIUS_DEMUX_REGS_H_
#define _SIRIUS_DEMUX_REGS_H_

#include "gx3211_regs.h"

#define SIRIUS_DEMUX_BASE_SYS              (0x8a200000+0x1b00)
#define SIRIUS_DEMUX_BASE_SYS1             (0x8a200000+0x11b00)
#define SIRIUS_DEMUX_BASE_CAS              (0x8a200000+0x20000)
#define SIRIUS_DEMUX_BASE_GP               (0x8a000000)
#define SIRIUS_DEMUX_BASE_GSE              (0x8a100000)
#define SIRIUS_DEMUX_BASE_PID_FILTER0      (0x8aa00000+0x1000)
#define SIRIUS_DEMUX_BASE_PID_FILTER1      (0x8aa00000+0x1100)
#define SIRIUS_DEMUX_BASE_PID_FILTER2      (0x8aa00000+0x1200)

#define SIRIUS_IRQ0         1
#define SIRIUS_IRQ1         9
#define SIRIUS_GSE_IRQ      10
#define SIRIUS_GP_IRQ       3
#define SIRIUS_PFILTER_IRQ  2

#define SIRIUS_TSW_SRC_SYNC0             0
#define SIRIUS_TSW_SRC_SYNC1             1
#define SIRIUS_TSW_SRC_SYNC2             2
#define SIRIUS_TSW_SRC_TSR               3
#define SIRIUS_TSW_SRC_DMXSLOT           4

/* pid cfg */
#define SIRIUS_BIT_PID_CFG_KEY_ID          12
#define SIRIUS_BIT_PID_CFG_KEY_VALID       17
#define SIRIUS_PID_CFG_KEY_ID              (0x1F << SIRIUS_BIT_PID_CFG_KEY_ID)
#define SIRIUS_PID_CFG_KEY_VALID           (0x01 << SIRIUS_BIT_PID_CFG_KEY_VALID)

/* cas modeule */
#define BIT_DEMUX_CW_WEN                   0
#define BIT_DEMUX_CW_WADDR                 16
#define DEMUX_CW_WEN                       (0x1 << BIT_DEMUX_CW_WEN)
#define DEMUX_CW_WADDR                     (0xff<< BIT_DEMUX_CW_WADDR)

#define BIT_DEMUX_CA_MODE                  25
#define DEMUX_CA_MODE                      (0x3 << BIT_DEMUX_CA_MODE)

/*debug:  gx_printf("cw_iv addr(%d) %d : %x\n", addr, i, val[i]);*/
#define DEMUX_SET_CW_IV(cas_reg, val, addr) \
	do { \
		int i; \
		for (i = 0; i < 4; i++) {\
			REG_SET_VAL(&(cas_reg->cw_buffer[i]), val[i]); \
		}\
		REG_SET_VAL(&(cas_reg->cw_wctrl), (((addr)<<BIT_DEMUX_CW_WADDR) | 0)); \
		REG_SET_VAL(&(cas_reg->cw_wctrl), (((addr)<<BIT_DEMUX_CW_WADDR) | 1)); \
		REG_SET_VAL(&(cas_reg->cw_wctrl), (((addr)<<BIT_DEMUX_CW_WADDR) | 0)); \
	} while(0)

struct cw_config {
       unsigned int cw_odd[4];
       unsigned int cw_even[4];
};

struct iv_config {
       unsigned int iv_odd[4];
       unsigned int iv_even[4];
};

struct tsr_buf {
       unsigned int iv_odd[4];
       unsigned int iv_even[4];
};

/*
 * cw_addr
 *    0x00 : cw
 *    0x40 : iv1
 *    0x80 : iv2
 *    0xc0 : multi2 cw
 */
struct sirius_reg_cas {
	unsigned int     cas_mode;
	unsigned int     ds_mode[4];
	unsigned int     cw_buffer[4];
	unsigned int     cw_wctrl;
	unsigned int     sys_sel[2];
	unsigned int     reserved[52];
	struct cw_config cw_config[32];
	struct iv_config iv_config[32];
};

struct sirius_reg_demux {
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

	unsigned int serial_mode;
	unsigned int pts_mode;
	struct reg_demux_rts_buf rts_buf;
	unsigned int tsw_buf_en_0_h;
	unsigned int tsw_buf_en_0_l;
	unsigned int tsw_buf_en_1_h;
	unsigned int tsw_buf_en_1_l;
	unsigned int ts_write_ctrl;
	struct reg_demux_rts_buf rts_buf1;
	unsigned int reserv_h[43];
	unsigned int cas_div;
	unsigned int reserv_g[3];
	struct reg_demux_tsw_buf tsw_buf[64];
	struct reg_demux_dfaddr df_addr[64];
	unsigned int axi_8to32_cfg;
};

/*************************************** PIDFILTER ********************************************/

#define BIT_PFILTER_ISR_TS_LOCK     (8)
#define BIT_PFILTER_ISR_TS_LOSS     (9)
#define BIT_PFILTER_ISR_ALLOW_CFG   (10)
#define BIT_PFILTER_ISR_BUF_FULL    (11)
#define BIT_PFILTER_ISR_TIME_ERR    (12)
#define BIT_PFILTER_ISR_BUF_AFULL   (13)

#define BIT_PFILTER_CTRL1_CFG_REQ     (0)
#define BIT_PFILTER_CTRL1_CLK_REVERSE (2)
#define BIT_PFILTER_CTRL1_OUTPUT_EN   (3)
#define BIT_PFILTER_CTRL1_INT_EN      (8)

#define MSK_PFILTER_CTRL1_INT_EN      (0x3f<<8)

#define BIT_PFILTER_CTRL0_INPUT_MODE  (0)
#define BIT_PFILTER_CTRL0_NOSYNC      (1)
#define BIT_PFILTER_CTRL0_NOVALID     (2)
#define BIT_PFILTER_CTRL0_STORE_MODE  (6)
#define BIT_PFILTER_CTRL0_TS_SEL      (9)
#define BIT_PFILTER_CTRL0_SYNC_GATE   (12)
#define BIT_PFILTER_CTRL0_LOSS_GATE   (16)
#define BIT_PFILTER_CTRL0_SYNC_EN     (31)

#define MSK_PFILTER_CTRL0_INPUT_MODE  (0x1<<0)
#define MSK_PFILTER_CTRL0_NOSYNC      (0x1<<1)
#define MSK_PFILTER_CTRL0_NOVALID     (0x1<<2)
#define MSK_PFILTER_CTRL0_STORE_MODE  (0x3<<6)
#define MSK_PFILTER_CTRL0_TS_SEL      (0x3<<9)
#define MSK_PFILTER_CTRL0_SYNC_GATE   (0xf<<12)
#define MSK_PFILTER_CTRL0_LOSS_GATE   (0xf<<16)

typedef union {
	unsigned int value;
	struct {
		unsigned input_mode      : 1;
		unsigned nosync          : 1;
		unsigned novalid         : 1;
		unsigned ts_des_en       : 1;
		unsigned ts_big_endian   : 1;
		unsigned pid_in_sdram    : 1;
		unsigned store_all       : 2;
		unsigned serial_mode     : 1;
		unsigned ts_input_sel    : 2;
		unsigned                 : 1;
		unsigned sync_gate       : 4;
		unsigned loss_gate       : 4;
		unsigned pid_endian      : 3;
		unsigned                 : 1;
		unsigned head_endian     : 3;
		unsigned                 : 1;
		unsigned pin_mux_sel     : 3;
		unsigned chan_sync_en    : 1;
	} bits;
} rCHAN_CTRL_L;

typedef union {
	unsigned int value;
	struct {
		unsigned cfg_req          : 1;
		unsigned buf_continue     : 1;
		unsigned clock_mode       : 1;
		unsigned output_en        : 1;
		unsigned                  : 4;
		unsigned int_ts_lock_en   : 1;
		unsigned int_ts_loss_en   : 1;
		unsigned int_chan_cfg_en  : 1;
		unsigned int_buf_full_en  : 1;
		unsigned int_time_err_en  : 1;
		unsigned int_buf_afull_en : 1;
		unsigned                  : 10;
		unsigned sync_byte        : 8;
	} bits;
} rCHAN_CTRL_H;

typedef union {
	unsigned int value;
	struct {
		unsigned ts_loss_info     : 3;
		unsigned cfg_req_timeout  : 1;
		unsigned                  : 4;
		unsigned int_ts_lock      : 1;
		unsigned int_ts_loss      : 1;
		unsigned int_allow_cfg    : 1;
		unsigned int_buf_full     : 1;
		unsigned int_time_err     : 1;
		unsigned int_buf_afull    : 1;
	} bits;
} rPFilterInt;

struct sirius_reg_pid_filter {
	unsigned int pid_en_h;
	unsigned int pid_en_l;
	unsigned int pid[32];
	rCHAN_CTRL_L ctrl0;
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
};

/*************************************** GSE ********************************************/

/* label type */
#define BIT_GSE_LABEL_TYPE          0
#define BIT_GSE_LABEL_TYPE_MASK     3

#define GSE_LABEL_TYPE              (0x03 << BIT_GSE_LABEL_TYPE)
#define GSE_LABEL_TYPE_MASK         (0x03 << BIT_GSE_LABEL_TYPE_MASK)

/* protocol type */
#define BIT_GSE_PROTOCOL_TYPE       0
#define BIT_GSE_PROTOCOL_TYPE_MASK  16

#define GSE_PROTOCOL_TYPE           (0xFFFF << BIT_GSE_PROTOCOL_TYPE)
#define GSE_PROTOCOL_TYPE_MASK      (0xFFFF << BIT_GSE_PROTOCOL_TYPE_MASK)

#define GSE_PROTOCOL_VAL_IPV4       0x0800
#define GSE_PROTOCOL_VAL_IPV6       0x86DD

/* label val */
#define BIT_GSE_LABEL_H             0
#define BIT_GSE_LABEL_L             0
#define BIT_GSE_LABEL_MASK_H        0
#define BIT_GSE_LABEL_MASK_L        0

#define GSE_LABEL_H                 (0xFFFF << BIT_GSE_LABEL_H)
#define GSE_LABEL_L                 (0xFFFFFFFF << BIT_GSE_LABEL_L)
#define GSE_LABEL_MASK_H            (0xFFFF << BIT_GSE_LABEL_MASK_H)
#define GSE_LABEL_MASK_L            (0xFFFFFFFF << BIT_GSE_LABEL_MASK_L)

/* if ctrl */
#define BIT_GSE_GS_IN_SEL           0
#define BIT_GSE_PLAY_RECORD         2
#define BIT_GSE_SERIAL_IN           3
#define BIT_GSE_NOVALID_MOD         4
#define BIT_GSE_BIG_ENDIAN          5
#define BIT_GSE_CLOCK_MOD           6
#define BIT_GSE_WORK_EN             7
#define BIT_GSE_PIN_MUX_SEL         8
#define BIT_GSE_RECORD_MODE         11
#define BIT_GSE_LOCK_GATE           16
#define BIT_GSE_LOSS_GATE           20
#define BIT_GSE_RECORD_BUF_SEL      24
#define BIT_GSE_SERIAL_2_4_BIT      29
#define BIT_GSE_INPUT_SYNC_MODE     30

#define GSE_GS_IN_SEL               (0x03 << BIT_GSE_GS_IN_SEL)
#define GSE_PLAY_RECORD             (0x01 << BIT_GSE_PLAY_RECORD)
#define GSE_SERIAL_IN               (0x01 << BIT_GSE_SERIAL_IN)
#define GSE_NOVALID_MOD             (0x01 << BIT_GSE_NOVALID_MOD)
#define GSE_BIG_ENDIAN              (0x01 << BIT_GSE_BIG_ENDIAN)
#define GSE_CLOCK_MOD               (0x01 << BIT_GSE_CLOCK_MOD)
#define GSE_WORK_EN                 (0x01 << BIT_GSE_WORK_EN)
#define GSE_PIN_MUX_SEL             (0x07 << BIT_GSE_PIN_MUX_SEL)
#define GSE_LOCK_GATE               (0x0F << BIT_GSE_LOCK_GATE)
#define GSE_LOSS_GATE               (0x0F << BIT_GSE_LOSS_GATE)
#define GSE_RECORD_BUF_SEL          (0x1F << BIT_GSE_RECORD_BUF_SEL)
#define GSE_INPUT_SYNC_MODE         (0x01 << BIT_GSE_INPUT_SYNC_MODE)

#define GSE_GS_SEL_PORT1            0
#define GSE_GS_SEL_PORT2            1
#define GSE_GS_SEL_PORT3            2
#define GSE_GS_SEL_SDRAM            3

/*over_time interupt*/
#define BIT_GSE_LOCK_INT_EN         0
#define BIT_GSE_LOSS_INT_EN         1
#define BIT_GSE_OVERTIME_INT_EN     2
#define BIT_GSE_LOCK_INT            4
#define BIT_GSE_LOSS_INT            5
#define BIT_GSE_OVERTIME_INT        6
#define BIT_GSE_RECORD_FINISH       7
#define BIT_GSE_RECORD_FINISH_EN    8
#define BIT_GSE_OVERTIME_GATE       16

#define GSE_LOCK_INT_EN             (0x01 << BIT_GSE_LOCK_INT_EN)
#define GSE_LOSS_INT_EN             (0x01 << BIT_GSE_LOSS_INT_EN)
#define GSE_OVERTIME_INT_EN         (0x01 << BIT_GSE_OVERTIME_INT_EN)
#define GSE_LOCK_INT                (0x01 << BIT_GSE_LOCK_INT)
#define GSE_LOSS_INT                (0x01 << BIT_GSE_LOSS_INT)
#define GSE_OVERTIME_INT            (0x01 << BIT_GSE_OVERTIME_INT)
#define GSE_RECORD_FINISH           (0x01 << BIT_GSE_RECORD_FINISH)
#define GSE_RECORD_FINISH_EN        (0x01 << BIT_GSE_RECORD_FINISH_EN)
#define GSE_OVERTIME_GATE           (0xFFFF << BIT_GSE_OVERTIME_GATE)

struct reg_gse_label {
    unsigned int label_h;
    unsigned int label_l;
};

struct reg_gse_label_mask {
    unsigned int label_mask_h;
    unsigned int label_mask_l;
};

struct reg_gse_data_buf {
    unsigned int start_addr;
    unsigned int buf_len;
    unsigned int phy_write_ptr;
    unsigned int logic_read_ptr;
    unsigned int logic_write_ptr;
    unsigned int reserve[3];
};

struct reg_gse_status_buf {
    unsigned int start_addr;
    unsigned int buf_len;
    unsigned int phy_write_ptr;
    unsigned int phy_read_ptr;
};

struct reg_gse_slot {
    unsigned int              label_type;
    unsigned int              protocol_type;
    struct reg_gse_label      label_val;
    struct reg_gse_label_mask label_mask;
};

struct reg_gse_filter {
    struct reg_gse_data_buf   data_buf[32];
    struct reg_gse_status_buf status_buf[32];
};

struct sirius_reg_gse {
    unsigned int          slot_en;
    struct reg_gse_slot   slots[32];

    unsigned int          gse_ctrl;
    struct reg_gse_filter filters;

    unsigned int          int_pdu_finish_en;
    unsigned int          int_pdu_finish;
    unsigned int          int_almost_full_en;
    unsigned int          int_almost_full;
    unsigned int          almost_full_gate[32];
    unsigned int          int_over_time;
    unsigned int          slot_buf_full_stop;
    unsigned int          int_buf_full_en;
    unsigned int          int_buf_full;
    unsigned int          reserve[406];
    unsigned int          frag_id_valid;
    unsigned int          frag_id[8];
    unsigned int          lable_standard_set;
    unsigned int          int_pdu_unfinish_en;
    unsigned int          int_pdu_unfinish;
    unsigned int          int_slot_close_finish_en;
    unsigned int          int_slot_close_finish;
};

/*************************************** GP ********************************************/

typedef union {
	unsigned int value;
	struct {
		unsigned gp_en                       : 1;
		unsigned in_endian                   : 2;
		unsigned out_endian                  : 2;
		unsigned                             : 3;
		unsigned gp_idle_int_en              : 1;
		unsigned                             : 11;
		unsigned av_buf_almost_full_int_en   : 4;
	} bits;
} rGP_CFG;

typedef union {
	unsigned int value;
	struct {
		unsigned                          : 4;
		unsigned av_buf_almost_full_int   : 4;
		unsigned                          : 4;
		unsigned gp_idle_int              : 1;
	} bits;
} rGP_INT;

typedef union {
	unsigned int value;
	struct {
		unsigned cw_wen          : 1;
		unsigned                 : 15;
		unsigned cw_addr         : 6;
	} bits;
} rGP_CW_WCTRL;

typedef struct {
	unsigned int start_addr;
	unsigned int size;
	unsigned int rptr;
	unsigned int wptr;
	unsigned int almost_full_gate;
} rGP_AV_CHANNEL;

struct sirius_reg_gp {
	rGP_CFG              config;
	rGP_INT              gp_int;
	unsigned int         ds_mode[8];
	unsigned int         cw_buffer[4];
	rGP_CW_WCTRL         wctrl;
	rGP_AV_CHANNEL       channel[4];
	unsigned int         logic_cmd_fifo_len;
	unsigned int         logic_cmd_fifo_wcount;
	unsigned int         cmd_fifo_start_addr;
	unsigned int         reserved_0[282];
	unsigned int         iv[64][4];            // 0x500
};

#endif

