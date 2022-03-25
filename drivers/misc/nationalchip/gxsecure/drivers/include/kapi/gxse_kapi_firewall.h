#ifndef _GXSE_KAPI_FIREWALL_H_
#define _GXSE_KAPI_FIREWALL_H_

int gxse_firewall_get_protect_buffer(void);
int gxse_firewall_config_filter(int addr, int size, int master_rd_permission, int master_wr_permission,int secure_flag);
int gxse_firewall_query_access_align(void);
int gxse_firewall_query_protect_align(void);

#endif
