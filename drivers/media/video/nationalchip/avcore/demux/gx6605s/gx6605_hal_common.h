#ifndef __HAL_GX6605_COMMON_H__
#define __HAL_GX6605_COMMON_H__

#include "gx6605_demux_regs.h"
#include "gxav_common.h"
#include "gx6605_demux.h"

#define TWO_FILTER_BLANK         2
#define VERSION_BYTE_DEPTH       3

#define SPDIF_HEAD               0x0B77
#define GXAV_DEMUX_SIZE          0x00001000


void gx6605_disable_slot         (struct dmx_slot *slot);
void gx6605_reset_cc             (struct dmx_slot *slot);
void gx6605_set_pid_slot         (struct dmx_slot *slot);
void gx6605_clr_pid_slot         (struct dmx_slot *slot);
void gx6605_set_stop_unit        (struct dmx_slot *slot);
void gx6605_set_interrupt_unit   (struct dmx_slot *slot);
void gx6605_set_filter_type      (struct dmx_slot *slot);
void gx6605_set_crc_disable      (struct dmx_slot *slot);
void gx6605_set_type_sel         (struct dmx_slot *slot);
void gx6605_set_av_type          (struct dmx_slot *slot);
void gx6605_set_pts_bypass       (struct dmx_slot *slot);
void gx6605_set_pts_to_sdram     (struct dmx_slot *slot);
void gx6605_close_pts_buffer_cnt (struct dmx_slot *slot);
void gx6605_open_pts_buffer_cnt  (struct dmx_slot *slot);
void gx6605_clr_pts_to_sdram     (struct dmx_slot *slot);
void gx6605_clr_slot_cfg         (struct dmx_slot *slot);
void gx6605_set_pid              (struct dmx_slot *slot);
void gx6605_enable_slot          (struct dmx_slot *slot);
void gx6605_set_avnum            (struct dmx_slot *slot);
void gx6605_link_avbuf           (struct dmx_slot *slot, unsigned int start_addr, unsigned int size, unsigned char id);
void gx6605_unlink_avbuf         (struct dmx_slot *slot);
void gx6605_link_ptsbuf          (struct dmx_slot *slot, unsigned int start_addr, unsigned int size, unsigned char id);
void gx6605_unlink_ptsbuf        (struct dmx_slot *slot);
void gx6605_set_av_out           (struct dmx_slot *slot);
void gx6605_set_ts_out           (struct dmx_slot *slot);
void gx6605_set_err_discard      (struct dmx_slot *slot);
void gx6605_set_dup_discard      (struct dmx_slot *slot);

void gx6605_set_audio_ac3(void);
void gx6605_clear_spdif(void);
void gx6605_clear_sram(void);
void gx6605_set_clock(void);
void gx6605_set_gate(struct dmx_demux *dmx);
void gx6605_set_start(void);
void gx6605_link_rts_sdc(unsigned int start_addr, unsigned int size, unsigned char id);
void gx6605_link_wts_sdc   (struct dmx_demux *dmx,unsigned int start_addr, unsigned int size, unsigned char id);
void gx6605_unlink_wts_sdc (struct dmx_demux *dmx,unsigned char id);
void gx6605_set_ts_source  (struct dmx_demux *dmx);
void gx6605_set_ts_mode    (struct dmx_demux *dmx);
void gx6605_set_pcr_sync   (struct dmx_demux *demux);
void gx6605_query_tslock   (struct dmx_demux *dmx);
int  gx6605_wts_cb(unsigned int id, unsigned int len, unsigned int dataflow, void *data);
int  gx6605_rts_cb(unsigned int id, unsigned int len, unsigned int dataflow, void *data);
void gx6605_unlink_rts_sdc(void);
void gx6605_set_apts_sync(unsigned long apts);
void gx6605_set_descramble(struct dmx_ca *ca, struct dmx_slot *slot);
void gx6605_set_mtc_descramble(struct dmx_ca *ca, struct dmx_slot *slot);
void gx6605_mtc_decrypt(gx_mtc_info *mtc_info_cfg, struct dmx_ca *ca);

void gx6605_clear_df_rw     ( int index);
void gx6605_clear_df_buf     ( int index);
void gx6605_clear_int_df    ( int index);
void gx6605_disable_filter  ( int index);
void gx6605_disable_pid_sel ( int slot_index, int filter_index);
void gx6605_set_select            (struct dmx_slot *slot, struct dmx_filter *filter);
void gx6605_set_filter_reset      (struct dmx_filter *filter);
void gx6605_set_dfbuf             (struct dmx_filter *filter);
void gx6605_set_filtrate          (struct dmx_filter *filter);
void gx6605_enable_filter         (struct dmx_filter *filter);
int gx6605_filter_ts_status       (struct dmx_filter *filter);
unsigned int gx6605_filter_ts_get (struct dmx_filter *filter, void *data_buf, unsigned int size);

#endif  /*__HAL_GX6605_COMMON_H__*/
