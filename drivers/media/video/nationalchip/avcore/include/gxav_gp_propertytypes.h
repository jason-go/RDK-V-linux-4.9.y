#ifndef __GXAV_GP_PROPERTYTYPES_H__
#define __GXAV_GP_PROPERTYTYPES_H__

#include "gxav_common.h"

enum gp_ds_mode {
	GP_DES_AES_ECB,
	GP_DES_AES_CBC,
	GP_DES_AES_CTR,
};

typedef struct {
	int             id;
	enum gp_ds_mode mode;
	unsigned int    cw_from_acpu;
	unsigned int    cw[4];
	unsigned int    iv[4];
	unsigned int    ctr_iv_size;
} GxGPProperty_Cw;

typedef struct {
	unsigned int cw_id;
	unsigned int data_ptr;
	unsigned int total_len;

	unsigned int unit_len;
	unsigned int unit_scramble_len;
	unsigned int iv_reset;
} GxGPProperty_Des;

typedef struct {
	void*        buffer;
	unsigned int max_size;
	unsigned int read_size;
} GxGPProperty_Read;

#endif

