#include "gxse_core.h"
#include "gxcrypto_virdev.h"

static int32_t gx_crypto_ioctl(GxSeModule *module, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
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
		{
			GxTfmCrypto _p = {0};
			memcpy(&_p, param, size);
			if (cmd == TFM_ENCRYPT) {
				if (tfm_ops->tfm_cipher.encrypt)
					ret = tfm_ops->tfm_cipher.encrypt(obj, &_p);
			} else {
				if (tfm_ops->tfm_cipher.decrypt)
					ret = tfm_ops->tfm_cipher.decrypt(obj, &_p);
			}
		}
		break;

	case TFM_CLEAR_KEY:
		if (tfm_ops->tfm_cipher.clr_key)
			ret = tfm_ops->tfm_cipher.clr_key(obj);
		break;

	case TFM_CAPABILITY:
		if (tfm_ops->tfm_cipher.capability)
			ret = tfm_ops->tfm_cipher.capability(obj, param);
		break;

#ifdef CPU_ACPU
	case TFM_SET_PVRK:
		{
			GxTfmPVRKey _p = {0};
			memcpy(&_p, param, size);
			if (tfm_ops->tfm_cipher.set_pvr_key)
				ret = tfm_ops->tfm_cipher.set_pvr_key(obj, &_p);
		}
		break;

	case TFM_ENABLE_KEY:
		if (tfm_ops->tfm_cipher.enable_key)
			ret = tfm_ops->tfm_cipher.enable_key(obj);
		break;

	case TFM_DISABLE_KEY:
		if (tfm_ops->tfm_cipher.disable_key)
			ret = tfm_ops->tfm_cipher.disable_key(obj);
		break;

	case TFM_CRYPT_KMALLOC:
		{
			unsigned int buflen = 0;
			if (tfm_ops->tfm_cipher.get_align_buf)
				ret = tfm_ops->tfm_cipher.get_align_buf(obj, param, &buflen);
		}
		break;
#endif

	default:
		break;
	}

	gx_mutex_unlock(obj->mutex);
	return ret;
}

static int32_t gx_crypto_fifo_crypt(GxSeModuleHwObj *obj, GxTfmCrypto *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxCryptoOps *ops = (GxCryptoOps *)GXSE_OBJ_HWOPS(obj);
	uint32_t blocklen = 0, first_block = 0, pos = 0, data_len = 0;
	uint32_t crypt_mode = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_ENCRYPT);
	uint32_t endian_flag   = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	uint32_t residue_flag  = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_RESIDUAL_CTS) |
							 TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_RESIDUAL_RBT) << 1;
	uint32_t shortmsg_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_SHORT_MSG_IV1) |
							 TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_SHORT_MSG_IV2) << 1;

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, crypt_mode ? TFM_ENCRYPT : TFM_DECRYPT)) < 0)
			return ret;
	}

	if (ops->set_work_mode)
		ops->set_work_mode(obj->reg, obj->priv, crypt_mode, 1, 0);

	if (ops->set_endian)
		ops->set_endian(obj->reg, obj->priv, endian_flag);

	if (ops->set_alg)
		ops->set_alg(obj->reg, obj->priv, param->alg);

	if (ops->set_key)
		ops->set_key(obj->reg, obj->priv, param->even_key, param->odd_key, param->soft_key, param->soft_key_len);

	if (ops->set_opt)
		ops->set_opt(obj->reg, obj->priv, param->opt, param->iv, 16);

	if (ops->set_residue_mode)
		ops->set_residue_mode(obj->reg, obj->priv, residue_flag);

	if (ops->set_shortmsg_mode)
		ops->set_shortmsg_mode(obj->reg, obj->priv, shortmsg_flag);

	if (ops->set_input_buf)
		ops->set_input_buf(obj->reg, obj->priv, param->src, param->input.buf, param->input.length);

	if (ops->set_output_buf)
		ops->set_output_buf(obj->reg, obj->priv, param->dst, param->output.buf, param->output.length);

	if (ops->get_blocklen)
		blocklen = ops->get_blocklen(obj->reg, obj->priv);

	if (blocklen == 0) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "blocklen is 0\n");
		return GXSE_ERR_GENERIC;
	}

#ifdef CPU_SCPU
	if (param->src.id == TFM_SRC_REG) {
		if (ops->set_ready)
			ops->set_ready(obj->reg, obj->priv);

		if (ops->wait_finish) {
			if ((ret = ops->wait_finish(obj->reg, obj->priv, 1)) < 0)
				goto out;
		}

		if (ops->fifo_get)
			ops->fifo_get(obj->reg, obj->priv, param->output.buf, param->output.length);
		goto out;
	}
#endif

	gx_osdep_cache_sync(param->input.buf, param->input.length, DMA_TO_DEVICE);
	while (pos < param->input.length) {
		data_len = param->input.length - pos;
		data_len = data_len > blocklen ? blocklen : data_len;
		first_block = pos == 0 ? 1 : 0;
		if (ops->fifo_put)
			ops->fifo_put(obj->reg, obj->priv, param->input.buf+pos, data_len, first_block); //set_enable

		if (ops->set_ready)
			ops->set_ready(obj->reg, obj->priv);

		if (ops->wait_finish) {
			if ((ret = ops->wait_finish(obj->reg, obj->priv, 1)) < 0)
				goto out;
		}

		if (ops->fifo_get)
			ops->fifo_get(obj->reg, obj->priv, param->output.buf+pos, data_len);

		pos += blocklen;
	}
	gx_osdep_cache_sync(param->output.buf, param->output.length, DMA_TO_DEVICE);

out:
	if (ops->clr_config)
		ops->clr_config(obj->reg, obj->priv);

	return ret;
}

static int32_t gx_crypto_dma_crypt(GxSeModuleHwObj *obj, GxTfmCrypto *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxCryptoOps *ops = (GxCryptoOps *)GXSE_OBJ_HWOPS(obj);
	uint32_t endian_flag   = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	uint32_t crypt_mode    = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_ENCRYPT);
	uint32_t ts_mode       = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_TS_PACKET_MODE);
	uint32_t switch_clr    = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_SWITCH_CLR);
	uint32_t residue_flag  = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_RESIDUAL_CTS) |
							 TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_RESIDUAL_RBT) << 1;
	uint32_t shortmsg_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_SHORT_MSG_IV1) |
							 TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_SHORT_MSG_IV2) << 1;
	uint32_t query_flag    = TFM_FLAG_GET(param->flags, TFM_FLAG_ISR_QUERY) |
							 TFM_FLAG_GET(param->flags, TFM_FLAG_ISR_QUERY_UNTIL) << 1;

	uint32_t buf, buflen = 0, align = 0, flag = 0;
	GxTfmBuf src_buf = {0}, dst_buf = {0};

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, crypt_mode ? TFM_ENCRYPT : TFM_DECRYPT)) < 0)
			return ret;
	}

	if (ops->set_work_mode)
		ops->set_work_mode(obj->reg, obj->priv, crypt_mode, 0, ts_mode);

	if (ops->set_endian)
		ops->set_endian(obj->reg, obj->priv, endian_flag);

	if (ops->set_alg)
		ops->set_alg(obj->reg, obj->priv, param->alg);

	if (ops->set_opt)
		ops->set_opt(obj->reg, obj->priv, param->opt, param->iv, 16);

	if (ops->set_residue_mode)
		ops->set_residue_mode(obj->reg, obj->priv, residue_flag);

	if (ops->set_shortmsg_mode)
		ops->set_shortmsg_mode(obj->reg, obj->priv, shortmsg_flag);

	if (ts_mode)
		if (ops->set_switch_clr)
			ops->set_switch_clr(obj->reg, obj->priv, switch_clr);

	if (ops->set_key)
		ops->set_key(obj->reg, obj->priv, param->even_key, param->odd_key, param->soft_key, param->soft_key_len);

	if (ops->get_align_param)
		ops->get_align_param(obj->reg, obj->priv, &buf, &buflen, &align, &flag);

	src_buf.buf    = (void *)buf;
	src_buf.length = buflen/2;
	if (gx_tfm_align_buf(&param->input, &src_buf, &param->input_paddr, align, (flag&0xf) | TFM_FLAG_IS_SRC) < 0)
		return GXSE_ERR_GENERIC;

	dst_buf.buf    = (void *)(buf+TFM_MAX_CIPHER_UNIT);
	dst_buf.length = buflen/2;
	if (gx_tfm_align_buf(&param->output, &dst_buf, &param->output_paddr, align, ((flag>>16)&0xf) & ~TFM_FLAG_IS_SRC) < 0)
		return GXSE_ERR_GENERIC;

	if (ops->set_input_buf)
		if (ops->set_input_buf(obj->reg, obj->priv, param->src, (void *)param->input_paddr, param->input.length) < 0)
			goto out;

	if (ops->set_output_buf)
		if (ops->set_output_buf(obj->reg, obj->priv, param->dst, (void *)param->output_paddr, param->output.length))
			goto out;

	if (ops->set_ready)
		ops->set_ready(obj->reg, obj->priv);

	if (ops->wait_finish)
		ret = ops->wait_finish(obj->reg, obj->priv, query_flag);

out:
	gx_tfm_restore_buf(&param->input, &src_buf, (flag&0xf) | TFM_FLAG_IS_SRC);
	gx_tfm_restore_buf(&param->output, &dst_buf, ((flag>>16)&0xf) & ~TFM_FLAG_IS_SRC);

	if (ops->clr_config)
		ops->clr_config(obj->reg, obj->priv);

	return ret;
}

int32_t gx_tfm_object_crypto_fifo_encrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param)
{
	param->flags |= TFM_FLAG_CRYPT_ENCRYPT;
	return  gx_crypto_fifo_crypt(obj, param);
}

int32_t gx_tfm_object_crypto_fifo_decrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param)
{
	param->flags &= ~TFM_FLAG_CRYPT_ENCRYPT;
	return  gx_crypto_fifo_crypt(obj, param);
}

int32_t gx_tfm_object_crypto_dma_encrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param)
{
	param->flags |= TFM_FLAG_CRYPT_ENCRYPT;
	return  gx_crypto_dma_crypt(obj, param);
}

int32_t gx_tfm_object_crypto_dma_decrypt(GxSeModuleHwObj *obj, GxTfmCrypto *param)
{
	param->flags &= ~TFM_FLAG_CRYPT_ENCRYPT;
	return  gx_crypto_dma_crypt(obj, param);
}

int32_t gx_tfm_object_crypto_get_align_buf(GxSeModuleHwObj *obj, uint32_t *buf, uint32_t *size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxCryptoOps *ops = (GxCryptoOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->get_align_param)
		ret = ops->get_align_param(obj->reg, obj->priv, buf, size, NULL, NULL);

	return ret;
}

int32_t gx_tfm_object_crypto_clr_key(GxSeModuleHwObj *obj)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxCryptoOps *ops = (GxCryptoOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->clr_key)
		ret = ops->clr_key(obj->reg, obj->priv);

	return ret;
}

int32_t gx_tfm_object_crypto_enable_key(GxSeModuleHwObj *obj)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxCryptoOps *ops = (GxCryptoOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->enable_key)
		ret = ops->enable_key(obj->reg, obj->priv);

	return ret;
}

int32_t gx_tfm_object_crypto_disable_key(GxSeModuleHwObj *obj)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxCryptoOps *ops = (GxCryptoOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->disable_key)
		ret = ops->disable_key(obj->reg, obj->priv);

	return ret;
}

int32_t gx_tfm_object_crypto_set_pvr_key(GxSeModuleHwObj *obj, GxTfmPVRKey *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxCryptoOps *ops = (GxCryptoOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->set_pvr_key)
		ret = ops->set_pvr_key(obj->reg, obj->priv, param);

	return ret;
}

int32_t gx_tfm_object_crypto_capability(GxSeModuleHwObj *obj, GxTfmCap *param)
{
	GxCryptoOps *ops = (GxCryptoOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->capability)
		ops->capability(obj->reg, obj->priv, param);

	return GXSE_SUCCESS;
}

GxSeModuleOps crypto_dev_ops = {
	.ioctl  = gx_crypto_ioctl,
	.init   = gxse_module_init,
#ifdef CPU_ACPU
	.deinit = gxse_module_deinit,
	.open   = gxse_module_open,
	.close  = gxse_module_close,
	.isr    = gxse_module_isr,
	.dsr    = gxse_module_dsr,
#endif
};
