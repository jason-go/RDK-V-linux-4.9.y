#ifndef __GX3211_DEMUX__
#define __GX3211_DEMUX__

#include "gxdemux.h"

#define TSW_FIFO_PUT(fifo, buf, len, flag) \
	do{ \
		if (flag & DVR_FLAG_PTR_MODE_EN) \
			gxfifo_shallow_put(fifo, buf, len); \
		else \
			gxfifo_put(fifo, buf, len); \
	}while(0);

void gx3211_clr_ints(void *vreg);
void gx3211_hw_init(void *vreg);
void gx3211_set_sync_gate(void *vreg, unsigned char timing_gate,
	unsigned char byt_cnt_err_gate, unsigned char sync_loss_gate, unsigned char sync_lock_gate);
int gx3211_query_tslock(void *vreg, unsigned int dmxid, unsigned int source);
void gx3211_set_input_source(void *vreg, unsigned int dmxid,unsigned int source);
void gx3211_set_apts_sync(void *vreg,int avid);
void gx3211_set_pcr_sync(void *vreg, unsigned int dmxid,unsigned short pcr_pid);
void gx3211_set_oddkey(void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int alg);
void gx3211_set_evenkey(void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int alg);
void gx3211_enable_av_int(void *vreg, int avid);
void gx3211_disable_av_int(void *vreg, int avid);
void gx3211_link_avbuf(int dmxid,void *vreg, int avid,unsigned int start_addr, unsigned int size, unsigned int gate);
void gx3211_unlink_avbuf(int dmxid,void *vreg,int avid);
void gx3211_link_ptsbuf(void *vreg, int avid,unsigned int start_addr, unsigned int size, unsigned int gate);
void gx3211_unlink_ptsbuf(void *vreg,int avid);
void gx3211_disable_slot(void *vreg,unsigned int dmxid,unsigned int slotid);
void gx3211_enable_slot(void *vreg,unsigned int dmxid,unsigned int slotid);
void gx3211_reset_cc(void *vreg,unsigned int slotid);
void gx3211_set_repeat_mode(void *vreg,unsigned int slotid);
void gx3211_set_one_mode(void *vreg,unsigned int slotid);
void gx3211_set_interrupt_unit(void *vreg,unsigned int slotid);
void gx3211_set_err_discard(void *vreg,unsigned int slotid, int flag);
void gx3211_set_dup_discard(void *vreg,unsigned int slotid, int flag);
void gx3211_set_av_out(void *vreg,unsigned int slotid, int flag);
void gx3211_set_ts_out(void *vreg,unsigned int slotid, int flag);
void gx3211_set_des_out(void *vreg,unsigned int slotid, int flag);
void gx3211_set_crc_disable(void *vreg,unsigned int slotid);
void gx3211_set_psi_type(void *vreg,unsigned int slotid);
void gx3211_set_pes_type(void *vreg,unsigned int slotid);
void gx3211_set_av_type(void *vreg,unsigned int slotid);
void gx3211_set_pts_bypass(void *vreg,unsigned int slotid);
void gx3211_set_pts_insert(void *vreg,unsigned int slotid);
void gx3211_set_pts_to_sdram(void *vreg,unsigned int slotid);
void gx3211_clr_slot_cfg(void *vreg,unsigned int slotid);
void gx3211_set_pid(void *vreg,unsigned int slotid,unsigned short pid);
void gx3211_get_pid(void *vreg,unsigned int slotid,unsigned short *pid);
void gx3211_set_avid(void *vreg,unsigned int slotid,int avid);
void gx3211_set_descid(void *vreg, unsigned int slotid,unsigned int caid, unsigned int flags);
void gx3211_set_even_valid(void *vreg, unsigned int slotid);
void gx3211_set_odd_valid(void *vreg, unsigned int slotid);
void gx3211_set_filterid(void *vreg, unsigned int slotid,unsigned int filterid);
void gx3211_clr_filterid(void *vreg, unsigned int slotid,unsigned int filterid);
void gx3211_clr_dfrw(void *vreg,unsigned int filterid);
void gx3211_clr_int_df(void *vreg,unsigned int filterid);
void gx3211_clr_int_df_en(void *vreg,unsigned int filterid);
void gx3211_set_dfbuf(void *vreg,unsigned int filterid,unsigned int start,unsigned int size,unsigned int gate);
void gx3211_set_select(void *vreg,unsigned int dmxid,unsigned int filterid);
void gx3211_set_match(void *vreg,unsigned int filterid,unsigned int depth,
	struct dmx_filter_key *pkey ,unsigned int eq_flag);
void gx3211_set_sec_irqen(void *vreg,unsigned int filterid);
void gx3211_set_gate_irqen(void *vreg,unsigned int filterid);
void gx3211_enable_filter(void *vreg,unsigned int filterid);
void gx3211_disable_filter(void *vreg,unsigned int filterid);
int gx3211_avbuf_callback(unsigned int channel_id,unsigned int length, unsigned int underflow,void *arg);
void gx3211_set_dvr_mode(int modid, void *vreg, int mode);
void gx3211_set_dvr_source(int dmxid, int tswid, void *vreg, struct dvr_info *info);
void gx3211_set_tsw_enable(int dmxid, int tswid, void *vreg);
void gx3211_set_tsw_disable(int dmxid, int tswid, void *vreg);
void gx3211_tsw_module_enable(int dmxid, void *vreg);
void gx3211_tsw_module_disable(int dmxid, void *vreg);
void gx3211_set_tsw_buf(int dmxid, int tswid, void *vreg,
		unsigned int start_addr, unsigned int size, unsigned int gate);
void gx3211_clr_tsw_buf(int dmxid, int tswid, void *vreg);
void gx3211_reset_tsw_buf(int dmxid, int tswid, void *vreg);
void gx3211_set_tsr_buf(int dmxid,void *vreg,
		unsigned int start_addr, unsigned int size, unsigned int gate);
void gx3211_clr_tsr_buf(int dmxid,void *vreg);
void gx3211_reset_tsr_buf(int dmxid,void *vreg);
int gx3211_dvr2dvr_dealwith(struct dvr_info *info, unsigned int df_pread, unsigned int df_len);

int gx3211_df_al_isr(void *dev);
int gx3211_df_isr(void *dev);
void gx3211_av_gate_isr(void *dev);
void gx3211_av_section_isr(void *dev);
void gx3211_check_ts_clk_err_isr(void *dev);
void gx3211_tsw_isr(void *dev, int manual_mode);
void gx3211_tsw_full_isr(void *dev);
int  gx3211_tsw_isr_en(void *dev);
void gx3211_tsr_isr(void *dev);
void gx3211_tsr_dealwith(void *dev, int modid, int tsr_id);
void gx3211_set_tsr_avid(int dmxid, void *vreg, int avid);
void gx3211_clr_tsr_avid(int dmxid, void *vreg, int avid);
void gx3211_tsw_update_rptr(void *dev, int id, unsigned int pread);

int gx3211_dvr_open(struct gxav_module *module);
int gx3211_dvr_close(struct gxav_module *module);
int gx3211_dvr_set_config(struct gxav_module *module, GxDvrProperty_Config *config);
void gx3211_bind_slot_tsw(void *vreg,unsigned int slotid,unsigned int tswid);

void gx3211_log_d_source(void *vreg);

#endif
