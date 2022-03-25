#ifndef __GXSE_HWOBJ_SECURE_H__
#define __GXSE_HWOBJ_SECURE_H__

#include "gxsecure.h"
#include "gxse_core.h"
#include "gxse_kapi_secure.h"

#define FW_HEAD_TOTAL_LEN      (64)
#define FW_HEAD_MAGIC          ("BEAF")
#define FW_HEAD_MAGIC_POS      (0)
#define FW_HEAD_MAGIC_LEN      (4)
#define FW_HEAD_VERSION        ("version")
#define FW_HEAD_VERSION_POS    (4)
#define FW_HEAD_VERSION_LEN    (24)
#define FW_HEAD_CHIP           ("chip")
#define FW_HEAD_CHIP_POS       (28)
#define FW_HEAD_CHIP_LEN       (16)
#define FW_HEAD_VENDOR         ("vendor")
#define FW_HEAD_VENDOR_POS     (44)
#define FW_HEAD_VENDOR_LEN     (16)

#define FW_VAL_APP_FORMAL      (0x0)
#define FW_VAL_APP_CUSTOM      (0xc)
#define FW_VAL_CHIP_SERIALIZED (0x88156088)
#define FW_VAL_APP_PASSWD      (0x88156089)

#define SECURE_EVENT_W         (1)
#define SECURE_EVENT_R         (2)

#define SECURE_SYNC_BYTE       (0xF7)

#define SECURE_USER_RX         GXSE_IOR('s', 11, GxSecureUserData)
#define SECURE_USER_TX         GXSE_IOW('s', 12, GxSecureUserData)
#define SECURE_SEND_ROOTKEY    GXSE_IOW('s', 13, GxTfmKlm)
#define SECURE_SEND_KN         GXSE_IOW('s', 14, GxTfmKlm)
#define SECURE_SEND_CW         GXSE_IOW('s', 15, GxTfmKlm)
#define SECURE_GET_RESP        GXSE_IOR('s', 16, GxTfmKlm)
#define SECURE_SEND_IRDETO_CMD GXSE_IOW('s', 17, GxTfmKlm)
#define SECURE_SEND_USER_KEY   GXSE_IOW('s', 18, GxSecureUserKey)
#define SECURE_DIRECT_RX       GXSE_IOR('s', 19, GxSecurePacket)
#define SECURE_DIRECT_TX       GXSE_IOW('s', 20, GxSecurePacket)

typedef struct {
	GxFWProtocolGenericRet type;
	unsigned int   len;
	unsigned char  buffer[96];
} GxSecureResult;

typedef struct {
	unsigned char  sync_byte;
	unsigned char  id;
	unsigned short size;
} GxSecureUserMsgHead;

typedef struct {
	unsigned int len;
	unsigned int oob;
	unsigned int data[32];
} GxSecurePacket;

typedef enum {
	FW_PASSWD_NOT_PASS,
	FW_PASSWD_PASS,
	FW_PASSWD_NOT_SUPPORT,
} GxSecurePasswdType;

typedef enum {
	/* common oob */
	MB_OOB_DATA_LEN = 0,
	MB_OOB_ACK,
	MB_OOB_APP_START,
	MB_OOB_APP_RESTART,

	/* common oob need to be consoled by app */
	MB_OOB_APP_IS_ALIVE = 0x90,

	/* irdeto add oob */
	MB_OOB_IFCP = 0xa0,

	/* common firmware function */
	MB_OOB_USER_TX = 0xc0,
	MB_OOB_USER_RX,
	MB_OOB_USER_CFG_DDR_INFO,

	/* key trans oob */
	MB_OOB_ACPU_SELECT_ROOT_KEY = 0xd0,
	MB_OOB_ACPU_SET_KN,
	MB_OOB_ACPU_SET_CW,
	MB_OOB_ACPU_GET_RESP,
	MB_OOB_ACPU_NEED_MISC_CW,
	MB_OOB_ACPU_NEED_MISC_GP,
	MB_OOB_ACPU_NEED_MISC_PVR,
	MB_OOB_ACPU_NEED_MISC_SOFT,
	MB_OOB_ACPU_SET_IRDETO_CMD_INFO,
	MB_OOB_ACPU_SET_IRDETO_CMD,
	MB_OOB_ACPU_SET_IRDETO_CMD_END,
	MB_OOB_SCPU_SEND_RESULT,

	/* autotest oob */
	MB_OOB_CASE = 0xe0,
	MB_OOB_CASE_FINISH,

	/* rom oob */
	MB_OOB_APP_ADDR = 0xf0,
	MB_OOB_APP,
	MB_OOB_SIG,
	MB_OOB_CRC,
	reserved_0,
	MB_OOB_ROM_RET_ROM_START,
	MB_OOB_ROM_RET_CHECK_ACK,
	reserved_1,
	reserved_2,
	reserved_3,
	MB_OOB_ACPU_START,
} GxMBType;

typedef struct {
	void *devops;
	void *hwops;

#ifdef CPU_ACPU
	int32_t (*load_firmware)   (GxSeModuleHwObj *obj, GxSecureLoader *param);
	int32_t (*get_status)      (GxSeModuleHwObj *obj, uint32_t *status);
	int32_t (*set_BIV)         (GxSeModuleHwObj *obj, GxSecureImageVersion *param);
	int32_t (*get_BIV)         (GxSeModuleHwObj *obj, GxSecureImageVersion *param);
	int32_t (*send_rootkey)    (GxSeModuleHwObj *obj, GxTfmKlm *param);
	int32_t (*send_kn)         (GxSeModuleHwObj *obj, GxTfmKlm *param);
	int32_t (*send_cw)         (GxSeModuleHwObj *obj, GxTfmKlm *param);
	int32_t (*send_irdeto_cmd) (GxSeModuleHwObj *obj, GxTfmKlm *param);
	int32_t (*get_resp)        (GxSeModuleHwObj *obj, GxTfmKlm *param);
	int32_t (*send_user_key)   (GxSeModuleHwObj *obj, GxSecureUserKey *param);

#else
	int32_t (*direct_tx)       (GxSeModuleHwObj *obj, GxSecurePacket *param);
	int32_t (*direct_rx)       (GxSeModuleHwObj *obj, GxSecurePacket *param);
#endif

} GxSeModuleHwObjSecureOps;

#ifdef CPU_SCPU
void gx_mbox_notice_acpu_ack(void);
void gx_mbox_notice_acpu_result(GxSecureResult *result);
void gx_mbox_notice_acpu_before_reset_scpu(void);
void gx_mbox_notice_acpu_chip_serialized(void);
int gx_mbox_read(GxSecurePacket *pkt, int mbox_is_tee);
int gx_mbox_write(GxSecurePacket *pkt, int mbox_is_tee);
#endif

#endif
