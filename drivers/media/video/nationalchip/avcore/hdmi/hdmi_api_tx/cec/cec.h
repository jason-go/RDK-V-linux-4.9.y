#ifndef __CEC_H__
#define __CEC_H__

#include "hdmi_hal.h"

typedef enum {
  RET_CEC_OK                 = (0),
  RET_CEC_WAKEUP             = (7), 
  RET_CEC_ERROR_FLOW         = (6),
  RET_CEC_ERROR_INITIATOR    = (5),
  RET_CEC_ARB_LOST           = (4),
  RET_CEC_NACK               = (3),
  RET_CEC_EOM                = (2),
  RET_CEC_DONE               = (1),
  RET_CEC_DEVICE_UNREGISTRED = (8),
  RET_CEC_TIMEOUT            = (9),
} CecRet;

CecRet cec_enable     (int enable);
CecRet cec_send_cmd   (GxHdmiCecOpcode  code);
CecRet cec_recv_cmd   (GxHdmiCecOpcode *code, unsigned timeout_ms);
CecRet cec_parse_state(unsigned state);
#endif

