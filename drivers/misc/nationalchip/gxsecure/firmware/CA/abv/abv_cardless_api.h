#ifndef __ABV_CARDLESS_API_H
#define __ABV_CARDLESS_API_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
name          	: 	ABVCardlessMemCopy
Description     :   将一块内存区域的数值拷贝到另一块内存区域（参见标准C库函数memcpy）
Parameters      : 	pDest： 目标内存指针
					pSrc：  源内存指针
					Length：拷贝的内存区域大小

Return Value  	:	指向目标内存的指针
NOTES			:
****************************************************************************/
void* ABVCardlessMemCopy(void *pDest, const void *pSrc, unsigned int Length);

/****************************************************************************
name          	: 	ABVCardlessMemSet
Description     :   设置一块内存区域填充为指定的值（参见标准C库函数memset）
Parameters      : 	pDest： 指向需要填充区域的指针
					Ch：  	用来填充区域的值
					Length：填充的长度

Return Value  	:	指向被填充内存起始位置的指针
NOTES			:
****************************************************************************/
void* ABVCardlessMemSet(void *pDest, int Ch, unsigned int Length);


/****************************************************************************
name          	: 	ABVCardlessMemCmp
Description     :   比较两块内存的值（参见标准C库函数memcmp）
Parameters      : 	pMem1： 第一块内存的首地址
					pMem2： 第二块内存的首地址
					Length：需要比较的内存长度

Return Value  	:	pMem1 > pMem2 返回1
					pMem1 = pMem2返回0
					pMem1 < pMem2返回-1
NOTES			:
****************************************************************************/
int ABVCardlessMemCmp(const void *pMem1, const void *pMem2, unsigned int Length);

/****************************************************************************
name          	: 	ABVCardLessGetMemBase
Description     :   获取ABV无卡COS数据在内存中的加载首地址
Parameters      : 	None

Return Value  	:	无卡COS在内存中的加载地址
NOTES			:
****************************************************************************/
unsigned long ABVCardLessGetMemBase(void);

/****************************************************************************
name          	: 	ABVCardLessAES128Encrypt
Description     :   对指定内存当中的数据进行AES加密，明文和密文可能是同一片内存区域
Parameters      : 	pKey:   AES加密使用的key
					pIn:    需要加密数据的首地址
					pOut:   加密后密文存放的首地址
					Length: 需要加密的数据长度
Return Value  	:	成功返回0，失败返回相应的错误代码（底层错误代码）
NOTES			:
****************************************************************************/
int ABVCardLessAES128Encrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int Length);


/****************************************************************************
name          	: 	ABVCardLessAES128Decrypt
Description     :   对指定内存当中的数据进行AES解密，明文和密文可能是同一片内存区域
Parameters      : 	pKey:   AES解密使用的key
					pIn:    需要解密数据的首地址
					pOut:   解密后明文存放的首地址
					Length: 需要解密的数据长度
Return Value  	:	成功返回0，失败返回相应的错误代码（底层错误代码）
NOTES			:
****************************************************************************/
int ABVCardLessAES128Decrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int  Length);


/****************************************************************************
name          	: 	ABVCardLessDESEncrypt
Description     :   对指定内存当中的数据进行DES加密，明文和密文可能是同一片内存区域
Parameters     	:	pKey:   DES加密使用的key
					pIn:    需要加密数据的首地址
					pOut:   加密后密文存放的首地址
					Length: 需要加密的数据长度

Return Value  	:	成功返回0，失败返回相应的错误代码（底层错误代码）
NOTES			:
****************************************************************************/
int ABVCardLessDESEncrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int Length);


/****************************************************************************
name          	: 	ABVCardLessDESDecrypt
Description     :   对指定内存当中的数据进行DES解密，明文和密文可能是同一片内存区域
Parameters      : 	pKey:   DES解密使用的key
					pIn:    需要解密数据的首地址
					pOut:   解密后明文存放的首地址
					Length: 需要解密的数据长度
Return Value  	:	成功返回0，失败返回相应的错误代码（底层错误代码）
NOTES			:
****************************************************************************/
int ABVCardLessDESDecrypt(unsigned char *pKey, unsigned char *pIn, unsigned char *pOut, unsigned int  Length);


/****************************************************************************
name          	: 	ABVCardLessRNGGetByte
Description     :   设置一块内存区域填充为指定的值（参见标准C库函数memset）
Parameters      : 	pDest： 指向需要填充区域的指针
					Ch：  	用来填充区域的值
					Length：填充的长度

Return Value  	:	指向被填充内存起始位置的指针
NOTES			:
****************************************************************************/
unsigned char ABVCardLessRNGGetByte(void);


/****************************************************************************
name          	: 	ABVCardLessIsNvmEmpty
Description     :   告诉无卡COS当前数据区是否为空
Parameters      : 	NONE
Return Value  	:	空返回1，非空返回0
NOTES			:
****************************************************************************/
int ABVCardLessIsNvmEmpty(void);


/****************************************************************************
name          	: 	ABVCardLessNVMWrite
Description     :   无卡COS调用该向Flash中写入数据
Parameters      : 	Offset: 相对于无卡COS数据在内存在加载首地址的偏移
					Length: 写入数据的长度
Return Value  	:	成功返回0，失败返回相应的错误代码（底层错误代码）
NOTES			:
****************************************************************************/
int ABVCardLessNVMWrite(unsigned int Offset,  unsigned int  Length);


/****************************************************************************
name          	: 	ABVCardLessSerialPrint
Description     :   无卡COS向串口输出打印信息（正式生产版本不输出任何信息）
Parameters      : 	参见标准C库函数printf
Return Value  	:	NONE
NOTES			:
****************************************************************************/
void ABVCardLessSerialPrint(const char *Format, ...);


/****************************************************************************
name          	: 	ABVCardLessInitialize
Description     :   ABV无卡COS初始化
Parameters      : 	NONE
Return Value  	:	NONE
NOTES			:
****************************************************************************/
void ABVCardLessInitialize(void);


/****************************************************************************
name          	: 	ABVCardLessDispatchCmd
Description     :   ABV无卡COS处理CA库发过来的命令。在接收到CA库发送的命令时调用该函数将命令传递给无卡COS
Parameters      : 	Cmd:         CA库发送的命令
					CmdLength:   命令长度
					Response:    无卡COS的应答数据（不包括SW）
					ResponseLen: 无卡COS的应答数据长度
					SW:          本次命令返回的状态字
Return Value  	:	NONE
NOTES			:
****************************************************************************/
void ABVCardLessDispatchCmd(const unsigned char Cmd[], unsigned char CmdLength, unsigned char Response[], unsigned char *ResponseLen, unsigned short *SW);

#ifdef __cplusplus
}
#endif
#endif

