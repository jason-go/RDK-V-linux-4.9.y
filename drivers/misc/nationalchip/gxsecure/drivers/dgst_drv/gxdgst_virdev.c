#include "gxse_core.h"
#include "gxdgst_virdev.h"
#include "gx_soft_sha.h"
#include "gx_soft_sm3.h"
#include "gx_soft_sm2.h"

static int32_t gx_dgst_ioctl(GxSeModule *module, uint32_t cmd, void *param, uint32_t size)
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
	case TFM_DGST:
		{
			GxTfmDgst *dgst = (GxTfmDgst *)param;
			if (dgst->alg >= TFM_ALG_SM3_HMAC) {
				if (tfm_ops->tfm_dgst.dgst_hmac)
					ret = tfm_ops->tfm_dgst.dgst_hmac(obj, param);

			} else {
				if (tfm_ops->tfm_dgst.dgst)
					ret = tfm_ops->tfm_dgst.dgst(obj, param);
			}
		}
		break;

	case TFM_CAPABILITY:
		if (tfm_ops->tfm_dgst.capability)
			ret = tfm_ops->tfm_dgst.capability(obj, param);
		break;

	default:
		break;
	}

	gx_mutex_unlock(obj->mutex);
	return ret;
}

static int32_t _tfm_dgst_fifo(GxSeModuleHwObj *obj, GxTfmDgst *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxDgstOps *ops = (GxDgstOps *)GXSE_OBJ_HWOPS(obj);
	uint32_t final_val = 0, pos = 0, left = 0;
	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);

	if (ops->set_work_mode)
		ops->set_work_mode(obj->reg, obj->priv, 0);

	if (ops->set_alg)
		ops->set_alg(obj->reg, obj->priv, param->alg);

	if (ops->set_endian)
		ops->set_endian(obj->reg, obj->priv, endian_flag);

	if (ops->fifo_put)
		ops->fifo_put(obj->reg, obj->priv, param->input.buf, param->input.length);

	pos  = param->input.length/4*4;
	left = param->input.length%4;
	if (left) {
		memcpy(&final_val, param->input.buf+pos, left);
		final_val |= left<<28;
	}

	if (ops->set_ready)
		ops->set_ready(obj->reg, obj->priv, final_val);

	if (ops->wait_finish) {
		if ((ret = ops->wait_finish(obj->reg, obj->priv, 1)) == 0) {
			if (ops->get_result)
				ops->get_result(obj->reg, obj->priv, param->output.buf, param->output.length);
		}
	}

	if (ops->clr_config)
		ops->clr_config(obj->reg, obj->priv);

	return ret;
}

#ifdef CPU_ACPU
static int32_t _tfm_dgst_dma(GxSeModuleHwObj *obj, GxTfmDgst *param)
{
	GxTfmBuf src_buf = {0};
	GxDgstOps *ops = (GxDgstOps *)GXSE_OBJ_HWOPS(obj);
	int32_t ret = GXSE_ERR_GENERIC;
	uint32_t pos = 0, left = 0, dma_left = 0;
	uint32_t buf = 0, buflen = 0, align = 0, flag = 0;
	uint32_t blocklen = 4, final_val = 0, paddr = 0;
	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);

	if (ops->set_work_mode)
		ops->set_work_mode(obj->reg, obj->priv, 1);

	if (ops->set_alg)
		ops->set_alg(obj->reg, obj->priv, param->alg);

	if (ops->set_endian)
		ops->set_endian(obj->reg, obj->priv, endian_flag);

	if (ops->get_blocklen)
		blocklen = ops->get_blocklen(obj->reg, obj->priv);

	if (blocklen == 0) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "blocklen is 0\n");
		return GXSE_ERR_GENERIC;
	}

	pos  = param->input.length/4*4;
	left = param->input.length%4;
	if (left) {
		memcpy(&final_val, param->input.buf+pos, left);
		final_val |= left<<28;
	}
	//TODO large buffer

	if (ops->get_align_param)
		ops->get_align_param(obj->reg, obj->priv, &buf, &buflen, &align, &flag);

	src_buf.buf    = (void *)buf;
	src_buf.length = buflen;
	if (gx_tfm_align_buf(&param->input, &src_buf, &paddr, align, (flag & 0xf) | TFM_FLAG_IS_SRC) < 0)
		return GXSE_ERR_GENERIC;

	pos = 0;
	dma_left = param->input.length % blocklen;
	if (dma_left && param->input.length > blocklen) {
		pos = param->input.length/blocklen*blocklen;
		if (ops->set_dma_buf)
			if (ops->set_dma_buf(obj->reg, obj->priv, (void *)paddr, pos) < 0)
				goto out;
	}

	if (ops->set_ready)
		ops->set_ready(obj->reg, obj->priv, final_val);

	if (ops->set_dma_buf)
		if (ops->set_dma_buf(obj->reg, obj->priv, (void *)(paddr+pos), param->input.length-pos) < 0)
			goto out;

	if (ops->wait_finish) {
		if ((ret = ops->wait_finish(obj->reg, obj->priv, 1)) == 0) {
			if (ops->get_result)
				ops->get_result(obj->reg, obj->priv, param->output.buf, param->output.length);
		}
	}

out:
	gx_tfm_restore_buf(&param->input, &src_buf, (flag & 0xf) | TFM_FLAG_IS_SRC);
	if (ops->clr_config)
		ops->clr_config(obj->reg, obj->priv);

	return ret;
}
#endif

int32_t gx_tfm_object_dgst(GxSeModuleHwObj *obj, GxTfmDgst *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxDgstOps *ops = (GxDgstOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param)) < 0)
			return ret;
	}

#ifdef CPU_ACPU
	if (param->input.length < 16) {
		if (param->alg == TFM_ALG_SHA1)
			return gx_soft_sha1(param->input.buf, param->input.length, param->output.buf);
		else if (param->alg == TFM_ALG_SHA256)
			return gx_soft_sha256(param->input.buf, param->input.length, param->output.buf);
		else if (param->alg == TFM_ALG_SM3) {
			gx_soft_sm3(param->input.buf, param->input.length, param->output.buf);
			return GXSE_SUCCESS;
		} else {
			gxlog_e(GXSE_LOG_MOD_DGST, "Dgst not support the algorithm = %d\n", param->alg);
			return GXSE_ERR_GENERIC;
		}
	}

	if (TFM_FLAG_GET(param->flags, TFM_FLAG_HASH_DMA_EN))
		ret = _tfm_dgst_dma(obj, param);
	else
#endif
		ret = _tfm_dgst_fifo(obj, param);

	return ret;
}

#ifdef CPU_ACPU
int32_t gx_tfm_object_dgst_hmac(GxSeModuleHwObj *obj, GxTfmDgst *param)
{
	GxTfmDgst tmp_dgst = {0};
	int32_t ret = GXSE_ERR_GENERIC;
	uint8_t Ipad[64], Opad[64], KO[64], temp_key[32] = {0}, *input;

	if (NULL == param || param->alg < TFM_ALG_SM3_HMAC) {
		gxlog_e(GXSE_LOG_MOD_DGST, "Parameter error\n");
		return GXSE_ERR_PARAM;
	}

	if (INTEGER_MAX_VALUE - 128 < param->input.length) {
		gxlog_e(GXSE_LOG_MOD_DGST, "%s: Integer Overflow\n", __func__);
		return GXSE_ERR_GENERIC;
	}

	input = gx_malloc(param->input.length+128);
	if (NULL == input) {
		gxlog_e(GXSE_LOG_MOD_DGST, "%s: temp buffer allocate failed.\n", __func__);
		return -1;
	}

	if (param->hmac_key.length > 64) {
		memcpy(&tmp_dgst, param, sizeof(GxTfmDgst));
		tmp_dgst.input.buf     = param->hmac_key.buf;
		tmp_dgst.input.length  = param->hmac_key.length;
		tmp_dgst.output.buf    = temp_key;
		tmp_dgst.output.length = 32;
		if ((ret = gx_tfm_object_dgst(obj, &tmp_dgst)) < 0)
			goto err;
	}

	memset(Ipad, 0x36, sizeof(Ipad));
	memset(Opad, 0x5c, sizeof(Opad));
	memset(KO, 0, sizeof(KO));
	memcpy(KO, temp_key, sizeof(temp_key));

	xor(KO, Ipad, sizeof(KO), input);
	memcpy(input+64, param->input.buf, param->input.length);
	memcpy(&tmp_dgst, param, sizeof(GxTfmDgst));
	tmp_dgst.input.buf     = input;
	tmp_dgst.input.length  = param->input.length+64;
	tmp_dgst.output.buf    = param->output.buf;
	tmp_dgst.output.length = 32;
	if ((ret = gx_tfm_object_dgst(obj, &tmp_dgst)) < 0)
		goto err;

	xor(KO, Opad, sizeof(KO), input);
	memcpy(input+64, param->output.buf, 32);
	memcpy(&tmp_dgst, param, sizeof(GxTfmDgst));
	tmp_dgst.input.buf     = input;
	tmp_dgst.input.length  = 96;
	tmp_dgst.output.buf    = param->output.buf;
	tmp_dgst.output.length = 32;
	if ((ret = gx_tfm_object_dgst(obj, &tmp_dgst)) < 0)
		goto err;
err:
	if (input)
		gx_free(input);
	return ret;
}
#endif

int32_t gx_tfm_object_dgst_capability(GxSeModuleHwObj *obj, GxTfmCap *param)
{
	GxDgstOps *ops = (GxDgstOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->capability)
		ops->capability(obj->reg, obj->priv, param);

	return GXSE_SUCCESS;
}

GxSeModuleOps dgst_dev_ops = {
	.ioctl  = gx_dgst_ioctl,
	.init   = gxse_module_init,
#ifdef CPU_ACPU
	.deinit = gxse_module_deinit,
	.open   = gxse_module_open,
	.close  = gxse_module_close,
	.isr    = gxse_module_isr,
	.dsr    = gxse_module_dsr,
#endif
};

