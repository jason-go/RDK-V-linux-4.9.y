#ifndef __GX_OTP_H
#define __GX_OTP_H

#define BETWEEN(value, min, max)  ((unsigned int)value < (unsigned int)min ? 0 : \
                ((unsigned int)value > (unsigned int)max ? 0 : 1))

#define SIRIUS_OTP_CTRL			*(volatile unsigned int *)(o_info->reg_base + 0x00)
#define SIRIUS_OTP_STATUS		*(volatile unsigned int *)(o_info->reg_base + 0x04)
#define SIRIUS_OTP_FLAGS0			*(volatile unsigned int *)(o_info->reg_base + 0x08)
#define SIRIUS_OTP_FLAGS1			*(volatile unsigned int *)(o_info->reg_base + 0x0C)
#define SIRIUS_OTP_FLAGS2			*(volatile unsigned int *)(o_info->reg_base + 0x10)
#define SIRIUS_OTP_FLAGS3			*(volatile unsigned int *)(o_info->reg_base + 0x14)

#define SIRIUS_OTP_CTRL_ADDRESS			(0)
#define SIRIUS_OTP_CTRL_WR_DATA			(16)
#define SIRIUS_OTP_CTRL_RD			    (30)
#define SIRIUS_OTP_CTRL_WR			    (31)

#define SIRIUS_OTP_STATUS_RD_DATA		(0)
#define SIRIUS_OTP_STATUS_BUSY			(8)
#define SIRIUS_OTP_STATUS_RD_VALID		(9)
#define SIRIUS_OTP_STATUS_RW_FAIL		(12)

#define TAURUS_OTP_CON_REG (*(volatile unsigned int*)(o_info->reg_base + 0x80))
#define TAURUS_OTP_CFG_REG (*(volatile unsigned int*)(o_info->reg_base + 0x84))
#define TAURUS_OTP_STA_REG (*(volatile unsigned int*)(o_info->reg_base + 0x88))

typedef int (*otp_write_ops)(unsigned int start_addr, int rd_num, unsigned char *data);
typedef int (*otp_read_ops)(unsigned int start_addr, int rd_num, unsigned char *data);

struct otp_table {
	unsigned int chip_id;
	unsigned int max_size;
	otp_read_ops   read;
	otp_write_ops  write;
};

struct otp_info {
	void __iomem *reg_base;
	struct device *dev;
	struct otp_table table;
};

extern unsigned int gx_chip_id_probe(void);

#endif
