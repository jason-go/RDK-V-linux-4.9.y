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
		shaNull,            /* 空指示参量 */
		shaInputTooLong,    /* 输入数据太长提示 */
		shaStateError       /* called Input after Result --以输入结果命名之 */
};
#endif
#define SHA1HashSize 20

/*
*  以下这种结构将会控制上下文消息 for the SHA-1
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
*  函数原型
*/
int SHA1Reset(SHA1Context *);
int SHA1Input(SHA1Context *context,const BYTE  *message_array,unsigned length);
int SHA1Result(SHA1Context *,BYTE Message_Digest[SHA1HashSize]);
void HashSHA1_Encrypt20(BYTE *Out, const BYTE *In,unsigned datalen);
//每5字节截掉第2个字节
void HashSHA1_Encrypt16(BYTE *Out, const BYTE *In,unsigned datalen);


//+++++++++++++++++++++++++++++++++++3DES
/*
功能：3DES加密
参数：
out:		输出的密文
in:			输入的明文
datalen:	输入的数据长度
key:		密钥－16字节
说明:In＝Out,此时加/解密后将覆盖输入缓冲区(In)的内容
*/
void TripleDes_Encrypt(BYTE *Out,BYTE *In,long datalen,BYTE *Key);

/*
功能：3DES解密
参数：
out:		输出的明文
in:			输入的密文
datalen:	输入的数据长度
key:		密钥－16字节
说明:In＝Out,此时加/解密后将覆盖输入缓冲区(In)的内容
*/
void TripleDes_Decrypt(BYTE *Out,BYTE *In,long datalen,BYTE *Key);

void dvt_memcpy(void  * pDest, const void * pSrc,  unsigned int iSize);
