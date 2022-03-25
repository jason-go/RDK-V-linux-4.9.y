#include "gxse_core.h"
#include "gxklm_virdev.h"

static uint32_t _klm_start[GXSE_MAX_KLM_COUNT] = {0};
static uint32_t _klm_cur_stage[GXSE_MAX_KLM_COUNT] = {0};
static uint32_t _klm_total_stage[GXSE_MAX_KLM_COUNT] = {0};
static int32_t gx_klm_ioctl(GxSeModule *module, uint32_t cmd, void *param, uint32_t size)
{
	uint32_t id = 0;
	int32_t ret = GXSE_SUCCESS, i = 0;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleHwObjTfmOps *tfm_ops = NULL;

	if (gxse_module_ioctl_check(module, GXSE_HWOBJ_TYPE_TFM, cmd, size) < 0)
		return GXSE_ERR_GENERIC;

	id = GXSE_KLM_ID(module->id);
	obj = module->hwobj;
	tfm_ops = (GxSeModuleHwObjTfmOps *)obj->ops;
	gx_mutex_lock(obj->mutex);
	switch (cmd) {
	case TFM_KLM_SELECT_ROOTKEY:
		if (tfm_ops->tfm_klm.select_rootkey) {
			if ((ret = tfm_ops->tfm_klm.select_rootkey(obj, param)) < 0)
				goto out;

			_klm_start[id] = 1;
			_klm_cur_stage[id] = 0;
			_klm_total_stage[id] = ((GxTfmKeyLadder *)param)->stage;
		}
		break;

	case TFM_KLM_SET_KN:
		if (!_klm_start[id]) {
			gxlog_e(GXSE_LOG_MOD_KLM, "The KLM module (%d) hasn't selected rootkey\n", module->id);
			ret = GXSE_ERR_GENERIC;
			goto out;
		}

		if (_klm_cur_stage[id]+1 >= _klm_total_stage[id]) {
			gxlog_e(GXSE_LOG_MOD_KLM, "The KLM module (%d)'s current stage(%d) > total stage(%d).\n",
					module->id, _klm_cur_stage[id], _klm_total_stage[id]);
			ret = GXSE_ERR_GENERIC;
			_klm_start[id] = 0;
			goto out;
		}

		if (tfm_ops->tfm_klm.set_kn) {
			if ((ret = tfm_ops->tfm_klm.set_kn(obj, param, _klm_cur_stage[id])) < 0) {
				_klm_start[id] = 0;
				goto out;
			}
			_klm_cur_stage[id]++;
		}
		break;

	case TFM_KLM_SET_CW:
		if (!_klm_start[id]) {
			gxlog_e(GXSE_LOG_MOD_KLM, "The KLM module (%d) hasn't selected rootkey\n", module->id);
			ret = GXSE_ERR_GENERIC;
			goto out;
		}

		for (i = _klm_cur_stage[id]; i < _klm_total_stage[id]-1; i++) {
			GxTfmKeyLadder kn_tfm = {0};

#ifdef CPU_ACPU
			unsigned char input[16] = {0};
			memcpy(&kn_tfm, param, sizeof(GxTfmKeyLadder));
			kn_tfm.input.buf = input;
			memset(input, 0x0, 8);
			memset(input+8, 0x1, 8);
#else
			memcpy(&kn_tfm, param, sizeof(GxTfmKeyLadder));
			memset(kn_tfm.input, 0x0, 8);
			memset(kn_tfm.input+8, 0x1, 8);
#endif

			if (tfm_ops->tfm_klm.set_kn) {
				if ((ret = tfm_ops->tfm_klm.set_kn(obj, &kn_tfm, _klm_cur_stage[id])) < 0) {
					_klm_start[id] = 0;
					goto out;
				}
				_klm_cur_stage[id]++;
			}
		}

		if (tfm_ops->tfm_klm.set_cw)
			ret = tfm_ops->tfm_klm.set_cw(obj, param);

		_klm_start[id] = 0;
		break;

	case TFM_KLM_UPDATE_TD_PARAM:
		if (tfm_ops->tfm_klm.update_TD_param)
			ret = tfm_ops->tfm_klm.update_TD_param(obj, param);
		break;

	case TFM_KLM_GET_RESP:
		if (tfm_ops->tfm_klm.get_resp)
			ret = tfm_ops->tfm_klm.get_resp(obj, param);
		break;

	case TFM_CAPABILITY:
		if (tfm_ops->tfm_klm.capability)
			ret = tfm_ops->tfm_klm.capability(obj, param);
		break;

	default:
		break;
	}

out:
	gx_mutex_unlock(obj->mutex);
	return ret;
}

int32_t gx_tfm_object_klm_select_rootkey(GxSeModuleHwObj *obj, GxTfmKeyLadder *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxKLMOps *ops = (GxKLMOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_KLM_SELECT_ROOTKEY)) < 0)
			return ret;
	}

	if (ops->set_rootkey)
		ret = ops->set_rootkey(obj->reg, obj->priv, param->key);

	if (ops->set_stage)
		ops->set_stage(obj->reg, obj->priv, param->stage);

	return ret;
}

int32_t gx_tfm_object_klm_set_kn(GxSeModuleHwObj *obj, GxTfmKeyLadder *param, uint32_t pos)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxKLMOps *ops = (GxKLMOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_KLM_SET_KN)) < 0)
			return ret;
	}

	gxlog_d(GXSE_LOG_MOD_KLM, "[CIPHER] stage(%d) : \n", pos);

	if (ops->set_ekn)
		ret = ops->set_ekn(obj->reg, obj->priv, TFM_GET_KLM_BUF(param->input), TFM_GET_KLM_BUFLEN(param->input), pos);

	return ret;
}

int32_t gx_tfm_object_klm_set_cw(GxSeModuleHwObj *obj, GxTfmKeyLadder *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxKLMOps *ops = (GxKLMOps *)GXSE_OBJ_HWOPS(obj);

#ifdef CPU_ACPU
	if (param->flags & TFM_FLAG_KLM_INPUT_HALF || param->input.length < 16)
		param->flags |= TFM_FLAG_KLM_CW_HALF;
#endif

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_KLM_SET_CW)) < 0)
			return ret;
	}

	if (ops->set_alg)
		ops->set_alg(obj->reg, obj->priv, param->alg);

	if (ops->set_dst)
		ops->set_dst(obj->reg, obj->priv, param->dst);

	gxlog_d(GXSE_LOG_MOD_KLM, "[CIPHER] ECW : \n");
	if (ops->set_ecw)
		ret = ops->set_ecw(obj->reg, obj->priv, TFM_GET_KLM_BUF(param->input), TFM_GET_KLM_BUFLEN(param->input));

	if (ops->set_ready)
		ops->set_ready(obj->reg, obj->priv);

	if (ops->wait_finish) {
		if ((ret = ops->wait_finish(obj->reg, obj->priv, 0)) < 0)
			goto clr_config;
	}

clr_config:
	if (ops->clr_config)
		ops->clr_config(obj->reg, obj->priv);

	return ret;
}

static uint32_t get_klm_id(GxSeModuleHwObj * obj)
{
	uint32_t id = -1, sub = 0;

	sub = obj->sub;

	switch (sub) {
	case TFM_MOD_KLM_SCPU_STATIC:
		id = GXSE_KLM_ID(GXSE_MOD_KLM_SCPU_IRDETO_GENERIC);
		break;

	case TFM_MOD_KLM_SCPU_DYNAMIC:
		id = GXSE_KLM_ID(GXSE_MOD_KLM_SCPU_IRDETO);
		break;

	case TFM_MOD_KLM_SCPU_NC_DYNAMIC:
		id = GXSE_KLM_ID(GXSE_MOD_KLM_SCPU_GENERIC);
		break;

	case TFM_MOD_KLM_IRDETO:
		id = GXSE_KLM_ID(GXSE_MOD_KLM_IRDETO);
		break;

	case TFM_MOD_KLM_GENERAL:
		id = GXSE_KLM_ID(GXSE_MOD_KLM_GENERIC);
		break;

	default:
		break;
	}
	return id;
}

int32_t gx_tfm_object_klm_get_resp(GxSeModuleHwObj *obj, GxTfmKeyLadder *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	uint32_t id = 0;

	GxKLMOps *ops = (GxKLMOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_KLM_GET_RESP)) < 0)
			return ret;
	}

	id = get_klm_id(obj);
	if (_klm_start[id]) {
		gxlog_e(GXSE_LOG_MOD_KLM, "The KLM module (%d) is in KLM process\n", id);
		ret = GXSE_ERR_GENERIC;
		return ret;
	}

	if (ops->set_alg)
		ops->set_alg(obj->reg, obj->priv, param->alg);

	if (ops->set_resp_rootkey)
		ops->set_resp_rootkey(obj->reg, obj->priv, param->key);

	if (ops->set_resp_kn)
		ops->set_resp_kn(obj->reg, obj->priv, TFM_GET_KLM_BUF(param->input), TFM_GET_KLM_BUFLEN(param->input), 0);

	if (ops->get_resp)
		ret = ops->get_resp(obj->reg, obj->priv, (void *)param->nonce, sizeof(param->nonce),
							TFM_GET_KLM_BUF(param->output), TFM_GET_KLM_BUFLEN(param->output));

	return ret;
}

int32_t gx_tfm_object_klm_update_TD_param(GxSeModuleHwObj *obj, GxTfmKeyLadder *param)
{
#ifdef CPU_ACPU
	uint32_t id = 0;
	int32_t ret = GXSE_ERR_GENERIC;
	uint8_t *buf = param->TD.buf;
	uint32_t size = param->TD.length, pos = 0, data_len = 0, blocklen = 16;

	GxKLMOps *ops = (GxKLMOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->check_param) {
		if ((ret = ops->check_param(obj->reg, obj->priv, param, TFM_KLM_UPDATE_TD_PARAM)) < 0)
			return ret;
	}

	id = get_klm_id(obj);
	if (_klm_start[id]) {
		gxlog_e(GXSE_LOG_MOD_KLM, "The KLM module (%d) is in KLM process\n", id);
		ret = GXSE_ERR_GENERIC;
		gx_mutex_unlock(obj->mutex);
		return ret;
	}

	gxlog_d(GXSE_LOG_MOD_KLM, "[TDC] ETASK : \n");
	if (ops->set_ecw)
		ops->set_ecw(obj->reg, obj->priv, param->input.buf, param->input.length);

	if (ops->set_tdc_ready)
		ops->set_tdc_ready(obj->reg, obj->priv, size);

	gxlog_d(GXSE_LOG_MOD_KLM, "[TDC] DATA : \n");
	while (pos < size) {
		uint32_t is_query = TFM_ISR_QUERY_UNTIL;
		data_len = (size - pos);
		data_len = data_len > blocklen ? blocklen : data_len;
		if (pos + blocklen >= size)
			is_query = 0;

		if (ops->wait_tdc_idle) {
			if ((ret = ops->wait_tdc_idle(obj->reg, obj->priv, is_query)) < 0)
				break;
		}

		if (ops->set_tdc_data)
			ops->set_tdc_data(obj->reg, obj->priv, buf+pos, data_len);

		pos += blocklen;
	}

	if (ops->wait_tdc_idle)
		ret = ops->wait_tdc_idle(obj->reg, obj->priv, 0);

	return ret;
#endif
	return GXSE_SUCCESS;
}

int32_t gx_tfm_object_klm_capability(GxSeModuleHwObj *obj, GxTfmCap *param)
{
	GxKLMOps *ops = (GxKLMOps *)GXSE_OBJ_HWOPS(obj);

	if (ops->capability)
		ops->capability(obj->reg, obj->priv, param);

	return GXSE_SUCCESS;
}

GxSeModuleOps klm_dev_ops = {
	.ioctl  = gx_klm_ioctl,
	.init   = gxse_module_init,
#ifdef CPU_ACPU
	.deinit = gxse_module_deinit,
	.open   = gxse_module_open,
	.close  = gxse_module_close,
	.isr    = gxse_module_isr,
	.dsr    = gxse_module_dsr,
#endif
};

