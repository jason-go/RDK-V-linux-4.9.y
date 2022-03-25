obj-$(CONFIG_FB_NATIONALCHIP) += gxfb.o

EXTRA_CFLAGS += -DLINUX_OS -DUSE_IMMEDIATE
EXTRA_CFLAGS += -DGX_DEBUG_PRINTF_LEVEL=2

EXTRA_CFLAGS += -include drivers/media/video/nationalchip/avcore/include/autoconf.h

EXTRA_CFLAGS += -Idrivers/media/video/nationalchip
EXTRA_CFLAGS += -Idrivers/media/video/nationalchip/avcore
EXTRA_CFLAGS += -Idrivers/media/video/nationalchip/avcore/include
EXTRA_CFLAGS += -Idrivers/media/video/nationalchip/avcore/vpu
EXTRA_CFLAGS += -Idrivers/media/video/nationalchip/avcore/vout
EXTRA_CFLAGS += -Idrivers/media/video/nationalchip/avcore/hdmi
EXTRA_CFLAGS += -Idrivers/media/video/nationalchip/avcore/audio

fb-source += gxfb_dev.o
fb-source += gxfb_sysfs.o

gxfb-objs += $(fb-source)

