#include "gxse_core.h"
#include "gxakcipher_virdev.h"
#include "gx_soft_sha.h"
#include "gx_soft_sm3.h"
#include "gx_soft_sm2.h"

static int32_t gx_akcipher_ioctl(GxSeModule *module, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleHwObjTfmOps *tfm_ops = NULL;

	if (gxse_module_ioctl_check(module, GXSE_HWOBJ_TYPE_TFM, cmd, size) < 0)
		return GXSE_ERR_GENERIC;

	obj = module->hwobj;
	tfm_ops = (GxSeModuleHwObjTfmOps *)obj->ops;
	gx_mutex_lock(obj->mutex);
	switch (cmd) {
	case TFM_ENCRYPT:
	case TFM_DECRYPT:
		if (size != sizeof(GxTfmCrypto)) {
			ret = GXSE_ERR_GENERIC;
			goto out;
		}

		if (cmd == TFM_ENCRYPT) {
			if (tfm_ops->tfm_akcipher.encrypt)
				ret = tfm_ops->tfm_akcipher.encrypt(obj, param);

		} else {
			if (tfm_ops->tfm_akcipher.decrypt)
				ret = tfm_ops->tfm_akcipher.decrypt(obj, param);
		}
		break;

	case TFM_VERIFY:
		if (size != sizeof(GxTfmVerify)) {
			ret = GXSE_ERR_GENERIC;
			goto out;
		}

		if (tfm_ops->tfm_akcipher.verify)
			ret = tfm_ops->tfm_akcipher.verify(obj, param);
		break;

	case TFM_DGST:
		if (size != sizeof(GxTfmDgst)) {
			ret = GXSE_ERR_GENERIC;
			goto out;
		}

		if (tfm_ops->tfm_akcipher.cal_za)
			ret = tfm_ops->tfm_akcipher.cal_za(obj, param);
		break;

	case TFM_CAPABILITY:
		if (size != sizeof(GxTfmCap)) {
			ret = GXSE_ERR_GENERIC;
			goto out;
		}

		if (tfm_ops->tfm_akcipher.capability)
			ret = tfm_ops->tfm_akcipher.capability(obj, param);
		break;

	default:
		break;
	}

out:
	gx_mutex_unlock(obj->mutex);
	return ret;
}

int32_t gx_tfm_object_akcipher_encrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxAkcipherOps *ops = (GxAkcipherOps *)GXSE_OBJ_HWOPS(obj);
	uint8_t *temp_a = NULL, *temp_b = NULL, *temp_p = NULL, *temp_xG = NULL, *temp_yG = NULL;
	uint8_t temp_xP[72] = {0}, temp_yP[72] = {0};

	unsigned int test_k[] = {
		0x49dd7b4f, 0x18e5388d, 0x5546d490, 0x8afa1742,
		0x3d957514, 0x5b92fd6c, 0x6ecfc2b9, 0x4c62eefd,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8,
	};

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_ENCRYPT)) < 0)
			return ret;
	}

	if (ops->get_curve_param)
		ops->get_curve_param(obj->reg, obj->priv, NULL, &temp_p, &temp_a, &temp_b, &temp_xG, &temp_yG);

	gx_buf_reverse(param->sm2_pub_key.buf, 32, temp_xP);
	gx_buf_reverse(param->sm2_pub_key.buf+32, 32, temp_yP);
	memcpy(temp_xP+68, test_k+17, 4);
	memcpy(temp_yP+68, test_k+17, 4);

	gx_soft_sm2_encrypt((void *)temp_p, (void *)temp_a, (void *)temp_b, (void *)test_k,
			(void *)temp_xG, (void *)temp_yG, (void *)temp_xP, (void *)temp_yP,
			param->input.buf, param->input.length, param->output.buf);

	return GXSE_SUCCESS;
}

int32_t gx_tfm_object_akcipher_decrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxAkcipherOps *ops = (GxAkcipherOps *)GXSE_OBJ_HWOPS(obj);
	uint8_t *temp_a = NULL, *temp_b = NULL, *temp_p = NULL, *cipher = NULL;

	unsigned int test_private_key[] = {
		0x20bb0da0, 0x08ddbc29, 0xb89463f2, 0x34aa7f7c,
		0x3fbf3535, 0x5e2efe28, 0xa00637bd, 0x1649ab77,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8,
	};

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_DECRYPT)) < 0)
			return ret;
	}

	if (NULL == (cipher = gx_malloc(param->input.length+1))) {
		gxlog_e(GXSE_LOG_MOD_AKCIPHER, "alloc cipher failed, size = 0x%x\n", param->input.length+1);
		return GXSE_ERR_GENERIC;
	}

	if (ops->get_curve_param)
		ops->get_curve_param(obj->reg, obj->priv, NULL, &temp_p, &temp_a, &temp_b, NULL, NULL);

	memcpy(cipher+1, param->input.buf, param->input.length);
	cipher[0] = 0x04;

	ret = gx_soft_sm2_decrypt((void *)temp_p, (void *)temp_a, (void *)temp_b, test_private_key,
			param->input.length-96, cipher, param->output.buf);

	if (cipher)
		gx_free(cipher);

	return GXSE_SUCCESS;
}

int32_t gx_tfm_object_akcipher_verify(GxSeModuleHwObj *obj, GxTfmVerify *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxAkcipherOps *ops = (GxAkcipherOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_VERIFY)) < 0)
			return ret;
	}

	if (ops->do_verify)
		ret = ops->do_verify(obj->reg, obj->priv, param->pub_key.buf, param->pub_key.length,
				param->hash.buf, param->hash.length,
				param->signature.buf, param->signature.length);

	return ret;
}

int32_t gx_tfm_object_akcipher_cal_za(GxSeModuleHwObj *obj, GxTfmDgst *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxAkcipherOps *ops = (GxAkcipherOps *)GXSE_OBJ_HWOPS(obj);
	uint32_t ida_len_in_bit = 0, ida_len = 0, total_len = 0;
	uint8_t *temp_a = NULL, *temp_b = NULL, *temp_xG = NULL, *temp_yG = NULL, *za_input = NULL;

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_DGST)) < 0)
			return ret;
	}

	ida_len = param->input.length;
	ida_len_in_bit = ida_len * 8;
	total_len = 2 + ida_len + 6*32;
	if (NULL == (za_input = gx_malloc(total_len))) {
		gxlog_e(GXSE_LOG_MOD_AKCIPHER, "alloc za_input buffer failed, size = 0x%x\n", total_len);
		return GXSE_ERR_GENERIC;
	}

	za_input[0] = (ida_len_in_bit&0xffff) >> 8;
	za_input[1] = ida_len_in_bit&0xff;

	if (ops->get_curve_param)
		ops->get_curve_param(obj->reg, obj->priv, NULL, NULL, &temp_a, &temp_b, &temp_xG, &temp_yG);

	gx_buf_reverse(temp_a , 32, temp_a);
	gx_buf_reverse(temp_b , 32, temp_b);
	gx_buf_reverse(temp_xG, 32, temp_xG);
	gx_buf_reverse(temp_yG, 32, temp_yG);

	memcpy(za_input+2, param->input.buf, param->input.length);
	memcpy(za_input+2+ida_len    , temp_a, 32);
	memcpy(za_input+2+ida_len+32 , temp_b, 32);
	memcpy(za_input+2+ida_len+64 , temp_xG, 32);
	memcpy(za_input+2+ida_len+96 , temp_yG, 32);
	memcpy(za_input+2+ida_len+128, param->pub_key.buf, 64);

	ret = gx_sm3_perform(za_input, total_len, param->output.buf, 0);
	if (za_input)
		gx_free(za_input);

	return ret;
}

int32_t gx_tfm_object_akcipher_capability(GxSeModuleHwObj *obj, GxTfmCap *param)
{
	GxAkcipherOps *ops = (GxAkcipherOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->capability)
		ops->capability(obj->reg, obj->priv, param);

	return GXSE_SUCCESS;
}

GxSeModuleOps akcipher_dev_ops = {
	.init   = gxse_module_init,
	.deinit = gxse_module_deinit,
	.open   = gxse_module_open,
	.close  = gxse_module_close,
	.isr    = gxse_module_isr,
	.ioctl  = gx_akcipher_ioctl,
};

