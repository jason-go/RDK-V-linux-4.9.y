#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/io.h>
#include <asm/irq.h>
#ifndef CONFIG_CSKY
#include <asm/mach/map.h>
#endif

#define GX_CHIP_ID_NAME                ("gx_chip_id")
#define GX_CHIP_SUB_ID_NAME                ("gx_chip_sub_id")

#define GXOS_ID_GX3211                (0x3211)
#define GXOS_ID_GX6605S               (0x6605)
#define GXOS_ID_SIRIUS                (0x6612)
#define GXOS_ID_TAURUS                (0x6616)
#define GXOS_ID_GEMINI                (0x6701)

#define GXCHIP_ID_CONFIG_BASE_3211    (0x0030a184)
#define GXCHIP_ID_CONFIG_BASE_SIRIUS  (0x89400184)
#define GXCHIP_ID_SUB_CONFIG_BASE_6605S    (0x00f8000c)

#define GXCHIP_ID_CONFIG_BASE_LEN     (4)

#define HAL_READ_UINT32( _register_, _value_ ) \
	((_value_) = *((volatile unsigned int *)(_register_)))

unsigned int sec_read(unsigned int addr, unsigned int delay)
{
#ifdef CONFIG_GX6605S
	unsigned int rdata;
	__asm__ __volatile("  \
			sync                \n\
			br      2f          \n\
			1:                  \n\
			subi    %2, %2, 1   \n\
			2:                  \n\
			cmpnei  %2, 0       \n\
			bt      1b          \n\
			sync                \n\
			ld      %0, (%1, 0) \n\
			sync                \n\
			":"=r"(rdata)
			:"r"(addr),"r"(delay)
			:
			);
	return(rdata);
#endif
	return 0;
}

static unsigned int gx_chipid = 0;
static unsigned int gx_chipsubid = 0xFFFFFFFF;

static int chipid_from_cmdline = 0;
unsigned int gx_chip_id_sub_probe(void);
static int __init chipid_setup(char *ptr)
{
	get_option(&ptr, &chipid_from_cmdline);
	return 0;
}
__setup("chipid=", chipid_setup);

static char *mem_request(unsigned int mem_base, unsigned int mem_len, char *name)
{
	char *mapped_addr = NULL;

	if (!request_mem_region(mem_base, mem_len, name)) {
		printk(KERN_ERR "mem_request :: request_mem_region failed");
		return NULL;
	}

	mapped_addr = ioremap(mem_base, mem_len);
	if (!mapped_addr) {
		release_mem_region(mem_base, mem_len);
		printk(KERN_ERR "ioremap failed.\n");
		return NULL;
	}

	return mapped_addr;
}

static int mem_free(char *mapped_addr, unsigned int mem_base, unsigned int mem_len)
{
	iounmap(mapped_addr);
	release_mem_region(mem_base, mem_len);
	return 0;
}

static unsigned int gx3211_probe(void)
{
	unsigned char *mapped_addr = NULL;
	unsigned int chip_id = 0;
	unsigned int chip_reg = 0;

	mapped_addr = mem_request(GXCHIP_ID_CONFIG_BASE_3211, GXCHIP_ID_CONFIG_BASE_LEN, GX_CHIP_ID_NAME);
	if (!mapped_addr)
		return 0;

	HAL_READ_UINT32((unsigned int *)(mapped_addr), chip_reg);
	chip_id = ((chip_reg >> 14) & 0xFFFF);
	mem_free(mapped_addr, GXCHIP_ID_CONFIG_BASE_3211, GXCHIP_ID_CONFIG_BASE_LEN);

	if (chip_id != GXOS_ID_GX3211)
		return 0;
	else
		return GXOS_ID_GX3211;
}

static unsigned int taurus_probe(void)
{
	unsigned char *mapped_addr = NULL;
	unsigned int chip_id = 0;
	unsigned int chip_reg = 0;

	mapped_addr = mem_request(GXCHIP_ID_CONFIG_BASE_3211, GXCHIP_ID_CONFIG_BASE_LEN, GX_CHIP_ID_NAME);
	if (!mapped_addr)
		return 0;

	HAL_READ_UINT32((unsigned int *)(mapped_addr), chip_reg);
	chip_id = ((chip_reg >> 14) & 0xFFFF);
	mem_free(mapped_addr, GXCHIP_ID_CONFIG_BASE_3211, GXCHIP_ID_CONFIG_BASE_LEN);

	if (chip_id != GXOS_ID_TAURUS)
		return 0;
	else
		return GXOS_ID_TAURUS;
}

static unsigned int gemini_probe(void)
{
	unsigned char *mapped_addr = NULL;
	unsigned int chip_id = 0;
	unsigned int chip_reg = 0;

	mapped_addr = mem_request(GXCHIP_ID_CONFIG_BASE_3211, GXCHIP_ID_CONFIG_BASE_LEN, GX_CHIP_ID_NAME);
	if (!mapped_addr)
		return 0;

	HAL_READ_UINT32((unsigned int *)(mapped_addr), chip_reg);
	chip_id = ((chip_reg >> 14) & 0xFFFF);
	mem_free(mapped_addr, GXCHIP_ID_CONFIG_BASE_3211, GXCHIP_ID_CONFIG_BASE_LEN);

	if (chip_id != GXOS_ID_GEMINI)
		return 0;
	else
		return GXOS_ID_GEMINI;
}

static unsigned int sirius_probe(void)
{
	unsigned char *mapped_addr = NULL;
	unsigned int chip_id = 0;
	unsigned int chip_reg = 0;

	mapped_addr = mem_request(GXCHIP_ID_CONFIG_BASE_SIRIUS, GXCHIP_ID_CONFIG_BASE_LEN, GX_CHIP_ID_NAME);
	if (!mapped_addr)
		return 0;

	HAL_READ_UINT32((unsigned int *)(mapped_addr), chip_reg);
	chip_id = ((chip_reg >> 14) & 0xFFFF);
	mem_free(mapped_addr, GXCHIP_ID_CONFIG_BASE_SIRIUS, GXCHIP_ID_CONFIG_BASE_LEN);

	if (chip_id != GXOS_ID_SIRIUS)
		return 0;
	else
		return GXOS_ID_SIRIUS;
}

static unsigned int gx6605s_sub_probe(void)
{
	unsigned char *mapped_addr = NULL;
	unsigned int chip_sub = 0;
	unsigned int chip_sub_reg = 0;

	mapped_addr = mem_request(GXCHIP_ID_SUB_CONFIG_BASE_6605S, GXCHIP_ID_CONFIG_BASE_LEN, GX_CHIP_SUB_ID_NAME);
	if (!mapped_addr)
		return 0;

	chip_sub_reg = sec_read((unsigned int)mapped_addr, 65536);
	chip_sub = ((chip_sub_reg >> 11) & 0x7);
	mem_free(mapped_addr, GXCHIP_ID_SUB_CONFIG_BASE_6605S, GXCHIP_ID_CONFIG_BASE_LEN);

	if (chip_sub)
		return 1;
	else
		return 0;
}

static unsigned int gx6605s_probe(void)
{
	/*get chipid from cmdline*/
	if(chipid_from_cmdline == GXOS_ID_GX6605S)
		return GXOS_ID_GX6605S;
	else
		return 0;
}

unsigned int (*gxchip_probe[])(void) = {
	gemini_probe,
	taurus_probe,
	sirius_probe,
	gx3211_probe,
	gx6605s_probe,
	NULL,
};

void check_chipid(void)
{
	int i;

#ifdef CONFIG_GEMINI
	gx_chipid = GXOS_ID_GEMINI;
#endif
#ifdef CONFIG_TAURUS
	gx_chipid = GXOS_ID_TAURUS;
#endif
#ifdef CONFIG_SIRIUS
	gx_chipid = GXOS_ID_SIRIUS;
#endif
#ifdef CONFIG_GX3211
	gx_chipid = GXOS_ID_GX3211;
#endif
#ifdef CONFIG_GX6605S
	gx_chipid = GXOS_ID_GX6605S;
#endif

	for(i = 0; gxchip_probe[i] != NULL; i++) {
		gx_chipid = (unsigned int)(*gxchip_probe[i])();
		if(gx_chipid != 0) break;
	}
}

static int gxchipid_proc_add(char *name, struct file_operations *ec_ops, struct proc_dir_entry *dir)
{
	struct proc_dir_entry *entry;

	entry = proc_create(name, S_IRUGO, dir, ec_ops);
	if (!entry)
		return -ENODEV;

	return 0;
}

static int gx_chip_id_proc_read(struct seq_file *seq, void *offset)
{
	int sub_type = gx_chip_id_sub_probe();
	seq_printf(seq, "%x%x\n", gx_chipid, sub_type);
	return 0;
}

static int gx_chip_id_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, gx_chip_id_proc_read, NULL);
}

static struct file_operations gx_chip_id_ops = {
	.open = gx_chip_id_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.owner = THIS_MODULE,
};

static int gx_chip_id_get(void)
{
	if (gx_chipid != 0) goto exit;

	check_chipid();
	printk("gxchip_init: %x.\n", gx_chipid);

exit:
	return 0;
}

unsigned int gx_chip_id_probe(void)
{
	int ret = gx_chip_id_get();
	if (ret < 0)
		return ret;

	return gx_chipid;
}
EXPORT_SYMBOL(gx_chip_id_probe);

unsigned int gx_chip_id_sub_probe(void)
{
	int ret = gx_chip_id_get();
	if (ret < 0)
		return ret;

	if (gx_chipsubid != 0xFFFFFFFF)
		return gx_chipsubid;

	if (gx_chipid == GXOS_ID_GX6605S)
		gx_chipsubid = gx6605s_sub_probe();
	else
		gx_chipsubid = 0;

	return gx_chipsubid;
}
EXPORT_SYMBOL(gx_chip_id_sub_probe);

static int gxchipid_init(void)
{
	int ret = gx_chip_id_get();
	if (ret < 0)
		return ret;
	gx_chip_id_sub_probe();
	return gxchipid_proc_add(GX_CHIP_ID_NAME, &gx_chip_id_ops, NULL);
}

static void gxchipid_exit(void)
{
	remove_proc_entry(GX_CHIP_ID_NAME, NULL);
}

arch_initcall(gxchipid_init);
module_exit(gxchipid_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("nationalchip chip id probe");
module_param(gx_chipid, int, S_IRUGO);
