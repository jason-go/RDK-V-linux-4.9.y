#include <linux/i2c.h>
#include <linux/slab.h>

struct gx_i2c_device
{
	int                 i2c_devid; /* the id is i2c device id */
	unsigned int        chip_address;
	unsigned int        address_width; /* 1 or 2 bytes, often */
	int                 send_noack;
	int                 send_stop;
	struct i2c_adapter *adap;
	struct gx_i2c_device *next;
};

static struct gx_i2c_device *g_i2c_list = NULL;
static int g_max_busid = -1;
static int g_bLittleEndian = 1;	/* default little endian */
extern unsigned int gx_chip_id_probe(void);

static int gx_i2c_bus_cnt(void)
{
	unsigned int gx_chipid = gx_chip_id_probe();

	switch (gx_chipid)
	{
		case 0x6612:
		case 0x6616:
			g_max_busid = 2;
			g_bLittleEndian = 1;
			return 3;	// i2c bus: 3 gx-i2c bus  2 dw-i2c bus
		case 0x3211:
			g_max_busid = 2;
			g_bLittleEndian = 1;
			return 3;   // i2c bus: 3 gx-i2c bus  2 dw-i2c bus
		default:
			printk("%s(), chip=0x%x probe fail.\n", __func__, gx_chipid);
			return -1;
	}
}

static struct gx_i2c_device* gx_i2c_dev_register(int id)
{
	struct gx_i2c_device* i2c_device = NULL;
	struct gx_i2c_device* last_node = g_i2c_list;
	int i2c_cnt = 0;

	/* chip must contain i2c controller */
	i2c_cnt = gx_i2c_bus_cnt();
	if (i2c_cnt < 0) {
		printk("%s(), no i2c controller in chip.\n", __func__);
		return NULL;
	}

	if (id >= i2c_cnt) {
		printk("error : id must less %d\n", i2c_cnt);
		return NULL;
	}

	i2c_device = (struct gx_i2c_device *)kzalloc(sizeof(struct gx_i2c_device), GFP_KERNEL);
	if (!i2c_device)
	{
		printk("%s(), i2c device node malloc error.\n", __func__);
		return NULL;
	}

	/* init the device node */
	i2c_device->chip_address  = 0;
	i2c_device->address_width = 2; /* remember to modify if your device regaddress width isn't 2 bytes */
	i2c_device->i2c_devid     = id;
	i2c_device->next          = NULL;
	i2c_device->send_noack    = true;
	i2c_device->send_stop     = true;
	i2c_device->adap	  = i2c_get_adapter(id);

	/* add this device node at list tail */
	if (!g_i2c_list)
		g_i2c_list = i2c_device;
	else {
		while (last_node->next)
			last_node = last_node->next;
		last_node->next = i2c_device;
	}

	return i2c_device;
}

static int gx_i2c_dev_unregister(struct gx_i2c_device* device)
{
	struct gx_i2c_device *previous_i2c  = g_i2c_list;
	struct gx_i2c_device *current_i2c   = g_i2c_list;

	if (!device) return -1;

	while (current_i2c) {
		/* delete the device node if found */
		if (device->i2c_devid==current_i2c->i2c_devid) {
			if (previous_i2c==current_i2c) /* first node */
				g_i2c_list = current_i2c->next;
			else
				previous_i2c->next = current_i2c->next;

			kfree(device);
			return 0;
		}

		previous_i2c = current_i2c;
		current_i2c  = current_i2c->next;
	}

	/* no the specified device found */
	return -2;
}

static struct gx_i2c_device *gx_i2c_open_dev(unsigned int id)
{
	struct gx_i2c_device *i2c_device = g_i2c_list;

	/* find the i2c device node */
	while (i2c_device) {
		if (i2c_device->i2c_devid == id) {
//			printk("%s(),find active i2c device,i2c_devid=%d\n", __func__, i2c_device->i2c_devid);
			return i2c_device;
		}
		i2c_device = i2c_device->next;
	};

	/* if list is NULL or device not found, create one node */
	return gx_i2c_dev_register(id);
}

static int gx_i2c_setdev(void *dev, unsigned int busid, unsigned int chip_addr,
		unsigned int address_width, int send_noack, int send_stop)
{
	struct gx_i2c_device *i2c_device = (struct gx_i2c_device *)dev;

	if (!i2c_device) return -1;
	if (g_max_busid < 0) return -2;
	if (busid > g_max_busid) return -3;
	/* I2C device address should be even */
	if (chip_addr & 1) return -4;
	//	if (!address_width) return -5; /* 0 width enabled (eg. A8293, SP5010) */
	if (!i2c_device->adap) return -6;

	//printk("%s:busid=%d, chip_addr=0x%x, address_width=%d, send_noack=%d, send_stop=%d\n",
	//	__func__, busid, chip_addr, address_width, send_noack, send_stop);

	if(i2c_device->i2c_devid == 3){	// if dw-i2c, chip_addr must be actual chip_addr.
		i2c_device->chip_address = chip_addr >> 1;
	} else
		i2c_device->chip_address  = chip_addr;

	i2c_device->address_width = address_width;
	i2c_device->send_noack    = send_noack;
	i2c_device->send_stop     = send_stop;

	return 0;
}

static int gx_i2c_transaction_tx(void *dev, unsigned int reg_address, const unsigned char *tx_data, unsigned int count)
{
	struct gx_i2c_device *i2c_device = (struct gx_i2c_device *)dev;
	int ret = 0;
	unsigned char buf[4] = {0};

	struct i2c_adapter *adap = i2c_device->adap;
	struct i2c_msg msgs[2];

	if (!i2c_device || !dev) return -EINVAL;

	/* change endianness for ARM */
	if (g_bLittleEndian)
	{
		switch (i2c_device->address_width)
		{
			case 0:
				break;

			case 1:
				buf[0] = (unsigned char)reg_address;
				break;

			case 2:
				buf[0] = (unsigned char)(reg_address>>8);
				buf[1] = (unsigned char)reg_address;
				break;

			case 4:
				buf[0] = (unsigned char)(reg_address>>24);
				buf[1] = (unsigned char)(reg_address>>16);
				buf[2] = (unsigned char)(reg_address>>8);
				buf[3] = (unsigned char)reg_address;
				break;

				/* invalid address width */
			default:
				return -EFAULT;
		}
	}
	else /* CK */
	{
		switch (i2c_device->address_width)
		{
			case 0:
				break;

			case 1:
				buf[0] = (unsigned char)reg_address;
				break;

			case 2:
				buf[0] = (unsigned char)reg_address;
				buf[1] = (unsigned char)(reg_address>>8);
				break;

			case 4:
				buf[0] = (unsigned char)reg_address;
				buf[1] = (unsigned char)(reg_address>>8);
				buf[2] = (unsigned char)(reg_address>>16);
				buf[3] = (unsigned char)(reg_address>>24);
				break;

				/* invalid address width */
			default:
				return -EFAULT;
		}
	}

	//I2C_DBG("\n\n\n--> %s: i2c device: %p\n", __func__, i2c_device);
	/* Send chip address. */
	msgs[0].len  = i2c_device->address_width;
	msgs[0].addr = i2c_device->chip_address;
	msgs[0].buf  = &buf[0];
	if (i2c_device->send_stop && i2c_device->address_width)
		msgs[0].flags = 0;
	else
		msgs[0].flags = I2C_M_REV_DIR_ADDR;

	/* Write data. */
	msgs[1].len  = count;
	msgs[1].addr = i2c_device->chip_address;
	msgs[1].buf  = (unsigned char *)tx_data;
	if (i2c_device->send_noack && i2c_device->address_width)
		msgs[1].flags = I2C_M_NOSTART ;
	else
		msgs[1].flags = 0;

	/* don't send chip address for a8293, send stop at last */
	if (!i2c_device->address_width)
		msgs[1].flags |= I2C_M_NOSTART;

#ifdef GX_I2C_LOCK
	ret = adap->algo->master_xfer(adap,msgs,2);
#else
	ret = i2c_transfer(adap, msgs, 2);
#endif
	if (ret < 0)
		printk("TX: Error during I2C_RDWR ioctl with error code: %d\n", ret);

	if (ret == 2)	// 2 msg success
		return count;
	else
		return -1;
}

static int gx_i2c_transaction_rx(void *dev, unsigned int reg_address, unsigned char *rx_data, unsigned int count)
{
	struct gx_i2c_device *i2c_device = (struct gx_i2c_device *)dev;
	unsigned char buf[4] = {0};
	int ret = 0;

	struct i2c_adapter *adap = i2c_device->adap;
	struct i2c_msg msgs[2];

	if (!i2c_device || !dev || !rx_data || !count) return -EINVAL;

	/* change endianness for ARM */
	if (g_bLittleEndian)
	{
		switch (i2c_device->address_width)
		{
			/* some chips needn't register address */
			case 0:
				break;

			case 1:
				buf[0] = (unsigned char)reg_address;
				break;

			case 2:
				buf[0] = (unsigned char)(reg_address>>8);
				buf[1] = (unsigned char)reg_address;
				break;

			case 4:
				buf[0] = (unsigned char)(reg_address>>24);
				buf[1] = (unsigned char)(reg_address>>16);
				buf[2] = (unsigned char)(reg_address>>8);
				buf[3] = (unsigned char)reg_address;
				break;

				/* invalid address width */
			default:
				return -EFAULT;
		}
	}
	else /* CK */
	{
		switch (i2c_device->address_width)
		{
			case 0:
				break;

			case 1:
				buf[0] = (unsigned char)reg_address;
				break;

			case 2:
				buf[0] = (unsigned char)reg_address;
				buf[1] = (unsigned char)(reg_address>>8);
				break;

			case 4:
				buf[0] = (unsigned char)reg_address;
				buf[1] = (unsigned char)(reg_address>>8);
				buf[2] = (unsigned char)(reg_address>>16);
				buf[3] = (unsigned char)(reg_address>>24);
				break;

				/* invalid address width */
			default:
				return -EFAULT;
		}
	}

	//I2C_DBG("\n\n\n--> %s: i2c device: %p\n", __func__, i2c_device);
	/* Send chip address. */
	msgs[0].len = i2c_device->address_width;
	msgs[0].addr = i2c_device->chip_address;
	msgs[0].buf = &buf[0];
	if (i2c_device->send_stop && i2c_device->address_width)
		msgs[0].flags = 0;
	else
		msgs[0].flags = I2C_M_REV_DIR_ADDR;

	if (!i2c_device->address_width)
		msgs[0].flags |= I2C_M_RD;

	/* Read data. */
	msgs[1].len = count;
	msgs[1].addr = i2c_device->chip_address;
	msgs[1].buf = (unsigned char *)rx_data;
	if (i2c_device->send_noack)
		msgs[1].flags = I2C_M_RD|I2C_M_NO_RD_ACK;
	else
		msgs[1].flags = I2C_M_RD;

	if (!i2c_device->address_width)
		msgs[1].flags |= I2C_M_NOSTART;

#ifdef GX_I2C_LOCK
	ret = adap->algo->master_xfer(adap,msgs,2);
#else
	ret = i2c_transfer(adap, msgs, 2);
#endif
	if (ret < 0)
		printk("RX: Error during I2C_RDWR ioctl with error code: %d\n", ret);

	if (ret == 2)	// 2 msg success
		return count;
	else
		return -1;
}

static int gx_i2c_closedev(void *dev)
{
	return gx_i2c_dev_unregister((struct gx_i2c_device *)dev);
}

//-----------Export functions-----------------
void *gx_i2c_open(unsigned int id)
{
	return (void *)gx_i2c_open_dev(id);
}

/* busid: 0: I2C1, 1: I2C2, 2: I2C3, 3: I2C4 ... */
/* chip_addr: device address, should be even */
/* address_width: device register address, 1 or 2 bytes, always */
/* send_noack: (rx) NOACK signal maybe necessary for all chips */
/* send_stop: (rx) means STOP is necessary or not before restart, it's optional, always */
int gx_i2c_set(void *dev, unsigned int busid, unsigned int chip_addr, unsigned int address_width, int send_noack, int send_stop)
{
	return gx_i2c_setdev(dev, busid, chip_addr, address_width, send_noack, send_stop);
}

int gx_i2c_tx(void *dev, unsigned int reg_address, const unsigned char *tx_data, unsigned int count)
{
	return gx_i2c_transaction_tx(dev, reg_address, tx_data, count);
}

int gx_i2c_rx(void *dev, unsigned int reg_address, unsigned char *rx_data, unsigned int count)
{
	return gx_i2c_transaction_rx(dev, reg_address, rx_data, count);
}

int gx_i2c_close(void *dev)
{
	return gx_i2c_closedev(dev);
}

int gx_i2c_transfer(void *dev, void *msgs, int num)
{
	struct gx_i2c_device *i2c = (struct gx_i2c_device *)dev;
	struct i2c_adapter *adap = i2c->adap;

	if (!i2c || !dev || !msgs ||!num) return -1;

	return i2c_transfer(adap, (struct i2c_msg *)msgs, num);
}

EXPORT_SYMBOL_GPL(gx_i2c_open);
EXPORT_SYMBOL_GPL(gx_i2c_set);
EXPORT_SYMBOL_GPL(gx_i2c_tx);
EXPORT_SYMBOL_GPL(gx_i2c_rx);
EXPORT_SYMBOL_GPL(gx_i2c_close);
EXPORT_SYMBOL_GPL(gx_i2c_transfer);

