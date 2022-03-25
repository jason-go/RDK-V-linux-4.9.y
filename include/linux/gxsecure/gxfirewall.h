/** \addtogroup <label>
 *  @{
 */
#ifndef _GX_FIREWALL_H_
#define _GX_FIREWALL_H_

#include "gxse_debug_level.h"

/**
 * 受保护的内存区域类型
 *
 */
enum Gx_Firewall_Protect_Buffer {
	GXFW_BUFFER_DEMUX_ES       = (1 << 0),    ///< ES保护buffer
	GXFW_BUFFER_DEMUX_TSW      = (1 << 1),    ///< TSW保护buffer
	GXFW_BUFFER_DEMUX_TSR      = (1 << 2),    ///< TSR保护buffer
	GXFW_BUFFER_GP_ES          = (1 << 3),    ///< GP模块ES保护buffer
	GXFW_BUFFER_AUDIO_FIRMWARE = (1 << 4),    ///< 音频固件保护buffer
	GXFW_BUFFER_AUDIO_FRAME    = (1 << 5),    ///< 音频帧保护buffer
	GXFW_BUFFER_VIDEO_FIRMWARE = (1 << 6),    ///< 视频固件保护buffer
	GXFW_BUFFER_VIDEO_FRAME    = (1 << 7),    ///< 视频帧保护buffer
	GXFW_BUFFER_VPU_CAP        = (1 << 8),    ///< vpu保护buffer
	GXFW_BUFFER_SCPU           = (1 << 9),    ///< scpu固件保护buffer
	GXFW_BUFFER_SOFT           = (1 << 15),   ///< 标识当前方案处在软件 Firewall 保护模式
};

#define FIREWALL_GET_PROTECT_BUFFER     GXSE_IOR('f', 0, unsigned int)
#define FIREWALL_QUERY_ACCESS_ALIGN     GXSE_IOR('f', 1, unsigned int)
#define FIREWALL_QUERY_PROTECT_ALIGN    GXSE_IOR('f', 2, unsigned int)

#endif
/** @}*/
