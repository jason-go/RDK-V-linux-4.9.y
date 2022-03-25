#ifndef __SIRIUS_DEMUX__
#define __SIRIUS_DEMUX__

#include "gxdemux.h"

int  sirius_set_cas_mode    (void *vreg, struct dmx_cas *cas);
void sirius_set_cas_oddkey  (void *vreg, struct dmx_cas *cas);
void sirius_set_cas_evenkey (void *vreg, struct dmx_cas *cas);
void sirius_set_key_valid   (void *vreg, unsigned int slotid);
void sirius_set_cas_descid  (void *vreg, unsigned int slotid, unsigned int casid);
void sirius_set_dsc_type    (void *vreg, unsigned int slotid, unsigned int dsc_type);
void sirius_set_ca_mode     (void *vreg, int dmxid, int slot_id, enum cas_ds_mode ds_mode);

void sirius_set_tsr_buf     (int dmxid, void *vreg, unsigned int start_addr, unsigned int size, unsigned int gate);
void sirius_clr_tsr_buf     (int dmxid, void *vreg);
void sirius_reset_tsr_buf   (int dmxid, void *vreg);
void sirius_set_tsr_avid    (int dmxid, void *vreg, int avid);
void sirius_clr_tsr_avid    (int dmxid, void *vreg, int avid);

void sirius_tsr_isr         (void *dev);
void sirius_tsr_dealwith    (void *dev, int modid, int tsr_id);

void sirius_gse_reg_init          (void *vreg);
void sirius_gse_set_sync_gate     (void *vreg);
void sirius_gse_set_input_source  (void *vreg, unsigned int source);
void sirius_gse_set_stream_mode   (void *vreg, unsigned int mode);
void sirius_gse_set_record_mode   (void *vreg, unsigned int mode);
int sirius_gse_query_tslock       (void *vreg);
void sirius_gse_clr_slot_cfg      (void *vreg, unsigned int slotid);
void sirius_gse_enable_slot       (void *vreg, unsigned int slotid, int work_mode, int mode);
void sirius_gse_disable_slot      (void *vreg, unsigned int slotid, int work_mode);
void sirius_gse_wait_finish       (void *vreg, unsigned int slotid, volatile unsigned int *slot_finish);
void sirius_gse_set_status_rptr   (void *vreg, unsigned int slotid, unsigned int rptr);
unsigned int sirius_gse_get_status_rptr(void *vreg, unsigned int slotid);
unsigned int sirius_gse_get_status_wptr(void *vreg, unsigned int slotid);
void sirius_gse_set_addr          (void *vreg, struct dmx_gse_slot *slot);
void sirius_gse_all_pass_en       (void *vreg, unsigned int slotid);
void sirius_gse_set_label         (void *vreg, struct dmx_gse_slot *slot);
void sirius_gse_set_protocol      (void *vreg, struct dmx_gse_slot *slot);
void sirius_gse_full_isr          (void *dev);
void sirius_gse_slot_finish_isr   (void *dev);
void sirius_gse_record_finish_isr (void *dev);
void sirius_gse_almost_full_isr   (void *dev);
void sirius_gse_pdu_isr           (void *dev);
int  sirius_gse_pdu_isr_en        (void *dev);
int  sirius_gse_al_isr_en         (void *dev);
int  sirius_gse_init    (struct gxav_device *device, struct gxav_module_inode *inode);
int  sirius_gse_cleanup (struct gxav_device *device, struct gxav_module_inode *inode);

void sirius_gp_almost_full_isr    (void *dev);
void sirius_gp_idle_isr           (void *dev);

int sirius_dvr_open               (struct gxav_module *module);
int sirius_dvr_close              (struct gxav_module *module);
int sirius_dvr_get_tsr_id         (struct gxav_module *module);
int sirius_dvr_add_pid            (struct gxav_module *module, int pid);
int sirius_dvr_del_pid            (struct gxav_module *module, int pid_handle);
int sirius_dvr_irq_entry          (struct gxav_module_inode *inode, int irq);
void sirius_set_dvr_source        (int dmxid, int tswid, void *vreg, struct dvr_info *info);
struct gxfifo *sirius_dvr_get_tsw_fifo(int dmxid);

void sirius_fpga_stream_producer_init(void);
void sirius_fpga_set_stream_addr(int input_mode);
void sirius_fpga_hot_rst(int continue_play, int record);

#endif
