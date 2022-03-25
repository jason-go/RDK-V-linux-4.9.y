#ifndef __SIRIUS_HASH_H__
#define __SIRIUS_HASH_H__

#include "gxdgst_virdev.h"

#define SIRIUS_HASH_ALG_SHA1   (0x1<<0)
#define SIRIUS_HASH_ALG_SHA256 (0x1<<1)
#define SIRIUS_HASH_ALG_SM3    (0x1<<2)

#define SIRIUS_DGST_BLOCKLEN   (64)
#define SIRIUS_SHA1_OUTLEN     (20)
#define SIRIUS_SHA256_OUTLEN   (32)

#endif
