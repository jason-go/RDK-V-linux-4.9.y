#include "crc32.h"

static unsigned int table[256];
static unsigned int crc = 0xFFFFFFFF;
static int s_table_empty = 1;

static unsigned int bitrev(unsigned int input, int bw)
{
    int i;
    unsigned int var;
    var = 0;
    for(i=0;i<bw;i++)
    {
        if(input & 0x01)
        {
            var |= 1<<(bw-1-i);
        }
        input>>=1;
    }
    return var;
}

//X32+X26+...X1+1,poly=(1<<26)|...|(1<<1)|(1<<0)
static void make_crc_table(void)
{
    int i;
    int j;
    unsigned int c, poly;

    poly=bitrev(0x4C11DB7,32);
    for(i=0; i<256; i++)
    {
        c = i;
        for (j=0; j<8; j++)
        {
            if(c&1)
            {
                c=poly^(c>>1);
            }
            else
            {
                c=c>>1;
            }
        }
        table[i] = c;
	//	printf("%08x\n",table[i]);
    }
	crc = 0XFFFFFFFF;
	s_table_empty = 0;
}

unsigned int gxse_common_crc32(unsigned char* input, int len)
{
    int i;
    unsigned char index;

	if (s_table_empty)
		make_crc_table();

    for (i=0; i<len; i++) {
        index = (unsigned char)(crc^*(input+i));
        crc = (crc>>8)^table[index];
    }
    return crc^0xFFFFFFFF;
}

