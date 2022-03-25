#include "stdio.h"
#include "string.h"

unsigned int atoi(const char *str)
{
	const char *cp;
	unsigned int data = 0;

	if (str[0] == '0' && (str[1] == 'X' || str[1] == 'x'))
		return htoi(str + 2);

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			data = data * 10 + *cp - '0';
		else
			break;
	}

	return data;
}

unsigned int htoi(const char *str)
{
	const char *cp;
	unsigned int data = 0, bdata = 0;

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			bdata = *cp - '0';
		else if (*cp >= 'A' && *cp <= 'F')
			bdata = *cp - 'A' + 10;
		else if (*cp >= 'a' && *cp <= 'f')
			bdata = *cp - 'a' + 10;
		else
			break;
		data = (data << 4) | bdata;
	}

	return data;
}

#ifdef CFG_MBOX_DEBUG_PRINT
#include "sirius_mbox.h"
inline int putc(int ch)
{
	gx_fw_debug_print(ch);

	return 0;
}

void puts(const char *t)
{
    while (*t) putc(*t++);
}

int printf(const char *fmt, ...)
{
	int i;
	va_list args;
	char buf[1024];

	gx_fw_debug_print(MBOX_PRINT_START);

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);	/* hopefully i < sizeof(buf) */
	va_end(args);

	puts(buf);

	gx_fw_debug_print(MBOX_PRINT_END);
	return i;
}
#endif

#if 0
extern void serial_compatible_putc(int ch);
extern int serial_getc(void);
inline int putc(int ch)
{
	serial_compatible_putc(ch);

	return 0;
}

inline int getc(void)
{
	return serial_getc();
}

enum { GETSTR_ESCAPE = 0x01, GETSTR_APPEND = 0x02, GETSTR_APPENDDISP = 0x04 };
char *gets(char *str, int maxlen, int flag)
{
	int pos = 0;
	int ch;

	if (maxlen < 0)
		maxlen = 256;

	if (flag & GETSTR_APPEND) {
		while (str[pos] != 0) {
			if (flag & GETSTR_APPENDDISP)
				serial_compatible_putc(str[pos]);
			++pos;
		}
	}

	for (;;) {
		ch = serial_getc();

		// escape sequence (0x1b, ...)
		if (ch == 0x1b) {
			if (flag & GETSTR_ESCAPE) {
				str[pos] = 0;
				return NULL;
			}
		}

		if (ch == 0x7f)	// DEL
			ch = '\b';
		else if (ch == '\n')
			ch = '\r';

		if (ch != '\b')
			serial_compatible_putc(ch);
		if (ch == '\r') {
			serial_compatible_putc('\n');
			break;
		}
		else if (ch == '\b') {
			if (pos > 0) {
				--pos;
				serial_compatible_putc(ch);
				serial_compatible_putc(' ');
				serial_compatible_putc(ch);
			}
		}
		else {
			if (pos < maxlen - 2)
				str[pos++] = ch;
		}
	}

	str[pos] = 0;

	return str;
}

void puts(const char *t)
{
    while (*t) serial_compatible_putc(*t++);
}

void putnstr( const char* str, int n)
{
	while (n && *str != '\0') {
		serial_compatible_putc(*str) ;
		str++;
		n--;
	}
}

int printf(const char *fmt, ...)
{
	char buf[1024];

	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);	/* hopefully i < sizeof(buf) */
	va_end(args);

	puts(buf);

	return i;
}
#endif
