/*****************************************
  Copyright (c) 2003-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/* This file is part of the boot loader */

/*
 * vsprintf.c
 *
 * Simple implemention of vsprintf()
 *
 * Written by Ho Lee 11/17/2003
 *
 * Supported directives :
 *   conversion specifier :
 *     %c : character
 *     %d : decimal
 *     %u : unsigned decimal
 *     %x, %X : hexadecimal
 *     %p : pointer (%08x)
 *     %s : string (pointer to array of characters)
 *     %% : '%'
 *   flag characters :
 *     0 : zero padded
 *   field width :
 *     number : width
 *     * : width given in the next argument
 *   precision : not supported
 *   length modifier :
 *     l : long integer or unsigned long integer (no difference)
 */
#include "stdio.h"
#include "string.h"
#include "util.h"

int isdigit(int ch)
{
       return (ch >= '0' && ch <= '9') ? 1 : 0;
}

char *itoa(char *buf, int data, int issigned, int printsign, int fmtwidth, int leftalign, int dividor, int zeropad,
		int upper)
{
	unsigned int udata, udata_tmp;
	int width, remainder, minus = 0, plus = 0;
	char *cp = buf;

	// option check
	if (fmtwidth == 0)
		leftalign = 0;

	// printsign
	if (printsign && ((issigned && data > 0) || (!issigned && data != 0))) {
		if (!zeropad)
			plus = 1;
		else
			*cp++ = '+';
	}
	// signed integer
	if (issigned && data < 0) {
		if (!zeropad)
			minus = 1;
		else
			*cp++ = '-';
		udata = (unsigned int)-data;
	} else
		udata = (unsigned int)data;

	// calculate actual width
	for (udata_tmp = udata, width = 1; udata_tmp >= dividor; udata_tmp /= (dividor == 16 ? 16 : 10), ++width) ;

	// sign and left padding
	if (!leftalign)
		while (fmtwidth-- > width)
			*cp++ = zeropad ? '0' : ' ';
	if (minus)
		*cp++ = '-';
	if (plus)
		*cp++ = '+';

	cp += width;

	// number
	do {
		remainder = udata % (dividor == 16 ? 16 : 10);
		*--cp = (remainder < 10) ? (remainder + '0') : (remainder - 10 + (upper ? 'A' : 'a'));
		udata /= (dividor == 16 ? 16 : 10);
	} while (udata != 0);

	cp += width;

	// right padding
	if (leftalign)
		while (fmtwidth-- > width)
			*cp++ = ' ';

	*cp = 0;

	return buf;
}

enum { STAT_NORMAL, STAT_FLAGWIDTH, STAT_CONVERSION };
enum { CONV_CHAR, CONV_INT, CONV_UINT, CONV_HEX_LOWER, CONV_HEX_UPPER,
	CONV_PTR, CONV_STRING
};

int vsprintf(char *buf, const char *fmt, va_list args)
{
	int stat = STAT_NORMAL;
	int zeropad = 0, leftalign = 0, printsign = 0, width = 0;
	int int_data;
	char *saved_buf = buf, *cp;

	for (; *fmt != 0; ++fmt) {
		switch (stat) {
			case STAT_NORMAL:
				if (*fmt == '%')
					stat = STAT_FLAGWIDTH;
				else
					*buf++ = *fmt;
				break;

			case STAT_FLAGWIDTH:
				// sign
				if (*fmt == '+') {
					++fmt;
					printsign = 1;
				} else
					printsign = 0;

				// left align
				if (*fmt == '-') {
					++fmt;
					leftalign = 1;
				} else
					leftalign = 0;

				// zero padding
				if (*fmt == '0') {
					zeropad = 1;
					++fmt;
				} else
					zeropad = 0;

				// width
				if (*fmt == '*') {
					width = va_arg(args, int);
				} else if (isdigit(*fmt)) {
					for (width = 0; isdigit(*fmt); ++fmt)
						width = width * 10 + *fmt - '0';
					--fmt;
				} else {
					width = 0;
					--fmt;
				}
				stat = STAT_CONVERSION;
				break;

			case STAT_CONVERSION:
				// length modifier
				if (*fmt == 'l')
					++fmt;

				// conversion directive
				switch (*fmt) {
					case 'c':
						int_data = va_arg(args, char);
						*buf++ = int_data;
						break;
					case 'p':
						zeropad = 1;
						width = sizeof(void *) * 2;
					case 'd':
					case 'u':
					case 'x':
					case 'X':
						int_data = va_arg(args, int);
						// integer
						itoa(buf, int_data,
								*fmt == 'd' ? 1 : 0, printsign, width, leftalign,
								(*fmt == 'x' || *fmt == 'X' || *fmt == 'p') ? 16 : 10,
								zeropad, *fmt == 'x' ? 0 : 1);
						while (*buf)
							++buf;
						break;
					case 's':
						cp = va_arg(args, char *);
						// right align
						if (!leftalign && width > 0) {
							width -= strlen(cp);
							while (width-- > 0)
								*buf++ = ' ';
						}
						// string
						while (*cp) {
							*buf++ = *cp++;
							if (width > 0)
								--width;
						}
						// left align
						if (leftalign && width > 0) {
							while (width-- > 0)
								*buf++ = ' ';
						}
						break;
					case '%':
						*buf++ = '%';
						break;
					default:
						// unknown conversion, ignore it!
						break;
				}
				stat = STAT_NORMAL;
				break;
		}
	}

	*buf = 0;

	return buf - saved_buf;
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	return i;
}
