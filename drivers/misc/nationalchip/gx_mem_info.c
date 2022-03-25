#include <linux/moduleparam.h>
#include <linux/sysrq.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/ctype.h>
#include <linux/gx_mem_info.h>
#include <asm/setup.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/uaccess.h>


#define MISC_MEMINFO_MINOR (MISC_DYNAMIC_MINOR)

/* You can use " around spaces, but can't escape ". */
/* Hyphens and underscores equivalent in parameter names. */
static char *next_arg(char *args, char **param, char **val)
{
	unsigned int i, equals = 0;
	int in_quote = 0, quoted = 0;
	char *next;

	if (*args == '"') {
		args++;
		in_quote = 1;
		quoted = 1;
	}

	for (i = 0; args[i]; i++) {
		if (isspace(args[i]) && !in_quote)
			break;
		if (equals == 0) {
			if (args[i] == '=')
				equals = i;
		}
		if (args[i] == '"')
			in_quote = !in_quote;
	}

	*param = args;
	if (!equals)
		*val = NULL;
	else {
		args[equals] = '\0';
		*val = args + equals + 1;

		/* Don't include quotes in value. */
		if (**val == '"') {
			(*val)++;
			if (args[i-1] == '"')
				args[i-1] = '\0';
		}
	}
	if (quoted && args[i-1] == '"')
		args[i-1] = '\0';

	if (args[i]) {
		args[i] = '\0';
		next = args + i + 1;
	} else
		next = args + i;

	/* Chew up trailing spaces. */
	return skip_spaces(next);
}

int gx_mem_info_get(char *name, struct gx_mem_info *info)
{
	int ret = 0;
	char *tmp_cmdline = NULL;
	char *args = NULL;
	char *param = NULL;
	char *val = NULL;

	if (!info) {
		printk("error: %s %d\n", __func__, __LINE__);
		return -1;
	}

	tmp_cmdline = args = kmalloc(COMMAND_LINE_SIZE, GFP_KERNEL);
	if (!args) {
		printk("error: %s %d, malloc failed\n", __func__, __LINE__);
		ret = -1;
		goto exit;
	}
	memset(args, 0x0, COMMAND_LINE_SIZE);

	strncpy(args, saved_command_line, COMMAND_LINE_SIZE);

	while (*args) {
		val = NULL;
		param = NULL;
		args = next_arg(args, &param, &val);
		if (!(param && val))
			continue;

		if (!strcmp(name, param)) {
			info->size = memparse(val, &val);
			if (*(val++) != '@') {
				printk("error: %s %d, <%s> param error\n", __func__, __LINE__, name);
				ret = -1;
				goto exit;
			}
			info->start = memparse(val, &val);
#ifdef GX_MEM_DEBUG
			printk("name : %-10s, start : 0x%08x, size : 0x%08x\n", name, info->start, info->size);
#endif

			ret = 0;
			goto exit;
		}
	}


	ret = -1;

exit:
	if (tmp_cmdline) {
		kfree(tmp_cmdline);
		tmp_cmdline = NULL;
	}
	return ret;
}

EXPORT_SYMBOL(gx_mem_info_get);

static int protect_flag = 0;
static int __init get_protect_flag(char *p)
{
	if (p)
		protect_flag = memparse(p, NULL);

	return 0;
}
__setup("protect_flag=", get_protect_flag);

int gx_mem_protect_type_get(int *type)
{
	if (!type)
		return -1;

	*type = protect_flag;
	return 0;
}
EXPORT_SYMBOL(gx_mem_protect_type_get);

#define MEM_NAME_LEN		(32)
static char mem_name[MEM_NAME_LEN] = {0};

static int gx_mem_info_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static long gx_mem_info_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	switch(cmd) {
		case GX_MEM_INFO_SET_NAME:
			{
				unsigned char __user *name = (unsigned char __user *)arg;
				memset(mem_name, 0, MEM_NAME_LEN);
				ret = copy_from_user(mem_name, (const void __user *)name, MEM_NAME_LEN);
				if (ret)
					ret = -EIO;
				break;
			}
		case GX_MEM_INFO_GET_INFO:
			{
				struct gx_mem_info info = {0};
				ret = gx_mem_info_get(mem_name, &info);
				if (ret)
					ret = -EIO;
				else
					ret = copy_to_user((void __user *)arg, (const void *)&info, sizeof(info));
				break;
			}
		default:
			return -ENOTTY;
	}

	return ret;
}

static struct file_operations gx_mem_info_fops = {
	.owner = THIS_MODULE,
	.open = gx_mem_info_open,
	.unlocked_ioctl = gx_mem_info_ioctl,
};

static struct miscdevice gx_mem_info_miscdev = {
	.minor = MISC_MEMINFO_MINOR,
	.name = "gx_mem_info",
	.fops = &gx_mem_info_fops,
};

static int gx_mem_info_probe(struct platform_device *pdev)
{
	int ret = -1;

	ret = misc_register(&gx_mem_info_miscdev);
	if (ret != 0) {
		printk("error: [%s %d] failed to register misc device\n", __func__, __LINE__);
	}

	return ret;
}

static int gx_mem_info_remove(struct platform_device *pdev)
{
	misc_deregister(&gx_mem_info_miscdev);
	return 0;
}

static struct platform_device gx_mem_info_device = {
	.name = "gx_mem_info",
	.id = -1,
};

static struct platform_driver gx_mem_info_driver = {
	.probe = gx_mem_info_probe,
	.remove = gx_mem_info_remove,
	.driver = {
		.name = "gx_mem_info",
	},
};


static int __init mem_info_init(void)
{
	printk("Nationalchip mem_info Driver\n");
	platform_device_register(&gx_mem_info_device);
	return platform_driver_register(&gx_mem_info_driver);
}

static void __exit mem_info_exit(void)
{
	platform_driver_unregister(&gx_mem_info_driver);
	platform_device_del(&gx_mem_info_device);
}

module_init(mem_info_init);
module_exit(mem_info_exit);

MODULE_DESCRIPTION("support for NationalChilp gx_mem_info modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
