#include "../gxmisc_virdev.h"
#include "sirius_sensor_reg.h"

struct sensor_priv {
	volatile SiriusSensorCtlReg *ctrl_reg[10];
	volatile SiriusSensorReg *reg;
	uint32_t err_status;
	uint32_t err_value[GXSE_SENSOR_TYPE_MAX];
};

static struct sensor_priv sirius_sensor_priv;

int32_t sirius_sensor_init(GxSeModuleHwObj *obj)
{
	int32_t i = 0;
	uint32_t base_reg = (uint32_t)obj->reg;
	struct sensor_priv *p = &sirius_sensor_priv;

	memset(p, 0, sizeof(struct sensor_priv));
	for (i = 0; i < 10; i++) {
		p->ctrl_reg[i] = (volatile SiriusSensorCtlReg *)(base_reg);
		base_reg += 0x1000;
		if (i == 6)
			base_reg += 0x1000;
	}
	p->reg = (volatile SiriusSensorReg *)((uint32_t)obj->reg + 0xf000);

	for (i = 0; i < 3; i++) {
		p->reg->s3_health_int_en[i] = 0xffffffff;
		p->reg->s22_health_int_en[i] = 0xffffffff;
		p->reg->s21_health_int_en[i] = 0xffffffff;
	}
	p->reg->s21_health_int_en[3] = 0xffffffff;
	p->reg->sensor_softer_mode.value = 0x7;

	for (i = 0; i < 10; i++)
		p->ctrl_reg[i]->health_test = 0xffffffff;

	for (i = 0; i < 3; i++) {
		p->reg->s3_alarm_int_en[i] = 0xffffffff;
		p->reg->s22_alarm_int_en[i] = 0xffffffff;
		p->reg->s21_alarm_int_en[i] = 0xffffffff;
	}
	p->reg->s21_alarm_int_en[3] = 0xffffffff;
	p->reg->key_recv_err_en.value = 0xbff;
	p->reg->otp_flag_err_en.value = 1;
	p->reg->err_counter_clear = 0xffffffff;

	return GXSE_SUCCESS;
}

int32_t sirius_sensor_get_err_status(GxSeModuleHwObj *obj, GxSeSensorErrStatus *param)
{
	struct sensor_priv *p = &sirius_sensor_priv;
	volatile SiriusSensorReg *reg = p->reg;

	param->err_status = p->err_status;
	param->err_counter = reg->err_counter;

	return GXSE_SUCCESS;
}

int32_t sirius_sensor_get_err_value(GxSeModuleHwObj *obj, uint32_t *status)
{
	struct sensor_priv *p = &sirius_sensor_priv;
	if (NULL == status || *status >= GXSE_SENSOR_TYPE_MAX)
		return GXSE_ERR_GENERIC;

	*status = p->err_value[*status];

	return GXSE_SUCCESS;
}

int gx_sensor_isr(int irq, void *pdata)
{
	int i = 0, type;
	volatile SiriusSensorReg *reg = sirius_sensor_priv.reg;

	for (i = 0; i < 3; i++) {
		if (reg->s3_alarm_int[i]) {
			type = GXSE_SENSOR_TYPE_XTAL0 + i;
			sirius_sensor_priv.err_status |= 0x1 << type;
			sirius_sensor_priv.err_value[type] |= reg->s3_alarm_int[i];
		}

		if (reg->s22_alarm_int[i]) {
			type = GXSE_SENSOR_TYPE_CLOCK0 + i;
			sirius_sensor_priv.err_status |= 0x1 << type;
			sirius_sensor_priv.err_value[type] |= reg->s22_alarm_int[i];
		}

		if (reg->s3_health_int[i]) {
			type = GXSE_SENSOR_TYPE_XTAL0 + i;
			sirius_sensor_priv.err_status |= 0x1 << type;
			sirius_sensor_priv.err_value[type] |= reg->s3_health_int[i];
		}

		if (reg->s22_health_int[i]) {
			type = GXSE_SENSOR_TYPE_CLOCK0 + i;
			sirius_sensor_priv.err_status |= 0x1 << type;
			sirius_sensor_priv.err_value[type] |= reg->s22_health_int[i];
		}
	}

	for (i = 0; i < 4; i++) {
		if (reg->s21_alarm_int[i]) {
			type = GXSE_SENSOR_TYPE_SCPU0 + i;
			sirius_sensor_priv.err_status |= 0x1 << type;
			sirius_sensor_priv.err_value[type] |= reg->s21_alarm_int[i];
		}

		if (reg->s21_health_int[i]) {
			type = GXSE_SENSOR_TYPE_SCPU0 + i;
			sirius_sensor_priv.err_status |= 0x1 << type;
			sirius_sensor_priv.err_value[type] |= reg->s21_health_int[i];
		}
	}

	if (reg->key_recv_err.value) {
		type = GXSE_SENSOR_TYPE_KEY_ERR;
		sirius_sensor_priv.err_status |= 0x1<<type;
		sirius_sensor_priv.err_value[type] |= reg->key_recv_err.value;
	}

	if (reg->otp_flag_err.value) {
		type = GXSE_SENSOR_TYPE_OTP_ERR;
		sirius_sensor_priv.err_status |= 0x1<<type;
		sirius_sensor_priv.err_value[type] |= reg->otp_flag_err.value;
	}

	return 0;
}

static GxSeModuleDevOps sirius_sensor_devops = {
	.init   = sirius_sensor_init,
	.ioctl  = gx_misc_sensor_ioctl,
};

static GxSeModuleHwObjMiscOps sirius_sensor_ops = {
	.devops = &sirius_sensor_devops,
	.misc_sensor = {
		.get_err_status = sirius_sensor_get_err_status,
		.get_err_value  = sirius_sensor_get_err_value,
	},
};

static GxSeModuleHwObj sirius_sensor_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops    = &sirius_sensor_ops,
	.priv   = &sirius_sensor_priv,
};

GxSeModule sirius_sensor_module = {
	.id   = GXSE_MOD_MISC_SENSOR,
	.ops  = &misc_dev_ops,
	.hwobj= &sirius_sensor_hwobj,
	.res  = {
		.reg_base  = GXSCPU_BASE_ADDR_SENSOR,
		.reg_len   = sizeof(SiriusSensorReg),
	},
};
