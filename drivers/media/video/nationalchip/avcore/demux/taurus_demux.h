#ifndef __TAURUS_DEMUX__
#define __TAURUS_DEMUX__

#include "gxdemux.h"

void taurus_hw_init(void *vreg);
void taurus_set_input_source(void *vreg, unsigned int dmxid,unsigned int source);
void taurus_pf_disable(void *vreg,unsigned int dmxid,unsigned int slotid);
void taurus_pf_enable(void *vreg,unsigned int dmxid,unsigned int slotid);
void taurus_pf_set_pid(void *vreg,unsigned int slotid,unsigned short pid);
void taurus_pf_clr_slot_cfg(void *vreg,unsigned int slotid);
void taurus_pf_set_ts_out(void *vreg,unsigned int slotid);
void taurus_pf_set_err_discard(void *vreg,unsigned int slotid);
void taurus_pf_set_dup_discard(void *vreg,unsigned int slotid);
void taurus_disable_slot(void *vreg,unsigned int dmxid,unsigned int slotid);
void taurus_enable_slot(void *vreg,unsigned int dmxid,unsigned int slotid);
void taurus_set_repeat_mode(void *vreg,unsigned int slotid);
void taurus_set_one_mode(void *vreg,unsigned int slotid);
void taurus_set_interrupt_unit(void *vreg,unsigned int slotid);
void taurus_set_err_discard(void *vreg,unsigned int slotid, int flag);
void taurus_set_dup_discard(void *vreg,unsigned int slotid, int flag);
void taurus_set_err_mask(void *vreg, unsigned int dmxid, unsigned int filterid);
void taurus_set_av_out(void *vreg,unsigned int slotid, int flag);
void taurus_set_ts_out(void *vreg,unsigned int slotid, int flag);
void taurus_set_des_out(void *vreg,unsigned int slotid, int flag);
void taurus_set_crc_disable(void *vreg,unsigned int slotid);
void taurus_set_psi_type(void *vreg,unsigned int slotid);
void taurus_set_pes_type(void *vreg,unsigned int slotid);
void taurus_set_av_type(void *vreg,unsigned int slotid);
void taurus_set_pts_bypass(void *vreg,unsigned int slotid);
void taurus_set_pts_insert(void *vreg,unsigned int slotid);
void taurus_set_pts_to_sdram(void *vreg,unsigned int slotid);
void taurus_clr_slot_cfg(void *vreg,unsigned int slotid);
void taurus_set_pid(void *vreg,unsigned int slotid,unsigned short pid);
void taurus_set_avid(void *vreg,unsigned int slotid, int avid);
void taurus_set_filterid(void *vreg, unsigned int slotid,unsigned int filterid);
void taurus_clr_filterid(void *vreg, unsigned int slotid,unsigned int filterid);
void taurus_set_key_valid(void *vreg, unsigned int slotid);
void taurus_set_dsc_type(void *vreg, unsigned int slotid, unsigned int dsc_type);
void taurus_set_cas_descid(void *vreg, unsigned int slotid, unsigned int casid);
void taurus_set_match(void *vreg,unsigned int filterid,unsigned int depth,
		struct dmx_filter_key *pkey ,unsigned int eq_flag);
int taurus_set_cas_mode(void *vreg, struct dmx_cas *cas);
void taurus_set_cas_syskey(void *vreg, struct dmx_sys_key *sys);

void taurus_tsr_isr(void *dev);
void taurus_tsr_dealwith(void *dev, int modid, int tsr_id);

int taurus_dvr_add_pid(struct gxav_module *module, int pid);
int taurus_dvr_del_pid(struct gxav_module *module, int pid_handle);
int taurus_dvr_pid_enable(struct gxav_module *module, int pid_handle);
int taurus_dvr_pid_disable(struct gxav_module *module, int pid_handle);
void taurus_set_dvr_source(int dmxid, int tswid, void *vreg, struct dvr_info *info);
void taurus_set_tsw_slot_buf(int dmxid, void *vreg,unsigned int start_addr, unsigned int size, unsigned int gate);
void taurus_clr_tsw_slot_buf(int dmxid, void *vreg);
void taurus_reset_tsw_slot_buf(int dmxid, void *vreg);

void taurus_bind_slot_tsw(void *vreg,unsigned int slotid,unsigned int tswid);
void taurus_pf_bind_slot_tsw(void *vreg,unsigned int slotid,unsigned int tswid);

int  taurus_enable_t2mi(void *vreg, int source);
int  taurus_disable_t2mi(void *vreg, int source);
void taurus_t2mi_config(void *vreg, int t2mi_id, int pid);

int taurus_gse_init(struct gxav_device *device, struct gxav_module_inode *inode);
int taurus_gse_cleanup(struct gxav_device *device, struct gxav_module_inode *inode);

#endif
