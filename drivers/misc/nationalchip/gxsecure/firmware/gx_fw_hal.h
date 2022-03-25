#ifndef __GX_FW_HAL_H__
#define __GX_FW_HAL_H__

#include <kernelcalls.h>
#include "gxse_core.h"
#include "gxse_hwobj_tfm.h"
#include "gxse_hwobj_misc.h"
#include "gxse_hwobj_secure.h"

typedef struct {
	int (*init)      ( void );
	void(*start)     ( void );
	int (*console)   ( GxSecurePacket *pkt );

	int (*mbox_peek)       ( unsigned char *buf, unsigned int size );
	int (*mbox_peek_final) ( void );
	int (*mbox_send)       ( unsigned char *buf, unsigned int *size );
	int (*mbox_send_check) ( unsigned int size, unsigned char *user, unsigned int len );
	int (*mbox_terminate)  ( void );

	int (*get_resp)       ( GxTfmKeyLadder *param );
	int (*select_rootkey) ( GxTfmKeyLadder *param );
	int (*set_kn)	      ( GxTfmKeyLadder *param );
	int (*set_cw)         ( GxTfmKeyLadder *param );
} GxFWOps;

int gx_fw_register(GxFWOps **ops);
int gx_fw_get_ddr_info(unsigned int *addr, unsigned int *size);

#endif
