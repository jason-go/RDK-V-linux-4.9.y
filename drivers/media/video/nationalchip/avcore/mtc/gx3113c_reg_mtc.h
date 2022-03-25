/*****************************************************************************
*                          CONFIDENTIAL                             
*        Hangzhou GuoXin Science and Technology Co., Ltd.            
*                      (C)2007, All right reserved
******************************************************************************
* Purpose   :   
* Release History:
  VERSION       Date              AUTHOR         Description
  0.1           2014.10.12                  creation
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef _GX3113C_MTC_REG_H_201506181003_
#define _GX3113C_MTC_REG_H_201506181003_

/* Includes */
#include "gxav_bitops.h"

/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif


#define MTC_BASE_ADDR             (0x00fc0000)

#define M2M_DEMUX_ADDR            (0xa4000000+0x3400)
#define M2M_DEMUX_SIZE            (0xa4000000+0x3404)


/*---------------------*/


#define bKEY_READY           (2)
#define bKEY_MODE_SELECT     (17)


#define mKEY_MODE_SELECT     (0x7)


#define MTC_KEY_MODE_SELECT(rp,value)                 \
	REG_SET_FIELD(&(rp->MTC_CTRL1),     \
				mKEY_MODE_SELECT<<bKEY_MODE_SELECT ,   \
				value,bKEY_MODE_SELECT)


#define MTC_KEY_READY_T(base)           REG_SET_BIT(&(base->MTC_CTRL1), bKEY_READY)
#define MTC_KEY_READY_F(base)           REG_CLR_BIT(&(base->MTC_CTRL1), bKEY_READY)


#define bNDS_MODE_AES_DES_READY     (4)

#define MTC_NDS_MODE_AES_DES_READY_T(base)           REG_SET_BIT(&(base->MTC_CTRL2), bNDS_MODE_AES_DES_READY)
#define MTC_NDS_MODE_AES_DES_READY_F(base)           REG_CLR_BIT(&(base->MTC_CTRL2), bNDS_MODE_AES_DES_READY)


#define bKEY_SELECT_FROM_INTERFACE         (0)
#define bKEY_UPDATE_FROM_NDS_INTERFACE     (2)
#define bKEY_TABLE_SELECT_ENABLE           (3)
#define bKEY_TABLE_SELECT                  (4)
#define bKEY_WRITE_DONE                    (7)
#define bCONTROL_UNIT_WRITE_ENABLE         (8)
#define bCONTROL_UNIT_WRITE_ADDR           (9)


#define mKEY_SELECT_FROM_INTERFACE         (0x3)
#define mKEY_TABLE_SELECT                  (0x7)
#define mCONTROL_UNIT_WRITE_ADDR           (0x7)


#define MTC_KEY_SELECT_FROM_INTERFACE(rp,value)                 \
	REG_SET_FIELD(&(rp->MTC_NDS_KEY_GEN),     \
				mKEY_SELECT_FROM_INTERFACE<<bKEY_SELECT_FROM_INTERFACE ,   \
				value,bKEY_SELECT_FROM_INTERFACE)



#define MTC_DO_UPDATE_KEY(base)           REG_SET_BIT(&(base->MTC_NDS_KEY_GEN), bKEY_UPDATE_FROM_NDS_INTERFACE)
#define MTC_DONOT_UPDATE_KEY(base)        REG_CLR_BIT(&(base->MTC_NDS_KEY_GEN), bKEY_UPDATE_FROM_NDS_INTERFACE)

#define MTC_TABLE_KEY_SELECT_ENABLE_T(base)     REG_SET_BIT(&(base->MTC_NDS_KEY_GEN), bKEY_TABLE_SELECT_ENABLE)
#define MTC_TABLE_KEY_SELECT_ENABLE_F(base)     REG_CLR_BIT(&(base->MTC_NDS_KEY_GEN), bKEY_TABLE_SELECT_ENABLE)


#define MTC_KEY_TABLE_SELECT(rp,value)                 \
	REG_SET_FIELD(&(rp->MTC_NDS_KEY_GEN),     \
				mKEY_TABLE_SELECT<<bKEY_TABLE_SELECT ,   \
				value&mKEY_TABLE_SELECT,bKEY_TABLE_SELECT)


#define MTC_KEY_WRITE_DONE(base)          REG_GET_BIT(&(base->MTC_NDS_KEY_GEN), bKEY_WRITE_DONE)


#define MTC_CONTROL_UNIT_WRITE_ENABLE_T(base)     REG_SET_BIT(&(base->MTC_NDS_KEY_GEN), bCONTROL_UNIT_WRITE_ENABLE)
#define MTC_CONTROL_UNIT_WRITE_ENABLE_F(base)     REG_CLR_BIT(&(base->MTC_NDS_KEY_GEN), bCONTROL_UNIT_WRITE_ENABLE)


#define MTC_CONTROL_UNIT_WRITE_KEY_ADDR(rp,value)                 \
	REG_SET_FIELD(&(rp->MTC_NDS_KEY_GEN),     \
				mCONTROL_UNIT_WRITE_ADDR<<bCONTROL_UNIT_WRITE_ADDR ,   \
				value&mCONTROL_UNIT_WRITE_ADDR,bCONTROL_UNIT_WRITE_ADDR)



#define bMTC_NDS_SHORT_MESSAGE_MODE      (20)
#define bMTC_NDS_RESIDUE_MODE            (16)
#define bMTC_NDS_CRYPTO_MODE_SELECT      (12)
#define bMTC_NDS_CRYPTO_ALGORITHM_SELECT (8)


#define MTC_NDS_CONTROL_SET(base,v1,v2,v3,v4,v5)  \
	REG_SET_VAL(&(base->MTC_NDS_CONTROL_SET),(v4<<bMTC_NDS_SHORT_MESSAGE_MODE)|(v3<<bMTC_NDS_RESIDUE_MODE)|\
	(v2<<bMTC_NDS_CRYPTO_MODE_SELECT)|(v1<<bMTC_NDS_CRYPTO_ALGORITHM_SELECT)|(v5))



#define bKEY_WRITE_ADDR                  (0)
#define mKEY_WRITE_ADDR                  (0x7)

#define MTC_WRITE_KEY_ADDR(rp,value)                 \
	REG_SET_FIELD(&(rp->MTC_NDS_KEY_WRITE_ADDR),     \
				mKEY_WRITE_ADDR<<bKEY_WRITE_ADDR ,   \
				value,bKEY_WRITE_ADDR)



#define bKEY_TABLE_FULL                  (0)

#define MTC_KEY_TABLE_IS_FULL(base)       REG_GET_BIT(&(base->MTC_NDS_KEY_FULL), bKEY_TABLE_FULL)
#define MTC_KEY_TABLE_CLEAR_FULL(base)    REG_CLR_BIT(&(base->MTC_NDS_KEY_FULL), bKEY_TABLE_FULL)



#define MTC_SET_COUNTER(base,i,value)  REG_SET_VAL(&(base->MTC_COUNTER[i]), value)
#define MTC_SET_IV1(base,i,value)      REG_SET_VAL(&(base->MTC_IV1[i]), value)
#define MTC_SET_IV2(base,i,value)      REG_SET_VAL(&(base->MTC_IV2[i]), value)

#define MTC_SET_READ_ADDR(base,value)      REG_SET_VAL(&(base->MTCR_SDR_START_ADDR), value)
#define MTC_SET_WRITE_ADDR(base,value)     REG_SET_VAL(&(base->MTCW_SDR_START_ADDR), value)
#define MTC_SET_DATA_SIZE(base,value)      REG_SET_VAL(&(base->MTCR_SDR_DATA_LEN), value)


#define bDATA_FINISH_INT_EN  (0)

#define MTC_DATA_FINISH_INT_EN_T(base)     REG_SET_BIT(&(base->MTC_INT_EN), bDATA_FINISH_INT_EN)
#define MTC_DATA_FINISH_INT_EN_F(base)     REG_CLR_BIT(&(base->MTC_INT_EN), bDATA_FINISH_INT_EN)


#define MTC_GET_DATA_FINISH_INT(base)       REG_GET_BIT(&(base->MTC_INT), bDATA_FINISH_INT_EN)
#define MTC_CLEAR_DATA_FINISH_INT(base)     REG_SET_BIT(&(base->MTC_INT), bDATA_FINISH_INT_EN)



/*
------------------- start 2 -----------------------------------------------
*/


#define bMTC_DES_KEY_MULTI_LAYER_USE_LOW (25)
#define bMTC_SHORT_MODE                  (23)
#define bMTC_RESIDUE_MODE                (21)
#define bMTC_DES_KEY_USE_LOW             (20)
#define bMTC_DATA_ENCRY_KEY_SELECT       (17)
#define bMTC_BIG_ENDIAN                  (16)
#define bMTC_M                           (10)
#define bMTC_OPTION_MODE                 (7)
#define bMTC_WORK_MODE                   (5)
#define bMTC_AES_MODE                    (3)
#define bMTC_KEY_RDY                     (2)
#define bMTC_ENCRYPT                     (1)
#define bMTC_TRI_DES                     (0)


#define MTC_DES_KEY_MULTI_LAYER_USE_LOW_T(base) REG_SET_BIT(&(base->MTC_CTRL1), bMTC_DES_KEY_MULTI_LAYER_USE_LOW)
#define MTC_DES_KEY_MULTI_LAYER_USE_LOW_F(base) REG_CLR_BIT(&(base->MTC_CTRL1), bMTC_DES_KEY_MULTI_LAYER_USE_LOW)

#define wMTC_SHORT_MODE   (0x3)
#define mMTC_SHORT_MODE   (wMTC_SHORT_MODE<<bMTC_SHORT_MODE)
#define MTC_SHORT_MODE(base, value)    \
    REG_SET_FIELD(&(base->MTC_CTRL1),mMTC_SHORT_MODE,(value & wMTC_SHORT_MODE),bMTC_SHORT_MODE)

#define wMTC_RESIDUE_MODE   (0x3)
#define mMTC_RESIDUE_MODE   (wMTC_RESIDUE_MODE<<bMTC_RESIDUE_MODE)
#define MTC_RESIDUE_MODE(base, value)    \
    REG_SET_FIELD(&(base->MTC_CTRL1),mMTC_RESIDUE_MODE,(value & wMTC_RESIDUE_MODE),bMTC_RESIDUE_MODE)

#define MTC_DES_KEY_USE_LOW_T(base)  REG_SET_BIT(&(base->MTC_CTRL1),bMTC_DES_KEY_USE_LOW)
#define MTC_DES_KEY_USE_LOW_F(base)  REG_CLR_BIT(&(base->MTC_CTRL1),bMTC_DES_KEY_USE_LOW)

#define wMTC_DATA_ENCRY_KEY_SELECT   (0x7)
#define mMTC_DATA_ENCRY_KEY_SELECT   (wMTC_DATA_ENCRY_KEY_SELECT<<bMTC_DATA_ENCRY_KEY_SELECT)
#define MTC_DATA_ENCRY_KEY_SELECT(base, value)    \
    REG_SET_FIELD(&(base->MTC_CTRL1),mMTC_DATA_ENCRY_KEY_SELECT,(value & wMTC_DATA_ENCRY_KEY_SELECT),bMTC_DATA_ENCRY_KEY_SELECT)

#define MTC_BIG_ENDIAN_T(base)  REG_SET_BIT(&(base->MTC_CTRL1),bMTC_BIG_ENDIAN)
#define MTC_BIG_ENDIAN_F(base)  REG_CLR_BIT(&(base->MTC_CTRL1),bMTC_BIG_ENDIAN)

#define wMTC_M   (0x3F)
#define mMTC_M   (wMTC_M<<bMTC_M)
#define MTC_M(base, value)    \
    REG_SET_FIELD(&(base->MTC_CTRL1),mMTC_M,(value & wMTC_M),bMTC_M)

#define wMTC_OPTION_MODE   (0x7)
#define mMTC_OPTION_MODE   (wMTC_OPTION_MODE<<bMTC_OPTION_MODE)
#define MTC_OPTION_MODE(base, value)    \
    REG_SET_FIELD(&(base->MTC_CTRL1),mMTC_OPTION_MODE,(value & wMTC_OPTION_MODE),bMTC_OPTION_MODE)

#define wMTC_WORK_MODE   (0x3)
#define mMTC_WORK_MODE   (wMTC_WORK_MODE<<bMTC_WORK_MODE)
#define MTC_WORK_MODE(base, value)    \
    REG_SET_FIELD(&(base->MTC_CTRL1),mMTC_WORK_MODE,(value & wMTC_WORK_MODE) ,bMTC_WORK_MODE)

#define wMTC_AES_MODE   (0x3)
#define mMTC_AES_MODE   (wMTC_AES_MODE<<bMTC_AES_MODE)
#define MTC_AES_MODE(base, value)    \
    REG_SET_FIELD(&(base->MTC_CTRL1),mMTC_AES_MODE,(value & wMTC_AES_MODE),bMTC_AES_MODE)

#define MTC_KEY_RDY_T(base)  REG_SET_BIT(&(base->MTC_CTRL1),bMTC_KEY_RDY)
#define MTC_KEY_RDY_F(base)  REG_CLR_BIT(&(base->MTC_CTRL1),bMTC_KEY_RDY)

#define MTC_ENCRYPT_T(base)  REG_SET_BIT(&(base->MTC_CTRL1),bMTC_ENCRYPT)
#define MTC_ENCRYPT_F(base)  REG_CLR_BIT(&(base->MTC_CTRL1),bMTC_ENCRYPT)

#define MTC_TRI_DES_T(base)  REG_SET_BIT(&(base->MTC_CTRL1),bMTC_TRI_DES)
#define MTC_TRI_DES_F(base)  REG_CLR_BIT(&(base->MTC_CTRL1),bMTC_TRI_DES)

////
#define bMTCR_SDR_START_ADDR         (0)

#define wMTCR_SDR_START_ADDR   (0xFFFFFFF)
#define mMTCR_SDR_START_ADDR   (wMTCR_SDR_START_ADDR<<bMTCR_SDR_START_ADDR)
#define MTCR_SDR_START_ADDR(base, value)    \
    REG_SET_FIELD(&(base->MTCR_SDR_START_ADDR),mMTCR_SDR_START_ADDR,(value & wMTCR_SDR_START_ADDR),bMTCR_SDR_START_ADDR)

////	
#define bMTCW_SDR_START_ADDR         (0)

#define wMTCW_SDR_START_ADDR   (0xFFFFFFF)
#define mMTCW_SDR_START_ADDR   (wMTCW_SDR_START_ADDR<<bMTCW_SDR_START_ADDR)
#define MTCW_SDR_START_ADDR(base, value)    \
    REG_SET_FIELD(&(base->MTCW_SDR_START_ADDR),mMTCW_SDR_START_ADDR,(value & wMTCW_SDR_START_ADDR),bMTCW_SDR_START_ADDR)

////
#define bMTCR_SDR_DATA_LEN         (0)

#define wMTCR_SDR_DATA_LEN   (0x1FFFFF)
#define mMTCR_SDR_DATA_LEN   (wMTCR_SDR_DATA_LEN<<bMTCR_SDR_DATA_LEN)
#define MTCR_SDR_DATA_LEN(base, value)    \
    REG_SET_FIELD(&(base->MTCR_SDR_DATA_LEN),mMTCR_SDR_DATA_LEN,(value & wMTCR_SDR_DATA_LEN),bMTCR_SDR_DATA_LEN)

////
#define bMTC_NDS_READY                  (4)
#define bMTC_SMS4_READY                 (3)
#define bMTC_MULTI2_READY               (2)
#define bMTC_ASE_READY                  (1)
#define bMTC_DES_READY                  (0)


#define MTC_NDS_READY_T(base)  REG_SET_BIT(&(base->MTC_CTRL2),bMTC_NDS_READY)
#define MTC_NDS_READY_F(base)  REG_CLR_BIT(&(base->MTC_CTRL2),bMTC_NDS_READY)

#define MTC_SMS4_READY_T(base)  REG_SET_BIT(&(base->MTC_CTRL2),bMTC_SMS4_READY)
#define MTC_SMS4_READY_F(base)  REG_CLR_BIT(&(base->MTC_CTRL2),bMTC_SMS4_READY)

#define MTC_MULTI2_READY_T(base)  REG_SET_BIT(&(base->MTC_CTRL2),bMTC_MULTI2_READY)
#define MTC_MULTI2_READY_F(base)  REG_CLR_BIT(&(base->MTC_CTRL2),bMTC_MULTI2_READY)

#define MTC_ASE_READY_T(base)  REG_SET_BIT(&(base->MTC_CTRL2),bMTC_ASE_READY)
#define MTC_ASE_READY_F(base)  REG_CLR_BIT(&(base->MTC_CTRL2),bMTC_ASE_READY)

#define MTC_DES_READY_T(base)  REG_SET_BIT(&(base->MTC_CTRL2),bMTC_DES_READY)
#define MTC_DES_READY_F(base)  REG_CLR_BIT(&(base->MTC_CTRL2),bMTC_DES_READY)

////
#define bMTC_OTP_KEY_LENGTH              (5)
#define bMTC_CA_KEY_LENGTH               (4)
#define bMTC_DCW_READY                   (3)
#define bMTC_DCK_READY                   (2)
#define bMTC_DSK_READY                   (1)
#define bMTC_CA_MODE                     (0)


#define MTC_OTP_KEY_LENGTH_T(base)  REG_SET_BIT(&(base->MTC_CA_MODE),bMTC_OTP_KEY_LENGTH)
#define MTC_OTP_KEY_LENGTH_F(base)  REG_CLR_BIT(&(base->MTC_CA_MODE),bMTC_OTP_KEY_LENGTH)

#define MTC_CA_KEY_LENGTH_T(base)  REG_SET_BIT(&(base->MTC_CA_MODE),bMTC_CA_KEY_LENGTH)
#define MTC_CA_KEY_LENGTH_F(base)  REG_CLR_BIT(&(base->MTC_CA_MODE),bMTC_CA_KEY_LENGTH)

#define MTC_DCW_READY_T(base)  REG_SET_BIT(&(base->MTC_CA_MODE),bMTC_DCW_READY)
#define MTC_DCW_READY_F(base)  REG_CLR_BIT(&(base->MTC_CA_MODE),bMTC_DCW_READY)

#define MTC_DCK_READY_T(base)  REG_SET_BIT(&(base->MTC_CA_MODE),bMTC_DCK_READY)
#define MTC_DCK_READY_F(base)  REG_CLR_BIT(&(base->MTC_CA_MODE),bMTC_DCK_READY)

#define MTC_DSK_READY_T(base)  REG_SET_BIT(&(base->MTC_CA_MODE),bMTC_DSK_READY)
#define MTC_DSK_READY_F(base)  REG_CLR_BIT(&(base->MTC_CA_MODE),bMTC_DSK_READY)

#define MTC_CA_MODE_T(base)  REG_SET_BIT(&(base->MTC_CA_MODE),bMTC_CA_MODE)
#define MTC_CA_MODE_F(base)  REG_CLR_BIT(&(base->MTC_CA_MODE),bMTC_CA_MODE)


////
#define bMTC_NDS_SHORT_MESSAGE_MODE      (20)
#define bMTC_NDS_RESIDUE_MODE            (16)
#define bMTC_NDS_CRYPTO_MODE_SELECT      (12)
#define bMTC_NDS_CRYPTO_ALGORITHM_SELECT (8)
#define bMTC_NDS_DECRY                   (1)
#define bMTC_NDS_ENCRY                   (0)

#define MTC_CONTROL_SET(base,v1,v2,v3,v4,v5)  \
	REG_SET_VAL(&(base->MTC_NDS_CONTROL_SET),(v4<<bMTC_NDS_SHORT_MESSAGE_MODE)|(v3<<bMTC_NDS_RESIDUE_MODE)|\
	(v2<<bMTC_NDS_CRYPTO_MODE_SELECT)|(v1<<bMTC_NDS_CRYPTO_ALGORITHM_SELECT)|(v5))

////
#define bMTC_NDS_CONTROL_UNIT_WRITE_ADDR (9)
#define bMTC_NDS_CONTROL_UNIT_WRITE_EN   (8)
#define bMTC_NDS_KEY_WRITE_DONE          (7)
#define bMTC_NDS_KEY_TABLE_SELECT        (4)
#define bMTC_NDS_KEY_TABLE_SELECT_EN     (3)
#define bMTC_NDS_KEY_UPDATE_READY        (2)
#define bMTC_NDS_KEY_SELECT              (0)


#define MTC_NDS_KEY_TABLE_SELECT_EN_T(base)  REG_SET_BIT(&(base->MTC_NDS_KEY_GEN),bMTC_NDS_KEY_TABLE_SELECT_EN)
#define MTC_NDS_KEY_TABLE_SELECT_EN_F(base)  REG_CLR_BIT(&(base->MTC_NDS_KEY_GEN),bMTC_NDS_KEY_TABLE_SELECT_EN)

#define wMTC_NDS_KEY_TABLE_SELECT   (0x7)
#define mMTC_NDS_KEY_TABLE_SELECT   (wMTC_NDS_KEY_TABLE_SELECT<<bMTC_NDS_KEY_TABLE_SELECT)
#define MTC_NDS_KEY_TABLE_SELECT(base, value)    \
    REG_SET_FIELD(&(base->MTC_NDS_KEY_GEN),mMTC_NDS_KEY_TABLE_SELECT,(value & wMTC_NDS_KEY_TABLE_SELECT),bMTC_NDS_KEY_TABLE_SELECT)

#define MTC_NDS_CONTROL_UNIT_WRITE_EN_T(base)  REG_SET_BIT(&(base->MTC_NDS_KEY_GEN),bMTC_NDS_CONTROL_UNIT_WRITE_EN)
#define MTC_NDS_CONTROL_UNIT_WRITE_EN_F(base)  REG_CLR_BIT(&(base->MTC_NDS_KEY_GEN),bMTC_NDS_CONTROL_UNIT_WRITE_EN)


#define wMTC_NDS_CONTROL_UNIT_WRITE_ADDR   (0x7)
#define mMTC_NDS_CONTROL_UNIT_WRITE_ADDR   (wMTC_NDS_CONTROL_UNIT_WRITE_ADDR<<bMTC_NDS_CONTROL_UNIT_WRITE_ADDR)
#define MTC_NDS_CONTROL_UNIT_WRITE_ADDR(base, value)    \
    REG_SET_FIELD(&(base->MTC_NDS_KEY_GEN),mMTC_NDS_CONTROL_UNIT_WRITE_ADDR,(value & wMTC_NDS_CONTROL_UNIT_WRITE_ADDR),bMTC_NDS_CONTROL_UNIT_WRITE_ADDR)


////
#define bMTC_NDS_KEY_WRITE_ADDR          (0)

////
#define bMTC_NDS_KEY_FULL                (0)

////

#define bMTC_MEMORY_KEY_WRITE_ADDR       (0)

#define wMTC_MEMORY_KEY_WRITE_ADDR   (0x3)
#define mMTC_MEMORY_KEY_WRITE_ADDR   (wMTC_MEMORY_KEY_WRITE_ADDR<<bMTC_MEMORY_KEY_WRITE_ADDR)
#define MTC_MEMORY_KEY_WRITE_ADDR(base, value)    \
    REG_SET_FIELD(&(base->MEMORY_KEY_WRITE_ADDR),mMTC_MEMORY_KEY_WRITE_ADDR,(value & wMTC_MEMORY_KEY_WRITE_ADDR),bMTC_MEMORY_KEY_WRITE_ADDR)


////
#define bMTC_MULTI_LAYER_KEY_EN          (9)
#define bMTC_CW_AES_HIGH_LOW_SELECT      (8)
#define bMTC_CW_3_ROUND_CDCAS_TYPE       (6)
#define bMTC_CW_3_ROUND_TYPE             (4)
#define bMTC_CW_3_ROUND                  (3)
#define bMTC_CW_2_ROUND                  (2)
#define bMTC_CW_1_ROUND                  (1)
#define bMTC_CW_EN                       (0)


#define MTC_MULTI_LAYER_KEY_EN_T(base)  REG_SET_BIT(&(base->MTC_CW_SELECT),bMTC_MULTI_LAYER_KEY_EN)
#define MTC_MULTI_LAYER_KEY_EN_F(base)  REG_CLR_BIT(&(base->MTC_CW_SELECT),bMTC_MULTI_LAYER_KEY_EN)

#define MTC_CW_AES_HIGH_LOW_SELECT_T(base)  REG_SET_BIT(&(base->MTC_CW_SELECT),bMTC_CW_AES_HIGH_LOW_SELECT)
#define MTC_CW_AES_HIGH_LOW_SELECT_F(base)  REG_CLR_BIT(&(base->MTC_CW_SELECT),bMTC_CW_AES_HIGH_LOW_SELECT)

#define wMTC_CW_3_ROUND_CDCAS_TYPE   (0x3)
#define mMTC_CW_3_ROUND_CDCAS_TYPE   (wMTC_CW_3_ROUND_CDCAS_TYPE<<bMTC_CW_3_ROUND_CDCAS_TYPE)
#define MTC_CW_3_ROUND_CDCAS_TYPE(base, value)    \
    REG_SET_FIELD(&(base->MTC_CW_SELECT),mMTC_CW_3_ROUND_CDCAS_TYPE,(value & wMTC_CW_3_ROUND_CDCAS_TYPE),bMTC_CW_3_ROUND_CDCAS_TYPE)

#define wMTC_CW_3_ROUND_TYPE   (0x3)
#define mMTC_CW_3_ROUND_TYPE   (wMTC_CW_3_ROUND_TYPE<<bMTC_CW_3_ROUND_TYPE)
#define MTC_CW_3_ROUND_TYPE(base, value)    \
    REG_SET_FIELD(&(base->MTC_CW_SELECT),mMTC_CW_3_ROUND_TYPE,(value & wMTC_CW_3_ROUND_TYPE),bMTC_CW_3_ROUND_TYPE)

#define MTC_CW_3_ROUND_T(base)  REG_SET_BIT(&(base->MTC_CW_SELECT),bMTC_CW_3_ROUND)
#define MTC_CW_3_ROUND_F(base)  REG_CLR_BIT(&(base->MTC_CW_SELECT),bMTC_CW_3_ROUND)

#define MTC_CW_2_ROUND_T(base)  REG_SET_BIT(&(base->MTC_CW_SELECT),bMTC_CW_2_ROUND)
#define MTC_CW_2_ROUND_F(base)  REG_CLR_BIT(&(base->MTC_CW_SELECT),bMTC_CW_2_ROUND)

#define MTC_CW_1_ROUND_T(base)  REG_SET_BIT(&(base->MTC_CW_SELECT),bMTC_CW_1_ROUND)
#define MTC_CW_1_ROUND_F(base)  REG_CLR_BIT(&(base->MTC_CW_SELECT),bMTC_CW_1_ROUND)

#define MTC_CW_EN_T(base)  REG_SET_BIT(&(base->MTC_CW_SELECT),bMTC_CW_EN)
#define MTC_CW_EN_F(base)  REG_CLR_BIT(&(base->MTC_CW_SELECT),bMTC_CW_EN)

////
#define bMTC_VENDER_SYSID                (4)
#define bMTC_MODULE_ID                   (1)
#define bMTC_K3_GEN_EN                   (0)

#define wMTC_VENDER_SYSID   (0x3)
#define mMTC_VENDER_SYSID   (wMTC_VENDER_SYSID<<bMTC_VENDER_SYSID)
#define MTC_VENDER_SYSID(base, value)    \
    REG_SET_FIELD(&(base->MTC_CA_K3_GEN),mMTC_VENDER_SYSID,(value & wMTC_VENDER_SYSID),bMTC_VENDER_SYSID)

#define wMTC_MODULE_ID   (0x3)
#define mMTC_MODULE_ID   (wMTC_MODULE_ID<<bMTC_MODULE_ID)
#define MTC_MODULE_ID(base, value)    \
    REG_SET_FIELD(&(base->MTC_CA_K3_GEN),mMTC_MODULE_ID,(value & wMTC_MODULE_ID),bMTC_MODULE_ID)

#define MTC_K3_GEN_EN_T(base)  REG_SET_BIT(&(base->MTC_CA_K3_GEN),bMTC_K3_GEN_EN)
#define MTC_K3_GEN_EN_F(base)  REG_CLR_BIT(&(base->MTC_CA_K3_GEN),bMTC_K3_GEN_EN)

////
#define bMTC_TDES_ROUND_FINISH_INT_EN      (3)
#define bMTC_ASE_ROUND_FINISH_INT_EN       (2)
#define bMTC_K3_FINISH_INT_EN              (1)
#define bMTC_DATA_FINISH_INT_EN            (0)


#define MTC_TDES_ROUND_FINISH_INT_EN_T(base)  REG_SET_BIT(&(base->MTC_INT_EN),bMTC_TDES_ROUND_FINISH_INT_EN)
#define MTC_TDES_ROUND_FINISH_INT_EN_F(base)  REG_CLR_BIT(&(base->MTC_INT_EN),bMTC_TDES_ROUND_FINISH_INT_EN)

#define MTC_ASE_ROUND_FINISH_INT_EN_T(base)  REG_SET_BIT(&(base->MTC_INT_EN),bMTC_ASE_ROUND_FINISH_INT_EN)
#define MTC_ASE_ROUND_FINISH_INT_EN_F(base)  REG_CLR_BIT(&(base->MTC_INT_EN),bMTC_ASE_ROUND_FINISH_INT_EN)

#define MTC_K3_FINISH_INT_EN_T(base)  REG_SET_BIT(&(base->MTC_INT_EN),bMTC_K3_FINISH_INT_EN)
#define MTC_K3_FINISH_INT_EN_F(base)  REG_CLR_BIT(&(base->MTC_INT_EN),bMTC_K3_FINISH_INT_EN)

#define MTC_DATA_FINISH_INTTERUPT_EN_T(base)  REG_SET_BIT(&(base->MTC_INT_EN),bMTC_DATA_FINISH_INT_EN)
#define MTC_DATA_FINISH_INTTERUPT_EN_F(base)  REG_CLR_BIT(&(base->MTC_INT_EN),bMTC_DATA_FINISH_INT_EN)


#define MTC_ALL_INT_EN(base,value)  REG_SET_VAL(&(base->MTC_INT_EN),value)

////
#define bMTC_TDES_ROUND_FINISH_INT         (3)
#define bMTC_ASE_ROUND_FINISH_INT          (2)
#define bMTC_K3_FINISH_INT                 (1)
#define bMTC_DATA_FINISH_INT               (0)


#define MTC_TDES_ROUND_FINISH_INT_T(base)  REG_SET_BIT(&(base->MTC_INT),bMTC_TDES_ROUND_FINISH_INT)
#define MTC_TDES_ROUND_FINISH_INT_F(base)  REG_CLR_BIT(&(base->MTC_INT),bMTC_TDES_ROUND_FINISH_INT)

#define MTC_ASE_ROUND_FINISH_INT_T(base)  REG_SET_BIT(&(base->MTC_INT),bMTC_ASE_ROUND_FINISH_INT)
#define MTC_ASE_ROUND_FINISH_INT_F(base)  REG_CLR_BIT(&(base->MTC_INT),bMTC_ASE_ROUND_FINISH_INT)

#define MTC_K3_FINISH_INT_T(base)  REG_SET_BIT(&(base->MTC_INT),bMTC_K3_FINISH_INT)
#define MTC_K3_FINISH_INT_F(base)  REG_CLR_BIT(&(base->MTC_INT),bMTC_K3_FINISH_INT)

#define MTC_DATA_FINISH_INT_T(base)  REG_SET_BIT(&(base->MTC_INT),bMTC_DATA_FINISH_INT)
#define MTC_DATA_FINISH_INT_F(base)  REG_CLR_BIT(&(base->MTC_INT),bMTC_DATA_FINISH_INT)

#define MTC_CLEAR_INT(base,bit)  REG_SET_BIT(&(base->MTC_INT),bit)
#define MTC_GET_ALL_INT_VALUE(base,value)  value = REG_GET_VAL(&(base->MTC_INT))

////
#define MTC_CA_ADDR(base,value)  REG_SET_VAL(&(base->MTC_CA_ADDR),value)

////
#define MTC_KEY_1_VALUE_L(base,value)  REG_SET_VAL(&(base->MTC_KEY1_L),value)
#define MTC_KEY_1_VALUE_H(base,value)  REG_SET_VAL(&(base->MTC_KEY1_H),value)
#define MTC_KEY_2_VALUE_L(base,value)  REG_SET_VAL(&(base->MTC_KEY2_L),value)
#define MTC_KEY_2_VALUE_H(base,value)  REG_SET_VAL(&(base->MTC_KEY2_H),value)
#define MTC_KEY_3_VALUE_L(base,value)  REG_SET_VAL(&(base->MTC_KEY3_L),value)
#define MTC_KEY_3_VALUE_H(base,value)  REG_SET_VAL(&(base->MTC_KEY3_H),value)
#define MTC_KEY_4_VALUE_L(base,value)  REG_SET_VAL(&(base->MTC_KEY4_L),value)
#define MTC_KEY_4_VALUE_H(base,value)  REG_SET_VAL(&(base->MTC_KEY4_H),value)
////
#define MTC_DSK_VALUE_BY_INDEX(base,id,value)  REG_SET_VAL(&(base->MTC_DSK[id]),value)
////
#define MTC_DCK_VALUE_BY_INDEX(base,id,value)  REG_SET_VAL(&(base->MTC_DCK[id]),value)
////
#define MTC_DCW_VALUE_BY_INDEX(base,id,value)  REG_SET_VAL(&(base->MTC_DCW[id]),value)

////
#define MTC_COUNTER_VALUE_BY_INDEX(base,id,value)  REG_SET_VAL(&(base->MTC_COUNTER[id]),value)

////
#define MTC_IV1_VALUE_BY_INDEX(base,id,value)  REG_SET_VAL(&(base->MTC_IV1[id]),value)

////
#define MTC_IV2_VALUE_BY_INDEX(base,id,value)  REG_SET_VAL(&(base->MTC_IV2[id]),value)


/*

		struct 
*/

typedef struct MTC_Regs_s
{
	unsigned int     MTC_CTRL1   ;          // 0x00
	unsigned int     MTC_KEY1_L  ;          // 0x04
	unsigned int     MTC_KEY1_H  ;          // 0x08
	unsigned int     MTC_KEY2_L  ;          // 0x0c
	unsigned int     MTC_KEY2_H  ;          // 0x10
	unsigned int     MTC_KEY3_L  ;          // 0x14
	unsigned int     MTC_KEY3_H  ;          // 0x18
	unsigned int     MTCR_SDR_START_ADDR ;  // 0x1c
	unsigned int     MTCW_SDR_START_ADDR ;  // 0x20
	unsigned int     MTCR_SDR_DATA_LEN   ;  // 0x24
	unsigned int     MTC_CTRL2   ;          // 0x28
	unsigned int     MTC_KEY4_L  ;          // 0x2c
	unsigned int     MTC_KEY4_H  ;          // 0x30
	unsigned int     MTC_COUNTER[4]  ;      // 0x34,38,3c,40
	unsigned int     MTC_IV1[4]      ;      // 0x44,48,4c,50
	unsigned int     MTC_MULTI_DATA_KEY_H ; // 0x54
	unsigned int     MTC_MULTI_DATA_KEY_L ; // 0x58
	unsigned int     MTC_CA_MODE ;          // 0x5c
	unsigned int     MTC_DCK[6]  ;          // 0x60,64,68,6c,70,74
	unsigned int     MTC_DCW[4]  ;          // 0x78,7c,80,84
	unsigned int     MTC_Reserv_1[2]   ;    // 0x88,8c
	unsigned int     MTC_CA_ADDR   ;        // 0x90
	unsigned int     MTC_Reserv_2[2]   ;    // 0x94,98
	unsigned int     MTC_NDS_CONTROL_SET ;  // 0x9c
	unsigned int     MTC_NDS_KEY_GEN ;      // 0xa0
	unsigned int     MTC_NDS_KEY_WRITE_ADDR;// 0xa4
	unsigned int     MTC_NDS_KEY_FULL ;     // 0xa8
	unsigned int     MTC_Reserv_3[21]   ;   // 0xac, 0xb0,0xc0,0xd0,0xe0,0xf0
	unsigned int     MTC_DSK[6] ;           // 0x100,....0x110,0x114 
	unsigned int     MTC_Reserv_4[2]   ;    // 0x118,0x11c
	unsigned int     MTC_CW_SELECT     ;    // 0x120
	unsigned int     MTC_NONCE[4]      ;    // 0x124,128,12c,130
	unsigned int     MTC_DA[4]      ;       // 0x134,138,13c,140
	unsigned int     MTC_Reserv_5[3]   ;    // 0x144,148,14c
	unsigned int     MTC_CA_K3_GEN     ;    // 0x150
	unsigned int     MTC_Reserv_6[3]   ;    // 0x154,158,15c
	unsigned int     MTC_INT_EN        ;    // 0x160
	unsigned int     MTC_INT           ;    // 0x164
	unsigned int     MTC_Reserv_7[6]   ;    // 0x168,16c,170,174,178,17c
	unsigned int     MTC_IV2[4]      ;      // 0x180,184,188,18c
}MTC_Regs_t;


#ifdef __cplusplus
}
#endif

#endif

