#include "common_dev.h"
#include "gxse_core.h"

static GxSeDevice s_firewall_dev = { .devname = "/dev/gxfirewall" };

static int32_t firewall_open(struct inode *inode, struct file *filp)
{
	uint32_t id = 0;
	int32_t ret = GXSE_SUCCESS;
	GxSeDevice *dev = NULL;

	if ((id = iminor(inode)) > GXSE_MAX_FIREWALL_COUNT) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL\n");
		return -ENODEV;
	}

	dev = &s_firewall_dev;
	ret = gxse_device_open(dev);
	filp->private_data = dev;

	return ret;
}

static struct file_operations s_firewall_fops = {
	.owner          = THIS_MODULE,
	.open           = firewall_open,
	.release        = gxse_os_dev_close,
	.unlocked_ioctl = gxse_os_dev_ioctl,
	.mmap           = gxse_os_dev_mmap,
};

static GxDeviceDesc s_firewall_desc = {
	.ch_name      = "firewall_ch",
	.class_name   = "firewall_class",

	.major        = FIREWALL_MAJOR,
	.dev_count    = GXSE_MAX_FIREWALL_COUNT,
	.sub_count    = {1, -1},
	.dev_name     = {"gxfirewall"},
	.dev_name_len = {10},

	.fops         = &s_firewall_fops,
};

extern int32_t misc_copy_to_usr(void *arg, void **param, uint32_t size, uint32_t cmd);
extern int32_t misc_copy_from_usr(void **param, void *arg, uint32_t size, uint32_t cmd);

int firewall_probe(void)
{
	GxSeDevice *dev = NULL;

	if (gxse_os_device_create(&s_firewall_desc) < 0)
		return -1;

	dev = &s_firewall_dev;
	if (gxse_device_init(dev) == 0) {
		mod_ops(dev->module)->copy_from_usr = misc_copy_from_usr;
		mod_ops(dev->module)->copy_to_usr = misc_copy_to_usr;
	}

	return 0;
}

int firewall_remove(void)
{
	GxSeDevice *dev = NULL;

	dev = &s_firewall_dev;
	gxse_device_deinit(dev);

	gxse_os_device_destroy(&s_firewall_desc);
	return 0;
}

