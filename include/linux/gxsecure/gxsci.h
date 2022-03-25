/** \addtogroup <label>
 *  @{
 */
#ifndef _GX_SCI_H_
#define _GX_SCI_H_

#include "gxse_debug_level.h"

/**
 * 智能卡通讯重发协议
 */
typedef enum {
    ENABLE_REPEAT_WHEN_ERR, ///< 校验错误时重发一个字节,这样每次交互都会多发一个应答位
    DISABLE_REPEAT_WHEN_ERR ///< 校验错误时也不重发,一般建议使用此模式
} GxSciRepeatMode;

/**
 * 智能卡通讯停止位
 */
typedef enum {
    GXSCI_STOPLEN_0BIT,
    GXSCI_STOPLEN_1BIT,
    GXSCI_STOPLEN_1D5BIT,
    GXSCI_STOPLEN_2BIT
} GxSciStopLen;

/**
 * 智能卡通讯数据反转
 */
typedef enum {
    GXSCI_DATA_CONV_DIRECT,
    GXSCI_DATA_CONV_INVERSE
} GxSciIoConv;

/**
 * 智能卡通讯校验
 */
typedef enum {
    GXSCI_PARITY_EVEN,
    GXSCI_PARITY_ODD
} GxSciParityType;

/**
 * 智能卡高低电平
 */
typedef enum {
    GXSCI_LOW_LEVEL,
    GXSCI_HIGH_LEVEL
} GxSciPol;

/**
 * 智能卡状态
 */
typedef enum {
    GXSCI_CARD_INIT,
    GXSCI_CARD_IN,
    GXSCI_CARD_OUT
} GxSciCardStatus;

/**
 * 智能卡协议
 */
typedef enum {
    GXSCI_PROTCL_T0 = 0,
    GXSCI_PROTCL_T1 = 1,
    GXSCI_PROTCL_T14 = 14,
    GXSCI_PROTCL_INVALID,
} GxSciProtocol;

/**
 * 参数配置
 */
typedef struct {
	GxSciRepeatMode repeat;      ///< repeat模式配置
	GxSciStopLen    stoplen;     ///< 停止位配置
	GxSciIoConv     convention;  ///< 奇偶校验类型配置
	GxSciParityType parity;      ///< 奇偶校验类型配置
	GxSciPol        vcc;         ///< vcc 信号高低电平配置
	GxSciPol        detect;      ///< detect 信号高低电平配置

	unsigned int   clock;        ///< 智能卡工作时钟，比如9600*372
	unsigned int   ETU;          ///< 智能卡基本时间单元
	unsigned int   EGT;          ///< 表示智能卡模块发送过程中两个字节的最大时间间隔，单位是时钟周期
	unsigned int   TGT;          ///< 表示智能卡模块开始发送到发送第一个字节的最大时间间隔，单位是时钟周期
	unsigned int   WDT;          ///< 表示智能卡模块接收过程中两个字节的最大时间间隔，单位是时钟周期
	unsigned int   TWDT;         ///< 表示智能卡模块开始接收到接收到第一个字节的最大时间间隔，单位是时钟周期

	unsigned int   flags;
#define SCI_FLAG_PARAM_RPT   (0x1<<0) ///< gxdocref GxSciRepeatMode
#define SCI_FLAG_PARAM_STL   (0x1<<1) ///< gxdocref GxSciStopLen
#define SCI_FLAG_PARAM_CON   (0x1<<2) ///< gxdocref GxSciConv
#define SCI_FLAG_PARAM_PAR   (0x1<<3) ///< gxdocref GxSciParity
#define SCI_FLAG_PARAM_VCC   (0x1<<4) ///< gxdocref GxSciPol vcc 高低电平
#define SCI_FLAG_PARAM_DET   (0x1<<5) ///< gxdocref GxSciPol detect 高低电平
#define SCI_FLAG_PARAM_CLK   (0x1<<6)
#define SCI_FLAG_PARAM_ETU   (0x1<<7)
#define SCI_FLAG_PARAM_EGT   (0x1<<8)
#define SCI_FLAG_PARAM_TGT   (0x1<<9)
#define SCI_FLAG_PARAM_WDT   (0x1<<10)
#define SCI_FLAG_PARAM_TWDT  (0x1<<11)
#define SCI_FLAG_PARAM_TIME  (0xfc0)   ///< 使能时间相关参数配置
#define SCI_FLAG_PARAM_ALL   (0xffff)  ///< 使能所有参数配置
#define SCI_FLAG_AUTO_ETU    (0x1<<24) ///< 开启 ETU 自适应
#define SCI_FLAG_AUTO_PARITY (0x1<<25) ///< 开启 parity 自适应
#define SCI_FLAG_BOOST_MODE  (0x1<<26) ///< 开启增强模式
} GxSciParam;

#define SCI_PARAM_SET        GXSE_IOW ('C', 0, GxSciParam)
#define SCI_PARAM_GET        GXSE_IOR ('C', 1, GxSciParam)
#define SCI_RESET            GXSE_IO  ('C', 2)
#define SCI_DEACT            GXSE_IO  ('C', 3)
#define SCI_STATUS_GET       GXSE_IOR ('C', 4, GxSciCardStatus)
#define SCI_PRINT_REG        GXSE_IO  ('C', 5)

#endif
/** @}*/
