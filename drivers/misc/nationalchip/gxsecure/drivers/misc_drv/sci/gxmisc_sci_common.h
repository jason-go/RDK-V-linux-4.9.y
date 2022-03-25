#ifndef __GXMISC_SCI_VIRDEV_H__
#define __GXMISC_SCI_VIRDEV_H__

#include "../gxmisc_virdev.h"
#include "sci_reg.h"

#define SCI_BLOCKLEN (64)
#define SCI_BUFFER_MAX_SIZE  (512)
#define SCI_EVENT_RW_MASK    (1)

#define SCI_READ_VALID       (0x1<<0)
#define SCI_READ_FINISH      (0x1<<1)

#define SCI_RX_TIMEOUT       (1000000)
#define SCI_DEFAULT_ETU      (372)

#define SCI_FIFOLEN_V1       (64)
#define SCI_FIFOLEN_V2       (512)

/* enums for interrupt DSR */
enum {
	SCI_INTSTATUS_INVALID          = 0,
	SCI_INTSTATUS_SCIIN            = (0x1<<0),
	SCI_INTSTATUS_CARD_NOACK       = (0x1<<1),
	SCI_INTSTATUS_REPEAT_ERROR     = (0x1<<2),
	SCI_INTSTATUS_RX_FIFO_OVERFLOW = (0x1<<3),
};

enum {
	SCI_STATUS_INIT = 0,
	SCI_STATUS_WRITE,
	SCI_STATUS_READ,
	SCI_STATUS_IDLE,
};

struct sci_priv {
	gx_mutex_t        mutex;
	gx_select_t       rd_queue;
	gx_event_t        wr_queue;
	gx_spin_lock_t    spin_lock;

	volatile uint8_t  card_status;
	volatile uint8_t  status;
	volatile uint8_t  write_end;
	volatile uint8_t  read_end;

	uint32_t sys_clk;
	uint32_t fifolen;
	uint32_t flags;
	uint32_t rptr;
	uint32_t rdata_num;
	uint32_t wptr;
	uint32_t wdata_num;

	uint8_t rx_buffer[SCI_BUFFER_MAX_SIZE];
};

typedef struct {
	int32_t (*init)            (void *vreg, void *priv);
	int32_t (*set_clk_fb)      (void *vreg, void *priv, uint32_t enable);
	int32_t (*set_rxfifo_gate) (void *vreg, void *priv, uint32_t gate);
	int32_t (*set_txfifo_gate) (void *vreg, void *priv, uint32_t gate);
} GxMiscSCIHWOps;

// ioctl ops
int32_t sci_init(GxSeModuleHwObj *obj);
int32_t sci_open(GxSeModuleHwObj *obj);
int32_t sci_write(GxSeModuleHwObj *obj, const uint8_t *kbuf, uint32_t klen);
int32_t sci_read(GxSeModuleHwObj *obj, uint8_t *kbuf, uint32_t klen);
int32_t sci_poll(GxSeModuleHwObj *obj, int32_t which, void **r_wait, void **w_wait);
int32_t sci_setup(GxSeModuleHwObj *obj, GxSeModuleResource *res);
int32_t sci_isr(GxSeModuleHwObj *obj);
int32_t sci_dsr(GxSeModuleHwObj *obj);

int32_t sci_set_param(GxSeModuleHwObj *obj, GxSciParam *param);
int32_t sci_get_param(GxSeModuleHwObj *obj, GxSciParam *param);
int32_t sci_ICC_reset(GxSeModuleHwObj *obj);
int32_t sci_ICC_poweroff(GxSeModuleHwObj *obj);
int32_t sci_get_status(GxSeModuleHwObj *obj, GxSciCardStatus *status);
int32_t sci_print_reg(GxSeModuleHwObj *obj);

#endif

