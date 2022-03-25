#ifndef __GX_TFM_UAPI_H__
#define __GX_TFM_UAPI_H__

#include "gxtfm.h"

int gx_tfm_encrypt            ( GxTfmCrypto *param );
int gx_tfm_decrypt            ( GxTfmCrypto *param );
int gx_tfm_verify             ( GxTfmVerify *param );
int gx_tfm_dgst               ( GxTfmDgst *param );
int gx_tfm_dgst_init          ( GxTfmDgst *param );
int gx_tfm_dgst_update        ( GxTfmDgst *param );
int gx_tfm_dgst_final         ( GxTfmDgst *param );

/* For OPTEE */
int gx_ltc_crypt(int alg, char enc, const unsigned char *src, unsigned char *dst,
        unsigned long blocks, unsigned char *key, unsigned int keyLen, unsigned char *IV);

#endif
