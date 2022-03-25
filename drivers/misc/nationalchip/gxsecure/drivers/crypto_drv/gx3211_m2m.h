#ifndef __GX3211_M2M_H__
#define __GX3211_M2M_H__

extern gx_mutex_t gx3211_mtc_mutex;
#define GX3211_M2M_KEY_ACPU_SOFT   (0)
#define GX3211_M2M_KEY_OTP         (1)
#define GX3211_M2M_KEY_NDS         (2)
#define GX3211_M2M_KEY_AES_CP      (3)
#define GX3211_M2M_KEY_TDES_CP     (4)
#define GX3211_M2M_KEY_MULTI_LAYER (5)

int32_t gx3211_m2m_get_key(GxTfmKeyBuf *param);

#endif
