#include "gxse_core.h"
#include "common_dev.h"

int gxse_os_device_create(GxDeviceDesc *desc)
{
	int i, j, ret, devno;
	int pos = 0, dev_name_len = 0;

	if (NULL == desc)
		return -1;

	devno = MKDEV(desc->major, 0);

	if ((ret = register_chrdev_region(devno, desc->dev_count,  desc->ch_name)) != 0) {
		gxlog_e(GXSE_LOG_MOD_DEV, "unable to get major %d %d\n", desc->major, ret);
		return ret;
	}

	cdev_init(&desc->cdev, desc->fops);
	if ((ret = cdev_add(&desc->cdev, devno, desc->dev_count)) != 0) {
		gxlog_e(GXSE_LOG_MOD_DEV, "unable register character device\n");
		goto error;
	}

	desc->class = class_create(THIS_MODULE, desc->class_name);
	if (IS_ERR(desc->class)) {
		ret = PTR_ERR(desc->class);
		goto error;
	}

	for (j = 0; j < MAX_DEV_NAME_NUM; j++) {
		if (desc->sub_count[j] == -1)
			break;

		for (i = 0; i < desc->sub_count[j]; i++) {
			dev_name_len = desc->dev_name_len[j];
			if (desc->sub_count[j] > 1) {
				desc->dev_name[j][dev_name_len] = '0'+i;
				desc->dev_name[j][dev_name_len+1] = '\0';
			}
			desc->dev = device_create(desc->class, NULL, MKDEV(desc->major, pos + i), NULL, desc->dev_name[j]);
		}
		pos += desc->sub_count[j];
	}

	return 0;

error:
	cdev_del(&desc->cdev);
	unregister_chrdev_region(devno, desc->dev_count);

	return ret;
}

int gxse_os_device_destroy(GxDeviceDesc *desc)
{
	int i;
	for (i = 0; i < desc->dev_count; i++)
		device_destroy(desc->class, MKDEV(desc->major, i));
	class_destroy(desc->class);
	cdev_del(&desc->cdev);
	unregister_chrdev_region(MKDEV(desc->major, 0), desc->dev_count);

	return 0;
}

int gx_common_copy_to_usr(void *u_ptr, void **k_ptr, unsigned int len)
{
    if (NULL == *k_ptr || NULL == u_ptr)
        return -1;

    if (copy_to_user(u_ptr, *k_ptr, len))
        return -1;

	gx_free(*k_ptr);
	*k_ptr = NULL;
    return 0;
}

int gx_common_copy_from_usr(void **k_ptr, void *u_ptr, unsigned int len)
{
	if (NULL == u_ptr || NULL == k_ptr)
		return -1;

    *k_ptr = gx_malloc(len/4*4+4);
    if (NULL == *k_ptr)
        return -1;

    if (copy_from_user(*k_ptr, u_ptr, len)) {
        gx_free(*k_ptr);
		*k_ptr = NULL;
        return -1;
    }

    return 0;
}

irqreturn_t gxse_os_dev_isr(int irq, void *dev_id)
{
	int i = 0;
	GxSeDeviceIRQ *ldev = (GxSeDeviceIRQ *)dev_id;

	for (i = 0; i < GXSE_MAX_DEV_IRQ; i++) {
		if (ldev->device_mask & (0x1<<i))
			gxse_device_isr(ldev->device[i]);
	}

	return IRQ_HANDLED;
}

void gxse_os_dev_dsr(void) { }

int gxse_os_dev_close(struct inode *inode, struct file *filp)
{
	GxSeDevice *dev = filp->private_data;

	return gxse_device_close(dev);
}

long gxse_os_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	GxSeDevice *dev = filp->private_data;

	long ret = gxse_device_ioctl(dev, cmd, (void *)arg, GXSE_IOC_SIZE(cmd));

	CONVERSION_RET(ret);
}

unsigned int gxse_os_dev_poll(struct file *filp, struct poll_table_struct *wait)
{
	int32_t ret = 0;
	uint32_t mask = 0;
	void *r_wait = NULL, *w_wait = NULL;
	unsigned long req_events = poll_requested_events(wait);
	GxSeDevice *dev = filp->private_data;

	if (req_events & POLLIN) {
		if ((ret = gxse_device_poll(dev, GXSE_POLL_R, &r_wait, &w_wait)) <= 0) {
			if (r_wait)
				poll_wait(filp, r_wait, wait);
		} else {
			if (ret & GXSE_POLL_R)
				mask = (POLLIN | POLLRDNORM);
		}
	}

	if (req_events & POLLOUT) {
		if ((ret |= gxse_device_poll(dev, GXSE_POLL_W, &r_wait, &w_wait)) <= 0) {
			if (w_wait)
				poll_wait(filp, w_wait, wait);
		} else {
			if (ret & GXSE_POLL_W)
				mask |= (POLLOUT | POLLWRNORM);
		}
	}

	return ret < 0 ? 0 : mask;
}

int gxse_os_dev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned int buf = (unsigned int)(vma->vm_pgoff << PAGE_SHIFT);

    if (remap_pfn_range(vma, vma->vm_start, gx_virt_to_phys(buf) >> PAGE_SHIFT,
				vma->vm_end - vma->vm_start, vma->vm_page_prot))
        return -EAGAIN;

    return 0;
}

ssize_t gxse_os_dev_read(struct file *filp, char __user * buf, size_t size,loff_t * ppos)
{
	GxSeDevice *dev = filp->private_data;

	long ret = gxse_device_read(dev, buf, size);

	CONVERSION_RET(ret);
}

ssize_t gxse_os_dev_write(struct file *filp,const char __user *buf, size_t size, loff_t *ppos)
{
	GxSeDevice *dev = filp->private_data;

	long ret = gxse_device_write(dev, buf, size);

	CONVERSION_RET(ret);
}

