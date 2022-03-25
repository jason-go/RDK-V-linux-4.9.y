#ifndef __HAL_GX3211_COMMON_H__
#define __HAL_GX3211_COMMON_H__

#include "gx3211_demux_regs.h"
#include "gxav_common.h"
#include "gx3211_demux.h"

#define TWO_FILTER_BLANK         2
#define VERSION_BYTE_DEPTH       3

#define SPDIF_HEAD               0x0B77
#define GXAV_DEMUX_SIZE          0x00001000


void gx3211_disable_slot         (struct dmx_slot *slot);
void gx3211_reset_cc             (struct dmx_slot *slot);
void gx3211_set_stop_unit        (struct dmx_slot *slot);
void gx3211_set_interrupt_unit   (struct dmx_slot *slot);
void gx3211_set_filter_type      (struct dmx_slot *slot);
void gx3211_set_crc_disable      (struct dmx_slot *slot);
void gx3211_set_type_sel         (struct dmx_slot *slot);
void gx3211_set_av_type          (struct dmx_slot *slot);
void gx3211_set_pts_bypass       (struct dmx_slot *slot);
void gx3211_set_pts_to_sdram     (struct dmx_slot *slot);
void gx3211_close_pts_buffer_cnt (struct dmx_slot *slot);
void gx3211_open_pts_buffer_cnt  (struct dmx_slot *slot);
void gx3211_clr_pts_to_sdram     (struct dmx_slot *slot);
void gx3211_clr_slot_cfg         (struct dmx_slot *slot);
void gx3211_set_pid              (struct dmx_slot *slot);
void gx3211_enable_slot          (struct dmx_slot *slot);
void gx3211_set_avnum            (struct dmx_slot *slot);
void gx3211_link_avbuf           (struct dmx_slot *slot, unsigned int start_addr, unsigned int size, unsigned char id);
void gx3211_unlink_avbuf         (struct dmx_slot *slot);
void gx3211_link_ptsbuf          (struct dmx_slot *slot, unsigned int start_addr, unsigned int size, unsigned char id);
void gx3211_unlink_ptsbuf        (struct dmx_slot *slot);
void gx3211_set_av_out           (struct dmx_slot *slot);
void gx3211_set_ts_out           (struct dmx_slot *slot);
void gx3211_set_err_discard      (struct dmx_slot *slot);
void gx3211_set_dup_discard      (struct dmx_slot *slot);
void gx3211_set_audio_ac3        (struct dmx_slot *slot);
void gx3211_clear_spdif          (struct dmx_slot *slot);

void gx3211_clear_sram           (struct reg_demux *reg);
void gx3211_clr_ints             (struct reg_demux *reg);
void gx3211_set_clock            (struct reg_demux *reg);
void gx3211_set_start            (struct reg_demux *reg);
void gx3211_set_gate             (struct dmx_demux *dmx);
void gx3211_link_rts_sdc         (struct dmx_demux *dmx,unsigned int start_addr, unsigned int size, unsigned char id);
void gx3211_unlink_rts_sdc       (struct dmx_demux *dmx);
void gx3211_link_wts_sdc         (struct dmx_demux *dmx,unsigned int start_addr, unsigned int size, unsigned char id);
void gx3211_unlink_wts_sdc       (struct dmx_demux *dmx,unsigned char id);
void gx3211_set_tsw              (struct dmx_demux *dmx,unsigned int slotid);
void gx3211_set_ts_source        (struct dmx_demux *dmx);
void gx3211_set_ts_mode          (struct dmx_demux *dmx);
void gx3211_set_pcr_sync         (struct dmx_demux *dmx);
void gx3211_query_tslock         (struct dmx_demux *dmx);
int  gx3211_wts_cb(unsigned int id, unsigned int len, unsigned int dataflow, void *data);
int  gx3211_rts_cb(unsigned int id, unsigned int len, unsigned int dataflow, void *data);
void gx3211_set_apts_sync(unsigned long apts);
// void gx3211_set_sync_mode(unsigned int stc_id, enum sync mode);
void gx3211_set_descramble(struct dmx_ca *ca, struct dmx_slot *slot);

void gx3211_clear_df_rw           (struct reg_demux *reg,unsigned int index);
void gx3211_clear_int_df          (struct dmx_filter *filter);
void gx3211_disable_filter        (struct dmx_filter *filter);
void gx3211_disable_pid_sel       (struct dmx_filter *filter,struct dmx_slot *slot);
void gx3211_set_select            (struct dmx_filter *filter,struct dmx_slot *slot);
void gx3211_set_filter_reset      (struct dmx_filter *filter);
void gx3211_set_dfbuf             (struct dmx_filter *filter);
void gx3211_set_filtrate          (struct dmx_filter *filter);
void gx3211_enable_filter         (struct dmx_filter *filter);
int gx3211_filter_ts_status       (struct dmx_filter *filter);
unsigned int gx3211_filter_ts_get (struct dmx_filter *filter, void *data_buf, unsigned int size);


#endif  /*__HAL_GX3211_COMMON_H__*/
