#include "common_dev.h"
#include "gxse_core.h"

static GxDeviceDesc s_secure_desc;
static GxSeDevice s_secure_dev[GXSE_MAX_SECURE_COUNT] = {
	{.devname = "/dev/gxsecure0"},  // GXSE_MOD_SECURE_MBOX = 0x60,
	{.devname = "/dev/gxsecure1"},  // GXSE_MOD_SECURE_MBOX_TEE,
};

static int32_t secure_open(struct inode *inode, struct file *filp)
{
	uint32_t id = 0;
	int32_t ret = GXSE_SUCCESS;
	GxSeDevice *dev = NULL;

	if ((id = iminor(inode)) > GXSE_MAX_SECURE_COUNT) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL\n");
		return -ENODEV;
	}

	dev = &s_secure_dev[id];
	ret = gxse_device_open(dev);
	filp->private_data = dev;

	return ret;
}

#define GXSCPU_FW_NAME "gxsecure.fw"
static struct firmware *fw_entry = NULL;
static void *k_buf = NULL, *u_buf = NULL;
static int32_t secure_copy_from_usr(void **param, void *arg, uint32_t size, uint32_t cmd)
{
    if (size) {
        if (gx_common_copy_from_usr(param, arg, size) < 0)
			goto err;
    }

	switch (cmd) {
	case SECURE_SEND_LOADER:
		{
			GxSecureLoader *loader = (GxSecureLoader *)*param;

			if (size != sizeof(GxSecureLoader))
				goto err;

			if (loader->loader && loader->size) {
				u_buf = loader->loader;
				CALL_COPY_FROM_USR(k_buf, loader->loader, loader->size, err);

			} else {
				if (request_firmware((void *)&fw_entry, GXSCPU_FW_NAME, s_secure_desc.dev)) {
					gxlog_e(GXSE_LOG_MOD_DEV, "request firmware failed.\n");
					goto err;
				}
				u_buf = loader->loader;
				loader->loader = (unsigned char *)fw_entry->data;
				loader->size   = fw_entry->size;
			}
		}
		break;

	case GXSE_CMD_TX:
	case GXSE_CMD_RX:
		{
			GxSecureUserData *p = (GxSecureUserData *)*param;

			if (size != sizeof(GxSecureUserData))
				goto err;

			u_buf = p->buf;
			CALL_COPY_FROM_USR(k_buf, p->buf, p->size, err);
		}
		break;

	default:
		break;
	}

	return GXSE_SUCCESS;

err:
	if (k_buf) {
		gx_free(k_buf);
		k_buf = NULL;
		u_buf = NULL;
	}

	return GXSE_ERR_GENERIC;
}

static int32_t secure_copy_to_usr(void *arg, void **param, uint32_t size, uint32_t cmd)
{
	switch (cmd) {
	case SECURE_SEND_LOADER:
		{
			GxSecureLoader *loader = (GxSecureLoader *)*param;

			loader->loader = u_buf;
		}
		break;

	case GXSE_CMD_TX:
	case GXSE_CMD_RX:
		{
			GxSecureUserData *p = (GxSecureUserData *)*param;

			p->buf = u_buf;
			if (cmd == GXSE_CMD_RX) {
				CALL_COPY_TO_USR(p->buf, k_buf, p->size, out);
			}
		}
		break;

	default:
		break;
	}

	if (size)
		gx_common_copy_to_usr(arg, param, size);

out:
	if (k_buf) {
		gx_free(k_buf);
		k_buf = NULL;
		u_buf = NULL;
	}

	if (fw_entry) {
		release_firmware(fw_entry);
		fw_entry = NULL;
	}

	if (*param)
		gx_free(*param);

	return GXSE_SUCCESS;
}

static struct file_operations s_secure_fops = {
	.owner          = THIS_MODULE,
	.open           = secure_open,
	.release        = gxse_os_dev_close,
	.read           = gxse_os_dev_read,
	.write          = gxse_os_dev_write,
	.unlocked_ioctl = gxse_os_dev_ioctl,
};

static GxDeviceDesc s_secure_desc = {
	.ch_name      = "secure_ch",
	.class_name   = "secure_class",

	.major        = SECURE_MAJOR,
	.dev_count    = GXSE_MAX_SECURE_COUNT,
	.sub_count    = {2, -1},
	.dev_name     = {"gxsecure"},
	.dev_name_len = {8},

	.fops         = &s_secure_fops,
};

int secure_probe(void)
{
	GxSeDevice *dev = NULL;
	int i = 0, begin = GXSE_MOD_SECURE_MBOX, end = GXSE_MOD_SECURE_MBOX+1;

#ifdef CFG_GXSE_FIRMWARE_MBOX_TEE
	end = GXSE_MOD_SECURE_MBOX_MAX;
#endif

	if (gxse_os_device_create(&s_secure_desc) < 0)
		return -1;

	for (i = begin; i < end; i++) {
		dev = &s_secure_dev[GXSE_SECURE_ID(i)];
		if (gxse_device_init(dev) == 0) {
			mod_ops(dev->module)->copy_from_usr = secure_copy_from_usr;
			mod_ops(dev->module)->copy_to_usr = secure_copy_to_usr;
		}
	}
	return 0;
}

int secure_remove(void)
{
	GxSeDevice *dev = NULL;
	int i = 0, begin = GXSE_MOD_SECURE_MBOX, end = GXSE_MOD_SECURE_MBOX+1;

#ifdef CFG_GXSE_FIRMWARE_MBOX_TEE
	end = GXSE_MOD_SECURE_MBOX_MAX;
#endif

	for (i = begin; i < end; i++) {
		dev = &s_secure_dev[GXSE_SECURE_ID(i)];
		gxse_device_deinit(dev);
	}

	gxse_os_device_destroy(&s_secure_desc);
	return 0;
}

