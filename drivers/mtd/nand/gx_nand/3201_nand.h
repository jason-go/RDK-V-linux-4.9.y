#ifndef __GX320X_NAND_H
#define __GX320X_NAND_H

#define CONFIG_MTD_NAND_GX320X
#define CONFIG_MTD_NAND_GX320X_HWECC
#define CONFIG_GXNAND_SUPPORT

#define GX320X_NAND_256_PAGESIZE 256
#define GX320X_NAND_512_PAGESIZE 512
#define GX320X_NAND_2K_PAGESIZE  2048

#define ECC_REG_OFFSET 0x100
#define WRONG_LOCATION_REG_OFFSET 0x200
#define GX320X_MAX_NAND_DEVICES 1

struct nand_regs{
	volatile u32  cfg_module;
	volatile u32  sel;
	volatile u32  timing_sequence;
	volatile u32  none;
	volatile u32  cfg_ecc; //changed
	volatile u32  ctl_receive;
	volatile u32  status_reg1;
	volatile u32  status_reg2;
	volatile u32  cmd_reg;
	volatile u32  addr_reg;
	volatile u32  wrdata_fifo;
	volatile u32  rddata_fifo;
	volatile u32  ecc_status;
	volatile u32  cfg_dma_trans_startaddr;//new add	
	volatile u32  cfg_dma_ctrl;	//changed
	volatile u32  cfg_dma_waittime;//new add
};


struct nand_ecc_config {

	u32 data_len;
	u8 type;
	u8 start_sector;
	u8 codes_per_sector;
	u8 load;
	u8 decode;
	u8 start_new_ecc;
};


struct gx_nand_ecc_regs{
	volatile u32 ecc_code[4][14];
};
struct gx_nand_wrong_location_regs{
	volatile u32 ecc_wrong_location[4][16];
};

struct gx320x_nand_info;

struct gx320x_nand_mtd {
	struct mtd_info			*mtd;
	struct nand_chip		chip;
	struct gx320x_nand_info		*info;
	int				scan_res;
};

/* overview of the gx32xx nand state */
struct gx320x_nand_info {
	/* mtd info */
	struct nand_hw_control		controller;
	struct gx320x_nand_mtd		*mtds;

	struct mtd_partition		*part;
	unsigned int			npart;
	/* device info */
	unsigned int			nand_base;
	struct device			*device;
	struct resource			*area;
	struct nand_regs        __iomem *regs;
	struct gx_nand_ecc_regs __iomem *ecc_regs;
	struct gx_nand_wrong_location_regs __iomem *wrong_location_regs;
	void __iomem			*sel_reg;
	int				sel_bit;
	int				mtd_count;
	unsigned long			save_nfconf;
};

#endif
