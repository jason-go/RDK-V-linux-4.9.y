#ifndef _GXSE_KAPI_MISC_H_
#define _GXSE_KAPI_MISC_H_

int gxse_fuse_get_chipname(unsigned char *buf, unsigned int size);
int gxse_fuse_get_publicid(unsigned char *buf, unsigned int size);
int gxse_fuse_get_marketid(unsigned char *buf, unsigned int size);
int gxse_fuse_get_macrovision_status(void);
int gxse_fuse_get_hdr_status(void);
int gxse_fuse_get_hevc_status(void);
unsigned int gxse_rng_request(void);

#endif
