#ifndef __AV_HAL_PROPERTYTYPES_SECURE_H__
#define __AV_HAL_PROPERTYTYPES_SECURE_H__

#include "secure_hal.h"

struct avhal_secure_property_encrypt {
	int res;
	int ret;
	GxTfmCrypto param;
};

struct avhal_secure_property_decrypt {
	int res;
	int ret;
	GxTfmCrypto param;
};

struct avhal_secure_property_otp_read {
	int res;
	int ret;
	unsigned int addr;
};

struct avhal_secure_property_get_protect_buffer {
	int res;
	int ret;
};

struct avhal_secure_property_query_access_align {
	int res;
	int ret;
};

struct avhal_secure_property_query_protect_align {
	int res;
	int ret;
};

struct avhal_secure_property_hdcp_state {
	int res;
	int ret;
};

struct avhal_secure_property_register_read {
	int res;
	int ret;
	unsigned int base;
	unsigned int offset;
};

struct avhal_secure_property_register_write {
	int res;
	int ret;
	unsigned int base;
	unsigned int offset;
	unsigned int value;
};

struct avhal_secure_property_get_macrovision {
	int res;
	int ret;
};

struct avhal_secure_property_get_hdr {
	int res;
	int ret;
};

struct avhal_secure_property_get_hevc {
	int res;
	int ret;
};

#endif
