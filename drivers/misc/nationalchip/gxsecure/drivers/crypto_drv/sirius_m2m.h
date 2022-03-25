#ifndef __SIRIUS_M2M_H__
#define __SIRIUS_M2M_H__

#define M2M_CONTENT_LIMIT         (gxse_firewall_get_protect_buffer() & GXFW_BUFFER_AUDIO_FIRMWARE)
#define M2M_IS_HW_PVR_MODE(param) (TFM_FLAG_GET((param)->flags, TFM_FLAG_CRYPT_HW_PVR_MODE))
#define M2M_IS_PVR_MODE(param)    (!M2M_IS_HW_PVR_MODE(param) && (param)->even_key.id == TFM_KEY_SSUK ? 1 : 0)
#define M2M_IS_FIRM_MODE(param)   ((param)->even_key.id == TFM_KEY_OTP_FIRM ? 1 : 0)
#define M2M_IS_NORMAL_MODE(param) (!M2M_IS_HW_PVR_MODE(chan) && !M2M_IS_PVR_MODE(chan) && !M2M_IS_FIRM_MODE(chan))

#define M2M_CONF_SOD      (0x1<<1)
#define M2M_CONF_EOD      (0x1<<0)
#define GXM2M_SECMEM_LEN  (0x20000)


typedef enum {
	M2M_STATUS_IDLE,
	M2M_STATUS_RUNNING,
	M2M_STATUS_DONE,
	M2M_STATUS_ERR,
	M2M_STATUS_SPACE_ERR,
	M2M_STATUS_ALMOST_EMPTY,
	M2M_STATUS_ALMOST_FULL,
} M2MStatus;

#define SIRIUS_M2M_MAX_MOD       (8)

#define SIRIUS_M2M_ALG_AES128    (0)
#define SIRIUS_M2M_ALG_TDES2K    (1)
#define SIRIUS_M2M_ALG_DES       (2)
#define SIRIUS_M2M_ALG_AES192    (3)
#define SIRIUS_M2M_ALG_AES256    (4)
#define SIRIUS_M2M_ALG_SM4       (5)

#define SIRIUS_M2M_OPT_ECB       (0)
#define SIRIUS_M2M_OPT_CBC       (1)

#define SIRIUS_M2M_KEY_RK_ADDR0  (0)
#define SIRIUS_M2M_KEY_RK_ADDR1  (1)
#define SIRIUS_M2M_KEY_RK_ADDR2  (2)
#define SIRIUS_M2M_KEY_RK_ADDR3  (3)
#define SIRIUS_M2M_KEY_RK_ADDR4  (4)
#define SIRIUS_M2M_KEY_RK_ADDR5  (5)
#define SIRIUS_M2M_KEY_RK_ADDR6  (6)
#define SIRIUS_M2M_KEY_RK_ADDR7  (7)
#define SIRIUS_M2M_KEY_RK_ADDR8  (8)
#define SIRIUS_M2M_KEY_RK_ADDR9  (9)
#define SIRIUS_M2M_KEY_RK_ADDR10 (10)
#define SIRIUS_M2M_KEY_RK_ADDR11 (11)
#define SIRIUS_M2M_KEY_RK_ADDR12 (12)
#define SIRIUS_M2M_KEY_RK_ADDR13 (13)
#define SIRIUS_M2M_KEY_RK_ADDR14 (14)
#define SIRIUS_M2M_KEY_RK_ADDR15 (15)
#define SIRIUS_M2M_KEY_SSUK      (16)
#define SIRIUS_M2M_KEY_ACPU_SOFT (17)
#define SIRIUS_M2M_KEY_SCPU_SOFT (18)
#define SIRIUS_M2M_KEY_OTP_SOFT  (19)
#define SIRIUS_M2M_KEY_OTP_FIRM  (20)

#endif
