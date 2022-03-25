ifeq ($(CHIP_ID), CHIP_GX_TAURUS)

cflags-compile += -DCONFIG_TAURUS

ifeq ($(CFG_FW_DDR), y)
LDFILE  = -Tarch/csky/mach-taurus/link_ddr.ld
else
LDFILE  = -Tarch/csky/mach-taurus/link_sram.ld
endif
secure-ssource = arch/csky/mach-taurus/start.S

CFG_GX_FIREWALL = n
CFG_GX_MISC_SENSOR = n
CFG_GX_MISC_HASH = n

endif
