#ifndef __GUESTBUS_H__
#define __GUESTBUS_H__


struct guestbus_device_info {
	void __iomem *regs;
	unsigned int reg_addr;
	unsigned int reg_size;
	unsigned int irq;
	struct device *dev;
};

#define MISC_GUESTBUS_MINOR 131
#define GUESTBUS_FW_NAME	"gport.fw"

#define	GUESTBUS_IOCTL_BASE	'G'

#define GUESTBUS_ENTRY     _IOR(GUESTBUS_IOCTL_BASE, 0, int)

#define GUESTBUS_RAM_SIZE 0x1000

#define SHAREMEM_BUF_LENGTH	8
#define SHAREMEM_ACPU2IOCPU	0x000
#define SHAREMEM_IOCPU2ACPU	0x100
#define SHAREMEM_STATUS		0X200
#define IOCPU_ENABLE_REG	(SHAREMEM_STATUS + 0x20)
#endif
