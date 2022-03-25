#include "gx_fw_hal.h"
#include "openssl/aes.h"
#include "des_api.h"
#include "abv_cardless_api.h"

#define MAX_COS_CACHE_BUFFER     (256 * 1024)
#define MAX_COS_SHARE_BUFFER     (64 * 1024)
#define MAX_COS_NVRAM_NODE_COUNT (128)
#define KLM_STAGE                (3)
#define DSK_ADDR                 (0x160)
//#define FW_DEBUG
typedef struct{
    uint32_t offset;
    uint32_t len;
}abv_nvram_nod;
static unsigned char *cache_buffer_share= NULL;
static unsigned char *cache_buffer= NULL;
static abv_nvram_nod nvram_write_nod[MAX_COS_NVRAM_NODE_COUNT];
static int s_kn_cur_stage = 0;
static uint8_t softkey[16] = {0};


int32_t abv_des(uint8_t* key, uint8_t* input, uint8_t* output, int length, const int enc)
{
    int32_t ret = 0, i = 0;
    uint8_t key1[8] = {0};
    uint8_t key2[8] = {0};
    des_key_schedule ks, ks2, ks3;
	unsigned char *p_in = input;
	unsigned char *p_out = output;

    memcpy(key1, key, 8);
    memcpy(key2, key + 8, 8);
    ret = DES_set_key(&key1, &ks);
    ret = DES_set_key(&key2, &ks2);
    ret = DES_set_key(&key1, &ks3);
    if (ret < 0)
        return -1;

	for (i = 0; i < length / 8; i++) {
		DES_ecb3_encrypt((const_DES_cblock *)p_in, (const_DES_cblock *)p_out, &ks, &ks2, &ks3, enc);
		p_in += 8;
		p_out += 8;
	}
	if (length % 8) {
		memcpy(p_out, p_in, length % 8);
	}
    return 0;
}

int32_t abv_tdes(uint8_t* key, uint8_t* input, uint8_t* output, int length, const int enc)
{
    int32_t ret = 0, i = 0;
    uint8_t key1[8] = {0};
    uint8_t key2[8] = {0};
    des_key_schedule ks, ks2, ks3;
	unsigned char *p_in = input;
	unsigned char *p_out = output;

    memcpy(key1, key, 8);
    memcpy(key2, key + 8, 8);
    ret = DES_set_key(&key1, &ks);
    ret = DES_set_key(&key2, &ks2);
    ret = DES_set_key(&key1, &ks3);
    if (ret < 0)
        return -1;

	for (i = 0; i < length / 16; i++) {
		DES_ecb3_encrypt((const_DES_cblock *)p_in, (const_DES_cblock *)p_out, &ks, &ks2, &ks3, enc);
		DES_ecb3_encrypt((const_DES_cblock *)(p_in + 8), (const_DES_cblock *)(p_out + 8), &ks, &ks2, &ks3, enc);
		p_in += 16;
		p_out += 16;
	}

	if (length % 16 >= 8) {
		DES_ecb3_encrypt((const_DES_cblock *)p_in, (const_DES_cblock *)p_out, &ks, &ks2, &ks3, enc);
		p_in += 8;
		p_out += 8;
		memcpy(p_out, p_in, length % 8 - 8);
	} else if (length % 16 < 8) {
		memcpy(p_out, p_in, length % 8);
	}

    return 0;
}

void abv_aes(uint8_t* key, uint8_t* input, uint8_t* output, int length, const int enc)
{
    AES_KEY decryptkey;
	unsigned char *p_in = input;
	unsigned char *p_out = output;
	int i = 0;
    if(enc == AES_DECRYPT)
    {
        AES_set_decrypt_key(key, 128, &decryptkey);
    }
    else
    {
        AES_set_encrypt_key(key, 128, &decryptkey);
    }

	for (i = 0; i < length / 16; i++) {
		AES_ecb_encrypt(p_in, p_out, &decryptkey, enc);
		p_in += 16;
		p_out += 16;
	}

	if(length % 16)
		memcpy(p_out, p_in, length % 16);

    return ;
}
int gx_aes_dec(uint8_t* key, uint8_t* input, uint8_t* output, int length)
{
	int ret;
	GxTfmCrypto ctrl = {0};
	ctrl.module = TFM_MOD_CRYPTO,
	ctrl.alg = TFM_ALG_AES128;
	ctrl.opt = TFM_OPT_ECB;
	ctrl.input.buf = input;
	ctrl.input.length = length;
	ctrl.output.buf = output;
	ctrl.output.length = length;
	ctrl.even_key.id = TFM_KEY_ACPU_SOFT;
	memcpy(ctrl.soft_key, key, 16);
	ret = gx_tfm_decrypt(&ctrl);
	return ret;
}

void * ABVCardlessMemCopy(void *pDest, const void *pSrc, unsigned int Length)
{
	void *ret = 0;

	ABVCardLessSerialPrint("%s, %d memcpy %x to %x length %d\n",__func__, __LINE__,  pSrc, pDest, Length);
	ret =  memcpy(pDest, pSrc, Length);
	ABVCardLessSerialPrint("s %x, %x, %x, %x\n", *(unsigned char *)pSrc, *((unsigned char *)pSrc + 1), *((unsigned char *)pSrc + 2), *((unsigned char *)pSrc + 3));
	ABVCardLessSerialPrint("d %x, %x, %x, %x\n", *(unsigned char *)pDest, *((unsigned char *)pDest + 1), *((unsigned char *)pDest + 2), *((unsigned char *)pDest + 3));
	ABVCardLessSerialPrint("memcpy %x to %x length %d, ret: %p\n", pSrc, pDest, Length, ret);
	return ret;
}
void * ABVCardlessMemSet(void *pDest, int Ch, unsigned int Length)
{
	void *ret = 0;
	ABVCardLessSerialPrint("%s, %d memset %x for %x length %d\n",__func__, __LINE__,  Ch, pDest, Length);
	ret = memset(pDest, Ch, Length);
	ABVCardLessSerialPrint("memset %x for %x length %d, ret: %p\n", Ch, pDest, Length, ret);
	return ret;
}
int ABVCardlessMemCmp(const void *pMem1, const void *pMem2, unsigned int Length)
{
	int ret = 0;
	ret = memcmp(pMem1, pMem2, Length);
	ABVCardLessSerialPrint("memcmp %x and %x length %d, ret: %p\n", pMem1, pMem2, Length, ret);
	return ret;
}
unsigned long abv_membase = 0;
unsigned long ABVCardLessGetMemBase(void)
{
	if (abv_membase == 0)
		while(1);
	else {
		ABVCardLessSerialPrint("get mem base %x\n", abv_membase);
		return abv_membase;
	}
}
int ABVCardLessAES128Encrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int length)
{
#define AES_MIN_DATA_SIZE (16)
    uint32_t offset = 0;
	ABVCardLessSerialPrint("ABV AES ENCRYPTO START, length %d, key %x, pin %x, pout %x\n", length, (unsigned int)pKey, (unsigned int)pIn, (unsigned int)pOut);
	ABVCardLessSerialPrint("ABV AES key %x, %x, %x, %x\n", *(unsigned int *)pKey, *((unsigned int *)pKey + 1), *((unsigned int *)pKey + 2), *((unsigned int *)pKey + 3));
	ABVCardLessSerialPrint("ABV AES in %x, %x, %x, %x\n", *(unsigned int *)pIn, *((unsigned int *)pIn + 1), *((unsigned int *)pIn + 2), *((unsigned int *)pIn + 3));
    for (offset = 0; offset < length; offset += AES_MIN_DATA_SIZE)
        abv_aes(pKey, pIn + offset, pOut + offset, AES_MIN_DATA_SIZE, AES_ENCRYPT);
	ABVCardLessSerialPrint("ABV AES out %x, %x, %x, %x\n", *(unsigned int *)pOut, *((unsigned int *)pOut + 1), *((unsigned int *)pOut + 2), *((unsigned int *)pOut + 3));
	ABVCardLessSerialPrint("ABV AES ENCRYPTO END\n");
    return 0;
}
int ABVCardLessAES128Decrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int length)
{
#define AES_MIN_DATA_SIZE (16)
    uint32_t offset = 0;
	ABVCardLessSerialPrint("ABV AES DECRYPTO START, length %d, key %x, pin %x, pout %x\n", length, (unsigned int)pKey, (unsigned int)pIn, (unsigned int)pOut);
	ABVCardLessSerialPrint("ABV AES key %x, %x, %x, %x\n", *(unsigned int *)pKey, *((unsigned int *)pKey + 1), *((unsigned int *)pKey + 2), *((unsigned int *)pKey + 3));
	ABVCardLessSerialPrint("ABV AES in %x, %x, %x, %x\n", *(unsigned int *)pIn, *((unsigned int *)pIn + 1), *((unsigned int *)pIn + 2), *((unsigned int *)pIn + 3));
    for (offset = 0; offset < length; offset += AES_MIN_DATA_SIZE)
        abv_aes(pKey, pIn + offset, pOut + offset, AES_MIN_DATA_SIZE, AES_DECRYPT);
	ABVCardLessSerialPrint("ABV AES out %x, %x, %x, %x\n", *(unsigned int *)pOut, *((unsigned int *)pOut + 1), *((unsigned int *)pOut + 2), *((unsigned int *)pOut + 3));
	ABVCardLessSerialPrint("ABV AES DECRYPTO END\n");
    return 0;
}

int ABVCardLessDESEncrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int length)
{
    uint8_t key_buffer[24] = {0};

	ABVCardLessSerialPrint("ABV DES ENCRYPTO START, length %d, key %x, pin %x, pout %x\n", length, (unsigned int)pKey, (unsigned int)pIn, (unsigned int)pOut);
	ABVCardLessSerialPrint("ABV DES key %x, %x\n", *(unsigned int *)pKey, *((unsigned int *)pKey + 1));
	ABVCardLessSerialPrint("ABV DES in %x, %x\n", *(unsigned int *)pIn, *((unsigned int *)pIn + 1));
    memcpy(key_buffer, pKey, 8);
    memcpy(key_buffer + 8, pKey, 8);
    memcpy(key_buffer + 16, pKey, 8);

	abv_des(key_buffer, pIn, pOut, length, DES_ENCRYPT);

	ABVCardLessSerialPrint("ABV DES out %x, %x\n", *(unsigned int *)pOut, *((unsigned int *)pOut + 1));
	ABVCardLessSerialPrint("ABV DES ENCRYPTO END\n");
    return 0;
}

int ABVCardLessDESDecrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int length)
{
    uint8_t key_buffer[24] = {0};

	ABVCardLessSerialPrint("ABV DES DECRYPTO START, length %d, key %x, pin %x, pout %x\n", length, (unsigned int)pKey, (unsigned int)pIn, (unsigned int)pOut);
	ABVCardLessSerialPrint("ABV DES key %x, %x\n", *(unsigned int *)pKey, *((unsigned int *)pKey + 1));
	ABVCardLessSerialPrint("ABV DES in %x, %x\n", *(unsigned int *)pIn, *((unsigned int *)pIn + 1));
    memcpy(key_buffer, pKey, 8);
    memcpy(key_buffer + 8, pKey, 8);
    memcpy(key_buffer + 16, pKey, 8);
    abv_des(key_buffer, pIn, pOut, length, DES_DECRYPT);
	ABVCardLessSerialPrint("ABV DES out %x, %x\n", *(unsigned int *)pOut, *((unsigned int *)pOut + 1));
	ABVCardLessSerialPrint("ABV DES DECRYPTO END\n");
    return 0;
}

int nvram_flag = 0xff;
int ABVCardLessIsNvmEmpty(void)
{
	if (nvram_flag == 0xff) {
		ABVCardLessSerialPrint("is nvm empty ret = 1\n");
		return 1;
	} else {
		ABVCardLessSerialPrint("is nvm empty ret = 0\n");
		return 0;
	}
}

unsigned char ABVCardLessRNGGetByte(void)
{
	unsigned int abv_rng = 0;
	abv_rng = gxse_rng_request();
	return (unsigned char)abv_rng;
}

#define ntoh(x) (((x<<24) & 0xFF000000) |((x<< 8) & 0x00FF0000) |((x >> 8) & 0x0000FF00) | ((x >> 24) & 0x000000FF))
int  nvram_write_count = 0;
abv_nvram_nod* nvram_write_nod_head;
#define MAX_COS_NVRAM_NODE_COUNT (128)
int ABVCardLessNVMWrite(unsigned int offset, unsigned int length)
{
	if (nvram_write_count > MAX_COS_NVRAM_NODE_COUNT)
		return -1;
	nvram_write_nod_head[nvram_write_count].offset = offset;
	nvram_write_nod_head[nvram_write_count].len = length;
	nvram_write_count++;
	return 0;
}
void ABVCardLessSerialPrint(const char *Format, ...)
{
#ifdef FW_DEBUG
	gx_fw_debug_print(0x88156088);
	char buf[1024];

	va_list args;
	int i;

	va_start(args, Format);
	i = vsprintf(buf, Format, args);	/* hopefully i < sizeof(buf) */
	va_end(args);

	puts(buf);

	gx_fw_debug_print(0x88156089);
#endif
	return;
}

//	37FB00C8DB1A9AB2ECD14D70E3E82325 orgkey
//	00d0005be2c800d0005be2c800d0005b chipid
//	5423423121F97CDE07D488D78373D976 tmp1
//	71A47E0A5112D660B9B02D1FEABF9D83 transformkey
//	0AC1ECA9D9BD46927A8ACCF08E76FFAA softkey
static int gx_fw_ABVCAS_init(void)
{
    uint8_t buf[8] = {0};
    uint8_t chipid[16] = {0};
    uint8_t i = 0;
    uint8_t tmp1[16] = {0};
	static int has_get_swkey = 0;
	unsigned char orgkey[16] ={0x37,0xFB,0x00,0xC8,0xDB,0x1A,0x9A,0xB2,0xEC,0xD1,0x4D,0x70,0xE3,0xE8,0x23,0x25};
	unsigned char transformkey[16]={0x71,0xA4,0x7E,0x0A,0x51,0x12,0xD6,0x60,0xB9,0xB0,0x2D,0x1F,0xEA,0xBF,0x9D,0x83};
	if(has_get_swkey == 0)
	{
		gxsecure_otp_get_publicid(buf, 8);

		for(i=0; i<16; i++)
		{
			chipid[i] = buf[i%6+2];
		}
		abv_aes(orgkey, chipid, tmp1, 16, AES_ENCRYPT);
		abv_aes(transformkey, tmp1, softkey, 16, AES_ENCRYPT);
		has_get_swkey = 1;
	}
	return 0;
}

static void gx_fw_ABVCAS_start(void)
{
	gx_fw_debug_print(0x0);
}

static int gx_fw_ABVCAS_select_rootkey(GxTfmKeyLadder *param)
{
	return gx_tfm_klm_select_rootkey(param);
}

static int gx_fw_ABVCAS_set_kn(GxTfmKeyLadder *param)
{
	int ret = 0;
    uint8_t cwkey[16] = {0};
	GxTfmScpuKlm *tfm = (GxTfmScpuKlm *)param;
	s_kn_cur_stage += 1;
	if (s_kn_cur_stage == 2) {
		abv_aes(softkey, (unsigned char *)tfm->input, cwkey, 16, AES_DECRYPT);
		memcpy(tfm->input, cwkey, 16);
	}

	ret = gx_tfm_klm_set_kn(param);
	if (ret < 0)
		s_kn_cur_stage = 0;

	return ret;
}

static int gx_fw_ABVCAS_set_cw(GxTfmKeyLadder *param)
{
	s_kn_cur_stage = 0;
	return gx_tfm_klm_set_cw(param);
}

static int cmd_cos_init()
{
	nvram_write_count = 0;
	abv_membase = (unsigned long)cache_buffer;
	nvram_write_nod_head = nvram_write_nod;
	ABVCardLessInitialize();
	return 0;
}

static int cmd_set_dispatchcmd(unsigned char *cmd, unsigned char cmd_len, unsigned char *response, unsigned char *response_len, unsigned short *sw)
{
	if (cmd == NULL || cmd_len <= 0 || response == NULL || response_len == NULL || sw == NULL)
		return -1;
	nvram_write_count = 0;
	ABVCardLessDispatchCmd((const unsigned char *)cmd, cmd_len, response, response_len, sw);
	ABVCardLessSerialPrint("%s, %d, cmd_len %d, response_len %d\n", __func__, __LINE__, cmd_len, response_len);
	ABVCardLessSerialPrint("cmd:\n");
	int i = 0;
	for (i = 0; i < cmd_len; i++)
		ABVCardLessSerialPrint("%x", cmd[i]);

	return 0;
}

#define CMD_COS_INIT (0x201900)
#define CMD_DISPATCHCMD (0x201901)
#define CMD_GET_RESP (0x201902)
#define CMD_GET_NVRAM_WRITE_INFO (0x201903)
#define CMD_GET_NVRAM_WRITE_DATA (0x201904)
#define CMD_SET_NVRAM_SYNC_DATA (0x201905)
#define CMD_SET_SHARE_MEM_ADDR (0x201906)
typedef struct {
	unsigned int id;
	unsigned int peek_size;
	unsigned char peek_buf[512];
}abv_packet;

typedef struct {
	unsigned int start;
	unsigned int size;
}abv_scpu_mem;

static abv_scpu_mem scpu_mem;
static abv_packet peek_data;
static unsigned char ret_value[512];
static unsigned char *p_send;

static int gx_fw_ABVCAS_console(abv_packet *pkt) {
	int ret = 0;
	unsigned int print_data, *print;
	unsigned char response_len = 0;
	unsigned int nvram_write_info_len = 0;
	unsigned short sw = 0;
	abv_nvram_nod nod = {0};

	if (pkt == NULL)
		return -1;

	switch (pkt->id) {
	case CMD_COS_INIT:
		memcpy(&nvram_flag, pkt->peek_buf, pkt->peek_size);
		cmd_cos_init();
		ret = nvram_write_count * sizeof(abv_nvram_nod);
		ABVCardLessSerialPrint("cmd_cos_init  nvram_write_count: %d, retval_len: %d\n", nvram_write_count, ret);
		break;
	case CMD_DISPATCHCMD:
		print = (unsigned int *)(pkt->peek_buf);
		print_data = *print;
		ret = cmd_set_dispatchcmd(pkt->peek_buf, pkt->peek_size, ret_value, &response_len, &sw);
		memcpy(ret_value + response_len, &sw, 2);
		response_len += 2;
		nvram_write_info_len = nvram_write_count * sizeof(abv_nvram_nod);
		memcpy(ret_value + response_len, &nvram_write_info_len, 4);
		response_len += 4;
		ret = response_len;
		ABVCardLessSerialPrint("cmd_dispatchcmd  cmd: %x, cmd_len: %d, resp_len:%d, sw: %x,nvram_write_count: %d, retval_len: %d\n", print_data, pkt->peek_size, response_len, sw, nvram_write_count, ret);
		break;
	case CMD_GET_RESP:
		p_send = ret_value;
		return 0;
	case CMD_GET_NVRAM_WRITE_INFO:
		p_send = (unsigned char *)nvram_write_nod;
		return 0;
	case CMD_GET_NVRAM_WRITE_DATA:
		{
			memcpy(&nod, pkt->peek_buf, pkt->peek_size);
			ABVCardLessAES128Encrypt(softkey, cache_buffer + nod.offset, cache_buffer_share, nod.len);
			ABVCardLessSerialPrint("nvm write: count: %x, offset:%x , length:%x\n", nvram_write_count, nod.offset, nod.len);
			unsigned char *wd = cache_buffer;
			ABVCardLessSerialPrint("write flash  %x, %x, %x, %x\n", *(unsigned int *)wd, *((unsigned int *)wd + 1), *((unsigned int *)wd + 2), *((unsigned int *)wd + 3));
			return 0;
		}
	case CMD_SET_NVRAM_SYNC_DATA:
		{
			memcpy(&nod, pkt->peek_buf, pkt->peek_size);
			ret = gx_aes_dec(softkey, cache_buffer_share, cache_buffer + nod.offset, nod.len);
			ABVCardLessSerialPrint("ABV cache_buffer_share %x, %x, %x, %x\n", *(unsigned int *)cache_buffer_share, *((unsigned int *)cache_buffer_share + 1), *((unsigned int *)cache_buffer_share + 2), *((unsigned int *)cache_buffer_share + 3));
			ABVCardLessSerialPrint("ABV cache_buffer %x, %x, %x, %x\n", *(unsigned int *)(cache_buffer + nod.offset), *((unsigned int *)(cache_buffer + nod.offset) + 1), *((unsigned int *)(cache_buffer + nod.offset) + 2), *((unsigned int *)(cache_buffer + nod.offset) + 3));
			return ret;
		}
	case CMD_SET_SHARE_MEM_ADDR:
		{
			unsigned int ddr_addr = 0, ddr_size = 0;
			ret = gx_fw_get_ddr_info(&ddr_addr, &ddr_size);
			if (ret < 0)
				return ret;
			cache_buffer = (unsigned char *)(ddr_addr + ddr_size);
			memcpy(&scpu_mem, pkt->peek_buf, pkt->peek_size);
			ABVCardLessSerialPrint("share mem addr %x,len %x\n", scpu_mem.start, scpu_mem.size);
			cache_buffer_share = (unsigned char *)scpu_mem.start;
			ABVCardLessSerialPrint("ABV cache_buffer %x, cache_buffer_share %x,len %x\n", cache_buffer, cache_buffer_share, scpu_mem.size);
			break;
		}
	default:
		break;
	}

	return ret;
}

static unsigned int peek_pos = 0;
static int gx_fw_ABVCAS_peek(unsigned char *buf, unsigned int size)
{
	if (buf == NULL || size <= 0 || size > 512)
		return -1;

    memcpy(((unsigned char *)&peek_data)+peek_pos, buf, size);
    peek_pos += size;

    return 0;
}

static int gx_fw_ABVCAS_peek_final()
{
	peek_pos = 0;
	return gx_fw_ABVCAS_console(&peek_data);
}

unsigned int total_size;
unsigned int  send_pos = 0;
static int gx_fw_ABVCAS_send(unsigned char *buf, unsigned int *size)
{
	if (*size <= total_size - send_pos) {
		memcpy(buf, p_send + send_pos, *size);
		send_pos += *size;
	}else {
		return -1;
	}

    return 0;
}

static int gx_fw_ABVCAS_send_check(unsigned int size, unsigned char *data_info, unsigned int data_len)
{
    if (data_info == NULL || size >= sizeof(abv_packet))
        return -1;
	total_size = size;
	send_pos = 0;
    memcpy(&peek_data, data_info, data_len);
	return gx_fw_ABVCAS_console(&peek_data);
}

static GxFWOps ABVCAS_fw_ops = {
	.init           = gx_fw_ABVCAS_init,
	.start          = gx_fw_ABVCAS_start,
	.select_rootkey = gx_fw_ABVCAS_select_rootkey,
	.set_kn         = gx_fw_ABVCAS_set_kn,
	.set_cw         = gx_fw_ABVCAS_set_cw,
	.mbox_peek           = gx_fw_ABVCAS_peek,
	.mbox_peek_final     = gx_fw_ABVCAS_peek_final,
	.mbox_send           = gx_fw_ABVCAS_send,
	.mbox_send_check     = gx_fw_ABVCAS_send_check,
	.mbox_terminate = NULL,
	.console        = NULL,
};

int gx_fw_register(GxFWOps **ops)
{
	*ops = &ABVCAS_fw_ops;
	return 0;
}

