#include "gx_fw_hal.h"
#include "gx_soft_des.h"

//#define DVT_DEBUG
#define NEED_DEC_K1_LENGTH (144)
#define RECV_TOTAL_LENGTH (((NEED_DEC_K1_LENGTH / 64) + 1) * 64)
#define KLM_STAGE (3)
#define DATA_KEY_ADDR (0x160)
#define DSK_ADDR (0x170)
#define PVRK_ADDR (0x180)
static unsigned char s_szData[NEED_DEC_K1_LENGTH] = {0};
static unsigned char Kn[KLM_STAGE][16], CW[16];
static unsigned char EKn[KLM_STAGE][16];
static int s_klm_start = 0;
static int s_klm_cur_stage = 0;
static int s_klm_total_stage = 0;
static int s_cur_user_data_len = 0;
static int dvt2_0_flag = 0;

static int gx_fw_DVTCAS_init(void)
{
	return 0;
}

static void gx_fw_DVTCAS_start(void)
{
	gx_fw_debug_print(0x0);
}

//extern int HashSHA1_Encrypt20(const unsigned char *Out, const unsigned char *In, size_t n);
static int get_ek1(char byDataLen, const unsigned char *szData, unsigned char *ek1)
{
	int byOffSet = 0,n = 0,m = 0,x = 0,i = 0,j = 0;
	unsigned char Rand[16],EnCW[16],EnK2[16],EnK1[16],EnEnK1[16],EnMap[64],EnMapKey[16];
	unsigned char Map[64],EnK1Key[16],tmpData[128],MapKey[16],RealMapKey[16],tmpKey[20],tmpRealKey[20];
	unsigned char DataKey[16];
	unsigned short wNoInGroup = 0;
	unsigned char chipid[8] = {0};
	unsigned short cssn = 0;

	if(NULL == szData)
		return 1;

	if(144 != byDataLen)
		return 2;

	//extract data from szData
	byOffSet = 0;

	memcpy(Rand,szData+byOffSet,16);
	byOffSet += 16;

	memcpy(EnCW,szData+byOffSet,16);
	byOffSet += 16;

	memcpy(EnK2,szData+byOffSet,16);
	byOffSet += 16;

	memcpy(EnEnK1,szData+byOffSet,16);
	byOffSet += 16;

	memcpy(EnMap,szData+byOffSet,64);
	byOffSet += 64;

	memcpy(EnMapKey,szData+byOffSet,16);
	byOffSet += 16;

	//计算MapKey
	memset(tmpData,0,sizeof(tmpData));

	// DataKey从OTP中读取
	gxsecure_otp_read_buf(DataKey, 16, DATA_KEY_ADDR, 16);
	TripleDes_Decrypt(MapKey,EnMapKey,16,DataKey);

	//计算RealMapKey
	memcpy(tmpData,MapKey,16);
	memcpy(tmpData+16,Rand,16);

	HashSHA1_Encrypt20(tmpRealKey, tmpData, 32);
	//20取16，每5字节截掉了第2个字节
	for(i=0,j=0;i<16;i++,j++){
		RealMapKey[i] = tmpRealKey[j];
		if(i%4==0)
			j++;
	}

	//解密Map
	memset(tmpData,0,sizeof(tmpData));
	TripleDes_Decrypt(Map,EnMap,64,RealMapKey);

	//校验组Map
	gxsecure_otp_get_publicid(chipid, 8);
	cssn = (chipid[6] << 8) | chipid[7];
	wNoInGroup = (unsigned short)(cssn & 0x01ff);

	n = wNoInGroup/8;
	m = wNoInGroup%8;
	x = 1<<m;
#ifdef DVT_DEBUG
	gx_fw_debug_print(0x99);
	unsigned int *tmp_data;
	tmp_data = (unsigned int *)Map;
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(cssn);
	gx_fw_debug_print((unsigned int)(wNoInGroup));
	gx_fw_debug_print(0x106);
#endif
	//如果对应的位数是0，表示禁止观看
	if(0 == (Map[n] & x)){
	//	gx_fw_debug_print(0x100);
		return 3;
	}

	//CA库会用全0的EnCW\EnK2\EnEnK1验证map的有效性，如果EnCW\EnK2\EnEnK1都是0,不要设置keyladder，直接返回成功。

	for(i=0;i<48;i++){
		if(0 != szData[16+i])
			break;
	}

#ifdef DVT_DEBUG
	tmp_data = (unsigned int *)Rand;
	gx_fw_debug_print(0x76);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x81);

	tmp_data = (unsigned int *)EnCW;
	gx_fw_debug_print(0x87);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x92);

	tmp_data = (unsigned int *)EnK2;
	gx_fw_debug_print(0x98);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x103);

	tmp_data = (unsigned int *)EnEnK1;
	gx_fw_debug_print(0x109);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x114);

	tmp_data = (unsigned int *)EnMap;
	gx_fw_debug_print(0x120);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x125);

	tmp_data = (unsigned int *)EnMapKey;
	gx_fw_debug_print(0x131);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x136);

	tmp_data = (unsigned int *)DataKey;
	gx_fw_debug_print(0x159);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x164);
	tmp_data = (unsigned int *)MapKey;
	gx_fw_debug_print(0x166);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x171);

	tmp_data = (unsigned int *)RealMapKey;
	gx_fw_debug_print(0x187);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x192);

	tmp_data = (unsigned int *)Map;
	gx_fw_debug_print(0x203);
	gx_fw_debug_print(tmp_data[0]);
	gx_fw_debug_print(tmp_data[1]);
	gx_fw_debug_print(tmp_data[2]);
	gx_fw_debug_print(tmp_data[3]);
	gx_fw_debug_print(0x208);
#endif
	if(48 == i){
		return 0;
	}

	//计算K1'Key
	memset(tmpData,0,sizeof(tmpData));
	memcpy(tmpData,Map,64);
	memcpy(tmpData+64,MapKey,16);
	HashSHA1_Encrypt20(tmpKey, tmpData, 80);
	//20取16，每5字节截掉了第2个字节
	for(i=0,j=0;i<16;i++,j++){
		EnK1Key[i] = tmpKey[j];
		if(i%4==0)
			j++;
	}

	//解密得到K1'
	TripleDes_Decrypt(EnK1,EnEnK1,16,EnK1Key);
	if (ek1 != NULL)
		memcpy(ek1, EnK1, 16);
	memcpy(EKn[0], EnK2, 16);
	memcpy(EKn[1], EnK1, 16);
	memcpy(EKn[2], EnCW, 16);
	dvt2_0_flag = 1;

	return 0;
}

static int gx_fw_DVTCAS_select_rootkey(GxTfmKeyLadder *param)
{
	s_klm_start = 1;
	s_klm_cur_stage = 0;
	s_klm_total_stage = param->stage;
	if (param->key.id == TFM_KEY_CWUK){
		gxsecure_otp_read_buf(Kn[0], 16, DSK_ADDR, 16);
	} else if (param->key.id == TFM_KEY_PVRK){
		gxsecure_otp_read_buf(Kn[0], 16, PVRK_ADDR, 16);
	}

	return 0;
}

static int gx_fw_DVTCAS_set_kn(GxTfmKeyLadder *param)
{
	if (s_klm_start == 0 ||
		s_klm_cur_stage+1 >= s_klm_total_stage) {
		return -1;
	}

	if (dvt2_0_flag == 0)
		memcpy(EKn[s_klm_cur_stage], param->input, 16);
	s_klm_cur_stage++;

	return 0;
}

static int gx_fw_DVTCAS_set_cw(GxTfmKeyLadder *param)
{
	int i = 0;
	GxSeScpuKey key;
	if (s_klm_start == 0 ||
		s_klm_cur_stage+1 != s_klm_total_stage) {
		return -1;
	}
	memcpy(EKn[s_klm_cur_stage], param->input, 16);

	for (i = 0; i < s_klm_total_stage; i++) {
		if (i == s_klm_total_stage - 1)
			TripleDes_Decrypt(CW,EKn[i],16,Kn[i]);
		else
			TripleDes_Decrypt(Kn[i+1],EKn[i],16,Kn[i]);
	}

#ifdef DVT_DEBUG
	unsigned int * tmp_cw = (unsigned int *)EKn[0];
	tmp_cw = (unsigned int *)Kn[0];
	gx_fw_debug_print(0x288);
	gx_fw_debug_print(tmp_cw[0]);
	gx_fw_debug_print(tmp_cw[1]);
	gx_fw_debug_print(tmp_cw[2]);
	gx_fw_debug_print(tmp_cw[3]);

	gx_fw_debug_print(0xFFFFFFFF);
	tmp_cw = (unsigned int *)EKn[0];
	gx_fw_debug_print(tmp_cw[0]);
	gx_fw_debug_print(tmp_cw[1]);
	gx_fw_debug_print(tmp_cw[2]);
	gx_fw_debug_print(tmp_cw[3]);

	gx_fw_debug_print(0xFFFFFFFF);
	tmp_cw = (unsigned int *)Kn[1];
	gx_fw_debug_print(tmp_cw[0]);
	gx_fw_debug_print(tmp_cw[1]);
	gx_fw_debug_print(tmp_cw[2]);
	gx_fw_debug_print(tmp_cw[3]);

	gx_fw_debug_print(0xFFFFFFFF);
	tmp_cw = (unsigned int *)EKn[1];
	gx_fw_debug_print(tmp_cw[0]);
	gx_fw_debug_print(tmp_cw[1]);
	gx_fw_debug_print(tmp_cw[2]);
	gx_fw_debug_print(tmp_cw[3]);

	gx_fw_debug_print(0xFFFFFFFF);
	tmp_cw = (unsigned int *)Kn[2];
	gx_fw_debug_print(tmp_cw[0]);
	gx_fw_debug_print(tmp_cw[1]);
	gx_fw_debug_print(tmp_cw[2]);
	gx_fw_debug_print(tmp_cw[3]);

	gx_fw_debug_print(0xFFFFFFFF);
	tmp_cw = (unsigned int *)EKn[2];
	gx_fw_debug_print(tmp_cw[0]);
	gx_fw_debug_print(tmp_cw[1]);
	gx_fw_debug_print(tmp_cw[2]);
	gx_fw_debug_print(tmp_cw[3]);

	tmp_cw = (unsigned int *)CW;
	gx_fw_debug_print(tmp_cw[0]);
	gx_fw_debug_print(tmp_cw[1]);
	gx_fw_debug_print(tmp_cw[2]);
	gx_fw_debug_print(tmp_cw[3]);
#endif


	if (param->dst.id == TFM_DST_M2M) {
		key.type = MISC_KEY_SCPU_PVR;
	} else if (param->dst.id == TFM_DST_TS) {
		key.type = MISC_KEY_SCPU_CW;
	}
	if (TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_INPUT_HALF) == 1) {
		key.length = 8;
		memcpy(key.value, CW, 8);
	} else {
		key.length = 16;
		memcpy(key.value, CW, 16);
	}
	gxse_misc_send_key(&key);

	s_klm_start = 0;
	s_klm_cur_stage = 0;

	return 0;
}

static int gx_fw_DVTCAS_peek(unsigned char *buf, unsigned int size)
{
	if (buf == NULL || s_cur_user_data_len >= NEED_DEC_K1_LENGTH || size <= 0 || size > NEED_DEC_K1_LENGTH)
		return -1;

	memcpy(s_szData + s_cur_user_data_len, buf, size);
	s_cur_user_data_len += size;
	return 0;
}

static int dvt_resp = 0;
static int gx_fw_DVTCAS_peek_final()
{
	s_cur_user_data_len = 0;
	dvt_resp = get_ek1(NEED_DEC_K1_LENGTH, s_szData, NULL);
	return sizeof(dvt_resp);
}

static int gx_fw_DVTCAS_send(unsigned char *buf, unsigned int *size)
{
	if (buf == NULL || size == NULL || *size != 4)
		return -1;

	memcpy(buf, &dvt_resp, *size);
	dvt_resp = 0;
	return 0;
}

static int gx_fw_DVTCAS_send_check(unsigned int size, unsigned char *data_info, unsigned int data_len)
{
	if (size != 4)
		return -1;

	return 0;
}

static GxFWOps DVTCAS_fw_ops = {
	.init           = gx_fw_DVTCAS_init,
	.start          = gx_fw_DVTCAS_start,
	.select_rootkey = gx_fw_DVTCAS_select_rootkey,
	.set_kn         = gx_fw_DVTCAS_set_kn,
	.set_cw         = gx_fw_DVTCAS_set_cw,
	.mbox_peek      = gx_fw_DVTCAS_peek,
	.mbox_peek_final= gx_fw_DVTCAS_peek_final,
	.mbox_send      = gx_fw_DVTCAS_send,
	.mbox_send_check= gx_fw_DVTCAS_send_check,
};

int gx_fw_register(GxFWOps **ops)
{
	*ops = &DVTCAS_fw_ops;
	return 0;
}
