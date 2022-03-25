#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <asm/io.h>

#include "gx_tfm_core.h"
#include "gxm2m.h"

static uint8_t *mtc_src_addr = NULL;
static uint8_t *mtc_dst_addr = NULL;

#ifdef CONFIG_CSKY
int mtc_decrypt_soft(uint8_t *src,uint8_t *dst,int size)
{
	GxTfmCrypto mtc_data;
#define KEY_MAX_SIZE 16
	uint8_t ckey[KEY_MAX_SIZE] = {
		0xc1,0x52,0x96,0xd9,0xab,0xdc,0xd3,0xf5,
		0x26,0x88,0x5f,0x86,0x51,0xbc,0x4a,0xbc}; 
	int ret = 0;
	
	if(size > PAGE_SIZE) {
		printk(KERN_ERR"mtc decrypt size %d too large!\n",size);
		return -EINVAL;
	}
	
	memcpy(mtc_src_addr,src,size);
	
	memset(&mtc_data,0,sizeof(GxTfmCrypto));

	mtc_data.module = TFM_TYPE_M2M;
	mtc_data.alg    = MTC_ALG_AES128;
	mtc_data.opt   = MTC_OPT_ECB;
	
	mtc_data.key   = MTC_KEY_ACPU_SOFT;
	memcpy(&mtc_data.soft_key[0], ckey, 16);

	mtc_data.cipher        = (unsigned int *)mtc_src_addr;
	mtc_data.cipher_len    = size;
	mtc_data.output        =  (unsigned int *)mtc_dst_addr;
	mtc_data.output_len    = size;

	ret = gx_tfm_decrypt(&mtc_data);
	if(ret != 0) {
		printk(KERN_ERR"mtc decrypt failed\n");
		memcpy(dst,mtc_src_addr,size);
		return -1;
	}
	
	memcpy(dst,mtc_dst_addr,size);

	return 0;
}
EXPORT_SYMBOL(mtc_decrypt_soft);

int mtc_decrypt_otp(uint8_t *src,uint8_t *dst,int size)
{
	GxTfmCrypto mtc_data;
	int ret = 0;
	
	if(size > PAGE_SIZE) {
		printk(KERN_ERR"mtc decrypt size %d too large!\n",size);
		return -EINVAL;
	}
	
	memcpy(mtc_src_addr,src,size);
	
	memset(&mtc_data,0,sizeof(GxTfmCrypto));

	mtc_data.module = TFM_TYPE_M2M;
	mtc_data.alg    = MTC_ALG_AES128;
	mtc_data.opt   = MTC_OPT_ECB;
	mtc_data.key   = MTC_KEY_OTP_SOFT;

	mtc_data.cipher        = (unsigned int *)mtc_src_addr;
	mtc_data.cipher_len    = size;
	mtc_data.output        = (unsigned int *)mtc_dst_addr;
	mtc_data.output_len    = size;

	ret = gx_tfm_decrypt(&mtc_data);
	if(ret != 0) {
		printk(KERN_ERR"mtc decrypt failed\n");
		memcpy(dst,mtc_src_addr,size);
		return -1;
	}	
	
	memcpy(dst,mtc_dst_addr,size);

	return 0;
}
EXPORT_SYMBOL(mtc_decrypt_otp);
#endif

#ifdef CONFIG_ARM
int mtc_decrypt_soft(uint8_t *src,uint8_t *dst,int size)
{
	GxTfmCrypto mtc_data;
#define KEY_MAX_SIZE 32
	uint8_t ckey[KEY_MAX_SIZE] = {
		0xc1,0x52,0x96,0xd9,0xab,0xdc,0xd3,0xf5,
		0x26,0x88,0x5f,0x86,0x51,0xbc,0x4a,0xbc}; 
	int ret = 0;
	
	if(size > PAGE_SIZE) {
		printk(KERN_ERR"mtc decrypt size %d too large!\n",size);
		return -EINVAL;
	}
	
	memcpy(mtc_src_addr,src,size);
	
	memset(&mtc_data,0,sizeof(GxTfmCrypto));

	mtc_data.module = TFM_TYPE_M2M;
	mtc_data.alg    = M2M_ALG_AES_256;
	mtc_data.opt   = M2M_OPT_ECB;

	mtc_data.key   = M2M_KEY_ACPU_SOFT;
	mtc_data.key_opt  = M2M_KEY_OPT_EVEN;

	memcpy(&mtc_data.soft_key[0], ckey, KEY_MAX_SIZE);

	mtc_data.cipher        = (unsigned int *)mtc_src_addr;
	mtc_data.cipher_len    = size;
	mtc_data.output        =  (unsigned int *)mtc_dst_addr;
	mtc_data.output_len    = size;

	ret = gx_tfm_decrypt(&mtc_data);
	if(ret < 0) {
		printk(KERN_ERR"mtc decrypt failed\n");
		memcpy(dst,mtc_src_addr,size);
		return -1;
	}
	
	memcpy(dst,mtc_dst_addr,size);

	return 0;
}
EXPORT_SYMBOL(mtc_decrypt_soft);

int mtc_decrypt_otp(uint8_t *src,uint8_t *dst,int size)
{
	GxTfmCrypto mtc_data;
	int ret = 0;
	
	if(size > PAGE_SIZE) {
		printk(KERN_ERR"mtc decrypt size %d too large!\n",size);
		return -EINVAL;
	}
	
	memcpy(mtc_src_addr,src,size);
	
	memset(&mtc_data,0,sizeof(GxTfmCrypto));

	mtc_data.module = TFM_TYPE_M2M;
	mtc_data.alg    = M2M_ALG_AES_256;
	mtc_data.opt   = M2M_OPT_ECB;

	mtc_data.key   = M2M_KEY_OTP_SOFT;
	mtc_data.key_opt  = M2M_KEY_OPT_EVEN;

	mtc_data.cipher        = (unsigned int *)mtc_src_addr;
	mtc_data.cipher_len    = size;
	mtc_data.output        = (unsigned int *)mtc_dst_addr;
	mtc_data.output_len    = size;

	ret = gx_tfm_decrypt(&mtc_data);
	if(ret < 0) {
		printk(KERN_ERR"mtc decrypt failed\n");
		memcpy(dst,mtc_src_addr,size);
		return -1;
	}	
	
	memcpy(dst,mtc_dst_addr,size);

	return 0;
}
EXPORT_SYMBOL(mtc_decrypt_otp);
#endif

static int __init gx_mtc_init(void)
{
	printk("gx_mtc technologies\n");

	mtc_src_addr = (unsigned char *)__get_free_page(GFP_KERNEL);
	if(mtc_src_addr == NULL)
		return -EINVAL;

	mtc_dst_addr = (unsigned char *)__get_free_page(GFP_KERNEL);
	if(mtc_dst_addr == NULL) {
		free_page((unsigned long) mtc_src_addr);
		return -EINVAL;
	}

	printk("mtc_src_addr %p mtc_dst_addr %p\n",mtc_src_addr,mtc_dst_addr);
	return 0;
}

static void __exit gx_mtc_exit(void)
{
	free_page((unsigned long) mtc_src_addr);
	free_page((unsigned long) mtc_dst_addr);
}

arch_initcall(gx_mtc_init);
module_exit(gx_mtc_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gx chip mtc");

