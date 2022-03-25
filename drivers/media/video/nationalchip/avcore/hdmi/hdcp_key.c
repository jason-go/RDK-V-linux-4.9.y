#include "kernelcalls.h"
#include "gxav_hdcp_key.h"
#include "gxav_common.h"

struct gxav_hdcpkey_info {
	unsigned char  addr[314];
	unsigned int   size;
	unsigned int   active;
	unsigned int   encrypt;
};

static struct gxav_hdcpkey_info hdcp_key;

int gxav_hdcp_key_init(void)
{
	gx_memset(&hdcp_key, 0, sizeof(hdcp_key));

	return 0;
}

void gxav_hdcp_key_uninit(void)
{
	gx_memset(&hdcp_key, 0, sizeof(hdcp_key));

	return;
}

static unsigned int rotl(const unsigned int value,const unsigned int bits, int shift)
{
	return (((value << shift) & (unsigned int)((0x1 << (bits)) - 1)) | (value >> ((bits) - shift)));
}

static unsigned int rotr(const unsigned int value,const unsigned int bits, int shift)
{
	return ((value >> shift) | ((value << ((bits) - shift)) & (unsigned int)((0x1 << (bits)) - 1)));
}

static int is_aksv_right(unsigned char* data, int len)
{
	int i1 = 0, i2 = 0;
	int i, j;

	for (i = 0; i < len; i++) {
		for (j = 0; j < 8; j++) {
			if(data[i] & (1 << j))
				i1++;
			else
				i2++;
		}
	}

	gx_printf("check ksv: i1 %d, i2 %d\n", i1, i2);
	return ((i1 == i2)?1:0);
}

int gxav_hdcp_key_register(struct gxav_hdcpkey_register *param)
{
	unsigned int offset = 0;

	if (CHIP_IS_GX6605S || CHIP_IS_GX3211 || CHIP_IS_SIRIUS || CHIP_IS_TAURUS || CHIP_IS_GEMINI
				|| CHIP_IS_CYGNUS) {
		unsigned i = 0, j = 0;

		hdcp_key.addr[offset + 0] = 0x00;
		hdcp_key.addr[offset + 1] = 0x00;
		offset += 2;
		for (i = 0; i < 5; i++)
			hdcp_key.addr[offset + 4 - i] = param->ksv1[i];
		offset += 5;

		if (param->key_encrypt) {
			hdcp_key.addr[offset + 0] = param->key_encrypt_seed[0];
			hdcp_key.addr[offset + 1] = param->key_encrypt_seed[1];
			offset += 2;
			for (i = 0; i < 280; i++)
				hdcp_key.addr[offset + i] = param->keys[i];
			offset += 280;
		} else {
			unsigned int sr1_key = 0xcac1234;
			unsigned int sr2_key = 0x1978F5E;
			unsigned int tmp = 0;
			unsigned int mask[8];

			hdcp_key.addr[offset + 0] = (sr1_key >> 8) & 0xff;
			hdcp_key.addr[offset + 1] = (sr1_key) & 0xff;
			offset += 2;
			for (j = 0; j < 40; j++) {
				unsigned long long encrypt_key = 0;

				encrypt_key |= ((unsigned long long)param->keys[j * 7 + 6]) << 48;
				encrypt_key |= ((unsigned long long)param->keys[j * 7 + 5]) << 40;
				encrypt_key |= ((unsigned long long)param->keys[j * 7 + 4]) << 32;
				encrypt_key |= ((unsigned long long)param->keys[j * 7 + 3]) << 24;
				encrypt_key |= ((unsigned long long)param->keys[j * 7 + 2]) << 16;
				encrypt_key |= ((unsigned long long)param->keys[j * 7 + 1]) << 8;
				encrypt_key |= ((unsigned long long)param->keys[j * 7 + 0]);
				for (i = 0; i < 7; i++) {
					mask[i] = 0;
					mask[i] = mask[i] | ((sr2_key >> 7) & (0x1 << 0));
					mask[i] = mask[i] | ((sr2_key >> 4) & (0x1 << 1));
					mask[i] = mask[i] | ((sr2_key >> 1) & (0x1 << 2));
					mask[i] = mask[i] | ((sr2_key << 2) & (0x1 << 3));
					mask[i] = mask[i] | ((sr2_key >> 2) & (0x1 << 4));
					mask[i] = mask[i] | ((sr2_key << 1) & (0x1 << 5));
					mask[i] = mask[i] | ((sr2_key << 4) & (0x1 << 6));
					mask[i] = mask[i] | ((sr2_key << 7) & (0x1 << 7));
					sr2_key = rotr(sr2_key, 25, 1);
					sr2_key = (sr2_key ^ sr1_key) & 0x1ffffff;

					tmp = (sr1_key >> 27) ^ ((sr1_key >> 24) & 1);
					sr1_key = rotl(sr1_key, 28, 1);
					sr1_key = (sr1_key & 0xFFFFFFE) + tmp;
					encrypt_key = encrypt_key ^ (((unsigned long long) mask[i]) << 8 * (6 - i));
				}
				for (i = 0; i < 7; i++)
					hdcp_key.addr[offset + 7 * j + i] = (encrypt_key >> 8 * (6 - i)) & 0xff;
			}
			offset += 280;
		}
	} else {
		unsigned i = 0;

		if (param->key_encrypt) {
			hdcp_key.size = 0;
			return -1;
		}

		for (i = 0; i < 5; i++)
			hdcp_key.addr[offset + i] = param->ksv1[i];
		offset += 5;
		hdcp_key.addr[offset + 0] = 0x00;
		hdcp_key.addr[offset + 1] = 0x00;
		offset += 2;
		for (i = 0; i < 5; i++)
			hdcp_key.addr[offset + i] = param->ksv2[i];
		offset += 5;
		hdcp_key.addr[offset + 0] = 0x00;
		hdcp_key.addr[offset + 1] = 0x00;
		offset += 2;
		for (i = 0; i < 280; i++)
			hdcp_key.addr[offset + i] = param->keys[i];
		offset += 280;
		for (i = 0; i < 20; i++)
			hdcp_key.addr[offset + i] = param->hash[i];
		offset += 20;
	}

	if (is_aksv_right(&hdcp_key.addr[2], 5) == 1) {
		hdcp_key.size = offset;
		hdcp_key.encrypt = param->key_encrypt;
		hdcp_key.active  = 1;
	}

	return 0;
}

int gxav_hdcp_key_fecth(unsigned int* addr, unsigned int* size)
{
	if(hdcp_key.active){
		*addr = (unsigned int)hdcp_key.addr;
		*size = hdcp_key.size;
	}

	return 0;
}
