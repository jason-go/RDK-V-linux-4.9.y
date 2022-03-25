#ifndef __GX6605_DEMUX_H__
#define __GX6605_DEMUX_H__

#include "gxav.h"
#include "gxav_demux_propertytypes.h"
#include "gxav_common.h"
#include "fifo.h"

#define DMX_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[DEMUX][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)

/* DEMUX DEBUG MACRO */
#ifdef GX_DMX_DEBUG
#define DMX_DBG(fmt, args...) \
	do { \
		gx_printf("\n[DEMUX][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)
#else
#define DMX_DBG(fmt, args...)   ((void)0)
#endif

#define DEMUX_SYS_DTO		(100000000)
#define DEMUX_OUT_DTO		(6000000)
#define DEMUX_STC_DTO		(108000000)

#define DMX_INTERRUPT_NUMBER     33

#define MAX_FILTER_KEY_DEPTH     18
#define MAX_FILTER_NUM           32
#define MAX_SLOT_NUM             32
#define MAX_CA_NUM               8
#define MAX_AVBUF_NUM            4
#define MAX_TSW_FIFO_NUM         32
#define MAX_DEMUX_CHANNEL        3

#define DMX_TOP_FIELD            (0xFF)
#define DMX_BOTTOM_FIELD         (0)

#define SET_VAL_BIT(value, bit)      (value |= (0x1ULL << bit))
#define CLEAR_VAL_BIT(value, bit)    (value &= ~(0x1ULL << bit))

#define GET_CW_HIGH(cw_arr)	\
	((cw_arr[0] << 24) |	\
	 (cw_arr[1] << 16) |	\
	 (cw_arr[2] <<  8) |	\
	 (cw_arr[3] <<  0))

#define GET_CW_LOW(cw_arr)	\
	((cw_arr[4] << 24) |	\
	 (cw_arr[5] << 16) |	\
	 (cw_arr[6] <<  8) |	\
	 (cw_arr[7] <<  0))

struct dmx_ca;
struct dmx_filter;
struct dmx_slot;
struct dmx_demux;
struct dmxdev;

struct dmx_ca {
	struct dmxdev*           demux_device;
	struct dmx_demux*        demux;
	int                      id;

	unsigned long            even_key_high;
	unsigned long            even_key_low;
	unsigned long            odd_key_high;
	unsigned long            odd_key_low;

	unsigned int             flags;
};

enum mtc_action_type
{
	GXMTCCA_ENCRYPT,
	GXMTCCA_DECRYPT
};

#define FIFO_PSI_SIZE      0x10000     /* 64KB  */
#define FIFO_PES_SIZE      0x20000     /* 128KB */

//FIXME: Don't fix!!!!! mailto:zhuzhg@nationalchip.com
#define BUFFER_PSI_SIZE    0x4000      /* 16KB  */
#define BUFFER_PES_SIZE    0x10000     /* 64KB  */
#define BUFFER_TS_SIZE     ((188/4)*1024*5)    /* 188KB, 188B 1k align don't 4k align  */
#define BUFFER_TS_GATE     188*200     /* */

typedef int (*filter_data_cb)(const unsigned char *buffer1, unsigned int buffer1_len,
		const unsigned char *buffer2, unsigned int buffer2_len,void *priv);
typedef int (*filter_event_cb)(unsigned int event,int ops,void *priv);

struct dmx_filter {
	struct dmx_slot*         slot;
	char                     id;
	char                     depth;

	struct dmx_filter_key    key[MAX_FILTER_KEY_DEPTH];

	void*                    virtual_dfbuf_addr;
	unsigned int             phy_dfbuf_addr;
	unsigned int             buffer_size;
	unsigned int             buffer_out;
	unsigned int             pread_rd;

	unsigned int             flags;

	struct gxfifo            section_fifo;

	filter_data_cb           data_cb;
	void                     *filter_priv;
	filter_event_cb          event_cb;
	void                     *event_priv;
#define DMX_STATE_FREE              0
#define DMX_STATE_ALLOCATED         1
#define DMX_STATE_SET               2
#define DMX_STATE_READY             3
#define DMX_STATE_GO                4
#define DMX_STATE_STOP              5
#define DMX_STATE_DELAY_FREE        6
	unsigned int             status;
	unsigned int             type;
	unsigned int             refcount;
	struct timeval           tv;
};

struct dmx_slot {
	struct dmx_demux*        demux;
	char                     id;
	char                     filter_num;
	unsigned short           pid;

	enum dmx_slot_type       type;
	enum gxav_direction      dir;

	struct dmx_filter*       filters[MAX_FILTER_NUM];
	struct dmx_filter*       muxfilters;
	struct dmx_ca*           ca;

	int                      avbuf_id;
	unsigned int             flags;
	unsigned int             sw_buffer_size;
	unsigned int             hw_buffer_size;
	unsigned int             almost_full_gate;
	unsigned int             tsw;
	unsigned int             parent;
	unsigned int             refcount;
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

struct dmx_demux {
	struct dmxdev*           demux_device;
	char                     id;
	char                     slot_num;
	char                     muxslot_num;
	unsigned short           pcr_pid;

	struct dmx_slot*         slots[MAX_SLOT_NUM];
	struct dmx_slot*         muxslots[MAX_SLOT_NUM];
	struct dmx_ca*           ca[MAX_CA_NUM];

	enum dmx_ts_select       ts_select;
	enum dmx_input           source;

	unsigned char            sync_lock_gate;
	unsigned char            sync_loss_gate;
	unsigned char            time_gate;
	unsigned char            byt_cnt_err_gate;
	enum dmx_stream_mode     stream_mode;    //parallel input or serial input
	//unsigned short           stc_count;
	enum sync_lock_flag      ts_lock;
};

struct dmxdev {
	struct dmx_demux         demuxs[MAX_DEMUX_CHANNEL];
	struct dmx_slot*         slots[MAX_SLOT_NUM];
	struct dmx_slot*         pidslots[MAX_SLOT_NUM];
	struct dmx_slot*         muxslots[MAX_SLOT_NUM];
	struct dmx_filter*       filters[MAX_FILTER_NUM];
	struct dmx_filter*       muxfilters[MAX_FILTER_NUM];
	struct dmx_ca*           ca[MAX_CA_NUM];
	struct dmx_slot*         avslots[MAX_AVBUF_NUM];
	struct demux_fifo        avfifo[MAX_AVBUF_NUM];
	struct demux_fifo        fifo[MAX_TSW_FIFO_NUM];
	struct demux_fifo        rfifo;

	long long                filter_mask;
	long long                mux_filter_mask;
	long long                slot_mask;
	long long                pid_slot_mask;
	long long                mux_slot_mask;
	unsigned int             ca_mask;
	unsigned char            avbuf_mask;
	void *pes_buffer_cache;
	void *psi_buffer_cache;
	void *ts_buffer_cache;

	gx_thread_id thread_id;
	gx_thread_info thread_info;
	gx_sem_id sem_id;
	gx_spin_lock_t demux_spin_lock;
};

extern struct dmxdev gx6605_dmx;

void         gx6605_dmx_initialization(struct dmxdev *demux_device);
void         gx6605_dmx_cleanup       (struct dmxdev *demux_device);
void         gx6605_dmx_config        (struct dmx_demux *dmx);
int          gx6605_dmx_link_fifo     (struct dmx_demux *dmx, struct demux_fifo *fifo);
int          gx6605_dmx_unlink_fifo   (struct dmx_demux *dmx, struct demux_fifo *fifo);

int          gx6605_dmx_alloc_slot    (struct dmxdev *demux_device, int demux_id, enum dmx_slot_type type,unsigned short pid);
void         gx6605_dmx_slot_config   (struct dmx_slot *slot);
void         gx6605_dmx_enable_slot   (struct dmx_slot *slot);
void         gx6605_dmx_disable_slot  (struct dmx_slot *slot);
void         gx6605_dmx_free_slot     (struct dmx_slot *slot);

int          gx6605_dmx_alloc_filter  (struct dmx_slot *slot);
void         gx6605_dmx_filter_config (struct dmx_filter *filter);
void         gx6605_dmx_enable_filter (struct dmx_filter *filter);
void         gx6605_dmx_disable_filter(struct dmx_filter *filter);
void         gx6605_dmx_free_filter   (struct dmx_filter *filter);
unsigned int gx6605_dmx_query_fifo    (long long *state);
unsigned int gx6605_dmx_filter_read   (struct dmx_filter *filter, unsigned char *data_buf, unsigned int size);

int          gx6605_dmx_alloc_ca      (struct dmxdev *demux_device,int demux_id);
void         gx6605_dmx_ca_config     (struct dmx_slot *slot, struct dmx_ca *ca);
void         gx6605_dmx_free_ca       (struct dmx_ca *ca);
void         gx6605_dmx_mtc_decrypt   (gx_mtc_info *mtc_info_cfg,struct dmx_ca *ca);

void         gx6605_dmx_query_tslock  (struct dmx_demux *dmx);

void         gx6605_dmx_pcr_sync      (struct dmxdev *demux_device, int demux_id, unsigned short pcr_pid);
void         gx6605_dmx_read_stc      (unsigned int *value);
int          gx6605_dmx_read_apts     (void);
int          gx6605_dmx_read_pcr      (void);
void         gx6605_dmx_write_apts    (unsigned int value, int immd);
void         gx6605_dmx_write_vpts    (unsigned int value, int immd);

void         gx6605_dmx_stc_set(unsigned int value, int immd);

void         gx6605_dmx_interrupt     (filter_event_cb event_cb,void *priv);
void         gx6605_demux_read_thread (void* p);

int	     gx6605_dmx_query_dmx_slot_fifo(unsigned int buf_id, int slot_type, unsigned int *dmx_id, struct dmx_slot *slot, struct demux_fifo *fifo);
int          gx6605_demux_link_fifo    (int sub, struct demux_fifo *fifo);
int          gx6605_demux_unlink_fifo  (int sub, struct demux_fifo *fifo);

#endif

