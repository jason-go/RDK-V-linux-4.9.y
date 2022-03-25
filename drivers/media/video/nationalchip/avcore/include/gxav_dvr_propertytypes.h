#ifndef __GXAV_DVR_PROPERTYTYPES_H__
#define __GXAV_DVR_PROPERTYTYPES_H__

/*
 * 1. TSR
 * +------+     +------+     +------+
 * |      |     |      |     |      |
 * | MEM  | ==> | DVR0 | ==> | DMX0 |
 * |      |     |      |     |      |
 * +------+     +------+     +------+
 *
 * 2. TSW
 * +------+     +------+     +------+
 * |      |     |      |     |      |
 * | DMX0 | ==> | DVR0 | ==> | MEM  |
 * |      |     |      |     |      |
 * +------+     +------+     +------+
 *
 * 3. DVR TO DVR
 * +------+     +------+     +------+     +------+     +------+
 * |      |     |      |     |      |     |      |     |      |
 * | MEM  | ==> | DVR0 | ==> | DMX0 | ==> | DVR1 | ==> | DMX1 |
 * |      |     |      |     |      |     |      |     |      |
 * +------+     +------+     +------+     +------+     +------+
 *
 * 4. DVR TO DVR TO DVR
 * +------+     +------+     +------+     +------+     +------+
 * |      |     |      |     |      |     |      |     |      |
 * | MEM  | ==> | DVR0 | ==> | DMX0 | ==> | DVR1 | ==> | DMX1 | ...
 * |      |     |      |     |      |     |      |     |      |
 * +------+     +------+     +------+     +------+     +------+
 *
 * 5. T2MI
 * +------+     +------+     +------+     +------+     +------+
 * |      |     |      |     |      |     |      |     |      |
 * | DMX0 | ==> | DVR0 | ==> | T2MI | ==> | DVR1 | ==> | DMX1 |
 * |      |     |      |     |      |     |      |     |      |
 * +------+     +------+     +------+     +------+     +------+
 *
 */

enum dvr_mode {
	DVR_RECORD_LOCK,
	DVR_RECORD_ALL,
	DVR_RECORD_BY_REVERSE_SLOT,
};

enum dvr_input {
	DVR_INPUT_TSPORT,
	DVR_INPUT_MEM,
	DVR_INPUT_DMX,

	DVR_INPUT_T2MI,
	DVR_INPUT_DVR0,
	DVR_INPUT_DVR1,
	DVR_INPUT_DVR2,
	DVR_INPUT_DVR3,
};

enum dvr_output {
	DVR_OUTPUT_DVR0,
	DVR_OUTPUT_DVR1,
	DVR_OUTPUT_DVR2,
	DVR_OUTPUT_DVR3,
	DVR_OUTPUT_MEM,
	DVR_OUTPUT_DSP,
	DVR_OUTPUT_DMX,
	DVR_OUTPUT_T2MI,
};

enum dvr_mod {
	DVR_MOD_TSR = 1,
	DVR_MOD_TSW,
	DVR_MOD_ALL,
};

enum dvr_alg {
	DVR_ALG_AES128,
	DVR_ALG_AES192,
	DVR_ALG_AES256,
	DVR_ALG_DES,
	DVR_ALG_TDES,
};

struct dvr_buffer {
	unsigned int sw_buffer_size;
	unsigned int hw_buffer_size;
	unsigned int almost_full_gate;
	unsigned int buffer_len;
	unsigned int buffer_freelen;
	unsigned int sw_buffer_ptr;
	unsigned int hw_buffer_ptr;
};

struct dvr_phy_buffer {
	unsigned int dvr_handle;

	void *vaddr;
	unsigned int size;
	unsigned int offset;
	unsigned int data_len;
};

typedef int (*pCallback)(struct dvr_phy_buffer *src, struct dvr_phy_buffer *dst);
extern int gxav_dvr_set_protocol_callback(int dvrid, int handle, pCallback func);

typedef struct {
	enum dvr_alg     alg;
	unsigned int key_sub;

	unsigned int flags;
#define DVR_ENCRYPT_ENABLE   (0x1<<0)
} GxDvrProperty_CryptConfig;

typedef struct {
	enum dvr_mode     mode;
	enum dvr_input    src;
	struct dvr_buffer src_buf;

	enum dvr_output   dst;
	struct dvr_buffer dst_buf;

	unsigned int blocklen;
	unsigned int flags;
#define DVR_FLAG_PTR_MODE_EN   (1<<0)
} GxDvrProperty_Config;

typedef struct {
	enum dvr_mod mod;
} GxDvrProperty_Reset;

typedef struct {
	unsigned char tsr_share_mask;
} GxDvrProperty_TSRShare;

typedef struct {
	unsigned int paddr;
	unsigned int size;
	unsigned int blocklen;
} GxDvrProperty_PtrInfo;

typedef struct {
	unsigned int flags;
#define DVR_FLOW_CONTROL_ES  (0x1<<0)
} GxDvrProperty_TSRFlowControl;

#endif
