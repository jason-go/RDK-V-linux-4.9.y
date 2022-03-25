#ifndef __SIRIUS_SCPU_MBOX_H__
#define __SIRIUS_SCPU_MBOX_H__

#include "sirius_mbox.h"
#include "sirius_mbox_reg.h"

struct scpu_mbox_priv {
	GxSeFifo fifo;

	volatile uint32_t write_en;
	volatile uint32_t read_en;
	volatile uint32_t ack_en;
	volatile SiriusMboxReg *reg;
};

int32_t sirius_scpu_mbox_tx(void *vreg, void *priv, uint32_t oob, uint8_t *buf, uint32_t size);
int32_t sirius_scpu_mbox_rx(void *vreg, void *priv, uint32_t *oob, uint8_t *buf, uint32_t size);
int32_t sirius_scpu_mbox_trx(void *vreg, void *priv, uint32_t oob,
		uint8_t *in, uint32_t inlen, uint8_t *out, uint32_t outlen);

#endif
