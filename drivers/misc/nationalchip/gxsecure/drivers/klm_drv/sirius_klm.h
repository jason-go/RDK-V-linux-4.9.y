#ifndef __SIRIUS_KLM_H__
#define __SIRIUS_KLM_H__

#define SIRIUS_KLM_ALG_TDES  (0)
#define SIRIUS_KLM_ALG_AES   (1)
#define SIRIUS_KLM_ALG_T3DES (2)
#define SIRIUS_KLM_ALG_TAES  (3)
#define SIRIUS_KLM_ALG_IPA   (4)

#define SIRIUS_KLM_DST_TS    (0)
#define SIRIUS_KLM_DST_GP    (1)

#define SIRIUS_KLM_KEY_CWUK  (0)
#define SIRIUS_KLM_KEY_CPUK  (1)
#define SIRIUS_KLM_KEY_TAUK  (2)

#define SIRIUS_KLM_KEY_DSK1  (0)
#define SIRIUS_KLM_KEY_DSK2  (1)
#define SIRIUS_KLM_KEY_RK1   (2)
#define SIRIUS_KLM_KEY_RK2   (3)

#define SIRIUS_KLM_KEY_SCPU  (7)

#define SIRIUS_KLM_STAGE(n)  (n-2)

extern gx_mutex_t sirius_klm_mutex;

#endif
