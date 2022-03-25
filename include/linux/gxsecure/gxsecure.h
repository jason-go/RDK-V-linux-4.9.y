/** \addtogroup <label>
 *  @{
 */
#ifndef __GX_SECURE_H__
#define __GX_SECURE_H__

#include "gxse_debug_level.h"

/**
 * 描述scpu固件相关信息
 */
typedef struct {
    unsigned char* loader;       ///< 存放scpu固件的内存首地址
    unsigned int   size;         ///< scpu固件的大小
    unsigned int   dst_addr;     ///< scpu固件的加载地址
} GxSecureLoader;

/**
 * 固件运行状态
 */
typedef enum {
    FW_RUNNING,        ///< scpu固件正常运行
    FW_RESET,          ///< scpu固件出错并且scpu复位
    FW_ERROR,          ///< scpu固件加载出错，应复位acpu
    FW_APP_START,      ///< scpu固件加载运行成功
    FW_CHIP_SERIALIZED,///< 芯片序列化完成
    FW_APP_DEAD,       ///< scpu固件运行时出错,卡死
    FW_PASSWD,         ///< scpu固件进入密码模式
} GxSecureStatus;

/**
 * S5H协议用户信息设置返回值
 */
typedef enum {
	FW_RET_NOERROR,          ///< 用户秘钥设置成功
	FW_RET_FUNC_DISABLE,     ///< 固件没有开启烧写用户秘钥功能
	FW_RET_SERIALIZED,       ///< 该芯片已经完成用户序列化
	FW_RET_ILLEGAL,          ///< 用户秘钥非法
	FW_RET_SAME,             ///< 用户秘钥已经配置
	FW_RET_DIFFERENT,        ///< 用户秘钥已经配置，且输入与上次的不同
	FW_RET_CUSTOMID_ILLEGAL, ///< 用户信息非法
} GxFWProtocolS5HRet;

/**
 * 通用协议私有数据传输返回值
 */
typedef enum {
	FW_RET_RW_OK,            ///< 传输成功
	FW_RET_RW_ERROR,         ///< 传输失败
	FW_RET_RW_RESP,          ///< 表示这次写操作有response
} GxFWProtocolGenericRet;

/**
 * HEVC编码支持的Bit数
 */
typedef enum {
	GXSE_HEVC_SUPPORT_8BIT  = ( 1 << 0),            ///< 支持8bit
	GXSE_HEVC_SUPPORT_10BIT = ( 1 << 1),            ///< 支持10bit
} GxSecureHEVCStatus;

/**
 * MBOX私有数据传输协议
 */
typedef enum {
	MBOX_PROTOCOL_S5H,     ///< 兼容老版本S5H系列固件私有数据传输协议
	MBOX_PROTOCOL_GENERIC, ///< 通用私有数据传输协议
	MBOX_PROTOCOL_IFCP,    ///< IFCP私有数据传输协议
	MBOX_PROTOCOL_MAX,
} GxFWProtocol;

/**
 * 客户化密钥相关信息
 */
typedef struct {
	unsigned int id;         ///< 选择指定第几个用户密钥
	unsigned int value[4];   ///< 设置密钥的值
	unsigned int check_val;  ///< 密钥的校验值
} GxSecureUserKey;

/**
 * 特定的客户私有数据
 */
typedef struct {
	unsigned int id;           ///< 数据id
	unsigned char *buf;        ///< 用来存储用户数据的buffer
	unsigned int size;         ///< buffer的实际大小
	unsigned int check_val;    ///< 用户数据的校验值
	unsigned int userdata_len; ///< 安全固件返回的用户数据的实际大小
	GxFWProtocol protocol;     ///< 传输协议选择
	int          ret;          ///< 实际功能是否执行成功
} GxSecureUserData;

typedef struct {
	unsigned int version[4];
} GxSecureImageVersion;

/**
 * 读取CSSN时使用的结构体
 */
typedef struct {
	unsigned char value[8];
} GxSecureCSSN;

/**
 * 读取CHIPNAME时使用的结构体
 */
typedef struct {
	unsigned char value[12];
} GxSecureChipName;

/**
 * 描述otp读写操作的参数
 */
typedef struct {
	unsigned int addr;    ///< otp读写的地址
	unsigned char *buf;   ///< otp读写数据存放的内存首地址
	unsigned int size;    ///< otp读写数据的大小
} GxSecureOtpBuf;

/**
 * 注册设备"/dev/gxmisc"，仅ecos/nos系统下使用, ecos还可以用MOD_SECURE代替
 *
 * \param void
 *
 * \return void
 */
void register_misc(void);

/**
 * 注册设备"/dev/gxcrypto"，仅ecos/nos系统下使用, ecos还可以用MOD_SECURE代替
 *
 * \param void
 *
 * \return void
 */
void register_crypto(void);

/**
 * 注册设备"/dev/gxklm"，仅ecos/nos系统下使用, ecos还可以用MOD_SECURE代替
 *
 * \param void
 *
 * \return void
 */
void register_klm(void);

/**
 * 注册设备"/dev/gxm2m[n]"，仅ecos/nos系统下使用, ecos还可以用MOD_SECURE代替
 *
 * \param void
 *
 * \return void
 */
void register_m2m(void);

/**
 * 注册设备"/dev/gxsecure"，仅ecos/nos系统下使用, ecos还可以用MOD_SECURE代替
 *
 * \param void
 *
 * \return void
 */
void register_secure(void);

/**
 * 注册设备"/dev/gxfirewall"，仅ecos/nos系统下使用, ecos还可以用MOD_SECURE代替
 *
 * \param void
 *
 * \return void
 */
void register_firewall(void);

#define GXSE_MISC_PATH          "/dev/gxmisc0"

#define SECURE_SEND_LOADER      GXSE_IOW('s', 0, GxSecureLoader)
#define SECURE_GET_STATUS       GXSE_IOR('s', 1, unsigned int)
#define SECURE_SET_BIV          GXSE_IOW('s', 2, GxSecureImageVersion)
#define SECURE_GET_BIV          GXSE_IOR('s', 3, GxSecureImageVersion)
#define SECURE_OTP_READ         GXSE_IOR('s', 4, GxSecureOtpBuf)
#define SECURE_OTP_WRITE        GXSE_IOW('s', 5, GxSecureOtpBuf)
#define SECURE_GET_RNG          GXSE_IOR('s', 6, unsigned int)

#define SECURE_GET_CSSN         GXSE_IOR('s', 7, GxSecureCSSN)
#define SECURE_GET_CHIPNAME     GXSE_IOR('s', 8, GxSecureChipName)
#endif
/** @}*/
