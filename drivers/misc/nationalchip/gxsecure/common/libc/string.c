#include "stdio.h"
#include "types.h"
#include "string.h"

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (long) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long))

#define UNALIGNED1(X)   ((long)X & (LITTLEBLOCKSIZE - 1))
#define TOO_SMALL(LEN) ((LEN) < LITTLEBLOCKSIZE)
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED2(X, Y) \
	(((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

int strlen(const char *str)
{
       const char *start = str;
       unsigned long *aligned_addr;

       if (!UNALIGNED1 (str))
       {
               /* If the string is word-aligned, we can check for the presence of
                  a null in each word-sized block.  */
               aligned_addr = (unsigned long*)str;
               while (!DETECTNULL (*aligned_addr))
                       aligned_addr++;

               /* Once a null is detected, we check each byte in that block for a
                  precise position of the null.  */
               str = (char*)aligned_addr;
       }

       while (*str)
               str++;
       return str - start;
}



/* Threshhold for punting to the byte copier.  */
void *memset(void *m, int c, size_t n)
{
	char *s = (char *) m;
	unsigned int d = c & 0xff;	/* To avoid sign extension, copy C to an
					   unsigned variable.  */

#ifndef CPU_SCPU
	int i;
	unsigned long buffer;
	unsigned long *aligned_addr;

	if (!TOO_SMALL (n) && !UNALIGNED1 (m))
	{
		/* If we get this far, we know that n is large and m is word-aligned. */
		aligned_addr = (unsigned long*)m;

		/* Store D into each char sized location in BUFFER so that
		   we can set large blocks quickly.  */
		if (LITTLEBLOCKSIZE == 4)
		{
			buffer = (d << 8) | d;
			buffer |= (buffer << 16);
		}
		else
		{
			buffer = 0;
			for (i = 0; i < LITTLEBLOCKSIZE; i++)
				buffer = (buffer << 8) | d;
		}

		while (n >= LITTLEBLOCKSIZE*4)
		{
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			n -= 4*LITTLEBLOCKSIZE;
		}

		while (n >= LITTLEBLOCKSIZE)
		{
			*aligned_addr++ = buffer;
			n -= LITTLEBLOCKSIZE;
		}
		/* Pick up the remainder with a bytewise loop.  */
		s = (char*)aligned_addr;
	}
#endif

	while (n--)
	{
		*s++ = (char)d;
	}

	return m;
}

void *memcpy(void *dst0, const void *src0, size_t len0)
{
	char *dst = dst0;
	const char *src = src0;
	int   len =  len0;

#ifndef CPU_SCPU
	long *aligned_dst;
	const long *aligned_src;

	/* If the size is small, or either SRC or DST is unaligned,
	   then punt into the byte copy loop.  This should be rare.  */
	if (!TOO_SMALL(len) && !UNALIGNED2 (src, dst))
	{
		aligned_dst = (long*)dst;
		aligned_src = (long*)src;

		/* Copy 4X long words at a time if possible.  */
		while (len >= BIGBLOCKSIZE)
		{
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			len -= BIGBLOCKSIZE;
		}

		/* Copy one long word at a time if possible.  */
		while (len >= LITTLEBLOCKSIZE)
		{
			*aligned_dst++ = *aligned_src++;
			len -= LITTLEBLOCKSIZE;
		}

		/* Pick up any residual with a byte copier.  */
		dst = (char*)aligned_dst;
		src = (char*)aligned_src;
	}
#endif

	while (len--)
		*dst++ = *src++;

	return dst0;
}

int memcmp(const void *cs, const void *ct, size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}

void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *p = s;
	while (n-- != 0) {
		if ((unsigned char)c == *p++) {
			return (void *)(p - 1);
		}
	}
	return NULL;
}
