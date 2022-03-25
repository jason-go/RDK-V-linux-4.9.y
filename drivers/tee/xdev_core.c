/*
 * Copyright (c) 2015-2016, Linaro Limited
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/tee_drv.h>
#include <linux/uaccess.h>
#include "tee_private.h"
#include "optee/optee_msg.h"
#include "optee/optee_smc.h"
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/tee_drv.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#define TEE_PAYLOAD_REF_COUNT 4

static u32 xdev_session = -1;

static int xdev_open_session(struct tee_context *ctx)
{
	int ret = 0;
	struct tee_ioctl_open_session_arg arg;
	struct tee_param param[TEE_PAYLOAD_REF_COUNT];
	unsigned char uuid[16] = { 0x55, 0xbb, 0x55, 0xbb, 0x55, 0xbb, 0x55, 0xbb,
		0x55, 0xbb, 0x55, 0xbb, 0x55, 0xbb, 0x55, 0xbb };

	if (xdev_session == -1) {
		memset(&arg, 0, sizeof(arg));
		memset(param, 0, sizeof(param));
		memcpy(arg.uuid, uuid, 16);

		ret = ctx->teedev->desc->ops->open_session(ctx, &arg, param);
		if (ret == 0) {
			xdev_session = (u32)arg.session;
		}
	}

	return ret;
}

static int tee_close_session(struct tee_context *ctx, u32 session)
{
	return ctx->teedev->desc->ops->close_session(ctx, session);
}

unsigned long gx_xdev_open(struct file *filp, void *path)
{
	int ret = 0;
	struct tee_ioctl_invoke_arg arg;
	struct tee_context *ctx = filp->private_data;
	struct tee_param params[TEE_PAYLOAD_REF_COUNT];
	size_t size = strlen(path) + 1;

	memset(&arg, 0, sizeof(arg));
	memset(params, 0, sizeof(params));

	xdev_open_session(ctx);

	arg.func = 1; //TA_XDEV_OPEN
	arg.session = xdev_session;
	arg.num_params = TEE_PAYLOAD_REF_COUNT;
	params[0].attr = OPTEE_MSG_ATTR_TYPE_RMEM_INPUT;
	params[0].u.memref.size = size;
	params[0].u.memref.shm = tee_shm_alloc(ctx, size, TEE_SHM_MAPPED);
	params[1].attr = OPTEE_MSG_ATTR_TYPE_VALUE_OUTPUT;
	params[1].u.value.a = -1;
	if (params[0].u.memref.shm != NULL) {
		memcpy(tee_shm_get_va(params[0].u.memref.shm, 0), path, size);
		ret = ctx->teedev->desc->ops->invoke_func(ctx, &arg, params);
		tee_shm_free(params[0].u.memref.shm);
		if (ret == 0) {
			printk("%s. xdev_handle = %d !\n", __func__, (int)params[1].u.value.a);
			return (unsigned long)params[1].u.value.a;
		}
		printk("%s. xdev_handle failed ! ret = %x\n", __func__, ret);
		return -1;
	}

	printk("%s. malloc NULL !\n", __func__);
	return -1;
}

int gx_xdev_close(struct file *filp, unsigned long h)
{
	struct tee_context *ctx = filp->private_data;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param params[TEE_PAYLOAD_REF_COUNT];

	memset(&arg, 0, sizeof(arg));
	memset(params, 0, sizeof(params));

	arg.func = 2; // TA_XDEV_CLOSE
	arg.session = xdev_session;
	arg.num_params = TEE_PAYLOAD_REF_COUNT;
	params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
	params[0].u.value.a = h;

	ctx->teedev->desc->ops->invoke_func(ctx, &arg, params);

	tee_close_session(ctx, xdev_session);
	xdev_session = -1;

	return 0;
}

int gx_xdev_ioctl(struct file *filp, unsigned long h, unsigned int cmd, void *data, unsigned int size)
{
	int ret;
	struct tee_context *ctx = filp->private_data;
	struct tee_ioctl_invoke_arg arg;
	struct tee_param params[TEE_PAYLOAD_REF_COUNT];

	memset(&arg, 0, sizeof(arg));
	memset(params, 0, sizeof(params));

	arg.func = 5; // TA_XDEV_IOCT
	arg.session = xdev_session;
	arg.num_params = TEE_PAYLOAD_REF_COUNT;

	params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INPUT;
	params[0].u.value.a = h;
	params[0].u.value.b = (u64)cmd;

	if (data != NULL && size != 0) {
		params[1].attr = OPTEE_MSG_ATTR_TYPE_RMEM_INOUT;
		params[1].u.memref.size = size;
		params[1].u.memref.shm = tee_shm_alloc(ctx, size, TEE_SHM_MAPPED);
		if (params[1].u.memref.shm == NULL) {
			printk("%s. malloc NULL !\n", __func__);
			return -1;
		}
		memcpy(tee_shm_get_va(params[1].u.memref.shm, 0), data, size);
	}

	ret = ctx->teedev->desc->ops->invoke_func(ctx, &arg, params);
	if (params[1].u.memref.shm != NULL) {
		if (ret == 0)
			memcpy(data, tee_shm_get_va(params[1].u.memref.shm, 0), size);
		tee_shm_free(params[1].u.memref.shm);
	}

	return ret;
}

EXPORT_SYMBOL_GPL(gx_xdev_open);
EXPORT_SYMBOL_GPL(gx_xdev_close);
EXPORT_SYMBOL_GPL(gx_xdev_ioctl);

