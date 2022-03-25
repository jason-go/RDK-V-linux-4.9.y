#include <linux/module.h>

static int __init nc_cpu_init(void)
{
	unsigned int tmp;

	/*
	 ** 获取 cr18
	 *		 */
	asm volatile(
			"mfcr %0, cr18\n"
			:"=r"(tmp));

	/* 
	 ** 通过 cr18 判断Dcache 是否打开
	 ** 如果 Dcache 打开 那么INV + CLR缓存
	 ** 如果 Dcache 关闭 那么仅INV缓存
	 */
	if (tmp&8)
		asm volatile(
				"movi a0, 0x33\n"
				"mtcr a0, cr17\n"
				:::"a0");
	else
		asm volatile(
				"movi a0, 0x23\n"
				"mtcr a0, cr17\n"
				:::"a0");

	/*
	 ** 开启 ck610 的所有特性
	 *		 */
	asm volatile(
			"movi a0, 0x1c\n"
			"mtcr a0, cr30\n"

			"movi a0, 0x7d\n"
			"mtcr a0, cr18\n"
			:::"a0");
	return 0;
}

early_initcall(nc_cpu_init);
