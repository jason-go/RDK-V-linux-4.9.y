#ifndef __GX_MEM_INFO_H__
#define __GX_MEM_INFO_H__

#include <linux/ioctl.h>


/*
 * gx_mem_info - memory info
 *
 * @start: the physical start addr of memory
 * @size : the size of memory
 *
 * */
struct gx_mem_info {
        unsigned int start;
        unsigned int size;
};

/*
 * gx_mem_info_get()
 *
 *  DESCRIPTION
 *      Get the memory info in cmdline
 *
 *  PARAMETER
 *      @name
 *           The name of the memory in cmdline，such as "mem"/"videomem"
 *      @info
 *           the struct data to store memory info
 *
 *  RETURN VALUE
 *      On sucess, zero is returned and memory info is stroaged in info struct
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 *  EXAMPLE
 *      struct gx_mem_info video_info;
 *      int ret = gx_mem_info_get("videomem", &video_info);
 *      if (ret) {
 *              printk("start = 0x%x\n", videv_info.start);
 *              printk("size  = 0x%x\n", videv_info.size);
 *      }
 *
 * */
int gx_mem_info_get(char *name, struct gx_mem_info *info);

/*
 * gx_mem_protect_type_get()
 *
 *  DESCRIPTION
 *      Get the memory protect type in cmdline
 *
 *  PARAMETER
 *      @protect_flag
 *          the addr to store memory protect type
 *
 *  RETURN VALUE
 *      On sucess, zero is returned and memory protect type is stroaged in type
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 *  EXAMPLE
 *
 * */
int gx_mem_protect_type_get(int *type);


#define GX_MEM_INFO			('M')
#define GX_MEM_INFO_SET_NAME		(_IOR(GX_MEM_INFO, 0, unsigned char))
#define GX_MEM_INFO_GET_INFO		(_IOR(GX_MEM_INFO, 0, struct gx_mem_info))




#endif
