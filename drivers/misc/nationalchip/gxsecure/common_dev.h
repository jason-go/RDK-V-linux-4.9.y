#ifndef __COMMON_DEVICE_H__
#define __COMMON_DEVICE_H__

#include "kernelcalls.h"

#define KLM_MAJOR         140
#define CRYPTO_MAJOR      141
#define MISC_MAJOR        142
#define SECURE_MAJOR      144
#define SECURE_TEE_MAJOR  145
#define FIREWALL_MAJOR    146

#define IRQF_GXSECURE IRQF_SHARED

#define MAX_DEV_NAME_NUM 3

typedef struct {
    char*          ch_name;
    char*          class_name;

    int            major;
    struct cdev    cdev;
    struct class*  class;
    struct device* dev;

	int            dev_count;
    int            sub_count[MAX_DEV_NAME_NUM];
    int            dev_name_len[MAX_DEV_NAME_NUM];
	char           dev_name[MAX_DEV_NAME_NUM][16];

    struct file_operations* fops;
} GxDeviceDesc;

int  gxse_os_device_create(GxDeviceDesc *desc);
int  gxse_os_device_destroy(GxDeviceDesc *desc);
int  gx_common_copy_from_usr(void **k_ptr, void *u_ptr, unsigned int len);
int  gx_common_copy_to_usr(void *u_ptr, void **k_ptr, unsigned int len);
int  gxse_os_dev_close(struct inode *inode, struct file *filp);
long gxse_os_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
int  gxse_os_dev_mmap(struct file *filp, struct vm_area_struct *vma);
void gxse_os_dev_dsr(void);
irqreturn_t gxse_os_dev_isr(int irq, void *dev_id);
ssize_t gxse_os_dev_read(struct file *filp, char __user * buf, size_t size,loff_t * ppos);
ssize_t gxse_os_dev_write(struct file *filp,const char __user *buf, size_t size, loff_t *ppos);
unsigned int gxse_os_dev_poll(struct file *filp, struct poll_table_struct *wait);

int klm_probe         ( void );
int klm_remove        ( void );
int misc_probe        ( void );
int misc_remove       ( void );
int crypto_probe      ( void );
int crypto_remove     ( void );
int secure_probe      ( void );
int secure_remove     ( void );
int firewall_probe    ( void );
int firewall_remove   ( void );

#define CALL_COPY_FROM_USR(k_buf, buf, len, tag) \
	do { \
		if (buf && len) { \
			if (gx_common_copy_from_usr((void **)&k_buf, (void *)buf, len)) \
				goto tag; \
			buf = k_buf; \
		} \
	} while(0)

#define CALL_COPY_TO_USR(buf, k_buf, len, tag) \
	do { \
		if (len) { \
			if (gx_common_copy_to_usr((void *)buf, (void **)&k_buf, len)) \
				goto tag; \
		} \
	} while(0)

#endif
