#ifndef __GXSECURE_PROTOCOL_H__
#define __GXSECURE_PROTOCOL_H__

#include "../gxsecure_virdev.h"

typedef int32_t (*_SecureCB)(GxSeModuleHwObj *obj, GxSecureUserData *param);
typedef struct {
	_SecureCB tx;
	_SecureCB rx;
} GxSecureProtocol;

extern GxSecureProtocol secure_protocol_generic;
extern GxSecureProtocol secure_protocol_S5H;
extern GxSecureProtocol secure_protocol_IFCP;

#endif

