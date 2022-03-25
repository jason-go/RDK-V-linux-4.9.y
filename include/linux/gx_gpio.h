#ifndef __GX_GPIO_H_
#define __GX_GPIO_H_

enum
{
	GX_GPIO_INPUT = 0,
	GX_GPIO_OUTPUT
};

enum
{
	GX_GPIO_LOW = 0,
	GX_GPIO_HIGH
};

enum
{
	GX_GPIO_CONFIG_INVALID = 0,
	GX_GPIO_CONFIG_VALID
};

//ioctl arg
struct gx_gpio_arg
{
	unsigned int gpio_num;
	unsigned int gpio_io;
	unsigned int gpio_level;
};

//ioctl pdm arg
struct gx_gpio_pdm_arg {
	unsigned int gpio_num;
	unsigned int pdm_enable;
	unsigned int div_clock;
	unsigned int duty_cycle;
};

#define		GPIO_IOCTL_BASE		'G'
//ioctl cmd
#define		GX_GPIO_SET_IO		_IOR(GPIO_IOCTL_BASE, 0, struct gx_gpio_arg)
#define		GX_GPIO_GET_IO		_IOR(GPIO_IOCTL_BASE, 1, struct gx_gpio_arg)
#define		GX_GPIO_SET_LEVEL	_IOR(GPIO_IOCTL_BASE, 2, struct gx_gpio_arg)
#define		GX_GPIO_GET_LEVEL	_IOR(GPIO_IOCTL_BASE, 3, struct gx_gpio_arg)
#define		GX_GPIO_SET_PDM		_IOR(GPIO_IOCTL_BASE, 4, struct gx_gpio_pdm_arg)
#define		GX_GPIO_GET_PDM		_IOR(GPIO_IOCTL_BASE, 5, struct gx_gpio_pdm_arg)


int gx_gpio_setio(unsigned long gpio, unsigned long io);
int gx_gpio_getio(unsigned long gpio);
int gx_gpio_getlevel(unsigned long gpio);
int gx_gpio_setlevel(unsigned long gpio, unsigned long value);
int  gx_gpio_getphy(unsigned long vir_gpio, unsigned long *phy_gpio);

/* Will add driver only when there are customers to use */
/*int gx_gpio_request_irq(unsigned long gpio, GX_GPIO_IRQ_HANDLER irq_handler);
int gx_gpio_free_irq(unsigned long gpio);
int gx_gpio_setinverse(unsigned long gpio, unsigned long value);
int gx_gpio_setirqmode(unsigned long gpio, unsigned long int_enabled, enum gx_gpio_int_triggermode mode);
int gx_gpio_getirqmode(unsigned long gpio, unsigned long* int_enabled, enum gx_gpio_int_triggermode* mode);*/

#endif

