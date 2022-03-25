#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/err.h>

#include "gxavdev.h"
#include "gxav_ioctl.h"
#include "kernelcalls.h"
#include "gxav_event_type.h"

#define malloc_cache(dev, mod, size) \
	((size) > MAX_PROPSIZE ? gx_malloc(size): ((dev)->module_buffer[mod]))

#define free_cache(p, size) \
	do{ if((size) > MAX_PROPSIZE) gx_free(p);}while(0)

#define check_ret(ret) do{ if(ret < 0) return -EFAULT;}while(0)

#define linuxav_mutex_lock(mutex)   mutex_lock(mutex)
#define linuxav_mutex_unlock(mutex) mutex_unlock(mutex)

#ifdef CONFIG_ARM
#define linuxav_copy_from_user copy_from_user
#define linuxav_copy_to_user   copy_to_user
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,9,0)
#define linuxav_copy_from_user __generic_copy_from_user
#define linuxav_copy_to_user   __generic_copy_to_user
#else
#define linuxav_copy_from_user raw_copy_from_user
#define linuxav_copy_to_user   raw_copy_to_user
#endif
#endif

static int linuxav_module_open(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_module_set *param = arg;
	struct gxav_module *module = NULL;
	int ret = -EFAULT;

	linuxav_mutex_lock(&dev->lock);

	param->module_id = gxav_module_open(dev->av_dev, param->module_type, param->channel_id);
	if (param->module_id < 0)
		goto error;

	module = dev->av_dev->modules_handle[param->module_id]->module;
	if(module == NULL || module->inode == NULL)
		goto error;

	if (module->inode->refcount == 1) {
		mutex_init(&dev->module_lock[param->module_id]);
		module->inode->lock = &dev->module_lock[param->module_id];

		module->inode->wq = gx_malloc(sizeof(wait_queue_head_t));
		if(module->inode->wq == NULL)
			goto error;

		GXAV_DBG("%s(),init_waitqueue_head start : param->module_id=%d,wq=0x%x\n",__FUNCTION__,param->module_id,
				(unsigned int)(module->inode->wq));

		init_waitqueue_head((wait_queue_head_t *)module->inode->wq);

		GXAV_DBG("%s(),init_waitqueue_head end : param->module_id=%d,wq=0x%x\n",__FUNCTION__,param->module_id,
				(unsigned int)(module->inode->wq));
	}

	ret = 0;
error:
	linuxav_mutex_unlock(&dev->lock);

	return ret;
}

static int linuxav_module_close(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_module_set *param = (struct gxav_module_set*)arg;
	struct gxav_module *module = NULL;
	int ret = -EFAULT;

	linuxav_mutex_lock(&dev->lock);

	if(dev->av_dev == NULL)
		goto error;
	if(param->module_id < 0 || param->module_id >= GXAV_MAX_HANDLE)
		goto error;
	if(dev->av_dev->modules_handle[param->module_id] == NULL)
		goto error;

	module = dev->av_dev->modules_handle[param->module_id]->module;
	param->ret = gxav_module_close(dev->av_dev, param->module_id);
	if(param->ret < 0 || module == NULL || module->inode == NULL)
		goto error;

	if (module->inode->refcount == 0) {
		init_waitqueue_head((wait_queue_head_t *)module->inode->wq);
		gx_free((void*)module->inode->wq);
		module->inode->wq = NULL;
		mutex_destroy((struct mutex*)module->inode->lock);
	}

	ret = 0;
error:
	linuxav_mutex_unlock(&dev->lock);

	return ret;
}

static int linuxav_property_set(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_property *param = arg;

	param->ret = gxav_module_set_property(dev->av_dev, param->module_id, param->prop_id, param->prop_val, param->prop_size);

	return 0;
}

static int linuxav_property_get(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_property *param = arg;

	param->ret = gxav_module_get_property(dev->av_dev, param->module_id, param->prop_id, param->prop_val, param->prop_size);

	return 0;
}

static int linuxav_write_ctrlinfo(struct gxav_priv_dev *dev, struct gxav_ctrlinfo* param)
{
	void* ctrl_info = 0;
	struct gxav_module *module;

	if(param->module_id < 0 || param->module_id >= GXAV_MAX_HANDLE) {
		gx_printf("\n\n[ERROR] %s :module_id = %d, control info\n\n", __func__, param->module_id);
		return -1;
	}

	if(dev->av_dev->modules_handle[param->module_id] == NULL){
		gx_printf("%s: Module:[%d] has been closed ! control info\n",
				__func__, param->module_id);
		return -EFAULT;
	}

	module = dev->av_dev->modules_handle[param->module_id]->module;

	linuxav_mutex_lock(module->inode->lock);

	if (param->ctrl_size > 0) {
		ctrl_info = (void*)malloc_cache(dev, param->module_id, param->ctrl_size);
		if ((ctrl_info == NULL) ||
				copy_from_user(ctrl_info, param->ctrl_info, param->ctrl_size) > 0) {
			free_cache(ctrl_info, param->ctrl_size);
			linuxav_mutex_unlock(module->inode->lock);
			return -EFAULT;
		}
	}

	param->ret = gxav_module_write_ctrlinfo(dev->av_dev, param->module_id, ctrl_info, param->ctrl_size);

	free_cache(ctrl_info, param->ctrl_size);
	linuxav_mutex_unlock(module->inode->lock);

	return 0;
}

static int linuxav_data_send(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_data *param = arg;

	if (param->data_size > 0 && param->fifo != NULL) {
		linuxav_mutex_lock(&dev->lock);

		ret = gxav_channel_put(dev->av_dev, param->fifo,
				(void * (*)(void *, const void *, int))linuxav_copy_from_user,
				param->data_buffer, param->data_size, param->pts);
		if(ret < 0) {
			linuxav_mutex_unlock(&dev->lock);
			return -EFAULT;
		}
		param->ret = ret;

		linuxav_mutex_unlock(&dev->lock);

		while (param->ret < param->data_size) {
			ret = gx_wait_event(gxav_device_getmod(dev->av_dev, dev->av_dev->sdc_module_id),
					EVENT_FIFO_EMPTY(gxav_channel_id_get(param->fifo)), param->timeout_us);
			if(ret <= 0) {
				gx_printf(" GXAV_IOCTL_DATA_SEND :: gx_wait_event fail \n");
				return 0;
			}

			linuxav_mutex_lock(&dev->lock);

			ret = gxav_channel_put(dev->av_dev, param->fifo,
					(void * (*)(void *, const void *, int))linuxav_copy_from_user, \
					param->data_buffer+param->ret, param->data_size - param->ret, -1);
			if(ret < 0) {
				linuxav_mutex_unlock(&dev->lock);
				return -EFAULT;
			}
			param->ret += ret;

			linuxav_mutex_unlock(&dev->lock);
		}

		if(param->pts != -1) {
			linuxav_mutex_lock(&dev->lock);

			ret = gxav_channel_put_pts(dev->av_dev, param->fifo,param->pts);
			if(ret < 0) {
				linuxav_mutex_unlock(&dev->lock);
				return -EFAULT;
			}

			linuxav_mutex_unlock(&dev->lock);

			while(ret == 0) {
				ret = gx_wait_event(gxav_device_getmod(dev->av_dev, dev->av_dev->sdc_module_id),
						EVENT_FIFO_EMPTY(gxav_channel_pts_id_get(param->fifo)), -1);
				if (ret <= 0){
					gx_printf(" START PTS_SEND :: gx_wait_event fail \n");
					return 0;
				}

				linuxav_mutex_lock(&dev->lock);

				ret = gxav_channel_put_pts(dev->av_dev, param->fifo,param->pts);
				if(ret < 0) {
					linuxav_mutex_unlock(&dev->lock);
					return -EFAULT;
				}

				linuxav_mutex_unlock(&dev->lock);
			}
		}

		if (gxav_channel_flag_get(dev->av_dev,param->fifo) == RW) {
			ret = gx_wake_event(gxav_device_getmod(dev->av_dev, dev->av_dev->sdc_module_id),
					EVENT_FIFO_FULL(gxav_channel_id_get(param->fifo)));
			if(ret < 0) {
				gx_printf(" gx_wake_event fail \n");
				return -EFAULT;
			}
		}
	}

	return 0;
}

static int linuxav_data_receive(struct gxav_priv_dev *dev, void *arg, int peek)
{
	int ret = 0;
	struct gxav_data *param = arg;

	if (param->data_size > 0 && param->fifo != NULL) {

		linuxav_mutex_lock(&dev->lock);

		ret = gxav_channel_get(dev->av_dev, param->fifo,
				(void * (*)(void *, const void *, int))linuxav_copy_to_user,
				param->data_buffer, param->data_size, peek);
		if(ret < 0) {
			linuxav_mutex_unlock(&dev->lock);
			return -EFAULT;
		}
		param->ret = ret;

		if(!peek)
			gxav_channel_get_pts(dev->av_dev, param->fifo, &param->pts);

		linuxav_mutex_unlock(&dev->lock);

		while (param->ret < param->data_size) {
			ret = gx_wait_event(gxav_device_getmod(dev->av_dev, dev->av_dev->sdc_module_id),
					EVENT_FIFO_FULL(gxav_channel_id_get(param->fifo)), param->timeout_us);
			if (ret <= 0){
				GXAV_DBG(" GXAV_IOCTL_DATA_RECEIVE :: gx_wait_event fail \n");
				linuxav_mutex_lock(&dev->lock);

				ret = gxav_channel_get(dev->av_dev, param->fifo,
						(void * (*)(void *, const void *, int))linuxav_copy_to_user,
						param->data_buffer + param->ret, param->data_size - param->ret, peek);
				if(ret < 0) {
					linuxav_mutex_unlock(&dev->lock);
					return -EFAULT;
				}
				param->ret += ret;

				if(!peek) {
					if (param->pts != -1)
						gxav_channel_get_pts(dev->av_dev, param->fifo, NULL);
					else
						gxav_channel_get_pts(dev->av_dev, param->fifo, &param->pts);
				}

				linuxav_mutex_unlock(&dev->lock);

				return 0;
			}

			linuxav_mutex_lock(&dev->lock);

			ret = gxav_channel_get(dev->av_dev, param->fifo,
					(void * (*)(void *, const void *, int))linuxav_copy_to_user,
					param->data_buffer + param->ret, param->data_size - param->ret, peek);
			if(ret < 0) {
				linuxav_mutex_unlock(&dev->lock);
				return -EFAULT;
			}
			param->ret += ret;
			if(!peek)
				gxav_channel_get_pts(dev->av_dev, param->fifo, &param->pts);
			linuxav_mutex_unlock(&dev->lock);
		}

		if (gxav_channel_flag_get(dev->av_dev,param->fifo) == RW) {
			ret = gx_wake_event(gxav_device_getmod(dev->av_dev, dev->av_dev->sdc_module_id),
					EVENT_FIFO_EMPTY(gxav_channel_id_get(param->fifo)));
			if(ret < 0) {
				GXAV_DBG(" gx_wake_event fail \n");
				return -EFAULT;
			}
		}
	}

	return 0;
}

static int linuxav_data_skip(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_data *param = arg;

	if (param->data_size > 0 && param->fifo != NULL) {

		linuxav_mutex_lock(&dev->lock);

		ret = gxav_channel_skip(dev->av_dev, param->fifo,param->data_size);
		if(ret < 0) {
			linuxav_mutex_unlock(&dev->lock);
			return -EFAULT;
		}
		param->ret = ret;

		linuxav_mutex_unlock(&dev->lock);

		while (param->ret < param->data_size) {
			ret = gx_wait_event(gxav_device_getmod(dev->av_dev, dev->av_dev->sdc_module_id),
					EVENT_FIFO_FULL(gxav_channel_id_get(param->fifo)), param->timeout_us);
			if (ret <= 0){
				GXAV_DBG(" GXAV_IOCTL_DATA_SKIP :: gx_wait_event fail \n");
				linuxav_mutex_lock(&dev->lock);

				ret = gxav_channel_skip(dev->av_dev, param->fifo,param->data_size - param->ret);
				if(ret < 0) {
					linuxav_mutex_unlock(&dev->lock);
					return -EFAULT;
				}
				param->ret += ret;

				linuxav_mutex_unlock(&dev->lock);

				return 0;
			}

			linuxav_mutex_lock(&dev->lock);

			ret = gxav_channel_skip(dev->av_dev, param->fifo,param->data_size - param->ret);
			if(ret < 0) {
				linuxav_mutex_unlock(&dev->lock);
				return -EFAULT;
			}
			param->ret += ret;
			linuxav_mutex_unlock(&dev->lock);
		}

		if (gxav_channel_flag_get(dev->av_dev,param->fifo) == RW) {
			ret = gx_wake_event(gxav_device_getmod(dev->av_dev, dev->av_dev->sdc_module_id),
					EVENT_FIFO_EMPTY(gxav_channel_id_get(param->fifo)));
			if(ret < 0) {
				GXAV_DBG(" gx_wake_event fail \n");
				return -EFAULT;
			}
		}
	}

	return 0;
}

static int linuxav_event_reset(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_event *param = arg;
	struct gxav_module *module;

	linuxav_mutex_lock(&dev->lock);

	if(dev->av_dev == NULL) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	if(param->module_id < 0 || param->module_id >= GXAV_MAX_HANDLE) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	if(dev->av_dev->modules_handle[param->module_id] == NULL) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	module = dev->av_dev->modules_handle[param->module_id]->module;
	if(module == NULL || module->inode == NULL || module->inode->wq == NULL) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}

	param->ret = gxav_module_inode_clear_event(module->inode, param->event_mask);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_event_set(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_event *param = arg;
	struct gxav_module *module;

	linuxav_mutex_lock(&dev->lock);

	if(dev->av_dev == NULL) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	if(param->module_id < 0 || param->module_id >= GXAV_MAX_HANDLE) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	if(dev->av_dev->modules_handle[param->module_id] == NULL) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	module = dev->av_dev->modules_handle[param->module_id]->module;
	if(module == NULL || module->inode == NULL || module->inode->wq == NULL) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}

	wake_up_interruptible_sync((wait_queue_head_t *)module->inode->wq);

	param->ret = gxav_module_inode_set_event(module->inode, param->event_mask);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_event_wait(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_event *param = arg;

	param->ret = gx_wait_event(gxav_device_getmod(dev->av_dev, param->module_id), param->event_mask, param->timeout_us);

	return 0;
}

static int linuxav_fifo_create(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_fifo_info *param = arg;

	linuxav_mutex_lock(&dev->lock);

	param->fifo = gxav_channel_apply(dev->av_dev, param->memory, param->data_size, param->type);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_destroy(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_fifo_info* param = arg;

	linuxav_mutex_lock(&dev->lock);

	param->ret = gxav_channel_free(dev->av_dev, param->fifo);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_reset(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_fifo_info* param = arg;

	linuxav_mutex_lock(&dev->lock);

	param->ret = gxav_channel_reset(dev->av_dev, param->fifo);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_rollback(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_fifo_info* param = arg;

	linuxav_mutex_lock(&dev->lock);

	param->ret = gxav_channel_rollback(dev->av_dev, param->fifo, param->data_size);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_config(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_fifo_info* param = arg;
	struct gxav_gate_info info;

	if (copy_from_user(&info, (char *)(param->sdc_info), sizeof(info)) > 0)
		return -EFAULT;

	linuxav_mutex_lock(&dev->lock);

	param->ret = gxav_channel_gate_set(dev->av_dev, param->fifo, &info);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_get_length(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_info *param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_channel_length_get(dev->av_dev, param->fifo);
	if(ret < 0) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	param->data_size = ret;

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_get_pts_length(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_info *param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_channel_pts_length(dev->av_dev, param->fifo);
	if(ret < 0) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	param->data_size = ret;

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_get_freesize(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_info *param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_channel_freesize_get(dev->av_dev, param->fifo);
	if(ret < 0) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	param->free_size = ret;

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_get_pts_freesize(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_info *param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_channel_pts_freesize(dev->av_dev, param->fifo);
	if(ret < 0) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	param->free_size = ret;

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_get_memory(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_fifo_info *param = arg;
	struct gxav_channel *channel = (struct gxav_channel*)param->fifo;

	if (channel)
		param->memory = channel->buffer;
	else
		param->memory = NULL;

	return 0;
}

static int linuxav_fifo_get_cap(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_info *param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_channel_cap_get(dev->av_dev, param->fifo);
	if(ret < 0) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	param->info_size = ret;

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_get_pts_cap(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_info *param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_channel_pts_cap(dev->av_dev, param->fifo);
	if(ret < 0) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}
	param->info_size = ret;

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_get_flag(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_info *param = arg;

	if (param->fifo == NULL)
		return -EFAULT;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_channel_flag_get(dev->av_dev,param->fifo);
	if(ret < 0) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}

	param->flag = ret;

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_pts_isfull(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_info *param = arg;

	if (param->fifo == NULL)
		return -EFAULT;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_channel_pts_isfull(dev->av_dev,param->fifo);
	if(ret < 0) {
		linuxav_mutex_unlock(&dev->lock);
		return -EFAULT;
	}

	param->pts_isfull = ret;

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_fifo_link(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_link *param = arg;

	if (param->channel == NULL)
		return -EFAULT;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_module_link(dev->av_dev, param->module_id,
			param->dir,
			param->pin_id,
			param->channel);

	linuxav_mutex_unlock(&dev->lock);

	return ret;
}

static int linuxav_fifo_unlink(struct gxav_priv_dev *dev, void *arg)
{
	int ret = 0;
	struct gxav_fifo_link *param = arg;

	if (param->channel == NULL)
		return -EFAULT;

	linuxav_mutex_lock(&dev->lock);

	ret =gxav_module_unlink(dev->av_dev, param->module_id, param->channel);

	linuxav_mutex_unlock(&dev->lock);

	return ret;
}

static int linuxav_chip_detect(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_chip_detect *param = arg;

	linuxav_mutex_lock(&dev->lock);

	param->chip_id = gxcore_chip_probe();

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

#ifdef CONFIG_AV_MODULE_FIRMWARE
static int linuxav_codec_register(struct gxav_priv_dev *dev, void *arg)
{
	int ret;

	struct gxav_codec_register *param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_firmware_register(param);

	linuxav_mutex_unlock(&dev->lock);

	return ret;
}

static int linuxav_codec_get_info(struct gxav_priv_dev *dev, void *arg)
{
	int ret;

	struct gxav_codec_info *param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_firmware_get_info(param);

	linuxav_mutex_unlock(&dev->lock);

	return ret;
}
#endif

static int linuxav_hdcp_key_register(struct gxav_priv_dev* dev, void *arg)
{
	int ret;

	struct gxav_hdcpkey_register * param = arg;

	linuxav_mutex_lock(&dev->lock);

	ret = gxav_hdcp_key_register(param);

	linuxav_mutex_unlock(&dev->lock);

	return ret;
}

static int linuxav_kmalloc(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_kmalloc *param = arg;

	if (param->size > MAX_KMALLOC_SIZE)
		return -EFAULT;

	linuxav_mutex_lock(&dev->lock);

	param->addr = (unsigned long)gx_page_malloc(param->size);
	gx_dcache_inv_range(param->addr, param->size);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

static int linuxav_kfree(struct gxav_priv_dev *dev, void *arg)
{
	struct gxav_kmalloc *param = arg;

	linuxav_mutex_lock(&dev->lock);

	gx_page_free((void*)param->addr, param->size);

	linuxav_mutex_unlock(&dev->lock);

	return 0;
}

int gxav_ioctl(struct gxav_priv_dev *dev, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	GXAV_DBGEV("%s: dev=%p, cmd=%u, arg=%lu\n", __FUNCTION__, dev, cmd, arg);

	switch (cmd) {
		case GXAV_IOCTL_MODULE_OPEN:
		case GXAV_IOCTL_MODULE_CLOSE:
			{
				struct gxav_module_set param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) > 0)
					return -EFAULT;

				if(cmd == GXAV_IOCTL_MODULE_OPEN)
					ret = linuxav_module_open(dev, &param);
				else
					ret = linuxav_module_close(dev, &param);

				check_ret(ret);

				if (copy_to_user((char *)arg, &param, sizeof(param)) != 0)
					return -EFAULT;

				return ret;
			}

		case GXAV_IOCTL_PROPERTY_SET:
		case GXAV_IOCTL_PROPERTY_GET:
			{
				struct gxav_property param;
				struct gxav_property kparam;
				struct gxav_module *module;

				gx_memset(&param, 0, sizeof(param));
				gx_memset(&kparam, 0, sizeof(kparam));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) > 0)
					return -EFAULT;

				if (param.module_id < 0 || param.module_id >= GXAV_MAX_HANDLE) {
					gx_printf("\n\n[ERROR] %s :module_id = %d, prop_id = %d\n\n", __func__, param.module_id, param.prop_id);
					return -1;
				}

				if (dev->av_dev->modules_handle[param.module_id] == NULL) {
					gx_printf("%s: Module:[%d] has been closed ! prop id = %d\n",
							__func__, param.module_id, kparam.prop_id);
					return -EFAULT;
				}

				module = dev->av_dev->modules_handle[param.module_id]->module;
				kparam = param;

				if (module == NULL || module->inode == NULL || module->inode->lock == 0) {
					gx_printf("%s: Module:[%d] has been closed ! prop id = %d\n",
							__func__, param.module_id, kparam.prop_id);
					return -EFAULT;
				}

				linuxav_mutex_lock(module->inode->lock);

				if (param.prop_size > 0) {
					kparam.prop_val = (void*)malloc_cache(dev, param.module_id, param.prop_size);
					if (param.prop_val==NULL || copy_from_user(kparam.prop_val, param.prop_val, param.prop_size) > 0) {
						free_cache(kparam.prop_val, param.prop_size);
						linuxav_mutex_unlock(module->inode->lock);
						return -EFAULT;
					}
				}

				if (cmd == GXAV_IOCTL_PROPERTY_GET) {
					linuxav_property_get(dev, &kparam);
					param.ret = kparam.ret;
					if (param.prop_val==NULL || copy_to_user(param.prop_val, kparam.prop_val, param.prop_size) > 0)
					{
						free_cache(kparam.prop_val, param.prop_size);
						linuxav_mutex_unlock(module->inode->lock);
						return -EFAULT;
					}
				}
				else{
					linuxav_property_set(dev, &kparam);
					param.ret = kparam.ret;
				}

				free_cache(kparam.prop_val, param.prop_size);
				linuxav_mutex_unlock(module->inode->lock);

				if (copy_to_user((char *)arg, &param, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}
		case GXAV_IOCTL_DATA_SEND:
		case GXAV_IOCTL_DATA_RECEIVE:
		case GXAV_IOCTL_DATA_PEEK:
		case GXAV_IOCTL_DATA_SKIP:
			{
				struct gxav_data param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) > 0)
					return -EFAULT;

				if(cmd == GXAV_IOCTL_DATA_SEND)
					ret = linuxav_data_send(dev, &param);
				else if(cmd == GXAV_IOCTL_DATA_RECEIVE)
					ret = linuxav_data_receive(dev, &param, 0);
				else if(cmd == GXAV_IOCTL_DATA_PEEK)
					ret = linuxav_data_receive(dev, &param, 1);
				else if(cmd == GXAV_IOCTL_DATA_SKIP)
					ret = linuxav_data_skip(dev, &param);
				check_ret(ret);

				if (copy_to_user((char *)arg, &param, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}
		case GXAV_IOCTL_WRITE_CTRLINFO:
			{
				struct gxav_ctrlinfo param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) > 0)
					return -EFAULT;

				ret = linuxav_write_ctrlinfo(dev, &param);

				check_ret(ret);

				if(copy_from_user(&param, (char *)arg, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}
		case GXAV_IOCTL_EVENT_SET:
		case GXAV_IOCTL_EVENT_WAIT:
		case GXAV_IOCTL_EVENT_RESET:
			{
				struct gxav_event param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) > 0)
					return -EFAULT;

				if(cmd == GXAV_IOCTL_EVENT_SET)
					ret = linuxav_event_set(dev, &param);
				else if(cmd == GXAV_IOCTL_EVENT_WAIT)
					ret = linuxav_event_wait(dev, &param);
				else if(cmd == GXAV_IOCTL_EVENT_RESET)
					ret = linuxav_event_reset(dev, &param);

				check_ret(ret);

				if (copy_to_user((char *)arg, &param, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}
		case GXAV_IOCTL_FIFO_LINK:
		case GXAV_IOCTL_FIFO_UNLINK:
		case GXAV_IOCTL_FIFO_RESET:
		case GXAV_IOCTL_FIFO_CONFIG:
		case GXAV_IOCTL_FIFO_CREATE:
		case GXAV_IOCTL_FIFO_DESTROY:
		case GXAV_IOCTL_FIFO_ROLLBACK:
		case GXAV_IOCTL_FIFO_GET_FLAG:
		case GXAV_IOCTL_FIFO_GET_LENGTH:
		case GXAV_IOCTL_FIFO_GET_FREESIZE:
		case GXAV_IOCTL_FIFO_GET_MEMORY:
		case GXAV_IOCTL_FIFO_GET_CAP:
		case GXAV_IOCTL_FIFO_PTS_ISFULL:
		case GXAV_IOCTL_FIFO_GET_PTS_LENGTH:
		case GXAV_IOCTL_FIFO_GET_PTS_FREESIZE:
		case GXAV_IOCTL_FIFO_GET_PTS_CAP:
			{
				struct gxav_fifo_info param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				if(cmd == GXAV_IOCTL_FIFO_LINK)
					ret = linuxav_fifo_link(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_UNLINK)
					ret = linuxav_fifo_unlink(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_RESET)
					ret = linuxav_fifo_reset(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_CONFIG)
					ret = linuxav_fifo_config(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_CREATE)
					ret = linuxav_fifo_create(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_DESTROY)
					ret = linuxav_fifo_destroy(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_ROLLBACK)
					ret = linuxav_fifo_rollback(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_GET_FLAG)
					ret = linuxav_fifo_get_flag(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_GET_LENGTH)
					ret = linuxav_fifo_get_length(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_GET_FREESIZE)
					ret = linuxav_fifo_get_freesize(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_GET_MEMORY)
					ret = linuxav_fifo_get_memory(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_GET_CAP)
					ret = linuxav_fifo_get_cap(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_PTS_ISFULL)
					ret = linuxav_fifo_pts_isfull(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_GET_PTS_LENGTH)
					ret = linuxav_fifo_get_pts_length(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_GET_PTS_FREESIZE)
					ret = linuxav_fifo_get_pts_freesize(dev, &param);
				else if(cmd == GXAV_IOCTL_FIFO_GET_PTS_CAP)
					ret = linuxav_fifo_get_pts_cap(dev, &param);
				check_ret(ret);

				if (copy_to_user((char *)arg, &param, sizeof(param)) != 0)
					return -EFAULT;

				return ret;
			}

		case GXAV_IOCTL_CHIP_DETECT:
			{
				struct gxav_chip_detect param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = linuxav_chip_detect(dev, &param);

				check_ret(ret);

				if (copy_to_user((char *)arg, &param, sizeof(param)) != 0)
					return -EFAULT;

				return ret;
			}

#ifdef CONFIG_AV_MODULE_FIRMWARE
		case GXAV_IOCTL_CODEC_REGISTER:
			{
				struct gxav_codec_register param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = linuxav_codec_register(dev, &param);

				check_ret(ret);

				return ret;
			}

		case GXAV_IOCTL_CODEC_INFO_GET:
			{
				struct gxav_codec_info param;

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = linuxav_codec_get_info(dev, (void*)(&param));

				check_ret(ret);

				if (copy_to_user((char *)arg, &param, sizeof(param)) != 0)
					return -EFAULT;

				return ret;
			}
#endif

#ifdef CONFIG_AV_MODULE_HDMI
		case GXAV_IOCTL_HDCP_KEY_REGISTER:
			{
				struct gxav_hdcpkey_register param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = linuxav_hdcp_key_register(dev, &param);

				check_ret(ret);

				return ret;
			}
#endif

		case GXAV_IOCTL_MEMHOLE_GET:
			{
				int ret;
				struct gxav_memhole_info param;
				struct gx_mem_info info = {0};

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = gx_mem_info_get(param.name, &info);
				if(ret == 0 && info.start) {
					param.size  = info.size;
					param.start = (unsigned int)gx_phys_to_virt(info.start);
					if (copy_to_user((char *)arg, &param, sizeof(param)) != 0)
						return -EFAULT;
				}

				return ret;
			}

		case GXAV_IOCTL_KMALLOC:
		case GXAV_IOCTL_KFREE:
			{
				struct gxav_kmalloc param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				if(cmd == GXAV_IOCTL_KMALLOC)
					ret = linuxav_kmalloc(dev, &param);
				else
					ret = linuxav_kfree(dev, &param);

				check_ret(ret);

				if (copy_to_user((char *)arg, &param, sizeof(param)) != 0)
					return -EFAULT;

				return ret;
			}
		case GXAV_IOCTL_MODULE_READ:
		case GXAV_IOCTL_MODULE_WRITE:
			{
				struct gxav_modrw_info param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) > 0)
					return -EFAULT;

				if (param.module_id < 0 || param.module_id >= GXAV_MAX_HANDLE) {
					gx_printf("\n\n[ERROR] %s :module_id = %d\n\n", __func__, param.module_id);
					return -1;
				}

				if (dev->av_dev->modules_handle[param.module_id] == NULL) {
					gx_printf("\n\n[ERROR] %s :module_id = %d\n\n", __func__, param.module_id);
					return -EFAULT;
				}

				if (cmd == GXAV_IOCTL_MODULE_WRITE)
					param.ret = gxav_module_write(dev->av_dev, param.module_id, param.buffer, param.size, param.timeout_us);
				else
					param.ret = gxav_module_read(dev->av_dev, param.module_id, param.buffer, param.size, param.timeout_us);

				if (copy_to_user((char *)arg, &param, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}
		case GXAV_IOCTL_MODULE_LOCK:
		case GXAV_IOCTL_MODULE_TRYLOCK:
		case GXAV_IOCTL_MODULE_UNLOCK:
			{
				struct gxav_modlock_info param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) > 0)
					return -EFAULT;

				if (param.module_id < 0 || param.module_id >= GXAV_MAX_HANDLE) {
					gx_printf("\n\n[ERROR] %s :module_id = %d\n\n", __func__, param.module_id);
					return -1;
				}

				if (dev->av_dev->modules_handle[param.module_id] == NULL) {
					gx_printf("\n\n[ERROR] %s :module_id = %d\n\n", __func__, param.module_id);
					return -EFAULT;
				}

				if (cmd == GXAV_IOCTL_MODULE_LOCK)
					param.ret = gxav_module_lock(dev->av_dev, param.module_id);
				else if (cmd == GXAV_IOCTL_MODULE_TRYLOCK)
					param.ret = gxav_module_trylock(dev->av_dev, param.module_id);
				else
					param.ret = gxav_module_unlock(dev->av_dev, param.module_id);

				if (copy_to_user((char *)arg, &param, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}
#ifdef CONFIG_AV_MODULE_STC
		case GXAV_IOCTL_SYNC_PARAMS_SET:
			{
				struct gxav_sync_params param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = gxav_set_sync_params (&param);

				check_ret(ret);

				return ret;
			}
#endif

		case GXAV_IOCTL_CAPABILITY_GET:
			{
				struct gxav_device_capability param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = gxav_device_capability(dev->av_dev, &param);

				if (copy_to_user((char *)arg, &param, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}

		case GXAV_IOCTL_DEBUG_CONFIG:
			{
				struct gxav_debug_config param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = gxav_debug_level_config(&param);

				if (copy_to_user((char *)arg, &param, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}

#ifdef CONFIG_AV_MODULE_DVR
		case GXAV_IOCTL_DVR2DVR_PROTOCOL:
			{
				struct gxav_dvr2dvr_protocol param;

				gx_memset(&param, 0, sizeof(param));

				if (copy_from_user(&param, (char *)arg, sizeof(param)) != 0)
					return -EFAULT;

				ret = gxav_dvr2dvr_protocol_control(&param);

				if (copy_to_user((char *)arg, &param, sizeof(param)) > 0)
					return -EFAULT;

				return ret;
			}
#endif

		default:
			return -2;
	}
}

int gxav_mod_open(struct gxav_priv_dev *dev, GxAvModuleType module_type, int sub)
{
	int ret = 0;
	struct gxav_module_set param;

	param.module_type = module_type;
	param.channel_id = sub;

	ret = linuxav_module_open(dev, &param);
	if (ret == 0) {
		return param.module_id;
	}
	else
		return ret;
}

int gxav_mod_close(struct gxav_priv_dev *dev, int module_id)
{
	struct gxav_module_set param;

	param.module_id = module_id;
	return linuxav_module_close(dev, &param);
}

EXPORT_SYMBOL(gxav_mod_open);
EXPORT_SYMBOL(gxav_mod_close);

