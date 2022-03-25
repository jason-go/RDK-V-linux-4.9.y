#ifndef __MINIFS_RAMDISK_H__
#define __MINIFS_RAMDISK_H__

#include "minifs.h"

#ifdef RAMDISK
int ramdisk_Init(minifs_device *dev);
#define MINIFS_DISK_INIT ramdisk_Init
#endif

#ifdef FILEDISK
int filedisk_Init(minifs_device *dev);
#define MINIFS_DISK_INIT filedisk_Init
#endif

#ifdef SPIFLASHDISK
int spiflash_Init(minifs_device *dev);
#define MINIFS_DISK_INIT spiflash_Init
#endif

#ifdef LINUXFLASHDISK
int linuxflash_Init(minifs_device *dev);
#define MINIFS_DISK_INIT linuxflash_Init
#endif

#ifdef __KERNEL__
int minifsMtd_Init(minifs_device *dev);
#define MINIFS_DISK_INIT minifsMtd_Init
#endif

#endif
