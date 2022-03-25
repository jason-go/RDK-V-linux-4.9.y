#include "common_dev.h"
#include "gxse_core.h"

static GxSeDevice s_klm_dev[GXSE_MAX_KLM_COUNT] = {
	{.devname = "/dev/gxklm0"},  // GXSE_MOD_KLM_GENERIC = 0x10,
	{.devname = "/dev/gxklm1"},  // GXSE_MOD_KLM_IRDETO,
	{.devname = "/dev/gxklm2"},  // GXSE_MOD_KLM_SCPU_GENERIC,
	{.devname = "/dev/gxklm3"},  // GXSE_MOD_KLM_SCPU_IRDETO,
	{.devname = "/dev/gxklm4"},  // GXSE_MOD_KLM_SCPU_IRDETO_GENERIC,
};

static int32_t klm_open(struct inode *inode, struct file *filp)
{
	uint32_t id = 0;
	int32_t ret = GXSE_SUCCESS;
	GxSeDevice *dev = NULL;

	if ((id = iminor(inode)) > GXSE_MAX_KLM_COUNT) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL\n");
		return -ENODEV;
	}

	dev = &s_klm_dev[id];
	ret = gxse_device_open(dev);
	filp->private_data = dev;

	return ret;
}

static void *k_TD = NULL,  *u_TD = NULL;
static void *k_src = NULL, *u_src = NULL;
static void *k_dst = NULL, *u_dst = NULL;
static int32_t klm_copy_from_usr(void **param, void *arg, uint32_t size, uint32_t cmd)
{
    if (size) {
        if (gx_common_copy_from_usr(param, arg, size) < 0)
			goto err;
    }

	switch (cmd) {
	case TFM_KLM_SET_KN:
	case TFM_KLM_SET_CW:
		{
			GxTfmKlm *klm = (GxTfmKlm *)*param;

			if (size != sizeof(GxTfmKlm))
				goto err;

			u_src = klm->input.buf;
			CALL_COPY_FROM_USR(k_src, klm->input.buf, klm->input.length, err);
		}
		break;

	case TFM_KLM_GET_RESP:
		{
			GxTfmKlm *klm = (GxTfmKlm *)*param;

			if (size != sizeof(GxTfmKlm))
				goto err;

			u_src = klm->input.buf;
			u_dst = klm->output.buf;
			CALL_COPY_FROM_USR(k_src, klm->input.buf, klm->input.length, err);
			CALL_COPY_FROM_USR(k_dst, klm->output.buf, klm->output.length, err);
		}
		break;

	case TFM_KLM_UPDATE_TD_PARAM:
		{
			GxTfmKlm *klm = (GxTfmKlm *)*param;

			if (size != sizeof(GxTfmKlm))
				goto err;

			u_TD = klm->TD.buf;
			u_src = klm->input.buf;
			CALL_COPY_FROM_USR(k_TD , klm->TD.buf, klm->TD.length, err);
			CALL_COPY_FROM_USR(k_src, klm->input.buf, klm->input.length, err);
		}
		break;

	default:
		break;
	}

	return GXSE_SUCCESS;

err:
	return GXSE_ERR_GENERIC;
}

static int32_t klm_copy_to_usr(void *arg, void **param, uint32_t size, uint32_t cmd)
{

	switch (cmd) {
	case TFM_KLM_SET_KN:
	case TFM_KLM_SET_CW:
		{
			GxTfmKlm *klm = (GxTfmKlm *)*param;
			klm->input.buf = u_src;
		}
		break;

	case TFM_KLM_GET_RESP:
		{
			GxTfmKlm *klm = (GxTfmKlm *)*param;
			klm->output.buf = u_dst;
			CALL_COPY_TO_USR(klm->output.buf, k_dst, klm->output.length, out);
		}
		break;

	case TFM_KLM_UPDATE_TD_PARAM:
		{
			GxTfmKlm *klm = (GxTfmKlm *)*param;
			klm->TD.buf = u_TD;
			klm->input.buf = u_src;
		}
		break;

	default:
		break;
	}

	if (size)
		gx_common_copy_to_usr(arg, param, size);

out:
	if (k_TD) {
		gx_free(k_TD);
		k_TD = NULL;
		u_TD = NULL;
	}

	if (k_src) {
		gx_free(k_src);
		k_src = NULL;
		u_src = NULL;
	}

	if (k_dst) {
		gx_free(k_dst);
		k_dst = NULL;
		u_dst = NULL;
	}

	if (*param)
		gx_free(*param);

	return GXSE_SUCCESS;
}

static struct file_operations s_klm_fops = {
	.owner          = THIS_MODULE,
	.open           = klm_open,
	.release        = gxse_os_dev_close,
	.unlocked_ioctl = gxse_os_dev_ioctl,
	.mmap           = gxse_os_dev_mmap,
};

static GxDeviceDesc s_klm_desc = {
	.ch_name      = "klm_ch",
	.class_name   = "klm_class",

	.major        = KLM_MAJOR,
	.dev_count    = GXSE_MAX_KLM_COUNT,
	.sub_count    = {GXSE_MAX_KLM_COUNT, -1},
	.dev_name     = {"gxklm"},
	.dev_name_len = {5},

	.fops         = &s_klm_fops,
};

int klm_probe(void)
{
	GxSeDevice *dev = NULL;
	int i = 0, begin = GXSE_MOD_KLM_GENERIC, end = GXSE_MOD_KLM_MAX;

	if (gxse_os_device_create(&s_klm_desc) < 0)
		return -1;

	for (i = begin; i < end; i++) {
		dev = &s_klm_dev[GXSE_KLM_ID(i)];
		if (gxse_device_init(dev) == 0) {
			mod_ops(dev->module)->copy_from_usr = klm_copy_from_usr;
			mod_ops(dev->module)->copy_to_usr = klm_copy_to_usr;
		}
	}
	return 0;
}

int klm_remove(void)
{
	GxSeDevice *dev = NULL;
	int i = 0, begin = GXSE_MOD_KLM_GENERIC, end = GXSE_MOD_KLM_MAX;

	for (i = begin; i < end; i++) {
		dev = &s_klm_dev[GXSE_KLM_ID(i)];
		gxse_device_deinit(dev);
	}

	gxse_os_device_destroy(&s_klm_desc);
	return 0;
}

