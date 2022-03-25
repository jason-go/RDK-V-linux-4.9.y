#include "gx_soft_sm2.h"
#include "gx_soft_sm3.h"
#include "kernelcalls.h"

//shift bits definition in divt
#ifndef DIVT_SHIFT
#define DIVT_SHIFT 2 //2 has the best performance
#if  DIVT_SHIFT == 1
    #define SHIFT_OUT 32
    #define SHIFT_IN 1
#elif  DIVT_SHIFT == 2
    #define SHIFT_OUT 16
    #define SHIFT_IN 2
#elif DIVT_SHIFT == 8
    #define SHIFT_OUT 4
    #define SHIFT_IN 8
#else
    #define SHIFT_OUT 8
    #define SHIFT_IN 4
#endif
#endif //end of "#ifndef DIV_SHIFT"
/**********************************************************************************************************
**
**
**********************************************************************************************************/
extern int add_i, sub_i, mul_i, divt_i, cmp_i, mov_i;
extern int jiami_work;
extern int test_i;
int add_count = 0;
int sub_count = 0;
int mov_count = 0;
int cmp_count = 0;
int mul_count1 = 0;
int mul_count2 = 0;
//struct timeval tv;
//struct timezone tz;
//unsigned int second1,second2,seconds;
//unsigned int usecond1,usecond2,useconds;
/**********************************************************************************************************
**
**
**********************************************************************************************************/
static int  cmp(U32 a1[SMAX],U32 a2[SMAX])
{   	U32 l1, l2;
	int i;
	cmp_count++;
	l1 = a1[SMAX-1];
	l2 = a2[SMAX-1];
	if (l1 > l2)
		return 1;
	if (l1 < l2)
		return -1;
	for(i = (l1 - 1); i >= 0 ;i--)
	{
	   if (a1[i] > a2[i])
		   return 1 ;
	   if (a1[i] < a2[i])
		   return -1;
	}
	return 0;
}
/**********************************************************************************************************
**
**
**********************************************************************************************************/
static void mov(U32 a[SMAX],U32 *b)
{
	int j;
	int dec_max;
	dec_max = a[SMAX-1];
	for (j = 0; j < dec_max; j++)
		b[j] = a[j];

	b[SMAX-2] = a[SMAX-2];
	b[SMAX-1] = a[SMAX-1];
	mov_count++;
	return ;
}
/**********************************************************************************************************
**
**
**********************************************************************************************************/
#if 0
static void mul(U32 a1[SMAX],U32 a2[SMAX],U32 *c)
{
 	int i,j, k, l_mul;
	U32 x;
	U32 l1, l2;
	U32 a1_0, a1_1; //a1_0, lower 16 bits of a1[i]; a1_1, higher 16 bits of a1[i];
	U32 a2_0, a2_1; //a2_0, lower 16 bits of a2[j]; a2_1, higher 16 bits of a2[j];
	U32 p10, p01, p00, p11;
	U32 temp;

	for(i=0; i<SMAX; i++)
	{
		c[i] = 0;
	}

	l1 = a1[SMAX-1];
	l2 = a2[SMAX-1];
    for(i = 0; i < (int)l1; i++)
    {
		a1_0 = a1[i] & 0xFFFF;
        a1_1 = a1[i] >> 16;
        for(j = 0;j < (int)l2; j++)
        {
			a2_0 = a2[j] & 0xFFFF;
			a2_1 = a2[j] >> 16;
			p00 = a2_0 * a1_0;
			p01 = a2_0 * a1_1;
			p10 = a2_1 * a1_0;
			p11 = a2_1 * a1_1;
			mul_count1+=4;
			k = i + j;
			//start to calculate lower 32 bits of multiplication result
			a2_0 = p01 << 16; //a2_0*a1_1
			a2_1 = p10 << 16; //a2_1*a1_0
			x = a2_0 + a2_1;
			//if (a2_0 > (U32)(~a2_1))
			if (a2_0 > x)
			{
				temp = 1;
			}
			else
			{
				temp = 0;
			}
			x = x + p00;
			//if (x > (U32)(~p00))
			if (p00 > x)
			{
				temp = temp + 1;
			}
			//x = x + p00;
			c[k] = c[k] + x;
			//if (c[k] > (U32)(~x))
			if (x > c[k])
			{
				temp = temp + 1;
			}
			//c[k] = c[k] + x;
			k++;
			//start to calculate higher 32 bits of multiplication result
			a2_0 = p01 >> 16;
			a2_1 = p10 >> 16;
			x = a2_0 + a2_1 + temp;

			x = x + p11;
			//if (x > (U32)(~p11))
			if (p11 > x)
			{
				temp = 1;
			}
			else
			{
				temp = 0;
			}
			//x = x + p11;
			c[k] = c[k] + x;
			//if (c[k] > (U32)(~x))
			if (x > c[k])
			{
				temp = temp + 1;
			}
			//c[k] = c[k] + x;
			k++;
     		while (temp != 0)
     		{
				c[k] = c[k] + temp;
				if (temp > c[k])
				{
					//c[k] = c[k] + temp;
					temp = 1;
				}
				else
				{
					//c[k] = c[k] + temp;
					temp = 0;
				}
				k++;
     		}
  		}
 	}
 	l_mul = l1 + l2;
	while((c[l_mul-1]==0) && (l_mul > 0))
	{
 		l_mul--;
	}
	c[SMAX-2] = 0;
	c[SMAX-1] = l_mul;
	mul_count2++;
 	return;
}
#endif
/**********************************************************************************************************
**

**
**********************************************************************************************************/
static void add(U32 a1[SMAX],U32 a2[SMAX],U32 *c)
{
 	int i;
	U32 l1,l2;
 	U32 len, k;
 	k  = 0;
	l1 = a1[SMAX-1];
	l2 = a2[SMAX-1];

	if(l1 < l2)
		len = l1;
 	else
 	 	len = l2;

 	for(i = 0; i < (int)len; i++)
 	{
		c[i] = a1[i] + a2[i];
		if (k == 0)
		{
			if (a1[i] > (~a2[i])) //~a2[i] + a2[i] = 0xffffffff
			{
				k    = 1;
			}
			else
			{
				k    = 0;
			}
		}
		else //k = 1, with carry
		{
			c[i] = c[i] + 1;
			if (a1[i] >= (~a2[i])) //~a2[i] + a2[i] = 0xffffffff
			{
				k    = 1;
			}
			else
			{
				k    = 0;
			}
		}
 	}
 	if(l1 > len)
 	{
  		for(i = (int)len; i < (int)l1; i++)
  		{
			c[i] = a1[i] + k;
			if (a1[i] > (U32)(~k))
			{
				k = 1;
			}
			else
			{
				k = 0;
			}
  		}
  		if(k != 0)
  		{
   			c[l1] = k;
   			len   = l1 + 1;
  		}
  		else
  			len = l1;
 	}
 	else
 	{
  		for(i = (int)len; i < (int)l2; i++)
  		{
			c[i] = a2[i] + k;
			if (a2[i] > (U32)(~k))
			{
				k = 1;
			}
			else
			{
				k = 0;
			}
  		}
  		if(k!=0)
  		{
   			c[l2] = k;
   			len   = l2 + 1;
  		}
  		else
  			len = l2;
 	}
	c[SMAX-2] = 0;
  	c[SMAX-1] = len;

	add_count++;
  	return;
}
/**********************************************************************************************************
**
**
**********************************************************************************************************/
static void sub(U32 a1[SMAX],U32 a2[SMAX],U32 *c)
{

 	int i;
	U32 l1,l2;
 	U32 k, len = 0;
	int in_cmp;
	sub_count++;

	k  = 0;
	l1 = a1[SMAX-1] ;
	l2 = a2[SMAX-1] ;
	in_cmp = cmp(a1, a2);
	//if(cmp(a1,a2)==1)
 	if (in_cmp == 1)
 	{
		c[SMAX-2] = 0;
		len = l2; //小数的长度
 		for(i = 0;i < (int)len; i++)
 		{
			if (k == 0)
			{
				if (a1[i] < a2[i])
				{
					c[i] = ~a2[i];  //0xFFFFFFFF - a2[i]
					c[i] = c[i] + 1;
					c[i] = c[i] + a1[i]; //a2[i]-a1[i]
					k = 1;
				}
				else
				{
					c[i] = a1[i] - a2[i];
					k = 0;
				}
			}
			else //k = 1
			{
				if (a1[i] <= a2[i])
				{
					c[i] = ~a2[i];
					//c[i] = c[i] + 1;
					c[i] = c[i] + a1[i];
					k = 1;
				}
				else
				{
					c[i] = a1[i] - a2[i] - 1;
					k = 0;
				}
			}
 		}

  		for(i = (int)len; i < (int)l1; i++)
  		{
 			if (a1[i] < k)
 			{
    	 		c[i] = ~k;
    	 		c[i] = c[i] + a1[i] + 1;
    	 		k    = 1;
		 	}
    	 	else
		 	{
			 	c[i] = a1[i] - k;
		     	k    = 0;
		 	}
  		}
  		if(c[l1-1] == 0)/*使得数组C中的前面所以0字符不显示了，如1000-20=0980--->显示为980了*/
  		{
		    len = l1 - 1;
			i   = 2;
		  	while (c[l1 - i]==0)/*111456-111450=00006，消除0后变成了6；*/
		  	{
		    	len = l1 - i;
				i++;
		  	}
  		}
  		else
  		{
		  	len = l1;
  		}
 	}
 	else if(in_cmp == -1)
 	{
	 	c[SMAX-2]='-';// c[SMAX-2]为符号位
	 	len = l1;
	 	for(i = 0; i < (int)len;i++)
 		{
			if (k == 0)
			{
				if (a2[i] < a1[i])
				{
					c[i] = ~a1[i];  //0xFFFFFFFF - a1[i]
					c[i] = c[i] + 1;
					c[i] = c[i] + a2[i];
					k = 1;
				}
				else
				{
					c[i] = a2[i] - a1[i];
					k = 0;
				}
			}
			else //k = 1
			{
				if (a2[i] <= a1[i])
				{
					c[i] = ~a1[i];
					//c[i] = c[i] + 1;
					c[i] = c[i] + a2[i];
					k = 1;
				}
				else
				{
					c[i] = a2[i] - a1[i] - 1;
					k = 0;
				}
			}
 		}
  		for(i = (int)len; i < (int)l2; i++)
  		{
 			if (a2[i] < k)
 			{
	 			c[i] = ~k;
	 			c[i] = c[i] + 1 + a2[i];
     			k    = 1;
 			}
     		else
	 		{
		 		c[i] = a2[i] - k;
	     		k    = 0;
	 		}
  		}
  		if(c[l2-1] == 0)
  		{
   			len = l2 - 1;
   			i = 2;
   			while (c[l2-i] == 0)
	  		{
	    		len = l1 - i;
	    		i++;
	  		}
  		}
  		else
  			len = l2;
 	}
	else if(in_cmp == 0)
	{
	   len      = 1;
	   c[len-1] = 0;
	   c[SMAX-2] = 0;
	}
	c[SMAX-1] = len;
	return;
}
#if 0
static void  divt(U32 t[SMAX],U32 b[SMAX],U32  *c ,U32 *w)/*//试商法//调用以后w为a mod b, C为a  div b;*/
{
	U32 a1,b1,m;/*w用于暂时保存数据*/
	int i,j;
	int k;
	U32 tmp_max;
	/*************************/
	U32 e[SMAX],f[SMAX],g[SMAX],a[SMAX];
	U32 *a_ptr, *f_ptr, *tmp_ptr;
	U32 temp;
	mov(t,a);
	for(i=0;i<SMAX;i++)
		c[i]=0;
	for(i=0;i<SMAX;i++)
		e[i]=0;
	for(i=0;i<SMAX;i++)
		g[i]=0;

	a1 = a[SMAX - 1];
	b1 = b[SMAX - 1];
	k = cmp(a, b);
	if (k ==(-1))
	{
		c[0]=0;
		c[SMAX-1]=1;
		c[SMAX-2]=0;
		mov(t,w);
		return;
	}
	else if (k==0)
	{
		c[0]=1;
		c[SMAX-2]=0;
		c[SMAX-1]=1;
		w[0]=0;
		w[SMAX-2]=0;
		w[SMAX-1]=1;
		return;
	}
	m=(a1 - b1);
	mov(b,g);
	tmp_max = g[SMAX-1];
	a_ptr = a;
	f_ptr = f;

	//mov divisor left to have same digits with diviend
	k = m;
	for (i = 0; i < (int)tmp_max; i++)
	{
		e[k] = g[i];
		k++;
	}
	e[SMAX - 1] = k;

	for(i = m;i >= 0; i--)/*341245/3=341245-300000*1--->41245-30000*1--->11245-3000*3--->2245-300*7--->145-30*4=25--->25-3*8=1*/
	{
	    for (j = 0; j < SHIFT_OUT; j++)  //SHIFT_IN * SHIFT_OUT = 32
	    {
	        while (cmp(a_ptr, e)!=(-1))
		    {

		    	c[i]++; //this is wrong, but dont afect the results
				sub(a_ptr,e,f_ptr);
		    	tmp_ptr = a_ptr;
		    	a_ptr = f_ptr;
		    	f_ptr = tmp_ptr;
		    }
		    if (i > 0)
			{   //shift right SHIFT_IN bits
		    	int si;
		        for (si = 0; si < (int)(e[SMAX-1]-1); si++)
		        {
		            e[si] = ((e[si+1] & 0xf) << (32-SHIFT_IN)) | (e[si] >> SHIFT_IN);
		        }
		        e[si] = e[si] >> SHIFT_IN;
		    }
	    }
	    e[SMAX-1] = e[SMAX-1] - 1;
	}
	mov(a_ptr,w);
	if (c[m] == 0)
	{
		c[SMAX-2]=0;
		c[SMAX-1]=m;
	}
	else
	{
		temp = m + 1;
		c[SMAX-2]=0;
		c[SMAX-1]=temp;
	}

	return;
}
#endif

/**********************************************************************************************************
**
**
**********************************************************************************************************/
static void rshift(unsigned int *data, unsigned int si) // shift right "si" bits
{
	int i;
	for (i = 0; i < (int)(data[SMAX-1]-1); i++)
	{
	    data[i] = ((data[i+1] & 0xf) << (32-si)) | (data[i] >> si);
	}
	data[i] = data[i] >> si;

	return;
}

static void lshift(unsigned int *data, unsigned int si) // shift left "si" bits
{
	int i;
	for (i = (int)(data[SMAX-1]); i > 0; i--)
	{
	    data[i] = ((data[i-1] & 0xf0000000) >> (32-si)) | (data[i] << si);
	}
	data[i] = data[i] << si;

	return;
}

static void delZero(U32 a[SMAX])
{
	int i;

	for(i=a[SMAX-1]-1; i>=0; i--)
	{
		if(a[i] == 0)
			a[SMAX-1]--;
		else
			break;
	}
}

// to change the order of int array: {int[0],int[1],int[2]}->{int[2],int[1],int[0]}
static void intorder(unsigned int *data, unsigned int datalen)
{
	unsigned int tmp;
	unsigned int i;

	for(i=0; i<datalen/2; i++)
	{
		tmp = data[i];
		data[i] = data[datalen-1-i];
		data[datalen-1-i] = tmp;
	}
}

// int array -> char array
static void int2char(unsigned int *intarray, unsigned int datalen, unsigned char *chararray)
{
	unsigned int tmp;
	unsigned int i,j;

	for(i=0; i<datalen; i++)
	{
		tmp = intarray[i];
		for(j=0; j<4; j++)
		{
			chararray[i*4+j] = (char)((tmp&0xff000000)>>24);
			tmp = tmp<<8;
		}
	}
}

// char array -> int array
static void char2int(unsigned char *chararray, unsigned int datalen, unsigned int *intarray)
{
	unsigned int i,j;
	unsigned int tmp = 0;

	for(i=0; i<datalen/4; i++)
	{
		for(j=0; j<4; j++)
		{
			tmp = tmp<<8;
			tmp = (tmp&0xffffff00)^((int)chararray[i*4+j]);
		}
		intarray[i] = tmp;
	}
}

// merge to char array
static void mergechar(unsigned char *array1, unsigned int array1_len, unsigned char *array2, unsigned int array2_len, unsigned char *array3)
{
	unsigned int i;
	unsigned int l;

	l = 0;
	for(i=0; i<array1_len; i++)
	{
		array3[l] = array1[i];
		l++;
	}

	for(i=0; i<array2_len; i++)
	{
		array3[l] = array2[i];
		l++;
	}
}

static void KDF(unsigned char *z, unsigned int zlen, unsigned char *k, unsigned int klen)
{
	unsigned int i,j,m;
	unsigned int times;
	unsigned int kcnt = 0;
	unsigned int left;
	unsigned char dgst[SM3_DIGEST_LENGTH];
	unsigned int ct = 0x1;
	unsigned int ct_tmp;

	left = klen%(SM3_DIGEST_LENGTH*8)/8;

	if(klen%(SM3_DIGEST_LENGTH*8) != 0)
		times = (klen/(SM3_DIGEST_LENGTH*8))+1;
	else
		times = (klen/(SM3_DIGEST_LENGTH*8));

	for(i=1; i<=times; i++)
	{
		//to concatenate ct
		ct_tmp = ct;
		for(j=0; j<4; j++)
		{
			z[zlen+j] = (char)((ct_tmp&0xff000000)>>24);
			ct_tmp = ct_tmp<<8;
		}

		//Hash
		gx_soft_sm3(z,zlen+4,dgst);
		if(i != times)
		{
			for(m=0; m<SM3_DIGEST_LENGTH; m++)
			{
				k[kcnt] = dgst[m];
				kcnt++;
			}
		}
		else if(klen%(SM3_DIGEST_LENGTH*8) == 0)
		{
			for(m=0; m<SM3_DIGEST_LENGTH; m++)
			{
				k[kcnt] = dgst[m];
				kcnt++;
			}
		}
		else
		{
			for(m=0; m<left; m++)
			{
				k[kcnt] = dgst[m];
				kcnt++;
			}
		}

		// ct
		ct++;
	}
}

static void modadd(U32 a[SMAX], U32 b[SMAX], U32 n[SMAX], U32 *m) // (a+b) mod n
{
	U32 tmp1[SMAX],tmp2[SMAX];

	add(a,b,tmp1);
	while(cmp(tmp1,n) == 1)
	{
		sub(tmp1,n,tmp2);
		mov(tmp2,tmp1);
	}
	mov(tmp1,m);
}

static void modsub(U32 a[SMAX], U32 b[SMAX], U32 n[SMAX], U32 *m) // (a-b) mod n
{
	U32 tmp1[SMAX],tmp2[SMAX];

	if(cmp(a,b) != -1)
	{
		sub(a,b,tmp1);
	}
	else
	{
		sub(b,a,tmp2);
		sub(n,tmp2,tmp1);
	}
	mov(tmp1,m);
}

static void modmul(U32 a[SMAX], U32 b[SMAX], U32 n[SMAX], U32 *m) // (a*b) mod n
{
	U32 c[SMAX];
	U32 t1[SMAX], t2[SMAX], t3[SMAX];
	U32 zero[SMAX];
	unsigned int len, inx;
	int i;

	for(i=0; i<SMAX-1; i++)
	{
		zero[i] = 0;
	}
	zero[SMAX-1] = 1;
	zero[0] = 0;

	// get bit-length of a
	mov(a,t1);
	len = 32*t1[SMAX-1];
	inx = t1[SMAX-1]-1;
	while(!(t1[inx] & 0x80000000))
	{
		len--;
		lshift(t1,1);
	}

	mov(a,t1); // t1=a
	mov(b,t2); // t2=b
	mov(zero,c);
	for(i=0; i<len; i++)
	{
		if(t1[0]&0x1)
		{
			modadd(c,t2,n,t3); // c=c+b (mod n)
			mov(t3,c);
		}

		rshift(t1,1); // a>>1

		modadd(t2,t2,n,t3); // b=2*b (mod n)
		mov(t3,t2);
	}
	mov(c,m);
}

static void modinv(U32 a[SMAX], U32 b[SMAX], U32 p[SMAX], U32 *m) // (a^-1)*b (mod p)
{
	U32 u[SMAX], v[SMAX];
	U32 x1[SMAX], x2[SMAX];
	U32 zero[SMAX],one[SMAX];
	U32 tmp1[SMAX],tmp2[SMAX];
	int i;

	for(i=0; i<SMAX-1; i++)
	{
		zero[i] = 0;
	}
	zero[SMAX-1] = 1;
	zero[0] = 0;

	for(i=0; i<SMAX-1; i++)
	{
		one[i] = 0;
	}
	one[SMAX-1] = 1;
	one[0] = 1;

	mov(a,u); // u=a
	mov(p,v); // v=p
	mov(b,x1);  // x1=b
	mov(zero,x2); // x2=0
	while((cmp(u,one) != 0) && (cmp(v,one) != 0)) // u!=1 && v!=1
	{
		while((u[0]&0x1) == 0) // u is even
		{
			rshift(u,1); // u=u>>1
			delZero(u);

			if((x1[0]&0x1) == 0) // x1 is even
			{
				rshift(x1,1); // x1=x1>>1
				delZero(x1);
			}
			else
			{
				add(x1,p,tmp1); // x1=(x1+p)>>1
				rshift(tmp1,1);
				delZero(tmp1);
				mov(tmp1,x1);
			}
		}
		while((v[0]&0x1) == 0) // v is even
		{
			rshift(v,1); // v=v>>1
			delZero(v);

			if((x2[0]&0x1) == 0) // x2 is even
			{
				rshift(x2,1); // x2=x2>>1
				delZero(x2);
			}
			else
			{
				add(x2,p,tmp1);	// x2=(x2+p)>>1
				rshift(tmp1,1);
				delZero(tmp1);
				mov(tmp1,x2);
			}
		}

		if(cmp(u,v) != -1) // u>=v
		{
			sub(u,v,tmp1);  // u=(u-v)>>1
			rshift(tmp1,1);
			delZero(tmp1);
			mov(tmp1,u);

			if(cmp(x1,x2) == 1) // x1>x2
			{
				sub(x1,x2,tmp1);
				if(!((x1[0]&0x1) ^ (x2[0]&0x1))) // x1,x2 both even, both odd
				{
					rshift(tmp1,1); // x1=(x1-x2)>>1
					delZero(tmp1);
					mov(tmp1,x1);
				}
				else
				{
					add(tmp1,p,tmp2); // x1=((x1-x2)+p)>>1
					rshift(tmp2,1);
					delZero(tmp2);
					mov(tmp2,x1);
				}
			}
			else
			{
				sub(x2,x1,tmp1);
				if(!((x1[0]&0x1) ^ (x2[0]&0x1))) // x1,x2 both even, both odd
				{
					rshift(tmp1,1); // x1=p-((x2-x1)>>1)
					delZero(tmp1);
					sub(p,tmp1,x1);
				}
				else
				{
					sub(p,tmp1,tmp2); // x1=(p-(x2-x1))>>1
					rshift(tmp2,1);
					delZero(tmp2);
					mov(tmp2,x1);
				}

			}
		}
		else
		{
			sub(v,u,tmp1);  // v=v-u>>1
			rshift(tmp1,1);
			delZero(tmp1);
			mov(tmp1,v);

			if(cmp(x1,x2) == -1) // x1<x2
			{
				sub(x2,x1,tmp1);
				if(!((x1[0]&0x1) ^ (x2[0]&0x1))) // x1,x2 both even, both odd
				{
					rshift(tmp1,1); // x2=(x2-x1)>>1
					delZero(tmp1);
					mov(tmp1,x2);
				}
				else
				{
					add(tmp1,p,tmp2); // x2=((x2-x1)+p)>>1
					rshift(tmp2,1);
					delZero(tmp2);
					mov(tmp2,x2);
				}
			}
			else
			{
				sub(x1,x2,tmp1);
				if(!((x1[0]&0x1) ^ (x2[0]&0x1))) // x1,x2 both even, both odd
				{
					rshift(tmp1,1); // x2=p-((x1-x2)>>1)
					delZero(tmp1);
					sub(p,tmp1,tmp2);
					mov(tmp2,x2);
				}
				else
				{
					sub(p,tmp1,tmp2); // x2=(p-(x1-x2))>>1
					rshift(tmp2,1);
					delZero(tmp2);
					mov(tmp2,x2);
				}
			}
		}
	}

	if(cmp(u,one) == 0) // u==1
		mov(x1,m);
	else
		mov(x2,m);
}

#if 0
void modmul(U32 a[SMAX], U32 b[SMAX], U32 n[SMAX], U32 *m) // (a*b) mod n
{
	U32 p[SMAX], q[SMAX], r[SMAX];

	mul(a,b,p);
 	divt(p,n,q,r);
 	mov (r,m);
}

void modinv(U32 a[SMAX], U32 p[SMAX], U32 *m) // (a^-1) mod p
{
	U32 t1[SMAX], t2[SMAX];
	U32 one[SMAX], two[SMAX];
	U32 q[SMAX], r[SMAX];
	U32 e[SMAX];
	U32 tmp[SMAX];
	U32 len, inx;
	U32 new[SMAX];
	int i;

 	for(i=0; i<SMAX-1; i++)// constant 1
 	{
 	  one[i] = 0x0;
 	}
 	one[SMAX-1] = 0x1;
 	one[0] = 0x1;

 	for(i=0; i<SMAX-1; i++)// constant 2
 	{
 	  two[i] = 0x0;
 	}
 	two[SMAX-1] = 0x1;
 	two[0] = 0x2;

	sub(p,one,t1);
	sub(p,two,t2);

	// e=a mod (n-1)
 	divt(t2,t1,q,r);
	mov(r,e);
	// get bits of 'e'
	mov(r,tmp);
	len = 32*tmp[SMAX-1];
	inx = tmp[SMAX-1]-1;
	while(!(tmp[inx] & 0x80000000))
	{
		len--;
		lshift(tmp,1);
	}
	// main loop
	mov(a,new);
	for(i=len-2;i>=0;i--)
	{
		modmul(new,new,p,tmp);
		mov(tmp,new);
		if((e[i/32]>>(i%32))&0x1)
		{
			modmul(new,a,p,tmp);
			mov(tmp,new);
		}
	}
	mov(new,m);

}
#endif

static void PointAdd(U32 x1[SMAX], U32 y1[SMAX], U32 x2[SMAX], U32 y2[SMAX], U32 p[SMAX], U32 *x3, U32 *y3) //
{
	U32 r[SMAX];
	U32 tmp1[SMAX], tmp2[SMAX], tmp3[SMAX];
	U32 zero[SMAX];
	int i;

	for(i=0;i<SMAX-1;i++)
	{
		zero[i] = 0;
	}
	zero[SMAX-1] = 1;

	if((cmp(x1,zero) == 0) && (cmp(y1,zero) == 0)) // x1==0 && y1==0
	{
		mov(x2,x3);
		mov(y2,y3);
		return;
	}
	if((cmp(x2,zero) == 0) && (cmp(y2,zero) == 0)) // x2==0 && y2==0
	{
		mov(x1,x3);
		mov(y1,y3);
		return;
	}
 	// r
	modsub(y2,y1,p,tmp1);
	modsub(x2,x1,p,tmp2);
	modinv(tmp2,tmp1,p,r);
	//modinv(tmp2,p,tmp3);
	//modmul(tmp1,tmp3,p,r);
	// x3
	modmul(r,r,p,tmp1);
	modsub(tmp1,x1,p,tmp2);
	modsub(tmp2,x2,p,tmp3);
	mov(tmp3,x3);
	// y3
	modsub(x1,tmp3,p,tmp1);
	modmul(r,tmp1,p,tmp2);
	modsub(tmp2,y1,p,tmp3);
	mov(tmp3,y3);
}

static void PointDouble(U32 x1[SMAX], U32 y1[SMAX], U32 p[SMAX], U32 a[SMAX], U32 *x3, U32 *y3) //
{
	U32 r[SMAX];
	U32 tmp1[SMAX], tmp2[SMAX], tmp3[SMAX];

	// r
	modmul(x1,x1,p,tmp1);
	modadd(tmp1,tmp1,p,tmp2);
	modadd(tmp2,tmp1,p,tmp3);
	modadd(tmp3,a,p,tmp1);
	modadd(y1,y1,p,tmp2);
	modinv(tmp2,tmp1,p,r);
	//modinv(tmp2,p,tmp3);
	//modmul(tmp1,tmp3,p,r);
	// x3
	modmul(r,r,p,tmp1);
	modsub(tmp1,x1,p,tmp2);
	modsub(tmp2,x1,p,tmp3);
	mov(tmp3,x3);
	// y3
	modsub(x1,tmp3,p,tmp1);
	modmul(r,tmp1,p,tmp2);
	modsub(tmp2,y1,p,tmp3);
	mov(tmp3,y3);
}

static void PointMul(U32 x1[SMAX], U32 y1[SMAX], U32 p[SMAX], U32 a[SMAX], U32 k[SMAX],  U32 *x3, U32 *y3) //
{
	U32 tmp[SMAX];
	U32 tmp1_x[SMAX], tmp1_y[SMAX];
	U32 tmp2_x[SMAX], tmp2_y[SMAX];
	U32 tmp3_x[SMAX], tmp3_y[SMAX];
	U32 tmp4_x[SMAX], tmp4_y[SMAX];
	U32 len, inx;
	int i;

	for(i=0; i<SMAX-1; i++)//
 	{
 	  tmp1_x[i] = 0x0;
 	  tmp1_y[i] = 0x0;
 	}
 	  tmp1_x[SMAX-1] = 0x1;
 	  tmp1_y[SMAX-1] = 0x1;

	// bits of 'k'
	mov(k,tmp);
	len = 32*tmp[SMAX-1];
	inx = tmp[SMAX-1]-1;
	while(!(tmp[inx] & 0x80000000))
	{
		len--;
		lshift(tmp,1);
	}

	// main loop
	mov(k,tmp);
	mov(x1,tmp4_x);
	mov(y1,tmp4_y);
	for(i=len-1;i>=0;i--)
	{
		if(tmp[0]&0x1)
		{
			PointAdd(tmp1_x,tmp1_y,tmp4_x,tmp4_y,p,tmp3_x,tmp3_y); // Q=Q+P
			mov(tmp3_x,tmp1_x);
			mov(tmp3_y,tmp1_y);
		}

		PointDouble(tmp4_x,tmp4_y,p,a,tmp2_x,tmp2_y); // P=2P
		mov(tmp2_x,tmp4_x);
		mov(tmp2_y,tmp4_y);

		rshift(tmp,1);
	}
	mov(tmp1_x,x3);
	mov(tmp1_y,y3);
}

void gx_soft_sm2_signature(U32 p[SMAX], U32 a[SMAX], U32 b[SMAX], U32 n[SMAX], U32 xG[SMAX], U32 yG[SMAX], U32 k[SMAX], U32 d[SMAX], U32 e[SMAX], U32 *r, U32 *s)
{
	U32 tmp_x[SMAX], tmp_y[SMAX];
	U32 tmp_r[SMAX];
	U32 tmp1[SMAX], tmp2[SMAX], tmp3[SMAX];
	U32 one[SMAX];
	int i;
	(void) b;

 	for(i=0; i<SMAX-1; i++)// constant 1
 	{
 	  one[i] = 0x0;
 	}
 	one[SMAX-1] = 0x1;
 	one[0] = 0x1;
	// kG
	PointMul(xG,yG,p,a,k,tmp_x,tmp_y);
	// r
	modadd(e,tmp_x,n,tmp_r);
	mov(tmp_r,r);
	// s
	modmul(tmp_r,d,n,tmp1);
	modsub(k,tmp1,n,tmp2);
	add(d,one,tmp3);
	modinv(tmp3,tmp2,n,s);
}

int  gx_soft_sm2_verify(U32 p[SMAX], U32 a[SMAX], U32 b[SMAX], U32 n[SMAX], U32 xG[SMAX], U32 yG[SMAX], U32 xP[SMAX], U32 yP[SMAX], U32 e[SMAX], U32 r[SMAX], U32 s[SMAX])
{
	U32 tmp1_x[SMAX], tmp1_y[SMAX];
	U32 tmp2_x[SMAX], tmp2_y[SMAX];
	U32 tmp3_x[SMAX], tmp3_y[SMAX];
	U32 t[SMAX];
	U32 R[SMAX];
	(void) b;

	modadd(r,s,n,t);
	PointMul(xG,yG,p,a,s,tmp1_x,tmp1_y);
	PointMul(xP,yP,p,a,t,tmp2_x,tmp2_y);
	PointAdd(tmp1_x,tmp1_y,tmp2_x,tmp2_y,p,tmp3_x,tmp3_y);
	modadd(e,tmp3_x,n,R);

	if(cmp(R,r) == 0)
		return 1;
	else
		return -1;
}

// mlen: bytes
void gx_soft_sm2_encrypt(U32 p[SMAX], U32 a[SMAX], U32 b[SMAX], U32 k[SMAX], U32 xG[SMAX], U32 yG[SMAX], U32 xP[SMAX], U32 yP[SMAX], unsigned char *m, unsigned int mlen, unsigned char *cipher_text)
{
//	U32 *tmp;
	U32 x1_tmp[SMAX], y1_tmp[SMAX];
	U32 x2_tmp[SMAX], y2_tmp[SMAX];
	unsigned char x1_tmp_c[SM2_LEN*4], y1_tmp_c[SM2_LEN*4];
	unsigned char x2_tmp_c[SM2_LEN*4], y2_tmp_c[SM2_LEN*4];
	unsigned char *t;
	unsigned char *C2, *C3;
	unsigned char *databuf1, *databuf2;
	unsigned int datalen; //byte length
	unsigned char dgst[SM3_DIGEST_LENGTH];
	(void) b;

	t = gx_malloc(mlen);
	C2 = gx_malloc(mlen);

	// kG
	PointMul(xG,yG,p,a,k,x1_tmp,y1_tmp);
	// kP
	PointMul(xP,yP,p,a,k,x2_tmp,y2_tmp);

	// KDF
	datalen = SM2_LEN*4*2+4; // x2+y2+ct
	databuf1 = gx_malloc(datalen);

	intorder(x2_tmp,SM2_LEN);
	intorder(y2_tmp,SM2_LEN);
	int2char(x2_tmp,SM2_LEN,x2_tmp_c);
	int2char(y2_tmp,SM2_LEN,y2_tmp_c);
	mergechar(x2_tmp_c,SM2_LEN*4,y2_tmp_c,SM2_LEN*4,databuf1);
	KDF(databuf1, SM2_LEN*4*2, t, mlen*8);

	// M ^ t
	xor(m,t,mlen,C2);

	// hash(x2 || M || y2)
	datalen = SM2_LEN*4*2+mlen;
	databuf2 = gx_malloc(datalen);
	mergechar(x2_tmp_c,SM2_LEN*4,m,mlen,databuf2);
	mergechar(databuf2,SM2_LEN*4+mlen,y2_tmp_c,SM2_LEN*4,databuf2);

	gx_soft_sm3(databuf2,datalen,dgst);
	C3 = dgst;

	// cipher_text
	// C1
	intorder(x1_tmp,SM2_LEN);
	intorder(y1_tmp,SM2_LEN);
	int2char(x1_tmp,SM2_LEN,x1_tmp_c);
	int2char(y1_tmp,SM2_LEN,y1_tmp_c);
	mergechar(x1_tmp_c,SM2_LEN*4,y1_tmp_c,SM2_LEN*4,cipher_text);
	// C3
	mergechar(cipher_text,SM2_LEN*4*2,C3,SM3_DIGEST_LENGTH,cipher_text);
	// C2
	mergechar(cipher_text,SM2_LEN*4*2+SM3_DIGEST_LENGTH,C2,mlen,cipher_text);

	gx_free(databuf1); databuf1 = NULL;
	gx_free(databuf2); databuf2 = NULL;
	gx_free(C2); C2 = NULL;
	gx_free(t); t = NULL;
}

int gx_soft_sm2_decrypt(U32 p[SMAX], U32 a[SMAX], U32 b[SMAX], U32 d[SMAX], unsigned int mlen, unsigned char *cipher_text, unsigned char *plain_text)
{
	unsigned int i;
	unsigned char C1_x[SM2_LEN*4], C1_y[SM2_LEN*4];
	unsigned char x2_c[SM2_LEN*4], y2_c[SM2_LEN*4];
	unsigned int xC1[SMAX], yC1[SMAX];
	unsigned int x2[SMAX], y2[SMAX];
	unsigned char *databuf1, *databuf2;
	unsigned int datalen;
	unsigned char *t;
	unsigned char *C2;
	unsigned char dgst[SM3_DIGEST_LENGTH],u[SM3_DIGEST_LENGTH];
	unsigned int  dgst_i[SMAX], u_i[SMAX];
	(void) b;

	// get xC1, yC1;
	for(i=0; i<SM2_LEN*4; i++)
	{
		C1_x[i] = cipher_text[i+1];
	}
	for(i=0; i<SM2_LEN*4; i++)
	{
		C1_y[i] = cipher_text[i+32+1];
	}

	char2int(C1_x,SM2_LEN*4,xC1); xC1[SMAX-1] = 0x8; xC1[SMAX-2] = 0x0;
	char2int(C1_y,SM2_LEN*4,yC1); yC1[SMAX-1] = 0x8; yC1[SMAX-2] = 0x0;
	intorder(xC1,SM2_LEN);
	intorder(yC1,SM2_LEN);

	// dC1
	PointMul(xC1,yC1,p,a,d,x2,y2);

	// KDF
	intorder(x2,SM2_LEN);
	intorder(y2,SM2_LEN);
	int2char(x2,SM2_LEN,x2_c);
	int2char(y2,SM2_LEN,y2_c);

	datalen = SM2_LEN*4*2+4;
	databuf1 = gx_malloc(datalen);
	mergechar(x2_c,SM2_LEN*4,y2_c,SM2_LEN*4,databuf1);
	t = gx_malloc(mlen);

	KDF(databuf1, SM2_LEN*4*2, t, mlen*8);

	// get C2
	C2 = gx_malloc(mlen);
	for(i=0; i<mlen; i++)
	{
		C2[i] = cipher_text[1+SM2_LEN*4*2+SM3_DIGEST_LENGTH+i];
	}

	// get plain text
	xor(C2,t,mlen,plain_text);

	// hash
	for(i=0; i<SM3_DIGEST_LENGTH; i++)
	{
		u[i] = cipher_text[1+SM2_LEN*4*2+i];
	}
	char2int(u,SM3_DIGEST_LENGTH,u_i); u_i[SMAX-1] = 0x0; u_i[SMAX-2] = 0x0;

	datalen = SM2_LEN*4*2+mlen;
	databuf2 = gx_malloc(datalen);
	mergechar(x2_c,SM2_LEN*4,plain_text,mlen,databuf2);
	mergechar(databuf2,SM2_LEN*4+mlen,y2_c,SM2_LEN*4,databuf2);
	gx_soft_sm3(databuf2,datalen,dgst);
	char2int(dgst,SM3_DIGEST_LENGTH,dgst_i); dgst_i[SMAX-1] = 0x0; dgst_i[SMAX-2] = 0x0;

	gx_free(databuf1); databuf1 = NULL;
	gx_free(databuf2); databuf2 = NULL;
	gx_free(C2); C2 = NULL;
	gx_free(t); t = NULL;

	if (cmp(u_i,dgst_i))
		return -1;
	return 0;
}
