#ifndef __AUTOTEST_H__
#define __AUTOTEST_H__

#define ARM_KEY_ADDR_CRYPTO_PVRKEY   0x10
#define ARM_KEY_ADDR_CRYPTO_CW       0x40
#define ARM_KEY_ADDR_CRYPTO_GPCW     0x70
#define ARM_KEY_ADDR_KLM_PVRKEY      0x20
#define ARM_KEY_ADDR_KLM_CW          0x50
#define ARM_KEY_ADDR_KLM_GPCW        0x80

typedef enum {
	TEST_MODULE_HASH,
	TEST_MODULE_OTP,
	TEST_MODULE_SKLM,
	TEST_MODULE_DKLM,
	TEST_MODULE_CRYPTO_BOOT,
	TEST_MODULE_CRYPTO_DY,
	TEST_MODULE_MISC,
	TEST_MODULE_MAX,
} AutoTestModule;

typedef enum {
	TEST_CHECK_SUCCESS,
	TEST_CHECK_RESULT,
	TEST_CHECK_SCPUKEY,
	TEST_CHECK_CW,
	TEST_CHECK_GPCW,
	TEST_CHECK_KCV,
	TEST_CHECK_RESP,
	TEST_CHECK_RK,
} AutoTestCheckFlag;

typedef enum {
	CASE_RET_SUCCESS,
	CASE_RET_FAIL,
	CASE_RET_VAL,
} AutoTestCaseRetFlag;

typedef struct {
	AutoTestModule module;
	unsigned int   case_num;
	AutoTestCaseRetFlag flag;
	unsigned int addr;
	unsigned int rst_len;
	unsigned int *rst;
} AutoTestRst;

typedef struct {
	AutoTestModule module;
	unsigned int   case_num;
} AutoTestCmd;

extern int do_klm_test     (AutoTestCmd *arg, AutoTestRst *rst);
extern int do_crypto_test  (AutoTestCmd *arg, AutoTestRst *rst);
extern int do_hash_test    (AutoTestCmd *arg, AutoTestRst *rst);
extern int do_otp_test     (AutoTestCmd *arg, AutoTestRst *rst);
extern int do_misc_test    (AutoTestCmd *arg, AutoTestRst *rst);
extern int do_ifcp_test    (void *pkt);

/************************************ TEST CASE ****************************************/

#if defined (CHIP_GX_TAURUS)
typedef enum {
	OTP_CASE_SECT0,
	OTP_CASE_SECT1,
	OTP_CASE_SECT2,
	OTP_CASE_SECT3,
	OTP_CASE_SECT4,
	OTP_CASE_SECT5,
	OTP_MAX_CASE,
} AutoTestOTPCase;

typedef enum {
	CRYPTO_BOOT_ECB_OTP,
	CRYPTO_BOOT_ECB_SOFT,
	CRYPTO_BOOT_CBC_OTP,
	CRYPTO_BOOT_CBC_SOFT,
	CRYPTO_BOOT_ECB_SM4_0,
	CRYPTO_BOOT_CBC_SM4,
	CRYPTO_BOOT_MAX_CASE,
} AutoTestCryptoBootCase;

typedef enum {
	/*buf test*/
	CRYPTO_DY_DI_DSK1_REG0_TS,
	CRYPTO_DY_DI_DSK1_REG1_TS,
	CRYPTO_DY_DI_DSK1_REG2_TS,
	CRYPTO_DY_DI_DSK1_REG3_TS,
	CRYPTO_DY_DI_DSK1_REG4_TS,
	CRYPTO_DY_DI_DSK1_REG5_TS,
	CRYPTO_DY_DI_DSK1_REG6_TS,
	CRYPTO_DY_DI_DSK1_REG7_TS,

	CRYPTO_DY_DI_DSK2_REG0_TS,
	CRYPTO_DY_DI_DSK2_REG1_TS,
	CRYPTO_DY_DI_DSK2_REG2_TS,
	CRYPTO_DY_DI_DSK2_REG3_TS,
	CRYPTO_DY_DI_DSK2_REG4_TS,
	CRYPTO_DY_DI_DSK2_REG5_TS,
	CRYPTO_DY_DI_DSK2_REG6_TS,
	CRYPTO_DY_DI_DSK2_REG7_TS,

	CRYPTO_DY_DI_DSK3_REG0_TS,
	CRYPTO_DY_DI_DSK3_REG1_TS,
	CRYPTO_DY_DI_DSK3_REG2_TS,
	CRYPTO_DY_DI_DSK3_REG3_TS,
	CRYPTO_DY_DI_DSK3_REG4_TS,
	CRYPTO_DY_DI_DSK3_REG5_TS,
	CRYPTO_DY_DI_DSK3_REG6_TS,
	CRYPTO_DY_DI_DSK3_REG7_TS,

	CRYPTO_DY_DI_DSK4_REG0_TS,
	CRYPTO_DY_DI_DSK4_REG1_TS,
	CRYPTO_DY_DI_DSK4_REG2_TS,
	CRYPTO_DY_DI_DSK4_REG3_TS,
	CRYPTO_DY_DI_DSK4_REG4_TS,
	CRYPTO_DY_DI_DSK4_REG5_TS,
	CRYPTO_DY_DI_DSK4_REG6_TS,
	CRYPTO_DY_DI_DSK4_REG7_TS,

	CRYPTO_DY_DI_RK1_REG0_TS,
	CRYPTO_DY_DI_RK1_REG1_TS,
	CRYPTO_DY_DI_RK1_REG2_TS,
	CRYPTO_DY_DI_RK1_REG3_TS,
	CRYPTO_DY_DI_RK1_REG4_TS,
	CRYPTO_DY_DI_RK1_REG5_TS,
	CRYPTO_DY_DI_RK1_REG6_TS,
	CRYPTO_DY_DI_RK1_REG7_TS,

	CRYPTO_DY_DI_RK2_REG0_TS,
	CRYPTO_DY_DI_RK2_REG1_TS,
	CRYPTO_DY_DI_RK2_REG2_TS,
	CRYPTO_DY_DI_RK2_REG3_TS,
	CRYPTO_DY_DI_RK2_REG4_TS,
	CRYPTO_DY_DI_RK2_REG5_TS,
	CRYPTO_DY_DI_RK2_REG6_TS,
	CRYPTO_DY_DI_RK2_REG7_TS,

	CRYPTO_DY_DI_ROOTKEY_REG0_TS,
	CRYPTO_DY_DI_ROOTKEY_REG1_TS,
	CRYPTO_DY_DI_ROOTKEY_REG2_TS,
	CRYPTO_DY_DI_ROOTKEY_REG3_TS,
	CRYPTO_DY_DI_ROOTKEY_REG4_TS,
	CRYPTO_DY_DI_ROOTKEY_REG5_TS,
	CRYPTO_DY_DI_ROOTKEY_REG6_TS,
	CRYPTO_DY_DI_ROOTKEY_REG7_TS,

	CRYPTO_DY_DI_SMK_REG0_TS,
	CRYPTO_DY_DI_SMK_REG1_TS,
	CRYPTO_DY_DI_SMK_REG2_TS,
	CRYPTO_DY_DI_SMK_REG3_TS,
	CRYPTO_DY_DI_SMK_REG4_TS,
	CRYPTO_DY_DI_SMK_REG5_TS,
	CRYPTO_DY_DI_SMK_REG6_TS,
	CRYPTO_DY_DI_SMK_REG7_TS,

	CRYPTO_DY_DI_TK1_REG0_TS,
	CRYPTO_DY_DI_TK1_REG1_TS,
	CRYPTO_DY_DI_TK1_REG2_TS,
	CRYPTO_DY_DI_TK1_REG3_TS,
	CRYPTO_DY_DI_TK1_REG4_TS,
	CRYPTO_DY_DI_TK1_REG5_TS,
	CRYPTO_DY_DI_TK1_REG6_TS,
	CRYPTO_DY_DI_TK1_REG7_TS,

	CRYPTO_DY_DI_TK2_REG0_TS,
	CRYPTO_DY_DI_TK2_REG1_TS,
	CRYPTO_DY_DI_TK2_REG2_TS,
	CRYPTO_DY_DI_TK2_REG3_TS,
	CRYPTO_DY_DI_TK2_REG4_TS,
	CRYPTO_DY_DI_TK2_REG5_TS,
	CRYPTO_DY_DI_TK2_REG6_TS,
	CRYPTO_DY_DI_TK2_REG7_TS, //80

	CRYPTO_DY_DI_DSK1_REG0_M2M,
	CRYPTO_DY_DI_DSK1_REG1_M2M,
	CRYPTO_DY_DI_DSK1_REG2_M2M,
	CRYPTO_DY_DI_DSK1_REG3_M2M,
	CRYPTO_DY_DI_DSK1_REG4_M2M,
	CRYPTO_DY_DI_DSK1_REG5_M2M,
	CRYPTO_DY_DI_DSK1_REG6_M2M,
	CRYPTO_DY_DI_DSK1_REG7_M2M,

	CRYPTO_DY_DI_DSK2_REG0_M2M,
	CRYPTO_DY_DI_DSK2_REG1_M2M,
	CRYPTO_DY_DI_DSK2_REG2_M2M,
	CRYPTO_DY_DI_DSK2_REG3_M2M,
	CRYPTO_DY_DI_DSK2_REG4_M2M,
	CRYPTO_DY_DI_DSK2_REG5_M2M,
	CRYPTO_DY_DI_DSK2_REG6_M2M,
	CRYPTO_DY_DI_DSK2_REG7_M2M,

	CRYPTO_DY_DI_DSK3_REG0_M2M,
	CRYPTO_DY_DI_DSK3_REG1_M2M,
	CRYPTO_DY_DI_DSK3_REG2_M2M,
	CRYPTO_DY_DI_DSK3_REG3_M2M,
	CRYPTO_DY_DI_DSK3_REG4_M2M,
	CRYPTO_DY_DI_DSK3_REG5_M2M,
	CRYPTO_DY_DI_DSK3_REG6_M2M,
	CRYPTO_DY_DI_DSK3_REG7_M2M,

	CRYPTO_DY_DI_DSK4_REG0_M2M,
	CRYPTO_DY_DI_DSK4_REG1_M2M,
	CRYPTO_DY_DI_DSK4_REG2_M2M,
	CRYPTO_DY_DI_DSK4_REG3_M2M,
	CRYPTO_DY_DI_DSK4_REG4_M2M,
	CRYPTO_DY_DI_DSK4_REG5_M2M,
	CRYPTO_DY_DI_DSK4_REG6_M2M,
	CRYPTO_DY_DI_DSK4_REG7_M2M,

	CRYPTO_DY_DI_RK1_REG0_M2M,
	CRYPTO_DY_DI_RK1_REG1_M2M,
	CRYPTO_DY_DI_RK1_REG2_M2M,
	CRYPTO_DY_DI_RK1_REG3_M2M,
	CRYPTO_DY_DI_RK1_REG4_M2M,
	CRYPTO_DY_DI_RK1_REG5_M2M,
	CRYPTO_DY_DI_RK1_REG6_M2M,
	CRYPTO_DY_DI_RK1_REG7_M2M,

	CRYPTO_DY_DI_RK2_REG0_M2M,
	CRYPTO_DY_DI_RK2_REG1_M2M,
	CRYPTO_DY_DI_RK2_REG2_M2M,
	CRYPTO_DY_DI_RK2_REG3_M2M,
	CRYPTO_DY_DI_RK2_REG4_M2M,
	CRYPTO_DY_DI_RK2_REG5_M2M,
	CRYPTO_DY_DI_RK2_REG6_M2M,
	CRYPTO_DY_DI_RK2_REG7_M2M,

	CRYPTO_DY_DI_ROOTKEY_REG0_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG1_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG2_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG3_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG4_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG5_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG6_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG7_M2M,

	CRYPTO_DY_DI_SMK_REG0_M2M,
	CRYPTO_DY_DI_SMK_REG1_M2M,
	CRYPTO_DY_DI_SMK_REG2_M2M,
	CRYPTO_DY_DI_SMK_REG3_M2M,
	CRYPTO_DY_DI_SMK_REG4_M2M,
	CRYPTO_DY_DI_SMK_REG5_M2M,
	CRYPTO_DY_DI_SMK_REG6_M2M,
	CRYPTO_DY_DI_SMK_REG7_M2M,

	CRYPTO_DY_DI_TK1_REG0_M2M,
	CRYPTO_DY_DI_TK1_REG1_M2M,
	CRYPTO_DY_DI_TK1_REG2_M2M,
	CRYPTO_DY_DI_TK1_REG3_M2M,
	CRYPTO_DY_DI_TK1_REG4_M2M,
	CRYPTO_DY_DI_TK1_REG5_M2M,
	CRYPTO_DY_DI_TK1_REG6_M2M,
	CRYPTO_DY_DI_TK1_REG7_M2M,

	CRYPTO_DY_DI_TK2_REG0_M2M,
	CRYPTO_DY_DI_TK2_REG1_M2M,
	CRYPTO_DY_DI_TK2_REG2_M2M,
	CRYPTO_DY_DI_TK2_REG3_M2M,
	CRYPTO_DY_DI_TK2_REG4_M2M,
	CRYPTO_DY_DI_TK2_REG5_M2M,
	CRYPTO_DY_DI_TK2_REG6_M2M,
	CRYPTO_DY_DI_TK2_REG7_M2M, //160

	/*test do*/
	CRYPTO_DY_DO,
	CRYPTO_DY_DO_KCV,
	CRYPTO_DY_DO_SCR,
	/*test src from DSK*/
	CRYPTO_DY_SRC_FROM_DSK,

	/*test half_cw*/
	CRYPTO_DY_HALFCW_LOW,
	CRYPTO_DY_HALFCW_HIGH,

	/*test xor*/
	CRYPTO_DY_XOR,

	/*test tdes*/
	CRYPTO_DY_TDES_DI_2WORDS,
	CRYPTO_DY_TDES_DI_4WORDS,
	CRYPTO_DY_SM4,

	/*key clr test*/
	CRYPTO_DY_KEYCLR_DSK1,
	CRYPTO_DY_KEYCLR_DSK2,
	CRYPTO_DY_KEYCLR_DSK3,
	CRYPTO_DY_KEYCLR_DSK4,
	CRYPTO_DY_KEYCLR_RK1,
	CRYPTO_DY_KEYCLR_RK2,
	CRYPTO_DY_KEYCLR_ROOTKEY,
	CRYPTO_DY_KEYCLR_REG0,
	CRYPTO_DY_KEYCLR_REG1,
	CRYPTO_DY_KEYCLR_REG2,
	CRYPTO_DY_KEYCLR_REG3,
	CRYPTO_DY_KEYCLR_REG4,
	CRYPTO_DY_KEYCLR_REG5,
	CRYPTO_DY_KEYCLR_REG6,
	CRYPTO_DY_KEYCLR_REG7,

	CRYPTO_DY_DCAS_K3,
	CRYPTO_DY_MAX_CASE,
} AutoTestCryptoDynamicCase;

#elif defined (CHIP_GX_SIRIUS)
typedef enum {
	OTP_CASE_SECT0_0,
	OTP_CASE_SECT0_1,
	OTP_CASE_SECT1,
	OTP_CASE_SECT2,
	OTP_CASE_SECT3,
	OTP_CASE_SECT4,
	OTP_CASE_SECT5,
	OTP_CASE_SECT6,
	OTP_CASE_SECT7_0,
	OTP_CASE_SECT7_1,
	OTP_CASE_SECT7_2,
	OTP_CASE_SECT8,
	OTP_CASE_SECT9,
	OTP_MAX_CASE,
} AutoTestOTPCase;

typedef enum {
	CRYPTO_BOOT_ECB_CSGK2,
	CRYPTO_BOOT_ECB_SIK,
	CRYPTO_BOOT_ECB_OTP,
	CRYPTO_BOOT_ECB_SOFT,
	CRYPTO_BOOT_CBC_CSGK2,
	CRYPTO_BOOT_CBC_SIK,
	CRYPTO_BOOT_CBC_OTP,
	CRYPTO_BOOT_CBC_SOFT,
	CRYPTO_BOOT_ECB_SM4_0,
	CRYPTO_BOOT_CBC_SM4,
	CRYPTO_BOOT_KEYCLR_CSGK2,
	CRYPTO_BOOT_KEYCLR_SIK,
	CRYPTO_BOOT_MAX_CASE,
} AutoTestCryptoBootCase;

typedef enum {
	/*buf test*/
	CRYPTO_DY_DI_DSK1_REG0_TS,
	CRYPTO_DY_DI_DSK1_REG1_TS,
	CRYPTO_DY_DI_DSK1_REG2_TS,
	CRYPTO_DY_DI_DSK1_REG3_TS,
	CRYPTO_DY_DI_DSK1_REG4_TS,
	CRYPTO_DY_DI_DSK1_REG5_TS,
	CRYPTO_DY_DI_DSK1_REG6_TS,
	CRYPTO_DY_DI_DSK1_REG7_TS,

	CRYPTO_DY_DI_DSK2_REG0_TS,
	CRYPTO_DY_DI_DSK2_REG1_TS,
	CRYPTO_DY_DI_DSK2_REG2_TS,
	CRYPTO_DY_DI_DSK2_REG3_TS,
	CRYPTO_DY_DI_DSK2_REG4_TS,
	CRYPTO_DY_DI_DSK2_REG5_TS,
	CRYPTO_DY_DI_DSK2_REG6_TS,
	CRYPTO_DY_DI_DSK2_REG7_TS,

	CRYPTO_DY_DI_RK1_REG0_TS,
	CRYPTO_DY_DI_RK1_REG1_TS,
	CRYPTO_DY_DI_RK1_REG2_TS,
	CRYPTO_DY_DI_RK1_REG3_TS,
	CRYPTO_DY_DI_RK1_REG4_TS,
	CRYPTO_DY_DI_RK1_REG5_TS,
	CRYPTO_DY_DI_RK1_REG6_TS,
	CRYPTO_DY_DI_RK1_REG7_TS,

	CRYPTO_DY_DI_RK2_REG0_TS,
	CRYPTO_DY_DI_RK2_REG1_TS,
	CRYPTO_DY_DI_RK2_REG2_TS,
	CRYPTO_DY_DI_RK2_REG3_TS,
	CRYPTO_DY_DI_RK2_REG4_TS,
	CRYPTO_DY_DI_RK2_REG5_TS,
	CRYPTO_DY_DI_RK2_REG6_TS,
	CRYPTO_DY_DI_RK2_REG7_TS,

	CRYPTO_DY_DI_ROOTKEY_REG0_TS,
	CRYPTO_DY_DI_ROOTKEY_REG1_TS,
	CRYPTO_DY_DI_ROOTKEY_REG2_TS,
	CRYPTO_DY_DI_ROOTKEY_REG3_TS,
	CRYPTO_DY_DI_ROOTKEY_REG4_TS,
	CRYPTO_DY_DI_ROOTKEY_REG5_TS,
	CRYPTO_DY_DI_ROOTKEY_REG6_TS,
	CRYPTO_DY_DI_ROOTKEY_REG7_TS,

	CRYPTO_DY_DI_SMK_REG0_TS,
	CRYPTO_DY_DI_SMK_REG1_TS,
	CRYPTO_DY_DI_SMK_REG2_TS,
	CRYPTO_DY_DI_SMK_REG3_TS,
	CRYPTO_DY_DI_SMK_REG4_TS,
	CRYPTO_DY_DI_SMK_REG5_TS,
	CRYPTO_DY_DI_SMK_REG6_TS,
	CRYPTO_DY_DI_SMK_REG7_TS, //48

	CRYPTO_DY_DI_DSK1_REG0_GP,
	CRYPTO_DY_DI_DSK1_REG1_GP,
	CRYPTO_DY_DI_DSK1_REG2_GP,
	CRYPTO_DY_DI_DSK1_REG3_GP,
	CRYPTO_DY_DI_DSK1_REG4_GP,
	CRYPTO_DY_DI_DSK1_REG5_GP,
	CRYPTO_DY_DI_DSK1_REG6_GP,
	CRYPTO_DY_DI_DSK1_REG7_GP,

	CRYPTO_DY_DI_DSK2_REG0_GP,
	CRYPTO_DY_DI_DSK2_REG1_GP,
	CRYPTO_DY_DI_DSK2_REG2_GP,
	CRYPTO_DY_DI_DSK2_REG3_GP,
	CRYPTO_DY_DI_DSK2_REG4_GP,
	CRYPTO_DY_DI_DSK2_REG5_GP,
	CRYPTO_DY_DI_DSK2_REG6_GP,
	CRYPTO_DY_DI_DSK2_REG7_GP,

	CRYPTO_DY_DI_RK1_REG0_GP,
	CRYPTO_DY_DI_RK1_REG1_GP,
	CRYPTO_DY_DI_RK1_REG2_GP,
	CRYPTO_DY_DI_RK1_REG3_GP,
	CRYPTO_DY_DI_RK1_REG4_GP,
	CRYPTO_DY_DI_RK1_REG5_GP,
	CRYPTO_DY_DI_RK1_REG6_GP,
	CRYPTO_DY_DI_RK1_REG7_GP,

	CRYPTO_DY_DI_RK2_REG0_GP,
	CRYPTO_DY_DI_RK2_REG1_GP,
	CRYPTO_DY_DI_RK2_REG2_GP,
	CRYPTO_DY_DI_RK2_REG3_GP,
	CRYPTO_DY_DI_RK2_REG4_GP,
	CRYPTO_DY_DI_RK2_REG5_GP,
	CRYPTO_DY_DI_RK2_REG6_GP,
	CRYPTO_DY_DI_RK2_REG7_GP,

	CRYPTO_DY_DI_ROOTKEY_REG0_GP,
	CRYPTO_DY_DI_ROOTKEY_REG1_GP,
	CRYPTO_DY_DI_ROOTKEY_REG2_GP,
	CRYPTO_DY_DI_ROOTKEY_REG3_GP,
	CRYPTO_DY_DI_ROOTKEY_REG4_GP,
	CRYPTO_DY_DI_ROOTKEY_REG5_GP,
	CRYPTO_DY_DI_ROOTKEY_REG6_GP,
	CRYPTO_DY_DI_ROOTKEY_REG7_GP,

	CRYPTO_DY_DI_SMK_REG0_GP,
	CRYPTO_DY_DI_SMK_REG1_GP,
	CRYPTO_DY_DI_SMK_REG2_GP,
	CRYPTO_DY_DI_SMK_REG3_GP,
	CRYPTO_DY_DI_SMK_REG4_GP,
	CRYPTO_DY_DI_SMK_REG5_GP,
	CRYPTO_DY_DI_SMK_REG6_GP,
	CRYPTO_DY_DI_SMK_REG7_GP, //96

	CRYPTO_DY_DI_DSK1_REG0_M2M,
	CRYPTO_DY_DI_DSK1_REG1_M2M,
	CRYPTO_DY_DI_DSK1_REG2_M2M,
	CRYPTO_DY_DI_DSK1_REG3_M2M,
	CRYPTO_DY_DI_DSK1_REG4_M2M,
	CRYPTO_DY_DI_DSK1_REG5_M2M,
	CRYPTO_DY_DI_DSK1_REG6_M2M,
	CRYPTO_DY_DI_DSK1_REG7_M2M,

	CRYPTO_DY_DI_DSK2_REG0_M2M,
	CRYPTO_DY_DI_DSK2_REG1_M2M,
	CRYPTO_DY_DI_DSK2_REG2_M2M,
	CRYPTO_DY_DI_DSK2_REG3_M2M,
	CRYPTO_DY_DI_DSK2_REG4_M2M,
	CRYPTO_DY_DI_DSK2_REG5_M2M,
	CRYPTO_DY_DI_DSK2_REG6_M2M,
	CRYPTO_DY_DI_DSK2_REG7_M2M,

	CRYPTO_DY_DI_RK1_REG0_M2M,
	CRYPTO_DY_DI_RK1_REG1_M2M,
	CRYPTO_DY_DI_RK1_REG2_M2M,
	CRYPTO_DY_DI_RK1_REG3_M2M,
	CRYPTO_DY_DI_RK1_REG4_M2M,
	CRYPTO_DY_DI_RK1_REG5_M2M,
	CRYPTO_DY_DI_RK1_REG6_M2M,
	CRYPTO_DY_DI_RK1_REG7_M2M,

	CRYPTO_DY_DI_RK2_REG0_M2M,
	CRYPTO_DY_DI_RK2_REG1_M2M,
	CRYPTO_DY_DI_RK2_REG2_M2M,
	CRYPTO_DY_DI_RK2_REG3_M2M,
	CRYPTO_DY_DI_RK2_REG4_M2M,
	CRYPTO_DY_DI_RK2_REG5_M2M,
	CRYPTO_DY_DI_RK2_REG6_M2M,
	CRYPTO_DY_DI_RK2_REG7_M2M,

	CRYPTO_DY_DI_ROOTKEY_REG0_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG1_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG2_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG3_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG4_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG5_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG6_M2M,
	CRYPTO_DY_DI_ROOTKEY_REG7_M2M,

	CRYPTO_DY_DI_SMK_REG0_M2M,
	CRYPTO_DY_DI_SMK_REG1_M2M,
	CRYPTO_DY_DI_SMK_REG2_M2M,
	CRYPTO_DY_DI_SMK_REG3_M2M,
	CRYPTO_DY_DI_SMK_REG4_M2M,
	CRYPTO_DY_DI_SMK_REG5_M2M,
	CRYPTO_DY_DI_SMK_REG6_M2M,
	CRYPTO_DY_DI_SMK_REG7_M2M, //144

	/*test do*/
	CRYPTO_DY_DO,
	CRYPTO_DY_DO_KCV,
	CRYPTO_DY_DO_SCR,
	/*test src from DSK*/
	CRYPTO_DY_SRC_FROM_DSK,

	/*test half_cw*/
	CRYPTO_DY_HALFCW_LOW,
	CRYPTO_DY_HALFCW_HIGH,

	/*test xor*/
	CRYPTO_DY_XOR,

	/*test tdes*/
	CRYPTO_DY_TDES_DI_2WORDS,
	CRYPTO_DY_TDES_DI_4WORDS,
	CRYPTO_DY_SM4,

	/*key clr test*/
	CRYPTO_DY_KEYCLR_DSK1,
	CRYPTO_DY_KEYCLR_DSK2,
	CRYPTO_DY_KEYCLR_RK1,
	CRYPTO_DY_KEYCLR_RK2,
	CRYPTO_DY_KEYCLR_ROOTKEY,
	CRYPTO_DY_KEYCLR_REG0,
	CRYPTO_DY_KEYCLR_REG1,
	CRYPTO_DY_KEYCLR_REG2,
	CRYPTO_DY_KEYCLR_REG3,
	CRYPTO_DY_KEYCLR_REG4,
	CRYPTO_DY_KEYCLR_REG5,
	CRYPTO_DY_KEYCLR_REG6,
	CRYPTO_DY_KEYCLR_REG7,

	CRYPTO_DY_DCAS_K3,
	/*Manufacturer app test*/
	CRYPTO_DY_ABV_AES,
	CRYPTO_DY_ABV_TDES,

	CRYPTO_DY_BYDESIGN_AES,

	CRYPTO_DY_DCAS_AES,
	CRYPTO_DY_DCAS_AES_HANDSHAKE,
	CRYPTO_DY_DCAS_TDES,
	CRYPTO_DY_DCAS_TDES_HANDSHAKE,

	CRYPTO_DY_VERIMATRIX_R2R,

	CRYPTO_DY_MAX_CASE,
} AutoTestCryptoDynamicCase;

#endif

typedef enum {
	HASH_CASE_SHA1_LEN16,
	HASH_CASE_SHA1_LEN512,
	HASH_CASE_SHA1_LEN17,
	HASH_CASE_SHA1_LEN18,
	HASH_CASE_SHA1_LEN511,
	HASH_CASE_SHA1_LEN513,

	HASH_CASE_SHA256_LEN16,
	HASH_CASE_SHA256_LEN512,
	HASH_CASE_SHA256_LEN17,
	HASH_CASE_SHA256_LEN18,
	HASH_CASE_SHA256_LEN511,
	HASH_CASE_SHA256_LEN513,
	HASH_MAX_CASE,
} AutoTestHashCase;

typedef enum {
	SKLM_CASE_NULL,
	SKLM_CASE_KLC1,
	SKLM_CASE_KLC2,
	SKLM_CASE_KLC6,
	SKLM_CASE_KLC6_KCV,
	SKLM_CASE_KLC7,
	SKLM_CASE_KLC7_KCV,
	SKLM_CASE_KLC8_IPA,
	SKLM_CASE_KLC8_TDC_OUT,
	SKLM_CASE_KLC3_TDES,
	SKLM_CASE_KLC3_TDES_KCV,
	SKLM_CASE_KLC3_AES,
	SKLM_CASE_KLC3_AES_KCV,
	SKLM_CASE_KLC3_IPA,
	SKLM_CASE_KLC3_IPA_KCV,
	SKLM_CASE_KLC3_CC_TDES,
	SKLM_CASE_KLC3_CC_TDES_KCV,
	SKLM_CASE_KLC3_CC_AES,
	SKLM_CASE_KLC3_CC_AES_KCV,
	SKLM_CASE_KLC3_CC_IPA,
	SKLM_CASE_KLC3_CC_IPA_KCV,
	SKLM_CASE_KLC3_CD_TDES,
	SKLM_CASE_KLC3_CD_AES,
	SKLM_CASE_KLC3_CD_IPA,
	SKLM_CASE_KLC3_CC_CD_TDES,
	SKLM_CASE_KLC3_CC_CD_AES,
	SKLM_CASE_KLC3_CC_CD_IPA,
	SKLM_CASE_KLC4_TDES,
	SKLM_CASE_KLC4_TDES_KCV,
	SKLM_CASE_KLC4_AES,
	SKLM_CASE_KLC4_AES_KCV,
	SKLM_CASE_KLC4_IPA,
	SKLM_CASE_KLC4_IPA_KCV,
	SKLM_CASE_KLC4_CC_TDES,
	SKLM_CASE_KLC4_CC_TDES_KCV,
	SKLM_CASE_KLC4_CC_AES,
	SKLM_CASE_KLC4_CC_AES_KCV,
	SKLM_CASE_KLC4_CC_IPA,
	SKLM_CASE_KLC4_CC_IPA_KCV,
	SKLM_CASE_KLC4_CD_TDES,
	SKLM_CASE_KLC4_CD_AES,
	SKLM_CASE_KLC4_CD_IPA,
	SKLM_CASE_KLC4_CC_CD_TDES,
	SKLM_CASE_KLC4_CC_CD_AES,
	SKLM_CASE_KLC4_CC_CD_IPA,
	SKLM_CASE_KLC3_CHALLENGE,
	SKLM_CASE_KLC4_CHALLENGE,
	SKLM_CASE_KLC5_TDES,
	SKLM_CASE_KLC5_TDES_KCV,
	SKLM_CASE_KLC5_AES,
	SKLM_CASE_KLC5_AES_KCV,
	SKLM_CASE_KLC3_HALFCW_TDES,
	SKLM_CASE_KLC3_HALFCW_TDES_KCV,
	SKLM_CASE_KLC3_HALFCW_AES,
	SKLM_CASE_KLC3_HALFCW_AES_KCV,
	SKLM_CASE_KLC3_HALFCW_IPA,
	SKLM_CASE_KLC3_HALFCW_IPA_KCV,
	SKLM_CASE_KLC3_HALFCW_CC_TDES,
	SKLM_CASE_KLC3_HALFCW_CC_TDES_KCV,
	SKLM_CASE_KLC3_HALFCW_CC_AES,
	SKLM_CASE_KLC3_HALFCW_CC_AES_KCV,
	SKLM_CASE_KLC3_HALFCW_CC_IPA,
	SKLM_CASE_KLC3_HALFCW_CC_IPA_KCV,
	SKLM_CASE_KLC3_HALFCW_CD_TDES,
	SKLM_CASE_KLC3_HALFCW_CD_AES,
	SKLM_CASE_KLC3_HALFCW_CD_IPA,
	SKLM_CASE_KLC3_HALFCW_CC_CD_TDES,
	SKLM_CASE_KLC3_HALFCW_CC_CD_AES,
	SKLM_CASE_KLC3_HALFCW_CC_CD_IPA,

	SKLM_MAX_CASE,
} AutoTestSKLMCase;

typedef enum {
  /*buf test*/
	DKLM_CASE_VBUF0_HBUF0,
	DKLM_CASE_VBUF0_HBUF1,
	DKLM_CASE_VBUF0_HBUF2,
	DKLM_CASE_VBUF0_HBUF3,
	DKLM_CASE_VBUF0_HBUF4,
	DKLM_CASE_VBUF0_HBUF5,
	DKLM_CASE_VBUF0_HBUF6,
	DKLM_CASE_VBUF0_HBUF7,
	DKLM_CASE_VBUF0_HBUF8,
	DKLM_CASE_VBUF0_HBUF9,
	DKLM_CASE_VBUF0_HBUF10,
	DKLM_CASE_VBUF0_HBUF11,
	DKLM_CASE_VBUF0_HBUF12,
	DKLM_CASE_VBUF0_HBUF13,
	DKLM_CASE_VBUF0_HBUF14,
	DKLM_CASE_VBUF0_HBUF15,

	DKLM_CASE_VBUF1_HBUF0,
	DKLM_CASE_VBUF1_HBUF1,
	DKLM_CASE_VBUF1_HBUF2,
	DKLM_CASE_VBUF1_HBUF3,
	DKLM_CASE_VBUF1_HBUF4,
	DKLM_CASE_VBUF1_HBUF5,
	DKLM_CASE_VBUF1_HBUF6,
	DKLM_CASE_VBUF1_HBUF7,
	DKLM_CASE_VBUF1_HBUF8,
	DKLM_CASE_VBUF1_HBUF9,
	DKLM_CASE_VBUF1_HBUF10,
	DKLM_CASE_VBUF1_HBUF11,
	DKLM_CASE_VBUF1_HBUF12,
	DKLM_CASE_VBUF1_HBUF13,
	DKLM_CASE_VBUF1_HBUF14,
	DKLM_CASE_VBUF1_HBUF15,

	DKLM_CASE_VBUF2_HBUF0,
	DKLM_CASE_VBUF2_HBUF1,
	DKLM_CASE_VBUF2_HBUF2,
	DKLM_CASE_VBUF2_HBUF3,
	DKLM_CASE_VBUF2_HBUF4,
	DKLM_CASE_VBUF2_HBUF5,
	DKLM_CASE_VBUF2_HBUF6,
	DKLM_CASE_VBUF2_HBUF7,
	DKLM_CASE_VBUF2_HBUF8,
	DKLM_CASE_VBUF2_HBUF9,
	DKLM_CASE_VBUF2_HBUF10,
	DKLM_CASE_VBUF2_HBUF11,
	DKLM_CASE_VBUF2_HBUF12,
	DKLM_CASE_VBUF2_HBUF13,
	DKLM_CASE_VBUF2_HBUF14,
	DKLM_CASE_VBUF2_HBUF15,

	DKLM_CASE_VBUF3_HBUF0,
	DKLM_CASE_VBUF3_HBUF1,
	DKLM_CASE_VBUF3_HBUF2,
	DKLM_CASE_VBUF3_HBUF3,
	DKLM_CASE_VBUF3_HBUF4,
	DKLM_CASE_VBUF3_HBUF5,
	DKLM_CASE_VBUF3_HBUF6,
	DKLM_CASE_VBUF3_HBUF7,
	DKLM_CASE_VBUF3_HBUF8,
	DKLM_CASE_VBUF3_HBUF9,
	DKLM_CASE_VBUF3_HBUF10,
	DKLM_CASE_VBUF3_HBUF11,
	DKLM_CASE_VBUF3_HBUF12,
	DKLM_CASE_VBUF3_HBUF13,
	DKLM_CASE_VBUF3_HBUF14,
	DKLM_CASE_VBUF3_HBUF15,

	DKLM_CASE_VBUF4_HBUF0,
	DKLM_CASE_VBUF4_HBUF1,
	DKLM_CASE_VBUF4_HBUF2,
	DKLM_CASE_VBUF4_HBUF3,
	DKLM_CASE_VBUF4_HBUF4,
	DKLM_CASE_VBUF4_HBUF5,
	DKLM_CASE_VBUF4_HBUF6,
	DKLM_CASE_VBUF4_HBUF7,
	DKLM_CASE_VBUF4_HBUF8,
	DKLM_CASE_VBUF4_HBUF9,
	DKLM_CASE_VBUF4_HBUF10,
	DKLM_CASE_VBUF4_HBUF11,
	DKLM_CASE_VBUF4_HBUF12,
	DKLM_CASE_VBUF4_HBUF13,
	DKLM_CASE_VBUF4_HBUF14,
	DKLM_CASE_VBUF4_HBUF15,

	DKLM_CASE_VBUF5_HBUF0,
	DKLM_CASE_VBUF5_HBUF1,
	DKLM_CASE_VBUF5_HBUF2,
	DKLM_CASE_VBUF5_HBUF3,
	DKLM_CASE_VBUF5_HBUF4,
	DKLM_CASE_VBUF5_HBUF5,
	DKLM_CASE_VBUF5_HBUF6,
	DKLM_CASE_VBUF5_HBUF7,
	DKLM_CASE_VBUF5_HBUF8,
	DKLM_CASE_VBUF5_HBUF9,
	DKLM_CASE_VBUF5_HBUF10,
	DKLM_CASE_VBUF5_HBUF11,
	DKLM_CASE_VBUF5_HBUF12,
	DKLM_CASE_VBUF5_HBUF13,
	DKLM_CASE_VBUF5_HBUF14,
	DKLM_CASE_VBUF5_HBUF15,

	DKLM_CASE_VBUF6_HBUF0,
	DKLM_CASE_VBUF6_HBUF1,
	DKLM_CASE_VBUF6_HBUF2,
	DKLM_CASE_VBUF6_HBUF3,
	DKLM_CASE_VBUF6_HBUF4,
	DKLM_CASE_VBUF6_HBUF5,
	DKLM_CASE_VBUF6_HBUF6,
	DKLM_CASE_VBUF6_HBUF7,
	DKLM_CASE_VBUF6_HBUF8,
	DKLM_CASE_VBUF6_HBUF9,
	DKLM_CASE_VBUF6_HBUF10,
	DKLM_CASE_VBUF6_HBUF11,
	DKLM_CASE_VBUF6_HBUF12,
	DKLM_CASE_VBUF6_HBUF13,
	DKLM_CASE_VBUF6_HBUF14,
	DKLM_CASE_VBUF6_HBUF15,

	DKLM_CASE_VBUF7_HBUF0,
	DKLM_CASE_VBUF7_HBUF1,
	DKLM_CASE_VBUF7_HBUF2,
	DKLM_CASE_VBUF7_HBUF3,
	DKLM_CASE_VBUF7_HBUF4,
	DKLM_CASE_VBUF7_HBUF5,
	DKLM_CASE_VBUF7_HBUF6,
	DKLM_CASE_VBUF7_HBUF7,
	DKLM_CASE_VBUF7_HBUF8,
	DKLM_CASE_VBUF7_HBUF9,
	DKLM_CASE_VBUF7_HBUF10,
	DKLM_CASE_VBUF7_HBUF11,
	DKLM_CASE_VBUF7_HBUF12,
	DKLM_CASE_VBUF7_HBUF13,
	DKLM_CASE_VBUF7_HBUF14,
	DKLM_CASE_VBUF7_HBUF15,

	DKLM_CASE_VBUF8_HBUF0,
	DKLM_CASE_VBUF8_HBUF1,
	DKLM_CASE_VBUF8_HBUF2,
	DKLM_CASE_VBUF8_HBUF3,
	DKLM_CASE_VBUF8_HBUF4,
	DKLM_CASE_VBUF8_HBUF5,
	DKLM_CASE_VBUF8_HBUF6,
	DKLM_CASE_VBUF8_HBUF7,
	DKLM_CASE_VBUF8_HBUF8,
	DKLM_CASE_VBUF8_HBUF9,
	DKLM_CASE_VBUF8_HBUF10,
	DKLM_CASE_VBUF8_HBUF11,
	DKLM_CASE_VBUF8_HBUF12,
	DKLM_CASE_VBUF8_HBUF13,
	DKLM_CASE_VBUF8_HBUF14,
	DKLM_CASE_VBUF8_HBUF15,

	DKLM_CASE_VBUF9_HBUF0,
	DKLM_CASE_VBUF9_HBUF1,
	DKLM_CASE_VBUF9_HBUF2,
	DKLM_CASE_VBUF9_HBUF3,
	DKLM_CASE_VBUF9_HBUF4,
	DKLM_CASE_VBUF9_HBUF5,
	DKLM_CASE_VBUF9_HBUF6,
	DKLM_CASE_VBUF9_HBUF7,
	DKLM_CASE_VBUF9_HBUF8,
	DKLM_CASE_VBUF9_HBUF9,
	DKLM_CASE_VBUF9_HBUF10,
	DKLM_CASE_VBUF9_HBUF11,
	DKLM_CASE_VBUF9_HBUF12,
	DKLM_CASE_VBUF9_HBUF13,
	DKLM_CASE_VBUF9_HBUF14,
	DKLM_CASE_VBUF9_HBUF15,

	DKLM_CASE_VBUF10_HBUF0,
	DKLM_CASE_VBUF10_HBUF1,
	DKLM_CASE_VBUF10_HBUF2,
	DKLM_CASE_VBUF10_HBUF3,
	DKLM_CASE_VBUF10_HBUF4,
	DKLM_CASE_VBUF10_HBUF5,
	DKLM_CASE_VBUF10_HBUF6,
	DKLM_CASE_VBUF10_HBUF7,
	DKLM_CASE_VBUF10_HBUF8,
	DKLM_CASE_VBUF10_HBUF9,
	DKLM_CASE_VBUF10_HBUF10,
	DKLM_CASE_VBUF10_HBUF11,
	DKLM_CASE_VBUF10_HBUF12,
	DKLM_CASE_VBUF10_HBUF13,
	DKLM_CASE_VBUF10_HBUF14,
	DKLM_CASE_VBUF10_HBUF15,

	DKLM_CASE_VBUF11_HBUF0,
	DKLM_CASE_VBUF11_HBUF1,
	DKLM_CASE_VBUF11_HBUF2,
	DKLM_CASE_VBUF11_HBUF3,
	DKLM_CASE_VBUF11_HBUF4,
	DKLM_CASE_VBUF11_HBUF5,
	DKLM_CASE_VBUF11_HBUF6,
	DKLM_CASE_VBUF11_HBUF7,
	DKLM_CASE_VBUF11_HBUF8,
	DKLM_CASE_VBUF11_HBUF9,
	DKLM_CASE_VBUF11_HBUF10,
	DKLM_CASE_VBUF11_HBUF11,
	DKLM_CASE_VBUF11_HBUF12,
	DKLM_CASE_VBUF11_HBUF13,
	DKLM_CASE_VBUF11_HBUF14,
	DKLM_CASE_VBUF11_HBUF15,

	DKLM_CASE_VBUF12_HBUF0,
	DKLM_CASE_VBUF12_HBUF1,
	DKLM_CASE_VBUF12_HBUF2,
	DKLM_CASE_VBUF12_HBUF3,
	DKLM_CASE_VBUF12_HBUF4,
	DKLM_CASE_VBUF12_HBUF5,
	DKLM_CASE_VBUF12_HBUF6,
	DKLM_CASE_VBUF12_HBUF7,
	DKLM_CASE_VBUF12_HBUF8,
	DKLM_CASE_VBUF12_HBUF9,
	DKLM_CASE_VBUF12_HBUF10,
	DKLM_CASE_VBUF12_HBUF11,
	DKLM_CASE_VBUF12_HBUF12,
	DKLM_CASE_VBUF12_HBUF13,
	DKLM_CASE_VBUF12_HBUF14,
	DKLM_CASE_VBUF12_HBUF15,

	DKLM_CASE_VBUF13_HBUF0,
	DKLM_CASE_VBUF13_HBUF1,
	DKLM_CASE_VBUF13_HBUF2,
	DKLM_CASE_VBUF13_HBUF3,
	DKLM_CASE_VBUF13_HBUF4,
	DKLM_CASE_VBUF13_HBUF5,
	DKLM_CASE_VBUF13_HBUF6,
	DKLM_CASE_VBUF13_HBUF7,
	DKLM_CASE_VBUF13_HBUF8,
	DKLM_CASE_VBUF13_HBUF9,
	DKLM_CASE_VBUF13_HBUF10,
	DKLM_CASE_VBUF13_HBUF11,
	DKLM_CASE_VBUF13_HBUF12,
	DKLM_CASE_VBUF13_HBUF13,
	DKLM_CASE_VBUF13_HBUF14,
	DKLM_CASE_VBUF13_HBUF15,

	DKLM_CASE_VBUF14_HBUF0,
	DKLM_CASE_VBUF14_HBUF1,
	DKLM_CASE_VBUF14_HBUF2,
	DKLM_CASE_VBUF14_HBUF3,
	DKLM_CASE_VBUF14_HBUF4,
	DKLM_CASE_VBUF14_HBUF5,
	DKLM_CASE_VBUF14_HBUF6,
	DKLM_CASE_VBUF14_HBUF7,
	DKLM_CASE_VBUF14_HBUF8,
	DKLM_CASE_VBUF14_HBUF9,
	DKLM_CASE_VBUF14_HBUF10,
	DKLM_CASE_VBUF14_HBUF11,
	DKLM_CASE_VBUF14_HBUF12,
	DKLM_CASE_VBUF14_HBUF13,
	DKLM_CASE_VBUF14_HBUF14,
	DKLM_CASE_VBUF14_HBUF15,

	DKLM_CASE_VBUF15_HBUF0,
	DKLM_CASE_VBUF15_HBUF1,
	DKLM_CASE_VBUF15_HBUF2,
	DKLM_CASE_VBUF15_HBUF3,
	DKLM_CASE_VBUF15_HBUF4,
	DKLM_CASE_VBUF15_HBUF5,
	DKLM_CASE_VBUF15_HBUF6,
	DKLM_CASE_VBUF15_HBUF7,
	DKLM_CASE_VBUF15_HBUF8,
	DKLM_CASE_VBUF15_HBUF9,
	DKLM_CASE_VBUF15_HBUF10,
	DKLM_CASE_VBUF15_HBUF11,
	DKLM_CASE_VBUF15_HBUF12,
	DKLM_CASE_VBUF15_HBUF13,
	DKLM_CASE_VBUF15_HBUF14,
	DKLM_CASE_VBUF15_HBUF15,

  /*Repeated dec test*/
	DKLM_CASE_VDEC9_HDEC4,

  /*Irdeto dec test*/
	DKLM_CASE_IR_LOAD_8BYTE_CW_DST_CSA2,
	DKLM_CASE_IR_LOAD_16BYTE_CW_DST_CSA3,
	DKLM_CASE_IR_LOAD_16BYTE_CW_DST_AES_CBC,
	DKLM_CASE_IR_KCV,

	DKLM_MAX_CASE,
} AutoTestDKLMCase;

typedef enum {
	MISC_CASE_RNG_REQ,
	MISC_CASE_GET_BIV,
	MISC_CASE_TIMER_1S,
	MISC_CASE_CLOSE_MBOX,
	MISC_MAX_CASE,
} AutoTestMiscCase;

#endif
