#ifndef __SIRIUS_FIREWALL_REG_H__
#define __SIRIUS_FIREWALL_REG_H__

#define FW_GLOBLE_CFG               (0x000)
#define FW_FILTER_CFG0              (0x004)
#define FW_FILTER_CFG1              (0x008)
#define FW_FILTER_CFG2              (0x00C)
#define FW_FILTER00_BASE            (0x010)
#define FW_FILTER00_TOP             (0x014)
#define FW_FILTER00_RD_MASK         (0x018)
#define FW_FILTER00_WR_MASK         (0x01C)
#define FW_FILTER15_BASE            (0x100)
#define FW_FILTER15_TOP             (0x104)
#define FW_FILTER15_RD_MASK         (0x108)
#define FW_FILTER15_WR_MASK         (0x10C)
#define FW_DEFAULT_RD_MASK          (0x200)
#define FW_DEFAULT_WR_MASK          (0x204)
#define FW_CONFIG_UPLOAD            (0x300)

#define FILTER_MASK                 (8)
#define FILTER_MAX_NUM              (24)
#define FW_FILTER_SKIP              (0x10)

#define FW_DEMUX_ES_BUF_ID          (0)
#define FW_DEMUX_ES_BUF_NUM         (4)
#define FW_DEMUX_TSW_BUF_ID         (4)
#define FW_DEMUX_TSW_BUF_NUM        (2)
#define FW_GP_ES_BUF_ID             (6)
#define FW_GP_ES_BUF_NUM            (4)
#define FW_VIDEO_FW_BUF_ID          (10)
#define FW_VIDEO_FW_BUF_NUM         (1)
#define FW_VIDEO_FRAME_BUF_ID       (11)
#define FW_VIDEO_FRAME_BUF_NUM      (1)
#define FW_AUDIO_FW_BUF_ID          (12)
#define FW_AUDIO_FW_BUF_NUM         (1)
#define FW_AUDIO_FRAME_BUF_ID       (13)
#define FW_AUDIO_FRAME_BUF_NUM      (1)

#define FW_FIRST_SOFT_BUFFER_ID     (15)

#endif
