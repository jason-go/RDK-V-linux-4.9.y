/** \addtogroup <label>
 *  @{
 */
#ifndef __GX_TFM_H__
#define __GX_TFM_H__

#include "gxse_debug_level.h"

/**
 * TFM支持硬件模块类型,不同芯片支持模块不同
 */
typedef enum {
	TFM_MOD_KLM_SCPU_STATIC,      ///< scpu 静态KLM 模块
	TFM_MOD_KLM_SCPU_DYNAMIC,     ///< scpu 动态KLM 模块
	TFM_MOD_KLM_SCPU_NC_DYNAMIC,  ///< scpu 国芯动态KLM 模块
	TFM_MOD_KLM_IRDETO,           ///< acpu irdeto KLM 模块
	TFM_MOD_KLM_GENERAL,          ///< acpu 通用KLM 模块
	TFM_MOD_KLM_MTC,              ///< acpu MTC klm模块

	TFM_MOD_CRYPTO,               ///< acpu crypto 模块
	TFM_MOD_M2M,                  ///< acpu m2m/mtc 模块
	TFM_MOD_MISC,                 ///< acpu misc 模块
	TFM_MOD_MAX,                  ///< 最大模块数
} GxTfmModule;

/**
 * TFM支持key类型,不同模块使用的key不同
 *
 *	KLM 相关 key, 仅KLM使用:
 *	TFM_KEY_CWUK,
 *	TFM_KEY_PVRK,
 *	TFM_KEY_TAUK,
 *
 *	CRYPTO 相关 key, 仅CRYPTO使用:
 *	TFM_KEY_BCK,
 *	TFM_KEY_IK,
 *	TFM_KEY_CCCK,
 *
 *	M2M 相关 key, 仅M2M使用:
 *	TFM_KEY_RK,
 *	TFM_KEY_SSUK,
 *	TFM_KEY_MULTI,
 *	TFM_KEY_MEMORY,
 *	TFM_KEY_NDS,
 *	TFM_KEY_CDCAS,
 *
 *	scpu crypto/klm 相关 rootkey,仅scpu国芯动态KLM模块使用:
 *	TFM_KEY_CSGK2,
 *	TFM_KEY_SIK,
 *	TFM_KEY_ROOTKEY,
 *	TFM_KEY_SMK,
 *	TFM_KEY_TK,
 *
 *	generic key from otp:
 *	TFM_KEY_OTP_SOFT,
 *	TFM_KEY_OTP_FIRM,
 *	TFM_KEY_OTP_FLASH,
 *
 *	generic key from acpu/scpu:
 *	TFM_KEY_ACPU_SOFT,
 *	TFM_KEY_SCPU_SOFT,
 *	TFM_KEY_SCPU_KLM,
 *	TFM_KEY_SCPU_MISC,
 *	TFM_KEY_REG,
 *
 */
typedef enum {

	TFM_KEY_CWUK,       ///< klm generate CW/GPCW
	TFM_KEY_PVRK,       ///< klm generate PVRK
	TFM_KEY_TAUK,       ///< klm update TD param

	TFM_KEY_BCK,        ///< acpu crypto decrypt loader
	TFM_KEY_IK,         ///< acpu crypto decrypt app
	TFM_KEY_CCCK,       ///< get chip config value，只支持加密, 可以调用GxTfm_ClearKey()清除

	TFM_KEY_RK,        ///< 可以调用GxTfm_ClearKey()清除
	TFM_KEY_SSUK,      ///< 可以调用GxTfm_ClearKey()清除
	TFM_KEY_MULTI,
	TFM_KEY_MEMORY,
	TFM_KEY_NDS,
	TFM_KEY_CDCAS,

	TFM_KEY_OTP_SOFT,
	TFM_KEY_OTP_FIRM,   ///< 只支持解密，alg只支持aes128
	TFM_KEY_OTP_FLASH,  ///< 只支持解密,可以调用GxTfm_DisableKey()禁用,GxTfm_EnableKey()使能

	TFM_KEY_ACPU_SOFT,
	TFM_KEY_SCPU_SOFT,
	TFM_KEY_SCPU_KLM,
	TFM_KEY_SCPU_MISC,
	TFM_KEY_REG,

	TFM_KEY_CSGK2,
	TFM_KEY_SIK,
	TFM_KEY_ROOTKEY,
	TFM_KEY_SMK,
	TFM_KEY_TK,
	TFM_KEY_MAX,
} GxTfmKey;

/**
 * TFM支持的所有的算法
 */
typedef enum {
	TFM_ALG_AES128,
	TFM_ALG_AES192,
	TFM_ALG_AES256,
	TFM_ALG_DES,
	TFM_ALG_TDES,
	TFM_ALG_T3DES,
	TFM_ALG_TAES,
	TFM_ALG_SM4,
	TFM_ALG_SM2,
	TFM_ALG_SM3,
	TFM_ALG_SHA1,
	TFM_ALG_SHA256,
	TFM_ALG_SM2_ZA,
	TFM_ALG_SM3_HMAC,
	TFM_ALG_SHA256_HMAC,
} GxTfmAlg;

/**
 * TFM支持的所有数据来源
 */
typedef enum {
	TFM_SRC_MEM, ///< 数据来源于内存，即软件通过CPU配置数据
	TFM_SRC_REG, ///< 数据来源于寄存器，一般指私有寄存器，cpu无法直接获取
} GxTfmSrc;

/**
 * TFM支持的所有数据结果输出目标
 */
typedef enum {
	TFM_DST_TS,  ///< 数据结果输出给dumux模块
	TFM_DST_GP,  ///< 数据结果输出给GP模块
	TFM_DST_M2M, ///< 数据结果输出给M2M模块
	TFM_DST_MEM, ///< 数据结果输出给内存，即cpu可访问
	TFM_DST_REG, ///< 数据结果输出给私有寄存器，cpu无法访问
} GxTfmDst;

/**
 * TFM支持的所有加解密模式
 */
typedef enum {
	TFM_OPT_ECB, ///< 电码本模式
	TFM_OPT_CBC, ///< 密码分组链接模式，需要IV
	TFM_OPT_CFB, ///< 密码反馈模式，需要IV
	TFM_OPT_OFB, ///< 输出反馈模式，需要IV
	TFM_OPT_CTR, ///< 计数器模式，需要counter
} GxTfmOpt;

/**
 * 描述数据buffer的相关信息
 */
typedef struct {
	unsigned char *buf;     ///< 数据buffer的首地址
	unsigned int   length;  ///< 数据buffer的长度
} GxTfmBuf;

/**
 * 描述数据来源的相关信息
 */
typedef struct {
	GxTfmSrc       id;  ///< 选择数据来源的类型
	unsigned char  sub; ///< 若选择的来源类型存在不同的子项，则选择具体子项
} GxTfmSrcBuf;

/**
 * 描述数据来源的相关信息
 */
typedef struct {
	GxTfmDst       id;  ///< 选择数据结果输出目标
	unsigned char  sub; ///< 若选择的数据结果输出目标存在不同的子项，则选择具体子项
} GxTfmDstBuf;

/**
 * 描述密钥来源的相关信息
 */
typedef struct {
	GxTfmKey       id;   ///< 选择密钥的类型
	unsigned char  sub;  ///< 若选择的密钥类型存在多个密钥，则选择具体密钥
} GxTfmKeyBuf;

/**
 * TFM框架中各个模块中特殊控制位的集合
 */
#define TFM_FLAG_BIG_ENDIAN            (1 << 0)   ///< 配置外部需要通过软件配置数据的大小端
#define TFM_FLAG_KEY_BIG_ENDIAN        (1 << 17)  ///< 配置外部需要通过软件配置密钥/IV的大小端
#define TFM_FLAG_ISR_QUERY             (1 << 24)  ///< timeout查询模式

#define TFM_FLAG_KLM_CW_XOR            (1 << 1)   ///< klm 模块输出异或控制
#define TFM_FLAG_KLM_CW_HALF           (1 << 2)   ///< klm 模块输出有效长度控制
#define TFM_FLAG_KLM_CW_HIGH_8BYTE     (1 << 3)   ///< klm 模块输出高低字节控制
#define TFM_FLAG_KLM_INPUT_HALF        (1 << 4)   ///< klm 模块输入有效长度控制
#define TFM_FLAG_KLM_COMBINE_CW         (1 << 5)   ///< mtc klm 产生的16字节cw是组合cw

#define TFM_FLAG_CRYPT_DES_IV_SHIFT    (1 << 5)   ///< mtc des算法IV选择
#define TFM_FLAG_CRYPT_EVEN_KEY_VALID  (1 << 6)   ///< m2m 模块选择even key 有效
#define TFM_FLAG_CRYPT_ODD_KEY_VALID   (1 << 7)   ///< m2m 模块选择ood key 有效, even key和ood key 可以同时选择
#define TFM_FLAG_CRYPT_TS_PACKET_MODE  (1 << 8)   ///< m2m/mtc 模块选择使用TS_PACKET模式进行加解密，对TS流进行处理
#define TFM_FLAG_CRYPT_SWITCH_CLR      (1 << 9)   ///< m2m模块 2 key模式切换初始化
#define TFM_FLAG_CRYPT_SHORT_MSG_IV1   (1 << 10)  ///< 短消息处理模式，使用IV1进行处理
#define TFM_FLAG_CRYPT_SHORT_MSG_IV2   (1 << 11)  ///< 短小时处理模式，使用IV2进行处理
#define TFM_FLAG_CRYPT_RESIDUAL_CTS    (1 << 12)  ///< 选择残差处理模式，CTS模式
#define TFM_FLAG_CRYPT_RESIDUAL_RBT    (1 << 13)  ///< 选择残差处理模式，RBT模式
#define TFM_FLAG_CRYPT_SET_AUDIO_PVRK  (1 << 14)  ///< 更新音频PVRkey时需要配置该flag，MTC TS解扰功能使用
#define TFM_FLAG_CRYPT_PVRK_FROM_ACPU  (1 << 15)  ///< 更新软件PVRkey时需要配置该flag
#define TFM_FLAG_CRYPT_HW_PVR_MODE     (1 << 16)  ///< 硬件PVR模式（SRC/DST中至少有一个是物理地址）

#define TFM_FLAG_HASH_DMA_EN           (1 << 20)  ///< hash模块 DMA 使能

/**
 * 密钥派生操作中所用到的参数
 */
typedef struct {
	GxTfmModule    module;            ///<  选择进行密钥派生的klm模块
	GxTfmAlg       alg;               ///<  选择进行密钥派生的算法
	GxTfmSrcBuf    src;               ///<  选择进行密钥派生的源数据来源
	GxTfmDstBuf    dst;               ///<  选择进行密钥派生的结果数据输出目标
	GxTfmKeyBuf    key;               ///<  选择进行密钥派生的密钥来源
	GxTfmBuf       input;             ///<  若源数据来源于内存，需要配置input buffer的内存数据
	GxTfmBuf       output;            ///<  若结果数据输出到内存，需要配置要输出到内存的地址
	GxTfmBuf       TD;                ///<  irdeto定义的私有数据
                                      ///<
	unsigned char  soft_key[16];      ///<  klm模块一般用不到soft key,不需要关心
	unsigned char  nonce[16];         ///<  DCAS 挑战应答输入数据
	unsigned int   stage;             ///<	KLM 密钥派生阶数
                                      ///<
	int            ret;               ///<  实际功能是否执行成功
	unsigned int   flags;             ///<  特定控制位集合配置
} GxTfmKlm;

/**
 * 加解密操作中使用的参数
 */
typedef struct {
	GxTfmModule    module;            ///<  选择进行加解密的加解密模块
	unsigned int   module_sub;        ///<  若加解密模块有多个设备节点，需要选择具体设备节点,如:M2M有8个通道，会产生8个设备节点
	GxTfmAlg       alg;               ///<  选择加解密的算法
	GxTfmOpt       opt;               ///<  选择加解密的模式
	GxTfmSrcBuf    src;               ///<  选择加解密的数据来源
	GxTfmDstBuf    dst;               ///<  选择加解密的结果输出目标
	GxTfmKeyBuf    even_key;          ///<  选择加解密的密钥来源,m2m模块分为even key和odd key,其他模块只能选择一个key来源
	GxTfmKeyBuf    odd_key;           ///<  选择加解密的odd key的密钥来源，只有m2m模块需要配置
	GxTfmBuf       input;             ///<	若源数据来源于内存，需要配置input buffer的内存数
	GxTfmBuf       output;            ///<  若结果数据输出到内存，需要配置要输出到内存的地址
	unsigned int   input_paddr;       ///<  驱动使用，用户不需要关心
	unsigned int   output_paddr;      ///<  驱动使用，用户不需要关心
	unsigned int   key_switch_gate;   ///<	m2m模块配置even key和odd key切换设置，每计算几个block切换一次
	unsigned char  soft_key[32];      ///<  若密钥来源于soft key时，需要将密钥填充的此数组
	unsigned int   soft_key_len;      ///<  配置soft key的长度，根据算法决定。AES128:16B; AES192:24B; AES256:32B; DES:8B; TDES:16B
	unsigned char  iv[16];            ///<	根据算法模式配置IV/COUNTER的值
	unsigned int   iv_id;             ///<	配置iv的类别。0:IV1; 1:IV2; 2:COUNTER
	GxTfmBuf       sm2_pub_key;       ///<	配置SM2算法验签使用的公钥

	int            ret;               ///<  实际功能是否执行成功
	unsigned int   flags;             ///<  特定控制位集合配置
} GxTfmCrypto;

/**
 * hash计算操作中使用的参数
 */
typedef struct {
	GxTfmModule  module;              ///<  选择进行hash计算的模块
	GxTfmAlg     alg;                 ///<	选择hash算法
	GxTfmBuf     input;               ///<	选择hash计算的数据来源
	GxTfmBuf     output;              ///<  选择hash计算的结果输出目标
	GxTfmBuf     pub_key;             ///<	处理sm2算法ZA时需要公钥
	GxTfmBuf     hmac_key;            ///<	sm3 hmac计算时需要hmac_key

	int          ret;                 ///<  实际功能是否执行成功
	unsigned int flags;               ///<  特定控制位集合配置
} GxTfmDgst;

/**
 * 非对称算法验签使用的参数
 */
typedef struct {
	GxTfmModule  module;              ///<  选择进行验证签名的模块
	GxTfmAlg     alg;                 ///<	选择进行验签的非对称算法
	GxTfmBuf     pub_key;             ///<  进行验签的公钥
	GxTfmBuf     hash;                ///<	进行验签的数据hash值
	GxTfmBuf     signature;           ///<	进行验签的签名

	int          ret;                 ///<  实际功能是否执行成功
	unsigned int flags;               ///<  特定控制位集合配置
} GxTfmVerify;

/**
 * 查询模块特性要求
 */
typedef struct {
	GxTfmModule module;
	unsigned int max_sub_num; //该模块支持的最大子模块数
	unsigned int alg; //该模块支持的算法
	unsigned int opt; //该模块支持的算法模式
	unsigned int src; //该模块支持的输入数据来源
	unsigned int dst; //该模块支持的输出数据目标
	unsigned int flags;//该模块支持的flag配置
	unsigned int key; //该模块支持的key来源
	unsigned char key_sub_num[TFM_KEY_MAX]; //该模块支持每种密钥的最大个数
	int ret;
} GxTfmCap;

/**
 * 配置SET PVRKey参数
 */
typedef struct {
	GxTfmModule   module;             ///<	选择设置PVRKey的模块
	unsigned int  pid;                ///<	指定需要解扰的PID，MTC TS解扰功能使用
	unsigned int  key_id;             ///<	选择将PVRKey存放到哪个寄存器
	unsigned char key[16];            ///<	若PVRKey为acpu配置，则填充该数组
	unsigned char iv[16];             ///<	若解扰需要IV则填充该数组，MTC TS解扰功能使用

	int           ret;                ///<	实际功能是否执行成功
	unsigned int  flags;              ///<  特定控制位集合配置
} GxTfmPVRKey;

#define TFM_KLM_SELECT_ROOTKEY  GXSE_IOW ('t', 0, GxTfmKlm)
#define TFM_KLM_SET_KN          GXSE_IOW ('t', 1, GxTfmKlm)
#define TFM_KLM_SET_CW          GXSE_IOW ('t', 2, GxTfmKlm)
#define TFM_KLM_UPDATE_TD_PARAM GXSE_IOW ('t', 3, GxTfmKlm)
#define TFM_KLM_GET_RESP        GXSE_IOWR('t', 4, GxTfmKlm)
#define TFM_DECRYPT             GXSE_IOWR('t', 5, GxTfmCrypto)
#define TFM_ENCRYPT             GXSE_IOWR('t', 6, GxTfmCrypto)
#define TFM_DGST                GXSE_IOWR('t', 7, GxTfmDgst)
#define TFM_VERIFY              GXSE_IOWR('t', 8, GxTfmVerify)
#define TFM_CAPABILITY          GXSE_IOR ('t', 9, GxTfmCap)

#define TFM_SET_PVRK            GXSE_IOW ('t',10, GxTfmPVRKey)
#define TFM_CLEAR_KEY           GXSE_IO  ('t',11)
#define TFM_ENABLE_KEY          GXSE_IO  ('t',12)
#define TFM_DISABLE_KEY         GXSE_IO  ('t',13)

#endif
/** @}*/
