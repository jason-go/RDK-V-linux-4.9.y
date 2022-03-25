#include "gxakcipher_virdev.h"
#include "sirius_sm2.h"


#include "gx_external_sm2.h"
#include "gx_soft_sm2.h"
#include "gx_soft_sm3.h"

struct akcipher_priv {
	unsigned int n[18];
	unsigned int p[18];
	unsigned int a[18];
	unsigned int b[18];
	unsigned int xG[18];
	unsigned int yG[18];
	unsigned int xP[18];
	unsigned int yP[18];

	void         *vreg;
	gx_mutex_t   mutex;
};

static unsigned int test_p[] = {
//	0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xffffffff // for sm2 verify
//	0x08f1dfc3, 0x722edb8b, 0x5c45517d, 0x45728391, 0xbf6ff7de, 0xe8b92435, 0x4c044f18, 0x8542d69e // for sm2 enc
	0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE // for SHUMA CA
};

static unsigned int test_a[] = {
//	0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF // for sm2 verify
//	0x3937e498, 0xec65228b, 0x6831d7e0, 0x2f3c848b, 0x73bbfeff, 0x2417842e, 0xfa32c3fd, 0x787968b4 // for sm2 enc
	0xFFFFFFFC, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE
};

static unsigned int test_b[] = {
//	0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0, 0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8 // for sm2 verify
//	0x27c5249a, 0x6e12d1da, 0xb16ba06e, 0xf61d59a5, 0x484bfe48, 0x9cf84241, 0xb23b0c84, 0x63e4c6d3 // for sm2 enc
	0x4D940E93, 0xDDBCBD41, 0x15AB8F92, 0xF39789F5, 0xCF6509A7, 0x4D5A9E4B, 0x9D9F5E34, 0x28E9FA9E
};

static unsigned int test_n[] = {
//	0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF // for sm2 verify
//	0xc32e79b7, 0x5ae74ee7, 0x0485628d, 0x29772063, 0xbf6ff7dd, 0xe8b92435, 0x4c044f18, 0x8542d69e // for sm2 enc
	0x39D54123, 0x53BBF409, 0x21C6052B, 0x7203DF6B, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE
};

static unsigned int test_xg[] = {
//	0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81, 0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2 // for sm2 verify
//	0x7fedd43d, 0x4c4e6c14, 0xadd50bdc, 0x32220b3b, 0xc3cc315e, 0x746434eb, 0x1b62eab6, 0x421debd6 // for sm2 enc
	0x334C74C7, 0x715A4589, 0xF2660BE1, 0x8FE30BBF, 0x6A39C994, 0x5F990446, 0x1F198119, 0x32C4AE2C
};

static unsigned int test_yg[] = {
//	0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357, 0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2 // for sm2 verify
//	0xe46e09a2, 0xa85841b9, 0xbfa36ea1, 0xe5d7fdfc, 0x153b70c4, 0xd47349d2, 0xcbb42c07, 0x0680512b // for sm2 enc
	0x2139F0A0, 0x02DF32E5, 0xC62A4740, 0xD0A9877C, 0x6B692153, 0x59BDCEE3, 0xF4F6779C, 0xBC3736A2
};

static int32_t sirius_sm2_check_param(void *vreg, void *priv, void *param, uint32_t cmd)
{
	(void) vreg;
	(void) priv;

	switch (cmd) {
	case TFM_ENCRYPT:
	case TFM_DECRYPT:
		{
			GxTfmCrypto *tfm = (GxTfmCrypto *)param;
			if (cmd == TFM_ENCRYPT) {
				if (NULL == tfm->sm2_pub_key.buf || tfm->sm2_pub_key.length != 64) {
					gxlog_e(GXSE_LOG_MOD_AKCIPHER, "public key error\n");
					return GXSE_ERR_PARAM;
				}

			} else {
				if (tfm->input.length < 96) {
					gxlog_e(GXSE_LOG_MOD_AKCIPHER, "Input buffer size shouldn't less than 96.\n");
					return GXSE_ERR_PARAM;
				}
			}

			if (NULL == tfm->input.buf || tfm->input.length == 0) {
				gxlog_e(GXSE_LOG_MOD_AKCIPHER, "input buf error\n");
				return GXSE_ERR_PARAM;
			}

			if (NULL == tfm->output.buf || tfm->output.length == 0) {
				gxlog_e(GXSE_LOG_MOD_AKCIPHER, "output buf error\n");
				return GXSE_ERR_PARAM;
			}
		}
		break;

	case TFM_DGST:
		{
			GxTfmDgst *tfm = (GxTfmDgst *)param;

			if (NULL == tfm->pub_key.buf || tfm->pub_key.length != 64) {
				gxlog_e(GXSE_LOG_MOD_AKCIPHER, "public key error\n");
				return GXSE_ERR_PARAM;
			}

			if (NULL == tfm->input.buf || tfm->input.length > 64) {
				gxlog_e(GXSE_LOG_MOD_AKCIPHER, "ida buf error\n");
				return GXSE_ERR_PARAM;
			}

			if (NULL == tfm->output.buf || tfm->output.length < 32) {
				gxlog_e(GXSE_LOG_MOD_AKCIPHER, "output buf error\n");
				return GXSE_ERR_PARAM;
			}
		}
		break;

	case TFM_VERIFY:
		{
			GxTfmVerify *tfm = (GxTfmVerify *)param;

			if (NULL == tfm->pub_key.buf || tfm->pub_key.length != 64) {
				gxlog_e(GXSE_LOG_MOD_AKCIPHER, "public key error\n");
				return GXSE_ERR_PARAM;
			}

			if (NULL == tfm->hash.buf || tfm->hash.length != 32) {
				gxlog_e(GXSE_LOG_MOD_AKCIPHER, "hash buf error\n");
				return GXSE_ERR_PARAM;
			}

			if (NULL == tfm->signature.buf || tfm->signature.length != 64) {
				gxlog_e(GXSE_LOG_MOD_AKCIPHER, "signature buf error\n");
				return GXSE_ERR_PARAM;
			}
		}
		break;

	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "Not support the cmd.\n");
		return GXSE_ERR_PARAM;
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_sm2_do_verify(void *vreg, void *priv, uint8_t *pub_key, uint32_t pub_key_len,
	uint8_t *hash, uint32_t hash_len, uint8_t *sig, uint32_t sig_len)
{
	uint32_t status = 0, result = 0;
	unsigned char temp_r[32], temp_s[32], temp_m[32];
	struct akcipher_priv *p = (struct akcipher_priv *)priv;
	(void) vreg;
	(void) pub_key_len;
	(void) hash_len;
	(void) sig_len;

	gx_buf_reverse(pub_key, 32, (void *)p->xP);
	gx_buf_reverse(pub_key+32, 32, (void *)p->yP);
	gx_buf_reverse(sig, 32, temp_r);
	gx_buf_reverse(sig+32, 32, temp_s);
	gx_buf_reverse(hash, 32, temp_m);

	status = sm2_verif(p->p,
			p->a,
			p->b,
			8,// size_p_in_words,
			p->n,
			8,// size_n_in_words,
			2,// sm2_h,
            p->xG,
            p->yG,
            p->xP,
            p->yP,
            (void *) temp_r,
            (void *) temp_s,
            (void *) temp_m,
            8,//size_m_in_words,
            (unsigned int *) &result);

	return (result == 1 && status == 0) ? GXSE_SUCCESS : GXSE_ERR_GENERIC;
}

static int32_t sirius_sm2_get_curve_param(void *vreg, void *priv, uint8_t **n, uint8_t **_p,
		uint8_t **a, uint8_t **b, uint8_t **xG, uint8_t **yG)
{
	struct akcipher_priv *p = (struct akcipher_priv *)priv;
	(void) vreg;

	if (n)
		*n = (void *)p->n;

	if (_p)
		*_p = (void *)p->p;

	if (a)
		*a = (void *)p->a;

	if (b)
		*b = (void *)p->b;

	if (xG)
		*xG = (void *)p->xG;

	if (yG)
		*yG = (void *)p->yG;

	return GXSE_SUCCESS;
}

static int32_t sirius_sm2_init(GxSeModuleHwObj *obj)
{
	struct akcipher_priv *p = (struct akcipher_priv *)obj->priv;
	memset(p, 0, sizeof(struct akcipher_priv));

	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;
	p->vreg = obj->reg;

	memcpy(p->n, test_n, 32);     p->n[17] = 8;
	memcpy(p->p, test_p, 32);     p->p[17] = 8;
	memcpy(p->a, test_a, 32);     p->a[17] = 8;
	memcpy(p->b, test_b, 32);     p->b[17] = 8;
	memcpy(p->xG, test_xg, 32);   p->xG[17] = 8;
	memcpy(p->yG, test_yg, 32);   p->yG[17] = 8;

	p->xP[17] = 8;
	p->yP[17] = 8;

	return GXSE_SUCCESS;
}

static int32_t sirius_sm2_deinit(GxSeModuleHwObj *obj)
{
	struct akcipher_priv *p = (struct akcipher_priv *)obj->priv;

	gx_mutex_destroy(&p->mutex);
	obj->mutex = NULL;
	p->vreg = NULL;

	return GXSE_SUCCESS;
}

static int32_t sirius_sm2_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->alg = 1 << TFM_ALG_SM2 | 1 << TFM_ALG_SM2_ZA;
	return GXSE_SUCCESS;
}

static struct akcipher_priv sirius_akcipher_priv;

unsigned long get_copro_base_address(void)
{
      return (unsigned long)sirius_akcipher_priv.vreg;
}

static GxSeModuleDevOps sirius_sm2_devops = {
	.init            = sirius_sm2_init,
	.deinit          = sirius_sm2_deinit,
};

static GxAkcipherOps sirius_sm2_hwops = {
	.check_param     = sirius_sm2_check_param,
	.do_verify       = sirius_sm2_do_verify,
	.get_curve_param = sirius_sm2_get_curve_param,

	.capability      = sirius_sm2_capability,
};

static GxSeModuleHwObjTfmOps sirius_sm2_objops = {
	.devops = &sirius_sm2_devops,
	.hwops = &sirius_sm2_hwops,

	.tfm_akcipher = {
		.encrypt         = gx_tfm_object_akcipher_encrypt,
		.decrypt         = gx_tfm_object_akcipher_decrypt,
		.verify          = gx_tfm_object_akcipher_verify,
		.cal_za          = gx_tfm_object_akcipher_cal_za,
		.capability      = gx_tfm_object_akcipher_capability,
	},
};

static GxSeModuleHwObj sirius_sm2_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_MISC_AKCIPHER,
	.ops  = &sirius_sm2_objops,
	.priv = &sirius_akcipher_priv,
};

GxSeModule sirius_sm2_module = {
	.id   = GXSE_MOD_MISC_AKCIPHER,
	.ops  = &akcipher_dev_ops,
	.hwobj= &sirius_sm2_tfm,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_SM2,
		.reg_len   = 0x4000,
		.irqs      = {-1},
	},
};
