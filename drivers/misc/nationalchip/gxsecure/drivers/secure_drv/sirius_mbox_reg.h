#ifndef __SIRIUS_MBOX_REG_H__
#define __SIRIUS_MBOX_REG_H__

typedef union {
	unsigned int value;
	struct {
		unsigned data_len    : 5;
		unsigned             : 3;
		unsigned oob         : 8;
		unsigned write_index : 5;
		unsigned             : 9;
		unsigned state       : 2;
	} bits;
} rMB_WCSR;

typedef union {
	unsigned int value;
	struct {
		unsigned data_len    : 5;
		unsigned             : 3;
		unsigned oob         : 8;
		unsigned write_index : 5;
		unsigned             : 3;
		unsigned dis_h_l     : 1;
		unsigned dis_l_h     : 1;
		unsigned             : 4;
		unsigned state       : 2;
	} bits;
} rMB_RCSR;

typedef union {
	unsigned int value;
	struct {
		unsigned not_full    : 1;
		unsigned not_empty   : 1;
		unsigned packet_done : 1;
		unsigned             : 28;
		unsigned sec_vio     : 1;
	} bits;
} rMB_INT;

typedef struct {
	rMB_WCSR        wcsr;
	rMB_RCSR        rcsr;
	rMB_INT         mb_int;
	rMB_INT         mb_int_en;
    unsigned int    write_seq;
    unsigned int    read_seq;
	unsigned int    reserved[2];
    unsigned int    data;

	unsigned int    reserved1[7];
	unsigned int    BIV[4];
} SiriusMboxReg;

#endif
