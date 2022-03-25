#ifndef __SIRIUS_TIMER_REG_H__
#define __SIRIUS_TIMER_REG_H__

typedef union {
	unsigned int value;
	struct {
		unsigned status        : 1;
	} bits;
} rTIMER_STATUS;

typedef union {
	unsigned int value;
	struct {
		unsigned reset         : 1;
		unsigned start         : 1;
	} bits;
} rTIMER_CTRL;

typedef union {
	unsigned int value;
	struct {
		unsigned enable        : 1;
		unsigned int_en        : 1;
	} bits;
} rTIMER_CONFIG;

typedef union {
	unsigned int value;
	struct {
		unsigned pre_div       : 16;
	} bits;
} rTIMER_PREDIV;

typedef struct {
	rTIMER_STATUS       counter_status;
	unsigned int        counter_val;
	unsigned int        counter_accsnap;
	unsigned int        reserved_0;
	rTIMER_CTRL         counter_ctrl;
	unsigned int        reserved_1[3];
	rTIMER_CONFIG       counter_config;
	rTIMER_PREDIV       counter_prediv;
	unsigned int        counter_ini;
	unsigned int        reserved_2;
	unsigned int        counter_acc;
} SiriusTimerReg;

#endif
