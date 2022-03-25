#include "minifs_def.h"
#include "minifs_diskif.h"
#include "compr.h"

#ifdef __KERNEL__

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 29)
#define getgid() (current)->fsgid
#define getuid() (current)->fsuid
#else
#define getgid() (from_kgid(&init_user_ns, current_fsgid()))
#define getuid() (from_kuid(&init_user_ns, current_fsuid()))
#endif


#endif

#define INT16_XOR(x) (((x) ^ ((x) >> 8)) & 0xFF)
#define BLOCK_MAGIC 0xabcd

#define BLOCK_INFO_SIZE sizeof(struct block_info)

static int _minifs_write(minifs_Handle *h, const void *buf, unsigned int nbyte, size_t offset)
{
	int nWritten = -1;
	if ( h && h->obj)
		nWritten = minifs_WriteDataToObject(h->obj, buf, offset, nbyte);

	return nWritten;
}

static int _minifs_read(minifs_Handle *h, void *buf, unsigned int nbyte, size_t offset)
{
	minifs_Object *obj = NULL;
	int nRead = -1;

	if ( h && h->obj) {
		obj = h->obj;

		if (offset < obj->variant.file_variant.fileSize) {
			if (offset + nbyte > obj->variant.file_variant.fileSize)
				nbyte = obj->variant.file_variant.fileSize - offset;

			nRead = minifs_ReadDataFromObject(obj, buf, offset, nbyte);
		}
	}

	return nRead;
}

static int _minifs_increment(minifs_Handle *h, int nbyte, off_t offset)
{
	if (offset + nbyte > h->blockTotalSize) {
		int i;
		int end_page   = (offset + nbyte + h->blockSize - 1) / h->blockSize;

		if (end_page <= h->blockCount)
			return nbyte;

		h->blockCount = end_page;

		for (i = 0; i < h->blockCount; i++) {
			if (h->blocks[i] != NULL)
				continue;

			h->blocks[i] = (struct cache*)YMALLOC(sizeof(struct cache));
			if (h->blocks[i] == NULL) {
				T("minifs error: %s %d\n", __func__, __LINE__);
				h->blockCount = i;
				return h->blockTotalSize - offset;
			}
			memset(h->blocks[i], 0, sizeof(struct cache));
			h->blocks[i]->info.magic = BLOCK_MAGIC;
			h->blockTotalSize += h->blockSize;
		}
	}

	return nbyte;
}

int minifs_cache(minifs_Handle *h)
{
	unsigned int read_len = 0;
	size_t rawFileSize;
	int ret = -1, i, raw_offset = 0;
	uint8_t *srcdata;

	rawFileSize = h->obj->Header.rawFileSize;

	if (rawFileSize == 0 || h->blockCount > 0) // 文件大小为 0 或者已经 cache.
		return 0;

	srcdata = (uint8_t*)YMALLOC(h->blockSize);
	if (srcdata == NULL) {
		T("minifs error: %s %d\n", __func__, __LINE__);
		return -1;
	}

	_minifs_increment(h, rawFileSize, 0);

	for (i=0; i < h->blockCount; i++) {
		uint32_t destLen, sourceLen, crc;
		struct block_info *info = &h->blocks[i]->info;

		read_len = _minifs_read(h, info, sizeof(struct block_info), raw_offset);
		if (read_len != sizeof(struct block_info)) {
			T("minifs error: %s %d\n", __func__, __LINE__);
			goto error;
		}
		raw_offset += read_len;

		if (info->magic != BLOCK_MAGIC) {
			T("minifs error: %s %d\n", __func__, __LINE__);
			goto error;
		}

		sourceLen = info->comprSize;
		if (sourceLen > h->blockSize || INT16_XOR(info->comprSize) != info->sizeLow) {// 超过块大小，或者低位
			T("minifs error: %s %d\n", __func__, __LINE__);
			goto error;
		}

		memset(srcdata, 0, h->blockSize);
		read_len = _minifs_read(h, srcdata, sourceLen, raw_offset);
		if (read_len != sourceLen) {
			T("minifs error: %s %d\n", __func__, __LINE__);
			goto error;
		}
		raw_offset += read_len;
		crc = minifs_crc32(0, srcdata, sourceLen);
		if (crc != info->crc) {
			T("minifs warning: Block CRC error 0x%x : 0x%x\n", crc, info->crc);
			goto error;
		}

		if (info->is_compress) {
			destLen = h->blockSize;
			if (minifs_uncompress(h->blocks[i]->data, &destLen, srcdata, sourceLen) != 0) {
				T("minifs error: %s %d\n", __func__, __LINE__);
				goto error;
			}
		} else
			memcpy(h->blocks[i]->data, srcdata, sourceLen);
	}

	ret = 0;
error:
	YFREE((void **)&srcdata);

	return ret;
}

minifs_Handle *minifs_open(minifs_device *device, const char *path, int flags, int mode)
{
	minifs_Object *obj = NULL;

	// try to find the existing object
	obj = minifs_FindObjectByName(device, path);

	if (obj == NULL) {
		if (flags & O_CREAT) {
			obj = minifs_CreateNewObjectByName(device, path, MINIFS_OBJECT_TYPE_FILE,
					getgid(), getuid(), mode, NULL, NULL);
		}
	}

	if (obj) {
		if (obj->handle == NULL) {
			minifs_Handle *h = (minifs_Handle*)YMALLOC(sizeof(minifs_Handle));
			memset(h, 0, sizeof(minifs_Handle));
			h->obj       = obj;
			h->dirty     = 0;
			h->blockSize = MINIFS_COMPR_BLOCK_SIZE;
			obj->handle  = h;
		}
		((minifs_Handle*)obj->handle)->ref++;

		return obj->handle;
	}

	return NULL;
}

int minifs_close(minifs_Handle *h)
{
	if (h && h->obj) {
		minifs_sync(h);
		h->ref--;

		if (h->ref <= 0) {
			int i;
			h->obj->handle = NULL;

			for (i = 0; i < h->blockCount; i++) {
				if (h->blocks[i]) {
					YFREE((void **)&h->blocks[i]);
				}
			}

			YFREE((void **)&h);
		}

		return 0;
	}

	return -1;
}

int minifs_truncate(minifs_Handle *h, size_t size)
{
	minifs_Object *obj = NULL;

	obj = h->obj;
	if (obj == NULL)
		return -1;

	if (obj->Header.type != MINIFS_OBJECT_TYPE_DIRECTORY) {
		if (size < obj->Header.rawFileSize) {
			int i = 0;
			int block_id = 0;
			int block_offset = 0;
			int start_block = 0;

			if (minifs_cache(h) != 0)
				return -1;

			obj->variant.file_variant.fileSize = obj->Header.objectSize = 0;
			obj->Header.rawFileSize = size;

			block_id = size / h->blockSize;
			block_offset = size % h->blockSize;
			if (block_offset) {
				h->blocks[block_id]->dirty = 1;
				start_block = block_id + 1;
			} else
				start_block = block_id;

			for (i = start_block; i < h->blockCount; i++) {
				if (h->blocks[i]) {
					YFREE((void **)&h->blocks[i]);
				}
			}
			h->blockCount = start_block;

			h->blockTotalSize = h->blockCount * h->blockSize;
			h->dirty = 1;
			h->obj->dirty = 1;
			return 0;
		}
	}

	return -1;
}

int minifs_read(minifs_Handle *h, void *buffer, size_t nbyte, off_t offset)
{
	if (h == NULL)
		return -1;

	if (minifs_cache(h) != 0)
		return -1;

	if (offset < h->obj->Header.rawFileSize) {
		int n, nDone = 0;
		int nToCopy, blockId, start;

		if (offset + nbyte > h->obj->Header.rawFileSize)
			nbyte = h->obj->Header.rawFileSize - offset;

		n = nbyte;
		while(n > 0) {
			blockId = offset / h->blockSize;
			start = offset % h->blockSize;

			if (start + n < h->blockSize)
				nToCopy = n;
			else
				nToCopy = h->blockSize - start;

			if (nToCopy > 0) {
				if (blockId >= h->blockCount)
					break;

				memcpy(buffer, h->blocks[blockId]->data + start, nToCopy);

				n      -= nToCopy;
				offset += nToCopy;
				buffer += nToCopy;
				nDone  += nToCopy;
			}
		}
		return nDone;
	}

	return 0;
}

int minifs_write(minifs_Handle *h, const void *buffer, size_t nbyte, off_t offset)
{
	int n = nbyte;
	int nDone = 0;
	int nToCopy, blockId, start;

	if (h == NULL)
		return -1;

	if (minifs_cache(h) != 0)
		return -1;

	if (nbyte + offset > MINIFS_FILE_MAX_SIZE)
		nbyte = MINIFS_FILE_MAX_SIZE - offset;

	if (nbyte <= 0)
		return 0;
	n = nbyte;

	nbyte = _minifs_increment(h, nbyte, offset);

	while(n > 0) {
		blockId = offset / h->blockSize;
		start = offset % h->blockSize;

		if (start + n <= h->blockSize)
			nToCopy = n;
		else
			nToCopy = h->blockSize - start;

		if (nToCopy > 0) {
			if (blockId >= h->blockCount)
				break;

			memcpy(h->blocks[blockId]->data + start, buffer, nToCopy);
			h->blocks[blockId]->dirty = 1;

			n      -= nToCopy;
			offset += nToCopy;
			buffer += nToCopy;
			nDone  += nToCopy;
		}
	}

	if (offset > h->obj->Header.rawFileSize)
		h->obj->Header.rawFileSize = offset;
	h->dirty = 1;

	return nDone;
}

int minifs_sync(minifs_Handle *h)
{
	int ret = -1;
	int raw_offset = 0, start_write = 0, blockId;
	uint8_t *destData;

	if (h == NULL || h->obj == NULL)
		return -1;

	if (h->dirty == 0 || h->blockCount == 0)
		return MINIFS_OK;

	destData = YMALLOC(h->blockSize);

	if (destData == NULL)
		return -1;

	if (minifs_UpdateObjectHeader(h->obj, NULL, -1) == 0) {
		T("minifs error: %s %d\n", __func__, __LINE__);
		goto error;
	}

	for (blockId = 0; blockId < h->blockCount; blockId++) {
		struct block_info *info = &h->blocks[blockId]->info;

		if (h->blocks[blockId]->dirty)
			start_write = 1;
		else if (start_write == 0)
			raw_offset += BLOCK_INFO_SIZE + info->comprSize;

		if (start_write == 1) {
			int write_size, compr_ok;
			unsigned char *cpage_out = NULL;
			uint32_t datalen, cdatalen;

			datalen = cdatalen = h->blockSize;

			memset(destData, 0, h->blockSize);
			if (minifs_compress(destData, &cdatalen, h->blocks[blockId]->data, datalen) == 0) {
				if (cdatalen < MINIFS_COMPR_BLOCK_SIZE - STREAM_END_SPACE) {
					cpage_out = destData;
					compr_ok  = 1;
				} else {
					cpage_out = h->blocks[blockId]->data;
					cdatalen  = h->blockSize;
					compr_ok  = 0;
		        }
			} else {
				cpage_out = h->blocks[blockId]->data;
				cdatalen  = h->blockSize;
				compr_ok  = 0;
			}

			info->magic       = BLOCK_MAGIC;
			info->crc         = minifs_crc32(0, cpage_out, cdatalen);
			info->comprSize   = cdatalen;
			info->sizeLow     = INT16_XOR(info->comprSize);
			info->is_compress = compr_ok;

			// write info
			write_size = _minifs_write(h, info, BLOCK_INFO_SIZE, raw_offset);
			if (write_size != BLOCK_INFO_SIZE)
				goto error;
			raw_offset += write_size;

			// write data
			write_size = _minifs_write(h, cpage_out, cdatalen, raw_offset);
			if (write_size != cdatalen)
				goto error;
			raw_offset += write_size;
			h->blocks[blockId]->dirty = 0;
		}
	}

	h->dirty = 0;
	minifs_FlushObject(h->obj);
	ret = 0;
error:
	YFREE((void **)&destData);

	return ret;
}

int minifs_size(minifs_Handle *h)
{
	if (h)
		return h->obj->Header.rawFileSize;

	return 0;
}

int minifs_rmfile(minifs_device *device, const char *path)
{
	minifs_Object  *obj, *parent = NULL;
	int ret;

	obj = minifs_FindObjectByName(device, path);

	if (obj) {
		parent = obj->parent;
		ret = minifs_DeleteObject(obj);
		if (ret == MINIFS_OK && parent->variant.dir_variant.children.next == 0)
			INIT_LIST_HEAD(&parent->variant.dir_variant.children);
		return ret;
	}

	return MINIFS_FAIL;
}

int minifs_rmdir(minifs_device *device, const char *path)
{
	return minifs_rmfile(device, path);
}

int minifs_exists(minifs_device *device, const char *filename)
{
	minifs_Object *obj = minifs_FindObjectByName(device, filename);

	if (obj)
		return MINIFS_OK;

	return MINIFS_FAIL;
}

int minifs_finfo(minifs_device *device, const char *path)
{
	minifs_Object*  obj = NULL;

	obj = minifs_FindObjectByName(device, path);

	if (obj) {
		minifs_ShowPage(obj);

		return MINIFS_OK;
	}

	return MINIFS_FAIL;
}

int minifs_mkdir(minifs_device *device, const char *path, uint32_t mode)
{
	minifs_Object*  obj = NULL;

	// try to find the existing object
	obj = minifs_FindObjectByName(device, path);

	if (obj == NULL) {
		obj = minifs_CreateNewObjectByName(device, path, MINIFS_OBJECT_TYPE_DIRECTORY,
				getgid(), getuid(), mode, NULL, NULL);
	}

	return obj ? 0 : -1;
}

int minifs_symlink(minifs_device *device, const char *oldPath, const char *newPath)
{
	minifs_Object *obj = minifs_CreateNewObjectByName(device, newPath, MINIFS_OBJECT_TYPE_SYMLINK,
			getgid(), getuid(), 0644, NULL, oldPath);

	return obj ? 0 : -1;
}

int minifs_readlink(minifs_device *device, const char *oldPath, char *newPath, size_t size)
{
	minifs_Object *obj = minifs_FindObjectByName(device, oldPath);
	if (obj && obj->Header.type == MINIFS_OBJECT_TYPE_SYMLINK)
		strncpy(newPath, obj->Header.alias, size);

	return 0;
}

int minifs_rename(minifs_device *device, const char *oldPath, const char *newPath)
{
	int ret = -1;

	minifs_Object *obj = minifs_FindObjectByName(device, oldPath);
	if (obj) {
		minifs_Object *parent;
		char *basename = YSTRDUP(newPath);
		char *filename = strrchr(basename, '/');
		if (filename) {
			*filename = '\0';
			filename++;
		}
		parent = minifs_FindObjectByName(device, basename);

		if (parent && filename) {
			list_del(&obj->siblings);
			obj->parent = parent;
			obj->Header.parent_obj_id = parent->objectId;
			list_add(&obj->siblings, &parent->variant.dir_variant.children);
			strcpy(obj->Header.name, filename);
			if (minifs_UpdateObjectHeader(obj, filename, 1) >= 0)
				ret = 0;
		}

		YFREE((void **)&basename);
	}

	return ret;
}

minifs_device *minifs_dev_mount(const char *devname, const char *path, void *buffer, size_t size, Init init)
{
	minifs_device *device = (struct minifs_device *)YMALLOC(sizeof(struct minifs_device));

	memset(device, 0, sizeof(struct minifs_device));
	device->ReservedBlocks = 0;    // Set this smaller for RAM
	device->StartBlock     = 1;    // Can now use block zero
	device->EndBlock       = 1024; // Last block in 64MB.
	device->devname        = YSTRDUP(devname);
	device->rootPath       = YSTRDUP(path);
	device->buffer         = buffer;
	device->size           = size;

	if (init == NULL)
		device->Init = MINIFS_DISK_INIT;
	else
		device->Init = init;
	device->Init(device);
	device_Init(device);
	mutex_init(&device->minifs_mutex);

	return device;
}

static void minifs_free_all_object(minifs_Object *obj)
{
	struct list_head *i, *n;
	minifs_Object *object;

	list_for_each_safe(i, n, &obj->variant.dir_variant.children) {
		object = list_entry(i, minifs_Object, siblings);
		if (object->Header.type == MINIFS_OBJECT_TYPE_DIRECTORY)
			minifs_free_all_object(object);
		else
			minifs_FreeObject(object);
	}
	minifs_FreeObject(obj);
}

void minifs_umount(minifs_device *device)
{
	minifs_free_all_object(device->root);
	YFREE((void **)&device->pageBits);
	if (device->devname)
		YFREE((void **)&device->devname);

	if (device->Release)
		device->Release(device);

	if (device->rootPath)
		YFREE((void **)&device->rootPath);

	if (device->blocks) {
		YFREE((void **)&device->blocks);
	}
	YFREE((void **)&device);
}

