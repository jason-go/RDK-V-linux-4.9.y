#ifndef __GXSE_DEBUG_LEVEL_H__
#define __GXSE_DEBUG_LEVEL_H__

typedef enum {
	GXSE_LOG_MOD_DEV,            ///< 设备层
	GXSE_LOG_MOD_HAL,            ///< HAL层
	GXSE_LOG_MOD_KLM,            ///< KLM驱动层
	GXSE_LOG_MOD_CRYPTO,         ///< Crypto驱动层
	GXSE_LOG_MOD_DGST,           ///< Hash驱动层
	GXSE_LOG_MOD_AKCIPHER,       ///< Hash驱动层
	GXSE_LOG_MOD_SECURE,         ///< Mailbox驱动层
	GXSE_LOG_MOD_MISC_FIREWALL,  ///< Misc firewall驱动层
	GXSE_LOG_MOD_MISC_CHIP,      ///< Misc chip config驱动层
	GXSE_LOG_MOD_MISC_OTP,       ///< Misc otp驱动层
	GXSE_LOG_MOD_MISC_RNG,       ///< Misc rng驱动层
	GXSE_LOG_MOD_MISC_SCI,       ///< Misc SCI驱动层
	GXSE_LOG_MOD_MAX,            ///< 打印模块枚举最大值
} GxSeLogModule;

typedef enum {
	GXSE_DEBUG_LEVEL_0,      ///< 无打印
	GXSE_DEBUG_LEVEL_1,      ///< 打印 [ERROR]
	GXSE_DEBUG_LEVEL_2,      ///< 打印 [ERROR] | [INFO]
	GXSE_DEBUG_LEVEL_3,      ///< 打印 [ERROR] | [INFO] | [WARNING]
	GXSE_DEBUG_LEVEL_4,      ///< 打印 [ERROR] | [INFO] | [WARNING] | [DEBUG]
} GxSeDebugLevel;

typedef struct {
	GxSeLogModule   module;  ///< 选择将要配置的打印模块
	GxSeDebugLevel  level;   ///< 选择将要配置的打印等级
} GxSecureDebug;

#define GXSE_MASK          (0xfff)
#define GXSE_IOC_NONE      (0x20000000)
#define GXSE_IOC_READ      (0x40000000)
#define GXSE_IOC_WRITE     (0x80000000)
#define GXSE_IOC_WR        (GXSE_IOC_READ | GXSE_IOC_WRITE)

#define GXSE_IOC(input, group, num, len) \
	((unsigned long)(input | ((len & 0xfff) << 16) | (((group) << 8) | (num))))
#define GXSE_IO(g, n)        GXSE_IOC(GXSE_IOC_NONE, (g), n, 0)
#define GXSE_IOR(g, n, t)    GXSE_IOC(GXSE_IOC_READ, (g), n, sizeof(t))
#define GXSE_IOW(g, n, t)    GXSE_IOC(GXSE_IOC_WRITE, (g), n, sizeof(t))
#define GXSE_IOWR(g, n, t)   GXSE_IOC(GXSE_IOC_WR, (g), n, sizeof(t))
#define GXSE_IOLEN(cmd)      ((cmd>>16) & 0xfff)

#define SECURE_SET_DEBUG       GXSE_IOW('s', 9, GxSecureDebug)

#endif
