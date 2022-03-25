/*
 * gxcore-av/src/driver-linux/gxavdev.c
 *
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
 *      28-5-2009   zhuzhg: fix for kernel > 2.6.18
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

#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/io.h>

#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

#include "gxavdev.h"
#include "avcore.h"
#include "kernelcalls.h"
#include "firewall.h"
#include "osdep-dma.h"

MODULE_AUTHOR("XXXxxx.XXX");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("support for NationalChilp AV modules");
MODULE_SUPPORTED_DEVICE("gx3110");

static struct gxav av_devices;

static DEFINE_MUTEX(av_mutex);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,25)
#define IRQF_DISABLED IRQF_SHARED
long gxav_io_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = (struct inode *)(file->f_path.dentry->d_inode);
	int minor = iminor(inode);
	struct gxav_priv_dev *dev = &av_devices.entry[minor];

	if (dev == NULL)
		return -EAGAIN;

	return gxav_ioctl(dev, cmd, arg);
}

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)
long gxav_io_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct inode *inode = file->f_dentry->d_inode;
	int minor = iminor(inode);
	struct gxav_priv_dev *dev = &av_devices.entry[minor];

	if (dev == NULL)
		return -EAGAIN;

	return gxav_ioctl(dev, cmd, arg);
}
#else
int gxav_io_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int minor = iminor(inode);
	struct gxav_priv_dev *dev = NULL;

	dev = &av_devices.entry[minor];

	if (dev == NULL)
		return -EAGAIN;

	return gxav_ioctl(dev, cmd, arg);
}
#endif

int gxav_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned int buf = (unsigned int)(vma->vm_pgoff << PAGE_SHIFT);

	if (gxav_firewall_access_align() == 0) {
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); /* disable cache */
	}

	if(remap_pfn_range(vma, vma->vm_start, gx_virt_to_phys(buf) >> PAGE_SHIFT, vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

#ifdef GXAV_ENABLE_DTB
int gxav_irq_num[GXAV_MAX_IRQ];
int gxav_irq_src[GXAV_MAX_IRQ];
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
static irqreturn_t quasar_interrupt(int irq, void *dev_t)
#else
static irqreturn_t quasar_interrupt(int irq, void *dev_t, struct pt_regs *regs)
#endif
{
	struct gxav_priv_dev *current_dev = dev_t;
	struct gxav_module_inode *inode;

#ifdef GXAV_ENABLE_DTB
	irq = gxav_irq_src[irq];
//	gx_printf("%s(),irq=%d\n",__func__,irq);
#endif

	inode = gxav_device_interrupt_entry(current_dev->av_dev, irq, GXAV_ISR_ENTRY);
	if (inode == NULL) {
		GXAV_DBG("In %s inode is NULL !!!  irq = %d \n", __func__, irq);
		gx_interrupt_unmask(irq);
		return IRQ_HANDLED;
	}

	if(inode->event_status && inode->wq) {
		wake_up_interruptible_sync((wait_queue_head_t *)inode->wq);
		GXAV_DBG("%s(),inode->event_status = 0x%x \n",__func__, inode->event_status);
	}

	GXAV_DBG("%s(),irq=%d\n",__func__,irq);
	return IRQ_HANDLED;
}

static int device_open(int minor)
{
	int i, ret;
	struct gxav_priv_dev *current_dev = NULL;
#ifdef GXAV_ENABLE_DTB
	struct device_node *dn = of_find_node_by_path("/soc/gxavdev");
#endif
	mutex_lock(&av_mutex);

	current_dev = &av_devices.entry[minor];
	if (current_dev == NULL || current_dev->av_dev == NULL)  {
		printk(KERN_EMERG "av_dev NULL \n");
		mutex_unlock(&av_mutex);
		return -EINVAL;
	}

	gxav_device_open(current_dev->av_dev);

	if(current_dev->av_dev->refcount == 1){
		current_dev->av_dev->modules_handle[current_dev->av_dev->sdc_module_id]->module->inode->wq = &current_dev->wq;
		for (i = 0; i < GXAV_MAX_IRQ; i++) {
			if (current_dev->av_dev->irq_list[i].irq_count != 0 &&
					current_dev->av_dev->irq_list[i].irq_entry[0].inode != NULL) {
#ifdef GXAV_ENABLE_DTB
				if (dn == NULL) {
					printk(KERN_EMERG "gxavdev node == NULL\n");
					return -EINVAL;
				}
				gxav_irq_num[i] = of_irq_get_byname(dn, current_dev->av_dev->irq_list[i].name);
				if (gxav_irq_num[i] < 0) {
					printk(KERN_EMERG "%s's irq not found !\n", current_dev->av_dev->irq_list[i].name);
					continue;
				}
				gxav_irq_src[gxav_irq_num[i]] = i;
				ret = request_irq(gxav_irq_num[i], quasar_interrupt, IRQF_DISABLED, INTERRUPT_ENTRY_NAME, current_dev);
				printk("request_irq :%d -> %d\n", i, gxav_irq_num[i]);
#else
				ret = request_irq(i, quasar_interrupt, IRQF_DISABLED, INTERRUPT_ENTRY_NAME, current_dev);
#endif
				if (ret != 0) {
					printk(KERN_EMERG "request_irq fail : %d\n", ret);
					while(i) {
						if(current_dev->av_dev->irq_list[i].irq_entry) {
							struct gxav_module_inode* inode = current_dev->av_dev->irq_list[i].irq_entry[0].inode;
							if (inode != NULL){
								wake_up_interruptible_sync((wait_queue_head_t *)inode->wq);
#ifdef GXAV_ENABLE_DTB
								free_irq(gxav_irq_num[i], current_dev);
#else
								free_irq(i, current_dev);
#endif
							}
						}
						i--;
					}

					mutex_unlock(&av_mutex);
					return ret;
				}
			}
		}
	}

	mutex_unlock(&av_mutex);

	return 0;
}

static int device_close(int minor)
{
	int i;
	struct gxav_priv_dev *current_dev = NULL;

	mutex_lock(&av_mutex);

	current_dev = &av_devices.entry[minor];
	if (current_dev == NULL || current_dev->av_dev == NULL)  {
		printk(KERN_EMERG "av_dev NULL \n");
		mutex_unlock(&av_mutex);
		return -EINVAL;
	}

	if(current_dev->av_dev->refcount == 1) {
		for (i = 0; i < GXAV_MAX_IRQ; i++) {
			if (current_dev->av_dev->irq_list[i].irq_count != 0 &&
					current_dev->av_dev->irq_list[i].irq_entry[0].inode != NULL) {
#ifdef GXAV_ENABLE_DTB
				free_irq(gxav_irq_num[i], current_dev);
#else
				free_irq(i, current_dev);
#endif
			}
		}
	}

	gxav_device_release_modules(current_dev->av_dev);
	gxav_device_close(current_dev->av_dev);

	mutex_unlock(&av_mutex);
	return 0;
}

int gxav_open(struct inode *inode, struct file *filp)
{
	const int minor = iminor(inode);

	return device_open(minor);
}

int gxav_close(struct inode *inode, struct file *filp)
{
	const int minor = iminor(inode);
	int ret;

	ret = device_close(minor);

	return ret;
}

extern void gxav_memhole_init(void);
extern void gxav_memhole_cleanup(void);
extern void gxav_chip_firmware_register(void);

int gxav_init(void)
{
	struct gxav_device *av_dev, *av_dev_head;

	gxav_dmainfo_init();
	
	gxav_memhole_init();

	gxav_chip_firmware_register();

	gx_memset(&av_devices, 0, sizeof(av_devices));

	av_dev_head = gxav_devices_setup(NULL);
	if (NULL == av_dev_head) {
		printk(KERN_ERR "probe dev failed\n");
		return -EINVAL;
	}

	av_dev = av_dev_head;

	do {
		mutex_init(&av_devices.entry[av_devices.count].lock);
		av_devices.entry[av_devices.count].av_dev = av_dev;
		init_waitqueue_head(&av_devices.entry[av_devices.count].wq);

		av_devices.count++;

		av_dev = av_dev->next;
	} while (av_dev != av_dev_head && av_devices.count < MAX_DEV);

	return av_devices.count;
}

void gxav_uninit(void)
{
	gxav_devices_cleanup();

	gxav_memhole_cleanup();
}

struct gxav_priv_dev *gxav_dev_open(int minor)
{
	int ret = device_open(minor);

	return (ret ==0 ? &(av_devices.entry[minor]): NULL);
}

int gxav_dev_close(int minor)
{
	return device_close(minor);
}

EXPORT_SYMBOL(gxav_ioctl);
EXPORT_SYMBOL(gxav_dev_open);
EXPORT_SYMBOL(gxav_dev_close);

