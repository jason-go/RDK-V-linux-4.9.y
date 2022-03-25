#ifndef __GXDEMUX_H__
#define __GXDEMUX_H__

#include "avcore.h"
#include "gxav.h"
#include "gxav_demux_propertytypes.h"
#include "gxav_gp_propertytypes.h"
#include "gxav_common.h"
#include "firewall.h"
#include "sdc_hal.h"
#include "fifo.h"
#include "log_printf.h"
#include "secure_hal.h"

//#define FPGA_TEST_STREAM_PRODUCER_EN

#define MAX_CA_NUM               16
#define MAX_CAS_NUM              32
#define MAX_CAS_PID_NUM          32
#define MAX_AVBUF_NUM            4

#define MAX_DEMUX_UNIT           2
#define MAX_DVR_UNIT             4
#define MAX_SLOT_NUM             64
#define MAX_FILTER_KEY_DEPTH     18
#define MAX_FILTER_NUM           64
#define MAX_PFILTER_UNIT         3
#define MAX_PFILTER_PID          64
#define MAX_TSW_SLOT             32
#define MAX_GSE_UNIT             1
#define MAX_GSE_SLOT             32
#define MAX_GP_UNIT              4
#define MAX_GP_CW                64
#define MAX_SECURE_TSW           2

#define MAX_DMX_NORMAL           4
#define MAX_DMX_TSW_SLOT         8
#define MAX_DMX_PID_FILTER       12
#define MAX_DMX_GSE              16

#define MAX_AVSLOT_FOR_MULTI_TRACK  16

#define FIFO_PSI_SIZE      0x10000     /* 64KB  */
#define FIFO_PES_SIZE      0x20000     /* 128KB */

//FIXME: Don't fix!!!!! mailto:zhuzhg@nationalchip.com
#define BUFFER_PSI_SIZE    0x4000      /* 16KB  */
#define BUFFER_PES_SIZE    0x10000     /* 64KB  */

#define BUFFER_GSE_STATUS_SIZE 0x1000
#define BUFFER_GSE_DATA_SIZE   0x4000
#define BUFFER_PID_BUF_SIZE    0x1000

#define DVR_INPUT_MEM_DMX       (DVR_INPUT_DVR3+1)

#define SET_MASK(value, bit)    (value |= (0x1ULL << bit))
#define CLR_MASK(value, bit)    (value &= ~(0x1ULL << bit))
#define SET_DEFAULT_VALUE(x, y, value) {x = (x) ? x : ((y == 0) ? value : y);}
int get_one(long long mask, unsigned int count, unsigned int min, unsigned int max);

struct dmx_ca;
struct dmx_filter;
struct dmx_slot;
struct dmx_demux;
struct dmxdev;

/*************************************** GSE *****************************************/
enum {
	GSE_MODE_IDLE,
	GSE_MODE_PDU,
	GSE_MODE_RECORD,
};

struct dmx_gse_slot {
	struct dmx_demux*        demux;
	int                      id;

	enum dmx_label_type      label_type;
	struct dmx_label_detail  label;
	enum dmx_protocol_type   protocol;

	void*                    vaddr_status;
	void*                    vaddr_data;
	unsigned int             paddr_status;
	unsigned int             paddr_data;

	unsigned int             pdu_pos;
	unsigned int             pdu_unuse_left;

	unsigned int             hw_buffer_size;
	unsigned int             sw_buffer_size;

	struct gxfifo            fifo;
	unsigned int             almost_full_gate;
	unsigned int             flags;
	unsigned int             refcount;
};

struct demux_gse_ops {
	void (*reg_init)        (void *vreg);
	void (*set_sync_gate)   (void *vreg);
	void (*set_input_source)(void *vreg, unsigned int source);
	void (*set_stream_mode) (void *vreg, unsigned int mode);
	int  (*query_tslock)    (void *vreg);

	void (*enable_slot)     (void *vreg, unsigned int slotid, int work_mode, int mode);
	void (*disable_slot)    (void *vreg, unsigned int slotid, int work_mode);
	void (*set_status_rptr) (void *vreg, unsigned int slotid, unsigned int rptr);
	void (*wait_finish)     (void *vreg, unsigned int slotid, volatile unsigned int *slot_finish);
	void (*clr_slot_cfg)    (void *vreg, unsigned int slotid);
	void (*all_pass_en)     (void *vreg, unsigned int slotid);
	void (*set_addr)        (void *vreg, struct dmx_gse_slot *slot);
	void (*set_label)       (void *vreg, struct dmx_gse_slot *slot);
	void (*set_protocol)    (void *vreg, struct dmx_gse_slot *slot);
	unsigned int (*get_status_rptr)(void *vreg, unsigned int slotid);
	unsigned int (*get_status_wptr)(void *vreg, unsigned int slotid);

	void (*record_finish_isr) (void *dev);
	void (*slot_close_isr)    (void *dev);
	void (*full_isr)          (void *dev);
	void (*al_isr)            (void *dev);
	void (*pdu_isr)           (void *dev);
	int  (*al_isr_en)         (void *dev);
	int  (*pdu_isr_en)        (void *dev);
};

int dvr_gse_open            (int sub);
int dvr_gse_close           (int sub);
int dmx_gse_set_config      (struct dmx_demux *dmx, GxDemuxProperty_ConfigDemux *pconfig);
int dmx_gse_get_lock        (struct dmx_demux *dmx, GxDemuxProperty_TSLockQuery *pquery);
int dmx_gse_slot_allocate   (struct dmx_demux *dmx, GxDemuxProperty_GseSlot *pslot);
int dmx_gse_slot_free       (struct dmx_demux *dmx, GxDemuxProperty_GseSlot *pslot);
int dmx_gse_slot_config     (struct dmx_demux *dmx, GxDemuxProperty_GseSlot *pslot);
int dmx_gse_slot_enable     (struct dmx_demux *dmx, GxDemuxProperty_GseSlot *pslot);
int dmx_gse_slot_disable    (struct dmx_demux *dmx, GxDemuxProperty_GseSlot *pslot);
int dmx_gse_slot_attribute  (struct dmx_demux *dmx, GxDemuxProperty_GseSlot *pslot);
int dmx_gse_slot_read       (struct dmx_demux *dmx, GxDemuxProperty_FilterRead *pread);
int dmx_gse_slot_fifo_query (struct dmx_demux *dmx, GxDemuxProperty_FilterFifoQuery *pquery);
int dmx_gse_set_property    (struct gxav_module *module, int property_id, void *property, int size);
int dmx_gse_get_property    (struct gxav_module *module, int property_id, void *property, int size);
int dvr_gse_read            (struct gxav_module *module, void *buf, unsigned int size);

/*************************************** PIDFILTER *****************************************/
enum dvr_pfilter_mode {
	DVR_PFILTER_MODE_PID,
	DVR_PFILTER_MODE_LOCK,
	DVR_PFILTER_MODE_ALL,
};

typedef struct  {
	unsigned int start_addr;
	unsigned int length;
	unsigned int next;
	unsigned int wptr;
} dvr_pfilter_node;

struct dvr_pfilter {
	int                 id;
	volatile void*      reg;
	unsigned int        base_addr;
	unsigned int        max_unit;
	unsigned int        max_pid_num;
	unsigned int        pid_buf;
	unsigned int        pid_buf_max_unit;
	unsigned int        pid_buf_mask;
	unsigned int        list_start_addr;
	unsigned int        list_node_num;
	dvr_pfilter_node list;

	unsigned int        mode;
	unsigned int        input_mode;
	unsigned int        nosync_en;
	unsigned int        novalid_en;
	enum dvr_input      source;
	enum dvr_output     dest;
	void*               ivaddr;
	unsigned int        ipaddr;
	unsigned int        sw_buffer_size;
	unsigned int        hw_buffer_size;
	unsigned int        almost_full_gate;

	unsigned int        cfg_done;
	unsigned int        time_err_flag;
	long long           pid_mask;
	struct gxfifo       fifo;
};

struct demux_pfilter_ops {
	int  (*open)            (struct dvr_pfilter *pfilter);
	int  (*close)           (struct dvr_pfilter *pfilter);
	void (*start)           (struct dvr_pfilter *pfilter);
	void (*stop)            (struct dvr_pfilter *pfilter);
	void (*set_ctrl)        (struct dvr_pfilter *pfilter);
	void (*set_gate)        (struct dvr_pfilter *pfilter);
	void (*add_pid)         (struct dvr_pfilter *pfilter, int pid_handle, int pid);
	void (*del_pid)         (struct dvr_pfilter *pfilter, int pid_handle);
	void (*pid_buf_add_pid) (struct dvr_pfilter *pfilter, int pid_handle, int pid);
	void (*pid_buf_del_pid) (struct dvr_pfilter *pfilter, int pid_handle);
	void (*pid_buf_init)    (struct dvr_pfilter *pfilter);
	void (*pid_buf_deinit)  (struct dvr_pfilter *pfilter);
	void (*tsw_isr)         (struct dvr_pfilter *pfilter);
	int  (*irq_entry)       (struct dvr_pfilter *pfilter);

	struct dvr_pfilter *(*get_pfilter)(int id);
};

int dvr_pfilter_init         (void);
int dvr_pfilter_deinit       (void);
int dvr_pfilter_open         (int id);
int dvr_pfilter_close        (int id);
int dvr_pfilter_get_tsr_id   (int id);
int dvr_pfilter_set_tsr_buf  (int dmxid, struct dvr_pfilter *pfilter);
int dvr_pfilter_clr_tsr_buf  (int dmxid);
int dvr_pfilter_irq_entry    (int max_pfilter_num);
int dvr_pfilter_set_property (struct gxav_module *module, int property_id, void *property, int size);
int dvr_pfilter_get_property (struct gxav_module *module, int property_id, void *property, int size);
struct gxfifo *dvr_pfilter_get_tsw_fifo(int id);

/*************************************** GP *****************************************/
enum gp_buffer_mode {
	GP_MODE_NORMAL,
	GP_MODE_LOOP,
};

enum gp_status {
	GP_STATUS_IDLE,
	GP_STATUS_ALMOST_FULL,
};

typedef union {
	unsigned int value;
	struct {
		unsigned index            : 6;
		unsigned iv_reset         : 1;
		unsigned mode             : 1;
		unsigned size_info_bit    : 6;
		unsigned av_channel       : 2;
		unsigned ctr_iv_start_bit : 7;
		unsigned                  : 1;
		unsigned ctr_iv_size      : 7;
		unsigned end_of_data      : 1;
	} bits;
} GPInfomation;

typedef struct _gp_cmd {
	GPInfomation    info;
	unsigned int    data_ptr;
	unsigned int    data_len;
	struct _gp_cmd *next;
} GPCmd;

struct dmxgp_chan {
	unsigned int id;
	unsigned int v_addr;
	unsigned int p_addr;
	unsigned int length;
	unsigned int afull_gate;
	unsigned int rptr;
	unsigned int cmd_no_align_len;

	gx_spin_lock_t  spin_lock;
	volatile unsigned int start;
	volatile unsigned int read_en;
};

struct dmxgp_cw {
	int             id;
	enum gp_ds_mode mode;
	unsigned int    cw_from_acpu;
	unsigned int    cw[4];
	unsigned int    iv[4];
	unsigned int    ctr_iv_size;
};

struct dmxgp {
	unsigned int      cmd_fifo;
	unsigned int      cmd_fifo_wptr;
	unsigned int      cmd_fifo_len;

	struct dmxgp_chan chan[MAX_GP_UNIT];
	struct dmxgp_cw  *configs[MAX_GP_CW];

	volatile unsigned int gp_done;
};

struct demux_gp_ops {
	void (*set_irq_en)    (void *vreg);
	void (*clr_irq_en)    (void *vreg);
	void (*set_cmd_fifo)  (void *vreg, unsigned char *buf, unsigned int size);
	void (*clr_cmd_fifo)  (void *vreg);
	void (*set_chan_addr) (void *vreg, struct dmxgp_chan *chan);
	void (*clr_chan_addr) (void *vreg, struct dmxgp_chan *chan);
	void (*set_cw)        (void *vreg, struct dmxgp_cw *gpcw);
	void (*update_cmd)    (void *vreg, unsigned int cmd_num);
	void (*update_rptr)   (void *vreg, unsigned int id, unsigned int rptr);
	unsigned int (*query_data_length)(void *vreg, unsigned int id);

	void (*almost_full_isr) (void *dev);
	void (*idle_isr)        (void *dev);
};

int dmx_gp_init         (struct gxav_device *device, struct gxav_module_inode *inode);
int dmx_gp_cleanup      (struct gxav_device *device, struct gxav_module_inode *inode);
int dmx_gp_open         (struct gxav_module *module);
int dmx_gp_close        (struct gxav_module *module);
int dmx_gp_set_property (struct gxav_module *module, int property_id, void *property, int size);
int dmx_gp_get_property (struct gxav_module *module, int property_id, void *property, int size);
struct gxav_module_inode* dmx_gp_irq(struct gxav_module_inode *inode, int irq);

/*************************************** Demux/DVR/Descramble *****************************************/

#define DMX_CAS_VERSION_2     (1<<31)
#define DMX_CAS_CAID_POS      (24)
#define DMX_CAS_CAID_MASK     (0xff)
#define DMX_CAS_PID_POS       (0)
#define DMX_CAS_PID_MASK      (0xffff)

struct dmx_sys_key {
	unsigned int sys_sel;
	unsigned int sys_key_h[4];
	unsigned int sys_key_l[4];
};

struct dmx_cas {
	struct dmx_demux*        demux;
	int                      id;
	int                      handle;

	enum cas_ds_mode         ds_mode;

	unsigned int             cw_even[4];
	unsigned int             cw_odd[4];
	unsigned int             iv_even[4];
	unsigned int             iv_odd[4];
	unsigned int             sys_sel_even;
	unsigned int             sys_sel_odd;
	unsigned int             _M;

	unsigned int             flags;
};

struct dmx_filter {
	struct dmx_slot*         slot;
	int                      id;
	int                      handle;
	enum dmx_slot_type       type;

	unsigned int             depth;
	struct dmx_filter_key    key[MAX_FILTER_KEY_DEPTH];

#define DMX_MAP_OVERFLOW      (1 << 11)
	unsigned int             flags;

	void*                    vaddr;
	unsigned int             paddr;
	unsigned int             size;
	unsigned int             gate;
	unsigned int             pread;
	struct gxfifo            fifo;

#define DMX_STATE_FREE              0
#define DMX_STATE_ALLOCATED         1
#define DMX_STATE_GO                2
#define DMX_STATE_STOP              3
#define DMX_STATE_DELAY_FREE        4
	unsigned int             status;
	struct timeval           tv;
};

#define DMX_CAS_INVALID_ID   (0xff)
struct dmx_slot {
	struct dmx_demux*        demux;
	int                      id;
	int                      handle;
	unsigned short           pid;
	unsigned short           casid;
	enum dmx_slot_type       type;

	unsigned int             filter_num;
	struct dmx_filter*       filters[MAX_FILTER_NUM];

	int                      avid;
	unsigned int             ts_out_pin;
	unsigned int             flags;
#define DMX_SLOT_IS_ENABLE   (1 << 31)
	unsigned int             refcount;
	unsigned int             enable_counter;
};

struct demux_fifo {
	unsigned char   channel_id;
	unsigned char   buffer_id;
	unsigned int    buffer_start_addr;
	unsigned int    buffer_end_addr;
	unsigned int    buffer_wptr;

	unsigned char   pts_channel_id;
	unsigned char   pts_buffer_id;
	unsigned int    pts_start_addr;
	unsigned int    pts_end_addr;

	int             direction;
	int             pin_id;
	void            *channel;
	unsigned int    dummy;
};

struct dvr_tsw_slot {
	int pid;
	int handle;
	int refcount;
};

struct dvr_info {
	int                  id;
	int                  tsr_share_mask;
	int                  tsr_share_major_id;
	int                  dvr_handle;
	volatile int         hw_full_count;

	enum dvr_input       source;
	enum dvr_output      dest;
	unsigned int         sw_buffer_size;
	unsigned int         hw_buffer_size;
	unsigned int         almost_full_gate;
	unsigned int         blocklen;
	void*                vaddr;
	unsigned int         paddr;
	unsigned int         flow_control;
	unsigned int         secure_tsw_id;
	unsigned int         link_channel_id;
	unsigned int         link_vaddr;
	unsigned int         link_paddr;
	unsigned int         link_size;
	unsigned char       *trans_temp_buf;
	struct gxfifo        fifo;

#define DVR_STATE_IDLE        0
#define DVR_STATE_STOP        1
#define DVR_STATE_READY       2
#define DVR_STATE_BUSY        3
#define DVR_STATE_SHARE_MAJOR 4
#define DVR_STATE_SHARE_SUB   5
	unsigned int         status;
	unsigned int         flags;
#define DVR_FLAG_PRIV_TSR_SAME_BUF  (1<<16)
#define DVR_FLAG_PRIV_TSR_DIFF_BUF  (1<<17)
#define DVR_FLAG_TSBUF_FROM_CMDLINE (1<<18)
#define DVR_FLAG_TSBUF_FROM_USER    (1<<21)
#define DVR_FLAG_DVR_TO_DSP         (1<<19)
#define DVR_FLAG_TSR_TO_DSP         (1<<20)

	pCallback            protocol_callback;
};

struct dmx_dvr {
	int                 id;
	enum dvr_mode       mode;
	struct dvr_info     tsr_info;
	struct dvr_info     tsr23_info;
	struct dvr_info     tsw_info[MAX_SLOT_NUM];
	struct dvr_tsw_slot tsw_slot[MAX_TSW_SLOT];

	unsigned int        flags;
#define DVR_T2MI_OUT_EN     (0x1<<30)
#define DVR_T2MI_IN_EN      (0x1<<31)
	unsigned int        refcount;
};

struct dmx_demux {
	struct dmxdev*           dev;
	int                      id;
	unsigned short           pcr_pid;

	int                      handles[GXAV_MAX_HANDLE];
	struct dmx_slot*         slots[MAX_SLOT_NUM];
	struct dmx_filter*       filters[MAX_FILTER_NUM];
	struct dmx_gse_slot*     gse_slots[MAX_GSE_SLOT];

	unsigned char            sync_lock_gate;
	unsigned char            sync_loss_gate;
	unsigned char            time_gate;
	unsigned char            byt_cnt_err_gate;

	enum dmx_stream_mode     stream_mode;    //parallel input or serial input
	enum dmx_ts_select       ts_select;
	enum dmx_input           source;

	enum sync_lock_flag      ts_lock;
	struct dmx_dvr           dvr;
	struct gxfifo            infifo;
	struct gxfifo            tsw_slot_fifo;
	unsigned int             refcount;
};

struct demux_ops{
	//pfilter
	void (*pf_disable_slot)    (void *vreg,unsigned int dmxid,unsigned int slotid);
	void (*pf_enable_slot)     (void *vreg,unsigned int dmxid,unsigned int slotid);
	void (*pf_set_pid)         (void *vreg,unsigned int slotid,unsigned short pid);
	void (*pf_set_ts_out)      (void *vreg,unsigned int slotid);
	void (*pf_clr_slot_cfg)    (void *vreg,unsigned int slotid);
	void (*pf_set_err_discard) (void *vreg,unsigned int slotid);
	void (*pf_set_dup_discard) (void *vreg,unsigned int slotid);
	void (*pf_set_t2mi_out)       (void *vreg, unsigned int slotid, int flag);

	//slot
	void (*disable_slot)       (void *vreg,unsigned int dmxid,unsigned int slotid);
	void (*enable_slot)        (void *vreg,unsigned int dmxid,unsigned int slotid);
	void (*reset_cc)           (void *vreg,unsigned int slotid);
	void (*set_repeat_mode)    (void *vreg,unsigned int slotid);
	void (*set_one_mode)       (void *vreg,unsigned int slotid);
	void (*set_interrupt_unit) (void *vreg,unsigned int slotid);
	void (*set_crc_disable)    (void *vreg,unsigned int slotid);
	void (*set_pts_to_sdram)   (void *vreg,unsigned int slotid);
	void (*set_pid)            (void *vreg,unsigned int slotid,unsigned short pid);
	void (*set_ts_out)         (void *vreg,unsigned int slotid, int flag);
	void (*set_av_out)         (void *vreg,unsigned int slotid, int flag);
	void (*set_des_out)        (void *vreg,unsigned int slotid, int flag);
	void (*set_err_discard)    (void *vreg,unsigned int slotid, int flag);
	void (*set_dup_discard)    (void *vreg,unsigned int slotid, int flag);
	void (*set_err_mask)       (void *vreg,unsigned int dmxid, unsigned int filterid);
	void (*set_pts_bypass)     (void *vreg,unsigned int slotid);
	void (*set_pts_insert)     (void *vreg,unsigned int slotid);
	void (*set_descid)         (void *vreg,unsigned int slotid,unsigned int caid, unsigned int flags);
	void (*set_cas_descid)     (void *vreg,unsigned int slotid,unsigned int caid);
	void (*set_avid)           (void *vreg,unsigned int slotid,int avid);
	void (*set_filterid)       (void *vreg,unsigned int slotid,unsigned int filterid);
	void (*clr_filterid)       (void *vreg,unsigned int slotid,unsigned int filterid);
	void (*set_even_valid)     (void *vreg,unsigned int slotid);
	void (*set_odd_valid)      (void *vreg,unsigned int slotid);
	void (*set_cas_key_valid)  (void *vreg,unsigned int slotid);
	void (*set_cas_dsc_type)   (void *vreg,unsigned int slotid,unsigned int dsc_type);
	void (*set_psi_type)       (void *vreg,unsigned int slotid);
	void (*set_pes_type)       (void *vreg,unsigned int slotid);
	void (*set_av_type)        (void *vreg,unsigned int slotid);
	void (*clr_slot_cfg)       (void *vreg,unsigned int slotid);
	void (*set_reverse_pid)    (void *vreg,unsigned int dmxid,unsigned short pid);

	//filter
	void (*enable_filter)  (void *vreg,unsigned int filterid);
	void (*disable_filter) (void *vreg,unsigned int filterid);
	void (*clr_dfrw)       (void *vreg,unsigned int filterid);
	void (*set_dfbuf)      (void *vreg,unsigned int filterid,unsigned int start,unsigned int size,unsigned int gate);
	void (*clr_int_df)     (void *vreg,unsigned int filterid);
	void (*clr_int_df_en)  (void *vreg,unsigned int filterid);
	void (*set_select)     (void *vreg,unsigned int dmxid,unsigned int filterid);
	void (*set_sec_irqen)  (void *vreg,unsigned int filterid);
	void (*set_gate_irqen) (void *vreg,unsigned int filterid);
	void (*set_match)      (void *vreg,unsigned int filterid,unsigned int depth,
			struct dmx_filter_key *pkey, unsigned int eq_flag);
	//desc
	void (*set_evenkey)     (void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int alg);
	void (*set_oddkey)      (void *vreg,unsigned int caid,unsigned int key_high,unsigned int key_low, unsigned int alg);
	void (*set_ca_mode)     (void *vreg, int dmxid, int slot_id, enum cas_ds_mode ds_mode);
	//cas
	int  (*set_cas_mode)    (void *vreg, struct dmx_cas *cas);
	void (*set_cas_oddkey)  (void *vreg, struct dmx_cas *cas);
	void (*set_cas_evenkey) (void *vreg, struct dmx_cas *cas);
	void (*set_cas_syskey)  (void *vreg, struct dmx_sys_key *sys);
	int  (*cas_enable)      (void *vreg, void *cas_reg);

	//sys
	void (*clr_ints)         (void *vreg);
	void (*hw_init)          (void *vreg);
	void (*set_sync_gate)    (void *vreg, unsigned char timing_gate, unsigned char byt_cnt_err_gate,
			unsigned char sync_loss_gate, unsigned char sync_lock_gate);
	void (*set_input_source) (void *vreg, unsigned int dmxid, unsigned int source);
	void (*set_pcr_sync)     (void *vreg, unsigned int dmxid, unsigned short pcr_pid);
	void (*set_apts_sync)    (void *vreg, int avid);
	int (*query_tslock)      (void *vreg, unsigned int dmxid, unsigned int source);

	//av
	void (*enable_av_int)  (void *vreg, int avid);
	void (*disable_av_int) (void *vreg, int avid);
	void (*link_avbuf)     (int dmxid, void *vreg, int avid,
			unsigned int start_addr, unsigned int size, unsigned int gate);
	void (*unlink_avbuf)   (int dmxid, void *vreg, int avid);
	void (*link_ptsbuf)    (void *vreg, int avid, unsigned int start_addr,
			unsigned int size, unsigned int gate);
	void (*unlink_ptsbuf)  (void *vreg, int avid);
	int  (*avbuf_cb)       (unsigned int channel_id,unsigned int length, unsigned int underflow,void *arg);

	//isr
	int  (*df_al_isr)      (void *dev);
	int  (*df_isr)         (void *dev);
	void (*av_gate_isr)    (void *dev);
	void (*av_section_isr) (void *dev);
	void (*ts_clk_err_isr) (void *dev);

	//t2mi
	void (*set_t2mi_out)   (void *vreg, unsigned int slotid, int flag);
	void (*t2mi_config)    (void *vreg, int t2mi_id, int pid);

	//log
	void (*log_d_source)   (void *vreg);
};

struct demux_dvr_ops {
	void (*set_tsr_avid)        (int dmxid, void *vreg, int avid);
	void (*clr_tsr_avid)        (int dmxid, void *vreg, int avid);
	void (*set_dvr_mode)        (int modid, void *vreg, int mode);
	void (*set_dvr_source)      (int modid, int tswid, void *vreg, struct dvr_info *info);
	void (*set_tsw_buf)         (int dmxid, int tswid, void *vreg,
			unsigned int start_addr, unsigned int size, unsigned int gate);
	void (*clr_tsw_buf)         (int dmxid, int tswid, void *vreg);
	void (*reset_tsw_buf)       (int dmxid, int tswid, void *vreg);
	void (*set_tsw_enable)      (int dmxid, int tswid, void *vreg);
	void (*set_tsw_disable)     (int dmxid, int tswid, void *vreg);
	void (*tsw_module_enable)   (int dmxid, void *vreg);
	void (*tsw_module_disable)  (int dmxid, void *vreg);
	void (*set_tsr_buf)         (int dmxid, void *vreg, unsigned int start_addr,
			unsigned int size, unsigned int gate);
	void (*clr_tsr_buf)         (int dmxid, void *vreg);
	void (*reset_tsr_buf)       (int dmxid, void *vreg);
	void (*set_tsw_slot_buf)    (int dmxid, void *vreg, unsigned int start_addr,
			unsigned int size, unsigned int gate);
	void (*clr_tsw_slot_buf)    (int dmxid, void *vreg);
	void (*reset_tsw_slot_buf)  (int dmxid, void *vreg);
	void (*bind_slot_tsw)       (void *vreg,unsigned int slotid,unsigned int tswid);
	void (*pf_bind_slot_tsw)    (void *vreg,unsigned int slotid,unsigned int tswid);

	void (*tsw_update_rptr)     (void *dev, int id, unsigned int pread);
	void (*tsw_isr)             (void *dev, int manual_mode);
	void (*tsw_full_isr)        (void *dev);
	int  (*tsw_isr_en)          (void *dev);
	void (*tsr_isr)             (void *dev);
	void (*tsr_dealwith)        (void *dev, int modid, int tsr_id);

	int (*get_tsr_id)           (struct gxav_module *module);
	int (*irq_entry)            (struct gxav_module_inode *inode, int irq);

	//t2mi
	void  (*t2mi_set_source) (int id, void *vreg);
	void  (*t2mi_clr_source) (int id, void *vreg);
	void  (*t2mi_set_dest)   (int id, void *vreg);
	void  (*t2mi_clr_dest)   (int id, void *vreg);
};

#ifdef  FPGA_TEST_STREAM_PRODUCER_EN
struct demux_fpga_ops {
	void (*set_stream_addr) (int stream_mode);
	void (*hot_rst)         (int continue_play, int record);
};
#endif

struct dmxdev {
	struct dmx_demux         demuxs[2];
	struct dmxgp             gp;
	struct dmx_cas*          cas_configs[MAX_CAS_NUM];
	unsigned int             pid_array[MAX_CAS_PID_NUM];
	unsigned int             pid_mask;
	unsigned int             irqs[4];
	unsigned int             id;
	struct demux_ops         *ops;
	struct demux_dvr_ops     *dvr_ops;
	struct demux_pfilter_ops *pfilter_ops;
	struct demux_gse_ops     *gse_ops;
	struct demux_gp_ops      *gp_ops;
	struct demux_fpga_ops    *fpga_ops;
	void                     *reg;
	void                     *cas_reg;
	void                     *t2mi_reg;
	void                     *gse_reg;
	void                     *gp_reg;
	void                     *inode;
	int                      dvr_handles[MAX_SLOT_NUM];
	struct demux_fifo        avfifo[MAX_AVBUF_NUM];
	unsigned int             secure_av_buf[MAX_AVBUF_NUM];
	unsigned char            tsr_share_mask[8];
	unsigned int             filter_nofree_en;

	struct dmx_filter*       map_filters;
	int                      map_filters_size;
	long long                map_filter_mask;
	unsigned int             max_filter;
	long long                filter_mask;
	unsigned int             max_slot;
	long long                slot_mask;
	unsigned int             max_tsw;
	long long                tsw_mask;
	unsigned int             max_av;
	unsigned char            avbuf_mask;
	unsigned char            avbuf_used;
	unsigned int             max_cas;
	unsigned int             cas_mask;
	unsigned int             max_t2mi;
	unsigned int             t2mi_mask;
	unsigned int             max_tsw_slot;
	unsigned int             tsw_slot_mask;
	unsigned int             max_gse;
	unsigned int             max_gse_slot;
	unsigned int             gse_slot_mask;
	unsigned int             gse_work_mode;
	unsigned int             max_gp;
	unsigned int             max_gp_cw;
	long long                gp_cw_mask;

	unsigned int             max_dmx_unit;
	unsigned int             max_dvr;
	unsigned int             dvr_mask;
	unsigned int             dmx_mask;
	unsigned int             max_pfilter;
	unsigned int             max_pfilter_pid;
	void*                    secure_tsw_buf[MAX_SECURE_TSW];
	unsigned int             secure_tsw_bufsize[MAX_SECURE_TSW];
	unsigned int             filter_min_psi_sw_bufsize;
	unsigned int             filter_min_psi_hw_bufsize;
	unsigned int             filter_min_pes_sw_bufsize;
	unsigned int             filter_min_pes_hw_bufsize;

	gx_thread_id   thread_id;
	gx_thread_info thread_info;
	gx_sem_id      swirq_sem;

	gx_thread_id   pf_thread_id;
	gx_thread_info pf_thread_info;
	gx_sem_id      pf_swirq_sem;

	gx_spin_lock_t spin_lock;
	gx_spin_lock_t df_spin_lock[MAX_FILTER_NUM];
	gx_spin_lock_t gse_spin_lock;
	volatile long long    thread_tsw;
	volatile unsigned int thread_pdu_finish;
	volatile unsigned int thread_slot_finish;
	volatile unsigned int thread_almost_full;
	volatile int          thread_stop;
	unsigned char *thread_stack;
	unsigned char *pf_thread_stack;
};

void gxdemux_lock(void);
void gxdemux_unlock(void);
void gxdemux_dvr_lock(void);
void gxdemux_dvr_unlock(void);
void gxdemux_tsw_lock(void);
void gxdemux_tsw_unlock(void);
struct dmx_demux *get_subdemux(int id);

int demux_set_pcrpid    (struct dmx_demux *dmx,GxDemuxProperty_Pcr *pcr);
int demux_set_config    (struct dmx_demux *dmx,unsigned int module_sub,GxDemuxProperty_ConfigDemux *pconfig);
int demux_get_lock      (struct dmx_demux *dmx,GxDemuxProperty_TSLockQuery *pquery);
int demux_link_fifo     (struct dmx_demux *dmx, struct demux_fifo *fifo);
int demux_unlink_fifo   (struct dmx_demux *dmx, struct demux_fifo *fifo);

int demux_slot_allocate (struct dmx_demux *dmx,GxDemuxProperty_Slot *pslot);
int demux_slot_free     (struct dmx_demux *dmx,GxDemuxProperty_Slot *pslot);
int demux_slot_enable   (struct dmx_demux *dmx,GxDemuxProperty_Slot *pslot);
int demux_slot_disable  (struct dmx_demux *dmx,GxDemuxProperty_Slot *pslot);
int demux_slot_config   (struct dmx_demux *dmx,GxDemuxProperty_Slot *pslot);
int demux_slot_query    (struct dmx_demux *dmx,GxDemuxProperty_SlotQueryByPid *pquery);
int demux_slot_attribute(struct dmx_demux *dmx,GxDemuxProperty_Slot *pslot);
int demux_tsw_slot_free(int sub, int id, int handle);

int map_filter_alloc        (struct dmxdev *dev, int filter_id, struct dmx_filter **filter);
int map_filter_free         (struct dmxdev *dev);

int demux_filter_allocate   (struct dmx_demux *dmx,GxDemuxProperty_Filter *pfilter);
int demux_filter_free       (struct dmx_demux *dmx,GxDemuxProperty_Filter *pfilter);
int demux_filter_config     (struct dmx_demux *dmx,GxDemuxProperty_Filter *pfilter);
int demux_filter_enable     (struct dmx_demux *dmx,GxDemuxProperty_Filter *pfilter);
int demux_filter_disable    (struct dmx_demux *dmx,GxDemuxProperty_Filter *pfilter);
int demux_filter_fifo_reset (struct dmx_demux *dmx,GxDemuxProperty_FilterFifoReset *preset);
int demux_filter_fifo_query (struct dmx_demux *dmx,GxDemuxProperty_FilterFifoQuery *pquery);
int demux_filter_read       (struct dmx_demux *dmx,GxDemuxProperty_FilterRead *pread);
int demux_filter_attribute  (struct dmx_demux *dmx,GxDemuxProperty_Filter *pfilter);

int demux_ca_config    (struct dmx_demux *dmx,GxDemuxProperty_CA *pca);
int demux_ca_attribute (struct dmx_demux *dmx,GxDemuxProperty_CA *pca);

int demux_cas_allocate  (struct dmx_demux *dmx);
int demux_cas_free      (struct dmx_demux *dmx, unsigned int caid);
int demux_cas_config    (struct dmx_demux *dmx,GxDemuxProperty_CAS *pcas);
int demux_cas_setcw     (struct dmx_demux *demux,GxDemuxProperty_CAS *pcas);
int demux_cas_set_syskey(struct dmx_demux *demux, GxDemuxProperty_SysKey *psys);

int demux_init         (struct gxav_device *device, struct gxav_module_inode *inode);
int demux_cleanup      (struct gxav_device *device, struct gxav_module_inode *inode);
int demux_open         (struct gxav_module *module);
int demux_close        (struct gxav_module *module);
int demux_set_property (struct gxav_module *module, int property_id, void *property, int size);
int demux_get_property (struct gxav_module *module, int property_id, void *property, int size);
int demux_set_entry    (struct gxav_module *module, int property_id, void *property, int size);
int demux_get_entry    (struct gxav_module *module, int property_id, void *property, int size);
struct gxav_module_inode* demux_irq(struct gxav_module_inode *inode, int irq);

int dvr_open         (struct gxav_module *module);
int dvr_close        (struct gxav_module *module);
int dvr_read         (struct gxav_module *module, void *buf, unsigned int size);
int dvr_write        (struct gxav_module *module, void *buf, unsigned int size);
int dvr_set_property (struct gxav_module *module, int property_id, void *property, int size);
int dvr_get_property (struct gxav_module *module, int property_id, void *property, int size);
int dvr_set_entry    (struct gxav_module *module, int property_id, void *property, int size);
int dvr_get_entry    (struct gxav_module *module, int property_id, void *property, int size);
struct gxav_module_inode* dvr_irq(struct gxav_module_inode *inode, int irq);

#ifdef  FPGA_TEST_STREAM_PRODUCER_EN
int dmx_fpga_init(void);
#endif

extern struct dmxdev gxdmxdev[MAX_DEMUX_UNIT];

#endif

