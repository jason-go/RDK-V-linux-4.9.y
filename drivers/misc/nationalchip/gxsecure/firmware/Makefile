include $(GX_SRC_PATH)/.config

secure-source += main.c
secure-source += gx_fw_$(FW_TYPE).c
ifeq ($(FW_TYPE), ABVCAS)
cflags-include += -I$(GX_SRC_PATH)/firmware/CA/abv
LIBCA = -L$(GX_SRC_PATH)/firmware/CA/abv -labvcarless45
endif

ifeq ($(CFG_GX_SOFT_DES), y)
	secure-source += ../drivers/soft_alg/gx_soft_des.c
endif
ifeq ($(CFG_GX_SOFT_SHA), y)
	secure-source += ../drivers/soft_alg/gx_soft_sha_common.c
ifeq ($(CFG_GX_SOFT_SHA1), y)
	secure-source += ../drivers/soft_alg/gx_soft_sha1.c
endif
ifeq ($(CFG_GX_SOFT_SHA256), y)
	secure-source += ../drivers/soft_alg/gx_soft_sha256.c
endif
endif # end of sha
ifeq ($(CFG_GX_SOFT_SM3), y)
	secure-source += ../drivers/soft_alg/gx_soft_sm3.c
endif

include $(GX_SRC_PATH)/firmware/sirius.mk
include $(GX_SRC_PATH)/firmware/taurus.mk
include $(GX_SRC_PATH)/scripts/conf.mk
include $(GX_SRC_PATH)/scripts/mod.mk
include $(GX_SRC_PATH)/scripts/inc.Makefile

