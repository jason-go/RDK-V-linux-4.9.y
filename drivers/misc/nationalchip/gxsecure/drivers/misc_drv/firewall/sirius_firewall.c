#include "../gxmisc_virdev.h"
#include "sirius_firewall.h"
#include "sirius_firewall_reg.h"

#define FW_OTP_DDR_CONTENT_LIMIT 0xEC
#define FW_OTP_DDR_SCRAMBLE      0xEC
#define FW_OTP_DEMUX_TSW_BUF     0xE6
#define FW_OTP_DEMUX_ES_BUF      0xE6
#define FW_OTP_GP_ES_BUF         0xE7
#define FW_OTP_VIDEO_FW_BUF      0xE7
#define FW_OTP_AUDIO_FW_BUF      0xE7

struct firewall_priv {
	gx_mutex_t mutex;
	uint32_t mask;
};

static void _fw_set_mask(uint32_t *mask, uint32_t start_bit, uint32_t num)
{
	int32_t i;
	if (mask) {
		for (i = 0; i < num; i++)
			*mask |= 0x1 << (start_bit + i);
	}
}

static int32_t sirius_fw_get_protect_buffer(GxSeModuleHwObj *obj, uint32_t *param)
{
	uint32_t protect_buffer = 0;
	struct firewall_priv *p = (struct firewall_priv *)obj->priv;

	// main switch
	if (!_fw_get_otp_flag(FW_OTP_DDR_CONTENT_LIMIT, 1)) {
		*param = protect_buffer;
		return GXSE_SUCCESS;
	}

	// DEMUX TSW buff switch
	if (_fw_get_otp_flag(FW_OTP_DEMUX_TSW_BUF, 0)) {
		protect_buffer |= (GXFW_BUFFER_DEMUX_TSW | GXFW_BUFFER_DEMUX_TSR);
		_fw_set_mask(&p->mask, FW_DEMUX_TSW_BUF_ID, FW_DEMUX_TSW_BUF_NUM);
	}

	// DEMUX ES buff switch
	if (_fw_get_otp_flag(FW_OTP_DEMUX_ES_BUF, 1)) {
		protect_buffer |= GXFW_BUFFER_DEMUX_ES;
		_fw_set_mask(&p->mask, FW_DEMUX_ES_BUF_ID, FW_DEMUX_ES_BUF_NUM);
	}

	// GP buff switch
	if (_fw_get_otp_flag(FW_OTP_GP_ES_BUF, 0)) {
		protect_buffer |= GXFW_BUFFER_GP_ES;
		_fw_set_mask(&p->mask, FW_GP_ES_BUF_ID, FW_GP_ES_BUF_NUM);
	}

	// VIDEO/AUDIO FIRMWARE buff switch
	if (_fw_get_otp_flag(FW_OTP_VIDEO_FW_BUF, 1)) {
		protect_buffer |= (GXFW_BUFFER_VIDEO_FIRMWARE | GXFW_BUFFER_AUDIO_FIRMWARE);
		_fw_set_mask(&p->mask, FW_VIDEO_FW_BUF_ID, FW_VIDEO_FW_BUF_NUM);
		_fw_set_mask(&p->mask, FW_AUDIO_FW_BUF_ID, FW_AUDIO_FW_BUF_NUM);
	}

	gxlog_d(GXSE_LOG_MOD_MISC_FIREWALL, "protect flag %x\n", protect_buffer);

	*param = protect_buffer;
	return GXSE_SUCCESS;
}

static int32_t sirius_fw_set_protect_buffer(GxSeModuleHwObj *obj, GxSeFirewallBuffer *param)
{
	int32_t i = 0;
	uint32_t fw_filter_cfg = 0;
	uint32_t fw_regs = (uint32_t)obj->reg;
	uint32_t cur_filter_num = 0, start_addr, end_addr, have_idle_filter = 0;
	uint32_t addr, size, rd, wr, secure_flag;
	struct firewall_priv *p = (struct firewall_priv *)obj->priv;

	if (NULL == obj->reg)
		return GXSE_ERR_GENERIC;

	addr = param->addr;
	size = param->size;
	wr   = param->wr;
	rd   = param->rd;
	secure_flag = param->flags & GXSE_FW_FLAG_TEE;

	if (addr & 0x3ff || size & 0x3ff) {
		gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "addr and size must be aligned in 1KB\n");
		return GXSE_ERR_GENERIC;
	}

	for (i = 0; i < FILTER_MAX_NUM; i++) {
		start_addr = readl((volatile void *)(fw_regs + i * FW_FILTER_SKIP + FW_FILTER00_BASE));
		end_addr   = readl((volatile void *)(fw_regs + i * FW_FILTER_SKIP + FW_FILTER00_TOP ));

        // find idle filter
		if ((have_idle_filter == 0) && (start_addr == 0)) {
			if (p->mask & (0x1<<i))
				continue;
			else {
				have_idle_filter = 1;
				cur_filter_num = i;
			}
		}

        // find same buffer
		if ((start_addr == addr) && ((end_addr - start_addr) == size)) {
			if ((p->mask & (0x1<<i)) && i < FILTER_FIRST_SOFT_BUFFER_ID) {
				gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "Can't modify HW buffer(%d) [%x~%x]", i, start_addr, end_addr);
				return GXSE_SUCCESS;
			}

			cur_filter_num = i;
			break;
		}
	}

	if (i == FILTER_MAX_NUM && !have_idle_filter) {
		gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "no enough filter to use\n");
		return GXSE_ERR_GENERIC;
	}
	if (secure_flag) {
		rd = MASTER_A7;
		wr = MASTER_A7;
	}

	writel(addr, (volatile void *)(fw_regs + FW_FILTER00_BASE + cur_filter_num * FW_FILTER_SKIP));
	writel(size, (volatile void *)(fw_regs + FW_FILTER00_TOP  + cur_filter_num * FW_FILTER_SKIP));
	writel(rd, (volatile void *)(fw_regs + FW_FILTER00_RD_MASK + cur_filter_num * FW_FILTER_SKIP));
	writel(wr, (volatile void *)(fw_regs + FW_FILTER00_WR_MASK + cur_filter_num * FW_FILTER_SKIP));

	secure_flag = 0x5 | (secure_flag<<1);
	fw_filter_cfg = readl((const volatile void *)(fw_regs + FW_FILTER_CFG0 + (cur_filter_num / FILTER_MASK) * 4));
	fw_filter_cfg &= ~((0xF) << ((cur_filter_num % FILTER_MASK) * 4));
	fw_filter_cfg |= (secure_flag << ((cur_filter_num % FILTER_MASK) * 4));
	writel(fw_filter_cfg, (volatile void *)(fw_regs + FW_FILTER_CFG0 + (cur_filter_num / FILTER_MASK) * 4));
	writel(0x1, (volatile void *)(fw_regs + FW_CONFIG_UPLOAD));

	return GXSE_SUCCESS;
}

static int sirius_fw_query_access_align(GxSeModuleHwObj *obj, uint32_t *param)
{
	(void) obj;

	*param = _fw_get_otp_flag(FW_OTP_DDR_SCRAMBLE, 0) ? 4 : 1;
	return GXSE_SUCCESS;
}

static int sirius_fw_query_protect_align(GxSeModuleHwObj *obj, uint32_t *param)
{
	(void) obj;

	*param = _fw_get_otp_flag(FW_OTP_DDR_CONTENT_LIMIT, 1) ? 1024 : 64;
	return GXSE_SUCCESS;
}

static void _fw_probe_filter(GxSeModuleHwObj *obj)
{
	int32_t i;
	uint32_t fw_regs = (uint32_t)obj->reg;
	uint32_t fw_filter_cfg = 0;
	struct firewall_priv *p = (struct firewall_priv *)obj->priv;

	for (i = 0; i < FILTER_MAX_NUM; i++) {
		fw_filter_cfg = readl((const volatile void *)(fw_regs + FW_FILTER_CFG0 + (i / FILTER_MASK) * 4));
		if (fw_filter_cfg & (0xf << ((i % FILTER_MASK) * 4)))
			_fw_set_mask(&p->mask, i, 1);
		else {
			writel(0x0, (volatile void *)(fw_regs + i * FW_FILTER_SKIP + FW_FILTER00_BASE));
			writel(0x0, (volatile void *)(fw_regs + i * FW_FILTER_SKIP + FW_FILTER00_TOP));
			writel(0x0, (volatile void *)(fw_regs + i * FW_FILTER_SKIP + FW_FILTER00_RD_MASK));
			writel(0x0, (volatile void *)(fw_regs + i * FW_FILTER_SKIP + FW_FILTER00_WR_MASK));
		}
	}
}

static int32_t sirius_firewall_init(GxSeModuleHwObj *obj)
{
	struct firewall_priv *p = (struct firewall_priv *)obj->priv;
	uint32_t fw_regs = (uint32_t)obj->reg;

	if (!fw_regs) {
		gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "reg base is NULL!\n");
		return GXSE_ERR_GENERIC;
	}

	_fw_probe_filter(obj);

	// enable all instance & not hide default filter
	writel(0x1f, (volatile void *)(fw_regs + FW_GLOBLE_CFG));

	// enable default filter all read & write
	writel(0xffffffff, (volatile void *)(fw_regs + FW_DEFAULT_RD_MASK));
	writel(0xffffffff, (volatile void *)(fw_regs + FW_DEFAULT_WR_MASK));

	// Upload reg config
	writel(0x01, (volatile void *)(fw_regs + FW_CONFIG_UPLOAD));
	gxlog_i(GXSE_LOG_MOD_MISC_FIREWALL, "gx firewall init success!\n");

	p->mask = 0;
	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;

	return GXSE_SUCCESS;
}

static int32_t sirius_firewall_deinit(GxSeModuleHwObj *obj)
{
	struct firewall_priv *p = (struct firewall_priv *)obj->priv;
	uint32_t fw_regs = (uint32_t)obj->reg;

	if (!fw_regs) {
		gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "reg base is NULL!\n");
		return GXSE_ERR_GENERIC;
	}
	p->mask = 0;
	// disable enable all instance & hide default filter
	writel(0x0, (volatile void *)(fw_regs + FW_GLOBLE_CFG));
	writel(0x1, (volatile void *)(fw_regs + FW_CONFIG_UPLOAD));

	return GXSE_SUCCESS;
}

static struct firewall_priv sirius_firewall_priv;

static GxSeModuleDevOps sirius_firewall_devops = {
	.init   = sirius_firewall_init,
	.deinit = sirius_firewall_deinit,
    .ioctl  = gx_misc_fw_ioctl,
};

static GxSeModuleHwObjMiscOps sirius_firewall_ops = {
	.devops = &sirius_firewall_devops,
	.misc_fw = {
		.get_protect_buffer  = sirius_fw_get_protect_buffer,
		.set_protect_buffer  = sirius_fw_set_protect_buffer,
		.query_access_align  = sirius_fw_query_access_align,
		.query_protect_align = sirius_fw_query_protect_align,
	},
};

static GxSeModuleHwObj sirius_firewall_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &sirius_firewall_ops,
	.priv = &sirius_firewall_priv,
};

GxSeModule sirius_firewall_module = {
	.id   = GXSE_MOD_MISC_FIREWALL,
	.ops  = &misc_dev_ops,
	.hwobj= &sirius_firewall_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_FIREWALL,
		.reg_len   = 0x400,
		.irqs      = {-1},
	},
};
