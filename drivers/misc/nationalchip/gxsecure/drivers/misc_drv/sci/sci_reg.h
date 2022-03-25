#ifndef __GX_SCI_REG_H__
#define __GX_SCI_REG_H__

#define SCI_ISR_BIT_SCIIN         (1<<0)
#define SCI_ISR_BIT_SCIOUT        (1<<1)
#define SCI_ISR_BIT_RXFIFO_STATUS (1<<2)
#define SCI_ISR_BIT_TXFIFO_STATUS (1<<3)
#define SCI_ISR_BIT_TXFIFO_EMPTY  (1<<4)
#define SCI_ISR_BIT_RXFIFO_OVERF  (1<<5)
#define SCI_ISR_BIT_REPEAT_ERROR  (1<<6)
#define SCI_ISR_BIT_RESET_FINISH  (1<<7)
#define SCI_ISR_BIT_NACK          (1<<8)
#define SCI_ISR_BIT_RECV_OVER     (1<<9)
#define SCI_ISR_BIT_CARD_IN       (1<<10)
#define SCI_ISR_BIT_CARD_IN_DIFF  (1<<11)

typedef union {
	unsigned int value;
	struct {
		unsigned cldrst   : 1;
		unsigned hotrst   : 1;
		unsigned TE       : 1;
		unsigned RE       : 1;
		unsigned deact    : 1;
		unsigned fiforst  : 1;
		unsigned scirst   : 1;
		unsigned rcv      : 1;
	} bits;
} rSCI_CNTL1;

typedef union {
	unsigned int value;
	struct {
		unsigned detect_pol   : 1;
		unsigned clk_en       : 1;
		unsigned clk_pol      : 1;
		unsigned vcc_en       : 1;
		unsigned repeat_dis   : 1;
		unsigned io_conv      : 1;
		unsigned data_pol     : 1;
		unsigned parity       : 1;
		unsigned no_parity    : 1;
		unsigned auto_receive : 1;
		unsigned sci_en       : 1;
		unsigned stoplen      : 2;
		unsigned vcc_pol      : 1;
		unsigned txfifo_level : 6;   // cygnus update
		unsigned rxfifo_level : 6;   // cygnus update
		unsigned vcc_ctrl     : 1;
		unsigned auto_ETU     : 1;
		unsigned auto_parity  : 1;
	} bits;
} rSCI_CNTL2;

typedef union {
	unsigned int value;
	struct {
		unsigned clkdiv       : 8;
		unsigned              : 8;
		unsigned repeat_time  : 4;
		unsigned ETU          : 12;
	} bits;
} rSCI_CNTL3;

typedef union {
	unsigned int value;
	struct {
		unsigned sci_in          : 1;
		unsigned sci_out         : 1;
		unsigned rxfifo_status   : 1;
		unsigned txfifo_status   : 1;
		unsigned txfifo_empty    : 1;
		unsigned rxfifo_overflow : 1;
		unsigned repeat_error    : 1;
		unsigned reset_finish    : 1;
		unsigned nack            : 1;
		unsigned rcv_over        : 1;
		unsigned card_in         : 1;
		unsigned card_in_diff    : 1;
		unsigned                 : 4;
		unsigned data_size       : 10;
	} bits;
} rSCI_INT;

typedef union {
	unsigned int value;
	struct {
		unsigned ETU_ack2        : 12;
		unsigned                 : 4;
		unsigned valid           : 1;
	} bits;
} rSCI_ACK2;

typedef union {
	unsigned int value;
	struct {
		unsigned cldrst_length   : 16;
		unsigned valid           : 1;
	} bits;
} rSCI_RST;

typedef union {
	unsigned int value;
	struct {
		unsigned no_calc_parity  : 1;
	} bits;
} rSCI_PARITY;

// sirius update
typedef union {
	unsigned int value;
	struct {
		unsigned clk_en          : 1;
		unsigned vcc_en          : 1;
		unsigned rst_en          : 1;
		unsigned                 : 1;
		unsigned clk_fb          : 4;   // gemini update
		unsigned din_hold_en     : 1;
		unsigned                 : 3;
		unsigned card_in_sw      : 1;
	} bits;
} rSCI_IO;

// cygnus update
typedef union {
	unsigned int value;
	struct {
		unsigned txfifo_level    : 9;
		unsigned                 : 7;
		unsigned rxfifo_level    : 9;
	} bits;
} rSCI_FIFO;

typedef struct {
	rSCI_CNTL1     ctrl1;
	rSCI_CNTL2     ctrl2;
	rSCI_CNTL3     ctrl3;
	rSCI_INT       isr;
	rSCI_INT       isr_en;
	uint32_t       EGT;
	uint32_t       TGT;
	uint32_t       WDT;
	uint32_t       TWDT;
	uint32_t       data;
	rSCI_ACK2      ack2_cfg;
	rSCI_RST       cldrst_cfg;
	rSCI_RST       hotrst_cfg;
	rSCI_PARITY    parity_cfg;
	uint32_t       rserv;
	rSCI_FIFO      fifo_gate;
	rSCI_IO        io_cfg;
} SCIReg;

#endif
