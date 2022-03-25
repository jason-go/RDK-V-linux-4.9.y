#include "gx_soft_des.h"

long int dvt_labs(long int number)
{
	   return (number>= 0 ? number : -number);
}

void dvt_memcpy(void  * pDest, const void * pSrc,  unsigned int iSize)
{
	int i;
    if(!pDest||!pSrc)
		return;

	if(iSize <= dvt_labs((BYTE *)pDest-(BYTE *)pSrc))
	{
		memcpy(pDest,pSrc,iSize);
	}
	else
	{
		if(pSrc>pDest)
		{
			for(i=0;i<iSize;i++)
			{
				*((BYTE *)pDest+i) = *((BYTE *)pSrc+i);
			}
		}
		else
		{
			while(iSize--)
				*((BYTE *)pDest+iSize) = *((BYTE *)pSrc+iSize);
		}
	}
	return;
}

#if 1
//sha1只给DVT用
/*
 *  以下是为 SHA1 向左环形移位宏 之定义
 */
#define SHA1CircularShift(bits,word) \
                (((word) << (bits)) | ((word) >> (32-(bits))))


/* 局部函数原型 */
void SHA1PadMessage(SHA1Context *context);
void SHA1ProcessMessageBlock(SHA1Context *context);
/*
 *  SHA1Reset
 *
 *  以下为数据初始化之操作
 *  Parameters:（参数设置）
 *  context: [in/out]
 *  The context to reset.
 *
 */
int SHA1Reset(SHA1Context *context)
{
    if (!context)
    {
        return shaNull;
    }

    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Intermediate_Hash[0]   = 0x67452301;
    context->Intermediate_Hash[1]   = 0xEFCDAB89;
    context->Intermediate_Hash[2]   = 0x98BADCFE;
    context->Intermediate_Hash[3]   = 0x10325476;
    context->Intermediate_Hash[4]   = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;
    return shaSuccess;
}

/*
 *  SHA1Result
 *
 *  以下为sha-1结果描述：
 *:
 *  该算法将会返回一个160比特的消息摘要队列
 *
 *  或者输出计算错误
 *
 */
int SHA1Result( SHA1Context *context,
                BYTE Message_Digest[SHA1HashSize])
{
    int i;

    if (!context || !Message_Digest)
    {
        return shaNull;
    }

    if (context->Corrupted)
    {
        return context->Corrupted;
    }

    if (!context->Computed)
    {
        SHA1PadMessage(context);
        for(i=0; i<64; ++i)
        {
            /* 消息清零 */
            context->Message_Block[i] = 0;
        }
        context->Length_Low = 0;    /* 长度清零 */
        context->Length_High = 0;
        context->Computed = 1;
    }

    for(i = 0; i < SHA1HashSize; ++i)
    {
        Message_Digest[i] = (BYTE)(context->Intermediate_Hash[i>>2] >> 8 * ( 3 - ( i & 0x03 ) ));
    }

    return shaSuccess;
}

/*
 *  以下为sha-1输入描述：
 *
 *  接收单位长度为8字节倍数的消息
 *
 */
int SHA1Input(    SHA1Context    *context,
                  const BYTE  *message_array,
                  unsigned       length)
{
    if (!length)
    {
        return shaSuccess;
    }

    if (!context || !message_array)
    {
        return shaNull;
    }

    if (context->Computed)
    {
        context->Corrupted = shaStateError;
        return shaStateError;
    }

    if (context->Corrupted)
    {
         return context->Corrupted;
    }
    while(length-- && !context->Corrupted)
    {
    context->Message_Block[context->Message_Block_Index++] =
                    (*message_array & 0xFF);

    context->Length_Low += 8;
    if (context->Length_Low == 0)
    {
        context->Length_High++;
        if (context->Length_High == 0)
        {
            /* Message is too long */
            context->Corrupted = 1;
        }
    }

    if (context->Message_Block_Index == 64)
    {
        SHA1ProcessMessageBlock(context);
    }

    message_array++;
    }

    return shaSuccess;
}

/*
 *  以下为sha-1消息块描述：
 *
 *  消息块长度为固定之512比特
 *
 */
void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const DWORD K[] =    {       /* Constants defined in SHA-1   */
                            0x5A827999,
                            0x6ED9EBA1,
                            0x8F1BBCDC,
                            0xCA62C1D6
                            };
    int           t;                 /* 循环计数 */
    DWORD      temp;              /* 临时缓存 */
    DWORD      W[80];             /* 字顺序   */
    DWORD      A, B, C, D, E;     /* 设置系统磁盘缓存块 */

    /*
     *  以下为初始化在W队列中的头16字数据
     */
    for(t = 0; t < 16; t++)
    {
         W[t] = (DWORD)context->Message_Block[t * 4] << 24 | (DWORD)context->Message_Block[t * 4 + 1] << 16 | (DWORD)context->Message_Block[t * 4 + 2] << 8 | (DWORD)context->Message_Block[t * 4 + 3];
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];

    /*
     *  以下为定义算法所用之数学函数及其迭代算法描述
     */

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
   B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    /*
     *  以下为迭代算法第80步（最后一步）描述
     */
    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;

    context->Message_Block_Index = 0;
}


/*
 *  SHA1PadMessage
 *  数据填充模块
 */

void SHA1PadMessage(SHA1Context *context)
{

    if (context->Message_Block_Index > 55)
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  把最后64位保存为数据长度
     */
    context->Message_Block[56] = (BYTE)(context->Length_High >> 24);
    context->Message_Block[57] = (BYTE)(context->Length_High >> 16);
    context->Message_Block[58] = (BYTE)(context->Length_High >> 8);
    context->Message_Block[59] = (BYTE)context->Length_High;
    context->Message_Block[60] = (BYTE)(context->Length_Low >> 24);
    context->Message_Block[61] = (BYTE)(context->Length_Low >> 16);
    context->Message_Block[62] = (BYTE)(context->Length_Low >> 8);
    context->Message_Block[63] = (BYTE)context->Length_Low;

    SHA1ProcessMessageBlock(context);
}

//**********************************************************************************
//Hash SHA1算法
void HashSHA1_Encrypt20(BYTE *Out, const BYTE *In,unsigned datalen)
{
	SHA1Context Context;
	BYTE temp[20];
	
	SHA1Reset(&Context);
	SHA1Input(&Context,In,datalen);
	SHA1Result(&Context,temp);
	dvt_memcpy(Out,temp,20);
}

void HashSHA1_Encrypt16(BYTE *Out, const BYTE *In,unsigned datalen)
{
	SHA1Context Context;
	BYTE temp[20];
	int i,j;
	
	SHA1Reset(&Context);
	SHA1Input(&Context,In,datalen);
	SHA1Result(&Context,temp);
	for(i=0,j=0;i<16;i++,j++){
		Out[i] = temp[j];
		if(i%4==0)
			j++;
	}
}

#endif
/*
Internal defines
*/
#define DES_ENCRYPT     0
#define DES_DECRYPT     1

typedef BYTE DesBlock;

/*
    Internal function prototypes
*/
static void DesDes(BYTE *, BYTE *, BYTE *, short);
static void DesPermut(DesBlock *, DesBlock *, short);
static void DesRotate(DesBlock *, short);
static void DesConvert(DesBlock *, DesBlock *);
static void DesHexToBlock(DesBlock *, BYTE *);
static void DesBlockToHex(BYTE *, DesBlock *);
/*

    Function: DesDes

    DES-Algorthim.

    Parameter: OUT BYTE[8]     En-/Decrypted data (binary)
               IN  BYTE[8]     data to en-/decrypt (binary)
               IN  BYTE[8]     key used for en-/decryption (binary)
               IN  short                Flag if data has to be encrypted(0)
                                        or decrypted(1)

    Return: no return

*/
static void DesDes(
        BYTE auchOutput[8],
        BYTE auchInput[8],
        BYTE auchKey[8],
        short sMethod)
{
    static DesBlock InitPermut[64] =
    {
        58, 50, 42, 34, 26, 18, 10,  2, 60, 52, 44, 36, 28, 20, 12,  4,
        62, 54, 46, 38, 30, 22, 14,  6, 64, 56, 48, 40, 32, 24, 16,  8,
        57, 49, 41, 33, 25, 17,  9,  1, 59, 51, 43, 35, 27, 19, 11,  3,
        61, 53, 45, 37, 29, 21, 13,  5, 63, 55, 47, 39, 31, 23, 15,  7
    };

    static DesBlock FinPermut[64] =
    {
        8, 40, 16, 48, 24, 56, 32, 64,  7, 39, 15, 47, 23, 55, 31, 63,
        6, 38, 14, 46, 22, 54, 30, 62,  5, 37, 13, 45, 21, 53, 29, 61,
        4, 36, 12, 44, 20, 52, 28, 60,  3, 35, 11, 43, 19, 51, 27, 59,
        2, 34, 10, 42, 18, 50, 26, 58,  1, 33,  9, 41, 17, 49, 25, 57
    };

    static DesBlock KeyPermut[56] =
    {
        57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18, 10,  2,
        59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36, 63, 55, 47, 39,
        31, 23, 15,  7, 62, 54, 46, 38, 30, 22, 14,  6, 61, 53, 45, 37,
        29, 21, 13,  5, 28, 20, 12,  4
    };

    static DesBlock TmpKeyPermut[48] =
    {
        14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10, 23, 19, 12,  4,
        26,  8, 16,  7, 27, 20, 13,  2, 41, 52, 31, 37, 47, 55, 30, 40,
        51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
    };

    static DesBlock Tmp1Permut[48] =
    {
        32,  1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,  8,  9, 10, 11,
        12, 13, 12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 20, 21, 20, 21,
        22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32,  1
    };

    static DesBlock Tmp2Permut[32] =
    {
        16,  7, 20, 21, 29, 12, 28, 17,  1, 15, 23, 26,  5, 18, 31, 10,
         2,  8, 24, 14, 32, 27,  3,  9, 19, 13, 30,  6, 22, 11,  4, 25
    };

    static DesBlock NumRotations[2][16] =
    {
        {
            1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
        },
        {
            0, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
        }
    };

    DesBlock KeyBlock[64];
    DesBlock WorkBlock[64];
    DesBlock TmpKeyBlock[64];
    DesBlock TmpWorkBlock[32];
    DesBlock TmpBlock[64];
    DesBlock HlpBlock[48];
    register short sCount;
    register short ii;

    /*
        Extract binary representation of input data (8 bytes -> 64 bits)
    */
    DesHexToBlock(WorkBlock, auchInput);
    /*
        Extract binary representation of key (8 bytes -> 64 bits)
    */
    DesHexToBlock(KeyBlock, auchKey);

    DesPermut(WorkBlock, InitPermut, 64);
    DesPermut(KeyBlock, KeyPermut, 56);

    for (sCount = 0 ; sCount < 16 ; sCount++)
    {
        dvt_memcpy(TmpWorkBlock, WorkBlock, sizeof(TmpWorkBlock));
        dvt_memcpy(WorkBlock, &WorkBlock[32], 32 * sizeof(DesBlock));

        for (ii = NumRotations[sMethod][sCount] ; ii > 0 ; ii--)
        {
            DesRotate(KeyBlock, sMethod);
        }

        dvt_memcpy(TmpBlock, WorkBlock, sizeof(TmpBlock));
        DesPermut(TmpBlock, Tmp1Permut, 48);

        dvt_memcpy(TmpKeyBlock, KeyBlock, sizeof(TmpKeyBlock));
        DesPermut(TmpKeyBlock, TmpKeyPermut, 48);

        for (ii = 0 ; ii < 48 ; ii++)
        {
            HlpBlock[ii] = TmpBlock[ii] ^ TmpKeyBlock[ii];
        }

        DesConvert(TmpBlock, HlpBlock);

        DesPermut(TmpBlock, Tmp2Permut, 32);

        for (ii = 0 ; ii < 32 ; ii++)
        {
            WorkBlock[ii + 32] = TmpWorkBlock[ii] ^ TmpBlock[ii];
        }
    } /* end for */

    DesPermut(WorkBlock, FinPermut, 64);
    /*
        Build output data (64 bits -> 8 bytes)
    */
    DesBlockToHex(auchOutput, WorkBlock);
} /* end DesDes() */

/*

    Function: DesPermut

    Permutaion of data

    Parameter: IN/OUT  DesBlock[]       Data to permut -> permuted data
               IN      DesBlock[]       Rule for permutation
               IN      short            Number of enties in permutation rules

    Return: no return

*/
static void DesPermut(
        DesBlock OutData[],
        DesBlock PermutTable[],
        short sNumEntries)
{
    DesBlock TmpBlock[64 + 1];
    register short ii;

    dvt_memcpy(&TmpBlock[1], OutData, 64 * sizeof(DesBlock));

    for (ii = 0 ; ii < sNumEntries ; ii++)
    {
        OutData[ii] = TmpBlock[PermutTable[ii]];
    }
} /* end DesPermut () */

/*

    Function: DesRotate

    Rotation of data

    Parameter: IN/OUT  DesBlock[64]     Data to rotate -> rotated data
               IN  short                Flag how to rotate left(0)
                                        or right(1)

    Return: no return

*/
static void DesRotate(
        DesBlock OutBlock[64],
        short sMethod)
{
    DesBlock TmpBlock[64];

    dvt_memcpy(TmpBlock, OutBlock, sizeof(TmpBlock));

    if (sMethod == DES_ENCRYPT)
    {
        dvt_memcpy(TmpBlock, &TmpBlock[1], 55 * sizeof(DesBlock));//zp:2011.12.21 将memmove替换为DVTCAS_dvt_memcpy

        TmpBlock[27] = OutBlock[0];
        TmpBlock[55] = OutBlock[28];
    } /* end if */
    else
    {
        dvt_memcpy(&TmpBlock[1], TmpBlock, 55 * sizeof(DesBlock));//zp:2011.12.21

        TmpBlock[0]  = OutBlock[27];
        TmpBlock[28] = OutBlock[55];
    } /* end else */

    dvt_memcpy(OutBlock, TmpBlock, 64 * sizeof(DesBlock));
} /* end DesRotate() */


/*

    Function: DesConvert

    Convert data

    Parameter: OUT DesBlock[32]         Converted bits
               IN  DesBlock[48]         Bits to convert

    Return: no return

*/
static void DesConvert(
        DesBlock OutBlock[32],
        DesBlock InBlock[48])
{
    static DesBlock ConvBits[8][64] =
    {
        {
            14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
             0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
             4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
            15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13
        },
        {
            15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
             3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
             0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
            13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9
        },
        {
            10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
            13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
            13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
             1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12
        },
        {
             7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
            13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
            10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
            03, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14
        },
        {
             2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
            14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
             4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
            11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3
        },
        {
            12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
            10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
             9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
             4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13
        },
        {
             4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
            13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
             1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
             6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12
        },
        {
            13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
             1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
             7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
             2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
        },
    };
    register short jj;
    register short ii;

    for (ii = 0 ; ii < 8 ; ii++)
    {
        jj = 0x20 * InBlock[0] + 0x10 * InBlock[5] + 0x08 * InBlock[1] +
                        0x04 * InBlock[2] + 0x02 * InBlock[3] + InBlock[4];
        InBlock += 6;

        jj = ConvBits[ii][jj];

        OutBlock[ii * 4 + 3] = (DesBlock) jj & 1;
        jj = jj >> 1;
        OutBlock[ii * 4 + 2] = (DesBlock) jj & 1;
        jj = jj >> 1;
        OutBlock[ii * 4 + 1] = (DesBlock) jj & 1;
        jj = jj >> 1;
        OutBlock[ii * 4] = (DesBlock) jj & 1;
    } /* end for */
} /* end DesConvert() */

/*

    Function: DesHexToBlock

    Extract binary representation of data

    Parameter: OUT DesBlock[64]         Binary representation
               IN  BYTE[8]     Data to extract

    Return: no return

*/
void DesHexToBlock(
        DesBlock OutBlock[64],
        BYTE auchInHex[8])
{
    register BYTE uchByte;
    register short ii;

    memset(OutBlock, 0, 64 * sizeof(DesBlock));
    for (ii = 0 ; ii < 8 ; ii++)
    {
        uchByte = auchInHex[ii];

        if (uchByte & 0x80)
        {
            *OutBlock = 1;
        }
        OutBlock++;
        if (uchByte & 0x40)
        {
            *OutBlock = 1;
        }
        OutBlock++;
        if (uchByte & 0x20)
        {
            *OutBlock = 1;
        }
        OutBlock++;
        if (uchByte & 0x10)
        {
            *OutBlock = 1;
        }
        OutBlock++;
        if (uchByte & 0x08)
        {
            *OutBlock = 1;
        }
        OutBlock++;
        if (uchByte & 0x04)
        {
            *OutBlock = 1;
        }
        OutBlock++;
        if (uchByte & 0x02)
        {
            *OutBlock = 1;
        }
        OutBlock++;
        if (uchByte & 0x01)
        {
            *OutBlock = 1;
        }
        OutBlock++;
    } /* end for */
} /* end DesHexToBlock() */

/*

    Function: DesBlockToHex

    Build data out of binary representation

    Parameter: OUT BYTE[8]     Byte reprentation
               IN  DesBlock[64]         Binary reprentation

    Return: no return

*/
void DesBlockToHex(
        BYTE auchOutHex[8],
        DesBlock InBlock[64])
{
    register short ii;

    for (ii = 0; ii < 8; ++ii)
    {
        auchOutHex[ii] = (BYTE) 0x80 * InBlock[0] +
                                0x40 * InBlock[1] +
                                0x20 * InBlock[2] +
                                0x10 * InBlock[3] +
                                0x08 * InBlock[4] +
                                0x04 * InBlock[5] +
                                0x02 * InBlock[6] +
                                InBlock[7];
        InBlock += 8;
    } /* end for */
} /* end DesBlockToHex() */


//**********************************************************************************
//following is added by zlz 20070819

void TripleDes_Encrypt(BYTE *Out, BYTE *In,long datalen,BYTE *Key)
{
	long i,j;

	if( !( Out && In && Key) )
		return;
	// 3次DES 加密:加(key0)-解(key1)-加(key0)
	for(i=0,j=datalen>>3; i<j; ++i,Out+=8,In+=8) {
		DesDes(Out, In, Key, DES_ENCRYPT);
		DesDes(Out, Out, &Key[8], DES_DECRYPT);
		DesDes(Out, Out, Key, DES_ENCRYPT);
	}
}

void TripleDes_Decrypt(BYTE *Out,BYTE *In,long datalen,BYTE *Key)
{
	long i,j;

	if( !( Out && In && Key) )
		return;
	// 3次DES 加密:解(key0)-加(key1)-解(key0)
	for(i=0,j=datalen>>3; i<j; ++i,Out+=8,In+=8) {
		DesDes(Out, In, Key, DES_DECRYPT);
		DesDes(Out, Out, &Key[8], DES_ENCRYPT);
		DesDes(Out, Out, Key, DES_DECRYPT);
	}
}
