#ifndef __SCI_CHIPTEST_H__
#define __SCI_CHIPTEST_H__

#define SCI_TEST_FLAG_ERROR_PARITY     (0x1<<0)
#define SCI_TEST_FLAG_RX_GATE          (0x1<<1)
#define SCI_TEST_FLAG_RX_OVERFLOW      (0x1<<2)
#define SCI_TEST_FLAG_RX_OVERTIME      (0x1<<3)

extern uint32_t g_sci_flag;

extern void gxse_chiptest_sci_init(uint32_t reg_base);

#endif
