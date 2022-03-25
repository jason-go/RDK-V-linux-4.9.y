#include "pegasus.h"
#include "chip_all.h"

DEFINE_CHIP_REGISTER(pegasus, gx6605s)

DEFINE_DEV_REGISTER(pegasus, gx6605s, klm)
DEFINE_DEV_REGISTER(pegasus, gx6605s, misc)
DEFINE_DEV_REGISTER(pegasus, gx6605s, crypto)

DEFINE_MODULE_REGISTER(pegasus, gx6605s, klm, generic)
DEFINE_MODULE_REGISTER(pegasus, gx6605s, misc, otp)
DEFINE_MODULE_REGISTER(pegasus, gx6605s, misc, chip)
DEFINE_MODULE_REGISTER(pegasus, gx6605s, misc, firewall)
DEFINE_MODULE_REGISTER(pegasus, gx6605s, misc, sci)
DEFINE_MODULE_REGISTER(pegasus, gx6605s, crypto, dma)
