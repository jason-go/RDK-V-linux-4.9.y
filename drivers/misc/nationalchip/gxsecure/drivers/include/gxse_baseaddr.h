#ifndef __BASE_ADDR_H_
#define __BASE_ADDR_H_

/************************ SCPU ************************/
#define GXSCPU_SYS_CLK                     148
#define GXSCPU_BASE_ADDR_CONFIG            0x08000000
#define GXSCPU_BASE_ADDR_MAILBOX           0x00100000
#define GXSCPU_BASE_ADDR_KLM               0x00110000
#define GXSCPU_BASE_ADDR_HASH              0x00120000
#define GXSCPU_BASE_ADDR_TIMER             0x00130000
#define GXSCPU_BASE_ADDR_MISC              0x00140000
#define GXSCPU_BASE_ADDR_CRYPTO            0x00150000
#define GXSCPU_BASE_ADDR_UART              0x00160000
#define GXSCPU_BASE_ADDR_MAILBOX_TEE       0x00170000
#define GXSCPU_BASE_ADDR_SENSOR            0x00180000
#define GXSCPU_BASE_ADDR_MPU               0x00200000
#define CK802_BASE_ADDR                    0xe0000000

/************************ SIRIUS ACPU ************************/

#define GXACPU_BASE_ADDR_SECURE_TEE        0x88300000
#define GXACPU_BASE_ADDR_SECURE            0x88400000
#define GXACPU_BASE_ADDR_SM2               0x88600000
#define GXACPU_BASE_ADDR_CRYPTO            0x89000000
#define GXACPU_BASE_ADDR_KLM               0x89100000
#define GXACPU_BASE_ADDR_M2M               0x89200000
#define GXACPU_BASE_ADDR_CHIP_CFG          0x89400000
#define GXACPU_BASE_ADDR_HASH              0x89600000
#define GXACPU_BASE_ADDR_OTP               0x89900000
#define GXACPU_BASE_ADDR_GPIO0             0x82404000
#define GXACPU_BASE_ADDR_GPIO1             0x82405000
#define GXACPU_BASE_ADDR_GPIO2             0x82406000
#define GXACPU_BASE_ADDR_GXI2C0            0x82000000
#define GXACPU_BASE_ADDR_GXI2C1            0x82001000
#define GXACPU_BASE_ADDR_GXI2C2            0x82002000
#define GXACPU_BASE_ADDR_DWI2C0            0x82004000
#define GXACPU_BASE_ADDR_DWI2C1            0x82005000
#define GXACPU_BASE_ADDR_DWUART0           0x82804000
#define GXACPU_BASE_ADDR_DWUART1           0x82805000
#define GXACPU_BASE_ADDR_DEMUX             0x8a200000
#define GXACPU_BASE_ADDR_FIREWALL          0x88e00000
#define GXACPU_BASE_ADDR_SCI               0x82c00000

#define GXACPU_IRQ_SECURE                  19
#define GXACPU_IRQ_SECURE_TEE              30
#define GXACPU_IRQ_CRYPTO                  25
#define GXACPU_IRQ_M2M                     26
#define GXACPU_IRQ_KLM                     27
#define GXACPU_IRQ_HASH                    28
#define GXACPU_IRQ_DEMUX                   1
#define GXACPU_IRQ_DWUART0                 34
#define GXACPU_IRQ_DWUART1                 33
#define GXACPU_IRQ_SCI                     32

#define GXACPU_GPIO_SRAM_BASE              0x80404000

/************************ TAURUS ACPU ************************/

#define GXACPU_BASE_ADDR_TAURUS_SECURE       0x00b00000
#define GXACPU_BASE_ADDR_TAURUS_CHIP_CFG     0x0030A000
#define GXACPU_BASE_ADDR_TAURUS_OTP          0x00f80000
#define GXACPU_BASE_ADDR_TAURUS_M2M          0x00fc0000
#define GXACPU_BASE_ADDR_TAURUS_SCI          0x00600000
#define GXACPU_BASE_ADDR_TAURUS_FIREWALL     0

#define GXACPU_IRQ_TAURUS_SECURE             63
#define GXACPU_IRQ_TAURUS_M2M                48
#define GXACPU_IRQ_TAURUS_SCI                7

/************************ GX3211 ACPU ************************/

#define GXACPU_BASE_ADDR_GX3211_SCI          0x00207000

#define GXACPU_IRQ_GX3211_SCI                7

/************************ GX6605S / PEGASUS ACPU ************************/

#define GXACPU_BASE_ADDR_GX6605S_SCI         0x00207000
#define GXACPU_BASE_ADDR_PEGASUS_SCI         0x00D00000

#define GXACPU_IRQ_GX6605S_SCI               7

#endif
