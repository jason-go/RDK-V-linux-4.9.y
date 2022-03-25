ifeq ($(CHIP_ID), CHIP_GX_SIRIUS)

cflags-compile += -DCONFIG_SIRIUS
LDFILE  = -Tarch/csky/mach-sirius/link.ld
secure-ssource = arch/csky/mach-sirius/start.S

endif
