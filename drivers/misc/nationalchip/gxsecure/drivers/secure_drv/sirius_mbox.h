#ifndef __SIRIUS_MBOX_H__
#define __SIRIUS_MBOX_H__

#include "gxsecure.h"
#include "gxse_hwobj_secure.h"
#include "fifo.h"

#define SIRIUS_MBOX_BLOCKLEN (128)
#define SIRIUS_MBOX_SIGLEN   (256)

#define MBOX_PRINT_START (0x8815608a)
#define MBOX_PRINT_END   (0x8815608b)

struct mbox_priv {
	GxSeFifo fifo;

	volatile GxSecureStatus status;
	volatile uint32_t rom_start;
	volatile uint32_t write_en;
	volatile uint32_t read_en;
	volatile uint32_t ack_en;
	volatile uint32_t cmd_over;
	volatile uint32_t isr_rw_flag;

	uint32_t fw_load_addr;
	uint32_t fw_load_size;

	gx_event_t        rd_queue;
	gx_event_t        wr_queue;
	gx_spin_lock_t    spin_lock;
	gx_mutex_t        mutex;
};

enum {
	FW_ST_IDLE,
	FW_ST_WRITE,
	FW_ST_READ,
};

enum {
	FW_SYNC_NONE,      // Asynchronous command.
	FW_SYNC_ACK,       // Synchronization command until the other side receive the packet.
	FW_SYNC_CMD_OVER,  // Synchronization command until it is completed.
	FW_SYNC_ACK_TIMEOUT,
};

enum {
	FW_POLL_NONE,
	FW_POLL_R,
	FW_POLL_W,
};

#define FW_IFCP_START  (FW_PASSWD+1)   // Indicates that IFCP is running

extern struct mbox_priv sirius_mbox_priv;

int32_t sirius_mbox_init(GxSeModuleHwObj *obj);
int32_t sirius_mbox_isr(GxSeModuleHwObj *obj);
int32_t sirius_mbox_dsr(GxSeModuleHwObj *obj);

int32_t sirius_mbox_check_param(void *vreg, void *priv, void *param, uint32_t cmd);
int32_t sirius_mbox_get_status(void *vreg, void *priv);
int32_t sirius_mbox_get_blocklen(void *vreg, void *priv);
int32_t sirius_mbox_get_siglen(void *vreg, void *priv);
int32_t sirius_mbox_firmware_is_alive(void *vreg, void *priv);
int32_t sirius_mbox_tx(void *vreg, void *priv, uint32_t oob, uint8_t *buf, uint32_t size);
int32_t sirius_mbox_rx(void *vreg, void *priv, uint32_t *oob, uint8_t *buf, uint32_t size);
int32_t sirius_mbox_trx(void *vreg, void *priv, uint32_t oob, uint8_t *in, uint32_t inlen,uint8_t *out,uint32_t outlen);
int32_t sirius_mbox_set_BIV(void *vreg, void *priv, uint8_t *buf, uint32_t size);
int32_t sirius_mbox_get_BIV(void *vreg, void *priv, uint8_t *buf, uint32_t size);

#endif
