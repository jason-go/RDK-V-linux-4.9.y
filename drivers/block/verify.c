/******************************************************************************
 *    COPYRIGHT (C) 2018 NationalChip
 *    All rights reserved.
 * ***
 *    Create by zhangj 2018-09
 *
******************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/bio.h>
#include <linux/genhd.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <asm/sizes.h>
#include <linux/reboot.h>
#include <linux/suspend.h>
#include <linux/crypto.h>
#include <crypto/hash.h>
#include <linux/scatterlist.h>
#include <asm/setup.h>
#include <linux/mount.h>
#include <linux/syscalls.h>

#define VERBLOCK_SIZE        512
#define HASG_LEN             32
#define HAS_PART             "shatable"
#define VERIFY_BLOCK_NAME    "verify-block"
#define VERIFY_BLOCK_COUNT   1
#define TOUSY

#define SHA256_DEV_NAME    "/dev/sha256"
#define ROOTFS_DEV_NAME	   "/dev/rootfs"
#define IMG_MAX_BLOCK_NUM        (5)

#define ROMBLOCK "/dev/romblock"
#define MTDBLOCK "/dev/mtdblock"
#define MMCBLOCK "/dev/mmcblk0p"
/******************************************************************************/

struct verify_block_device {
	int idx;
	char hhfile_name[64];
	char blkdev_name[64];
	char blkname[32];
	char verify_tmp[HASG_LEN];
	char hash_tmp[HASG_LEN];

	struct gendisk *gd;
	spinlock_t queue_lock;
	struct request_queue *queue;
	struct mutex dev_mutex;

	struct request_queue *rq;
	struct workqueue_struct *wq;
	struct work_struct work;

	struct file *blkdev_file;
	struct file *blkdev_fp;
	unsigned int u32ImageOffset;

	int refcnt;
};

typedef struct hi_CAImgHead_S
{
    unsigned char  u8MagicNumber[32];          	//Magic Number: "xxxxxxxxxxxxxxxxxxxxxx"
    unsigned char  u8HeaderVersion[8];                   //version: "V000 0001"
    unsigned int u32TotalLen;                          //Total length
    unsigned int u32CodeOffset;                        //Image offset
    unsigned int u32SignedImageLen;                    //Signed Image file size
    unsigned int u32SignatureOffset;                     //Signed Image offset
    unsigned int u32SignatureLen;                       //Signature length
    unsigned int u32BlockNum;                          //Image block number
    unsigned int u32BlockOffset[IMG_MAX_BLOCK_NUM];    //Each Block offset
    unsigned int u32BlockLength[IMG_MAX_BLOCK_NUM];    //Each Block length
    unsigned int u32SoftwareVerion;                      //Software version
    unsigned int Reserverd[31];
    unsigned int u32CRC32;                             //CRC32 value
} HI_CAImgHead_S;
#define DCAS_LEN_MAGICNUMBER            (32)
#define DCAS_STR_MAGICNUMBER            ("Hisilicon_ADVCA_ImgHead_MagicNum")
#define CHECK_TAILSECTION(pu8Section)   (0 == memcmp(pu8Section, DCAS_STR_MAGICNUMBER, DCAS_LEN_MAGICNUMBER))

#define GX_HASH_MAGIC                   ("GX_HASH_ImageHeader_MagicNum")
#define GX_HASH_MAGIC_LEN               (28)
typedef struct gx_ImgHead_S
{
    unsigned char u8Magic[GX_HASH_MAGIC_LEN];  //Magic Number: "xxxxxxxxxxxxxxxxxxxxxx"
    unsigned int  u32TotalLen;                 //hash bin length
} GX_ImgHead_S;
#define CHECK_GX_HASH_MAGIC(pu8Section) (0 == memcmp(pu8Section, GX_HASH_MAGIC, GX_HASH_MAGIC_LEN))

static int vblk_dev_major = 0;
static struct verify_block_device verify_block_devices[VERIFY_BLOCK_COUNT];
static unsigned char verified[32*1024];
/******************************************************************************/
void set_verify_flag(unsigned int block_index)
{
	unsigned char tmp = 0;
	if(block_index/8 >= 32*1024)
		return;
	tmp = verified[block_index/8];
	tmp = tmp | (0x01 << (block_index%8));
	verified[block_index/8] = tmp;
}

int get_verify_flag(unsigned int block_index)
{
	unsigned char tmp;

	if(block_index/8 >= 32*1024)
		return 0;

	tmp = verified[block_index/8];
	return tmp & (0x01 << (block_index%8));
}

static char* dev_prefix_get(void)
{

	if (strstr(boot_command_line, "blkdevparts"))
		return MMCBLOCK;

	if (strstr(boot_command_line, "root_hash_dev=romblock"))
		return ROMBLOCK;

	return MTDBLOCK;
}

#ifndef MODULE
static int part_find_num(char *name)
{
	char * b;
	char * tmp;
	char * blkdevparts;
	char buffer[COMMAND_LINE_SIZE];
	int num = 0;

	blkdevparts = strstr(boot_command_line, "blkdevparts");
	if(!blkdevparts) {
		blkdevparts = strstr(boot_command_line, "mtdparts");
		num = -1; // mtdblock从0开始计算，mmcblk0p从1开始计算
	}

	memset(buffer,0,sizeof(buffer));
	memcpy(buffer,blkdevparts,sizeof(buffer));
	b = strim(buffer);
	while (b) {
		tmp = strsep(&b, ",");
		if (!tmp)
			continue;
		num++;
		if(strstr(tmp,name))
			break;
	}
	return num;
}
/******************************************************************************/
// root_hash_part=ROOT:HASH

static int __init early_verify_paramter(char *p)
{
	int i;
	char * b;
	char * tmp_buf;
	char buffer[128];
	char * devname;

	memset(buffer,0,sizeof(buffer));
	memcpy(buffer,p,sizeof(buffer));
	b = strim(buffer);

	for (i = 0; i < VERIFY_BLOCK_COUNT ; i++) {
		tmp_buf = strsep(&b, ":,");
		if(!tmp_buf)
			continue ;
		devname = dev_prefix_get();
		sprintf(verify_block_devices[i].blkdev_name,"%s%d", devname, part_find_num(tmp_buf));
		sprintf(verify_block_devices[i].blkname,"%s","rootfs");
		tmp_buf = strsep(&b, ":,");
		if(!tmp_buf)
			continue ;
		sprintf(verify_block_devices[i].hhfile_name,"%s%d", devname, part_find_num(tmp_buf));
	}
	return 0;
}
early_param("root_hash_part", early_verify_paramter);
#endif

/******************************************************************************/
int init_verify_file(struct verify_block_device *vbdev)
{
	HI_CAImgHead_S ca_header;

	memset(&ca_header,0,sizeof(ca_header));

	vbdev->blkdev_fp= filp_open(vbdev->hhfile_name, O_RDONLY | O_LARGEFILE, 0600);
	if (IS_ERR(vbdev->blkdev_fp)) {
		sys_mknod(SHA256_DEV_NAME, S_IFBLK|0600, new_encode_dev(name_to_dev_t(vbdev->hhfile_name)));
		vbdev->blkdev_fp= filp_open(SHA256_DEV_NAME, O_RDONLY | O_LARGEFILE, 0600);
		if (IS_ERR(vbdev->blkdev_fp)) {
			pr_err("can't open block device '%s'.\n", vbdev->hhfile_name);
			return -ENOENT;
		}
	}
	kernel_read(vbdev->blkdev_fp, 0, (char *)&ca_header, sizeof(ca_header));

	if (CHECK_GX_HASH_MAGIC((unsigned char *)&ca_header))
		vbdev->u32ImageOffset = sizeof(GX_ImgHead_S);
	if (CHECK_TAILSECTION((unsigned char *)&ca_header))
		vbdev->u32ImageOffset = ca_header.u32CodeOffset;
	else
		vbdev->u32ImageOffset = 0;

	return 0;
}
/******************************************************************************/

static int vblk_dev_open(struct block_device *bdev, fmode_t mode)
{
	int ret;
	loff_t size;
	struct file *blkdev_file;
	struct block_device *src_bdev;
	struct verify_block_device *vbdev = bdev->bd_disk->private_data;

	ret = init_verify_file(vbdev);
	if (ret < 0)
		return ret;
	mutex_lock(&vbdev->dev_mutex);
	if (vbdev->refcnt > 0)
		goto out_done;

	if (mode & FMODE_WRITE) {
		ret = -EPERM;
		goto out_unlock;
	}

	blkdev_file = filp_open(vbdev->blkdev_name, O_RDONLY | O_LARGEFILE, 0600);
	if (IS_ERR(blkdev_file)) {
		sys_mknod(ROOTFS_DEV_NAME, S_IFBLK|0600, new_encode_dev(name_to_dev_t(vbdev->blkdev_name)));
		blkdev_file = filp_open(ROOTFS_DEV_NAME, O_RDONLY | O_LARGEFILE, 0600);
		if (IS_ERR(blkdev_file)) {
			pr_err("can't open block device '%s'.\n", vbdev->blkdev_name);
			ret = -ENODEV;
			goto out_unlock;
		}
	}

	if (!(blkdev_file->f_mode & FMODE_READ)) {
		filp_close(blkdev_file, NULL);
		pr_err("block device '%s' is not readable.\n", vbdev->blkdev_name);
		ret = -EPERM;
		goto out_unlock;
	}
	vbdev->blkdev_file = blkdev_file;
	src_bdev = blkdev_file->private_data;
	size = i_size_read(blkdev_file->f_mapping->host) >> 9;
	set_capacity(vbdev->gd, size);

out_done:
	vbdev->refcnt++;
	mutex_unlock(&vbdev->dev_mutex);
	return 0;

out_unlock:
	mutex_unlock(&vbdev->dev_mutex);
	return ret;
}
/******************************************************************************/

static void vblk_dev_release(struct gendisk *gd, fmode_t mode)
{
	struct verify_block_device *vbdev = gd->private_data;

	mutex_lock(&vbdev->dev_mutex);
	vbdev->refcnt--;
	if (vbdev->refcnt == 0) {
		filp_close(vbdev->blkdev_file, NULL);
	}
	mutex_unlock(&vbdev->dev_mutex);
}
/******************************************************************************/

static int vblk_dev_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	geo->heads = 1;
	geo->cylinders = 1;
	geo->sectors = get_capacity(bdev->bd_disk);
	geo->start = 0;
	return 0;
}
/******************************************************************************/

static const struct block_device_operations vblk_dev_ops = {
	.owner = THIS_MODULE,
	.open = vblk_dev_open,
	.release = vblk_dev_release,
	.getgeo = vblk_dev_getgeo,
};
/******************************************************************************/

static void vblk_dev_request(struct request_queue *rq)
{
	struct verify_block_device *vbdev;
	struct request *req;

	vbdev = rq->queuedata;

	if (!vbdev)
		while ((req = blk_fetch_request(rq)) != NULL)
			__blk_end_request_all(req, -ENODEV);
	else
		queue_work(vbdev->wq, &vbdev->work);
}
/******************************************************************************/

bool verify_key_right(struct verify_block_device *vbdev  ,unsigned char *hash ,u64 off)
{
	int i;
	kernel_read(vbdev->blkdev_fp, vbdev->u32ImageOffset+(off<<5), vbdev->verify_tmp, HASG_LEN);
	if(memcmp(vbdev->verify_tmp,hash,HASG_LEN)) {
		pr_err("part_hash=  ");
		for(i=0;i<HASG_LEN;i++)
			pr_err("%.2x",vbdev->verify_tmp[i]);
		pr_err("\n");
		pr_err("part_root= ");
		for(i=0;i<HASG_LEN;i++)
			pr_err("%.2x",hash[i]);
		pr_err("\n");
		return false ;
	}
	else
		return true ;
}
/******************************************************************************/

static int sha256_string(char *buf,unsigned char *hash)
{
	int ret;
	struct crypto_shash *tfm;
	struct shash_desc *desc;
	tfm = crypto_alloc_shash("sha256", 0, CRYPTO_TFM_REQ_MAY_SLEEP);
	if (IS_ERR(tfm)) {
		pr_err("Failed to load transform for sha256 \n");
		return -1;
	}

	desc = kzalloc(sizeof(*desc) + crypto_shash_descsize(tfm) + 4, GFP_KERNEL);
	desc->tfm = tfm;
	desc->flags = 0;

	ret = crypto_shash_digest(desc, buf, 512, hash);

	crypto_free_shash(tfm);
	kfree(desc);

	return 0;
}
/******************************************************************************/

static int print_check_buffer(char* buff, int len)
{
	int i = 0;
	int x = 16;
	int j = 0;
	int y = len / x;
	printk("== VERIFY == check_buffer:\n");
	for (j = 0; j < y; j++) {
		for (i = 0; i < x; i++) {
			if (i == 8)
				printk(" ");
			printk("%02x ", buff[i + j*16]);
		}
		printk("\n");
	}
	return 0;
}

static int vblk_read(struct verify_block_device *vbdev, char *buffer,
		     sector_t sec, int len)
{
	int blk_index = 0;
	int blk_count = len >> 9;
	char* check_buffer = NULL;
	u64 pos = sec << 9;
	kernel_read(vbdev->blkdev_file, pos, buffer, len);
	if(len == 0)
		return 0;
#if 1
	for(blk_index = 0; blk_index < blk_count; blk_index++) {
		if(get_verify_flag(sec + blk_index))
			continue;

		check_buffer = buffer + blk_index * VERBLOCK_SIZE;
		sha256_string(check_buffer, vbdev->hash_tmp);
		if(false == verify_key_right(vbdev, vbdev->hash_tmp, (sec + blk_index))) {
			pr_err("block %d, addr: 0x%x, verify hash error , system reboot ... !\n", (int)(sec + blk_index), (int)(sec + blk_index) * VERBLOCK_SIZE);
			print_check_buffer(check_buffer, VERBLOCK_SIZE);
			kernel_restart(NULL);
			return -1;
		}

		set_verify_flag(sec + blk_index);
	}
#endif
	return 0;
}
/******************************************************************************/

static int do_verify_block_request(struct verify_block_device *vbdev,
				   struct request *req)
{
	int len, ret;
	sector_t sec;

	if (req->cmd_type != REQ_TYPE_FS)
		return -EIO;

	if (blk_rq_pos(req) + blk_rq_cur_sectors(req) >
	    get_capacity(req->rq_disk))
		return -EIO;

	if (rq_data_dir(req) != READ)
		return -ENOSYS;

	sec = blk_rq_pos(req);
	len = blk_rq_cur_bytes(req);

	mutex_lock(&vbdev->dev_mutex);
	ret = vblk_read(vbdev, bio_data(req->bio), sec, len);
	mutex_unlock(&vbdev->dev_mutex);

	return ret;
}
/******************************************************************************/

static void vblk_dev_do_work(struct work_struct *work)
{
	struct verify_block_device *vbdev =
		container_of(work, struct verify_block_device, work);
	struct request_queue *rq = vbdev->rq;
	struct request *req;
	int res;

	spin_lock_irq(rq->queue_lock);

	req = blk_fetch_request(rq);
	while (req) {
		spin_unlock_irq(rq->queue_lock);
		res = do_verify_block_request(vbdev, req);
		spin_lock_irq(rq->queue_lock);
		if (!__blk_end_request_cur(req, res))
			req = blk_fetch_request(rq);
	}

	spin_unlock_irq(rq->queue_lock);
}
/******************************************************************************/

int vblk_dev_create(struct verify_block_device *vbdev, int idx)
{
	int ret;
	struct gendisk *gd;

	mutex_init(&vbdev->dev_mutex);

	gd = alloc_disk(1);
	if (!gd) {
		pr_err("alloc_disk failed");
		return -ENODEV;
	}

	gd->fops = &vblk_dev_ops;
	gd->major = vblk_dev_major;
	gd->first_minor = idx;
	gd->private_data = vbdev;

	set_capacity(gd, VERBLOCK_SIZE);

	sprintf(gd->disk_name, "verify_%s",vbdev->blkname);
	vbdev->gd = gd;

	spin_lock_init(&vbdev->queue_lock);
	vbdev->rq = blk_init_queue(vblk_dev_request, &vbdev->queue_lock);
	if (!vbdev->rq) {
		pr_err("blk_init_queue failed.\n");
		ret = -ENODEV;
		goto out_put_disk;
	}

	vbdev->rq->queuedata = vbdev;
	vbdev->gd->queue = vbdev->rq;

	vbdev->wq = alloc_workqueue("%s", 0, 0, gd->disk_name);
	if (!vbdev->wq) {
		pr_err("alloc_workqueue failed.\n");
		ret = -ENOMEM;
		goto out_free_queue;
	}
	INIT_WORK(&vbdev->work, vblk_dev_do_work);

	add_disk(vbdev->gd);

	pr_info("created verify block %s from %s and hash from %s\n",
		gd->disk_name, vbdev->blkdev_name, vbdev->hhfile_name);

	return 0;

out_free_queue:
	blk_cleanup_queue(vbdev->rq);
out_put_disk:
	put_disk(vbdev->gd);

	vbdev->gd = NULL;

	return ret;
}
/******************************************************************************/

static void verify_block_remove_all(void)
{
	int i;

	for (i = 0; i < VERIFY_BLOCK_COUNT; i++) {
		struct verify_block_device *vbdev = &verify_block_devices[i];

		if (!vbdev->blkdev_name)
			break;

		destroy_workqueue(vbdev->wq);

		del_gendisk(vbdev->gd);
		blk_cleanup_queue(vbdev->rq);

		pr_info("%s released\n", vbdev->gd->disk_name);

		put_disk(vbdev->gd);
		vfree(vbdev->blkdev_name);
		filp_close(vbdev->blkdev_fp,NULL);
	}
}
/******************************************************************************/

static int __init vblk_device_init(void)
{
	int i, ret;

	vblk_dev_major = register_blkdev(0, VERIFY_BLOCK_NAME);
	if (vblk_dev_major < 0)
		return -ENODEV;

	for (i = 0; i < VERIFY_BLOCK_COUNT; i++) {
		struct verify_block_device *vbdev = &verify_block_devices[i];

		if (0 == strlen(vbdev->blkdev_name))
			break;

		ret = vblk_dev_create(vbdev, i);
		if (ret) {
			pr_err("can't create '%s' verify block, err=%d\n",
			       vbdev->blkdev_name, ret);
			goto err_uncreate;
		}
	}

	return 0;

err_uncreate:
	unregister_blkdev(vblk_dev_major, VERIFY_BLOCK_NAME);
	verify_block_remove_all();

	return ret;
}
/******************************************************************************/

static void __exit vblk_device_exit(void)
{
	verify_block_remove_all();
	unregister_blkdev(vblk_dev_major, VERIFY_BLOCK_NAME);
}
/******************************************************************************/

module_init(vblk_device_init);
module_exit(vblk_device_exit);
MODULE_LICENSE("GPL");
