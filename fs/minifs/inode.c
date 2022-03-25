#include <linux/module.h>
#include <linux/fs.h>
#include <linux/swap.h>
#include <linux/pagemap.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/uio.h>
#include <linux/dcache.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/mtd/super.h>
#include <linux/statfs.h>
#include <asm/uaccess.h>
#include "linux/namei.h"
#include "minifs.h"

static struct minifs_device * minifs_dev;

struct minifs_inode_info {
	minifs_Object *object;
	struct inode vfs_inode;
	struct mutex sem;
};

static inline struct minifs_inode_info *MINIFS_I(struct inode *inode)
{
	return container_of(inode, struct minifs_inode_info, vfs_inode);
}

static int minifs_readpage_locked(struct file *f, struct page *pg)
{
	/* Lifted from jffs2 */
	int retval = 0;
	loff_t size = 0;
	loff_t offset = 0;
	loff_t read_len = 0;
	minifs_Handle *h = NULL;
	unsigned char *pg_buf = NULL;
	minifs_Object *object = NULL;
	struct inode *inode = pg->mapping->host;

	object = MINIFS_I(inode)->object;
	if (object == NULL) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		return -ENOENT;
	}

	h = object->handle;
	if (h == NULL) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		return -ENOENT;
	}

	size = i_size_read(inode);

	offset = page_offset(pg);
	if (offset > size) {
		// read hole region, for simple only think of success
		retval = 0;
		goto exit;
	}

	read_len = ((size - offset) > PAGE_SIZE) ? (PAGE_SIZE) : (size - offset);

	BUG_ON(!PageLocked(pg));

	pg_buf = kmap(pg);
	/* FIXME: Can kmap fail? */
	if (pg_buf == NULL) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	mutex_lock(&minifs_dev->minifs_mutex);
	retval = minifs_read(h, pg_buf, (size_t)read_len, offset);
	mutex_unlock(&minifs_dev->minifs_mutex);
	if (retval >= 0)
		retval = 0;

exit:
	if (retval) {
		ClearPageUptodate(pg);
		SetPageError(pg);
	} else {
		SetPageUptodate(pg);
		ClearPageError(pg);
	}

	flush_dcache_page(pg);
	kunmap(pg);

	return retval;
}

static int minifs_readpage_unlock(struct file *f, struct page *pg)
{
	int retval = minifs_readpage_locked(f, pg);

	unlock_page(pg);
	return retval;
}

static int minifs_readpage(struct file *f, struct page *pg)
{
	int retval = 0;
	struct minifs_inode_info *minifs_i = MINIFS_I(pg->mapping->host);

	mutex_lock(&minifs_i->sem);
	retval =  minifs_readpage_unlock(f, pg);
	mutex_unlock(&minifs_i->sem);

	return retval;
}

/*
 * Prepare for write in mapping->apos.
 */
static int minifs_write_begin(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned flags,
		struct page **pagep, void **fsdata)
{
	int retval = 0;
	loff_t offset = 0;
	loff_t size = 0;
	pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	struct minifs_inode_info *minifs_i = MINIFS_I(mapping->host);

	mutex_lock(&minifs_i->sem);

	*pagep = grab_cache_page_write_begin(mapping, index, flags);
	if (!*pagep) {
		printk("minifs error %s %d\n", __func__, __LINE__);
		mutex_unlock(&minifs_i->sem);
		return -ENOMEM;
	}

	size = i_size_read(mapping->host);
	offset = page_offset(*pagep);
	if ((offset < size) && (!PageUptodate(*pagep)))
		retval = minifs_readpage_locked(file, *pagep);

	if (retval) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		retval = -EIO;
		goto error;
	}

	mutex_unlock(&minifs_i->sem);
	return 0;

error:
	unlock_page(*pagep);
	page_cache_release(*pagep);
	mutex_unlock(&minifs_i->sem);
	return retval;
}

/*
 * Do real write.
 */
static int minifs_write_end(struct file *file, struct address_space *mapping,
		loff_t pos, unsigned len, unsigned copied,
		struct page *page, void *fsdata)
{
	int retval = 0;
	void *kva = NULL;
	void *addr = NULL;
	minifs_Handle *h = NULL;
	struct inode *inode = NULL;
	minifs_Object *object = NULL;
	uint32_t offset_into_page = 0;
	struct minifs_inode_info *minifs_i = NULL;

	inode = page->mapping->host;
	minifs_i = MINIFS_I(inode);

	mutex_lock(&minifs_i->sem);

	offset_into_page = pos & (PAGE_CACHE_SIZE - 1);
	object = MINIFS_I(inode)->object;
	if (object->handle == NULL) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		retval = -ENOENT;
		goto exit;
	} else
		h = object->handle;

	if (offset_into_page + copied > PAGE_SIZE) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		retval = -EIO;
		goto exit;
	}

	kva = kmap(page);
	addr = kva + offset_into_page;
	mutex_lock(&minifs_dev->minifs_mutex);
	retval = minifs_write(h, addr, copied, (off_t)pos);
	mutex_unlock(&minifs_dev->minifs_mutex);
	kunmap(page);

	if (retval != copied) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		SetPageError(page);
		retval = -EIO;
		goto exit;
	}
	inode->i_mtime = inode->i_atime = CURRENT_TIME;

	if ((pos + copied) > inode->i_size) {
		inode->i_size = pos + copied;
		inode->i_ctime = CURRENT_TIME;
	}


exit:
	unlock_page(page);
	page_cache_release(page);
	mutex_unlock(&minifs_i->sem);
	return retval;
}

/*
 * sync file
 */
static int minifs_sync_file(struct file *file, loff_t start, loff_t end, int datasync)
{
	minifs_Handle *h = NULL;
	minifs_Object *obj = NULL;
	struct minifs_inode_info *minifs_i = MINIFS_I(file->f_path.dentry->d_inode);

	mutex_lock(&minifs_i->sem);

	if (file->f_path.dentry)
		obj = MINIFS_I(file->f_path.dentry->d_inode)->object;

	if (obj) {
		h = obj->handle;
		if(h) {
			mutex_lock(&minifs_dev->minifs_mutex);
			minifs_sync(h);
			mutex_unlock(&minifs_dev->minifs_mutex);
		} else
			printk("minifs error: %s %d\n", __func__, __LINE__);
	}

	mutex_unlock(&minifs_i->sem);
	return 0;
}

#if 0
/* writepage stolen from minifs, minifs's inspired by/stolen from smbfs */

static int minifs_writepage(struct page *page, struct writeback_control *wbc)
{
	struct address_space *mapping = page->mapping;
	struct inode *inode;
	unsigned long end_index;
	char *buffer;
	int n_written = 0;
	unsigned n_bytes;
        minifs_Handle *h;
	loff_t i_size;

	if (!mapping)
		BUG();
	inode = mapping->host;
	if (!inode)
		BUG();

        if (!MINIFS_I(inode)->object)
                BUG();

        h = MINIFS_I(inode)->object->handle;
        if (!h)
                BUG();

	i_size = i_size_read(inode);


	end_index = i_size >> PAGE_CACHE_SHIFT;

	if (page->index < end_index) {
		n_bytes = PAGE_CACHE_SIZE;
                printk("%s-%s offset: %d, nbytes: %d.\n", __func__, h->obj->Header.name, page->index << PAGE_CACHE_SHIFT, n_bytes);
	} else {
		n_bytes = i_size & (PAGE_CACHE_SIZE - 1);
                printk("%s-%s offset: %d, nbytes: %d.\n", __func__, h->obj->Header.name, page->index << PAGE_CACHE_SHIFT, n_bytes);

		if (page->index > end_index || !n_bytes) {
			printk( "minifs_writepage at %08x, inode size = %08x!!!",
				(unsigned)(page->index << PAGE_CACHE_SHIFT),
				(unsigned)inode->i_size);
			printk("                -> don't care!!");

			zero_user_segment(page, 0, PAGE_CACHE_SIZE);
			set_page_writeback(page);
			unlock_page(page);
			end_page_writeback(page);
			return 0;
		}
	}

	if (n_bytes != PAGE_CACHE_SIZE)
		zero_user_segment(page, n_bytes, PAGE_CACHE_SIZE);

	get_page(page);

	buffer = kmap(page);

        //n_written = minifs_wr_file(obj, buffer, page->index << PAGE_CACHE_SHIFT, n_bytes, 0);
        n_written = minifs_write(h, buffer, n_bytes, page->index << PAGE_CACHE_SHIFT);

	kunmap(page);
	set_page_writeback(page);
	unlock_page(page);
	end_page_writeback(page);
	put_page(page);

	return (n_written == n_bytes) ? 0 : -ENOSPC;
}
#endif

/*
 * use page mapping, we can map a file to memory.
 */
const struct address_space_operations minifs_aops = {
	.readpage         = minifs_readpage,
    //.writepage      = minifs_writepage,
	.write_begin	  = minifs_write_begin,
	.write_end	      = minifs_write_end,
	.set_page_dirty   = __set_page_dirty_no_writeback,
};

/*
 * seems only be called close(file).
 */
static int minifs_flush (struct file *file, fl_owner_t id)
{
	minifs_Handle *h = NULL;
	minifs_Object *obj = NULL;
	struct minifs_inode_info *minifs_i = MINIFS_I(file->f_path.dentry->d_inode);

	mutex_lock(&minifs_i->sem);

	if (file->f_path.dentry)
		obj = MINIFS_I(file->f_path.dentry->d_inode)->object;

	if (obj) {
		h = obj->handle;
		if(h) {
			mutex_lock(&minifs_dev->minifs_mutex);
			minifs_sync(h);
			mutex_unlock(&minifs_dev->minifs_mutex);
		} else
			printk("minifs error: %s %d\n", __func__, __LINE__);
	}
	mutex_unlock(&minifs_i->sem);

	return 0;
}

/*
 * file operations.
 */
const struct file_operations minifs_file_operations = {
	.read_iter		= generic_file_read_iter,
	.write_iter		= generic_file_write_iter,
	.mmap			= generic_file_mmap,
	.fsync			= minifs_sync_file,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		    = generic_file_llseek,
	.flush		    = minifs_flush,
};

static void minifs_do_truncate(struct inode *i)
{
	struct minifs_inode_info *minifs_i = MINIFS_I(i);

	mutex_lock(&minifs_i->sem);
	mutex_lock(&minifs_dev->minifs_mutex);
	minifs_truncate(MINIFS_I(i)->object->handle, i->i_size);
	mutex_unlock(&minifs_dev->minifs_mutex);
	mutex_unlock(&minifs_i->sem);
}

int minifs_setattr(struct dentry *dentry, struct iattr *iattr)
{
        struct inode *inode = d_inode(dentry);
        int error;

        error = setattr_prepare(dentry, iattr);
        if (error)
                return error;

        if (iattr->ia_valid & ATTR_SIZE)
                truncate_setsize(inode, iattr->ia_size);

        setattr_copy(inode, iattr);

        if (iattr->ia_valid & ATTR_SIZE)
		minifs_do_truncate(inode);

        mark_inode_dirty(inode);
        return 0;
}

/*
 * file inode operations, we mostly use default func.
 */
const struct inode_operations minifs_file_inode_operations = {
	.setattr	= minifs_setattr,
	.getattr	= simple_getattr,
//	.truncate   = minifs_do_truncate,
};

/*
 * Don't care it.
 */
#define MINIFS_MAGIC	0x858458f7

static const struct super_operations minifs_super_ops;
static const struct inode_operations minifs_dir_inode_operations;
static const struct file_operations minifs_dir_operations;

static int minifs_type(int type)
{
	switch(type) {
	case MINIFS_OBJECT_TYPE_FILE:
		return DT_REG;
	case MINIFS_OBJECT_TYPE_DIRECTORY:
		return DT_DIR;
	case MINIFS_OBJECT_TYPE_SYMLINK:
		return DT_LNK;
	default:
		return DT_UNKNOWN;
	}
}

static const char *minifs_get_link(struct dentry *dentry, struct inode *inode, struct delayed_call *done)
{
	minifs_Object *obj = NULL;
	struct minifs_inode_info *minifs_i = MINIFS_I(dentry->d_inode);
	mutex_lock(&minifs_i->sem);

	obj = MINIFS_I(dentry->d_inode)->object;
	if (obj == NULL) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		goto exit;
	}

	if (obj->Header.type != MINIFS_OBJECT_TYPE_SYMLINK) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		goto exit;
	}

	return (char *)obj->Header.alias;
exit:
	mutex_unlock(&minifs_i->sem);
	return ERR_PTR(-EACCES);

}

const struct inode_operations minifs_symlink_inode_operations = {
	.readlink	= generic_readlink,
	.get_link	= minifs_get_link,
};

/*
 * common inode create.
 * the minifs object must be create already.
 */
static struct inode *minifs_get_inode(struct super_block *sb, char *fsname)
{
	int type = 0;
	unsigned int mode = 0;
	struct inode *i = NULL;
	minifs_Handle *h = NULL;
	minifs_Object *object = NULL;

	object = minifs_FindObjectByName(minifs_dev, fsname);
	if (!object)
		return NULL;

	type = minifs_type(object->Header.type);

	i = iget_locked(sb, (unsigned long)object);
	if (!i) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		return NULL;
	}

	if (!(i->i_state & I_NEW))
		return i;

	MINIFS_I(i)->object = object;

	if (type == DT_REG) {
		h = minifs_open(minifs_dev, fsname, O_RDWR, 0644);
		if (h == NULL) {
			printk("minifs error: %s %d\n", __func__, __LINE__);
			goto error;
		}
		minifs_sync(h);
		i->i_size = minifs_size(h);
	}

	i->i_mtime.tv_sec = i->i_atime.tv_sec = i->i_ctime.tv_sec = object->Header.yst_mtime;
	i->i_mtime.tv_nsec = i->i_atime.tv_nsec = i->i_ctime.tv_nsec = 0;
	i_uid_write(i, 0);
	i_gid_write(i, 0);
	mode = object->Header.yst_mode;

	switch (type) {
	case DT_DIR:
		i->i_op   = &minifs_dir_inode_operations;
		i->i_fop  = &minifs_dir_operations;
		i->i_mode = S_IFDIR + (mode & 0777);
		i->i_size = 0;
		i->__i_nlink = 2;
		break;
	case DT_REG:
		i->i_fop  = &minifs_file_operations;
		i->i_op   = &minifs_file_inode_operations;
		i->i_mode = S_IFREG + (mode & 0777);
		i->i_data.a_ops = &minifs_aops;
		i->__i_nlink = 1;
		break;
	case DT_LNK:
		i->i_op   = &minifs_symlink_inode_operations;
		i->i_mode = S_IFLNK + (mode | 0777);
		i->i_size = strlen(object->Header.alias);
		i->__i_nlink = 1;
		break;
	default:
		printk("minifs warning: %s %d\n", __func__, __LINE__);
		init_special_inode(i, object->objectId, MKDEV(0x234>>16,0x234&0xffff));
	}

	unlock_new_inode(i);
	return i;
error:
	unlock_new_inode(i);
	iput(i);
	return NULL;
}

/*
 * setup the whole path name.
 */
static int minifs_dentry_path(char *name, unsigned int *offset, struct dentry *dentry)
{
	if (dentry->d_name.len > MINIFS_MAX_NAME_LENGTH)
		return -ENAMETOOLONG;

	if (dentry->d_parent != dentry) {
		int res = minifs_dentry_path(name, offset, dentry->d_parent);
		if (res < 0)
			return res;
	} else {
		name[0] = '/';
		*offset = 1;
		return 0;
	}

	memcpy(name+(*offset), dentry->d_name.name, dentry->d_name.len);
	name[(*offset) + dentry->d_name.len] = '/';
	*offset = *offset + dentry->d_name.len + 1;

	return 0;
}

/*
 * fill all name in dir dentry.
 */
static int minifs_do_readdir(struct file *filp, struct dir_context *ctx)
{
	ino_t ino = 0;
	int stored = 0;
	unsigned int len = 0;
	char fsname[256] = {0};
	struct list_head *i, *n = NULL;
	minifs_Object *parent, *object = NULL;
	struct inode *inode = filp->f_path.dentry->d_inode;
	struct minifs_inode_info *minifs_i = MINIFS_I(inode);

	mutex_lock(&minifs_i->sem);


	if (minifs_dentry_path(fsname, &len, filp->f_path.dentry) < 0)
		goto exit;
	fsname [len - 1] = '\0';
	parent = minifs_FindObjectByName(minifs_dev, fsname);

	if (!parent) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		goto exit;
	}

	if (parent->Header.type != MINIFS_OBJECT_TYPE_DIRECTORY) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		goto exit;
	}

	list_for_each_safe(i, n, &parent->variant.dir_variant.children) {
		if (stored < ctx->pos) {
			stored++;
			continue;
		}

		object = list_entry(i, minifs_Object, siblings);
		ino = (ino_t) object;
		if (!dir_emit(ctx, object->Header.name, strlen(object->Header.name), ino, minifs_type(object->Header.type)))
			goto noerror;

		stored++;
		ctx->pos++;
	}

noerror:
	inode->i_atime = CURRENT_TIME;

exit:
	mutex_unlock(&minifs_i->sem);
	return stored;
}

/*
 * Lookup the data. This is trivial - if the dentry didn't already
 * exist, we know it is negative.  Set d_op to delete negative dentries.
 */
static struct dentry *minifs_do_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	long res = 0;
	unsigned int len = 0;
	char fsname[256] = {0};
	struct inode *i = NULL;
	struct minifs_inode_info *minifs_i = MINIFS_I(dir);

	mutex_lock(&minifs_i->sem);

	if (dentry->d_name.name[0] == '.')
		goto exit;

	res = minifs_dentry_path(fsname, &len, dentry);
	if (res < 0)
		goto exit;
	fsname [len - 1] = '\0';

	i = minifs_get_inode(dir->i_sb, fsname);
	if (i == NULL)
		goto exit;

	d_add(dentry, i);
	res = 0;

exit:
	mutex_unlock(&minifs_i->sem);
	return ERR_PTR(res);
}

/*
 * Setup a file.
 */
static int minifs_do_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	int error = 0;
	unsigned int len = 0;
	char fsname[256] = {0};
	struct inode *i = NULL;
	minifs_Object *object = NULL;
	struct minifs_inode_info *minifs_i = MINIFS_I(dir);

	mutex_lock(&minifs_i->sem);

	error = minifs_dentry_path(fsname, &len, dentry);
	if (error < 0)
		goto exit;
	fsname [len - 1] = '\0';

	object = minifs_FindObjectByName(minifs_dev, fsname);
	if (object) {
		error = -ENOSPC;
		printk("minifs error: %s %d\n", __func__, __LINE__);
		goto exit;
	}

	mutex_lock(&minifs_dev->minifs_mutex);
	object = minifs_CreateNewObjectByName(minifs_dev, fsname, MINIFS_OBJECT_TYPE_FILE,
			0, 0, mode, NULL, NULL);
	mutex_unlock(&minifs_dev->minifs_mutex);
	if (!object) {
		error = -ENOSPC;
		printk("minifs error: %s %d\n", __func__, __LINE__);
		goto exit;
	}

	i = minifs_get_inode(dir->i_sb, fsname);
	if (!i) {
		error = -ENOSPC;
		printk("minifs error: %s %d\n", __func__, __LINE__);
		goto exit;
	} else {
		if (dir->i_mode & S_ISGID) {
			i->i_gid = dir->i_gid;
			if (S_ISDIR(mode))
				i->i_mode |= S_ISGID;
		}
		d_instantiate(dentry, i);
		error = 0;
		dir->i_atime = dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}

exit:
	mutex_unlock(&minifs_i->sem);
	return error;
}

/*
 * Mkdir a dir.
 */
static int minifs_do_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int retval = 0;
	unsigned int len = 0;
	char fsname[256] = {0};
	struct inode *i = NULL;
	struct minifs_inode_info *minifs_i = MINIFS_I(dir);

	mutex_lock(&minifs_i->sem);

	retval = minifs_dentry_path(fsname, &len, dentry);
	if (retval < 0)
		goto exit;
	fsname [len - 1] = '\0';

	mutex_lock(&minifs_dev->minifs_mutex);
	retval = minifs_mkdir(minifs_dev, fsname, mode);
	mutex_unlock(&minifs_dev->minifs_mutex);
	if (retval != 0) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		retval = -EIO;
		goto exit;
	}

	i = minifs_get_inode(dir->i_sb, fsname);
	if (!i)  {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		retval = -ENOSPC;
		goto exit;
	} else {
		if (dir->i_mode & S_ISGID) {
			i->i_gid = dir->i_gid;
			if (S_ISDIR(mode))
				i->i_mode |= S_ISGID;
		}
		d_instantiate(dentry, i);
		retval = 0;
		dir->i_atime = dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}

	if (!retval)
		inc_nlink(dir);

exit:
	mutex_unlock(&minifs_i->sem);
	return retval;
}

/*
 * remove a empty dir.
 */
static int minifs_rmdir_i(struct inode *dir, struct dentry *dentry)
{
	int ret = 0;
	unsigned int len = 0;
	char fsname[256] = {0};
	minifs_Object *obj = NULL;
	struct list_head *tmp = NULL;
	struct inode *inode = dentry->d_inode;
	minifs_Object *object = MINIFS_I(inode)->object;

	if (object->Header.type != MINIFS_OBJECT_TYPE_DIRECTORY) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		ret = -ENOTDIR;
		goto exit;
	}
	// is a empty dir ?
	list_for_each(tmp, &object->variant.dir_variant.children) {
		obj = list_entry(tmp, minifs_Object, siblings);
		if (obj) {
			printk("minifs error: %s %d\n", __func__, __LINE__);
			ret = -ENOTEMPTY;
			goto exit;
		}
	}

	ret = minifs_dentry_path(fsname, &len, dentry);
	if (ret < 0)
		goto exit;
	fsname [len - 1] = '\0';

	mutex_lock(&minifs_dev->minifs_mutex);
	ret = minifs_rmdir(minifs_dev, fsname);
	mutex_unlock(&minifs_dev->minifs_mutex);
	if (ret != 1) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		ret = -EAGAIN;
		goto exit;
	} else
		ret = 0;

	dir->i_atime = dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	drop_nlink(dir);
	clear_nlink(inode);
exit:
	return ret;
}

static int minifs_do_rmdir(struct inode *dir, struct dentry *dentry)
{
	int retval = 0;
	struct minifs_inode_info *minifs_i = MINIFS_I(dir);

	mutex_lock(&minifs_i->sem);
	retval = minifs_rmdir_i(dir, dentry);
	mutex_unlock(&minifs_i->sem);
	return retval;
}

/*
 * remove the file.
 */
static int minifs_unlink_i(struct inode *dir, struct dentry *dentry)
{
	int retval = 0;
	unsigned int len = 0;
	char fsname[256] = {0};
	struct inode *inode = dentry->d_inode;
	struct minifs_inode_info * minifs_i = MINIFS_I(inode);

	if (minifs_i->object) {
		if (minifs_i->object->handle)
			if (minifs_close(minifs_i->object->handle) < 0) {
				printk("minifs error: %s %d\n", __func__, __LINE__);
				retval = -EAGAIN;
				goto exit;
			}
	} else {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		retval = -ENOENT;
		goto exit;
	}

	retval = minifs_dentry_path(fsname, &len, dentry);
	if (retval < 0)
		goto exit;
	fsname [len - 1] = '\0';

	mutex_lock(&minifs_dev->minifs_mutex);
	retval = minifs_rmfile(minifs_dev, fsname);
	mutex_unlock(&minifs_dev->minifs_mutex);
	if(retval != 1) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		retval = -EAGAIN;
		goto exit;
	} else
		retval = 0;

	dir->i_atime = dir->i_mtime = dir->i_ctime = CURRENT_TIME;
exit:
	return retval;
}

static int minifs_do_unlink(struct inode *dir, struct dentry *dentry)
{
	int retval = 0;
	struct minifs_inode_info *minifs_i = MINIFS_I(dir);

	mutex_lock(&minifs_i->sem);
	retval = minifs_unlink_i(dir, dentry);
	mutex_unlock(&minifs_i->sem);
	return retval;
}

/*
 * just rename.
 */
static int minifs_do_rename(struct inode *old_dir, struct dentry *old_dentry,
		struct inode *new_dir, struct dentry *new_dentry, unsigned int flags)
{
	int retval = 0;
	unsigned int len = 0;
	char fsname_old[256] = {0};
	char fsname_new[256] = {0};
	struct inode *old_inode = old_dentry->d_inode;
	struct inode *new_inode = new_dentry->d_inode;
	int old_is_dirs = S_ISDIR(old_inode->i_mode);
	struct minifs_inode_info *minifs_i = MINIFS_I(old_dir);

	if (flags & ~RENAME_NOREPLACE)
		return -EINVAL;

	mutex_lock(&minifs_i->sem);

	retval = minifs_dentry_path(fsname_old, &len, old_dentry);
	if (retval < 0)
		goto exit;
	fsname_old [len - 1] = '\0';

	retval = minifs_dentry_path(fsname_new, &len, new_dentry);
	if (retval < 0)
		goto exit;
	fsname_new [len - 1] = '\0';

	/* The VFS will check for us and prevent trying to rename a
	 * file over a directory and vice versa, but if it's a directory,
	 * the VFS can't check whether the new_dentry is empty. The filesystem
	 * needs to do that for itself.
	 */

	if (new_inode) {
		minifs_Object *new_object = NULL;
		int new_is_dirs = S_ISDIR(new_inode->i_mode);
		if (old_is_dirs ^ new_is_dirs) {
			printk("minifs error: %s %d\n", __func__, __LINE__);
			retval = -EBADE;
			goto exit;
		}

		new_object = minifs_FindObjectByName(minifs_dev, fsname_new);
		if (!new_object) {
			printk("minifs error: %s %d\n", __func__, __LINE__);
			retval = -EBADE;
			goto exit;
		}
		if (new_is_dirs) {
			minifs_Object *obj = NULL;
			struct list_head *tmp = NULL;
			list_for_each(tmp, &new_object->variant.dir_variant.children) {
				obj = list_entry(tmp, minifs_Object, siblings);
				if (obj) {
					printk("minifs error: %s %d\n", __func__, __LINE__);
					retval = -ENOTEMPTY;
					goto exit;
				}
			}
		}
		if (new_object->Header.type == MINIFS_OBJECT_TYPE_DIRECTORY)
			retval = minifs_rmdir_i(new_dir, new_dentry);
		else
			retval = minifs_unlink_i(new_dir, new_dentry);
		if (retval < 0) {
			printk("minifs error: %s %d\n", __func__, __LINE__);
			goto exit;
		}
	}

	mutex_lock(&minifs_dev->minifs_mutex);
	retval = minifs_rename(minifs_dev, fsname_old, fsname_new);
	mutex_unlock(&minifs_dev->minifs_mutex);
	if (retval < 0) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		goto exit;
	}

	if (old_is_dirs) {
		drop_nlink(old_dir);
		if (!new_dentry->d_inode)
			inc_nlink(new_dir);
	}

	old_dir->i_atime = old_dir->i_ctime = old_dir->i_mtime =
		new_dir->i_atime = new_dir->i_ctime = new_dir->i_mtime = CURRENT_TIME;
exit:
	mutex_unlock(&minifs_i->sem);
	return retval;
}

/*
 * Create a symlink.
 */
static int minifs_do_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
	unsigned int len = 0;
	struct inode *inode;
	minifs_Object *object;
	char fsname[256];
	char fsname_old[256];
	int error = -ENOSPC;
	struct minifs_inode_info *minifs_i = MINIFS_I(dir);

	mutex_lock(&minifs_i->sem);

	error = minifs_dentry_path(fsname, &len, dentry);
	if (error < 0)
		goto error_free;
	fsname [len - 1] = '\0';

	object = minifs_FindObjectByName(minifs_dev, fsname);
	if (object) {
		printk("object already exist %s, %d: %s.\n", __func__, __LINE__, fsname);
		goto error_free;
	}

	len = strlen(symname);
	memcpy(fsname_old, symname, len);
	fsname_old[len] = '\0';

	mutex_lock(&minifs_dev->minifs_mutex);
	object = minifs_CreateNewObjectByName(minifs_dev, fsname, MINIFS_OBJECT_TYPE_SYMLINK,
			0, 0, 0644, NULL, fsname_old);
	mutex_unlock(&minifs_dev->minifs_mutex);
	if (object == NULL) {
		printk("%s: minifs_CreateNewObjectByName failed.\n", __func__);
		goto error_free;
	}

	inode = minifs_get_inode(dir->i_sb, fsname);
	if (inode) {
		if (dir->i_mode & S_ISGID)
			inode->i_gid = dir->i_gid;
		d_instantiate(dentry, inode);
		dir->i_mtime = dir->i_ctime = CURRENT_TIME;
		error = 0;
	}

error_free:
	mutex_unlock(&minifs_i->sem);
	return error;
}

/*
 * dir inode operations.
 */
static const struct inode_operations minifs_dir_inode_operations = {
	.create     = minifs_do_create,
	.lookup		= minifs_do_lookup,
	.unlink		= minifs_do_unlink,
	.symlink	= minifs_do_symlink,
	.mkdir		= minifs_do_mkdir,
	.rmdir		= minifs_do_rmdir,
	.rename		= minifs_do_rename,
};

/*
 * dir file operations.
 */
static const struct file_operations minifs_dir_operations = {
	.open		= dcache_dir_open,
	.release	= dcache_dir_close,
	.llseek		= dcache_dir_lseek,
	.read		= generic_read_dir,
	.iterate_shared = minifs_do_readdir,
	.fsync		= noop_fsync,
};

/*
 * mempool for minifs_inode.
 */
static struct kmem_cache * minifs_inode_cachep;

/*
 * Create minifs_inode.
 */
static struct inode *minifs_alloc_inode(struct super_block *sb)
{
	struct minifs_inode_info *mi;
	mi = kmem_cache_alloc(minifs_inode_cachep, GFP_KERNEL);
	if (!mi)
		return NULL;
	return &mi->vfs_inode;
}

/*
 * Destroy the minifs_inode.
 */
static void minifs_destroy_inode(struct inode *inode)
{
	struct minifs_inode_info * minifs_i = MINIFS_I(inode);

	if (minifs_i->object) {
		if(minifs_i->object->handle) {
			if(minifs_close(minifs_i->object->handle) < 0)
				printk("minifs error: %s %d\n", __func__, __LINE__);
		}
	}
	kmem_cache_free(minifs_inode_cachep, minifs_i);
}

/*
 * just simple init.
 */
static void minifs_init_once(void *foo)
{
	struct minifs_inode_info *i = foo;

	mutex_init(&i->sem);
	inode_init_once(&i->vfs_inode);
}

/*
 * Setup inode mempool.
 */
static int init_inodecache(void)
{
	minifs_inode_cachep = kmem_cache_create("minifs_inode_cache",
			sizeof(struct minifs_inode_info),
			0, (SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD),
			minifs_init_once);
	if (minifs_inode_cachep == NULL)
		return -ENOMEM;
	return 0;
}

/*
 * Destory inode mempool.
 */
static void destroy_inodecache(void)
{
	kmem_cache_destroy(minifs_inode_cachep);
}

static int minifs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	mutex_lock(&minifs_dev->minifs_mutex);
	buf->f_type = MINIFS_MAGIC;
	buf->f_namelen = MINIFS_MAX_NAME_LENGTH;
	buf->f_blocks = minifs_dev->EndBlock - minifs_dev->StartBlock + 1;
	buf->f_bfree = minifs_dev->ErasedBlocks;
	buf->f_bavail = buf->f_bfree;
	buf->f_bsize = minifs_dev->DataBytesPerChunk * minifs_dev->ChunksPerBlock;
	mutex_unlock(&minifs_dev->minifs_mutex);

	return 0;
}

/*
 * Common super operations.
 */
static const struct super_operations minifs_super_ops = {
	.alloc_inode	= minifs_alloc_inode,
	.destroy_inode	= minifs_destroy_inode,
	.statfs		    = minifs_statfs,
	.drop_inode	    = generic_delete_inode,
};

extern struct mtd_info *g_mtd_info;

/*
 * Called by mount.
 */
static int minifs_fill_super(struct super_block * sb, void * data, int silent)
{
	struct inode * inode = NULL;
	struct dentry * root = NULL;

	g_mtd_info = sb->s_mtd;
	minifs_dev = minifs_dev_mount("minifs_root", "/", NULL, 0, NULL);

	if (sb->s_mtd->type != MTD_NORFLASH) {
		printk("minifs error: %s %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = MINIFS_MAGIC;
	sb->s_op = &minifs_super_ops;
	sb->s_time_gran = 1;
	inode = minifs_get_inode(sb, "/");
	if (!inode)
		return -ENOMEM;

	root = d_make_root(inode);
	if (!root) {
		iput(inode);
		return -ENOMEM;
	}
	sb->s_root = root;
	printk("minifs filesystem mount success!\n");

	return 0;
}

/*
 * use mtd system for the minifs.
 */
static struct dentry *minifs_mount(struct file_system_type *fs_type,
			int flags, const char *dev_name,
			void *data)
{
	return mount_mtd(fs_type, flags, dev_name, data, minifs_fill_super);
}

/*
 * umount.
 */
void minifs_kill_sb(struct super_block *sb)
{
	kill_mtd_super(sb);
	printk("[%s %d] minifs info : minifs_umount.\n", __func__, __LINE__);
	minifs_umount(minifs_dev);
}

static struct file_system_type minifs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "minifs",
	.mount		= minifs_mount,
	.kill_sb	= minifs_kill_sb,
};

extern int __init minifs_alloc_workspaces(void);
extern void minifs_free_workspaces(void);

static int __init init_minifs_fs(void)
{
	int err = init_inodecache();
	if (err)
		goto out1;
        err = register_filesystem(&minifs_fs_type);
	if (err)
		goto out;

	err = minifs_alloc_workspaces();
	if (err)
		goto out2;

	return 0;
out2:
	unregister_filesystem(&minifs_fs_type);
out:
	destroy_inodecache();
out1:
	return err;
}

static void __exit exit_minifs_fs(void)
{
	unregister_filesystem(&minifs_fs_type);
	destroy_inodecache();
	minifs_free_workspaces();
}

module_init(init_minifs_fs)
module_exit(exit_minifs_fs)

MODULE_LICENSE("GPL");
