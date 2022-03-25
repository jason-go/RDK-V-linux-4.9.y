#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/firmware.h>

extern int gx_irr_probe(struct platform_device *pdev);
extern int gx_irr_remove(struct platform_device *pdev);

static struct of_device_id gxirr_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-pt2264",
		.data = NULL,
	},
};

static struct platform_driver gx_pt2264_driver = {
	.probe = gx_irr_probe,
	.remove = gx_irr_remove,
	.driver = {
		.name = "gx_pt2264",
		.of_match_table = gxirr_device_match,
	},
};

module_platform_driver(gx_pt2264_driver);

MODULE_DESCRIPTION("support for NationalChilp irr modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
