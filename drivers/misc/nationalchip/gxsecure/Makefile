obj-$(CONFIG_GXSECURE) += gxsecure.o


EXTRA_CFLAGS += -DLINUX_OS -DCPU_ACPU -DARCH_ARM
EXTRA_CFLAGS += -DGX_DEBUG_PRINTF_LEVEL=2
EXTRA_CFLAGS += -DCFG_CRYPTO_DMA_CHANNEL_NUM=1

EXTRA_CFLAGS += -Iinclude/linux/gxsecure
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/include/ 
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/include
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/include/kapi
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/include/external
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/include/common
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/include/soft_alg
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/crypto_drv
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/secure_drv
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/misc_drv
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/misc_drv/otp
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/misc_drv/rng
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/misc_drv/chip
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/misc_drv/timer
EXTRA_CFLAGS += -Idrivers/misc/nationalchip/gxsecure/drivers/misc_drv/sensor

EXTRA_CFLAGS += -DCFG_GXSE_FIRMWARE
EXTRA_CFLAGS += -DCFG_GXSE_FIRMWARE_MBOX
EXTRA_CFLAGS += -DCFG_MBOX_PROTOCOL_S5H
EXTRA_CFLAGS += -DCFG_MBOX_PROTOCOL_IFCP
EXTRA_CFLAGS += -DCFG_MBOX_PROTOCOL_GENERIC
EXTRA_CFLAGS += -DCFG_GXSE_KLM
EXTRA_CFLAGS += -DCFG_GXSE_KLM_GENERIC
EXTRA_CFLAGS += -DCFG_GXSE_KLM_IRDETO
EXTRA_CFLAGS += -DCFG_GXSE_KLM_SECURE_GENERIC
EXTRA_CFLAGS += -DCFG_GXSE_CRYPTO
EXTRA_CFLAGS += -DCFG_GXSE_CRYPTO_FIFO
EXTRA_CFLAGS += -DCFG_GXSE_CRYPTO_DMA
EXTRA_CFLAGS += -DCFG_GXSE_MISC
EXTRA_CFLAGS += -DCFG_GXSE_MISC_SCI
EXTRA_CFLAGS += -DCFG_GXSE_MISC_OTP
EXTRA_CFLAGS += -DCFG_GXSE_MISC_RNG
EXTRA_CFLAGS += -DCFG_GXSE_MISC_CHIP_CFG
EXTRA_CFLAGS += -DCFG_GXSE_FIREWALL
EXTRA_CFLAGS += -DCFG_GXSE_MISC_HASH

secure-source += common_dev.o
secure-source += gxsecure_dev.o
secure-source += firewall_dev.o
secure-source += secure_dev.o 
secure-source += klm_dev.o
secure-source += crypto_dev.o
secure-source += misc_dev.o
  
secure-source += osdep/osdep-linux.o
secure-source += common/crc32.o
secure-source += common/log.o
secure-source += common/fifo.o

secure-source += drivers/gxse_core.o

ifeq ($(CONFIG_SIRIUS), y)
secure-source += drivers/chips/sirius.o
endif

secure-source += drivers/kapi/gxse_kapi_firewall.o
secure-source += drivers/kapi/gxse_kapi_secure.o
secure-source += drivers/kapi/gxse_kapi_misc.o
secure-source += drivers/kapi/gxse_kapi_tfm.o
secure-source += drivers/kapi/gxse_kapi_external.o
secure-source += drivers/secure_drv/gxsecure_virdev.o
secure-source += drivers/secure_drv/sirius_mbox.o
secure-source += drivers/secure_drv/protocol/gxsecure_protocol.o
secure-source += drivers/secure_drv/protocol/gxsecure_protocol_S5H.o
secure-source += drivers/secure_drv/protocol/gxsecure_protocol_IFCP.o
secure-source += drivers/secure_drv/protocol/gxsecure_protocol_generic.o
secure-source += drivers/klm_drv/gxklm_virdev.o
secure-source += drivers/klm_drv/sirius_klm_generic.o
secure-source += drivers/klm_drv/sirius_klm_irdeto.o
secure-source += drivers/klm_drv/sirius_klm_secure_dynamic.o
secure-source += drivers/crypto_drv/gxcrypto_virdev.o
secure-source += drivers/crypto_drv/sirius_crypto.o
secure-source += drivers/crypto_drv/sirius_m2m.o
secure-source += drivers/misc_drv/gxmisc_virdev.o
secure-source += drivers/misc_drv/sci/gxmisc_sci_common.o
secure-source += drivers/misc_drv/sci/sci_internal.o
secure-source += drivers/misc_drv/sci/taurus_sci.o
secure-source += drivers/misc_drv/sci/sirius_sci.o
secure-source += drivers/misc_drv/otp/gxmisc_otp_common.o
secure-source += drivers/misc_drv/otp/sirius_otp.o
secure-source += drivers/misc_drv/rng/gxmisc_rng_common.o
secure-source += drivers/misc_drv/rng/sirius_rng.o
secure-source += drivers/misc_drv/chip/gxmisc_chip_common.o
secure-source += drivers/misc_drv/chip/sirius_chip.o
secure-source += drivers/misc_drv/firewall/gxmisc_fw_common.o
secure-source += drivers/misc_drv/firewall/sirius_firewall.o
secure-source += drivers/dgst_drv/gxdgst_virdev.o
secure-source += drivers/dgst_drv/sirius_hash.o
secure-source += drivers/soft_alg/gx_soft_sha_common.o
secure-source += drivers/soft_alg/gx_soft_sha1.o
secure-source += drivers/soft_alg/gx_soft_sha256.o
secure-source += drivers/soft_alg/gx_soft_sm3.o

gxsecure-objs += $(secure-source)

