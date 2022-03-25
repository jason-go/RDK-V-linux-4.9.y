#ifndef __ABV_CARDLESS_API_H
#define __ABV_CARDLESS_API_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
name          	: 	ABVCardlessMemCopy
Description     :   ��һ���ڴ��������ֵ��������һ���ڴ����򣨲μ���׼C�⺯��memcpy��
Parameters      : 	pDest�� Ŀ���ڴ�ָ��
					pSrc��  Դ�ڴ�ָ��
					Length���������ڴ������С

Return Value  	:	ָ��Ŀ���ڴ��ָ��
NOTES			:
****************************************************************************/
void* ABVCardlessMemCopy(void *pDest, const void *pSrc, unsigned int Length);

/****************************************************************************
name          	: 	ABVCardlessMemSet
Description     :   ����һ���ڴ��������Ϊָ����ֵ���μ���׼C�⺯��memset��
Parameters      : 	pDest�� ָ����Ҫ��������ָ��
					Ch��  	������������ֵ
					Length�����ĳ���

Return Value  	:	ָ������ڴ���ʼλ�õ�ָ��
NOTES			:
****************************************************************************/
void* ABVCardlessMemSet(void *pDest, int Ch, unsigned int Length);


/****************************************************************************
name          	: 	ABVCardlessMemCmp
Description     :   �Ƚ������ڴ��ֵ���μ���׼C�⺯��memcmp��
Parameters      : 	pMem1�� ��һ���ڴ���׵�ַ
					pMem2�� �ڶ����ڴ���׵�ַ
					Length����Ҫ�Ƚϵ��ڴ泤��

Return Value  	:	pMem1 > pMem2 ����1
					pMem1 = pMem2����0
					pMem1 < pMem2����-1
NOTES			:
****************************************************************************/
int ABVCardlessMemCmp(const void *pMem1, const void *pMem2, unsigned int Length);

/****************************************************************************
name          	: 	ABVCardLessGetMemBase
Description     :   ��ȡABV�޿�COS�������ڴ��еļ����׵�ַ
Parameters      : 	None

Return Value  	:	�޿�COS���ڴ��еļ��ص�ַ
NOTES			:
****************************************************************************/
unsigned long ABVCardLessGetMemBase(void);

/****************************************************************************
name          	: 	ABVCardLessAES128Encrypt
Description     :   ��ָ���ڴ浱�е����ݽ���AES���ܣ����ĺ����Ŀ�����ͬһƬ�ڴ�����
Parameters      : 	pKey:   AES����ʹ�õ�key
					pIn:    ��Ҫ�������ݵ��׵�ַ
					pOut:   ���ܺ����Ĵ�ŵ��׵�ַ
					Length: ��Ҫ���ܵ����ݳ���
Return Value  	:	�ɹ�����0��ʧ�ܷ�����Ӧ�Ĵ�����루�ײ������룩
NOTES			:
****************************************************************************/
int ABVCardLessAES128Encrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int Length);


/****************************************************************************
name          	: 	ABVCardLessAES128Decrypt
Description     :   ��ָ���ڴ浱�е����ݽ���AES���ܣ����ĺ����Ŀ�����ͬһƬ�ڴ�����
Parameters      : 	pKey:   AES����ʹ�õ�key
					pIn:    ��Ҫ�������ݵ��׵�ַ
					pOut:   ���ܺ����Ĵ�ŵ��׵�ַ
					Length: ��Ҫ���ܵ����ݳ���
Return Value  	:	�ɹ�����0��ʧ�ܷ�����Ӧ�Ĵ�����루�ײ������룩
NOTES			:
****************************************************************************/
int ABVCardLessAES128Decrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int  Length);


/****************************************************************************
name          	: 	ABVCardLessDESEncrypt
Description     :   ��ָ���ڴ浱�е����ݽ���DES���ܣ����ĺ����Ŀ�����ͬһƬ�ڴ�����
Parameters     	:	pKey:   DES����ʹ�õ�key
					pIn:    ��Ҫ�������ݵ��׵�ַ
					pOut:   ���ܺ����Ĵ�ŵ��׵�ַ
					Length: ��Ҫ���ܵ����ݳ���

Return Value  	:	�ɹ�����0��ʧ�ܷ�����Ӧ�Ĵ�����루�ײ������룩
NOTES			:
****************************************************************************/
int ABVCardLessDESEncrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int Length);


/****************************************************************************
name          	: 	ABVCardLessDESDecrypt
Description     :   ��ָ���ڴ浱�е����ݽ���DES���ܣ����ĺ����Ŀ�����ͬһƬ�ڴ�����
Parameters      : 	pKey:   DES����ʹ�õ�key
					pIn:    ��Ҫ�������ݵ��׵�ַ
					pOut:   ���ܺ����Ĵ�ŵ��׵�ַ
					Length: ��Ҫ���ܵ����ݳ���
Return Value  	:	�ɹ�����0��ʧ�ܷ�����Ӧ�Ĵ�����루�ײ������룩
NOTES			:
****************************************************************************/
int ABVCardLessDESDecrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int  Length);


/****************************************************************************
name          	: 	ABVCardLessRNGGetByte
Description     :   ����һ���ڴ��������Ϊָ����ֵ���μ���׼C�⺯��memset��
Parameters      : 	pDest�� ָ����Ҫ��������ָ��
					Ch��  	������������ֵ
					Length�����ĳ���

Return Value  	:	ָ������ڴ���ʼλ�õ�ָ��
NOTES			:
****************************************************************************/
unsigned char ABVCardLessRNGGetByte(void);


/****************************************************************************
name          	: 	ABVCardLessIsNvmEmpty
Description     :   �����޿�COS��ǰ�������Ƿ�Ϊ��
Parameters      : 	NONE
Return Value  	:	�շ���1���ǿշ���0
NOTES			:
****************************************************************************/
int ABVCardLessIsNvmEmpty(void);


/****************************************************************************
name          	: 	ABVCardLessNVMWrite
Description     :   �޿�COS���ø���Flash��д������
Parameters      : 	Offset: ������޿�COS�������ڴ��ڼ����׵�ַ��ƫ��
					Length: д�����ݵĳ���
Return Value  	:	�ɹ�����0��ʧ�ܷ�����Ӧ�Ĵ�����루�ײ������룩
NOTES			:
****************************************************************************/
int ABVCardLessNVMWrite(unsigned int Offset,  unsigned int  Length);


/****************************************************************************
name          	: 	ABVCardLessSerialPrint
Description     :   �޿�COS�򴮿������ӡ��Ϣ����ʽ�����汾������κ���Ϣ��
Parameters      : 	�μ���׼C�⺯��printf
Return Value  	:	NONE
NOTES			:
****************************************************************************/
void ABVCardLessSerialPrint(const char *Format, ...);


/****************************************************************************
name          	: 	ABVCardLessInitialize
Description     :   ABV�޿�COS��ʼ��
Parameters      : 	NONE
Return Value  	:	NONE
NOTES			:
****************************************************************************/
void ABVCardLessInitialize(void);


/****************************************************************************
name          	: 	ABVCardLessDispatchCmd
Description     :   ABV�޿�COS����CA�ⷢ����������ڽ��յ�CA�ⷢ�͵�����ʱ���øú���������ݸ��޿�COS
Parameters      : 	Cmd:         CA�ⷢ�͵�����
					CmdLength:   �����
					Response:    �޿�COS��Ӧ�����ݣ�������SW��
					ResponseLen: �޿�COS��Ӧ�����ݳ���
					SW:          ��������ص�״̬��
Return Value  	:	NONE
NOTES			:
****************************************************************************/
void ABVCardLessDispatchCmd(const unsigned char Cmd[], unsigned char CmdLength, unsigned char Response[], unsigned char *ResponseLen, unsigned short *SW);

#ifdef __cplusplus
}
#endif
#endif

