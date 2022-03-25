/*
 * Real Time Clock driver for Marvell 88PM80x PMIC
 *
 * Copyright (c) 2012 Marvell International Ltd.
 *  Wenzeng Chen<wzch@marvell.com>
 *  Qiao Zhou <zhouqiao@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/rtc.h>

struct rtc_regs
{
	unsigned long RTC_CONTROL;
	unsigned long RTC_IER;
	unsigned long RTC_ISR;
	unsigned long RTC_DIV_PRESCALE;
	unsigned long RTC_TICK_COUNT;
	unsigned long RTC_ALARM1_MASK_CONTROL;
	unsigned long RTC_ALARM1_US;
	unsigned long RTC_ALARM1_MS;
	unsigned long RTC_ALARM1_SECOND;
	unsigned long RTC_ALARM1_MINUTE;
	unsigned long RTC_ALARM1_HOUR;
	unsigned long RTC_ALARM1_WEEK;
	unsigned long RTC_ALARM1_DATE;
	unsigned long RTC_ALARM1_MONTH;
	unsigned long RTC_ALARM1_YEAR;
	unsigned long RTC_ALARM2_MASK_CONTROL;
	unsigned long RTC_ALARM2_US;
	unsigned long RTC_ALARM2_MS;
	unsigned long RTC_ALARM2_SECOND;
	unsigned long RTC_ALARM2_MINUTE;
	unsigned long RTC_ALARM2_HOUR;
	unsigned long RTC_ALARM2_WEEK;
	unsigned long RTC_ALARM2_DATE;
	unsigned long RTC_ALARM2_MONTH;
	unsigned long RTC_ALARM2_YEAR; /* maybe unavailable */
	unsigned long RTC_TIME_US;
	unsigned long RTC_TIME_MS;
	unsigned long RTC_TIME_SECOND;
	unsigned long RTC_TIME_MINUTE;
	unsigned long RTC_TIME_HOUR;
	unsigned long RTC_TIME_WEEK;
	unsigned long RTC_TIME_DATE;
	unsigned long RTC_TIME_MONTH;
	unsigned long RTC_TIME_YEAR;
};

struct gxrtc_info {
	unsigned int rtc_base;
	struct resource *area;
	struct rtc_regs __iomem *regs;

	int irq;
	unsigned int sys_clk;
	struct list_head lines;		/* list of lines. */
	spinlock_t spin_lock;
	
	struct rtc_device *rtc_dev;
	struct device *dev;
};

struct gxrtc_info *g_rtc_info = NULL;

static int gxrtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct gxrtc_info *info = dev_get_drvdata(dev);
	volatile struct rtc_regs __iomem *regs = info->regs;
	uint32_t second       = 0;
	uint32_t minute       = 0;
	uint32_t hour         = 0;
	uint32_t date         = 0;
	uint32_t ms           = 0;
	uint32_t ticks = 0;

	ms          	= regs->RTC_TIME_MS;
	second  	= regs->RTC_TIME_SECOND;
	second     = ((second>>4)&7)*10+(second&0xf);
	minute  	= regs->RTC_TIME_MINUTE;
	minute      = ((minute>>4)&7)*10+(minute&0xf);
	hour    	= regs->RTC_TIME_HOUR;
	hour        = ((hour>>4)&3)*10+(hour&0xf);
	date    	= regs->RTC_TIME_DATE;
	date        = ((date>>4)&3)*10+(date&0xf);

	ticks = (((date*24+hour)*60+minute)*60+second)*1000+ms;

	rtc_time_to_tm(ticks, tm);
	return 0;
}

static int gxrtc_set_time(struct device *dev, struct rtc_time *tm)
{
	return 0;
}


static const struct rtc_class_ops gxrtc_ops = {
	.read_time = gxrtc_read_time,
	.set_time = gxrtc_set_time,
};

typedef void (*gxrtc_timer_callback)(unsigned long data);

struct gxrtc_timer {
	struct list_head list;		/* next/prev line lists */
	gxrtc_timer_callback callback;
	unsigned long data; /* argument for callback */
	unsigned long trigger;
	unsigned long interval;
};

static void gxrtc_timer_start(struct gxrtc_info *info)
{
	volatile struct rtc_regs __iomem *regs = info->regs;
	
	/* enable tick */
	regs->RTC_IER |= 1;
	regs->RTC_CONTROL |= 1<<1;
}

static void gxrtc_timer_stop(struct gxrtc_info *info)
{
	volatile struct rtc_regs __iomem *regs = info->regs;
	
	/* disable tick */
	regs->RTC_IER &= ~1;
	regs->RTC_CONTROL &= ~(1<<1);
}

static irqreturn_t gxrtc_irq(int irq, void *data)
{
	struct gxrtc_info *info = (struct gxrtc_info *)data;
	volatile struct rtc_regs __iomem *regs = info->regs;
	struct gxrtc_timer *timer;
	unsigned int timer_done = 0;
	
	/* clear & disable irq */
	regs->RTC_IER &= ~1;
	regs->RTC_ISR &= ~1;

	list_for_each_entry(timer, &info->lines, list) {
		if(timer) {
			if(timer->trigger) {
				--timer->trigger;
				timer_done = 0;
			}
			else {
				if (timer->callback)
					(*timer->callback)(timer->data);

				if (timer->interval) {
					timer->trigger = timer->interval;
					timer_done = 0;
				}
				else {
					timer_done = 1;
				}
			}
		}
	}

	if (timer_done)
		gxrtc_timer_stop(info);

	/* enable irq again */
	regs->RTC_IER |= 1;
	
	return IRQ_HANDLED;
}

int gxrtc_timer_create(gxrtc_timer_callback callback, unsigned long data, unsigned long trigger, unsigned long interval)
{
	struct gxrtc_info *info = (struct gxrtc_info *)g_rtc_info;
	struct gxrtc_timer *timer;
	unsigned long flags;
	
	timer = (struct gxrtc_timer *)kzalloc(sizeof(struct gxrtc_timer),GFP_KERNEL);
	if (!timer)
		return -ENOMEM;

	spin_lock_irqsave(&info->spin_lock, flags);

	/* init the timer */
	timer->callback 	= callback;
	timer->data        	= data;
	timer->trigger     	= trigger;
	timer->interval    	= interval;
		
	if (list_empty(&info->lines))
		gxrtc_timer_start(info);

	list_add_tail(&timer->list, &info->lines);

	spin_unlock_irqrestore(&info->spin_lock, flags);
	
	return (int)timer;
}
EXPORT_SYMBOL(gxrtc_timer_create);

int gxrtc_timer_delete(int timer_id)
{
	struct gxrtc_info *info = (struct gxrtc_info *)g_rtc_info;
	struct gxrtc_timer *i,*tmp;
	struct gxrtc_timer *timer = (struct gxrtc_timer *)timer_id;
	unsigned long flags;
	
	if(!timer)
		return -ENODEV;

	spin_lock_irqsave(&info->spin_lock, flags);
	
	list_for_each_entry_safe(i, tmp, &info->lines, list)
		if (i == timer) {
			list_del(&timer->list);
			kfree(timer);
			break;
		}

	if (list_empty(&info->lines))
		gxrtc_timer_stop(info);

	spin_unlock_irqrestore(&info->spin_lock, flags);
	
	return 0;
}
EXPORT_SYMBOL(gxrtc_timer_delete);

static int gxrtc_remove(struct platform_device *pdev)
{
	struct gxrtc_info *info = platform_get_drvdata(pdev);

	if(!info)
		return -1;

	platform_set_drvdata(pdev, NULL);

	free_irq(info->irq, info);

	devm_rtc_device_unregister(info->dev,info->rtc_dev);

	kfree(info);

	return 0;
}

static void gxrtc_initialize(struct device *dev)
{
	struct gxrtc_info *info = dev_get_drvdata(dev);
	volatile struct rtc_regs __iomem *regs = info->regs;
	uint32_t rtc_pre=270;

	regs->RTC_DIV_PRESCALE = rtc_pre;

	regs->RTC_TIME_DATE	= 0;
	regs->RTC_TIME_HOUR	= 0;
	regs->RTC_TIME_MINUTE	= 0;
	regs->RTC_TIME_MONTH	= 0;
	regs->RTC_TIME_MS		= 0;
	regs->RTC_TIME_SECOND	= 0;
	regs->RTC_TIME_US		= 0;
	regs->RTC_TIME_WEEK	= 0;
	regs->RTC_TIME_YEAR	= 0;

	regs->RTC_CONTROL	   |= 1;

	/* 1ms interval */
	regs->RTC_TICK_COUNT = 100;
}

static int gxrtc_probe(struct platform_device *pdev)
{
	struct platform_device *device = to_platform_device(&pdev->dev);
	struct device_node *rtc_node = pdev->dev.of_node;
	struct gxrtc_info *info;
	int ret;

	info =kzalloc(sizeof(struct gxrtc_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	g_rtc_info = info;
	info->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, info);

	info->irq = platform_get_irq(pdev, 0);
	if (info->irq < 0) {
		dev_err(&pdev->dev, "No IRQ resource!\n");
		ret = -EINVAL;
		goto out;
	}

	info->area = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!info->area) {
		dev_err(&pdev->dev, "no regmap!\n");
		ret = -EINVAL;
		goto out;
	}

	info->rtc_base = info->area->start;
	printk(KERN_INFO"rtc regs base addr: 0x%x\n", info->rtc_base);

	info->regs = devm_ioremap_resource(info->dev, info->area);
	if (!info->regs) {
		dev_err(&pdev->dev, "no ioremap!\n");
		ret = -EINVAL;
		goto out;
	}
	printk(KERN_INFO"rtc regs mapped addr: 0x%p\n", info->regs);

	ret = of_property_read_u32(rtc_node, "system-clock-frequency", &info->sys_clk);
	if (ret) {
		dev_err(&pdev->dev, "no ioremap!\n");
		ret = -EINVAL;
		goto out;
	}

	ret = request_irq(info->irq, gxrtc_irq,IRQF_SHARED, "nc-rtc", info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request IRQ: #%d: %d\n",
				info->irq, ret);
		goto out;
	}

	info->rtc_dev = devm_rtc_device_register(&pdev->dev, "gx-rtc",
			&gxrtc_ops, THIS_MODULE);
	if (IS_ERR(info->rtc_dev)) {
		ret = PTR_ERR(info->rtc_dev);
		dev_err(&pdev->dev, "Failed to register RTC device: %d\n", ret);
		goto out;
	}

	spin_lock_init(&info->spin_lock);
	
	INIT_LIST_HEAD(&info->lines);
	
	gxrtc_initialize(info->dev);
	
	return 0;

out:
	return gxrtc_remove(pdev);
}

static struct of_device_id gxrtc_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-rtc",
		.data = NULL,
	},
};

static struct platform_driver gxrtc_driver = {
	.driver = {
		.name = "gx_rtc",
		.of_match_table = gxrtc_device_match,
	},
	.probe = gxrtc_probe,
	.remove = gxrtc_remove,
};

module_platform_driver(gxrtc_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Marvell GX RTC driver");
MODULE_AUTHOR("Qiao Zhou <zhouqiao@marvell.com>");
MODULE_ALIAS("platform:gx-rtc");
