#ifndef __HASH_REG_H__
#define __HASH_REG_H__

typedef union {
	unsigned int value;
	struct {
		unsigned dma_on          : 1;
		unsigned dma_start       : 1;
		unsigned                 : 6;
		unsigned fixed_brst      : 1;
		unsigned mixed_brst      : 1;
		unsigned addr_align_brst : 1;
	} bits;
} rHashDmaConf;

typedef union {
	unsigned int value;
	struct {
		unsigned hash_endian     : 1;
		unsigned data_endian     : 1;
		unsigned                 : 6;
		unsigned alg             : 8;
	} bits;
} rHashCtrl1;

typedef union {
	unsigned int value;
	struct {
		unsigned msg_start       : 1;
		unsigned msg_finish      : 1;
		unsigned bytes_valid     : 2;
	} bits;
} rHashCtrl2;

typedef union {
	unsigned int value;
	struct {
		unsigned hash_busy       : 1;
	} bits;
} rHashStatus;

typedef union {
	unsigned int value;
	struct {
		unsigned sha_1_done      : 1;
		unsigned sha_256_done    : 1;
		unsigned sm3_done        : 1;
		unsigned                 : 5;
		unsigned dma_done        : 1;
	} bits;
} rHashInt;

typedef struct {
	unsigned int    msg_data;
	unsigned int    reserved0[19];

	unsigned int    dma_start_addr;
	unsigned int    dma_len;
	rHashDmaConf    dma_conf;
	unsigned int    reserved1[41];

	rHashCtrl1      ctrl1;
	rHashCtrl2      ctrl2;
	rHashStatus     status;
	unsigned int    reserved2[17];

	rHashInt        hash_int_en;
	rHashInt        hash_int;

	unsigned int    reserved3[938];
	unsigned int    sha_1[5];
	unsigned int    reserved[3];
	unsigned int    sha_256[8];
	unsigned int    sm3[8];
} SiriusDgstReg;

#endif
