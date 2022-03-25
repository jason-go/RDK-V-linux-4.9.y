/** \addtogroup gxavdev
 **  @{
 **/

#ifndef __GXAV_STC_PROPERTYTYPES_H__
#define __GXAV_STC_PROPERTYTYPES_H__

/**
* STC 的恢复模式
**/
typedef struct {
	unsigned int mode;
#define AVPTS_RECOVER     0  ///< PCR  线性纠错 + Offset 调整(会参考APTS、VPTS), 推荐用于 DVB 直播
#define VPTS_RECOVER      1  ///< VPTS 硬件修复 + Offset 调整(会参考APTS), 推荐用于 PVR HWTS Playback
#define APTS_RECOVER      2  ///< APTS 硬件修复 + Offset 调整(会参考VPTS)，推荐用于 多媒体文件 Playback
#define PCR_RECOVER       3  ///< PCR 线性纠错, 适用于 DVB 直播
#define PURE_APTS_RECOVER 4  ///< 原始 APTS 修复， 适用于 多媒体文件 Playback
#define NO_RECOVER        5  ///< 自由增长，当计数器使用
#define FIXD_VPTS_RECOVER 6  ///< VPTS 硬件修复, 推荐用于 PVR HWTS Playback
#define FIXD_APTS_RECOVER 7  ///< APTS 硬件修复, 推荐用于 多媒体文件 Playback
} GxSTCProperty_Config;

typedef struct {
	int freq_HZ;
} GxSTCProperty_TimeResolution;

typedef struct {
	unsigned long long time;
} GxSTCProperty_Time;

typedef struct {
	unsigned int integer;
	unsigned int decimal;
} GxSTCProperty_Speed;

typedef struct {
	int valule;
} GxSTCProperty_Adjust;

#endif /* __GXAV_STC_PROPERTYTYPES_H__ */

/** @}*/
