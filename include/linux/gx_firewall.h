#ifndef __GX_FIREWALL_H__
#define __GX_FIREWALL_H__


#define MASTER_DEMUX_ES				(1<<1)
#define MASTER_DEMUX_TS_WRITE			(1<<2)
#define MASTER_DEMUX_TS_READ			(1<<3)
#define MASTER_DEMUX_OTHER			(1<<4)
#define MASTER_PIDFILTER			(1<<7)
#define MASTER_GSE				(1<<8)
#define MASTER_GP_READ				(1<<9)
#define MASTER_GP_WRITE				(1<<10)
#define MASTER_VIDEO_FW				(1<<11)
#define MASTER_VIDEO_FRAME			(1<<12)
#define MASTER_AUDIODEC_FW			(1<<13)
#define MASTER_AUDIODEC_CM			(1<<14)
#define MASTER_PP				(1<<15)
#define MASTER_GA				(1<<17)
#define MASTER_JPEG				(1<<18)
#define MASTER_VPU				(1<<19)
#define MASTER_AUDIOPLAY			(1<<21)
#define MASTER_M2M_NORMAL_ENCRYPT		(1<<22)
#define MASTER_M2M_NORMAL_DECRYPT		(1<<23)
#define MASTER_M2M_PRV_ENCRYPT			(1<<24)
#define MASTER_M2M_PRV_DECRYPT			(1<<25)
#define MASTER_M2M_FW_DECRYPT			(1<<26)
#define MASTER_RCC				(1<<28)
#define MASTER_HASH				(1<<29)
#define MASTER_MAC_USB_NFC			(1<<30)
#define MASTER_A7_TEE                           (1<<31)
#define MASTER_A7_TEE_REE                       (1<<0)

/*
 * firewall_config_filter()
 *
 *  DESCRIPTION
 *      Set master read and write permissions to filter(can also understand the filter as buf)
 *
 *  PARAMETER
 *      @addr
 *           The start addr in RAM of this filter, addr must aligned by 1KB
 *      @size
 *           The size in RAM of this filter, size must aligned by 1KB
 *      @master_rd_permission
 *           Configure which masters have read permissions to this filter, example MASTER_A7_TEE|MASTER_MAC_USB_NFC
 *      @master_wr_permission
 *           Configure which masters have write permissions to this filter, example MASTER_A7_TEE|MASTER_MAC_USB_NFC
 *
 *  RETURN VALUE
 *      On sucess, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 *  EXAMPLE
 *      firewall_config_filter(0x10000000, 0x200000, MASTER_A7_TEE | MASTER_MAC_USB_NFC, MASTER_GSE | MASTER_A7_TEE | MASTER_VPU);
 *      set protect space (0x10000000 ~ 0x10200000)，set master A7_TEE/MAC/USB/NFC has read permission, master GSE/A7_TEE/VPU has write permission
 *
 * */
int firewall_config_filter(int addr, int size, int master_rd_permission, int master_wr_permission);

#endif
