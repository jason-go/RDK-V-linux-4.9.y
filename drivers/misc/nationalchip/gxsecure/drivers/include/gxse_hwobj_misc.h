#ifndef _GX_MISC_H_
#define _GX_MISC_H_

/************************************************************************
 *                         Sirius OTP MAP
 *
 * {SECT0_0, 0x0000~0x00A7, OTP_AUTH_NONE, OTP_AUTH_NONE}, // Irdeto private key
 * {SECT0_1, 0x0800~0x08FF, OTP_AUTH_NONE, OTP_AUTH_NONE}, // Chip private key
 * {SECT1  , 0x00A8~0x00AF, OTP_AUTH_NONE, OTP_AUTH_BOTH}, // 55AA address
 * {SECT2  , 0x00B0~0x00DF, OTP_AUTH_NONE, OTP_AUTH_BOTH}, // Chip information and some security flag will program in test stage
 * {SECT3  , 0x00E0~0x01FF, OTP_AUTH_BOTH, OTP_AUTH_BOTH}, // Some security flag can program in system by ACPU and SCPU
 *                                                            Some user data can be used by ACPU or SCPU
 * {SECT4  , 0x0200~0x02FF, OTP_AUTH_NONE, OTP_AUTH_BOTH}, // ACPU internal RSA public key
 * {SECT5  , 0x0300~0x03FF, OTP_AUTH_NONE, OTP_AUTH_SCPU}, // SCPU internal RSA public key
 * {SECT6  , 0x0400~0x043F, OTP_AUTH_SCPU, OTP_AUTH_BOTH}, // SCPU version DATA
 * {SECT7_0, 0x0440~0x047F, OTP_AUTH_SCPU, OTP_AUTH_SCPU}, // CA_data(1)
 * {SECT7_1, 0x0480~0x04FF, OTP_AUTH_SCPU, OTP_AUTH_SCPU}, // CA_data(2)
 * {SECT7_2, 0x0500~0x07FF, OTP_AUTH_SCPU, OTP_AUTH_SCPU}, // CA_data(3)
 * {SECT8  , 0x0900~0x13FF, OTP_AUTH_BOTH, OTP_AUTH_BOTH}, // Switch market ID and TBD for ACPU
 * {SECT9  , 0x1400~0x1FFF, OTP_AUTH_SCPU, OTP_AUTH_SCPU}, // TBD for SCPU
 *
 ***************************************************************************/

/************************************************************************
 *                         Taurus OTP MAP
 *
 * {SECT0  , 0x0000~0x00FF, OTP_AUTH_NONE, OTP_AUTH_NONE}, // private key
 * {SECT1  , 0x0100~0x012F, OTP_AUTH_NONE, OTP_AUTH_BOTH}, // 55AA address & Security flag
 * {SECT2  , 0x0130~0x015F, OTP_AUTH_BOTH, OTP_AUTH_BOTH}, // Security flag
 * {SECT3  , 0x0160~0x01DF, OTP_AUTH_SCPU, OTP_AUTH_SCPU}, // SCPU CAS KEY
 * {SECT4  , 0x01E0~0x02FF, OTP_AUTH_BOTH, OTP_AUTH_BOTH}, // Open for customer
 *
 * SCPU internal RSA public key region
 * {SECT5  , 0x0300~0x03FF, OTP_AUTH_NONE, OTP_AUTH_BOTH}, // if security boot enable
 * {SECT5  , 0x0300~0x03FF, OTP_AUTH_BOTH, OTP_AUTH_BOTH}, // if security boot disable
 *
 ***************************************************************************/

#include "gxse_core.h"
#include "gxfw_uapi.h"
#include "gxmisc_uapi.h"
#include "gxse_kapi_misc.h"
#include "gxse_kapi_firewall.h"

#define GXSE_MISC_CSSN_LEN     (8)
#define GXSE_MISC_CHIPNAME_LEN (12)

#define GXSE_FW_FLAG_TEE  (0x1)

typedef struct {
	uint32_t addr;
	uint32_t size;
	uint32_t wr;
	uint32_t rd;
	uint32_t flags;
} GxSeFirewallBuffer;

typedef enum {
    GXSE_SENSOR_TYPE_SCPU0,
    GXSE_SENSOR_TYPE_SCPU1,
    GXSE_SENSOR_TYPE_SCPU2,
    GXSE_SENSOR_TYPE_SCPU3,
    GXSE_SENSOR_TYPE_XTAL0,
    GXSE_SENSOR_TYPE_XTAL1,
    GXSE_SENSOR_TYPE_XTAL2,
    GXSE_SENSOR_TYPE_CLOCK0,
    GXSE_SENSOR_TYPE_CLOCK1,
    GXSE_SENSOR_TYPE_CLOCK2,
    GXSE_SENSOR_TYPE_KEY_ERR,
    GXSE_SENSOR_TYPE_OTP_ERR,
    GXSE_SENSOR_TYPE_MAX,
} GxSeSensorType;

typedef struct {
    uint32_t err_status;
    uint32_t err_counter;
} GxSeSensorErrStatus;

typedef enum {
    MISC_KEY_SCPU_CW,
    MISC_KEY_SCPU_GP,
    MISC_KEY_SCPU_PVR,
    MISC_KEY_SCPU_SOFT,
} GxSeScpuKeyType;

typedef struct {
    GxSeScpuKeyType type;
    unsigned int  length;
    unsigned char value[32];
} GxSeScpuKey;

typedef enum {
    GXSE_MULTIPIN_SWITCH,
    GXSE_MULTIPIN_RECOVER,
} GxSeMultipinStatus;

#define FIREWALL_SET_PROTECT_BUFFER    GXSE_IOR('t', 14, GxSeFirewallBuffer)
#define MISC_UDELAY                    GXSE_IOW('m',  0, unsigned int)
#define MISC_SENSOR_GET_STATUS         GXSE_IOR('m',  1, GxSeSensorErrStatus)
#define MISC_SENSOR_GET_VALUE          GXSE_IOR('m',  2, unsigned int)
#define MISC_CHIP_SCPU_RESET           GXSE_IO ('m',  3)
#define MISC_CHIP_SEND_KEY             GXSE_IOW('m',  4, GxSeScpuKey)
#define MISC_CHIP_GET_BIV              GXSE_IOR('m',  5, GxSecureImageVersion)
#define MISC_CHIP_SWITCH_MULTIPIN      GXSE_IOW('m',  6, GxSeMultipinStatus)
#ifdef CPU_ACPU
#define misc_fw    misc_u.fw_ops
#define misc_otp   misc_u.otp_ops
#define misc_rng   misc_u.rng_ops
#define misc_chip  misc_u.chip_ops
#define misc_sci   misc_u.sci_ops
#else
#define misc_otp   otp_ops
#define misc_rng   rng_ops
#define misc_chip  chip_ops
#define misc_timer timer_ops
#define misc_sensor sensor_ops
#endif

typedef struct {
	int32_t (*get_protect_buffer)  (GxSeModuleHwObj *obj, uint32_t *param);
	int32_t (*set_protect_buffer)  (GxSeModuleHwObj *obj, GxSeFirewallBuffer *param);
	int32_t (*query_access_align)  (GxSeModuleHwObj *obj, uint32_t *param);
	int32_t (*query_protect_align) (GxSeModuleHwObj *obj, uint32_t *param);
} GxMiscFirewallOps;

typedef struct {
	int32_t (*otp_read)  (GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size);
	int32_t (*otp_write) (GxSeModuleHwObj *obj, uint32_t addr, uint8_t *buf, uint32_t size);
} GxMiscOTPOps;

typedef struct {
	int32_t (*rng_request)       (GxSeModuleHwObj *obj, uint32_t *val);
} GxMiscRNGOps;

typedef struct {
	int32_t (*udelay)            (GxSeModuleHwObj *obj, uint32_t us);
} GxMiscTimerOps;

typedef struct {
	int32_t (*get_err_status)    (GxSeModuleHwObj *obj, GxSeSensorErrStatus *param);
	int32_t (*get_err_value)     (GxSeModuleHwObj *obj, uint32_t *status);
} GxMiscSensorOps;

typedef struct {
	int32_t (*set_param)    (GxSeModuleHwObj *obj, GxSciParam *param);
	int32_t (*get_param)    (GxSeModuleHwObj *obj, GxSciParam *param);
	int32_t (*ICC_reset)    (GxSeModuleHwObj *obj);
	int32_t (*ICC_poweroff) (GxSeModuleHwObj *obj);
	int32_t (*get_status)   (GxSeModuleHwObj *obj, GxSciCardStatus *status);
	int32_t (*print_reg)    (GxSeModuleHwObj *obj);
} GxMiscSCIOps;

typedef struct {
#ifdef CPU_ACPU
	int32_t (*get_CSSN)          (GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
	int32_t (*get_chipname)      (GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
	int32_t (*switch_multipin)   (GxSeModuleHwObj *obj, GxSeMultipinStatus status);
#else
	int32_t (*chip_reset)        (GxSeModuleHwObj *obj);
	int32_t (*get_BIV)           (GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
	int32_t (*send_secure_key)   (GxSeModuleHwObj *obj, GxSeScpuKeyType type, uint8_t *buf, uint32_t size);
	int32_t (*send_m2m_soft_key) (GxSeModuleHwObj *obj, uint8_t *buf, uint32_t size);
#endif
} GxMiscChipOps;

typedef struct {
	void *devops;
	void *hwops;

#ifdef CPU_ACPU
	union {
		GxMiscFirewallOps fw_ops;
		GxMiscOTPOps      otp_ops;
		GxMiscRNGOps      rng_ops;
		GxMiscChipOps     chip_ops;
		GxMiscSCIOps      sci_ops;
	} misc_u;
#else
	GxMiscOTPOps      otp_ops;
	GxMiscRNGOps      rng_ops;
	GxMiscChipOps     chip_ops;
	GxMiscTimerOps    timer_ops;
	GxMiscSensorOps   sensor_ops;
#endif
} GxSeModuleHwObjMiscOps;

/*************************************** Interface *********************************************/

int gxse_switch_multipin(GxSeMultipinStatus status);
// Scpu
int gxsecure_otp_get_fixed_klm_stage(GxTfmKeyBuf *key, int stage);
void gxse_sensor_get_err_status (GxSeSensorErrStatus *param);
unsigned int gxse_sensor_get_err_value (unsigned int status);
void gxse_misc_scpu_reset(void);
void gxse_misc_send_key(GxSeScpuKey *key);
int gxse_misc_get_BIV(GxSecureImageVersion *biv);

#define gxse_fuse_get_fixed_klm_stage    gxsecure_otp_get_fixed_klm_stage
#define gxse_fuse_read                   gxsecure_otp_read
#define gxse_fuse_write                  gxsecure_otp_write
#define gxse_fuse_read_buf               gxsecure_otp_read_buf
#define gxse_fuse_write_buf              gxsecure_otp_write_buf
#define gxse_fuse_get_hevc_status        gxsecure_otp_get_hevc_status
#define gxse_fuse_get_chipname           gxsecure_otp_get_chipname
#define gxse_fuse_get_publicid           gxsecure_otp_get_publicid
#define gxse_fuse_get_marketid           gxsecure_otp_get_marketid
#define gxse_fuse_get_macrovision_status gxsecure_otp_get_macrovision_status
#define gxse_fuse_get_hdr_status         gxsecure_otp_get_hdr_status
#define gxsecure_rng_request             gxse_rng_request

#endif
