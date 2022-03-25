#ifndef _HDCP_KEY_H_
#define _HDCP_KEY_H_

#ifdef __cplusplus
extern "C"{
#endif

struct gxav_hdcpkey_register {
	unsigned char        ksv1[5];
	unsigned char        ksv2[5];
	unsigned int         key_encrypt;
	unsigned char        key_encrypt_seed[2];
	unsigned char        keys[280];
	unsigned char        hash[20];
};

int  gxav_hdcp_key_init(void);
void gxav_hdcp_key_uninit(void);
int  gxav_hdcp_key_register(struct gxav_hdcpkey_register *param);
int  gxav_hdcp_key_fecth(unsigned int* addr, unsigned int* size);

#ifdef __cplusplus
}
#endif

#endif
