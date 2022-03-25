#include "gxse_core.h"
#include "gxse_hwobj_misc.h"
#include "gxse_kapi_firewall.h"

int gxse_firewall_get_protect_buffer(void)
{
	int32_t ret = GXSE_ERR_GENERIC;
	static int32_t protect_buffer = 0;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_FIREWALL);

	if (protect_buffer)
		return protect_buffer;

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "Can't find the module. id = %d\n", GXSE_MOD_MISC_FIREWALL);
		return 0; // return default value
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, FIREWALL_GET_PROTECT_BUFFER, &protect_buffer, 4);

	return (ret < 0) ? 0 : protect_buffer;
}

int gxse_firewall_config_filter(int addr, int size, int master_rd_permission, int master_wr_permission, int secure_flag)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_FIREWALL);
	GxSeFirewallBuffer param = {0};

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "Can't find the module. id = %d\n", GXSE_MOD_MISC_FIREWALL);
		return GXSE_ERR_GENERIC;
	}

	param.addr  = addr;
	param.size  = size;
	param.wr    = master_wr_permission;
	param.rd    = master_rd_permission;
	param.flags = secure_flag ? GXSE_FW_FLAG_TEE : 0;
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, FIREWALL_SET_PROTECT_BUFFER, &param, sizeof(GxSeFirewallBuffer));

	return ret;
}

int gxse_firewall_query_access_align(void)
{
	static int32_t align = 0;
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_FIREWALL);

	if (align)
		return align;

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "Can't find the module. id = %d\n", GXSE_MOD_MISC_FIREWALL);
		return 1; // return default value
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, FIREWALL_QUERY_ACCESS_ALIGN, &align, 4);

	align = (ret < 0) ? 1 : align;
	return align;
}

int gxse_firewall_query_protect_align(void)
{
	static int32_t align = 0;
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_FIREWALL);

	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_MISC_FIREWALL, "Can't find the module. id = %d\n", GXSE_MOD_MISC_FIREWALL);
		return 64; // return default value
	}
	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, FIREWALL_QUERY_PROTECT_ALIGN, &align, 4);

	align = (ret < 0) ? 64 : align;
	return align;
}

int gx_firewall_get_protect_buffer(void)
{
	return gxse_firewall_get_protect_buffer();
}

int gx_firewall_query_protect_align(void)
{
	return gxse_firewall_query_protect_align();
}

int gx_firewall_query_access_align(void)
{
	return gxse_firewall_query_access_align();
}

int gx_firewall_config_filter(int addr, int size, int master_rd_permission, int master_wr_permission, int secure_flag)
{
	return gxse_firewall_config_filter(addr, size, master_rd_permission, master_wr_permission, secure_flag);
}

#if defined (LINUX_OS)
EXPORT_SYMBOL(gx_firewall_query_protect_align);
EXPORT_SYMBOL(gx_firewall_query_access_align);
EXPORT_SYMBOL(gx_firewall_get_protect_buffer);
EXPORT_SYMBOL(gx_firewall_config_filter);

EXPORT_SYMBOL(gxse_firewall_query_protect_align);
EXPORT_SYMBOL(gxse_firewall_query_access_align);
EXPORT_SYMBOL(gxse_firewall_get_protect_buffer);
EXPORT_SYMBOL(gxse_firewall_config_filter);
#endif
