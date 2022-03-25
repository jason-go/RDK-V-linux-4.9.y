#include "common_dev.h"
#include "gxse_core.h"

GxSeDevice s_misc_dev[GXSE_MAX_MISC_COUNT] = {
	{.devname = "/dev/gxmisc0"},    // GXSE_MOD_MISC_CHIP_CFG
	{.devname = "/dev/gxmisc1"},    // GXSE_MOD_MISC_OTP
	{.devname = "/dev/gxmisc2"},    // GXSE_MOD_MISC_RNG
	{.devname = "/dev/gxmisc3"},    // GXSE_MOD_MISC_DGST
	{.devname = "/dev/gxmisc4"},    // GXSE_MOD_MISC_AKCIPHER
	{.devname = "/dev/gxmisc5"},    // GXSE_MOD_MISC_SCI
};

static int32_t misc_open(struct inode *inode, struct file *filp)
{
	uint32_t id = 0;
	int32_t ret = GXSE_SUCCESS;
	GxSeDevice *dev = NULL;

	if ((id = iminor(inode)) > GXSE_MAX_MISC_COUNT) {
		gxlog_e(GXSE_LOG_MOD_DEV, "Device is NULL\n");
		return -ENODEV;
	}

	dev = &s_misc_dev[id];
	ret = gxse_device_open(dev);
	filp->private_data = dev;

	return ret;
}

static void *k_buf = NULL, *u_buf = NULL;
static int32_t misc_otp_copy_from_usr(void *param, void *arg, uint32_t size, uint32_t cmd)
{
 	switch (cmd) {
	case SECURE_OTP_WRITE:
	case SECURE_OTP_READ:
		{
			GxSecureOtpBuf *ctrl = (GxSecureOtpBuf *)param;

			if (size != sizeof(GxSecureOtpBuf))
				goto err;

			u_buf = ctrl->buf;
			CALL_COPY_FROM_USR(k_buf, ctrl->buf, ctrl->size, err);
		}
		break;

	default:
		break;
	}

	return GXSE_SUCCESS;
err:
	return GXSE_ERR_GENERIC;
}

static int32_t misc_otp_copy_to_usr(void *arg, void *param, uint32_t size, uint32_t cmd)
{
	switch (cmd) {
	case SECURE_OTP_WRITE:
	case SECURE_OTP_READ:
		{
			GxSecureOtpBuf *ctrl = (GxSecureOtpBuf *)param;

			ctrl->buf = u_buf;
			if (cmd == SECURE_OTP_READ) {
				CALL_COPY_TO_USR(ctrl->buf, k_buf, ctrl->size, out);
			}
		}
		break;

	default:
		break;
	}

out:
	if (k_buf) {
		gx_free(k_buf);
		k_buf = NULL;
		u_buf = NULL;
	}

	return GXSE_SUCCESS;
}

static void *k_src = NULL, *u_src = NULL;
static void *k_dst = NULL, *u_dst = NULL;
static void *k_pubk = NULL, *u_pubk = NULL;
static void *k_mack = NULL, *u_mack = NULL;
static int32_t misc_dgst_copy_from_usr(void *param, void *arg, uint32_t size, uint32_t cmd)
{
   	switch (cmd) {
	case TFM_DGST:
		{
			GxTfmDgst *ctrl = (GxTfmDgst *)param;

			if (size != sizeof(GxTfmDgst))
				goto err;

			u_src = ctrl->input.buf;
			CALL_COPY_FROM_USR(k_src , ctrl->input.buf, ctrl->input.length, err);

			u_dst = ctrl->output.buf;
			CALL_COPY_FROM_USR(k_dst , ctrl->output.buf, ctrl->output.length, err);

			if (ctrl->alg == TFM_ALG_SM2_ZA) {
				u_pubk = ctrl->pub_key.buf;
				CALL_COPY_FROM_USR(k_pubk, ctrl->pub_key.buf, ctrl->pub_key.length, err);

			} else if (ctrl->alg >= TFM_ALG_SM3_HMAC) {
				u_mack = ctrl->hmac_key.buf;
				CALL_COPY_FROM_USR(k_mack, ctrl->hmac_key.buf, ctrl->hmac_key.length, err);
			}
		}
		break;

	default:
		break;
	}

	return GXSE_SUCCESS;
err:
	return GXSE_ERR_GENERIC;
}

static int32_t misc_dgst_copy_to_usr(void *arg, void *param, uint32_t size, uint32_t cmd)
{
	switch (cmd) {

	case TFM_DGST:
		{
			GxTfmDgst *ctrl = (GxTfmDgst *)param;

			ctrl->input.buf = u_src;
			ctrl->output.buf = u_dst;

			if (ctrl->alg == TFM_ALG_SM2_ZA)
				ctrl->pub_key.buf = u_pubk;
			else if (ctrl->alg >= TFM_ALG_SM3_HMAC)
				ctrl->hmac_key.buf = u_mack;

			CALL_COPY_TO_USR(ctrl->output.buf, k_dst, ctrl->output.length, out);
		}
		break;

	default:
		break;
	}

out:
	if (k_pubk) {
		gx_free(k_pubk);
		k_pubk = NULL;
		u_pubk = NULL;
	}

	if (k_mack) {
		gx_free(k_mack);
		k_mack = NULL;
		u_mack = NULL;
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

	return GXSE_SUCCESS;
}

static void *k_vhash = NULL, *u_vhash = NULL;
static void *k_vpubk = NULL, *u_vpubk = NULL;
static void *k_vsig  = NULL, *u_vsig  = NULL;
static void *k_vsrc = NULL, *u_vsrc = NULL;
static void *k_vdst = NULL, *u_vdst = NULL;
static int32_t misc_akcipher_copy_from_usr(void *param, void *arg, uint32_t size, uint32_t cmd)
{
	switch (cmd) {
	case TFM_ENCRYPT:
	case TFM_DECRYPT:
		{
			GxTfmCrypto *ctrl = (GxTfmCrypto *)param;

			if (size != sizeof(GxTfmCrypto))
				goto err;

			u_vsrc = ctrl->input.buf;
			CALL_COPY_FROM_USR(k_vsrc, ctrl->input.buf, ctrl->input.length, err);

			u_vdst = ctrl->output.buf;
			CALL_COPY_FROM_USR(k_vdst , ctrl->output.buf, ctrl->output.length, err);

			if (cmd == TFM_ENCRYPT) {
				u_vpubk = ctrl->sm2_pub_key.buf;
				CALL_COPY_FROM_USR(k_vpubk, ctrl->sm2_pub_key.buf, ctrl->sm2_pub_key.length, err);
			}
		}
		break;

	case TFM_VERIFY:
		{
			GxTfmVerify *ctrl = (GxTfmVerify *)param;

			if (size != sizeof(GxTfmVerify))
				goto err;

			u_vhash = ctrl->hash.buf;
			CALL_COPY_FROM_USR(k_vhash, ctrl->hash.buf, ctrl->hash.length, err);

			u_vpubk = ctrl->pub_key.buf;
			CALL_COPY_FROM_USR(k_vpubk, ctrl->pub_key.buf, ctrl->pub_key.length, err);

			u_vsig = ctrl->signature.buf;
			CALL_COPY_FROM_USR(k_vsig , ctrl->signature.buf, ctrl->signature.length, err);
		}
		break;

	default:
		break;
	}

	return GXSE_SUCCESS;
err:
	return GXSE_ERR_GENERIC;
}

static int32_t misc_akcipher_copy_to_usr(void *arg, void *param, uint32_t size, uint32_t cmd)
{
	switch (cmd) {
	case TFM_ENCRYPT:
	case TFM_DECRYPT:
		{
			GxTfmCrypto *ctrl = (GxTfmCrypto *)param;

			ctrl->input.buf = u_vsrc;

			ctrl->output.buf = u_vdst;
			CALL_COPY_TO_USR(ctrl->output.buf, k_vdst, ctrl->output.length, out);

			if (cmd == TFM_ENCRYPT) {
				ctrl->sm2_pub_key.buf = u_vpubk;
			}
		}
		break;

	case TFM_VERIFY:
		{
			GxTfmVerify *ctrl = (GxTfmVerify *)param;

			ctrl->hash.buf = u_vhash;
			ctrl->pub_key.buf = u_vpubk;
			ctrl->signature.buf = u_vsig;
		}
		break;

	default:
		break;
	}

out:
	if (k_vhash) {
		gx_free(k_vhash);
		k_vhash = NULL;
		u_vhash = NULL;
	}

	if (k_vpubk) {
		gx_free(k_vpubk);
		k_vpubk = NULL;
		u_vpubk = NULL;
	}

	if (k_vsig) {
		gx_free(k_vsig);
		k_vsig = NULL;
		u_vsig = NULL;
	}

	if (k_vsrc) {
		gx_free(k_vsrc);
		k_vsrc = NULL;
		u_vsrc = NULL;
	}

	if (k_vdst) {
		gx_free(k_vdst);
		k_vdst = NULL;
		u_vdst = NULL;
	}

	return GXSE_SUCCESS;
}

int32_t misc_copy_from_usr(void **param, void *arg, uint32_t size, uint32_t cmd)
{
	if (size) {
		if (gx_common_copy_from_usr(param, arg, size) < 0)
			return GXSE_ERR_GENERIC;
	}

	if (misc_otp_copy_from_usr(*param, arg, size, cmd) < 0)
		return GXSE_ERR_GENERIC;

	if (misc_dgst_copy_from_usr(*param, arg, size, cmd) < 0)
		return GXSE_ERR_GENERIC;

	if (misc_akcipher_copy_from_usr(*param, arg, size, cmd) < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int32_t misc_copy_to_usr(void *arg, void **param, uint32_t size, uint32_t cmd)
{
	if (misc_otp_copy_to_usr(arg, *param, size, cmd) < 0)
		return GXSE_ERR_GENERIC;

	if (misc_dgst_copy_to_usr(arg, *param, size, cmd) < 0)
		return GXSE_ERR_GENERIC;

	if (misc_akcipher_copy_to_usr(arg, *param, size, cmd) < 0)
		return GXSE_ERR_GENERIC;

    if (size) {
        gx_common_copy_to_usr(arg, param, size);
	}

	if (*param)
		gx_free(*param);

	return GXSE_SUCCESS;
}

static struct file_operations s_misc_fops = {
	.owner          = THIS_MODULE,
	.open           = misc_open,
	.release        = gxse_os_dev_close,
	.read           = gxse_os_dev_read,
	.write          = gxse_os_dev_write,
	.poll           = gxse_os_dev_poll,
	.unlocked_ioctl = gxse_os_dev_ioctl,
};

static GxDeviceDesc s_misc_desc = {
	.ch_name      = "misc_ch",
	.class_name   = "misc_class",

	.major        = MISC_MAJOR,
	.dev_count    = GXSE_MAX_MISC_COUNT,
	.sub_count    = {GXSE_MAX_MISC_COUNT, -1},
	.dev_name     = {"gxmisc"},
	.dev_name_len = {6},

	.fops         = &s_misc_fops,
};

int misc_probe(void)
{
	GxSeDevice *dev = NULL;
	int i = 0, begin = GXSE_MOD_MISC_CHIP_CFG, end = GXSE_MOD_MISC_MAX;

	if (gxse_os_device_create(&s_misc_desc) < 0)
		return -1;

	for (i = begin; i < end; i++) {
		dev = &s_misc_dev[GXSE_MISC_ID(i)];
		if (gxse_device_init(dev) == 0) {
			mod_ops(dev->module)->copy_from_usr = misc_copy_from_usr;
			mod_ops(dev->module)->copy_to_usr = misc_copy_to_usr;
		}
	}
	return 0;
}

int misc_remove(void)
{
	GxSeDevice *dev = NULL;
	int i = 0, begin = GXSE_MOD_MISC_CHIP_CFG, end = GXSE_MOD_MISC_MAX;

	for (i = begin; i < end; i++) {
		dev = &s_misc_dev[GXSE_MISC_ID(i)];
		gxse_device_deinit(dev);
	}

	gxse_os_device_destroy(&s_misc_desc);
	return 0;
}

