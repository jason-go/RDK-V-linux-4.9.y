#ifndef __GXAV_DEMUX_PROPERTYTYPES_H__
#define __GXAV_DEMUX_PROPERTYTYPES_H__

#ifndef TEE_OS
/********************************************************
 * 1. demux & dvr open module id :
 *       0~3   : demux
 *       4~7   : demux for tsw_slot
 *       8~11  : demux for pid_filter
 *       12~15 : demux for gse
 *
 * 2. define the pin_id 0~63 that the demux link fifo
 *       pin_id: 0~31     for link av/pts fifo
 *       pin_id: 32~37    for other fifo of the demux output
 *       pin_id: 38~43    for other fifo of the demux input
 *
 *******************************************************/
#define PIN_ID_SDRAM_OUTPUT0   32
#define PIN_ID_SDRAM_OUTPUT1   33
#define PIN_ID_SDRAM_OUTPUT2   34
#define PIN_ID_SDRAM_OUTPUT3   35
#define PIN_ID_SDRAM_OUTPUT4   36
#define PIN_ID_SDRAM_OUTPUT5   37
#define PIN_ID_SDRAM_OUTPUT6   38
#define PIN_ID_SDRAM_OUTPUT7   39
#define PIN_ID_SDRAM_OUTPUT8   40
#define PIN_ID_SDRAM_OUTPUT9   41
#define PIN_ID_SDRAM_OUTPUT10  42
#define PIN_ID_SDRAM_OUTPUT11  43
#define PIN_ID_SDRAM_OUTPUT12  44
#define PIN_ID_SDRAM_OUTPUT13  45
#define PIN_ID_SDRAM_OUTPUT14  46
#define PIN_ID_SDRAM_OUTPUT15  47

#define PIN_ID_SDRAM_OUTPUT  64
#define PIN_ID_SDRAM_INPUT  128

enum dmx_stream_mode {
	DEMUX_PARALLEL,
	DEMUX_SERIAL,
	DEMUX_SERIAL_2BIT,
	DEMUX_SERIAL_4BIT,
};

enum dmx_protocol_type {
	GSE_PROTOCOL_NOT_USED,
	GSE_PROTOCOL_IPV4,
	GSE_PROTOCOL_IPV6
};

enum dmx_label_type {
	GSE_LABEL_MAC,                /* 6 bytes label     */
	GSE_LABEL_ALTERNATIVE,        /* 3 bytes label     */
	GSE_LABEL_BROADCAST,          /* all kind of label */
	GSE_LABEL_REUSE,              /* last label        */
	GSE_LABEL_NOT_USED = 0xf,
};

struct dmx_label_detail {
	unsigned int h;
	unsigned int l;
};

enum dmx_ts_select {
	FRONTEND,
	OTHER
};

enum dmx_input {
	DEMUX_TS1,
	DEMUX_TS2,
	DEMUX_TS3,
	DEMUX_SDRAM,
};

enum sync_lock_flag {
	TS_SYNC_UNLOCKED,
	TS_SYNC_LOCKED
};

enum dmx_slot_type {
	DEMUX_SLOT_PSI,
	DEMUX_SLOT_PES,
	DEMUX_SLOT_PES_AUDIO,
	DEMUX_SLOT_PES_VIDEO,
	DEMUX_SLOT_TS,
	DEMUX_SLOT_MUXTS,
	DEMUX_SLOT_AUDIO,
	DEMUX_SLOT_SPDIF,
	DEMUX_SLOT_AUDIO_SPDIF,   /* 暂时不支持 */
	DEMUX_SLOT_VIDEO,
};

enum mtc_arith_type {
	GXMTCCA_DES_ARITH,
	GXMTCCA_3DES_ARITH,
	GXMTCCA_AES128_ARITH,
	GXMTCCA_AES192_ARITH,
	GXMTCCA_AES256_ARITH,
	GXMTCCA_MULTI2_ARITH
};

enum mtc_arith_mode {
	GXMTCCA_ECB_MODE,
	GXMTCCA_CBC_MODE,
	GXMTCCA_CFB_MODE,
	GXMTCCA_OFB_MODE,
	GXMTCCA_CTR_MODE
};

struct dmx_filter_key {
	unsigned char value;
	unsigned char mask;
};

typedef struct {
	unsigned char        sync_lock_gate;
	unsigned char        sync_loss_gate;
	unsigned char        time_gate;
	unsigned char        byt_cnt_err_gate;

	enum dmx_stream_mode stream_mode;
	enum dmx_ts_select   ts_select;
	enum dmx_input       source;

	unsigned int         flags;
#define DMX_NOSYNC_EN             (1 << 1)
#define DMX_NOVALID_EN            (1 << 2)
} GxDemuxProperty_ConfigDemux;

typedef struct {
	int                slot_id;
	enum dmx_slot_type type;                     // av channel or common channel
	unsigned short     pid;
	unsigned int       ts_out_pin;

	unsigned int       flags;
#define DMX_CRC_DISABLE           (1 << 0)
#define DMX_REPEAT_MODE           (1 << 1)                        // common channel
#define DMX_PTS_INSERT            (1 << 2)                        // av channel
#define DMX_PTS_TO_SDRAM          (1 << 3)                        // av channel
#define DMX_TSOUT_EN              (1 << 4)
#define DMX_AVOUT_EN              (1 << 5)
#define DMX_DES_EN                (1 << 6)
#define DMX_ERR_DISCARD_EN        (1 << 7)
#define DMX_DUP_DISCARD_EN        (1 << 10)
#define DMX_T2MI_OUT_EN           (1 << 11)
#define DMX_REVERSE_SLOT          (1 << 12)
} GxDemuxProperty_Slot;

typedef struct {
	int                   slot_id;
	int                   filter_id;
	unsigned int          sw_buffer_size;
	unsigned int          hw_buffer_size;
	unsigned int          almost_full_gate;

	struct dmx_filter_key key[18];
	int                   depth;

	unsigned int          flags;
#define DMX_CRC_IRQ               (1 << 0)
#define DMX_EQ                    (1 << 1)
#define DMX_SW_FILTER             (1 << 7)
#define DMX_ERR_MASK_EN           (1 << 8)
#define DMX_AUTO_RESET_EN         (1 << 9)
#define DMX_MAP                   (1 << 10)
} GxDemuxProperty_Filter;

typedef struct {
	unsigned int          filter_id;
	unsigned int          buffer_addr;
	unsigned int          buffer_size;
	unsigned int          control_addr;
	unsigned int          control_size;
} GxDemuxProperty_FilterMap;

typedef struct {
	int                     slot_id;

	enum dmx_label_type     label_type;
	struct dmx_label_detail label;
	enum dmx_protocol_type  protocol;

	unsigned int            sw_buffer_size;
	unsigned int            hw_buffer_size;
	unsigned int            almost_full_gate;

	unsigned int            flags;
#define DMX_GSE_ALL_PASS_EN (1 << 0)
} GxDemuxProperty_GseSlot;

typedef struct {
	int          filter_id;
	void*        buffer;
	unsigned int max_size;
	unsigned int read_size; // 该变量可以用作配置filter读取模式
#define FILTER_READ_MODE_NORMAL      (0x0)
#define FILTER_READ_MODE_ONE_SECTION (0xFFFF0001)

	int          bit_error;
} GxDemuxProperty_FilterRead;

typedef struct {
	unsigned int psi_sw_bufsize;
	unsigned int psi_hw_bufsize;
	unsigned int pes_sw_bufsize;
	unsigned int pes_hw_bufsize;
} GxDemuxProperty_FilterMinBufSize;

enum cas_ds_mode {
    DEMUX_DES_DVB_CSA2,
    DEMUX_DES_DVB_CSA2_CONFORMANCE,
    DEMUX_DES_DVB_CSA3,
    DEMUX_DES_AES128_ECB,
    DEMUX_DES_IDSA,
    DEMUX_DES_AES128_CBC,
    DEMUX_DES_DES_ECB,
    DEMUX_DES_TDES_ECB,
    DEMUX_DES_SM4_ECB,
    DEMUX_DES_SM4_CBC,
    DEMUX_DES_MULTI2,
};

typedef struct {
	int          ca_id;
	int          slot_id;

	enum cas_ds_mode ds_mode;
	unsigned int even_key_high;
	unsigned int even_key_low;
	unsigned int odd_key_high;
	unsigned int odd_key_low;
	unsigned int flags;
#define DMX_CA_KEY_EVEN           1
#define DMX_CA_KEY_ODD            2
#define DMX_CA_DES_MODE           128
#define DMX_CA_PES_LEVEL          256
} GxDemuxProperty_CA;

typedef struct {
	int          cas_id;
	int          slot_id;

	enum cas_ds_mode ds_mode;
	unsigned int pes_level_dsc;

	unsigned int cw_from_acpu;
	unsigned int set_cw_addr_even;
	unsigned int cw_even[4];
	unsigned int cw_odd[4];
	unsigned int iv_even[4];
	unsigned int iv_odd[4];
	unsigned int sys_sel_even;
	unsigned int sys_sel_odd;
	unsigned int _M;
} GxDemuxProperty_CAS;

typedef struct {
	int            cas_id;
	int            slot_id;
	unsigned short pid;

	enum cas_ds_mode ds_mode;
	unsigned int cw[4];
	unsigned int iv[4];
	unsigned int sys_sel;
	unsigned int _M;
	unsigned int flags;
#define DMX_CAS_CW_FROM_CPU    (1<<16)
#define DMX_CAS_CW_EVEN        (1<<17)
#define DMX_CAS_PES_LEVEL      (1<<18)
#define DMX_CAS_BIND_ALL_SLOT  (1<<19)
#define DMX_CAS_BIND_MODE_SAVE (1<<20)
} GxDemuxProperty_CAS_EX;

typedef struct {
	unsigned int sys_sel;
	unsigned int sys_key_h[4];
	unsigned int sys_key_l[4];
} GxDemuxProperty_SysKey;

typedef struct {
	unsigned short pcr_pid;              /* pcr recover */
} GxDemuxProperty_Pcr;

typedef struct {
	enum sync_lock_flag ts_lock;
} GxDemuxProperty_TSLockQuery;

typedef struct {
	int filter_id;
} GxDemuxProperty_FilterFifoReset;

typedef struct {
	long long state;
} GxDemuxProperty_FilterFifoQuery;

typedef struct {
	int enable;
} GxDemuxProperty_FilterNofree;

typedef struct {
	unsigned short pid;
	int slot_id;
} GxDemuxProperty_SlotQueryByPid;

typedef struct {
	unsigned int stc_value;
} GxDemuxProperty_ReadStc;

typedef struct {
	unsigned int pcr_value;
} GxDemuxProperty_ReadPcr;

typedef struct {
	int filter_id;
	unsigned int free_size;
} GxDemuxProperty_FilterFifoFreeSize;

typedef struct {
	int                 ca_id;
	int                 slot_id;

	unsigned char*      eck;
	unsigned int        eck_len;
	unsigned char*      ecw_even;
	unsigned int        ecw_even_len;
	unsigned char*      ecw_odd;
	unsigned int        ecw_odd_len;
	unsigned int        flags;

	enum mtc_arith_type arith_type;
	enum mtc_arith_mode arith_mode;

#define MTC_KEY_EVEN              1
#define MTC_KEY_ODD               2
} GxDemuxProperty_MTCCA;

#endif

#endif
