/*******************************************************************************
FileName          : $FILENAME
Author            : zhuzhiguo
Version           : 1.0
Date              : 2008.08.28
Description       :
Function List     :
 ********************************************************************************
History: Scroll to the end of this file.
 ********************************************************************************/

#ifndef _MINIFS_H_
#define _MINIFS_H_

#include "minifs_def.h"

struct block_info {
	uint32_t magic;
	uint32_t crc;
	uint16_t comprSize;
	uint8_t  sizeLow;
	uint8_t is_compress;
};

struct cache {
	struct block_info info;
	uint8_t data[MINIFS_COMPR_BLOCK_SIZE];
	char dirty;
};

typedef struct {
	minifs_Object* obj;         // the object
	uint32_t       blockCount;
	struct cache   *blocks[MINIFS_COMPR_BLOCK_COUNT];
	uint32_t       blockSize;
	uint32_t       blockTotalSize;

	int            dirty;
	int            ref;
}minifs_Handle;

typedef int (*Init)(minifs_device *dev);

/*------------------------ FUNCTION PROTOTYPE(S) -----------------------------*/
minifs_device *minifs_dev_mount(const char *devname, const char *path, void *buffer, size_t size, Init init);
void minifs_umount (minifs_device *dev);
int minifs_cache   (minifs_Handle *h);
int minifs_close   (minifs_Handle *h);
int minifs_read    (minifs_Handle *h, void *buf, size_t nbyte, off_t offset);
int minifs_write   (minifs_Handle *h, const void *buf, size_t nbyte, off_t offset);
int minifs_size    (minifs_Handle *h);
int minifs_sync    (minifs_Handle *h);
int minifs_truncate(minifs_Handle *h, size_t size);

minifs_Handle *minifs_open(minifs_device *dev, const char *filename, int flags, int mode);
int minifs_readlink (minifs_device *dev, const char *oldPath, char *newPath, size_t size);
int minifs_symlink  (minifs_device *dev, const char *oldPath, const char *newPath);
int minifs_rename   (minifs_device *dev, const char *oldPath, const char *newPath);
int minifs_rmfile   (minifs_device *dev, const char *filename);
int minifs_mkdir    (minifs_device *dev, const char *path, uint32_t mode);
int minifs_rmdir    (minifs_device *dev, const char *dirname);
int minifs_finfo    (minifs_device *dev, const char *filename);
int minifs_dump     (minifs_device *dev, const char *image);
int minifs_load     (minifs_device *dev, const char *local_directory, int level);
int minifs_Traversal(minifs_device *dev, minifs_Object *parent, int (*process)(minifs_Object *obj));

#endif

