#include "minifs.h"
#include "compr.h"

/* Available compressors are on this list */

extern struct minifs_compressor minifs_zlib_comp;

int minifs_compress(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen)
{
	struct minifs_compressor *this;
	this = &minifs_zlib_comp;

	return this->compress(dest, destLen, source, sourceLen);
}

int minifs_uncompress(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen)
{
	struct minifs_compressor *this = &minifs_zlib_comp;

	return this->uncompress(dest, destLen, source, sourceLen);
}

uint32_t crc32 (uint32_t crc, const uint8_t *buf, uint32_t len);
static int crc_table_empty = 1;
static uint32_t crc_table[256];
static void make_crc_table (void);

/*
   Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
   x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

   Polynomials over GF(2) are represented in binary, one bit per coefficient,
   with the lowest powers in the most significant bit.  Then adding polynomials
   is just exclusive-or, and multiplying a polynomial by x is a right shift by
   one.  If we call the above polynomial p, and represent a byte as the
   polynomial q, also with the lowest power in the most significant bit (so the
   byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
   where a mod b means the remainder after dividing a by b.

   This calculation is done using the shift-register method of multiplying and
   taking the remainder.  The register is initialized to zero, and for each
   incoming bit, x^32 is added mod p to the register if the bit is a one (where
   x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
   x (which is shifting right by one and adding x^32 mod p if the bit shifted
   out is a one).  We start with the highest power (least significant bit) of
   q and repeat for all eight bits of q.

   The table is simply the CRC of all possible eight bit values.  This is all
   the information needed to generate CRC's on data a byte at a time for all
   combinations of CRC register values and incoming bytes.
   */
static void make_crc_table()
{
	uint32_t c;
	int n, k;
	unsigned long poly;		/* polynomial exclusive-or pattern */
	/* terms of polynomial defining this crc (except x^32): */
	static const uint8_t p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

	/* make exclusive-or pattern from polynomial (0xedb88320L) */
	poly = 0L;
	for (n = 0; n < sizeof(p)/sizeof(uint8_t); n++)
		poly |= 1L << (31 - p[n]);

	for (n = 0; n < 256; n++)
	{
		c = (unsigned long)n;
		for (k = 0; k < 8; k++)
			c = c & 1 ? poly ^ (c >> 1) : c >> 1;
		crc_table[n] = c;
	}
	crc_table_empty = 0;
}

/* ========================================================================= */
#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================= */
uint32_t minifs_crc32(uint32_t crc, const uint8_t *buf, size_t len)
{
	if (crc_table_empty)
		make_crc_table();

	crc = crc ^ 0xffffffffL;
	while (len >= 8)
	{
		DO8(buf);
		len -= 8;
	}
	if (len) do {
		DO1(buf);
	} while (--len);
	return crc ^ 0xffffffffL;
}

