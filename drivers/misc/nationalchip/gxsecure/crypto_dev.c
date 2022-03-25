#include "common_dev.h"
#include "gxse_core.h"

static GxSeDevice s_crypto_dev[GXSE_MAX_CRYPTO_COUNT] = {
	{.devname = "/dev/gxcrypto0"},      // GXSE_MOD_CRYPTO_FIFO = 0x0,
	{.devname = "/dev/gxcrypto1/ch0"},  // GXSE_MOD_CRYPTO_DMA0,
	{.devname = "/dev/gxcrypto1/ch1"},  // GXSE_MOD_CRYPTO_DMA1,
	{.devname = "/dev/gxcrypto1/ch2"},  // GXSE_MOD_CRYPTO_DMA2,
	{.devname = "/dev/gxcrypto1/ch3"},  // GXSE_MOD_CRYPTO_DMA3,
	{.devname = "/dev/gxcrypto1/ch4"},  // GXSE_MOD_CRYPTO_DMA4,
	{.devname = "/dev/gxcrypto1/ch5"},  // GXSE_MOD_CRYPTO_DMA5,
	{.devname = "/dev/gxcrypto1/ch6"},  // GXSE_MOD_CRYPTO_DMA6,
	{.devname = "/dev/gxcrypto1/ch7"},  // GXSE_MOD_CRYPTO_DMA7,
};

static int32_t crypto_open(struct inode *inode, struct file *filp)
{
	uint32_t id = 0;
	int32_t ret = GXSE_SUCCESS;
	GxSeDevice *dev = NULL;

	if ((id = iminor(inode)) > GXSE_MAX_CRYPTO_COUNT) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL\n");
		return -ENODEV;
	}

	dev = &s_crypto_dev[id];
	ret = gxse_device_open(dev);
	filp->private_data = dev;

	return ret;
}

static int32_t crypto_copy_from_usr(void **param, void *arg, uint32_t size, uint32_t cmd)
{
	if (size) {
		if (gx_common_copy_from_usr(param, arg, size) < 0)
			return GXSE_ERR_GENERIC;
	}

	return GXSE_SUCCESS;
}

static int32_t crypto_copy_to_usr(void *arg, void **param, uint32_t size, uint32_t cmd)
{
	if (size)
		gx_common_copy_to_usr(arg, param, size);

	if (*param)
		gx_free(*param);

	return GXSE_SUCCESS;
}

static struct file_operations s_crypto_fops = {
	.owner          = THIS_MODULE,
	.open           = crypto_open,
	.release        = gxse_os_dev_close,
	.unlocked_ioctl = gxse_os_dev_ioctl,
	.mmap           = gxse_os_dev_mmap,
};

static GxDeviceDesc s_crypto_desc = {
	.ch_name      = "crypto_ch",
	.class_name   = "crypto_class",

	.major        = CRYPTO_MAJOR,
	.dev_count    = 9,
	.sub_count    = {1, 8, -1},
	.dev_name     = {"gxcrypto0", "gxcrypto1/ch"},
	.dev_name_len = {9, 12},
	.fops         = &s_crypto_fops,
};

int crypto_probe(void)
{
	GxSeDevice *dev = NULL;
	int i = 0, begin = GXSE_MOD_CRYPTO_FIFO, end = GXSE_MOD_CRYPTO_DMA7;

	if (gxse_os_device_create(&s_crypto_desc) < 0)
		return -1;

	for (i = begin; i <= end; i++) {
		dev = &s_crypto_dev[GXSE_CRYPTO_ID(i)];
		if (gxse_device_init(dev) == 0) {
			mod_ops(dev->module)->copy_from_usr = crypto_copy_from_usr;
			mod_ops(dev->module)->copy_to_usr = crypto_copy_to_usr;
		}
	}

	return 0;
}

int crypto_remove(void)
{
	GxSeDevice *dev = NULL;
	int i = 0, begin = GXSE_MOD_CRYPTO_FIFO, end = GXSE_MOD_CRYPTO_DMA7;

	for (i = begin; i <= end; i++) {
		dev = &s_crypto_dev[GXSE_CRYPTO_ID(i)];
		gxse_device_deinit(dev);
	}

	gxse_os_device_destroy(&s_crypto_desc);
	return 0;
}

