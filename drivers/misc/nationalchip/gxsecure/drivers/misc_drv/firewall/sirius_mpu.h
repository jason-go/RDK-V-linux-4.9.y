#ifndef __GXSCPU_MPU_H__
#define __GXSCPU_MPU_H__

#define MPU_REG_REGION_EN          0x0
#define MPU_REG_FAULT_ADDR         0x8
#define MPU_REG_FAULT_ATTR         0xc

#define mMPU_FAULT_ATTR_HPROT0     (0x1<<0)
#define mMPU_FAULT_ATTR_HPROT1     (0x1<<1)
#define mMPU_FAULT_ATTR_HWRITE     (0x1<<5)
#define mMPU_FAULT_ATTR_FLAG       (0x1<<17)

#define MPU_REG_AUTH0              0x10
#define MPU_REG_AUTH1              0x14
#define MPU_REG_AUTH2              0x18
#define MPU_REG_AUTH3              0x1c

#define MPU_REG_REGION0_START      0x80
#define MPU_REG_REGION1_START      0x88
#define MPU_REG_REGION2_START      0x90
#define MPU_REG_REGION3_START      0x98
#define MPU_REG_REGION4_START      0xa0
#define MPU_REG_REGION5_START      0xa8
#define MPU_REG_REGION6_START      0xb0
#define MPU_REG_REGION7_START      0xb8
#define MPU_REG_REGION8_START      0xc0
#define MPU_REG_REGION9_START      0xc8
#define MPU_REG_REGION10_START     0xd0
#define MPU_REG_REGION11_START     0xd8
#define MPU_REG_REGION12_START     0xe0
#define MPU_REG_REGION13_START     0xe8
#define MPU_REG_REGION14_START     0xf0
#define MPU_REG_REGION15_START     0xf8

#define MPU_REG_REGION0_END        0x84
#define MPU_REG_REGION1_END        0x8c
#define MPU_REG_REGION2_END        0x94
#define MPU_REG_REGION3_END        0x9c
#define MPU_REG_REGION4_END        0xa4
#define MPU_REG_REGION5_END        0xac
#define MPU_REG_REGION6_END        0xb4
#define MPU_REG_REGION7_END        0xbc
#define MPU_REG_REGION8_END        0xc4
#define MPU_REG_REGION9_END        0xcc
#define MPU_REG_REGION10_END       0xd4
#define MPU_REG_REGION11_END       0xdc
#define MPU_REG_REGION12_END       0xe4
#define MPU_REG_REGION13_END       0xec
#define MPU_REG_REGION14_END       0xf4
#define MPU_REG_REGION15_END       0xfc

#endif
