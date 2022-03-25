#include <string.h>

#ifndef _SHA_enum_
#define _SHA_enum_

#ifndef BYTE //8bit
#define BYTE unsigned char
#endif
#ifndef WORD //16bit
#define WORD unsigned short
#endif
#ifndef HRESULT
#define	HRESULT long
#endif
#ifndef DWORD
#define DWORD unsigned long
#endif
#ifndef bool //8bit
#define bool unsigned char
#endif
#ifndef NULL
#define NULL 0
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

//++++++++++++++++++++++++++++++SHA1
enum
{
    shaSuccess = 0,
		shaNull,            /* ��ָʾ���� */
		shaInputTooLong,    /* ��������̫����ʾ */
		shaStateError       /* called Input after Result --������������֮ */
};
#endif
#define SHA1HashSize 20

/*
*  �������ֽṹ���������������Ϣ for the SHA-1
*  hashing operation
*/
typedef struct SHA1Context
{
    DWORD Intermediate_Hash[SHA1HashSize/4]; /* Message Digest  */
	
    DWORD Length_Low;            /* Message length in bits      */
    DWORD Length_High;           /* Message length in bits      */
	
	/* Index into message block array   */
    WORD Message_Block_Index;
    BYTE Message_Block[64];      /* 512-bit message blocks      */
	
    int Computed;               /* Is the digest computed?         */
    int Corrupted;             /* Is the message digest corrupted? */
} SHA1Context;

/*
*  ����ԭ��
*/
int SHA1Reset(SHA1Context *);
int SHA1Input(SHA1Context *context,const BYTE  *message_array,unsigned length);
int SHA1Result(SHA1Context *,BYTE Message_Digest[SHA1HashSize]);
void HashSHA1_Encrypt20(BYTE *Out, const BYTE *In,unsigned datalen);
//ÿ5�ֽڽص���2���ֽ�
void HashSHA1_Encrypt16(BYTE *Out, const BYTE *In,unsigned datalen);


//+++++++++++++++++++++++++++++++++++3DES
/*
���ܣ�3DES����
������
out:		���������
in:			���������
datalen:	��������ݳ���
key:		��Կ��16�ֽ�
˵��:In��Out,��ʱ��/���ܺ󽫸������뻺����(In)������
*/
void TripleDes_Encrypt(BYTE *Out,BYTE *In,long datalen,BYTE *Key);

/*
���ܣ�3DES����
������
out:		���������
in:			���������
datalen:	��������ݳ���
key:		��Կ��16�ֽ�
˵��:In��Out,��ʱ��/���ܺ󽫸������뻺����(In)������
*/
void TripleDes_Decrypt(BYTE *Out,BYTE *In,long datalen,BYTE *Key);

void dvt_memcpy(void  * pDest, const void * pSrc,  unsigned int iSize);
