/*
 * Copyright (c) 2008 Nationalchip Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 *
 * History:
 *      18-Nov-2008 create this file
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
#include <linux/of_platform.h>
#endif
#include <linux/platform_device.h>

#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

#include "gxavdev_module.h"
#include "kernelcalls.h"

MODULE_AUTHOR("zhuzhg");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("support for NationalChilp AV modules");
MODULE_SUPPORTED_DEVICE("gx3110");

extern int gxav_init(void);
extern void gxav_uninit(void);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
extern long gxav_io_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#else
extern int gxav_io_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#endif
extern int gxav_mmap(struct file *filp, struct vm_area_struct *vma);
extern int gxav_open(struct inode *inode, struct file *filp);
extern int gxav_close(struct inode *inode, struct file *filp);

static const struct file_operations gxav_fops = {
	.owner = THIS_MODULE,

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	.unlocked_ioctl = gxav_io_ioctl,
#else
	.ioctl = gxav_io_ioctl,
#endif
	.mmap = gxav_mmap,
	.open = gxav_open,
	.release = gxav_close,
};

static struct cdev cdev;
static struct class *gxav_class;
static int device_count = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
#	define CLASS_DEV_CREATE(class, devt, device, id)      device_create(class, devt, device, NULL, "gxav%d", id)
#	define CLASS_DEV_DESTROY(class, devt)                 device_destroy(class, devt)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#	define CLASS_DEV_CREATE(class, devt, device, id)      device_create(class, devt, device, NULL, "gxav%d", id)
#	define CLASS_DEV_DESTROY(class, devt)                 device_destroy(class, devt)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
#	define CLASS_DEV_CREATE(class, devt, device, id)      class_device_create(class, devt, device, NULL, "gxav%d", id)
#	define CLASS_DEV_DESTROY(class, devt)                 class_device_destroy(class, devt)
#else
#	define CLASS_DEV_CREATE(class, devt, device, id)      device_create_drvdata(class, devt, device, "gxav%d", id)
#	define CLASS_DEV_DESTROY(classs, devt)                device_destroy(class, devt)
#endif

/*
 * Surface count implements reading /proc/gxav/surface.
 */

// TODO
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,25)
static void gxav_create_proc_entries(void)
{
}
#else
static struct proc_dir_entry *gxav_proc_directory = NULL;
static int gxav_proc_read_status(char *Page, char **Start, off_t Offset,
		int Count, int *EOF, void *Data)
{
	*Start = Page;
	gx_sprintf(Page, "surface count: %d\n", 0);

	return strlen(Page);
}

static void gxav_create_proc_entries(void)
{
	struct proc_dir_entry *StatusProcEntry;
	if (gxav_proc_directory == NULL) {
		gxav_proc_directory = proc_mkdir("gxav", NULL);
		StatusProcEntry = create_proc_read_entry("surface", 0,
				gxav_proc_directory,
				gxav_proc_read_status, NULL);
	}
}
#endif

static int gxav_module_init(void)
{
	dev_t dev_id;
	int ret = -1, i;

	device_count = gxav_init();

	if (device_count < 0)
		return device_count;

	dev_id = MKDEV(GXAV_MAJOR, 0);
	if ((ret = register_chrdev_region(dev_id, device_count, GXAV_DEVICE_NAME)) != 0) {
		printk(KERN_ERR "gxav-core: unable to get major %d\n", GXAV_MAJOR);
		gxav_uninit();

		return ret;
	}

	cdev_init(&cdev, &gxav_fops);
	if ((ret = cdev_add(&cdev, dev_id, device_count)) != 0) {
		printk(KERN_ERR "gxav-core: unable register character device\n");
		goto error;
	}

	gxav_class = class_create(THIS_MODULE, GXAV_CLASS_NAME);
	if (IS_ERR(gxav_class)) {
		ret = PTR_ERR(gxav_class);
		goto error;
	}

	for (i = 0; i < device_count; i++) {
		CLASS_DEV_CREATE(gxav_class, NULL, MKDEV(GXAV_MAJOR, i), i);
	}
	gxav_create_proc_entries();

	printk(KERN_EMERG "%s: ok! \n", __FUNCTION__);
	return 0;

error:
	cdev_del(&cdev);
	unregister_chrdev_region(dev_id, device_count);
	gxav_uninit();

	return ret;
}

static void gxav_module_exit(void)
{
	int i;

	printk(KERN_EMERG "%s: av_devices.count = %d\n", __FUNCTION__, device_count);

	for (i = 0; i < device_count; i++) {
		CLASS_DEV_DESTROY(gxav_class, MKDEV(GXAV_MAJOR, i));
	}

	gxav_uninit();
	class_destroy(gxav_class);
	cdev_del(&cdev);
	unregister_chrdev_region(MKDEV(GXAV_MAJOR, 0), device_count);

	printk(KERN_EMERG "%s: ok! \n", __FUNCTION__);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
extern int gxav_tee_probe(struct platform_device *pdev);
extern int gxav_tee_remove(struct platform_device *pdev);
static int gxav_probe(struct platform_device *pdev)
{
	gxav_tee_probe(pdev);
	return gxav_module_init();
}

static int gxav_remove(struct platform_device *pdev)
{
	gxav_tee_remove(pdev);
	gxav_module_exit();
	return 0;
}

static const struct of_device_id gxav_match[] = {
	{ .compatible = "arm,gxavdev" },
	{},
};

static struct platform_driver gxav_driver = {
	.driver = {
		.name = "gxav",
		.of_match_table = gxav_match,
	},
	.probe = gxav_probe,
	.remove = gxav_remove,
};
#endif

static int __init gxav_driver_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	struct device_node *node;

	/*
	 * Preferred path is /soc/gxav, but it's the matching that
	 * matters.
	 */
	for_each_matching_node(node, gxav_match)
		of_platform_device_create(node, NULL, NULL);

	return platform_driver_register(&gxav_driver);
#else
	return gxav_module_init();
#endif
}
module_init(gxav_driver_init);

static void __exit gxav_driver_exit(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
	platform_driver_unregister(&gxav_driver);
#else
	gxav_module_exit();
#endif
}
module_exit(gxav_driver_exit);

MODULE_AUTHOR("Nationalchip");
MODULE_DESCRIPTION("gxavdev driver");
MODULE_SUPPORTED_DEVICE("");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");

