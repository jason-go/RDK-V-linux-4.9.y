#ifndef _GXAVDEV_H_
#define _GXAVDEV_H_

#include <linux/list.h>
#include <linux/kfifo.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>

#include "kernelcalls.h"
#include "avcore.h"
#include "gxavdev_module.h"

#define INTERRUPT_ENTRY_NAME    "quasar"

#define REMAP_PIO               0x1000

#define MAX_DEV                 8
#define MAX_PROPSIZE            1024

struct gxav_priv_dev {
	struct gxav_device* av_dev;
	wait_queue_head_t wq;
	struct mutex lock;
	struct mutex module_lock[GXAV_MAX_MODULE];
	char module_buffer[GXAV_MAX_MODULE][MAX_PROPSIZE];
};

struct gxav {
	int                     count;
	struct gxav_priv_dev    entry[MAX_DEV];
};

int gxav_mod_open(struct gxav_priv_dev *dev, GxAvModuleType module_type, int sub);
int gxav_mod_close(struct gxav_priv_dev *dev, int module_id);
struct gxav_priv_dev *gxav_dev_open(int minor);
int gxav_dev_close(int minor);
int gxav_ioctl(struct gxav_priv_dev *card, unsigned int cmd, unsigned long arg);

#endif

